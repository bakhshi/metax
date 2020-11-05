SETUP METAX BUILD FOR ANDROID
--------------------------------------------------------------------------------
These are common steps to build metax apk for android
1. Download Android studio
    https://developer.android.com/studio/index.html
2. Install it
    https://developer.android.com/studio/install.html
3. Go to leviathan git -> metax/android/metax
4. Open the 3th point's project via Android Studio - "already existing project"
5. Replace the Metax's configs from metax/app/src/main/assets/config.xml
   ( do this point if it is necessary to change Metax's default configs)
6. Install all necessary sdks, build tools, libraries
   6.1. Make sure you haven't already installed needed components and
        replace the SDK, NDK paths from metax/local.properties file
   6.2. Install SDK, NDK, cmake etc and replace paths into file mentioned in 6.1
   6.3. Build "Poco", "OpenSSL"libraries or copy already built poco and openssl
        (this libs paths are hardcoded to be in /opt/poco and /opt/openssl)
7. Click to Build tab -> Make Project 
   (if 6.1 components are installed properly, should success without any errors)
8. Click to Build tab -> Build APK
   (if all libs are built properly, should success without any errors)
   note: If you need to build signed APK click to Build tab -> Build Signed APK
         and choose needed keystore file and passkeys

NOTE: This file has been written on 15.11.17.
      Any changes after this date need to be added.
--------------------------------------------------------------------------------
If generating metax as android library is needed, then the following additional steps are needed:

Go to ./app/gradle.build and:
	1. Find the line and delete it:
		applicationId "am.leviathan.metax"
	2. Find the line:
		apply plugin: 'com.android.application'
	   and change with the following:
		apply plugin: 'com.android.library'
	3. In the end of file, please find commented lines related to
	   creating jar, uncomment the lines and run the project.
After follow the steps of the top answer of the following link:
	https://stackoverflow.com/questions/21712714/how-to-make-a-jar-out-from-an-android-studio-project
The project will generate metax.jar and libmetax.so which will work as library
together. 
	
