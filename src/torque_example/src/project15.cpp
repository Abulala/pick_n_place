/*
// KUKA Project 2015
*/

#include "ros/ros.h"
//#include "trajectory_generator/CStoCS.h"
//#include "trajectory_generator/Circle.h"
//#include "trajectory_msgs/JointTrajectoryPoint.h"
#include "Eigen/Dense"
#include <Eigen/Geometry>
#include <brics_actuator/JointPositions.h>
#include <brics_actuator/JointVelocities.h>
#include <geometry_msgs/Pose.h>
#include <iostream>
#include <boost/units/systems/si.hpp>
#include <torque_control/torque_trajectoryAction.h>
#include "ik_solver_service/SolveClosestIK.h"
#include "ik_solver_service/SolvePreferredTypeIK.h"
#include <torque_control/step.h>
//#include <actionlib/client/simple_action_client.h>
#include "rpg_youbot_common/rpg_youbot_common.h"

//#define USE_MULTI_TRAJ
#define USE_GRIPPER
//#define PICK_FROM_TOP
#define CAM_ACTIVE

#ifdef CAM_ACTIVE

  // includes necessary for ROS TF listener
  #include <tf/transform_listener.h>
  #include <find_object_2d/ObjectsStamped.h>
  #include <QtCore/QString>

#endif

using namespace std;

////// Coordinates of the robot:
 /// http://www.youbot-store.com/wiki/images/2/2b/YoubotCoordinates.png

/// youbot axis limits
 /// http://www.youbot-store.com/wiki/images/thumb/5/5f/KUKA_youBot_Arm_Dimensions.jpg/800px-KUKA_youBot_Arm_Dimensions.jpg


// Conversion function from RPY (Roll Pitch Yaw) to quaternion
//

Eigen::Quaternion<double> RPY2Quat(double roll, double pitch, double yaw){
  Eigen::AngleAxisd rollAngle(roll, Eigen::Vector3d::UnitZ());
  Eigen::AngleAxisd yawAngle(yaw, Eigen::Vector3d::UnitY());
  Eigen::AngleAxisd pitchAngle(pitch, Eigen::Vector3d::UnitX());

  return rollAngle * yawAngle * pitchAngle;

}

// Conversion functions from quaternion to Euler angles
// source: http://www.tinkerforge.com/en/doc/Software/Bricks/IMU_Brick_CSharp.html
//

double Quat2Xangle(Eigen::Quaternion<double> q){
 // Conversion from quaternion to x axis angle
  return atan2(2*q.y()*q.w() - 2*q.x()*q.z(), 1 - 2*q.y()*q.y() - 2*q.z()*q.z()) * 180/M_PI;
}
double Quat2Yangle(Eigen::Quaternion<double> q){
 // Conversion from quaternion to y axis angle
  return atan2(2*q.x()*q.w() - 2*q.y()*q.z(), 1 - 2*q.x()*q.x() - 2*q.z()*q.z()) * 180/M_PI;
}
double Quat2Zangle(Eigen::Quaternion<double> q){
 // Conversion from quaternion to z axis angle
  return asin (2*q.x()*q.y() + 2*q.z()*q.w()) * 180/M_PI;
}

// Conversion functions from quaternion to RPY (Roll Pitch Yaw)
// source: http://www.tinkerforge.com/en/doc/Software/Bricks/IMU_Brick_CSharp.html
//

double Quat2yaw(Eigen::Quaternion<double> q){
  // Conversion from quaternion to yaw angle
  return atan2(2*q.x()*q.y() + 2*q.w()*q.z(), q.w()*q.w() + q.x()*q.x() - q.y()*q.y() - q.z()*q.z()) * 180/M_PI;
}
double Quat2pitch(Eigen::Quaternion<double> q){
  // Conversion from quaternion to pitch angle
  return -asin (2*q.w()*q.y() - 2*q.x()*q.z()) * 180/M_PI;
}
double Quat2roll(Eigen::Quaternion<double> q){
  // Conversion from quaternion to roll angle
  return -atan2(2*q.y()*q.z() + 2*q.w()*q.x(), -q.w()*q.w() + q.x()*q.x() + q.y()*q.y() - q.z()*q.z()) * 180/M_PI;
}

