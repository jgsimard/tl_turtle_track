#include <ros/ros.h>
#include <tf/tf.h>
#include <tf/transform_listener.h>
#include <time.h>
#include <geometry_msgs/Pose2D.h>
#include "tl_turtle_track/pose.h"
#include <angles/angles.h>
#include <cmath>
#include <geometry_msgs/Twist.h>

#define BASE_TF "turtle1"
#define WORLD_TF "world"

bool transformPose2D(const geometry_msgs::Pose2D& pose_src,
		     std::string source_frame,
		     geometry_msgs::Pose2D& pose_dst,
		     std::string target_frame) {
  
  ros::Time now = ros::Time(0);

  // Build up a geometry_msgs::PoseStamped from a geometry_msgs::Pose2D
  geometry_msgs::PoseStamped pose_src_stamped;
  pose_src_stamped.header.seq = 0;
  pose_src_stamped.header.stamp = now;
  pose_src_stamped.header.frame_id = source_frame;
  pose_src_stamped.pose.position.x = pose_src.x;
  pose_src_stamped.pose.position.y = pose_src.y;
  pose_src_stamped.pose.position.z = 0;
  pose_src_stamped.pose.orientation = tf::createQuaternionMsgFromYaw(pose_src.theta);

  tf::Stamped<tf::Pose> tf_pose_src, tf_pose_dst;
  
  // Build up a tf::Stamped<Pose> from a geometry_msgs::PoseStamped
  tf::poseStampedMsgToTF(pose_src_stamped, tf_pose_src);
  
  tf::TransformListener tf_listener;
  try {
    // Let us wait for the frame transformation to be avaiable
    tf_listener.waitForTransform(source_frame, target_frame, now,  ros::Duration(1.0));
    
    // And compute the transformation
    tf_listener.transformPose(target_frame, tf_pose_src, tf_pose_dst);
  }
  catch (tf::TransformException ex) {
    ROS_ERROR("%s",ex.what());
    return false;
  }

  double useless_pitch, useless_roll, yaw;
  tf_pose_dst.getBasis().getEulerYPR(yaw, useless_pitch, useless_roll);
  // move the yaw in [-pi, pi]
  yaw = angles::normalize_angle(yaw);


  // Transform the tf::Pose back into the Pose2D
  pose_dst.x = tf_pose_dst.getOrigin().x();
  pose_dst.y = tf_pose_dst.getOrigin().y();
  pose_dst.theta = yaw;

  return true;
}


bool transform_callback(tl_turtle_track::pose::Request& request, tl_turtle_track::pose::Response& response) {

  geometry_msgs::Pose2D goal = request.pose;
  geometry_msgs::Pose2D target_pose;

  // Let us transform the current goal from the world to the 
  // base frame
  transformPose2D(goal, WORLD_TF, response.pose, BASE_TF);

  return true;
}

#define MAX_DIST 0.05
#define MAX_ANGL 0.05

bool moveTo(const geometry_msgs::Pose2D& target_pose_world, geometry_msgs::Twist& cmd_vel)
{

  geometry_msgs::Pose2D target_pose_base;
  transformPose2D(target_pose_world, WORLD_TF, target_pose_base, BASE_TF);

  double distance = std::sqrt(std::pow(target_pose_base.x,2) + std::pow(target_pose_base.y,2));
  double angle = std::atan2(target_pose_base.y,target_pose_base.x);
  
  if(distance > MAX_DIST) {

    if (std::abs(angle) > MAX_ANGL) {
      // ROS_INFO("1 ; dist = %f ; angle = %f ; y = %f", cmd_vel.linear.x, angle, target_pose_base.y);
      //ROS_INFO("1");
      cmd_vel.linear.x = 0.5*cmd_vel.linear.x;
      cmd_vel.angular.z = 5*angle;
    }
    
    else { //go to destination
      // ROS_INFO("2 ; dist = %f ; x = %f", distance, target_pose_base.x);
      //ROS_INFO("2");
      cmd_vel.linear.x = 2*target_pose_base.x;
      cmd_vel.angular.z = 0.0;
    }
  }
  
  else if (std::abs(target_pose_base.theta) > MAX_ANGL){
    // ROS_INFO("3 ; dist = %f ; theta = %f", distance, target_pose_base.theta);
    //ROS_INFO("3");
    cmd_vel.linear.x = 0.0;
    cmd_vel.angular.z = 5*target_pose_base.theta;
  }

  else{
    //ROS_INFO("4");
    cmd_vel.linear.x = 0.0;
    cmd_vel.angular.z = 0.0;
    return true;
  }
  return false;
  
}

int main(int argc, char* argv[]) {

  ros::init(argc, argv, "transform_pose2D");  

  ros::NodeHandle nh;
  ros::ServiceServer service = nh.advertiseService("transform_pose2D", transform_callback);

  ros::NodeHandle node1;
  ros::Publisher pub = node1.advertise<geometry_msgs::Twist>("cmd_vel", 1000);

  ROS_INFO("Ready to transform poses from %s into %s", WORLD_TF, BASE_TF);
  
  ROS_INFO("ARGC  %i", argc);
  
  if (argc == 4) {
    double x = atof(argv[1]), y = atof( argv[2]), theta = atof(argv[3]);

    geometry_msgs::Pose2D target_pose_world;
    target_pose_world.x = x;
    target_pose_world.y = y;
    target_pose_world.theta = theta;

    geometry_msgs::Twist cmd_vel;
    
    while(!moveTo(target_pose_world,cmd_vel)) {
      pub.publish(cmd_vel);
      ros::spinOnce();
    }

    pub.publish(cmd_vel);
  }
  
  //ros::spin();

  return 0;
}
