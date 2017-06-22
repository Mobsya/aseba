# Thymio VPL Mobile

A phone and tablet Visual Programming Environment for the Thymio II robot, by Stéphane Magnenat, Martin Voelkle and Maria Beltrán and contributors.
See the [authors file](AUTHORS.md) for a full list of contributions.
This program is open-source under [LGPL-3.0](LICENSE.txt).

## Understanding the source tree

This program is built using [Qt5/QML](https://www.qt.io/), on top of a C++14 core.
Its dependencies, namely the [Aseba](https://github.com/aseba-community/aseba/) framework and the [Dashel](https://github.com/aseba-community/dashel) communication libraries, are available as [git submodules](https://git-scm.com/docs/git-submodule).

## Compilation instructions

Thymio VPL Mobile is easy to compile:
1. Install the [latest version of Qt5](https://www.qt.io/download-open-source/).
2. Clone this repository recursively:
   `git clone --recursive https://github.com/aseba-community/thymio-vpl2.git`
3. Open `thymio-vpl2.pro` in Qt Creator and run the project.

Note that on Linux, you need to install libudev first in order to enumerate serial ports (Ubuntu: package `libudev-dev`).
