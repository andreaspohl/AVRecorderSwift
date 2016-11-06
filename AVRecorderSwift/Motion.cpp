//
//  Motion.cpp
//  AVRecorderSwift
//
//  Created by Andreas Pohl on 29.10.16.
//  Copyright © 2016 Andreas Pohl. All rights reserved.
//

#include "Motion.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio/videoio.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <iostream>
#include <stdio.h>

using namespace cv;
using namespace std;


//Originally written by  Kyle Hounslow, December 2013
//and modified by Andreas Pohl, November 2014 to December 2015
//Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software")
//, to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
//and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
//IN THE SOFTWARE.

//history
//0.1 freshly checked in, running from local file and doing good
//0.2 file slizing introduced to prevent 4GB overriding
//0.3 trying to record and store to file

#include <iostream>
#include <iomanip>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;
using namespace cv;

//our sensitivity value to be used in the threshold() function
const static int SENSITIVITY_VALUE = 20;
//size of blur used to smooth the intensity image output from absdiff() function
const static int BLUR_SIZE = 10;
//we'll have just one object to search for and keep track of its position.
int theObject[2] = { 0, 0 };
//bounding rectangle of the object, we will use the center of this as its position.
Rect objectBoundingRectangle = Rect(0, 0, 0, 0);
//filtered object position;
Point2f filteredPosition;

//input video size
const Size IN_VIDEO_SIZE = Size(1920, 1080);

//output video size. For movies from Lumix, this is also the input video size.
const Size OUT_VIDEO_SIZE = Size(1280, 720);

//max frames per file, estimated to not exceed the opencv 4GB file size limit
//further limited to make short output movies, as VideoWriter slows down extremely with larger file size
//250 frames at 25 fps --> 10 sec.
const static int MAX_FRAMES = 125;

//int to string helper function
string intToString(int number) {
    
    //this function has a number input and string output
    std::stringstream ss;
    ss << number;
    return ss.str();
}

//bool to print timestamps
const static bool silent = 1;

inline void timestamp(string s) {
    
    // prints a timestamp to the console
    
    if (!silent) {
        static int64 t0 = getTickCount();
        int64 t1 = getTickCount();
        cout << setw(10) << s << ":" << setfill(' ') << setw(10) << (int)((t1 - t0) / getTickFrequency() * 1000) << " ms" << endl;
        t0 = t1;
    }
}

void inertiaFilter(Point &p) {
    
    //implements an "inertia" filter
    //think of a mass, lying on the video plane (friction fr),
    //moved around by p which is connected to the mass by a spring.
    //the mass center is the filter output, giving the filter a "real" feeling,
    //as if somebody follows the movement with a heavy camera mounted on a tripod.
    
    const float mass = 10.0;
    //friction
    const float fr = 0.9;
    //spring factor
    const float spring = 0.1;
    
    //inertia state vektor is x, y, vx, vy, ax, ay
    static Mat state(6, 1, CV_32F, Scalar::all(0));
    
    //initialize filter position to the center of movement p
    static bool firstTime = true;
    if (firstTime) {
        state.at<float>(0, 0) = p.x;
        state.at<float>(1, 0) = p.y;
        firstTime = false;
    }
    
    //transition Matrix
    //Newton:
    //x = x + vx + 1/2ax
    //vx = vx*friction + ax
    //ax = force / mass
    //and the same for y...
    static Mat transitionMatrix =  (Mat_<float>(6, 6) << 1, 0, 1, 0, .5, 0, 0, 1, 0, 1, 0, .5, 0, 0, fr, 0, 1, 0, 0, 0, 0, fr, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1);
    
    //force that "moves" mass is the difference between last point and actual measurement,
    //multiplied with a "spring" factor
    Point pos = Point(state.at<float>(0, 0), state.at<float>(1, 0));
    Point force = spring * (p - pos);
    
    //acceleration
    Point2f accel;
    accel.x = force.x / mass;
    accel.y = force.y / mass;
    
    //update state
    state.at<float>(4, 0) = accel.x;
    state.at<float>(5, 0) = accel.y;
    
    //calculate new mass or camera position
    state = transitionMatrix * state;
    
    //update output value
    p.x = (int) state.at<float>(0, 0);
    p.y = (int) state.at<float>(1, 0);
}

