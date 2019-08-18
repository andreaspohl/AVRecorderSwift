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

let usbButtonDeviceId = "usbmodem"
let usbButtonDeviceBaudRate = "9600"

let appDelegate = AVRecorderDelegate()

enum ApplicationState {
    case initializationState
    case waitingForPortSelectionState([ORSSerialPort])
    case waitingForBaudRateInputState
    case waitingForUserInputState
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
        let availablePorts = ORSSerialPortManager.shared().availablePorts
        var i = 0
        for port in availablePorts {
            i = i + 1
            print("\(i). \(port.name)")
        }
    }
    
    func promptForBaudRate() {
        print("Baud rate:", terminator: "");
    }
}

class StateMachine : NSObject, ORSSerialPortDelegate {
    
    func serialPortWasRemovedFromSystem(_ serialPort: ORSSerialPort) {
        print("ERROR: SERIAL PORT WAS REMOVED FROM SYSTEM")
    }
    
    var currentState = ApplicationState.initializationState
    let standardInputFileHandle = FileHandle.standardInput
    let prompter = UserPrompter()
    
    var serialPort: ORSSerialPort? {
        didSet {
            serialPort?.delegate = self;
            serialPort?.open()
        }
    }
    
    func runProcessingInput() {
        setbuf(stdout, nil)
        
        standardInputFileHandle.readabilityHandler = { (fileHandle: FileHandle) in
            let data = fileHandle.availableData
            DispatchQueue.main.async {
                self.handleUserInput(data)
            }
        }
        
        prompter.printIntroduction()
        
        let availablePorts = ORSSerialPortManager.shared().availablePorts
        if availablePorts.count == 0 {
            print("No connected serial ports found. Please connect your USB to serial adapter(s) and run the program again.")
            exit(EXIT_SUCCESS)
        }
        prompter.promptForSerialPort()
        //currentState = .WaitingForPortSelectionState(availablePorts)
        
        var portFound = false
        var portNumber = 0
        for port in availablePorts {
            print("\(port.name)")
            if port.name.contains(usbButtonDeviceId) {
                portFound = true
                break //device found
            }
            portNumber += 1
        }
        
        if portFound {
            setupAndOpenPortWithSelectionString(String(portNumber), availablePorts: availablePorts)
            setBaudRateOnPortWithString(usbButtonDeviceBaudRate)
        } else {
            print("no port found, starting anyway")
        }
        
        currentState = .waitingForUserInputState
        
        RunLoop.current.run() // Required to receive data from ORSSerialPort and to process user input
    }
    
    // MARK: Port Settings
    @discardableResult func setupAndOpenPortWithSelectionString(_ selectionString: String, availablePorts: [ORSSerialPort]) -> Bool {
        var selectionString = selectionString
        selectionString = selectionString.trimmingCharacters(in: CharacterSet.whitespacesAndNewlines)
        if let index = Int(selectionString) {
            let clampedIndex = min(max(index, 0), availablePorts.count-1)
            self.serialPort = availablePorts[clampedIndex]
            return true
        } else {
            return false
        }
    }
    
    @discardableResult func setBaudRateOnPortWithString(_ selectionString: String) -> Bool {
        var selectionString = selectionString
        selectionString = selectionString.trimmingCharacters(in: CharacterSet.whitespacesAndNewlines)
        if let baudRate = Int(selectionString) {
            self.serialPort?.baudRate = baudRate as NSNumber
            print("Baud rate set to \(baudRate)")
            return true
        } else {
            return false
        }
    }
    
    // MARK: Data Processing
    func handleUserInput(_ dataFromUser: Data) {
        if let string = NSString(data: dataFromUser, encoding: String.Encoding.utf8.rawValue) as String? {
            
            switch self.currentState {

            case .waitingForPortSelectionState(let availablePorts):
                if !setupAndOpenPortWithSelectionString(string, availablePorts: availablePorts) {
                    print("\nError: Invalid port selection.", terminator: "")
                    prompter.promptForSerialPort()
                    return
                }
            
            case .waitingForBaudRateInputState:
                if !setBaudRateOnPortWithString(string) {
                    print("\nError: Invalid baud rate. Baud rate should consist only of numeric digits.", terminator: "")
                    prompter.promptForBaudRate();
                    return;
                }
                currentState = .waitingForUserInputState
                prompter.printPrompt()
            
            case .waitingForUserInputState:
                //self.serialPort?.send(dataFromUser)
                if string.lowercased().hasPrefix("start") {
                    print("override start")
                    appDelegate.movieNumber = 0 //reset movie count, to prolong session another 90 minutes
                    appDelegate.startRecording()
                }
                
                if string.lowercased().hasPrefix("stop") {
                    print("override stop")
                    appDelegate.stopRecording()
                }
                
                prompter.printPrompt()
            default:
                break;
            }
        }
    }
    
    
    // MARK: ORSSerialPortDelegate
    func serialPort(_ serialPort: ORSSerialPort, didReceive data: Data) {
        if let string = NSString(data: data, encoding: String.Encoding.utf8.rawValue) {
            print("Received: \"\(string)\"", terminator: "")
            
            if string.contains("start") {
                print("Aufnahme starten!")
                appDelegate.movieNumber = 0 //reset movie count, to prolong session another 90 minutes
                appDelegate.startRecording()
            }
            
            if string.contains("stop") {
                print("Aufnahme stoppen!")
                appDelegate.stopRecording()
            }
        }
    }
    
    func serialPortWasRemoved(fromSystem serialPort: ORSSerialPort) {
        self.serialPort = nil
    }
    
    func serialPort(_ serialPort: ORSSerialPort, didEncounterError error: Error) {
        print("Serial port (\(serialPort)) encountered error: \(error)")
    }
    
    func serialPortWasOpened(_ serialPort: ORSSerialPort) {
        print("Serial port \(serialPort) was opened", terminator: "")
        //prompter.promptForBaudRate()
        //currentState = .WaitingForBaudRateInputState
        currentState = .waitingForUserInputState
    }
}

//run the file handler
DispatchQueue.global(qos: DispatchQoS.QoSClass.utility).async {
    FileHandler().run()
}

//run the stitcher
DispatchQueue.global(qos: DispatchQoS.QoSClass.utility).async {
    Stitcher().run()
}

//run the video processor
DispatchQueue.global(qos: DispatchQoS.QoSClass.utility).async {
    VideoProcessor().run()
}


//run the state machine
StateMachine().runProcessingInput()

