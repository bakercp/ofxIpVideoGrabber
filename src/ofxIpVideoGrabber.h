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
#include "Poco/Token.h"
#include "Poco/URI.h"

enum ConnectionState {
  DISCONNECTED,
  CONNECTING,
  CONNECTED,
  DISCONNECTING
};
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

class ofxIpVideoGrabber : public ofBaseVideoDraws, protected ofThread {
public:
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
    
    void setName(const string& name);
    string getName();
    
    // set video URI
    void setURI(const string& uri);
    void setURI(const URI& uri);

    string getURI() const;
    URI getPocoURI() const;
    
    // poco uri access
    string getHost() const;
    string getQuery() const;
    int getPort() const;
    string getFragment() const;
    
    // cookies
    void setCookie(const string& key, const string& value);
    void eraseCookie(const string& key);
    string getCookie(const string& key) const;

    // basic authentication
    void setUsername(const string& username);
    void setPassword(const string& password);
    
    string getUsername() const;
    string getPassword() const;
    
    // proxy server
    void setProxyUsername(const string& username);
    void setProxyPassword(const string& password);
    void setProxyHost(const string& host);
    void setProxyPort(int port);

    string getProxyUsername() const;
    string getProxyPassword() const;
    string getProxyHost() const;
    int    getProxyPort() const;
    
    HTTPClientSession* getSession();
    
    bool isConnected() const;
    bool isDisconnected() const;
    bool isConnecting() const;
    bool isDisconnecting() const;
    
    ofEvent<ofResizeEventArgs> 	videoResized;
    
protected:
    
    void threadedFunction(); // connect to server
    void imageResized(int width, int height);
    void resetStats();
    
    
private: 

    string name;
    
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
    
    ConnectionState connectionState;
    
    
    HTTPClientSession session;

    NameValueCollection cookies;
    
};
