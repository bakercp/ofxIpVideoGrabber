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

#include <iostream>

#include "ofMain.h"

#include "Poco/Exception.h"
#include "Poco/Path.h"
#include "Poco/StreamTokenizer.h"
#include "Poco/StreamCopier.h"
#include "Poco/ScopedLock.h"
#include "Poco/Token.h"
#include "Poco/URI.h"

#include "Poco/Net/HTTPBasicCredentials.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPCookie.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPStreamFactory.h"
#include "Poco/Net/MessageHeader.h"
#include "Poco/Net/NameValueCollection.h"
#include "Poco/Net/NetException.h"

using namespace Poco;
using namespace Poco::Net;

typedef Poco::FastMutex::ScopedLock ofxScopedLock;

class ofxIpVideoGrabber : public ofBaseVideoDraws, protected ofThread {
public:
    
    Poco::FastMutex myMutex;
    
    ofxIpVideoGrabber();
    virtual ~ofxIpVideoGrabber();
    
    void exit(ofEventArgs & a);

    void update();
    
    void update(ofEventArgs & a);

    void connect();
    void disconnect();
    void waitForDisconnect();

    // ofBaseVideo
	bool isFrameNew();
	void close();
    
    void reset();

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
    
    unsigned long getNumFramesReceived(); // not const b/c we access a mutex
    unsigned long getNumBytesReceived();  // not const b/c we access a mutex

    float getFrameRate();
    float getBitRate();
    
    string getCameraName();
    void setCameraName(const string& cameraName);
    
    // set video URI
    void setURI(const string& uri);
    void setURI(const URI& uri);

    string getURI();
    URI getPocoURI();
    
    // poco uri access
    string getHost();
    string getQuery();
    int getPort();
    string getFragment();
    
    // cookies
    void setCookie(const string& key, const string& value);
    void eraseCookie(const string& key);
    string getCookie(const string& key);

    // basic authentication
    void setUsername(const string& username);
    void setPassword(const string& password);
    
    string getUsername();
    string getPassword();
    
    // proxy server
    
    
    void setUseProxy(bool useProxy);
    void setProxyUsername(const string& username);
    void setProxyPassword(const string& password);
    void setProxyHost(const string& host);
    void setProxyPort(UInt16 port);

    bool   getUseProxy();
    string getProxyUsername();
    string getProxyPassword();
    string getProxyHost();
    UInt16 getProxyPort();
    
    HTTPClientSession& getSessionRef();
    
    bool isConnected();
    
    bool hasConnectionFailed() const;
    
    void setReconnectTimeout(unsigned long ms);
    unsigned long getReconnectTimeout() const; // ms
    bool getNeedsReconnect();
    bool getAutoReconnect() const;
    unsigned long getReconnectCount();
    unsigned long getMaxReconnects() const;
    void setMaxReconnects(unsigned long num);
    unsigned long getAutoRetryDelay();
    void setAutoRetryDelay(unsigned long delay_ms);
    unsigned long getNextAutoRetry();

    void setDefaultBoundaryMarker(const string& boundarMarker);
    string getDefaultBoundaryMarker() ;
        
    ofEvent<ofResizeEventArgs> 	videoResized;
    
protected:
    
    void threadedFunction(); // connect to server
    void imageResized(int width, int height);
    
    
private: 

    string defaultBoundaryMarker_a;
    
    string cameraName_a;

    // credentials
    string username_a;
    string password_a;
    
    bool   bUseProxy_a;
    string proxyUsername_a;
    string proxyPassword_a;
    string proxyHost_a;
    UInt16 proxyPort_a;
    
    //ofPixels pix;
    
    int ci; // current image index
    ofImage image_a[2]; // image double buffer.  this flips
    ofPtr<ofImage> img;
    
    bool isNewFrameLoaded;       // is there a new frame ready to be uploaded to glspace
    bool isBackBufferReady_a;
    
    unsigned long connectTime_a; // init time
    unsigned long elapsedTime_a;
    
    unsigned long nBytes_a;
    unsigned long nFrames_a;
    
    float currentBitRate;
    float currentFrameRate;
    
    float minBitrate; // the minimum acceptable bitrate before reconnecting
    unsigned long lastValidBitrateTime; // the time of the last valid bitrate (will wait for reconnectTime time)
    unsigned long reconnectTimeout; // ms the amount ot time we will wait to reach the min bitrate
    
    
    unsigned long autoRetryDelay_a; // retry delay in ms
    unsigned long nextAutoRetry_a;
    bool connectionFailure; // max reconnects exceeded, is dead.
    bool needsReconnect_a; // needs reconnecting
    bool autoReconnect;  // shoudl automatically reconnect
    unsigned long reconnectCount_a; // the number of reconnects attempted
    unsigned long maxReconnects;  // the maximum number of reconnect attempts that will be made

    
    unsigned long sessionTimeout; // ms
    URI uri_a;
    
    NameValueCollection cookies;
  
};

typedef ofPtr< ofxIpVideoGrabber > ofxSharedIpVideoGrabber;

