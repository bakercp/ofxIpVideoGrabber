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


#include <iostream>
#include "ofMain.h"
#include "Poco/Exception.h"
#include "Poco/Path.h"
#include "Poco/StreamTokenizer.h"
#include "Poco/StreamCopier.h"
#include "Poco/UTF8String.h"
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


namespace ofx {
namespace Video {


class IPVideoGrabber: public ofBaseVideoDraws, protected ofThread
{
public:
    typedef std::shared_ptr<IPVideoGrabber> SharedPtr;
    
    IPVideoGrabber();
    virtual ~IPVideoGrabber();

    void exit(ofEventArgs& a);

    void update();
    
    void update(ofEventArgs& a);

    void connect();
    void disconnect();
    void waitForDisconnect();

    // ofBaseVideo
	bool isFrameNew() const;
	void close();
	bool isInitialized() const;
	bool setPixelFormat(ofPixelFormat pixelFormat);
	ofPixelFormat getPixelFormat() const;

    void reset();

    // ofBaseHasPixels
	ofPixels& getPixels();
	const ofPixels& getPixels() const;

    std::shared_ptr<ofImage> getFrame();
    
    // ofBaseHasTexture
	ofTexture& getTexture();
	const ofTexture& getTexture() const;
	void setUseTexture(bool bUseTex);
	bool isUsingTexture() const;

	// ofBaseHasTexturePlanes
	std::vector<ofTexture>& getTexturePlanes();
	const std::vector<ofTexture>& getTexturePlanes() const;

    // ofBaseDraws
    void draw(float x, float y) const;
	void draw(float x, float y, float w, float h) const;
	void draw(const ofPoint& point) const;
	void draw(const ofRectangle& rect) const;
    
    void setAnchorPercent(float xPct, float yPct);
    void setAnchorPoint(float x, float y);
	void resetAnchor();
    
    float getWidth() const;
    float getHeight() const;
    
    unsigned long getNumFramesReceived(); // not const b/c we access a mutex
    unsigned long getNumBytesReceived();  // not const b/c we access a mutex

    float getFrameRate();
    float getBitRate();
    
    std::string getCameraName();
    void setCameraName(const std::string& cameraName);
    
    // set video URI
    void setURI(const std::string& uri);
    void setURI(const Poco::URI& uri);

    std::string getURI();
    Poco::URI getPocoURI();
    
    // poco uri access
    std::string getHost();
    std::string getQuery();
    int getPort();
    std::string getFragment();
    
    // cookies
    void setCookie(const std::string& key, const std::string& value);
    void eraseCookie(const std::string& key);
    std::string getCookie(const std::string& key);

    // basic authentication
    void setUsername(const std::string& username);
    void setPassword(const std::string& password);
    
    std::string getUsername();// const;
    std::string getPassword();// const;
    
    // proxy server
    void setUseProxy(bool useProxy);
    void setProxyUsername(const std::string& username);
    void setProxyPassword(const std::string& password);
    void setProxyHost(const std::string& host);
    void setProxyPort(Poco::UInt16 port);

    bool getUseProxy(); // const;
    std::string getProxyUsername();// const;
    std::string getProxyPassword();// const;
    std::string getProxyHost();// const;
    Poco::UInt16 getProxyPort();// const;
    
    Poco::Net::HTTPClientSession& getSessionRef();
    
    bool isConnected();
    
    bool hasConnectionFailed() const;
    
    void setReconnectTimeout(unsigned long ms);
    unsigned long getReconnectTimeout() const; // ms
    bool getNeedsReconnect();
    bool getAutoReconnect() const;
    unsigned long getReconnectCount();
    unsigned long getMaxReconnects() const;
    void setMaxReconnects(unsigned long num);
    unsigned long getAutoRetryDelay(); // const ofThread ...
    void setAutoRetryDelay(unsigned long delay_ms);
    unsigned long getNextAutoRetryTime(); // const ofThread...;
    unsigned long getTimeTillNextAutoRetry(); // const ofThread...;

    void setDefaultBoundaryMarker(const std::string& boundarMarker);
    std::string getDefaultBoundaryMarker(); // const;
        
    ofEvent<ofResizeEventArgs> 	videoResized;

protected:    
    void threadedFunction() override;// connect to server
    void imageResized(int width, int height);
    
private:
    std::string defaultBoundaryMarker_a;
    
    std::string cameraName_a;

    // credentials
    std::string username_a;
    std::string password_a;
    
    bool bUseProxy_a;
    std::string proxyUsername_a;
    std::string proxyPassword_a;
    std::string proxyHost_a;
    Poco::UInt16 proxyPort_a;
    
    //ofPixels pix;
    
    int ci; // current image index
    ofPixels image_a[2]; // image double buffer.  this flips
    std::shared_ptr<ofImage> img;
	mutable std::vector<ofTexture> tex;

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
    bool autoReconnect;  // should automatically reconnect
    unsigned long reconnectCount_a; // the number of reconnects attempted
    unsigned long maxReconnects;  // the maximum number of reconnect attempts that will be made

    
    unsigned long sessionTimeout; // ms
    Poco::URI uri_a;
    
    Poco::Net::NameValueCollection cookies;

};


typedef IPVideoGrabber::SharedPtr SharedIPVideoGrabber;


} } // namespace ofx::Video
