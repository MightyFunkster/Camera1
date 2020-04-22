/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include "Camera.h"

#include <stdio.h>
#include <string.h>
#include <opencv2/highgui/highgui.hpp>
#include <memory>
// #include <vision_opencv>

using std::deque;

using cv::Mat;

#define IMAGE_QUALITY        (95)   /*< >*/
#define CAMERA_TIMEOUT       (1000) /*< >*/
#define THREAD_SLEEP_TIMEOUT (2)    /*< >*/

Camera::Camera(uint8_t camID, uint16_t frameWidth, uint16_t frameHeight)
{
    // Instatiate camera object and mutex
    this->camID = camID;
    camera = new VideoCapture(camID);
    
    this->frameWidth = frameWidth;
    this->frameHeight = frameHeight;
    

    // New data is not yet available
    newDataAvailable = false;
    
    imageData     = new Mat();
    imageWidth    = 0;
    imageHeight   = 0;
    imageChannels = 0;
    
    pthread_mutex_init(&dataLock, NULL);
}

Camera::~Camera()    
{  
    newDataAvailable = false;
    
    // Stop the camera thread
    Stop();
    
    // On teardown try and close the camera if it is opened
    if(camera->isOpened())
    {
        camera->release();
    }
    
    // Destroy the mutex
    pthread_mutex_destroy(&dataLock);
}

bool Camera::Connect()
{    
    // Open dev node 0
    camera->open(camID);

    // Open the camera and start the capture process
    if (camera->isOpened())
    {
        // Once the camera is opened try and set the frame height and width
        camera->set(cv::CAP_PROP_FRAME_WIDTH, frameWidth);
        camera->set(cv::CAP_PROP_FRAME_HEIGHT, frameHeight);
        //camera->set(cv::CAP_PROP_CONVERT_RGB, 1);
        //frameFormat = camera->get(cv::CAP_PROP_FORMAT); 
    }
    else
    {
        printf("Camera::Connect - Camera not opened\n");        
        return (false);
    }
    
    // Wait a few seconds for the camera to stabilise
    Sleep(CAMERA_TIMEOUT);
    printf("Camera::Connect - Grabbing Test frame after camera stabilisation\n");
    
    // Check to make sure the camera is open
    // If it is then start the thread
    if(camera->isOpened())
    {        
        printf("Spinning up Camera thread\n");
        // Grab a new camera image
        camera->grab();
        
        // Fire off the thread
        Start();
    }
    
    // Whether or not it is opened return it to the calling class
    return (camera->isOpened());
}

bool Camera::IsCameraOpen()
{
    // Return the current state of the camera
    return (camera->isOpened());
}

Mat* Camera::GetImageData(long int* length)
{
    // Set the length to -1 to start indicate no image available
    *length = -1;
  
    // Check the new data variable
    // This is only cleared on access of this function
    // So if this flag is true we have unseen data
    if (newDataAvailable)
    {
        // Lock the mutex for data access
        pthread_mutex_lock(&dataLock);

        // Create a new temporary thread
        Mat* data = new Mat(); 

        // Copy our held image data into the new data Mat
        imageData->copyTo(*data);

        // Unlock the data mutex and reset external variables
        pthread_mutex_unlock(&dataLock);
        
        // Clear the new Data flag as we have accessed the most recent data
        newDataAvailable = false;
        
        // Set our passed length variable to the length of the Mat
        *length = (data->cols * data->rows * data->channels());
        
        // Return the data Mat
        return (data);
    }
    
    // If we have arrived here there is no data so return null
    // The user can check against the length variable to see if it is null
    return (NULL);
}

Mat Camera::GetImageData()
{
    Mat frame;
    if (newDataAvailable)
    {
        
        pthread_mutex_lock(&dataLock);
        frame = imageData->clone(); 
        pthread_mutex_unlock(&dataLock);
    }
    
    return (frame);
}

bool Camera::Execute()
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
    
    pthread_mutex_unlock(&dataLock);
    newDataAvailable = true;

    // Configure the image width and height
    imageHeight = imageData->rows;
    imageWidth = imageData->cols;
    imageChannels = imageData->channels();
    
    // Set the new data flag
    newDataAvailable = true;
    
    // Really only want to grab 20 frames a second - max
    SleepTo(THREAD_SLEEP_TIMEOUT);
    
    // Keep going around the loop
    return (true);
}
