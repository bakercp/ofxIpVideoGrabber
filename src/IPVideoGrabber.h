//
// Copyright (c) 2009 Christopher Baker <https://christopherbaker.net>
//
// SPDX-License-Identifier:    MIT
//


#pragma once


#include <atomic>
#include <string>
#include "Poco/URI.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/NameValueCollection.h"
#include "ofJson.h"
#include "ofPixels.h"
#include "ofRectangle.h"
#include "ofVideoBaseTypes.h"


namespace ofx {
namespace Video {


class IpVideoGrabberSettings
{
public:
    enum class AuthType
    {
        NONE,
        BASIC,
        COOKIE
    };

    IpVideoGrabberSettings()
    {
    }

    IpVideoGrabberSettings(const std::string& url): _url(url)
    {
    }

    IpVideoGrabberSettings(const std::string& name,
                           const std::string& url,
                           const std::string& username,
                           const std::string& password,
                           const AuthType authType):
        _name(name),
        _url(url),
        _username(username),
        _password(password),
        _authType(authType)
    {
    }

    void setName(const std::string& name) { _name = name; }
    std::string getName() const { return _name; }

    void setURL(const std::string& url) { _url = url; }
    std::string getURL() const { return _url; }

    void setUsername(const std::string& username) { _username = username; }
    std::string getUsername() const { return _username; }

    void setPassword(const std::string& password) { _password = password; }
    std::string getPassword() const { return _password; }

    void setAuthType(AuthType authType) { _authType = authType; }
    AuthType getAuthType() const { return _authType; }


    static IpVideoGrabberSettings fromJSON(const ofJson& json)
    {
        IpVideoGrabberSettings stream;

        std::string auth = json.value("auth-type", "NONE");

        IpVideoGrabberSettings::AuthType authType = IpVideoGrabberSettings::AuthType::NONE;

        if (auth == "NONE")
        {
            authType = IpVideoGrabberSettings::AuthType::NONE;
        }
        else if (auth == "BASIC")
        {
            authType = IpVideoGrabberSettings::AuthType::BASIC;
        }
        else if (auth == "COOKIE")
        {
            authType = IpVideoGrabberSettings::AuthType::COOKIE;
        }

        return IpVideoGrabberSettings(json.value("name", ""),
                                      json.value("url", ""),
                                      json.value("username", ""),
                                      json.value("password", ""),
                                      authType);
    }

    static std::vector<IpVideoGrabberSettings> fromFile(const std::string& json)
    {
        std::vector<IpVideoGrabberSettings> cams;

        ofJson streams = ofLoadJson(json);

        for (auto& stream: streams)
        {
            cams.push_back(fromJSON(stream));
        }

        return cams;
    }

private:
    std::string _name;
    std::string _url;
    std::string _username;
    std::string _password;
    AuthType _authType = AuthType::NONE;

};


class IPVideoGrabber: public ofBaseVideoDraws
{
public:
    IPVideoGrabber();
    virtual ~IPVideoGrabber();

    void setup(const IpVideoGrabberSettings& settings);

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
	void draw(const ofRectangle& rect) const override;
    void draw(const glm::vec3& point) const;

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
    int getReconnectCount() const;
    int getMaxReconnects() const;
    void setMaxReconnects(int num);
    uint64_t getAutoRetryDelay() const;
    void setAutoRetryDelay(uint64_t delay_ms);
    uint64_t getNextAutoRetryTime() const;
    uint64_t getTimeTillNextAutoRetry() const;

    void setDefaultBoundaryMarker(const std::string& boundarMarker);
    std::string getDefaultBoundaryMarker() const;
        
    ofEvent<ofResizeEventArgs> 	videoResized;

protected:    
    void threadedFunction();// override;// connect to server
    void imageResized(int width, int height);
    
private:
    std::thread _thread;

#if OF_VERSION_MAJOR < 1 && OF_VERSION_MINOR >= 10
    ofEventListener _exitListener;
#endif

    std::atomic<bool> _isConnected;

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
	mutable std::vector<ofTexture> texPlanes;

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
    int reconnectCount_a; // the number of reconnects attempted
    int maxReconnects;  // the maximum number of reconnect attempts that will be made

    uint64_t sessionTimeout; // ms
    Poco::URI uri_a;
    
    Poco::Net::NameValueCollection cookies;

    mutable std::mutex mutex;

	const static std::size_t MIN_JPEG_SIZE;
	const static std::size_t BUF_LEN;
	const static char JFF;
	const static char SOI;
	const static char EOI;
};


} } // namespace ofx::Video
