# ofxIpVideoGrabber

![Screenshot](https://github.com/bakercp/ofxIpVideoGrabber/raw/master/screen.png)

## Description

ofxIpVideoGrabber is an Open Frameworks addon used to capture video streams from IP Cameras that use the mjpeg streaming protocol. Public cameras can be found with a google search like this:

[http://www.google.com/search?q=inurl%3A%22axis-cgi%2Fmjpg%22](http://www.google.com/search?q=inurl%3A%22axis-cgi%2Fmjpg%22)

See others examples below.

## Getting Started

To get started, generate the example project files using the openFrameworks [Project Generator](http://openframeworks.cc/learning/01_basics/how_to_add_addon_to_project/).

### Compatability

- Edimax IC-3005
- Axis Cameras
- [mjpeg-streamer](https://github.com/jacksonliam/mjpg-streamer) for Raspiberry Pi, etc.
- [ofxHTTP MJPEG Server](https://github.com/bakercp/ofxHTTP/tree/master/example_basic_server_mjpeg_video)
- Many others.

_Note: This is not compatible with H264 streams that can be found on many modern cameras. For H264 cameras, consider [ofxGStreamer](https://github.com/arturoc/ofxGStreamer)._

### How Do I Find Cameras?

You might try some of the following Google Searches:

```
inurl:”ViewerFrame?Mode=
intitle:Axis 2400 video server
inurl:/view.shtml
intitle:”Live View / – AXIS
inurl:view/view.shtml
inurl:ViewerFrame?Mode=
inurl:ViewerFrame?Mode=Refresh
inurl:axis-cgi/jpg
inurl:axis-cgi/mjpg (motion-JPEG)
inurl:view/indexFrame.shtml
inurl:view/index.shtml
inurl:view/view.shtml
liveapplet
intitle:”live view” intitle:axis
intitle:liveapplet
allintitle:”Network Camera NetworkCamera”
intitle:axis intitle:”video server”
intitle:liveapplet inurl:LvAppl
intitle:”EvoCam” inurl:”webcam.html”
intitle:”Live NetSnap Cam-Server feed”
intitle:”Live View / – AXIS”
intitle:”Live View / – AXIS 206M”
intitle:”Live View / – AXIS 206W”
intitle:”Live View / – AXIS 210?
inurl:indexFrame.shtml Axis
inurl:”MultiCameraFrame?Mode=Motion”
intitle:start inurl:cgistart
intitle:”WJ-NT104 Main Page”
intext:”MOBOTIX M1? intext:”Open Menu”
intext:”MOBOTIX M10? intext:”Open Menu”
intext:”MOBOTIX D10? intext:”Open Menu”
intitle:snc-z20 inurl:home/
intitle:snc-cs3 inurl:home/
intitle:snc-rz30 inurl:home/
intitle:”sony network camera snc-p1?
intitle:”sony network camera snc-m1?
site:.viewnetcam.com -www.viewnetcam.com
intitle:”Toshiba Network Camera” user login
intitle:”netcam live image”
intitle:”i-Catcher Console – Web Monitor”
```

## Documentation

API documentation can be found here.

## Build Status

Linux, macOS [![Build Status](https://travis-ci.org/bakercp/ofxIpVideoGrabber.svg?branch=master)](https://travis-ci.org/bakercp/ofxIpVideoGrabber)

Visual Studio, MSYS (Not tested recently, but working in the past).

## Compatibility

The `stable` branch of this repository is meant to be compatible with the openFrameworks [stable branch](https://github.com/openframeworks/openFrameworks/tree/stable), which corresponds to the latest official openFrameworks release.

The `master` branch of this repository is meant to be compatible with the openFrameworks [master branch](https://github.com/openframeworks/openFrameworks/tree/master).

Some past openFrameworks releases are supported via tagged versions, but only `stable` and `master` branches are actively supported.

## Versioning

This project uses Semantic Versioning, although strict adherence will only come into effect at version 1.0.0.

## Licensing

See `LICENSE.md`.

## Contributing

Pull Requests are always welcome, so if you make any improvements please feel free to float them back upstream :)

1. Fork this repository.
2. Create your feature branch (`git checkout -b my-new-feature`).
3. Commit your changes (`git commit -am 'Add some feature'`).
4. Push to the branch (`git push origin my-new-feature`).
5. Create new Pull Request.
