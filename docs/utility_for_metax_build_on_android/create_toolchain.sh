#!/bin/bash

/opt/android-ndk-r16b/build/tools/make_standalone_toolchain.py --install-dir /home/builder/poco_build_for_android/toolchain/ --api 15 --arch arm --stl libc++ 
