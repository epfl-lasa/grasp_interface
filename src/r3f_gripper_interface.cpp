// Copyright (c) 2016, The University of Texas at Austin
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDEr3f AND CONTRIBUTOr3f "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTOr3f BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "grasp_interface/r3f_gripper_interface.h"

std::vector<std::string> r3fGripperInterface::fingerNames
  {"robotiq_finger_1_link_0", "robotiq_finger_1_link_1", "robotiq_finger_1_link_2", "robotiq_finger_1_link_3",
  "robotiq_finger_2_link_0", "robotiq_finger_2_link_1", "robotiq_finger_2_link_2", "robotiq_finger_2_link_3",
  "robotiq_finger_middle_link_0", "robotiq_finger_middle_link_1", "robotiq_finger_middle_link_2", "robotiq_finger_middle_link_3"};

r3fGripperInterface::r3fGripperInterface(bool _sim):
  n(),
  spinner(2),
  command(),
  status(),
  sim(_sim)
{
  spinner.start();
  // In case commands are sent via a ROS topic:
  gripperCommandSub = n.subscribe("grip_command",1, &r3fGripperInterface::cb_command,this);

  if(sim) {
    ROS_INFO("[r3fGripperInterface] Connecting in simulation mode.");
    connected = true;
    block = false; // make calls finish instantly
  } else {

    gripperCommandPub = n.advertise<robotiq_3f_gripper_control::Robotiq3FGripper_robot_output>("r3fModelRobotOutput",1);
    gripperStatusSub = n.subscribe("r3fModelRobotInput",1,&r3fGripperInterface::cb_getGripperStatus,this);

    //command.rICF = 1; //command fingers separately, always
    //gripperCommandPub.publish(command);

    float printTime = 10, retryTime = 0.1;
    ros::Time start = ros::Time::now();
    while(
      (gripperCommandPub.getNumSubscribers() <= 0 || messagesReceived_ == 0) && // wait for connection
      !ros::isShuttingDown())                                             // but stop if ROS shuts down
    {
      ROS_INFO_STREAM_THROTTLE(printTime, "[r3fGripperInterface] Waiting for connection with gripper (" << (ros::Time::now() - start) << "s elasped)");
      ros::Duration(retryTime).sleep();
    }
    if(!ros::isShuttingDown()) {
      //ros::Duration(0.5).sleep();  // give a chance to receive the current gripepr status
      ROS_INFO("[r3fGripperInterface] Connected to gripper");
      connected = true;

      //check for pre-activation
      if(status.gIMC == 3) {
        activated = true;

        // set internal position/mode to actual gripper position/mode
        command.rICF = 1;
        command.rPRA = status.gPOA;
        command.rPRB = status.gPOB;
        command.rPRC = status.gPOC;
        command.rPRS = status.gPRS;
        command.rMOD = status.gMOD;
        ROS_DEBUG_STREAM("[r3fGripperInterface] Gripper already activated, connected with mode " << ((int)status.gMOD));
      }
    }
  }
}

void r3fGripperInterface::sendCommand() {
  if(!isConnected()) {
    ROS_ERROR("[r3fGripperInterface] Can't control the gripper, it's not connected or activated yet.");
    return;
  }
  //command.rACT //Shows if the gripper has been activated
  //command.rMOD //Gripper Modes. 0 = Basic, 1 = pinch, 2 = wide, 3 = scissor
  //command.rGTO //1 = all fingers go to commanded positions
  //command.rATR //Automatic Release routine. Used for E-Stop routines.
  //command.rGLV //Glove Mode option. MUST USE to use robotiq glove
  //command.rICF //Individual Finger Control. 0 is off
  //command.rICS //Individual control of Scissor
  //command.rPRA //Commanded Position of Finger A
  //command.rSPA //Speed of Finger A
  //command.rFRA //Force of Finger A
  //command.rPRB //Commanded Position of Finger A
  //command.rSPB //Speed of Finger B
  //command.rFRB //Force of Finger B
  //command.rPRC //Commanded Position of Finger A
  //command.rSPC //Speed of Finger C
  //command.rFRC //Force of Finger C
  //command.rPr3f //Scissor Position Request
  //command.rSPS //Speed for Scissor
  //command.rFr3f //Force for Scissor
  if(!activated) {
    ROS_ERROR("[r3fGripperInterface] Can't control the gripper, it's not activated yet. Call activate()");
    return;
  }

  if(sim) {
    return;
  }
  gripperCommandPub.publish(command);
  return;
}

