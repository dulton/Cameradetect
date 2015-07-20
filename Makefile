
CROSS_COMPILE =
AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CXX     	= $(CROSS_COMPILE)g++
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm

STRIP			= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump

export AS LD CC CXX  CPP AR NM
export STRIP OBJCOPY OBJDUMP


CXXFLAGS := -Wall  -O2 -g
CXXFLAGS += -I $(shell pwd)/include
CXXFLAGS += $(shell pkg-config --cflags OpenCVPC) -lavformat -lavcodec -lavfilter -lavutil -lswresample -lswscale -lz -D__STDC_CONSTANT_MACROS

LDLIBS    := $(shell pkg-config --libs  OpenCVPC)  -lpthread -lm -levent -ltinyxml 

export CXXFLAGS LDLIBS  

TOPDIR := $(shell pwd)
export TOPDIR

TARGET := camera_demo


obj-y += main.o
obj-y += analyze/
obj-y += udptrans/
obj-y += xmlparser/
obj-y += tcpserver/
obj-y += tcpclient/

all : 
	make  -C ./ -f $(TOPDIR)/Makefile.build
	$(CXX)  -o $(TARGET) built-in.o  $(LDLIBS) $(CXXFLAGS)

clean:
	rm -f $(shell find -name "*.o")
	rm -f $(TARGET)

distclean:
	rm -f $(shell find -name "*.o")
	rm -f $(shell find -name "*.d")
	rm -f $(TARGET)
	