void searchForMovement(Mat thresholdImage, Mat &cameraFeed, Mat &zoomedImage) {
    
    //notice how we use the '&' operator, objectDetected and cameraFeed and zoomedImage. This is because we wish
    //to take the values passed into the function and manipulate them, rather than just working with a copy.
    //eg. we draw to the cameraFeed to be displayed in the main() function.
    Moments mu;
    //holds last tracking center
    static Point previousCenter(-1, -1);
    
    //calculate moments
    mu = moments(thresholdImage, true);
    //and calculate mass center of the threshold image
    timestamp("moments");
    if (mu.m00 > 0) {
        //if there is a mass center (i.e. some white points on thresholdImage), update theObject
        theObject[0] = mu.m10 / mu.m00;
        theObject[1] = mu.m01 / mu.m00;
    }
    timestamp("mass");
    
    //make some temp x and y variables so we dont have to type out so much
    int x = theObject[0];
    int y = theObject[1];
    
    //draw some crosshairs around the object
    line(cameraFeed, Point(x, y), Point(x, y - 25), Scalar(0, 255, 0), 1);
    line(cameraFeed, Point(x, y), Point(x, y + 25), Scalar(0, 255, 0), 1);
    line(cameraFeed, Point(x, y), Point(x - 25, y), Scalar(0, 255, 0), 1);
    line(cameraFeed, Point(x, y), Point(x + 25, y), Scalar(0, 255, 0), 1);
    
    timestamp("line");
    
    
    //draw a variable circle, depending on mass of object
    //m > 10'000 --> r = 1
    //m = 0      --> r = 250
    int r = 0;
    r = (int) ((10000 - mu.m00) / 10000 * 250);
    if (r <= 0) {
        r = 1;
    }
    
    timestamp("radius");
    
    //initialize at the beginning
    if (previousCenter.x == -1) {
        previousCenter.x = x;
        previousCenter.y = y;
    }
    
    //actual target center p
    Point p(x, y);
    
    //do not move p if inside circle. If p is outside of circle, move only so that it gets inside circle again
    Point cp;
    
    cp = p - previousCenter;
    
    int cpLength = (int) sqrt(cp.x * cp.x + cp.y * cp.y);
    
    int o = cpLength - r;
    if (o > 0) {
        //p outside circle
        float f = o / cpLength;
        Point d(0, 0); //d is the radial vector from circle to p;
        d.x = (int) cp.x * f;
        d.y = (int) cp.y * f;
        //update p
        p = p + d;
    } else {
        //p inside circle --> don't move
        p = previousCenter;
    }
    
    timestamp("calculate");
    
    //filter
    inertiaFilter(p);
    
    timestamp("filter");
    
    circle(cameraFeed, p, r, Scalar(255, 255, 0), 1);
    
    timestamp("circle");
    
    //make zoomed Image
    //TODO: adapt to circle size
    Size zoomedWindow = Size(640, 360);
    
    Mat cutImage;
    int xx, yy;
    xx = p.x - ( (int) zoomedWindow.width / 2 );
    yy = p.y - ( (int) zoomedWindow.height / 2);

    //limit against border of image
    if (xx < 0) xx = 0;
    if (yy < 0) yy = 0;
    
    int maxX = IN_VIDEO_SIZE.width - zoomedWindow.width - 1;
    int maxY = IN_VIDEO_SIZE.height - zoomedWindow.height - 1;
    
    if (xx > maxX) xx = maxX;
    if (yy > maxY) yy = maxY;
    
    cutImage = cameraFeed(Rect(xx, yy, zoomedWindow.width, zoomedWindow.height));
    timestamp("rect");
    resize(cutImage, zoomedImage, OUT_VIDEO_SIZE, 0, 0, INTER_CUBIC);
    timestamp("resize");
    
    previousCenter = p;
}


