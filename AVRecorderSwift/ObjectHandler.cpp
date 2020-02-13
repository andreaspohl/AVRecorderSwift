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

//add 1 to every objects age
void ObjectHandler::ageObjects() {
    for (auto object = objects.begin(); object != objects.end(); ++object) {
        (*object).age++;
        //limit max age
        if ((*object).age > 100) {
            (*object).age = 100;
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
        for (auto center = centers.begin(); center != centers.end(); ++center) {
            if (overlaps((*object), (*center))) {
                //inherit age from overlapping object
                if ((*object).age > (*center).age) {
                    (*center).age = (*object).age;
                }
                objects.erase(object--); //-- is necessary to set iterator back to a not erased instance
            }
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
        if ((*object).age > 5) {
            objectPoints.push_back((*object).point);
        }
        
    }
    
    return objectPoints;
}
