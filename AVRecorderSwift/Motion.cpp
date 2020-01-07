//
//  Motion.cpp
//  AVRecorderSwift
//
//  Created by Andreas Pohl on 29.10.16.
//  Copyright © 2016 Andreas Pohl. All rights reserved.
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

#include "Motion.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio/videoio.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <iomanip>

#include <dispatch/dispatch.h>

using namespace std;
using namespace cv;

//our sensitivity value to be used in the threshold() function
const static int SENSITIVITY_VALUE = 30; //was 20 initially
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

//max zoom window
Size MAX_ZOOMED_WINDOW = Size(640, 360);


//factor for reducing the frames for speed
const static double reduceFactor = 0.5;

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

//for testing
static bool test = false;

inline void timestamp(string s) {
    
    // prints a timestamp to the console
    
    if (!silent) {
        static int64 t0 = getTickCount();
        int64 t1 = getTickCount();
        cout << setw(10) << s << ":" << setfill(' ') << setw(10) << (int)((t1 - t0) / getTickFrequency() * 1000) << " ms" << endl;
        t0 = t1;
    }
}

void Motion::setTest() {
    test = true;
}

//replace part in string
std::string ReplaceString(std::string subject, const std::string& search,
                          const std::string& replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
    return subject;
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

void searchForMovement(Mat thresholdImage, Mat &cameraFeed, Mat &zoomedImage, Mat redFrame) {
    
    //notice how we use the '&' operator, objectDetected and cameraFeed and zoomedImage. This is because we wish
    //to take the values passed into the function and manipulate them, rather than just working with a copy.
    //eg. we draw to the cameraFeed to be displayed in the main() function.
    Moments mu;
    //holds last tracking center
    static Point previousCenter(-1, -1);
    
    //holds last zoomFactor
    static double previousZoomFactor = 0;
    
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
    
    //calculate the bounding rectangle for all non zero points
    vector<Point> points;
    findNonZero(thresholdImage, points);
    objectBoundingRectangle = boundingRect(points);
    
    //calculate a variable circle, depending on mass of object, and zoomFactor
    //m > 10'000 --> r = 1
    //m = 0      --> r = maxR
    const int maxR = (int) IN_VIDEO_SIZE.height / 8; //TODO: an 1/8 is the max R
    int r = 0;
    r = (int) ((10000 - mu.m00) / 10000 * maxR / ((100 - previousZoomFactor)/100*3));  //TODO: 3 is about the factor between in_video and max_zoom --> calculate
    if (r <= 0) {
        r = 1;
    }
    
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
    
    //calculate zoom factor
    int cameraVerticalPosition = (int) IN_VIDEO_SIZE.height / 2; 
    Size zoomedWindow = MAX_ZOOMED_WINDOW;
    
    //calculate zoom window size
    //if p.y is above cameraVerticalPosition --> maximal zoom
    //if p.y is halfway between cameraVerticalPosition and lower image border --> no zoom
    //calculate zoom factor
    double zoomFactor = 0.0;  // zoomFaktor will be between 0 (no zoom) and 1 (max zoom)
    zoomFactor = 1.0 - (2.0 * (p.y / reduceFactor - cameraVerticalPosition) / (IN_VIDEO_SIZE.height - cameraVerticalPosition));
    if (zoomFactor > 1.0 ) {
        zoomFactor = 1.0;
    } else if (zoomFactor < 0.0) {
        zoomFactor = 0.0;
    }
    
    //store for later usage
    previousZoomFactor = zoomFactor;
    
    //make zoomed Image
    zoomedWindow.width = (int)(IN_VIDEO_SIZE.width - zoomFactor * (IN_VIDEO_SIZE.width - MAX_ZOOMED_WINDOW.width));
    zoomedWindow.height = (int)(IN_VIDEO_SIZE.height - zoomFactor * (IN_VIDEO_SIZE.height - MAX_ZOOMED_WINDOW.height));
    
    if (zoomedWindow.width > IN_VIDEO_SIZE.width) {
        zoomedWindow.width = IN_VIDEO_SIZE.width;
    }
    if (zoomedWindow.height > IN_VIDEO_SIZE.height) {
        zoomedWindow.height = IN_VIDEO_SIZE.height;
    }
    
    Mat cutImage;
    int xx, yy;
    xx = ( (int) p.x / reduceFactor ) - ( (int) zoomedWindow.width / 2 );
    yy = cameraVerticalPosition - ( (int) zoomedWindow.height / 2 ); // fix vertical camera swing
    
    //limit against border of image
    if (xx < 0) xx = 0;
    if (yy < 0) yy = 0;
    
    int maxX = IN_VIDEO_SIZE.width - zoomedWindow.width;
    int maxY = IN_VIDEO_SIZE.height - zoomedWindow.height;
    
    if (xx > maxX) xx = maxX;
    if (yy > maxY) yy = maxY;
    
    //cout << "cutImage " << xx << " " << yy << " " << zoomedWindow.width << " " << zoomedWindow.height << "\n";
    
    cutImage = cameraFeed(Rect(xx, yy, zoomedWindow.width, zoomedWindow.height));
    timestamp("rect");
    resize(cutImage, zoomedImage, OUT_VIDEO_SIZE, 0, 0, INTER_CUBIC);
    timestamp("resize");
    
    previousCenter = p;
    
    //draw debug information
    if (test) {
        //draw center of gravity of image moment
        Mat motionImage;
        //cvtColor(redFrame, motionImage, COLOR_GRAY2RGB);
        redFrame.copyTo(motionImage);
        line(motionImage, Point(x, y), Point(x, y - 25), Scalar(0, 255, 0), 3);
        line(motionImage, Point(x, y), Point(x, y + 25), Scalar(0, 255, 0), 3);
        line(motionImage, Point(x, y), Point(x - 25, y), Scalar(0, 255, 0), 3);
        line(motionImage, Point(x, y), Point(x + 25, y), Scalar(0, 255, 0), 3);
        
        //draw center of camera (after inertia filtering)
        line(motionImage, p, Point(p.x, p.y - 25), Scalar(255, 255, 0), 3);
        line(motionImage, p, Point(p.x, p.y + 25), Scalar(255, 255, 0), 3);
        line(motionImage, p, Point(p.x - 25, p.y), Scalar(255, 255, 0), 3);
        line(motionImage, p, Point(p.x + 25, p.y), Scalar(255, 255, 0), 3);
        
        //draw inverse mass circle
        circle(motionImage, p, r, Scalar(255, 255, 0));
        
        //draw bounding rectangle
        rectangle(motionImage, objectBoundingRectangle.tl(), objectBoundingRectangle.br(), Scalar(0, 255, 255), 3);
        
        //draw zoom rectangle
        Point tl(xx, yy);
        Point br(xx + zoomedWindow.width, yy + zoomedWindow.height);
        tl = tl * reduceFactor;
        br = br * reduceFactor;
        rectangle(motionImage, tl, br, Scalar(255, 0, 0), 3);
        
        imshow("Movement", motionImage);
    }
    
}

void reduce(Mat in, Mat &out) {
    resize(in, out, Size(), reduceFactor, reduceFactor, INTER_CUBIC);
}


void Motion::processVideo(const char * pathName) {
    cout << "Motion.processVideo started with " << pathName << "\n";
    
    //some boolean variables for testing
    bool showDifference = false;
    bool showActualFrame = false;
    bool showOutput = false;
    bool showMask = false;
    
    if (test) {
        showDifference = false;
        showActualFrame = false;
        showOutput = true;
        showMask = false;
    }
    
    
    //strip input file name of ´new´
    string sPathName = (string) pathName;
    string videoFileName = sPathName.substr(sPathName.find_last_of("/") + 1 );
    string path = sPathName.substr(0, sPathName.find_last_of("/") + 1 );
    string inFileNameNew = videoFileName;
    string inFileName = inFileNameNew.substr(0, inFileNameNew.find_last_of(" ")); //get the filename without ´new´
    
    
    //motionTracking section
    
    //set up the matrices that we we'll need
    //the input frame
    Mat origFrame;
    //the reduced input frame$
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
    Mat mask = imread(path + "../0_mask/horseSampleShotMask.png", IMREAD_GRAYSCALE);
    
    if (!mask.data)                              // Check for invalid input
    {
        cout << "NO MASK IMAGE FOUND" << std::endl;
    } else {
        reduce(mask, mask);
        threshold(mask, mask, SENSITIVITY_VALUE, 255, THRESH_BINARY);
    }
    
    //video capture object.
    VideoCapture capture;
    
    //video output
    VideoWriter outVideo;
    
    //we can loop the video by re-opening the capture every time the video reaches its last frame
    capture.open(pathName);
    
    if (!capture.isOpened()) {
        cout << "ERROR ACQUIRING VIDEO FEED\n";
        return;
    }
    
    //open output stream
    //working codes:
    //CV_FOURCC('j', 'p', 'e', 'g');
    //CV_FOURCC('m', 'p', '4', 'v');
    //int videoCodec = CV_FOURCC('m', 'p', '4', 'v');
    int videoCodec = outVideo.fourcc('m', 'p', '4', 'v');
    
    int frameCount = 0; //we track the file size to limit max file size
    int fileCount = 1; //files are numbered,
    
    string fileName = path + inFileName + " 00" + std::to_string(fileCount) + " processing.mov";
    
    outVideo.open(fileName, videoCodec, capture.get(CAP_PROP_FPS), OUT_VIDEO_SIZE, true);
    if (!outVideo.isOpened()) {
        cout << "ERROR OPENING OUTPUT STREAM\n";
        return;
    }
    
    //read frame
    bool success = capture.read(origFrame);
    
    //reduce frame to gain speed
    reduce(origFrame, frame);
    
    //convert frame to gray scale for frame differencing
    if (success) {
        cvtColor(frame, grayImage2, COLOR_BGR2GRAY);
    }
    
    if (showMask) {
        imshow("Mask", mask);
    }
    
    while (true) {
        
        //set first grayImage to the last one read from camera
        swap(grayImage1, grayImage2);
        
        //read next frame
        if (!capture.read(origFrame)) break;
        reduce(origFrame, frame);
        
        if (showActualFrame) {
            imshow("actualFrame", frame);
        }
        
        //convert frame to gray scale for frame differencing
        cvtColor(frame, grayImage2, COLOR_BGR2GRAY);
        
        //perform frame differencing with the sequential images. This will output an "intensity image"
        //do not confuse this with a threshold image, we will need to perform thresholding afterwards.
        absdiff(grayImage1, grayImage2, differenceImage);
        
        //now mask the result to filter only the relevant regions of the picture
        Mat temp;
        differenceImage.copyTo(temp, mask);
        temp.copyTo(differenceImage);
        
        //threshold intensity image at a given sensitivity value
        threshold(differenceImage, thresholdImage, SENSITIVITY_VALUE, 255, THRESH_BINARY);
        
        if (showDifference) {
            //show the difference image and the threshold image
            imshow("Difference Image", differenceImage);
            imshow("Threshold Image", thresholdImage);
        }
        
        //blur the image to get rid of the noise. This will output an intensity image
        blur(thresholdImage, thresholdImage, Size(BLUR_SIZE, BLUR_SIZE));
        
        //threshold again to obtain binary image from blur output
        threshold(thresholdImage, thresholdImage, SENSITIVITY_VALUE, 255, THRESH_BINARY);
        
        if (showDifference) {
            //show the threshold image after it's been "blurred"
            imshow("Final Threshold Image", thresholdImage);
        }
        
        //search for movement in our thresholded image
        searchForMovement(thresholdImage, origFrame, zoomedImage, frame);
        
        if (showOutput) {
            imshow("Zoomed Image", zoomedImage);
        }
        
        outVideo.write(zoomedImage);
        
        //update file size / frame count
        frameCount++;
        
        //check for max file size, if MAX_FRAMES is exceeded, open a new file.
        if (frameCount > MAX_FRAMES) {
            frameCount = 0;
            outVideo.release();
            fileCount++;
            //add leading 0 to fileCount so later the snippets get sorted correctly (001, 002, 003, ...)
            std::string fileCountString = std::to_string(fileCount);
            fileCountString = std::string(3 - fileCountString.length(), '0') + fileCountString;
            fileName = path + inFileName + " " + fileCountString + " processing.mov";
            outVideo.open(fileName, videoCodec, capture.get(CAP_PROP_FPS), OUT_VIDEO_SIZE, true);
            if (!outVideo.isOpened()) {
                cout << "ERROR OPENING OUTPUT STREAM\n";
                return;
            }
            
            
        }
        
        if (test) {
            //this 1ms delay is necessary for proper operation of this program
            //if removed, frames will not have enough time to refresh and a blank
            //image will appear.
            waitKey(1);
        }
    }
    
    capture.release();
    outVideo.release();
    return;
}