void Motion::processVideo(const char * videoFileName) {
    cout << "Motion.processVideo started with";
    cout << videoFileName;
    cout << "\n";

    //some boolean variables for added functionality
    //these can be toggled by pressing 'd', 't' or 'p'
    bool debugMode = false;
    bool trackingEnabled = true;
    bool pause = false;
    
    //switch to show the output stream
    bool showOutput = true;
    

    //motionTracking section
    
    //set up the matrices that we will need
    //the input frame
    Mat frame;
    //their grayscale images (needed for comparing)
    Mat grayImage1, grayImage2;
    //resulting difference (and masked) image
    Mat differenceImage;
    //thresholded difference image (for use in findContours() function)
    Mat thresholdImage;
    //zoomed image
    Mat zoomedImage;
    
    //mask
    Mat mask = imread("mask/horseSampleShotMask.png", CV_LOAD_IMAGE_GRAYSCALE);
    if (!mask.data)                              // Check for invalid input
    {
        cout << "ERROR OPENING MASK IMAGE" << std::endl;
        return;
    }
    threshold(mask, mask, SENSITIVITY_VALUE, 255, THRESH_BINARY);
    
    //video capture object.
    VideoCapture capture;
    
    //video output
    VideoWriter outVideo;
    
    //use for livecam
    //capture.open(0);
    
    //set to true when the video should be restarted after ending
    bool loopVideo = false;
    do {
        
        //we can loop the video by re-opening the capture every time the video reaches its last frame
        capture.open(videoFileName);
        
        if (!capture.isOpened()) {
            cout << "ERROR ACQUIRING VIDEO FEED\n";
            getchar();
            return;
        }
        
        //open output stream
        //working codes:
        //CV_FOURCC('j', 'p', 'e', 'g');
        //CV_FOURCC('m', 'p', '4', 'v');
        int videoCodec = CV_FOURCC('m', 'p', '4', 'v');
        
        int frameCount = 0; //we track the file size to limit max file size
        int fileCount = 1; //files are numbered,
        
        string baseFileName = " done";
        string fileName = baseFileName + std::to_string(fileCount) + ".mov";
        
        outVideo.open(fileName, videoCodec, capture.get(CV_CAP_PROP_FPS), OUT_VIDEO_SIZE, true);
        if (!outVideo.isOpened()) {
            cout << "ERROR OPENING OUTPUT STREAM\n";
            getchar();
            return;
        }
        
        //read frame
        capture.read(frame);
        //convert frame to gray scale for frame differencing
        cvtColor(frame, grayImage2, COLOR_BGR2GRAY);
        
        //just show so that there is something
        //imshow("StartFrame", frame);
        //imshow("Mask", mask);
        
        //check if the video has reach its last frame.
        //we add '-2' because we are reading two frames from the video at a time.
        //if this is not included, we get a memory error!
        //this while loop is not need for livecam
        while (capture.get(CV_CAP_PROP_POS_FRAMES) < capture.get(CV_CAP_PROP_FRAME_COUNT) - 2) {
            
            timestamp("init");
            
            //set first grayImage to the last one read from camera
            swap(grayImage1, grayImage2);
            
            //measure time
            timestamp("swap");
            
            //read next frame
            capture.read(frame);
            
            //measure time
            timestamp("read");
            
            //convert frame to gray scale for frame differencing
            cvtColor(frame, grayImage2, COLOR_BGR2GRAY);
            
            //measure time
            timestamp("cvtColor");
            
            //perform frame differencing with the sequential images. This will output an "intensity image"
            //do not confuse this with a threshold image, we will need to perform thresholding afterwards.
            absdiff(grayImage1, grayImage2, differenceImage);
            
            //measure time
            timestamp("absdiff");
            
            //now mask the result to filter only the relevant regions of the picture
            //TODO: reactive mask (needs to be same size as video)
            //Mat temp;
            //differenceImage.copyTo(temp, mask);
            //temp.copyTo(differenceImage);
            
            //measure time
            timestamp("copy");
            
            //threshold intensity image at a given sensitivity value
            threshold(differenceImage, thresholdImage, SENSITIVITY_VALUE, 255, THRESH_BINARY);
            
            //measure time
            timestamp("thresh");
            
            if (debugMode == true) {
                //show the difference image and threshold image
                //imshow("Difference Image", differenceImage);
                //imshow("Threshold Image", thresholdImage);
            } else {
                //if not in debug mode, destroy the windows so we don't see them anymore
                destroyWindow("Difference Image");
                destroyWindow("Threshold Image");
            }
            
            //measure time
            timestamp("debug");
            
            //blur the image to get rid of the noise. This will output an intensity image
            blur(thresholdImage, thresholdImage, Size(BLUR_SIZE, BLUR_SIZE));
            
            //measure time
            timestamp("blur");
            
            //threshold again to obtain binary image from blur output
            threshold(thresholdImage, thresholdImage, SENSITIVITY_VALUE, 255, THRESH_BINARY);
            
            //measure time
            timestamp("thresh");
            
            if (debugMode == true) {
                //show the threshold image after it's been "blurred"
                //imshow("Final Threshold Image", thresholdImage);
            } else {
                //if not in debug mode, destroy the windows so we don't see them anymore
                destroyWindow("Final Threshold Image");
            }
            
            //measure time
            timestamp("show");
            
            //if tracking enabled, search for movement in our thresholded image
            if (trackingEnabled) {
                
                searchForMovement(thresholdImage, frame, zoomedImage);
                
                //measure time
                timestamp("search");
                
                destroyWindow("Frame");
                
                //measure time
                timestamp("destroy");
                
                if (showOutput) {
                    //imshow("Zoomed Image", zoomedImage);
                }
                
                //measure time
                timestamp("show");
                
                outVideo.write(zoomedImage);
                
                //update file size / frame count
                frameCount++;
                
                //measure time
                timestamp("write");
                
            } else {
                
                destroyWindow("Zoomed Image");
                destroyWindow("Filtered Zoomed Image");
                
                //show our captured frame
                //imshow("Frame", frame);
            }
            
            //check for max file size, if MAX_FRAMES is exceeded, open a new file.
            if (frameCount > MAX_FRAMES) {
                frameCount = 0;
                outVideo.release();
                fileCount++;
                fileName = baseFileName + std::to_string(fileCount) + ".mov";
                outVideo.open(fileName, videoCodec, capture.get(CV_CAP_PROP_FPS), OUT_VIDEO_SIZE, true);
                if (!outVideo.isOpened()) {
                    cout << "ERROR OPENING OUTPUT STREAM\n";
                    getchar();
                    return;
                }
                
                
            }
            
            //check to see if a button has been pressed.
            //this 10ms delay is necessary for proper operation of this program
            //if removed, frames will not have enough time to refresh and a blank
            //image will appear.
            switch (waitKey(1)) {
                    
                case 27: //'esc' key has been pressed, exit program.
                    return;
                    
                case 116: //'t' has been pressed. this will toggle tracking
                    trackingEnabled = !trackingEnabled;
                    if (trackingEnabled == false)
                        cout << "Tracking disabled." << endl;
                    else
                        cout << "Tracking enabled." << endl;
                    break;
                    
                case 100: //'d' has been pressed. this will toggle debug mode
                    debugMode = !debugMode;
                    if (debugMode == false)
                        cout << "Debug mode disabled." << endl;
                    else
                        cout << "Debug mode enabled." << endl;
                    break;
                    
                case 112: //'p' has been pressed. this will pause/resume the code.
                    pause = !pause;
                    if (pause == true) {
                        cout << "Code paused, press 'p' again to resume" << endl;
                        while (pause == true) {
                            //stay in this loop until
                            switch (waitKey()) {
                                    //a switch statement inside a switch statement? Mind blown.
                                case 112:
                                    //change pause back to false
                                    pause = false;
                                    cout << "Code Resumed" << endl;
                                    break;
                            }
                        }
                    }
                    
            }
            
            //measure time
            timestamp("key");
            if (!silent){
                cout << "---------------------------------" << endl;
            }
            
        }
        //release the capture before re-opening and looping again.
        capture.release();
    } while (loopVideo);
    
    return;
    
}