////////////////////////////////////////////////

// Function to wait for user input before continuing the execution
//

bool wait4enterkey(string echo_str){
    cout << echo_str << endl;
    //cout << "PRESS ENTER KEY TO EXECUTE PICK UP PROCESS.";
    if(cin.get() == '\n'){
    	return true;
    }else{
    	//cout << "YOU HIT A RANDOM KEY -> EXITING PROGRAM.";
    	return false;
    }
}


#ifdef CAM_ACTIVE
bool object_detected = false;
float objectQuat[7]; // 7:  pos_x, pos_y, pos_z, orient_x, orient_y, orient_z, orient_w


// Function: TF listener to gather object information (Position + Orientation) from find_object2d package
class TfExample
{
public:
	TfExample() :
		mapFrameId_("/map"),
		objFramePrefix_("object")
	{
		ros::NodeHandle pnh("~");
		pnh.param("map_frame_id", mapFrameId_, mapFrameId_);
		pnh.param("object_prefix", objFramePrefix_, objFramePrefix_);

		ros::NodeHandle nh;
		subs_ = nh.subscribe("objectsStamped", 1, &TfExample::objectsDetectedCallback, this);
	}

	void objectsDetectedCallback(const find_object_2d::ObjectsStampedConstPtr & msg)
	{
		if(msg->objects.data.size())
		{
			for(unsigned int i=0; i<msg->objects.data.size(); i+=12)
			{
				// get data
				int id = (int)msg->objects.data[i];
				std::string objectFrameId = QString("%1_%2").arg(objFramePrefix_.c_str()).arg(id).toStdString(); // "object_1", "object_2"

				tf::StampedTransform pose;
				try
				{
					// Get transformation from "object_#" frame to target frame "map"
					// The timestamp matches the one sent over TF
					tfListener_.lookupTransform(mapFrameId_, objectFrameId, msg->header.stamp, pose);
				}
				catch(tf::TransformException & ex){
					// if no proper information received -> continue and do not start with motion
					// ROS_WARN("%s",ex.what()); // may be activated to print warnings.
					continue;
				}

				// Proper pose (position + orientation) information received -> save gathered information in public variable.
				objectQuat[0] = pose.getOrigin().x();
				objectQuat[1] = pose.getOrigin().y();
				objectQuat[2] = pose.getOrigin().z();
				objectQuat[3] = pose.getRotation().x();
				objectQuat[4] = pose.getRotation().y();
				objectQuat[5] = pose.getRotation().z();
				objectQuat[6] = pose.getRotation().w();

				// set object_detected flag to break ros spin
				object_detected = true;

				// Print information (position + orientation) about detected object 
				ROS_INFO("OBJECT DETECTED: position[%.2f, %.2f, %.2f] orientation[%.4f, %.4f, %.4f, %.4f]", objectQuat[0], objectQuat[1], objectQuat[2], objectQuat[3], objectQuat[4], objectQuat[5], objectQuat[6]);
				//printf("OBJECT DETECTED: position[%.2f, %.2f, %.2f] orientation[%.4f, %.4f, %.4f, %.4f]", objectQuat[0], objectQuat[1], objectQuat[2], objectQuat[3], objectQuat[4], objectQuat[5], objectQuat[6]);

			}
		}
	}

private:
	std::string mapFrameId_;
	std::string objFramePrefix_;
	ros::Subscriber subs_;
	tf::TransformListener tfListener_;

};

#endif

