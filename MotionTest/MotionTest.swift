//
//  MotionTest.swift
//  MotionTest
//
//  Created by Andreas Pohl on 01.01.20.
//  Copyright © 2020 Andreas Pohl. All rights reserved.
//

import XCTest

class MotionTest: XCTestCase {

    override func setUp() {
        // Put setup code here. This method is called before the invocation of each test method in the class.
    }

    override func tearDown() {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
    }

    func testMotion() {
//        MotionWrapper().processVideoDebug("/Users/andreas/Movies/AVRecorderTest/2019-12-10 10-08-10 CAM0 new.mov")
//        MotionWrapper().processVideoDebug("/Users/andreas/Movies/AVRecorderTest/1 new.mov")
//        MotionWrapper().processVideoDebug("/Users/andreas/Movies/AVRecorderTest/2 new.mov")
        MotionWrapper().processVideoDebug("/Users/andreas/Movies/AVRecorderTest/3 new.mov")
    }
    
    func testExample() {
        // This is an example of a functional test case.
        // Use XCTAssert and related functions to verify your tests produce the correct results.
    }

}
