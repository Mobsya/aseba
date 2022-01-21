# Author: Yves Piguet, EPFL
# attempt to compile tdm as simply as possible
# 18 July 2019: success on macOS 10.13.6

# Ubuntu if not HAS_LIBUSB:
# sudo apt install libusb-dev

# git clone --recurse-submodules https://github.com/Mobsya/aseba.git

# dep (except for those in aseba/third_party got with --recurse-submodules):
# boost 1.67 <https://www.boost.org/users/download/>
# !! boost 1.70 incompatible as of 18 July 2019; see https://github.com/Mobsya/aseba/issues/584
# git clone --recurse-submodules https://github.com/openssl/openssl.git
# (cd openssl; ./Configure; make)

# TartanLlama/expected: git clone https://github.com/TartanLlama/expected

# to build flatbuffer, use cmake; on Mac, get it at https://cmake.org/download/
# (cd aseba/third_party/flatbuffers/; cmake .; make)
# or just cmake on Linux
# If you get errors, try with the current version of flatc:
# (cd aseba/third_party; rm -Rf flatbuffers; git clone https://github.com/google/flatbuffers.git; cd flatbuffers; cmake .; make)

# environment variables:
# HAS_ZEROCONF=anything:
#  include zeroconf to advertise tcp and ws ports (_mobsya._tcp) and (if MOBSYA_TDM_ENABLE_TCP) browse tcp devices (_aseba._tcp)
#  requires bonjour or avahi
# HAS_FIRMWARE_UPDATE=anything:
#  include firmware update
#  requires openssl
# MOBSYA_TDM_ENABLE_TCP=anything:
#  include tcp devices
# HAS_IPV6=anything:
#  listens also on ipv6
# OPTIM=TRUE (default):
#  optimize for size with -Os
# OPTIM=FALSE:
#  no optimization, build for debugging with -g

TARGET ?= $(shell uname | tr '[:upper:]' '[:lower:]')
OPTIM ?= TRUE


# BOOST = boost_1_67_0
BOOST = boost_1_73_0

vpath %.cpp aseba/aseba/thymio-device-manager:aseba/aseba/common/msg:aseba/aseba/compiler:aseba/aseba/common/utils:$(BOOST)/libs/filesystem/src:$(BOOST)/libs/thread/src/pthread:$(BOOST)/libs/thread/src:aseba/third_party/aware/src/bonjour:aseba/third_party/aware/src/detail:aseba/third_party/aware/src/bonjour/detail:aseba/third_party/aware/src:aseba/third_party/pugixml/src
vpath %.c aseba/third_party/libusb/libusb/libusb
vpath %.cc aseba/third_party/fmt/src

CXXFLAGS = \
	-std=gnu++17 \
	-isystem $(BOOST) \
	-isystem aseba/dashel \
	-isystem aseba/third_party/fmt/include \
	-isystem aseba/third_party/spdlog/spdlog/include \
	-isystem aseba/third_party/aware/include \
	-isystem aseba/third_party/mpark.variant/include \
	-isystem aseba/third_party/expected/expected \
	-isystem aseba/third_party/pugixml/src \
	-isystem aseba/third_party/range-v3/include \
	-isystem aseba/third_party/belle/include \
	-I aseba/third_party/flatbuffers/include \
	-isystem openssl/include \
	-isystem aseba \
	-I aseba/aseba \
	-I aseba/third_party/libusb/libusb \
	-DBOOST_BEAST_ALLOW_DEPRECATED \
	-DBOOST_ENDIAN_DEPRECATED_NAMES \
	-DBOOST_THREAD_PROVIDES_FUTURE_CONTINUATION \
	-DSPDLOG_FMT_EXTERNAL \
	-DBOOST_ALLOW_DEPRECATED_HEADERS \
	-DBOOST_CHRONO_HEADER_ONLY \
	-isystem expected/include

CXXFLAGS += \
	-DHAS_FB_TCP \
	-DHAS_FB_WS

OBJ_FMT = format.o  posix.o

OBJ_AWARE = monitor_socket.o native_socket.o bonjour_error.o announce_socket.o handle.o throw_on_error.o \
	monitor.o browser.o utility.o resolver.o announcer.o properties.o contact.o

OBJ_PUGI = \
	pugixml.o

OBJ_BOOST = \
	operations.o path.o unique_path.o utf8_codecvt_facet.o \
	thread.o once_atomic.o future.o \
	exception.o directory.o

OBJ_LIBUSB = \
	aseba/third_party/libusb/liblibusb.a

OPEN_SSL = \
	openssl/libssl.a \
	openssl/libcrypto.a

OBJ = \
	main.o \
	log.o \
	tdm_error.o \
	aesl_parser.o \
	aseba_node_registery.o \
	aseba_device.o \
	aseba_endpoint.o \
	aseba_node.o \
	app_server.o \
	app_endpoint.o \
	wireless_configurator_service.o \
	group.o \
	interfaces.o \
	property_flexbuffer.o \
	utils_aseba.o \
	msg.o \
	TargetDescription.o \
	compiler.o \
	compiler_error.o \
	analysis.o \
	parser.o \
	lexer.o \
	identifier-lookup.o \
	tree-emit.o \
	tree-dump.o \
	tree-optimize.o \
	tree-build.o \
	tree-expand.o \
	tree-typecheck.o \
	utils_flatbuffers.o \
	FormatableString.o \
	\
	$(OBJ_FMT) \
	$(OBJ_PUGI) \
	$(OBJ_BOOST)

MISC = \
	expected/include/tl/expected.hpp

ifdef HAS_ZEROCONF

CXXFLAGS += \
	-DHAS_ZEROCONF

