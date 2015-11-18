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

    void update() override;
    
    void update(ofEventArgs& a);

    void connect() ;
    void disconnect();
    void waitForDisconnect();

    // ofBaseVideo
	bool isFrameNew() const override;
	void close() override;
	bool isInitialized() const override;
	bool setPixelFormat(ofPixelFormat pixelFormat) override;
	ofPixelFormat getPixelFormat() const override;

    void reset();

    // ofBaseHasPixels
	ofPixels& getPixels() override;
	const ofPixels& getPixels() const override;

    std::shared_ptr<ofImage> getFrame();
    
    // ofBaseHasTexture
	ofTexture& getTexture() override;
	const ofTexture& getTexture() const override;
	void setUseTexture(bool bUseTex) override;
	bool isUsingTexture() const override;

	// ofBaseHasTexturePlanes
	std::vector<ofTexture>& getTexturePlanes() override;
	const std::vector<ofTexture>& getTexturePlanes() const override;

    // ofBaseDraws
    void draw(float x, float y) const override;
	void draw(float x, float y, float w, float h) const override;
	void draw(const ofPoint& point) const override;
	void draw(const ofRectangle& rect) const override;
    
    void setAnchorPercent(float xPct, float yPct) override;
    void setAnchorPoint(float x, float y) override;
	void resetAnchor() override;
    
    float getWidth() const override;
    float getHeight() const override;
    
    uint64_t getNumFramesReceived() const;
    uint64_t getNumBytesReceived() const;

    float getFrameRate() const;
    float getBitRate() const;
    
    std::string getCameraName() const;
    void setCameraName(const std::string& cameraName);
    
    // set video URI
    void setURI(const std::string& uri);
    void setURI(const Poco::URI& uri);

    std::string getURI() const;
    Poco::URI getPocoURI() const;
    
    // poco uri access
    std::string getHost() const;
    std::string getQuery() const;
    uint16_t getPort() const;
    std::string getFragment() const;
    
    // cookies
    void setCookie(const std::string& key, const std::string& value);
    void eraseCookie(const std::string& key);
    std::string getCookie(const std::string& key) const;

    // basic authentication
    void setUsername(const std::string& username);
    void setPassword(const std::string& password);
    
    std::string getUsername() const;
    std::string getPassword() const;
    
    // proxy server
    void setUseProxy(bool useProxy);
    void setProxyUsername(const std::string& username);
    void setProxyPassword(const std::string& password);
    void setProxyHost(const std::string& host);
    void setProxyPort(uint16_t port);

    bool getUseProxy() const;
    std::string getProxyUsername() const;
    std::string getProxyPassword() const;
    std::string getProxyHost() const;
    Poco::UInt16 getProxyPort() const;
    
    Poco::Net::HTTPClientSession& getSession();
    
    bool isConnected() const;
    
    bool hasConnectionFailed() const;
    
    void setReconnectTimeout(uint64_t ms);
    uint64_t getReconnectTimeout() const;
    bool getNeedsReconnect() const;
    bool getAutoReconnect() const;
    uint64_t getReconnectCount() const;
    uint64_t getMaxReconnects() const;
    void setMaxReconnects(uint64_t num);
    uint64_t getAutoRetryDelay() const;
    void setAutoRetryDelay(uint64_t delay_ms);
    uint64_t getNextAutoRetryTime() const;
    uint64_t getTimeTillNextAutoRetry() const;

    void setDefaultBoundaryMarker(const std::string& boundarMarker);
    std::string getDefaultBoundaryMarker() const;
        
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
    uint16_t proxyPort_a;
    
    //ofPixels pix;
    
    int ci; // current image index
    ofPixels image_a[2]; // image double buffer.  this flips
    std::shared_ptr<ofImage> img;
	mutable std::vector<ofTexture> tex;

    bool isNewFrameLoaded;       // is there a new frame ready to be uploaded to glspace
    bool isBackBufferReady_a;
    
    uint64_t connectTime_a; // init time
    uint64_t elapsedTime_a;
    
    uint64_t nBytes_a;
    uint64_t nFrames_a;
    
    float currentBitRate;
    float currentFrameRate;
    
    float minBitrate; // the minimum acceptable bitrate before reconnecting
    uint64_t lastValidBitrateTime; // the time of the last valid bitrate (will wait for reconnectTime time)
    uint64_t reconnectTimeout; // ms the amount ot time we will wait to reach the min bitrate
    
    
    uint64_t autoRetryDelay_a; // retry delay in ms
    uint64_t nextAutoRetry_a;
    bool connectionFailure; // max reconnects exceeded, is dead.
    bool needsReconnect_a; // needs reconnecting
    bool autoReconnect;  // should automatically reconnect
    uint64_t reconnectCount_a; // the number of reconnects attempted
    uint64_t maxReconnects;  // the maximum number of reconnect attempts that will be made

    
    uint64_t sessionTimeout; // ms
    Poco::URI uri_a;
    
    Poco::Net::NameValueCollection cookies;

	const static std::size_t MIN_JPEG_SIZE;
	const static std::size_t BUF_LEN;
	const static char JFF;
	const static char SOI;
	const static char EOI;
};


typedef IPVideoGrabber::SharedPtr SharedIPVideoGrabber;


} } // namespace ofx::Video
