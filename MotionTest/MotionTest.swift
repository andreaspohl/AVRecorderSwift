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
            6: "fast entry new.mov",
            7: "nothing new.mov",
            8: "fast new.mov"
        ]
        
        let movieToTest = 8
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
