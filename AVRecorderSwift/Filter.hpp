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
    
public:
    enum class BorderType {
        LEFT,
        RIGHT,
        BOTTOM,
        NONE
    };
    Filter(double x, BorderType border);
    double update(double x);
    double getValue();
    
private:
    double x;
    double vx;
    double ax;
    BorderType borderType;
    
};
#endif /* Filter_hpp */
