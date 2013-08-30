// =============================================================================
//
// Copyright (c) 2009-2013 Christopher Baker <http://christopherbaker.net>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// =============================================================================


#pragma once


#include "ofMain.h"
#include "ofxXmlSettings.h"
#include "ofxIpVideoGrabber.h"


#if defined(TARGET_OF_IPHONE) || defined(TARGET_ANDROID) || defined(TARGET_LINUX_ARM)
    #define NUM_CAMERAS 1
    #define NUM_ROWS 1
    #define NUM_COLS 1
#else
    #define NUM_CAMERAS 9
    #define NUM_ROWS 3
    #define NUM_COLS 3
#endif


class IPCameraDef
{
public:
    IPCameraDef()
    {
    }

    IPCameraDef(const std::string& _url): url(_url)
    {
    }
    
    IPCameraDef(const std::string& _name,
                const std::string& _url,
                const std::string& _username,
                const std::string& _password):
        name(_name),
        url(_url),
        username(_username),
        password(_password)
    {
    }
    
    std::string name;
    std::string url;
    std::string username;
    std::string password;
};

class ofApp: public ofBaseApp
{
public:
    void setup();
    void update();
    void draw();
    
    void keyPressed(int key);

    std::vector<ofxSharedIpVideoGrabber> ipGrabber;

    void loadCameras();
    IPCameraDef& getNextCamera();
    std::vector<IPCameraDef> ipcams; // a list of IPCameras
    int nextCamera;

    // This message occurs when the incoming video stream image size changes. 
    // This can happen if the IPCamera has a single broadcast state (some cheaper IPCams do this)
    // and that broadcast size is changed by another user. 
    void videoResized(const void* sender, ofResizeEventArgs& arg);
    
};