void r3fGripperInterface::eStop() {
  ROS_FATAL("[r3fGripperInterface] Beginning soft r3f gripper E-stop");
  command.rATR = 1;
  sendCommand();
  if(block) {
    while(status.gFLT < 13) {
      ROS_WARN_DELAYED_THROTTLE(1, "Waiting for E-Stop to complete");
    }
  }
  ROS_FATAL("[r3fGripperInterface] Finished soft r3f gripper E-stop");
}


void r3fGripperInterface::reset()
{
  ROS_DEBUG("[r3fGripperInterface] Beginning reset");
  deactivate();
  command = robotiq_3f_gripper_control::Robotiq3FGripper_robot_output();
  activate();
  ROS_DEBUG("[r3fGripperInterface] Finished reset");
}

void r3fGripperInterface::deactivate()
{
  ROS_INFO("[r3fGripperInterface] Deactivating");
  command.rACT = 0;
  sendCommand();
  if(block) {
    while(status.gACT != 0) {
      ROS_WARN_DELAYED_THROTTLE(5, "[r3fGripperInterface] Waiting for gripper to turn off...");
    }
    ROS_DEBUG("[r3fGripperInterface] Finished dectivation");
  }
  activated = false;
}

void r3fGripperInterface::activate()
{
  ROS_INFO("[r3fGripperInterface] Activating");
  // if(activated)
  //   return;
  if(!sim) { // because we aren't calling sendCommand we need an explicit check for simulation here
    command.rACT = 1; //do the activation
    //don't call sendCommand, because sendCommand includes an activation check. Instead
    // just manually send the command
    gripperCommandPub.publish(command);
    if(block) {
      while(status.gIMC != 3) {
  ROS_INFO_DELAYED_THROTTLE(10, "[r3fGripperInterface] Waiting for activation to complete...");
      }
      ROS_DEBUG("[r3fGripperInterface] Finished activation");
    }
  }
  activated = true;
}

void r3fGripperInterface::setMode(Mode newMode)
{
  int modeNum = static_cast<int>(newMode);
  setMode(modeNum);
}

void r3fGripperInterface::setMode(int newMode)
{
  if(newMode == 3) {
    ROS_ERROR("[r3fGripperInterface] Scissor mode is not supported, ignoring mode switch command.");
  }
  if(newMode > 3 || newMode < 0) {
    ROS_ERROR_STREAM("[r3fGripperInterface] Attemped to set gripper mode " << newMode <<
      ", which is outside acceptable range (0,1,2,3). Remaining in current mode, which is " << (int)status.gMOD);
    return;
  }
  ROS_DEBUG_STREAM("[r3fGripperInterface] Setting gripper mode to " << (int)newMode);
  command.rMOD = newMode;

  //do not return the fingers to their positions after changing mode
  command.rPRA = 0;
  command.rPRB = 0;
  command.rPRC = 0;
  sendCommand();

  if(block) {
    while(status.gMOD != newMode) {
      ROS_WARN_DELAYED_THROTTLE(5, "[r3fGripperInterface] Waiting for mode change to echo...");
      //wait for mode to be echoed
    }
    while(status.gIMC != 3) {
      ROS_INFO_DELAYED_THROTTLE(5, "[r3fGripperInterface] Waiting for mode change to complete...");
    }
  }
}

