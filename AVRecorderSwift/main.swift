//
//  main.swift
//  AVRecorderSwift
//
//  Created by Andreas Pohl on 16.01.16.
//  Copyright © 2016 Andreas Pohl. All rights reserved.
//
// heavily based on:
//
//  CommandLineDemo
//
//  Created by Andrew Madsen on 4/13/15.
//  Copyright (c) 2015 Open Reel Software. All rights reserved.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the
//	"Software"), to deal in the Software without restriction, including
//	without limitation the rights to use, copy, modify, merge, publish,
//	distribute, sublicense, and/or sell copies of the Software, and to
//	permit persons to whom the Software is furnished to do so, subject to
//	the following conditions:
//
//	The above copyright notice and this permission notice shall be included
//	in all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

import Foundation
import ORSSerial

let usbButtonDeviceId = "usbmodem1171481"
let usbButtonDeviceBaudRate = "9600"

let appDelegate = AVRecorderDelegate()

/*
for _ in 1...4 {
appDelegate.startRecording()
sleep(10)
}
appDelegate.stopRecording()
*/

enum ApplicationState {
    case InitializationState
    case WaitingForPortSelectionState([ORSSerialPort])
    case WaitingForBaudRateInputState
    case WaitingForUserInputState
}

// MARK: User prompts

struct UserPrompter {
    func printIntroduction() {
        //print("This program demonstrates the use of ORSSerialPort")
        //print("in a Foundation-based command-line tool.")
        //print("Please see http://github.com/armadsen/ORSSerialPort/\nor email andrew@openreelsoftware.com for more information.\n")
    }
    
    func printPrompt() {
        print("\n> ", terminator: "")
    }
    
    func promptForSerialPort() {
        print("Serial Ports:")
        let availablePorts = ORSSerialPortManager.sharedSerialPortManager().availablePorts
        var i = 0
        for port in availablePorts {
            print("\(i++). \(port.name)")
        }
    }
    
    func promptForBaudRate() {
        print("Baud rate:", terminator: "");
    }
}

class StateMachine : NSObject, ORSSerialPortDelegate {
    var currentState = ApplicationState.InitializationState
    let standardInputFileHandle = NSFileHandle.fileHandleWithStandardInput()
    let prompter = UserPrompter()
    
    var serialPort: ORSSerialPort? {
        didSet {
            serialPort?.delegate = self;
            serialPort?.open()
        }
    }
    
    func runProcessingInput() {
        setbuf(stdout, nil)
        
        /*
        standardInputFileHandle.readabilityHandler = { (fileHandle: NSFileHandle) in
        let data = fileHandle.availableData
        dispatch_async(dispatch_get_main_queue(), { () -> Void in self.handleUserInput(data) })
        }
        */
        
        prompter.printIntroduction()
        
        let availablePorts = ORSSerialPortManager.sharedSerialPortManager().availablePorts
        if availablePorts.count == 0 {
            print("No connected serial ports found. Please connect your USB to serial adapter(s) and run the program again.")
            exit(EXIT_SUCCESS)
        }
        prompter.promptForSerialPort()
        //currentState = .WaitingForPortSelectionState(availablePorts)
        
        var portNumber = 0
        for port in availablePorts {
            print("\(port.name)")
            if port.name.containsString(usbButtonDeviceId) {
                break //device found
            }
            portNumber++
        }
        
        setupAndOpenPortWithSelectionString(String(portNumber), availablePorts: availablePorts)
        setBaudRateOnPortWithString(usbButtonDeviceBaudRate)
        
        currentState = .WaitingForUserInputState
        
        NSRunLoop.currentRunLoop().run() // Required to receive data from ORSSerialPort and to process user input
    }
    
    // MARK: Port Settings
    func setupAndOpenPortWithSelectionString(var selectionString: String, availablePorts: [ORSSerialPort]) -> Bool {
        selectionString = selectionString.stringByTrimmingCharactersInSet(NSCharacterSet.whitespaceAndNewlineCharacterSet())
        if let index = Int(selectionString) {
            let clampedIndex = min(max(index, 0), availablePorts.count-1)
            self.serialPort = availablePorts[clampedIndex]
            return true
        } else {
            return false
        }
    }
    
    func setBaudRateOnPortWithString(var selectionString: String) -> Bool {
        selectionString = selectionString.stringByTrimmingCharactersInSet(NSCharacterSet.whitespaceAndNewlineCharacterSet())
        if let baudRate = Int(selectionString) {
            self.serialPort?.baudRate = baudRate
            print("Baud rate set to \(baudRate)")
            return true
        } else {
            return false
        }
    }
    
    // MARK: Data Processing
    func handleUserInput(dataFromUser: NSData) {
        if let string = NSString(data: dataFromUser, encoding: NSUTF8StringEncoding) as? String {
            
            if string.lowercaseString.hasPrefix("exit") ||
                string.lowercaseString.hasPrefix("quit") {
                    print("Quitting...")
                    exit(EXIT_SUCCESS)
            }
            
            switch self.currentState {
            case .WaitingForPortSelectionState(let availablePorts):
                if !setupAndOpenPortWithSelectionString(string, availablePorts: availablePorts) {
                    print("\nError: Invalid port selection.", terminator: "")
                    prompter.promptForSerialPort()
                    return
                }
            case .WaitingForBaudRateInputState:
                if !setBaudRateOnPortWithString(string) {
                    print("\nError: Invalid baud rate. Baud rate should consist only of numeric digits.", terminator: "")
                    prompter.promptForBaudRate();
                    return;
                }
                currentState = .WaitingForUserInputState
                prompter.printPrompt()
            case .WaitingForUserInputState:
                self.serialPort?.sendData(dataFromUser)
                prompter.printPrompt()
            default:
                break;
            }
        }
    }
    
    // MARK: ORSSerialPortDelegate
    
    func serialPort(serialPort: ORSSerialPort, didReceiveData data: NSData) {
        if let string = NSString(data: data, encoding: NSUTF8StringEncoding) {
            print("\nReceived: \"\(string)\"", terminator: "")
            
            if string.containsString("start") {
                print("Aufnahme starten!\n")
                appDelegate.startRecording()
                
            }
            
            if string.containsString("stop") {
                print("Aufnahme stoppen!\n")
                appDelegate.stopRecording()
            }
        }
    }
    
    func serialPortWasRemovedFromSystem(serialPort: ORSSerialPort) {
        self.serialPort = nil
    }
    
    func serialPort(serialPort: ORSSerialPort, didEncounterError error: NSError) {
        print("Serial port (\(serialPort)) encountered error: \(error)")
    }
    
    func serialPortWasOpened(serialPort: ORSSerialPort) {
        print("Serial port \(serialPort) was opened", terminator: "")
        //prompter.promptForBaudRate()
        //currentState = .WaitingForBaudRateInputState
        currentState = .WaitingForUserInputState
    }
}

//run the file handler
dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0)) {
    FileHandler().run()
}

//run the state machine
StateMachine().runProcessingInput()