int main(int argc, char **argv)
{
  ros::init(argc, argv, "prj15_node");
  
  #ifdef CAM_ACTIVE
  TfExample sync;
  while(!object_detected) { ros::spinOnce(); }
  #endif
  
  ros::NodeHandle nh;

  ros::Rate lr(10);
  
  // publisher for arm joint positions
  ros::Publisher arm_pub_pos = nh.advertise<brics_actuator::JointPositions > ("/arm_1/arm_controller/position_command", 1);

  // publisher for (left and right) gripper positions
  ros::Publisher gripper_pub_pos = nh.advertise<brics_actuator::JointPositions > ("arm_1/gripper_controller/position_command", 1);

  #ifdef USE_GRIPPER
  brics_actuator::JointPositions command;
  vector <brics_actuator::JointValue> gripperJointPositions;

  static const int numberOfGripperJoints = 2;
  gripperJointPositions.resize(numberOfGripperJoints);
  #endif

  bool feasible = true; // false
  geometry_msgs::Pose init_p, start_p, low_p, end_p, home_p, plate_p, straight_p;

  // home position: final position of arm - arm folded
  double home_joints[5] = {0.0204, 0.0609, -0.1547, 0.2797, 1.4708};
  home_p.position.x = 0.0; // orig in base_link frame: 0.166
  home_p.position.y = 0.0; // orig in base_link frame: 0.000
  home_p.position.z = 0.48; // orig in base_link frame: 0.396
  Eigen::Quaterniond home_grip(-0.0643, 0.0694, -0.805, 0.5857); // RPY[ -0.1881 -0.0222 -1.8814 ]
  home_p.orientation.x = home_grip.x();
  home_p.orientation.y = home_grip.y();
  home_p.orientation.z = home_grip.z();
  home_p.orientation.w = home_grip.w();

  // straight position: temporary position to avoid collisions - arm completly unfolded
  double straight_joints[5] = {3.0, 0.92, -2.6, 1.7, 3.0};
  straight_p.position.x = -0.15; // orig in base_link frame: 0.086
  straight_p.position.y = 0.0; // orig in base_link frame: 0.0027
  straight_p.position.z = 0.64; // orig in base_link frame: 0.5643
  Eigen::Quaterniond straight_grip(0.0038, -0.1747, -0.0539, 0.9831); // RPY[ 0.028 -0.35 -0.115 ]
  straight_p.orientation.x = straight_grip.x();
  straight_p.orientation.y = straight_grip.y();
  straight_p.orientation.z = straight_grip.z();
  straight_p.orientation.w = straight_grip.w();

  // plate position: place picked up object here.
  double plate_joints[5] = {3.0, 0.8, -3.3, 0.5, 3.0};
  plate_p.position.x = -0.31; // orig in base_link frame: -0.62
  plate_p.position.y = 0.0; // orig in base_link frame: 0.0075
  plate_p.position.z = 0.19; // orig in base_link frame: 0.2782
  Eigen::Quaterniond plate_grip(0.018, -0.921, -0.0205, 0.3886); // RPY[ 3.068 -0.7967 -3.071 ]
  plate_p.orientation.x = plate_grip.x();
  plate_p.orientation.y = plate_grip.y();
  plate_p.orientation.z = plate_grip.z();
  plate_p.orientation.w = plate_grip.w();

  // inverse kinematics solver service with desired gripper orientation
  ros::ServiceClient solve_preferred_type_ik_client = nh.serviceClient<ik_solver_service::SolvePreferredTypeIK>("solve_preferred_type_ik");
  ik_solver_service::SolvePreferredTypeIK pickup;
  ik_solver_service::SolvePreferredTypeIK pickup_low;

  // define pickup position above detected object 
  pickup.request.arm_to_front = false;
  pickup.request.arm_bended_up = false;
  pickup.request.gripper_downwards = true;
  pickup.request.des_position[0] = objectQuat[0]; // detected object x coordinate (unit: cm)
  pickup.request.des_position[1] = objectQuat[1]; // detected object y coordinate (unit: cm)
  pickup.request.des_position[2] = 0.015; // manually set to ensure correct plane. objectQuat[2] may also be used from detected object information
  
  Eigen::Quaternion<double> pickup_quat(objectQuat[3], objectQuat[4], objectQuat[5], objectQuat[6]);
  double pickup_yaw = Quat2yaw(pickup_quat);  
  
  printf("\nObject orientation: RPY[%.4f° %.4f° %.4f°]\n",Quat2roll(pickup_quat), Quat2pitch(pickup_quat), Quat2yaw(pickup_quat));  
  
  pickup.request.des_normal[0] = (pickup_yaw-90)/180*M_PI*10.0; // TO BE DONE: objectQuat[3..6] convert to RPY and use yaw information for rotation of gripper
//  pickup.request.des_normal[0] = 0.0; // TO BE DONE: objectQuat[3..6] convert to RPY and use yaw information for rotation of gripper
  pickup.request.des_normal[1] = 1.0; // 1.0
  pickup.request.des_normal[2] = 0.0; // 0.0

  // define pickup position at object - same position but 3cm deeper
  pickup_low.request.arm_to_front = false;
  pickup_low.request.arm_bended_up = false;
  pickup_low.request.gripper_downwards = true;
  pickup_low.request.des_position[0] = pickup.request.des_position[0]; // objectQuat[0]
  pickup_low.request.des_position[1] = pickup.request.des_position[1]; //objectQuat[1]
  pickup_low.request.des_position[2] = pickup.request.des_position[2]-0.03; // 3cm deeper
  pickup_low.request.des_normal[0] = pickup.request.des_normal[0];
  pickup_low.request.des_normal[1] = pickup.request.des_normal[1];
  pickup_low.request.des_normal[2] = pickup.request.des_normal[2];


  // Calculating joint coordinates for room coordinates gathered from camera
  //

  double pickup_joints[5];
  double pickup_low_joints[5];
  
  //ROS_INFO("Starting desired inverse kinematics calculations for pickup and pickup_low.");
  if (solve_preferred_type_ik_client.call(pickup)){
    for (int j = 0; j < 1; j++){
    	// In case multiple solutions should be handled this loop can be changed.
    	
      for (int i = 0; i < 5; i++){
        // saving calculated joint positions in new variable.
        pickup_joints[i] = pickup.response.joint_angles[i];
        //printf("%f\t", pickup_joints[i]);
      }
      
      printf("\nfeasible: %d arm_to_front: %d arm_bended_up: %d gripper_downwards: %d\n\n",
            pickup.response.feasible, pickup.response.arm_to_front, pickup.response.arm_bended_up, pickup.response.gripper_downwards);      
      
      if(pickup.response.gripper_downwards > 0)
      	ROS_INFO("Joint coordinates properly gained (1/2).");
      else{
      	ROS_ERROR("Feasible coordinates available but object may not be picked from above. (1/2)");
      	feasible = false;
      }
    }
  }else{
    ROS_ERROR("No feasible coordinates available. Possibly object out of range. (1/2)");
    feasible = false;
    return 1;
  }

  if (solve_preferred_type_ik_client.call(pickup_low)){
    for (int j = 0; j < 1; j++){
    	// In case multiple solutions should be handled this loop can be changed.
    	
      for (int i = 0; i < 5; i++){
        // saving calculated joint positions in new variable.
        pickup_low_joints[i] = pickup_low.response.joint_angles[i];
        //printf("%f\t", pickup_low_joints[i]);
      }
      printf("\nfeasible: %d arm_to_front: %d arm_bended_up: %d gripper_downwards: %d\n\n",
            pickup_low.response.feasible, pickup_low.response.arm_to_front, pickup_low.response.arm_bended_up, pickup_low.response.gripper_downwards);      
      
      if(pickup.response.gripper_downwards > 0)
	      ROS_INFO("Joint coordinates properly gained (2/2).");
      else{
      	ROS_ERROR("Feasible coordinates available but object may not be picked from above. (2/2)");
      	feasible = false;
      }
    }
  }else{
    ROS_ERROR("No feasible coordinates available. Possibly object out of range. (2/2)");
    feasible = false;
    return 1;
  }


///////////////////////////
  if(feasible){
    // declarations
    //bool finished_before_timeout;
    //torque_control::torque_trajectoryGoal goal;
#ifdef USE_GRIPPER
    gripperJointPositions[0].joint_uri = "gripper_finger_joint_l";
    gripperJointPositions[1].joint_uri = "gripper_finger_joint_r";
    gripperJointPositions[0].unit = boost::units::to_string(boost::units::si::meter);
    gripperJointPositions[1].unit = boost::units::to_string(boost::units::si::meter);
    float gripper_position = 0.0; // in meters - max val: 0.022 - min val: 0.0
#endif

    /////////////// motion begin

    if(!wait4enterkey("PRESS ENTER KEY TO EXECUTE MOVEMENT TO STRAIGHT.")) {cout << "YOU HIT A RANDOM KEY -> EXITING PROGRAM." << endl; return 1;}
    
    //// executing movement to straight   
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(straight_joints)); ros::spinOnce(); sleep(0.5);
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(straight_joints)); ros::spinOnce(); sleep(0.5);
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(straight_joints)); ros::spinOnce(); sleep(1.0);

    if(!wait4enterkey("PRESS ENTER KEY TO EXECUTE MOVEMENT TO OBJECT.")) {cout << "YOU HIT A RANDOM KEY -> EXITING PROGRAM."; return 1;}
    
    //// executing movement to detected object
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(pickup_joints)); ros::spinOnce(); sleep(0.5);
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(pickup_joints)); ros::spinOnce(); sleep(0.5);
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(pickup_joints)); ros::spinOnce(); sleep(1.0);

    if(!wait4enterkey("PRESS ENTER KEY TO EXECUTE PICK UP PROCESS.")) {cout << "YOU HIT A RANDOM KEY -> EXITING PROGRAM." << endl; return 1;}

    //// Gripper open
