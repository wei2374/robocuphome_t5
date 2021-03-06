#include <ros/ros.h>
#include <std_srvs/Empty.h>
#include <geometry_msgs/Twist.h>
#include <move_base_msgs/MoveBaseAction.h>
#include <actionlib/client/simple_action_client.h>
#include <moveit/move_group_interface/move_group_interface.h>
#include <string>
#include <vector>
#include <map>


typedef actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> MoveBaseClient;

int move_head()
{
	ROS_INFO("Reach the move it function");
	// correspond to x, y, z, r, p, y
	std::map<std::string, double> target_position;
  	target_position["head_1_joint"] = 0;
  	target_position["head_2_joint"] = -0.7;
	std::vector<std::string> torso_head_joint_names;
	//select group of joints
	moveit::planning_interface::MoveGroupInterface group_head_torso("head");
	//choose your preferred planner
	group_head_torso.setPlannerId("SBLkConfigDefault");
	group_head_torso.setPoseReferenceFrame("base_footprint");
	ros::AsyncSpinner spinner(1); 
	spinner.start();

	torso_head_joint_names = group_head_torso.getJoints();
	group_head_torso.setStartStateToCurrentState();
	group_head_torso.setMaxVelocityScalingFactor(1.0);
	
	for (unsigned int i = 0; i < torso_head_joint_names.size(); ++i)
    if ( target_position.count(torso_head_joint_names[i]) > 0 )
    {
      ROS_INFO_STREAM("\t" << torso_head_joint_names[i] << " goal position: " << target_position[torso_head_joint_names[i]]);
      group_head_torso.setJointValueTarget(torso_head_joint_names[i], target_position[torso_head_joint_names[i]]);
    }
	moveit::planning_interface::MoveGroupInterface::Plan my_plan;
  	group_head_torso.setPlanningTime(5.0);
  	group_head_torso.plan(my_plan);
	group_head_torso.move();
	
	spinner.stop();			

	return 0;
}

int main(int argc, char** argv)
{
	ros::init(argc, argv, "nav_the_map");
	ros::NodeHandle n;

	//tell the action client that we want to spin a thread by default
	MoveBaseClient ac("move_base", true);

	//wait for the action server to come up
	while(!ac.waitForServer(ros::Duration(5.0)))
	{
		ROS_INFO("Waiting for the move_base action server to come up");
	}

	// initial localization
	std_srvs::Empty srv;
	ros::service::call("/global_localization", srv);
	ROS_INFO("call service global localization");
	// emulate key_tele
	ros::Publisher pub=n.advertise<geometry_msgs::Twist>("/mobile_base_controller/cmd_vel",100);
	geometry_msgs::Twist msg;
	msg.linear.x = 0;
	msg.linear.y = 0;
	msg.linear.z = 0;
	msg.angular.x = 0;
	msg.angular.y = 0;
	msg.angular.z = 2;
	int c = 30;
	while(c)
	{
		pub.publish(msg);
		ROS_INFO_STREAM("send message to turn "<<c);
		c--;
		ros::Duration(0.8).sleep();
	}
	ros::service::call("/move_base/clear_costmaps", srv);
	ROS_INFO("call service clear costmap");


	double target[4] = {0.6535, 0.4, 0.71, 0.702};
	int count = 0;
	while (ros::ok())
	{
		move_base_msgs::MoveBaseGoal goal;
		goal.target_pose.header.frame_id = "map";
		goal.target_pose.header.stamp = ros::Time::now();
		goal.target_pose.pose.position.x = target[0];
		goal.target_pose.pose.position.y = target[1];
		goal.target_pose.pose.orientation.z = target[2];
		goal.target_pose.pose.orientation.w = target[3];
		ROS_INFO_STREAM("Sending goal to position.");
		ac.sendGoal(goal);
		ac.waitForResult();
		if(ac.getState() == actionlib::SimpleClientGoalState::SUCCEEDED)
		{
			ROS_INFO_STREAM("Reach position.");
			move_head();
		}
		else
			ROS_INFO("Failed to move forward to the target");
  }
  return 0;
}
