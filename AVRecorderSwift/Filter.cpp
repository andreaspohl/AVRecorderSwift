//
//  Filter.cpp
//  AVRecorderSwift
//
//  Created by Andreas Pohl on 07.01.20.
//  Copyright © 2020 Andreas Pohl. All rights reserved.
//

#include "Filter.hpp"

#include <iostream>
#include <cmath>

using namespace std;

const double SPRING = 1; // translates pixel distance into force
const double MASS = 20;
const double FRICTION = 10;
const double MAX_V = 50;
const double MAX_MINUS_V = .1; //value can grow with MAX_V, but shrink only with MAX_MINUS_V
const double BORDER_HYSTERESIS = 10;

Filter::Filter(int in, enum BorderType border) {
    x = in;
    vx = 0;
    ax = 0;
    borderType = border;
}

int Filter::update(double in) {
    
    //hysteresis
    if (vx < BORDER_HYSTERESIS and abs(in - x) < BORDER_HYSTERESIS) {
        in = x;
    }
    
    ax = SPRING * (in - x) / MASS;
    vx = vx / FRICTION + ax;
    
    if (borderType == BorderType::LEFT) {
        if (vx > MAX_MINUS_V) {
            vx = MAX_MINUS_V;
        } else if (vx < -MAX_V) {
            vx = -MAX_V;
        }
    } else if (borderType == BorderType::RIGHT or borderType == BorderType::BOTTOM) {
        if (vx > MAX_V) {
            vx = MAX_V;
        } else if (vx < -MAX_MINUS_V) {
            vx = -MAX_MINUS_V;
        }
    } else {
        if (vx > MAX_V) {
            vx = MAX_V;
        } else if (vx < -MAX_V) {
            vx = -MAX_V;
        }
    }
    
    x = x + vx + ax / 2;
    return (int) x;
}
