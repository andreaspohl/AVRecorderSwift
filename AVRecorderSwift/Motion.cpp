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
#include "Filter.hpp"
#include "ObjectHandler.hpp"

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

//bezel, free room between object and zoomed window
const int BEZEL = 100 * reduceFactor;

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

//calculate zoom window from bounding rectangle
void calcZoom(Rect boundingRectangle, int &zoomXPosition, double &zoomFactor) {
    
    static Filter leftBorderFilter(0, Filter::BorderType::NONE);
    static Filter rightBorderFilter((int) IN_VIDEO_SIZE.width * reduceFactor, Filter::BorderType::NONE);
    static Filter bottomBorderFilter((int) IN_VIDEO_SIZE.height * reduceFactor, Filter::BorderType::NONE);
    static Filter zoomXPositionFilter((int) IN_VIDEO_SIZE.width * reduceFactor, Filter::BorderType::NONE);
    
    //unfiltered borders, yet
    int leftBorder = leftBorderFilter.update(boundingRectangle.x - BEZEL);
    int rightBorder = rightBorderFilter.update(boundingRectangle.x + boundingRectangle.width + BEZEL);
    int bottomBorder = bottomBorderFilter.update(boundingRectangle.y + boundingRectangle.height + BEZEL / 2);

    //calculate zoom factor, only from width yet
    double tempWidth = (rightBorder - leftBorder) / reduceFactor;
    zoomFactor =  (IN_VIDEO_SIZE.width - tempWidth) / (IN_VIDEO_SIZE.width - MAX_ZOOMED_WINDOW.width) * 100;
    
    //check bottom border, if it results in smaller zoomFactor, take that one
    double tempHeight = (bottomBorder - IN_VIDEO_SIZE.height * reduceFactor / 2) / reduceFactor * 2; //vertical zoom center is always height/2
    double vertZoomFactor = (IN_VIDEO_SIZE.height - tempHeight) / (IN_VIDEO_SIZE.height - MAX_ZOOMED_WINDOW.height) * 100;
    
    if (vertZoomFactor < zoomFactor) {
        zoomFactor = vertZoomFactor;
    }
    
    if (zoomFactor > 100.0) {
        zoomFactor = 100.0;
    } else if (zoomFactor < 0.0) {
        zoomFactor = 0.0;
    }
    
    zoomXPosition = zoomXPositionFilter.update(zoomXPosition);
    
}

void reduce(Mat in, Mat &out) {
    resize(in, out, Size(), reduceFactor, reduceFactor, INTER_CUBIC);
}

//tries to put max 4 clusters
void cluster(vector<Point> nonZeroPoints, Mat &redFrame, Mat &thresholdImage) {
    
    //cast points into 2D floating point array
    int sampleCount = (int) nonZeroPoints.size();
    Mat points(sampleCount, 1, CV_32FC2);
    for (int i = 0; i < sampleCount; i++) {
        points.at<Point2f>(i) = nonZeroPoints.at(i);
    }
    
    int clusterCount = MIN(4, sampleCount);
    Mat centers, labels;
    static ObjectHandler objHandler = ObjectHandler(redFrame.cols, redFrame.rows);
    vector<Point2f> objects = objHandler.getObjects();

    if (test) {
        //draw circles around objects
        for (auto obj = objects.begin(); obj != objects.end(); ++obj) {
            circle(redFrame, *obj, 30, Scalar(0, 0, 255), FILLED, LINE_AA);
        }
    }
    
    if (clusterCount > 0) {
        TermCriteria crit = TermCriteria( TermCriteria::EPS+TermCriteria::COUNT, 5, 1.0);
        
        kmeans(points, clusterCount, labels, crit, 3, KMEANS_PP_CENTERS, centers);
        
        objects = objHandler.update(centers);
        
        if (test) {
            //draw circles around the centers for debugging
            for (int i = 0; i < centers.rows; ++i)
            {
                Point2f c = centers.at<Point2f>(i);
                circle( redFrame, c, 60, Scalar(255, 0, 255), 1, LINE_AA );
            }
            
            //draw sample points (non zero points)
            for (int i = 0; i < sampleCount; i++) {
                Point ipt = points.at<Point2f>(i);
                circle(redFrame, ipt, 1, Scalar(255, 255, 0), FILLED, LINE_AA);
            }
        }
    }
    
    //draw circles around objects to thresholdImage, so later zoom frame detection will take them into account
    //TODO: this is probably not the best way to interface the objects...
    for (auto obj = objects.begin(); obj != objects.end(); ++obj) {
        circle(thresholdImage, *obj, 30, Scalar(255), FILLED, LINE_AA);
    }

}

