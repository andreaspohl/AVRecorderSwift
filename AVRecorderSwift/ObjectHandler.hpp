//
//  ObjectHandler.hpp
//  AVRecorderSwift
//
//  Created by Andreas Pohl on 12.01.20.
//  Copyright © 2020 Andreas Pohl. All rights reserved.
//

#ifndef ObjectHandler_hpp
#define ObjectHandler_hpp

#include <stdio.h>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio/videoio.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

class ObjectHandler {
    
public:
    ObjectHandler(int width, int height);
    vector<Point2f> update(Mat centers);
    vector<Point2f> getObjects();
    
private:
    
    struct Object {
        Point2f point; // center of the object
        int age; // age (in number of frames) of the object
    };
    
    vector<Object> objects;

    int width, height;
    
    void mergeObjects();
    void addCentersToObjects(vector<Object> centers);
    void matToVector(Mat in, vector<Object> &out);
    bool overlaps(Point2f p1, vector<Object> centers);


};

#endif /* ObjectHandler_hpp */
