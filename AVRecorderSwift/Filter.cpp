//
//  Filter.cpp
//  AVRecorderSwift
//
//  Created by Andreas Pohl on 07.01.20.
//  Copyright © 2020 Andreas Pohl. All rights reserved.
//

#include "Filter.hpp"

#include <iostream>

using namespace std;

const double MASS = 10.0;
const double FRICTION = 1.0;
const double MAX_V = 10;
const double MAX_MINUS_V = 2; //value can grow with MAX_V, but shrink only with MAX_MINUS_V

Filter::Filter(int in, enum BorderType border) {
    x = in;
    vx = 0;
    ax = 0;
    borderType = border;
}

int Filter::update(int in) {
    ax = (in - x) / MASS;
    vx = vx + ax/2 - (vx * FRICTION);
    
    if (borderType == BorderType::LEFT) {
        if (vx > MAX_MINUS_V) {
            vx = MAX_MINUS_V;
        } else if (vx < -MAX_V) {
            vx = -MAX_V;
        }
    } else {
        if (vx > MAX_V) {
            vx = MAX_V;
        } else if (vx < -MAX_MINUS_V) {
            vx = -MAX_MINUS_V;
        }
    }
    
    x = x + vx;
    return (int) x;
}
