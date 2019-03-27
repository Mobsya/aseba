Building Aseba
==============

Requierements & Dependencies
----------------------------

Aseba requires a C++14 compatible compiler. We recommend ``GCC 6``,
``Clang 5`` or ``MSVC 19`` (Visual Studio 17).

Aseba depends on Qt5.9 or greater. You will also need ``cmake`` 3.1 or
greater, we recommend you use the latest version available.

Getting the source code
-----------------------

The `source code of Aseba <https://github.com/mobsya/aseba>`_
is hosted on GitHub.
To fetch the source, you must first `install Git <https://git-scm.com/book/en/v2/Getting-Started-Installing-Git>`_
.

Then, to clone aseba, execute:

::

    git clone --recursive https://github.com/mobsya/aseba.git
    cd aseba

When pulling, it might be necessary to update the submodules with ``git submodule update --init``.
Alternatively, git can do that for you if you configure it with ``git config --global submodule.recurse true``.


All the commands given in the rest of this document assume the current path is the root folder of the cloned repository.


Getting Started on Windows with MSVC
------------------------------------

Aseba Builds on Windows Seven SP1 or greater.

Download and install the following components:

.. csv-table::
   :header: "Dep", "Dowload", "Notes"

   "Visual Studio 2017 (15.9.4+)", "`Download <https://visualstudio.microsoft.com/downloads/>`_", Install the "Desktop development with C++" workload
   "Cmake 3.12+", `Website <https://cmake.org/download/>`__, Make sure the version of boost you choose is compatible with the cmake version
   "Boost 1.67+", `Website <https://www.boost.org/>`_ `x64 <https://sourceforge.net/projects/boost/files/boost-binaries/1.67.0/boost_1_67_0-msvc-14.1-64.exe/download>`_ `x86 <https://sourceforge.net/projects/boost/files/boost-binaries/1.67.0/boost_1_67_0-msvc-14.1-32.exe/download>`_
   "Qt 5.11.2+",   `Installer <https://download.qt.io/official_releases/online_installers/qt-unified-windows-x86-online.exe>`_, Install the MSVC 2017 binaries as well as the ``Qt Charts`` component. For ``x86`` you can choose the ``MSVC 2015 32 bits`` binaries instead in the Qt installer components screen.
   Bonjour, You will find the installer in ``third_party/bonjour/bonjoursdksetup.exe``
   Node & Npm, "`Download <https://nodejs.org/en/download/>`_", ``npm.exe`` must be in the path
   7Zip, "`Download <https://www.7-zip.org/download.html>`_"
   NSIS 2, "`Download <https://sourceforge.net/projects/nsis/files/NSIS%202/>`_", For building the installer; ``nsis.exe`` must be in the path;
   Python, "`Download <https://www.python.org/downloads/windows/>`_", For signing the installer; ``python.exe`` must be in the path;


To build Aseba, you first need to generate a Visual Studio Solution.

To do so:

1. Launch ``Developer Command Prompt for VS 2017``

   Navigate to the directory in which you want to build aseba. It is recommended not to build in the source directory

2. Run ``set "CMAKE_PREFIX_PATH=C:`<BOOST_INSTALLATION_PATH>\boost\_1_67_0;C:\<QT_INSTALLATION_PATH>\<QT_VERTION>\msvc2017_64;"``

   where ``<BOOST_INSTALLATION_PATH>`` and ``<QT_INSTALLATION_PATH>`` are the paths where Boost and Qt are installed, respectively.

   ``<QT_VERTION>`` is the version of Qt you installed. A folder of that name exists in the Qt installation directory.

3. ``set "BOOST_ROOT=<BOOST_INSTALLATION_PATH>"``

4. To build for x64:

::

   cmake -G"Visual Studio 15 2017 Win64" -DBoost_DEBUG=ON -DBUILD_SHARED_LIBS=OFF "-DBOOST_ROOT=%BOOST_ROOT%" "-DBOOST_INCLUDEDIR=%BOOST_ROOT%/boost" "-DBOOST_LIBRARYDIR=%BOOST_ROOT%/lib64-msvc-14.1" "-DCMAKE_TOOLCHAIN_FILE=<ASEBA_SOURCE_DIRECTORY>\windows\cl-toolchain.cmake" <ASEBA_SOURCE_DIRECTORY>

To build for x86:

::

   cmake -G"Visual Studio 15 2017" -DBoost_DEBUG=ON -DBUILD_SHARED_LIBS=OFF "-DBOOST_ROOT=%BOOST_ROOT%" "-DBOOST_INCLUDEDIR=%BOOST_ROOT%/boost" "-DBOOST_LIBRARYDIR=%BOOST_ROOT%/lib32-msvc-14.1" "-DCMAKE_TOOLCHAIN_FILE=<ASEBA_SOURCE_DIRECTORY>\windows\cl-toolchain.cmake" <ASEBA_SOURCE_DIRECTORY>

where ``<ASEBA_SOURCE_DIRECTORY>`` is the directory containing the aseba repository.

Then, to build the project, you can either run ``msbuild aseba.sln`` or open ``aseba.sln`` with Visual Studio 2017.
Refer to the documentation of msbuild and Visual Studio for more informations.

Getting Started on OSX
----------------------

You will need OSX 10.11 or greater

-  Install `Homebrew <https://brew.sh/>`__.
-  In the cloned repository run

::

   brew update brew tap homebrew/bundle brew bundle

Then you can create a build directory and build Aseba

::

    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF ..
    make

Getting Started on Linux
------------------------

Dependencies On Ubuntu & Debian
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You will need a C++17 able compiler. GCC 8 is known to work.
The requireded dependency may vary accros distributions.
The following instructions are given for Ubuntu 18.10 cosmic

Install the following packages:

::

    sudo apt-get install
        mesa-common-dev libgl1-mesa-dev \
        clang clang-format g++-multilib gdb \
        git \
        cmake \
        ninja-build \
        libavahi-compat-libdnssd-dev \
        libudev-dev \
        libssl-dev \
        libfreetype6 \
        libfontconfig \
        libnss3 libasound2 libxtst6 libxrender1 libxi6 libxcursor1 libxcomposite1

`Download Qt 5.12 <https://www.qt.io/download-qt-installer>`__

You will need to select the QtWebEngine, QtCharts components.

.. image:: qt-linux.png


You then need to define an environment variable CMAKE_PREFIX_PATH pointing
to the Qt installation folder:

::

    export CMAKE_PREFIX_PATH=<Qt_Install_Directory/<version>/gcc_64>

Docker Image
~~~~~~~~~~~~

We also provide a docker image `Docker Image <https://hub.docker.com/r/mobsya/linux-dev-env>`__
with the dependencies already installed.

Building Aseba
~~~~~~~~~~~~~~
::

    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF ..
    make

A note about permissions
~~~~~~~~~~~~~~~~~~~~~~~~

If you will be connecting to your robot through a serial port, you might
need to add yourself to the group that has permission for that port. In
many distributions, this is the "dialout" group and you can add yourself
to that group and use the associated permissions by running the
following commands:

::

    sudo usermod -a -G dialout $USER
    newgrp dialout


Getting Started on Android
--------------------------
VPL 2 can be built for Android. Other tools such as studio, playground, and the old VPL
are not compatible with Android.

To build the Android version you will need:
 * `The Android tools for your system <https://developer.android.com/studio/index.html#downloads>`_
 * `The Android NDK <https://developer.android.com/ndk/downloads/index.html>`_ - tested with version 10 - currently not compatible with newer NDK
 * Qt 5.10 for Android - which you can install through the Qt installer
 * CMake 3.7 or greater

Building VPL 2
~~~~~~~~~~~~~~
First, you need to prepare some environment variables

::

    export ANDROID_SDK=<path_of_the_android_sdk>
    export ANDROID_NDK=<path_of_the_android_ndk>
    export CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH}:$HOME/<path_of_qt5_for_android>"

Then you can build vpl2 with cmake. An APK will be generated in ``build/bin``

::

    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DANDROID_NATIVE_API_LEVEL=14 -DANDROID_STL=gnustl_shared -DCMAKE_TOOLCHAIN_FILE=`pwd`/../android/qt-android-cmake/toolchain/android.toolchain.cmake
    make


Advanced Setup
--------------

Running tests
~~~~~~~~~~~~~

Once the build is complete, you can run ``ctest`` in the build directory
to run the tests.

Ninja
~~~~~

The compilation of Aseba can be significantly speedup using ``ninja``
instead of make. Refer to the documentation of ``cmake`` and ``ninja``.
