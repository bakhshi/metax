Leviathan Android Project Build
==================================================================================


### CONTENT
1. PREAPARE ANDROID ENVIRONMENT
2. BUILD POCO LIBRARY
3. INSTALL GRADLE
4. PROJECT BUILD&RUN

### 1. PREAPARE ANDROID ENVIRONMENT
##### Download Android SDK and Android NDK (CrystaX/native) 

* [Native NDK] (http://developer.android.com/ndk/downloads/index.html)

* [CrystaX NDK] (https://www.crystax.net/en/download)

* [Android SDK] (http://developer.android.com/sdk/index.html#Other)

##### Create standalone toolchain

Run command:
>  > $(NDK-PATH)/build/tools/make-standalone-toolchain.sh --platform=android-8
                                                          --install-dir=$HOME/$(NDK-NAME)-android-toolchain
                                                          --toolchain=arm-linux-androideabi-$(VERSION)


*NOTE:  As of 2016 Mar 01 available compilre versions are 4.8 & 4.9 for native NDK, 4.9 & 5 for CrystaX NDK.  
Default version of gcc/g++: 4.8 for Native NDK,  4.9 for Crystax NDK.*


##### Add the directory containing the toolchain executables to your $PATH:

> > export PATH=$PATH:$HOME/$(NDK-NAME)-android-toolchain/bin

### 2. BUILD POCO LIBRARY
##### Download POCO project

    * [Poco Framework] (http://pocoproject.org/download/index.html) - v 1.6.1 is used as of 2016-Mar-01

##### Build Poco Library using POCO's GNU Make-based Build System
Go to the poco library folder. In build/config/Android make the following updates:
> * set LINKMODE ?= SHARED
> * add -std=c++11 to CXXFLAGS
> * SHAREDLIBEXT     = .so

##### Configure Poco library for Android.
> > ./configure --config=Android --no-samples --no-tests --omit=Data/ODBC,Data/MySQL,PageCompiler,NetSSL_OpenSSL,Crypto --prefix=/opt/poco/Android

For armeabi-v7a, set the ANDROID_ABI make variable to armeabi-v7a:
> > make install -s -j4 ANDROID_ABI=armeabi-v7a

*Note: Sometimes build fails on Data/OCDB Data/MySQL due to absence of some third party libraries.
As they are currently not used in metax we are omiting those libraries by --omit option of configure script.*

### 3. INSTALL GRADLE


   * [gradle] (https://spring.io/guides/gs/gradle-android/#initial)

### 4. PROJECT BUILD & RUN


##### Setup env vars
**TODO:** add info regarding ndk_dir env var in local.properties

##### Build project from command line using gradle
For command line build please run:
> > ./gradlew assembleDebug

**TODO:** add info how to build release version.

##### Install project from command line
For command line please run:
> > $(ADNROID_SDK_PATH)/platform-tools/adb install -r app/build/outputs/apk/app-arm7-debug.apk


*NOTE: Project can also be opened/built/run in Android studio environment as well.*

**TODO:** embedded jni makefiles should be updated to match current source tree
##### Generate `*.so` files
Go to ndk directory and run:
> > ndk-build

*Note: If POCO_PREFIX variable is not set, generated .so files will be located in /usr/local/lib.*

##### After .so files are ready 
Go to android project application libs directory ($(PROJECT_DIR)/app/src/main/libs)
create symbolic links to .so files.
