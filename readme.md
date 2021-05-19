

# TDM modifications

This repository contains experiments with the TDM (Thymio Device Manager).

## Static build with make

- Make a new directory to download all that's required and build the tdm:
    ```
    mkdir tdm-build
    cd tdm-build
    ```
- Clone the repository and all its submodules:
    ```
    git clone --recurse-submodules https://github.com/ypiguet-epfl/aseba-1.git
    ```
- Rename the repository to its standard name:
    ```
    mv aseba-1 aseba
    ```
- Get other dependencies:
    - Boost: download Boost 1.73 from [https://www.boost.org/users/history/](https://www.boost.org/users/history/) (pick `boost_1_73_0.tar.gz` or `boost_1_73_0.zip` and uncompress it into `tdm-build`, so that its directory is `tdm-build/boost_1_73_0`)
    - OpenSSL: still in `tdm-build`, clone OpenSSL and build it with
        ```
        git clone --recurse-submodules https://github.com/openssl/openssl.git
        (cd openssl; ./Configure; make)
        ```
    - TartanLlama/expected: clone it with
        ```
        git clone https://github.com/TartanLlama/expected
        ```

- Build flatbuffer with `cmake`. On Mac, get it at [https://cmake.org/download/](https://cmake.org/download/). Then
    ```
    (cd aseba/third_party/flatbuffers/; cmake .; make)
    ```
    If you get errors, try with the current version of flatc:
    ```
    (cd aseba/third_party; rm -Rf flatbuffers; git clone https://github.com/google/flatbuffers.git)
    (cd aseba/third_party/flatbuffers/; cmake .; make)
    ```

- On Ubuntu, install `libudev-dev`:
    ```
    sudo apt install libudev-dev
    ```

- Build the TDM:
    ```
    make -f aseba/newbuild/Makefile -j
    ```

## Build without zeroconf

On Linux, the TDM relies on the Avahi implementation of zeroconf, with `aware` and possibly other code to provide compatibility with Bonjour, the Apple zeroconf implementation. The exact details aren't fully understood yet. That's the reason why the zeroconf functionality in the TDM has been made optional with `#ifdef HAS_ZEROCONF`.

If `HAS_ZEROCONF` is undefined, the TDM is compiled without zeroconf. The Makefile checks if the envisonment variable `HAS_ZEROCONF`is defined and compiles everything accordingly, including the files `aseba_tcpacceptor.cpp` and `aseba_tcpacceptor.h` only with zeroconf (they support connections to the simulator Aseba Playground whose TCP port is also advertised by zeroconf). So to include zeroconf (especially on macOS), you can type
```
HAS_ZEROCONF=TRUE make -f aseba/newbuild/Makefile -j
```
To compile the TDM without zeroconf, just do it as explained in the previous section.

Currently, the TDM without zeroconf is invisible to Thymio Suite which relies on zeroconf to discover the TDM TCP port number. But you can do it using command-line tools, as explained below.

## Zeroconf advertisement with command-line tools

The TCP port changes every time the TDM is launched. It's displayed by the TDM shortly after it's launched, on a line like this:
```
[2021-05-19 11:17:20.781] [console] [trace]             main.cpp@L85:	=> TCP Server connected on 44125
```
Many lines follow quickly; maybe it's easier to clear the terminal with `clear`, type `tdm` and scroll up as soon as you get the first lines displayed. In the line above, the TCP port is 44125. The WebSocket port (WS Server) is always 8597.

On Linux with Avahi, you can advertise the TDM service as follows (replace 44125 with the value you found):
```
avahi-publish -s "TDM" _mobsya._tcp 44125 ws-port=8597 uuid=`uuidgen`
```

To check:
```
avahi-browse _mobsya._tcp
```

With bonjour on Mac:
```
dns-sd -R "Thymio Device Manager" _mobsya._tcp local 44125 ws-port=8597 uuid=`uuidgen`
```

To check:
```
dns-sd -Z _mobsya._tcp local
```

Then Thymio Suite, or the Python package `tdmclient`, should find your TDM.