OBJ += \
	$(OBJ_AWARE)

ifdef MOBSYA_TDM_ENABLE_TCP

CXXFLAGS += \
	-DMOBSYA_TDM_ENABLE_TCP

OBJ += \
	aseba_tcpacceptor.o

endif

endif

ifdef HAS_FIRMWARE_UPDATE

CXXFLAGS += \
	-DHAS_FIRMWARE_UPDATE

OBJ += \
	fw_update_service.o \
        thymio2_fwupgrade.o \
        thymio2_fwupgrade_impl.o \
	\
	$(OPEN_SSL)

endif

ifdef HAS_IPV6

CXXFLAGS += \
	-DHAS_IPV6

endif

ifeq ($(TARGET), darwin)
CXXFLAGS += \

# mac (see aseba/thymio-device-manager/CMakeLists.txt)
OBJ += serialacceptor_osx.o serialserver.o
CXXFLAGS += -DMOBSYA_TDM_ENABLE_SERIAL
LIB += -framework IOKit -framework CoreFoundation

endif

ifeq ($(TARGET), linux)

CFLAGS += \
	-DNOT_HAVE_SA_LEN

CXXFLAGS += \
	-isystem aseba/third_party/bonjour/include

LIB += -ldl -lpthread

ifdef HAS_LIBUSB

# libusb

OBJ += usbserver.o usbacceptor.o usbdevice.o $(OBJ_LIBUSB)

CXXFLAGS += \
	-DMOBSYA_TDM_ENABLE_USB \
	-DHAS_MPARK \
	-isystem aseba/third_party/libusb/libusb

CFLAGS += \
	-Iaseba/third_party/libusb/include \
	-Iaseba/third_party/libusb/libusb/libusb

else

# udev

OBJ += serialacceptor_udev.o serialserver.o

CXXFLAGS += \
	-DMOBSYA_TDM_ENABLE_SERIAL \
	-DMOBSYA_TDM_ENABLE_UDEV

LIB += -ludev

endif

ifdef HAS_ZEROCONF

vpath %.c aseba/third_party/avahi-compat-libdns_sd:aseba/third_party/qtzeroconf/bonjour-sdk

OBJ += compat.o # dnssd_clientstub.o

endif

endif

ifeq ($(OPTIM), TRUE)
CXXFLAGS += -Os
LDFLAGS += -O
else
CXXFLAGS += -g
LDFLAGS += -g
endif

tdm: $(OBJ) $(MISC) aseba
	$(CXX) $(LDFLAGS) -o $@ $(OBJ) $(LIB)

main.o: aseba/aseba/flatbuffers/thymio_generated.h $(MISC)

tdm_error.o: aseba/aseba/thymio-device-manager/error.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

compiler_error.o: aseba/aseba/compiler/errors.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

bonjour_error.o: aseba/third_party/aware/src/bonjour/error.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

utils_flatbuffers.o: aseba/third_party/flatbuffers/src/util.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

utils_aseba.o: aseba/aseba/common/utils/utils.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

aseba/aseba/flatbuffers/thymio_generated.h: aseba/third_party/flatbuffers/flatc aseba/aseba/flatbuffers/thymio.fbs
	$< --cpp --no-includes --gen-mutable --scoped-enums \
		--gen-object-api -o aseba/aseba/flatbuffers \
		--cpp-ptr-type std::unique_ptr \
		aseba/aseba/flatbuffers/thymio.fbs

aseba/third_party/flatbuffers/flatc:
	(cd aseba/third_party/flatbuffers/; cmake .; make)

aseba:
	git clone --recurse-submodules https://github.com/Mobsya/aseba.git

openssl/libssl.a openssl/libcrypto.a:
	(cd openssl; ./config; $(MAKE))

expected/include/tl/expected.hpp:
	git clone https://github.com/TartanLlama/expected

aseba/third_party/libusb/liblibusb.a:
	(cd aseba/third_party/libusb; cmake .; make)

# some dependencies for tdm source code

tdmpth = aseba/aseba/thymio-device-manager
aesl_parser.o: $(tdmpth)/aesl_parser.h $(tdmpth)/variant_compat.h
app_endpoint.o: $(tdmpth)/app_endpoint.h
app_server.o: $(tdmpth)/app_server.h
aseba_device.o: $(tdmpth)/aseba_device.h $(tdmpth)/variant_compat.h
aseba_endpoint.o: $(tdmpth)/aseba_endpoint.h $(tdmpth)/aseba_node_registery.h $(tdmpth)/variant_compat.h
aseba_node_registery.o: $(tdmpth)/aseba_node_registery.h $(tdmpth)/aseba_endpoint.h
aseba_node.o: $(tdmpth)/aseba_node.h $(tdmpth)/aseba_endpoint.h $(tdmpth)/aseba_node_registery.h $(tdmpth)/variant_compat.h
aseba_tcpacceptor.o: $(tdmpth)/aseba_tcpacceptor.h $(tdmpth)/aseba_endpoint.h
error.o: $(tdmpth)/error.h

# assume pwd is in parent directory of aseba repository
.PHONY: prepare-ubuntu
prepare-ubuntu:
	sudo apt update
	sudo apt upgrade -y
	sudo apt install -y build-essential cmake git wget libudev-dev
	wget -q https://boostorg.jfrog.io/artifactory/main/release/1.73.0/source/boost_1_73_0.tar.gz
	tar xfz boost_1_73_0.tar.gz
	git clone https://github.com/TartanLlama/expected
	(cd ./aseba/third_party; rm -Rf flatbuffers; git clone https://github.com/google/flatbuffers.git)
	(cd ./aseba/third_party/flatbuffers/; cmake .; make -j)
