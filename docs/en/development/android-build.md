# Building Thymio Suite for Android

**All instructions in this document have been tested on Ubuntu 20.04.**

## Command Line

### Prerequisites

Install Docker. On recent version of Ubuntu or other Debian derivatives, type `sudo apt install docker.io`.

### Building within Containerized Environment

Please use the documentation available in the dedicated [reposirory]( https://github.com/Mobsya/docker/blob/android-porting/README.md)

This is the way Thymio Suite is built on the CI.

## IDE with Qt Creator

### Prerequisites

1. Install Android SDK: the easiest way is to install Android Studio and make sure you have:
   1. NDK 21 installed
   2. Android SDK 24 installed
   3. Android build tools 29 installed
2. Install Qt for Linux and make sure you select Qt 5.15.x

### Configure Aseba Project in Qt Creator

1. Open Qt Creator 4.14
2. Use *File -> Open file or project* and pick the root CMakeList.txt
3. On first run, you should see the default Kit management window: make sure you select at least the Kit named *Android 5.12.x Clang Multi ABI*
4. Use *Build -> Execute CMake* to setup the project
5. Toggle to the Projects context with the icon in the left side bar
   1. Make sure that the Android Kit is active (i.e. *Android 5.15.x Clang Multi ABI*) and select the *Build* settings
   2. Adjust the CMake settings:
      1. ANDROID_ABI: armeabi-v7a
      2. ANDROID_BUILD_ABI_armeabiv7a: ON
      3. ANDROID_NATIVE_API_LEVEL: 24
      4. ANDROID_NDK: path/to/android/ndk/**21**
   3. **Do not forget to click on *Apply Configuration Changes*** to enforce your modifications
   4. Show details of *Build Android APK* step and make sure that  the platform *android-24* is selected (there are incompatibility issue with all other versions).
   5. Add a custom build step to work around the way project named are handled in Qt Creator:
      1. Command: cp
      2. Arguments: android-build/libs/armeabi-v7a/libthymio-launcher_armeabi-v7a.so android-build/libs/armeabi-v7a/libThymioSuite_armeabi-v7a.so
      3. Use the chevron icons on the top left corner of your custom build step container to move your step after the build and before the APK build 
   6. Add a custom build step to copy web apps into the *android-build/assets* folder:
      1. Command: cp
      2. Arguments: -R "path to web apps parent folder/*" android-build/assets
      3. Use the chevron icons on the top left corner of your custom build step container to move your step right before the APK build 
6. Select the *Run* settings of your active Android Kit (i.e. *Android 5.15.x Clang Multi ABI*)
   1. In *Deployment* section, make sure that the option `Uninstall the existing app first` is no checked; otherwise, you may not be able to deploy the app to the device
   2. In *Execute* section, make sure that you select *thymio-launcher* as *Execution configuration*.
7. Toggle to the *Edit* context with the icon in the left side bar: you should see the content of the project
8. Build the project with the build icon at the bottom of the left side bar
   1. In case of warning treated as error in *flatbuffer* thrown during the build, use `Ctrl+Alt+F` in order to search all `-Werror` switches in your *CMake* files. Remove all of them (do not commit the files)
9. In order to deploy to a physical or virtual device, use one of the *Run* icons at the bottom of the left side bar 

### Common Pitfalls

Every time you run *Build -> Clean CMake* you need to check that everything is properly configured in step #5 above. Qt Creator will parse the build folder to sync your CMake configuration with the content of that folder. You will loose all settings that do not match the defaults of the CMake shipped with Qt.

Every time you use the *Build -> Rebuild* all projects, you may need to check step #5 above.

In case of failure when building the APK, make sure that you followed the steps #5.3 and #5.4 above.

Every time you change the target architecture, you'll have to go over all settings from steps #5 above.

In case you encounter an error "is not a directory" or "directory does not exist", please create the required directory by hand in the build folder.

### Limitations

Since Qt creator has its own way of creating the Android manifest and building the APK (or AAB), some changes in the source code related to the APK and AAB file won't affect the produced application! Be  aware, that those changes cannot be tested without further configuration tricks in Qt Creator.