#ifdef USE_GRIPPER2
    gripper_pub_pos.publish(rpg_youbot_common::generateGripperPositionMsg(0.011,0.011)); //0.011, 0.011 is the maximum possible gripping width
    sleep(3); // wait until gripping process is done.
#endif
#ifdef USE_GRIPPER
    gripper_position = 0.022; // in meters - max val: 0.022 - min val: 0.0
    gripperJointPositions[0].value = gripper_position/2;
    gripperJointPositions[1].value = gripper_position/2;

    command.positions = gripperJointPositions;
    gripper_pub_pos.publish(command);

    sleep(3); // wait until gripping process is done.
#endif

    if(!wait4enterkey("PRESS ENTER KEY TO EXECUTE MOVEMENT TO OBJECT.")) {cout << "YOU HIT A RANDOM KEY -> EXITING PROGRAM."; return 1;}
    
    //// executing movement to detected object - grasping position
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(pickup_low_joints)); ros::spinOnce(); sleep(0.5);
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(pickup_low_joints)); ros::spinOnce(); sleep(0.5);
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(pickup_low_joints)); ros::spinOnce(); sleep(1.0);

    //// Gripper close
#ifdef USE_GRIPPER2
  gripper_pub_pos.publish(rpg_youbot_common::generateGripperPositionMsg(0.000,0.000)); // 0.0, 0.0: close as narrow as possible
  sleep(3); // wait until gripping process is done.
