# NES Emulator

A hobby project to build an 8-bit Nintendo emulator in C++. It's not intended to be different or
better than any of the other emulators out there. My goal is to get it functional enough to run
The Legend of Zelda with a USB controller.

# Getting Started

You'll need an installation of QT6 and then update the below instructions with your QT installation path.

* `mkdir build`
* `cd build`
* `cmake ../platform -DQt6_DIR=~/Qt/6.6.1/macos/lib/cmake/Qt6`
* `make`
