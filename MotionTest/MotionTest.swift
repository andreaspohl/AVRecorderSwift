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
        MotionWrapper().processVideoWrapped("/nobpath")
    }
    
    func testExample() {
        // This is an example of a functional test case.
        // Use XCTAssert and related functions to verify your tests produce the correct results.
    }

}