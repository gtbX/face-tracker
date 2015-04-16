# Simple Motorized Face Tracker

This program uses OpenCV and a USB foam dart "rocket launcher" to track faces.  Fire mechanism is currently unimplemented, for safety reasons (ie., my launcher is broken and no longer fires).

Adapted from the OpenCV objectDetection tutorial, found at http://docs.opencv.org/doc/tutorials/objdetect/cascade_classifier/cascade_classifier.html (https://github.com/Itseez/opencv/blob/master/samples/cpp/tutorial_code/objectDetection/objectDetection.cpp)

## Features

* Video input is through a webcam attached to the top of the launcher
* Will point at and follow faces in view of the camera
* Can stream captured video back into a v4l2 device (such as v4l2loopback) for further consumption in other applications.
* Spartan, no-frills implementation.

## Building

This program was developed on Linux, and uses gmake to build.  No guarantees/warranties/suggestions/implications made on compatibility.  Requires at least OpenCV and libusb.  To build, run:

$ make

## Usage

### Supported Model

USB Rocket Launcher is made by Dream Cheeky:
* 0a81:0701 Chesen Electronics Corp. USB Missile Launcher

Other models may work, with adaptation.

### OpenCV Data files
Requires the following files to be copied from your OpenCV installation to the current directory:
* haarcascade_frontalface_alt.xml
* haarcascade_eye_tree_eyeglasses.xml

### Linux usbhid unbinding
On Linux, the kernel "usbhid" driver will probably claim exclusive access to the USB rocket launcher device.  In order to unbind it from the driver (and avoid needing to be root all the time), the unbind_usb.sh script is provided.  Run it after plugging in the rocket launcher:

$ ./unbind_usb.sh [USB VID/PID]
(see script contents for format)
(TODO: This would be totally rad as a udev rule instead!)

After that, the objectdetection demo should be able to access the launcher.

### Arguments

Launch the program as such:

$ ./objectDetection {camera-no} [output-device]

Where:
* camera-no: Number for the camera to use as input.  For example, if your webcam is on /dev/video0, put 0 here.
* output-device (optional): If using v4l2loopback to stream video out, put the full path to the device here (eg. /dev/video1)

Have fun! Don't shoot your eye out!
