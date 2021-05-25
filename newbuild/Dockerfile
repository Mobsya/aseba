FROM ubuntu:latest
MAINTAINER Yves Piguet

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update
RUN apt upgrade -y
RUN apt install -y \
	build-essential cmake git wget \
	libudev-dev

RUN mkdir /home/tdm
RUN cd /home/tdm; git clone --recurse-submodules https://github.com/ypiguet-epfl/aseba-1.git aseba
RUN cd /home/tdm; wget -q https://boostorg.jfrog.io/artifactory/main/release/1.73.0/source/boost_1_73_0.tar.gz
RUN cd /home/tdm; tar xfz boost_1_73_0.tar.gz
RUN cd /home/tdm; git clone https://github.com/TartanLlama/expected
RUN cd /home/tdm/aseba/third_party; rm -Rf flatbuffers; git clone https://github.com/google/flatbuffers.git
RUN cd /home/tdm/aseba/third_party/flatbuffers/; cmake .; make -j
RUN cd /home/tdm; make -f aseba/newbuild/Makefile -j

WORKDIR /home/tdm