void trackObjects(Mat thresholdImage, Mat &cameraFeed, Mat &zoomedImage, Mat redFrame) {
    
    //holds last tracking center
    static Point previousCenter(-1, -1);
    
    //holds last zoomFactor
    static double previousZoomFactor = 0;
    
    //find non zero points
    vector<Point> points;
    findNonZero(thresholdImage, points);

    //try clustering
    cluster(points, redFrame, thresholdImage);
    
    //calculate the bounding rectangle for all non zero points
    //TODO: clumsy!
    findNonZero(thresholdImage, points); //find non zero points again, as thresholdImage has been altered by cluster
    
    //bounding rectangle. If computed image is empty, take whole picture
    if (points.size() > 0) {
        objectBoundingRectangle = boundingRect(points);
    } else {
        objectBoundingRectangle.x = 0;
        objectBoundingRectangle.y = 0;
        objectBoundingRectangle.width = redFrame.cols;
        objectBoundingRectangle.height = redFrame.rows;
    }
    
    //new simple calculation of camera center
    int x = objectBoundingRectangle.x + (int) objectBoundingRectangle.width / 2;
    int y = objectBoundingRectangle.y + (int) objectBoundingRectangle.height / 2;
    
    //calculate a variable circle, vary with zoomFactor, for later use as hysteresis range
    const int maxR = (int) IN_VIDEO_SIZE.height / 8; //TODO: an 1/8 is the max R
    int r; //TODO: find some variable way for hysteresis of camera position, maybe dependant on speed?
    r = (int) (0.5 * maxR / (1 + 2 * previousZoomFactor / 100));  //TODO: 3 is about the factor between in_video and max_zoom --> calculate
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
    
    //calculate zoom factor
    int cameraVerticalPosition = (int) IN_VIDEO_SIZE.height / 2;
    Size zoomedWindow = MAX_ZOOMED_WINDOW;
    double zoomFactor = 0.0;  // zoomFaktor will be between 0 (no zoom) and 100 (max zoom)
    
    calcZoom(objectBoundingRectangle, p.x, zoomFactor);

    //store for later usage
    previousZoomFactor = zoomFactor;

    //make zoomed Image
    zoomedWindow.width = (int)(IN_VIDEO_SIZE.width - zoomFactor * (IN_VIDEO_SIZE.width - MAX_ZOOMED_WINDOW.width) / 100);
    zoomedWindow.height = (int)(IN_VIDEO_SIZE.height - zoomFactor * (IN_VIDEO_SIZE.height - MAX_ZOOMED_WINDOW.height) / 100);
    
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
        //draw center of boundary rectangle of image moment
        //cvtColor(redFrame, motionImage, COLOR_GRAY2RGB);
        line(redFrame, Point(x, y + 25), Point(x, y - 25), Scalar(0, 255, 0), 3);
        line(redFrame, Point(x + 25, y), Point(x - 25, y), Scalar(0, 255, 0), 3);
        
        //draw center of camera (after inertia filtering)
        line(redFrame, Point(p.x, p.y + 25), Point(p.x, p.y - 25), Scalar(255, 255, 0), 3);
        line(redFrame, Point(p.x + 25, p.y), Point(p.x - 25, p.y), Scalar(255, 255, 0), 3);
        
        //draw hysteresis circle
        circle(redFrame, p, r, Scalar(255, 255, 0));
        
        //draw bounding rectangle
        rectangle(redFrame, objectBoundingRectangle.tl(), objectBoundingRectangle.br(), Scalar(0, 255, 255), 3);
        
        //draw zoom rectangle
        Point tl(xx, yy);
        Point br(xx + zoomedWindow.width, yy + zoomedWindow.height);
        tl = tl * reduceFactor;
        br = br * reduceFactor;
        rectangle(redFrame, tl, br, Scalar(255, 0, 0), 3);
        
        imshow("Movement", redFrame);
    }
    
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
    
    //TODO: calculate age of objects, if very young, do not take into account
    
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
    
    
    while (capture.read(origFrame)) {
        
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

        //set first grayImage to the last one read from camera
        swap(grayImage1, grayImage2);
        
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
        
        //blur the image to get rid of the noise. This will output an intensity image
        blur(thresholdImage, thresholdImage, Size(BLUR_SIZE, BLUR_SIZE));
        
        //threshold again to obtain binary image from blur output
        threshold(thresholdImage, thresholdImage, SENSITIVITY_VALUE, 255, THRESH_BINARY);
        
        if (showDifference) {
            //show the difference image and the threshold image
            imshow("Difference Image", differenceImage);
            imshow("Threshold Image", thresholdImage);
        }
        
        //search for movement in our thresholded image
        trackObjects(thresholdImage, origFrame, zoomedImage, frame);
        
        if (showOutput) {
            imshow("Zoomed Image", zoomedImage);
        }
        
        outVideo.write(zoomedImage);
        
        //update file size / frame count
        frameCount++;
        
        if (showDifference) {
            //show the threshold image after it's been "blurred"
            imshow("Final Threshold Image", thresholdImage);
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


