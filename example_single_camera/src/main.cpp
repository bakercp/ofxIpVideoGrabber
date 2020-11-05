//
// Copyright (c) 2020 Christopher Baker <https://christopherbaker.net>
//
// SPDX-License-Identifier: MIT
//


#include "ofApp.h"


int main()
{
	ofSetupOpenGL(640 * 2, 480 * 1, OF_WINDOW);
    return ofRunApp(std::make_shared<ofApp>());
}
