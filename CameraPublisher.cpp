/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "CameraPublisher.h"
#include "Camera.h"
#include <sstream>

#define THREAD_SLEEP_TIMEOUT (15)

using std::stringstream;

CameraPublisher::CameraPublisher(std::string nodeName, ros::NodeHandle* nh, uint8_t camID, uint16_t frameWidth, uint16_t frameHeight)
    : Camera(camID, frameWidth, frameHeight)
{
    this->nh = nh;
    this->nodeName = nodeName;
}

CameraPublisher::~CameraPublisher()
{
    delete (transporter);
}

bool CameraPublisher::Connect()
{
    if (Camera::Connect())
    {    
        printf("camera name = %s\n", nodeName.c_str());
    
        transporter = new image_transport::ImageTransport(*nh);
    
        // Advertise with the passed Camera ID and 5 depth queue
        pub = transporter->advertise(nodeName, 5);
        
        return (true);
    }
    
    return (false);
}

bool CameraPublisher::Execute()
{
    // Starting the loop, grab a frame from the camera!
    camera->grab();

    // Lock the mutex
    pthread_mutex_lock(&dataLock);
        
    // Delete the previous image before we 
    delete (imageData);
    imageData = new Mat();
    
    // Try and get the camera image
    camera->retrieve(*imageData);
    
    cv_bridge::CvImagePtr cv_image = boost::make_shared<cv_bridge::CvImage>(std_msgs::Header(), "bgr8", *imageData);
    cv_image = cv_bridge::cvtColor(cv_image, "rgb8");
    sensor_msgs::ImagePtr msg = cv_image->toImageMsg();
    pub.publish(*msg);
    
    pthread_mutex_unlock(&dataLock);
    
    // Really only want to grab 20 frames a second - max
    SleepTo(THREAD_SLEEP_TIMEOUT);
    
    // Keep spinning
    return (true);
}