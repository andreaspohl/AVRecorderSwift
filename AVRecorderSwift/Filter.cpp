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

Filter::Filter(int in) {
    initial = in;
    x = in;
    vx = 0;
    ax = 0;
}

int Filter::update(int in) {
    ax = (in - x) / MASS;
    vx = vx + ax/2 - (vx * FRICTION);
    if (vx > MAX_V) {
        vx = MAX_V;
    } else if (vx < -MAX_V) {
        vx = -MAX_V;
    }
    x = x + vx;
    return (int) x;
}
