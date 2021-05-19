

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
