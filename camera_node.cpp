#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
// #include <vision_opencv>
#include <image_transport/image_transport.h>
#include <opencv2/highgui/highgui.hpp>
#include <cv_bridge/cv_bridge.h>

#include <iostream>
#include <csignal>
#include <vector>
#include <sstream>

#include "camera_lib/Time.h"
#include "camera_lib/CameraPublisher.h"

using namespace cv;

using std::vector;
using std::stringstream;

//#define VERBOSE

#define ROS_THREAD_SLEEP     (1000)
#define MAX_CAMERAS          (2)

#define DEFAULT_FRAME_WIDTH  (1024)
#define DEFAULT_FRAME_HEIGHT (768)

vector<CameraPublisher*> cameras;

void sigintHandler(int sig);

int main(int argc, char** argv)
{
    // Do this first!
    ros::init(argc, argv, "camera_node");

    ROS_INFO("Node Init Successful!");

    // Create new Handle
    ros::NodeHandle nodeHandle;

    // Set the loop rate for ROS
    ros::Rate LoopRate(ROS_THREAD_SLEEP);

    ROS_INFO("Set ROS Loop Rate!");

    // Create signal handler
    signal(SIGINT, sigintHandler);

    ROS_INFO("Assigned Signal Handler!");

    // We should be able to get four cameras
    for (int cnt=0; cnt < MAX_CAMERAS; cnt++)
    {
        stringstream ss;
        ss << "camera" << cnt << "/Image";
        CameraPublisher* temp = new CameraPublisher(ss.str(), &nodeHandle, cnt, DEFAULT_FRAME_WIDTH, DEFAULT_FRAME_HEIGHT);

        if (temp->Connect())
        {
            ROS_INFO("Camera %d Connected!", cnt);
        }
        else
        {
            ROS_INFO("Unable to open Camera %d", cnt);
        }
    }

    ROS_INFO("%d Cameras detected and connected", (int)cameras.size());

    TIME start = GetTime();

    // Main Node Loop grab the mats and publish them
    while (ros::ok())
    {
        #ifdef VERBOSE
        ROS_INFO("Starting to Spin ROS Node");
        #endif

        // Spin once
        ros::spinOnce();

        LoopRate.sleep();
    }

    // Terminate gracefully... with a bit of luck
    return (0);
}

void sigintHandler(int sig)
{
    ROS_INFO(" Ctrl-C detecting, shutting down");

    for (int cnt = 0; cnt< cameras.size(); cnt++)
    {
        delete cameras[cnt];
    }

    ROS_INFO(" Goodbye :-( ");

    // Shutdown ros node on sigint handle
    ros::shutdown();
}