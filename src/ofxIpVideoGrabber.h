/*==============================================================================
 
 Copyright (c) 2011, 2012 Christopher Baker <http://christopherbaker.net>
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 
 ==============================================================================*/

#pragma once

#include "ofMain.h"

#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPStreamFactory.h"
#include "Poco/Net/HTTPBasicCredentials.h"
#include "Poco/Net/MessageHeader.h"
#include "Poco/Net/NetException.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/StreamCopier.h"
#include "Poco/Path.h"
#include "Poco/URI.h"
#include "Poco/Exception.h"

#include "Poco/Net/NameValueCollection.h"

#include "Poco/StreamTokenizer.h"
#include "Poco/Token.h"

#include <iostream>

using Poco::Net::HTTPClientSession;
using Poco::Net::HTTPStreamFactory;
using Poco::Net::HTTPBasicCredentials;
using Poco::Net::MessageHeader;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPMessage;
using Poco::Net::NameValueCollection;
using Poco::Net::NoMessageException;
using Poco::StreamCopier;
using Poco::Path;
using Poco::URI;
using Poco::Exception;

using Poco::StreamTokenizer;
using Poco::Token;
using Poco::trim;
using Poco::trimRightInPlace;

using Poco::icompare;

class ofxIpVideoGrabber : public ofBaseVideoDraws, protected ofThread {
public:
    ofxIpVideoGrabber();
    virtual ~ofxIpVideoGrabber();
    
    void update();
    
    void update(ofEventArgs & a);
    void exit(ofEventArgs & a);

    void connect();
    void disconnect();
    void waitForDisconnect();

    // ofBaseVideo
	bool isFrameNew();
	void close();

    // ofBaseHasPixels
	unsigned char * getPixels();
	ofPixelsRef getPixelsRef();
    
    ofPtr<ofImage> getFrame();
    
    // ofBaseHasTexture
    ofTexture & getTextureReference();
	void setUseTexture(bool bUseTex);

    // ofBaseDraws
    void draw(float x,float y);
	void draw(float x,float y,float w, float h);
	void draw(const ofPoint & point);
	void draw(const ofRectangle & rect);
    
    void setAnchorPercent(float xPct, float yPct);
    void setAnchorPoint(float x, float y);
	void resetAnchor();
    
    float getWidth();
    float getHeight();
    
    float getFrameRate();
    float getBitRate();
    
    // set video URI
    void setURI(string uri);
    void setURI(URI uri);

    string getURI();
    
    // basic authentication
    void setUsername(string username);
    void setPassword(string password);
    
    string getUsername();
    string getPassword();
    
    // proxy server
    void setProxyUsername(string username);
    void setProxyPassword(string password);
    void setProxyHost(string host);
    void setProxyPort(int port);

    string getProxyUsername();
    string getProxyPassword();
    string getProxyHost();
    int    getProxyPort();
    
    HTTPClientSession* getSession();
    
    bool isConnected();
    
    ofEvent<ofResizeEventArgs> 	videoResized;
    
protected:
    
    void threadedFunction(); // connect to server
    void imageResized(int width, int height);
    void resetStats();
    
private: 

    
    int ci; // current image index
    ofPtr<ofImage> image[2]; // image double buffer.  this flips
    ofPtr<ofImage> img;
    bool isNewFrameLoaded;       // is there a new frame ready to be uploaded to glspace
    bool isBackBufferReady;
    
    unsigned long t0; // init time
    unsigned long elapsedTime;
    
    unsigned long nBytes;
    unsigned long lastByteTime;
    float bps; // bytes / second
    
    unsigned long nFrames;
    unsigned long lastFrameTime;
    
    URI     uri;
    HTTPBasicCredentials credentials;
    
    bool bIsConnected;
    
    HTTPClientSession session;
    
};
