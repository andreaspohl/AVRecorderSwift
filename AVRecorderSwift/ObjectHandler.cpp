//
//  ObjectHandler.cpp
//  AVRecorderSwift
//
//  Created by Andreas Pohl on 12.01.20.
//  Copyright © 2020 Andreas Pohl. All rights reserved.
//

#include "ObjectHandler.hpp"

const int ISOLATION = 100; //distance over which separate objects are recognized

void matToVector(Mat in, vector<Point2f> &out) {
    for (int i = 0; i < in.rows; i++) {
        Point2f c = in.at<Point2f>(i);
        out.push_back(c);
    }
}

//checks if p1 is overlapping with any of the centers (distance ISOLATION)
bool overlaps(Point2f p1, vector<Point2f> centers) {
    static int isolationSqare = pow(ISOLATION,2);
    int centerCount = (int) centers.size();
    for (int i = 0; i < centerCount; i++) {
        Point2f p2 = centers[i];
        int distanceSquare = pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2);
        if (distanceSquare < isolationSqare) {
            return true;
        }
    }
    return false;
}

void addCentersToObjects(vector<Point2f> centers, vector<Point2f> &objects) {
    int centerCount = (int) centers.size();
    for (int i = 0; i < centerCount; i ++) {
        Point2f c = centers.at(i);
        objects.push_back(c);
    }
}

//expects cluster centers as a n x 1 Mat with 2f entries
ObjectHandler::ObjectHandler() {
}

//compare previous cluster centers with new ones
//if overlapping with any of the new ones, discard previous one
//if not overlapping --> keep
//in the end, add all new centers to new previous ones
vector<Point2f> ObjectHandler::update(Mat clusterCenters) {
    
    vector<Point2f> centers;
    matToVector(clusterCenters, centers);
    
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        if (overlaps(*it, centers)) {
            objects.erase(it--); //-- is necessary to set iterator back to a not erased instance
        }
    }
    
    addCentersToObjects(centers, objects);
    
    return objects;
}

vector<Point2f> ObjectHandler::getObjects() {
    return objects;
}
