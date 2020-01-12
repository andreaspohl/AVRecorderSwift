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
    ObjectHandler(Mat centers);
    vector<Point2f> update(Mat centers);
    
private:
    vector<Point2f> objects;
    
    void mergeObjects();


};

#endif /* ObjectHandler_hpp */
