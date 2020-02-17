//
//  ObjectHandler.cpp
//  AVRecorderSwift
//
//  Created by Andreas Pohl on 12.01.20.
//  Copyright © 2020 Andreas Pohl. All rights reserved.
//

#include "ObjectHandler.hpp"

const int ISOLATION = 60; //distance over which separate objects are recognized
const int BORDER_ZONE = 10; //no storing of objects near the image borders (object may have left the frame)
const int TOO_YOUNG = 10; //ignore objects younger than TOO_YOUNG frames / lifes
const int MAX_LIFES = 900; //maximum frames (aka lifes) an object can accumulate (and can live withouth moving) (900 / 15 = 60 seconds)

void ObjectHandler::matToVector(Mat in, vector<Object> &out) {
    for (int i = 0; i < in.rows; i++) {
        Object object;
        object.point = in.at<Point2f>(i);
        out.push_back(object);
    }
}

//checks if p1 is overlapping with the given center (distance ISOLATION)
bool ObjectHandler::overlaps(Object object, Object center) {
    static int isolationSqare = pow(ISOLATION,2);
    Point2f p1 = object.point;
    Point2f p2 = center.point;
    int distanceSquare = pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2);
    if (distanceSquare < isolationSqare) {
        return true;
    }
    return false;
}

//add given centers to objects
void ObjectHandler::addCentersToObjects(vector<Object> centers) {
    for (auto center = centers.begin(); center != centers.end(); ++center) {
        Point2f c = (*center).point;
        //only add if not too near to margins, and only if in the middle band of the image
        if (c.x > BORDER_ZONE and c.x < width - BORDER_ZONE and c.y < height * 0.75 and c.y > height * 0.25) {
            objects.push_back((*center));
        }
    }
}

//expects cluster centers as a n x 1 Mat with 2f entries
ObjectHandler::ObjectHandler(int inWidth, int inHeight) {
    width = inWidth;
    height = inHeight;
}

//to gradually eliminate non moving objects, remove one life from all objects
void ObjectHandler::ageObjects() {
    for (auto object = objects.begin(); object != objects.end(); ++object) {
        (*object).lifes--;
        //remove old non moving objects, might have been be some dust...
        if ((*object).lifes < 0 ) {
            objects.erase(object--);
        }
    }
}


//compare previous cluster centers with new ones
//if overlapping with any of the new ones, discard previous one
//if not overlapping --> keep
//in the end, add all new centers to new previous ones
vector<Point2f> ObjectHandler::update(Mat clusterCenters) {
    
    vector<Object> centers;
    matToVector(clusterCenters, centers);
    
    //iterate through all objects
    for (auto object = objects.begin(); object != objects.end(); ++object) {
        //and check if any center is overlapping
        bool overlapping = false;
        for (auto center = centers.begin(); center != centers.end(); ++center) {
            if (overlaps((*object), (*center))) {
                overlapping = true;
                //inherit lifes from overlapping object, if larger than own
                if ((*object).lifes >= (*center).lifes - 1) {
                    (*center).lifes = (*object).lifes + 2; //increase lifes by 2, as later 1 is reduced for general aging
                    if ((*object).lifes > MAX_LIFES) {
                        (*object).lifes = MAX_LIFES; //limit lifes, so non moving objects are removed after MAX_LIFES
                    }
                }
            }
        }
        if (overlapping) {
            objects.erase(object--); //-- is necessary to set iterator back to a not erased instance
        }
    }
    
    addCentersToObjects(centers);
    ageObjects();
    
    vector<Point2f> objectPoints;
    
    for (auto object = objects.begin(); object != objects.end(); ++object) {
        objectPoints.push_back((*object).point);
    }
    
    return getObjects();
}

vector<Point2f> ObjectHandler::getObjects() {
    vector<Point2f> objectPoints;
    
    for (auto object = objects.begin(); object != objects.end(); ++object) {
        //only return older objects
        if ((*object).lifes > TOO_YOUNG) {
            objectPoints.push_back((*object).point);
        }
        
    }
    
    return objectPoints;
}