#endif
#ifdef USE_GRIPPER
    gripper_position = 0.00; // in meters - max val: 0.022 - min val: 0.0
    gripperJointPositions[0].value = gripper_position/2;
    gripperJointPositions[1].value = gripper_position/2;

    command.positions = gripperJointPositions;
    gripper_pub_pos.publish(command);

    sleep(3); // wait until gripping process is done.
#endif
    
    if(!wait4enterkey("PRESS ENTER KEY TO EXECUTE MOVEMENT TO OBJECT.")) {cout << "YOU HIT A RANDOM KEY -> EXITING PROGRAM."; return 1;}
    
    //// executing movement to same position as before - but with the gripped object
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(pickup_joints)); ros::spinOnce(); sleep(0.5);
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(pickup_joints)); ros::spinOnce(); sleep(0.5);
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(pickup_joints)); ros::spinOnce(); sleep(1.0);


    if(!wait4enterkey("PRESS ENTER KEY TO EXECUTE MOVEMENT TO STRAIGHT.")) {cout << "YOU HIT A RANDOM KEY -> EXITING PROGRAM." << endl; return 1;}

    //// executing movement to straight   
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(straight_joints)); ros::spinOnce(); sleep(0.5);
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(straight_joints)); ros::spinOnce(); sleep(0.5);
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(straight_joints)); ros::spinOnce(); sleep(1.0);

    if(!wait4enterkey("PRESS ENTER KEY TO EXECUTE MOVEMENT TO PLATE.")) {cout << "YOU HIT A RANDOM KEY -> EXITING PROGRAM." << endl; return 1;}

    //// executing movement to plate
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(plate_joints)); ros::spinOnce(); sleep(0.5);
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(plate_joints)); ros::spinOnce(); sleep(0.5);
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(plate_joints)); ros::spinOnce(); sleep(1.0);

    if(!wait4enterkey("PRESS ENTER KEY TO OPEN GRIPPER.")) {cout << "YOU HIT A RANDOM KEY -> EXITING PROGRAM." << endl; return 1;}

    //// Gripper open