void r3fGripperInterface::home()
{
  setSpeed(255);
  setForce(128);
  setMode(0);
  setPosition(0);
}

void r3fGripperInterface::fullOpen()
{
  ROS_INFO("[r3fGripperInterface] Opening gripper");
  setPosition(0);
}

void r3fGripperInterface::fullClose()
{
  ROS_INFO("[r3fGripperInterface] Closing gripper");
  switch(command.rMOD) {
    case 0: //basic
      setPosition(255);
      break;
    case 1: //pinch
      setPosition(113);
      break;
    case 2: //wide TODO
    case 3: //scissor TODO
    default:
      ROS_ERROR_STREAM("[r3fGripperInterface] fullClose isn't implemented yet for mode " << (int)command.rMOD << ". Please use setPosition() instead");
      break;
  }
}


void r3fGripperInterface::setPosition(int position) {
  clampByte(position, "finger position"); //to avoid triple print out in three-position call
  ROS_DEBUG_STREAM("[r3fGripperInterface] Moving all fingers to position " << position);
  setPosition(position, position, position);
}

void r3fGripperInterface::setPosition(int positionA, int positionB, int positionC)
{
  clampByte(positionA, "finger A position");
  clampByte(positionB, "finger B position");
  clampByte(positionC, "finger C position");

  ROS_DEBUG_STREAM("[r3fGripperInterface] Moving fingers to position " << positionA << ", " << positionB << ", " << positionC);

  switch(command.rMOD) {
    case 0:
      //any position is good
      break;
    case 1: //pinch
      //According to Matt, any position is good. The gripper understands what mode it's in
      break;
    case 2: //wide TODO
      ROS_ERROR_STREAM("[r3fGripperInterface] setPosition() isn't implemented yet for wide mode.");
      break;
    case 3: //scissor TODO
      ROS_ERROR_STREAM("[r3fGripperInterface] setPosition() isn't implemented yet for scissor mode.");
      break;
    default:
      ROS_WARN_STREAM("[r3fGripperInterface] Finger limits haven't been determined for mode " << (int)command.rMOD << ". The fingers may never reach their final positions.");
      break;
  }

  command.rPRA = positionA;
  command.rPRB = positionB;
  command.rPRC = positionC;
  command.rGTO = 1;
  sendCommand();

  //wait for position message to post
  if(block) {
    if(status.gPRA != positionA || status.gPRB != positionB || status.gPRC != positionC) {
      while(status.gDTA != 0 && status.gDTB != 0 && status.gDTC != 0 && status.gDTS != 0) {
        ROS_INFO_THROTTLE(5, "[r3fGripperInterface] Waiting for move to begin...");
      }
    }
    while(status.gDTA == 0 || status.gDTB == 0 || status.gDTC == 0 || status.gDTS == 0) {
      ROS_INFO_THROTTLE(5, "[r3fGripperInterface] Waiting for move to complete...");
    }
  }
}

void r3fGripperInterface::setSpeed(int speed)
{
  clampByte(speed, "finger speed");
  ROS_DEBUG_STREAM("[r3fGripperInterface] Setting all fingers to speed " << speed);
  command.rSPA = speed;
  command.rSPB = speed;
  command.rSPC = speed;
}

void r3fGripperInterface::setForce(int force)
{
  clampByte(force, "finger force");
  ROS_DEBUG_STREAM("[r3fGripperInterface] Setting all fingers to force " << force);
  command.rFRA = force;
  command.rFRB = force;
  command.rFRC = force;
}


bool r3fGripperInterface::isObjectHeld() {
  if(sim) return true;
  if(block) {
    while(status.gSTA == 0) {
      ROS_INFO_THROTTLE(5, "[r3fGripperInterface] Waiting for move to complete...");
    }
  }
  return status.gSTA == 1 || status.gSTA == 2;
}

