//
// Copyright (c) 2020 Christopher Baker <https://christopherbaker.net>
//
// SPDX-License-Identifier: MIT
//


#pragma once


#include "ofMain.h"
#include "IPVideoGrabber.h"


class ofApp: public ofBaseApp
{
public:
    void setup() override;
    void update() override;
    void draw() override;

    void keyPressed(int key) override;
    
    ofx::Video::IPVideoGrabber grabber;

    // These are modified copies of the pixels and the textures.
    ofPixels cameraPix;
    ofTexture cameraTex;
    
    // Set the display size for the cameras.
    std::size_t cameraWidth = 640;
    std::size_t cameraHeight = 480;
    
    bool showStats = false;
};
