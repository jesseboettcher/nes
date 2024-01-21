# Building

## with Bazel (headless)

bazel build //:app
bazel run //:app /Users/jesse/code/nes/roms/donkey_kong.nes

## Linux

cmake --build /home/jesse/code/nes/linux/build-nes_qt-Desktop_Qt_6_6_1_GCC_64bit-Debug --target all
./linux/build-nes_qt-Desktop_Qt_6_6_1_GCC_64bit-Debug/appnes_qt

# Options

Modify config/flags.hpp to adjust the display options (X11, headless, mac)