void r3fGripperInterface::clampByte(int& toClamp, std::string name) {
  if(toClamp < 0) {
    ROS_WARN_STREAM("[r3fGripperInterface] received " << name << " " << toClamp << ". This will be clamped to 0");
    toClamp = 0;
  }
  else if(toClamp > 255) {
    ROS_WARN_STREAM("[r3fGripperInterface] received " << name << " " << toClamp << ". This will be clamped to 255");
    toClamp = 255;
  }
}

void r3fGripperInterface::cb_getGripperStatus(const robotiq_3f_gripper_control::Robotiq3FGripper_robot_input& msg)
{
  messagesReceived_++;
  status = msg;
  //msg.gACT;  // 0 = reset, 1 = activation
  //msg.gMOD;  // Mode. 0 = basic, 1 = pinch, 2 = wide, 3 = scissor
  //msg.gGTO;  // 1 = going to position, 0 = doing something else
  //msg.gIMC;  // 0 = gripper in reset state, 1 = activating, 2 = changing mode, 3 = activation/mode change complete
  //msg.gSTA;  // 0 = moving to position (with gGTO=1), 1=gripper stopped, at least one finger completed move, 2 = no fingers completed, 3 = all fingers completed

  //msg.gDTA;  // 0 = finger A in motion (with gGTO=1), 1 = finger A stopped due to contact while open, 2 = finger A stopped due to contact while closing, 3 = finger A at position
  //msg.gDTB;  // same for finger b
  //msg.gDTC;  // same for finger c
  //msg.gDTS;  // same, but for scissor move only

  //msg.gFLT;  // Fault indicator
      // 0 - no fault

      // priority
      // 5 - action delayed, wait for activation to finish
      // 6 - action delayed, wait for mode change to finish
      // 7 - not activated

      // minor
      // 9 - communication chip not ready (may be booting)
      // 10 - changing mode fault, interference on scissor (<20sec)
      // 11 - automatic release in progress

      // major - reset required
      // 13 - activation fault
      // 14 - fault changing mode, interference detected on scissor (>20sec)
      // 15 - auto release completed. reset and activation required

  //msg.gPRA; //Position request of Finger A (echoed command)
  //msg.gPOA; // Position of Finger A. 0 = open, 255 = closed
  //msg.gCUA; //Finger A current

  //msg.gPRB; //finger B position request
  //msg.gPOB; //finger B position
  //msg.gCUB; //finger B current

  //msg.gPRC; //finger C position request
  //msg.gPOC; //finger C position
  //msg.gCUC; //finger C current

  //msg.gPr3f; //scissor position request
  //msg.gPOS; //scissor position
  //msg.gCUS; //scissor current

  switch(status.gFLT) {
    case 0:
      break;
    case 5:
      ROS_WARN_THROTTLE(1, "[r3fGripperInterface] Activation is not complete!");
      break;
    case 6:
      ROS_WARN_THROTTLE(1, "[r3fGripperInterface] Mode change is not complete!");
      break;
    case 7:
      ROS_WARN_THROTTLE(1, "[r3fGripperInterface] Gripper is not activated yet!");
      break;
    case 9:
    case 10:
    case 11:
      ROS_ERROR_STREAM_THROTTLE(10, "[r3fGripperInterface] Minor fault detected, #" << (int)status.gFLT);
      break;
    case 13:
    case 14:
    case 15:
      ROS_FATAL_STREAM_THROTTLE(10, "[r3fGripperInterface] Major fault detected, #" << (int)status.gFLT);
      break;
    default:
      break;
  }
}

// Process incoming ROS commands
void r3fGripperInterface::cb_command(const grasp_interface::r3fGripperCommand& alpha)
{
  if (activated != true)
    activate();

  //order is important
  setSpeed(alpha.speed);
  setForce(alpha.force);
  setMode(alpha.mode);
  setPosition(alpha.position);
}

bool r3fGripperInterface::isConnected() {
  if(!connected) {
    ROS_WARN_THROTTLE(2.0, "[r3fGripperInterface]: Not connected!");
    return false;
  }
  return true;
}
