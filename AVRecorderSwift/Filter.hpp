//
//  Filter.hpp
//  AVRecorderSwift
//
//  Created by Andreas Pohl on 07.01.20.
//  Copyright © 2020 Andreas Pohl. All rights reserved.
//

#ifndef Filter_hpp
#define Filter_hpp

#include <stdio.h>

class Filter {
private:
    double initial;
    double x;
    double vx;
    double ax;

public:
    Filter(int x);
    int update(int x);
};
#endif /* Filter_hpp */