#ifdef USE_GRIPPER2
  gripper_pub_pos.publish(rpg_youbot_common::generateGripperPositionMsg(0.011,0.011));
  sleep(3); // wait until gripping process is done.
#endif

#ifdef USE_GRIPPER
    gripper_position = 0.022; // in meters - max val: 0.022 - min val: 0.0
    gripperJointPositions[0].value = gripper_position/2;
    gripperJointPositions[1].value = gripper_position/2;

    command.positions = gripperJointPositions;
    gripper_pub_pos.publish(command);

    sleep(3); // wait until gripping process is done.
#endif

    if(!wait4enterkey("PRESS ENTER KEY TO EXECUTE MOVEMENT TO HOME.")) {cout << "YOU HIT A RANDOM KEY -> EXITING PROGRAM." << endl; return 1;}

    //// executing movement to straight   
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(straight_joints)); ros::spinOnce(); sleep(0.5);
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(straight_joints)); ros::spinOnce(); sleep(0.5);
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(straight_joints)); ros::spinOnce(); sleep(1.0);
   
    //// executing movement to home   
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(home_joints)); ros::spinOnce(); sleep(0.5);
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(home_joints)); ros::spinOnce(); sleep(0.5);
    arm_pub_pos.publish(rpg_youbot_common::generateJointPositionMsg(home_joints)); ros::spinOnce(); sleep(1.0);

    //// Gripper open
#ifdef USE_GRIPPER2
    gripper_pub_pos.publish(rpg_youbot_common::generateGripperPositionMsg(0.00,0.00));
    sleep(3); // wait until gripping process is done.
#endif
#ifdef USE_GRIPPER
    gripper_position = 0.00; // in meters - max val: 0.022 - min val: 0.0
    gripperJointPositions[0].value = gripper_position/2;
    gripperJointPositions[1].value = gripper_position/2;

    command.positions = gripperJointPositions;
    gripper_pub_pos.publish(command);

    sleep(3); // wait until gripping process is done.
#endif
  }

  return 0;
}
