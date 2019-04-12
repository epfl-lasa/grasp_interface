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
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <ros/ros.h>
#include <iostream>
#include <grasp_interface/r2f_gripper_interface.h>

int main(int argc, char **argv)
{
  // Initialize the ros grab_interface_node
  ros::init(argc, argv, "r2f_gripper_interface_node");
  
  r2fGripperInterface r = r2fGripperInterface();

  ROS_INFO("[r2fGripperInterfaceTest] activating");
  r.activate();

  // return 1;
  
  //temp tests/////////////////////////////
  
  ros::Duration(2.0).sleep();
  
  //full functionality/////////////////////

  
  ROS_INFO("[r2fGripperInterfaceTest] fullClose");
  //slow close
  r.home();
  r.setSpeed(-1);
  r.fullClose();
  ros::Duration(2.0).sleep();
  
  //fast open
  ROS_INFO("[r2fGripperInterfaceTest] fullOpen");

  r.setSpeed(-1);
  r.fullOpen();
  ros::Duration(2.0).sleep();
  
  ROS_INFO("[r2fGripperInterfaceTest] close a bit low speed");

  r.setSpeed(128);
  r.setPosition(128);
  ros::Duration(2.0).sleep();

  ROS_INFO("[r2fGripperInterfaceTest] fullClose low speed");
  r.setSpeed(128);
  r.setPosition(255);
  ros::Duration(2.0).sleep();

  ROS_INFO("[r2fGripperInterfaceTest] fullOpen high speed");
  r.setSpeed(-1);
  r.setPosition(0);
  ros::Duration(2.0).sleep();

	ROS_INFO("[r2fGripperInterfaceTest] resetting");
  r.reset();

  ros::waitForShutdown();
  return 0;
}
