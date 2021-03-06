/*

g++ -o position-client-example -Wall -ansi -pedantic -O3 position-client-example.cpp -lboost_system -lpthread -std=c++11

*/

#include <iostream>
#include <string>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <iomanip>
#include <vector>
#include <iterator>

#include <boost/asio.hpp>

#include <ros/ros.h>
#include "tl_turtle_track/ArenaPosition.h"
#include "tl_turtle_track/ArenaPositions.h"

struct Point {
  double x,y;
};

void positions_callback(const tl_turtle_track::ArenaPositions::ConstPtr &msg, boost::asio::ip::tcp::iostream &socket)
{
  for (const auto& ap : msg->ArenaPosition_Array)
    socket << "put " << ap.x << ' ' << ap.y << std::endl;
}

int main(int argc,char* argv[]) {

  if(argc < 3) {
    ROS_ERROR_STREAM("Usage : " << argv[0] 
		     << " <hostname> <port> " << std::endl);
    
    ROS_ERROR_STREAM("Currently using " << argc  << " arguments" << std::endl);

    for(int i = 0; i < argc; i++)
      ROS_ERROR_STREAM("Arg #"<< i << " " << argv[i] << std::endl);
    
    return 1;
  }
  
  ros::init(argc, argv, "share");
  ros::NodeHandle nh;

  ROS_INFO("boobs");
  try{
    // SOCKET CREATION
    std::string hostname = argv[1];
    std::string port     = argv[2];
    boost::asio::ip::tcp::iostream socket;
    
    socket.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);    
    socket.connect(hostname,port);
    
    ros::Subscriber sub_share = nh.subscribe<tl_turtle_track::ArenaPositions>
      ("positions_in",1,std::bind(positions_callback,
				  std::placeholders::_1,
				  std::ref(socket)));

    ros::spin();
  }			    
  catch(std::exception& e) {
    ROS_ERROR_STREAM("Exception caught : " << e.what() << std::endl);
  }

  return 0;
}
