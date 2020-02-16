//
//  MotionTest.swift
//  MotionTest
//
//  Created by Andreas Pohl on 01.01.20.
//  Copyright © 2020 Andreas Pohl. All rights reserved.
//

import XCTest
import Foundation

class MotionTest: XCTestCase {
    
    override func setUp() {
        // Put setup code here. This method is called before the invocation of each test method in the class.
    }
    
    override func tearDown() {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
    }
    
    func testMotion() {
        let testMovies = [
            1: "1 new.mov",
            2: "2 new.mov",
            3: "3 new.mov",
            4: "4 new.mov",
            5: "10sec new.mov",
            6: "2019-12-10 10-08-10 CAM0 new.mov",
            7: "2020-01-05 11-30-00 CAM0 new.mov",
            8: "2020-01-18 10-39-41 CAM0 new.mov",
            9: "2020-02-06 13-48-11 CAM0 new.mov",
            10: "2020-02-10 13-27-02 CAM0 new.mov",
            11: "2020-02-15 15-29-11 CAM0 new.mov",
            12: "fast new.mov"
        ]
        
        let movieToTest = 12
        let movieName = testMovies[movieToTest]!

        let basePath = "/Users/andreas/Movies/AVRecorderTest/"
        let tempPath = basePath + "/temp/"
        let sourceName = basePath + movieName
        let tempName = tempPath + movieName
        let fileManager = FileManager()
        do {
            try fileManager.copyItem(at: URL(fileURLWithPath: sourceName), to: URL(fileURLWithPath: tempName));
        } catch let moveError as NSError {
            print(moveError.localizedDescription)
        }
        MotionWrapper().processVideoDebug(tempName)
    }
    
    func testExample() {
        // This is an example of a functional test case.
        // Use XCTAssert and related functions to verify your tests produce the correct results.
    }
    
}
