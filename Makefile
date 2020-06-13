#!/usr/bin/make -f

CC= gcc
CXX= g++
RM= rm -f

PKGCONFIG= pkg-config
PACKAGES= eigen3 gl glu glui

ifndef PACKAGES
PKG_CONFIG_CFLAGS=
PKG_CONFIG_LDFLAGS=
PKG_CONFIG_LIBS=
else
PKG_CONFIG_CFLAGS=`pkg-config --cflags $(PACKAGES)`
PKG_CONFIG_LDFLAGS=`pkg-config --libs-only-L $(PACKAGES)`
PKG_CONFIG_LIBS=`pkg-config --libs-only-l $(PACKAGES)`
endif

CFLAGS= \
	-pedantic \
	-Wall \
	-fstack-protector-strong \
	-D_FORTIFY_SOURCE=2 \
	-Wno-unused-variable \
	-Wno-unused-value \
	-Wno-unused-function \
	-Wno-unused-but-set-variable

LDFLAGS= \
	-Wl,--as-needed \
	-Wl,--no-undefined \
	-Wl,--no-allow-shlib-undefined

CSTD=-std=c11
CPPSTD=-std=c++11

OPTS= -O0 -ggdb

DEFS=

INCS= \
	-I.

LIBS= \
	-lm -lglut

BINARY= SkeletonPoser

BINARY_SRCS= \
	src/arm.cpp \
	src/main.cpp \
	src/point.cpp \
	src/segment.cpp \
	src/csv.cpp \
	src/tinyfiledialogs.cpp \
	src/arghelper.cpp

BINARY_OBJS= $(subst .cpp,.o,$(BINARY_SRCS))

all: $(BINARY)

$(BINARY): $(BINARY_OBJS)
	$(CXX) $(CPPSTD) $(CSTD) $(PKG_CONFIG_LDFLAGS) $(LDFLAGS) -o $@ $(BINARY_OBJS) $(PKG_CONFIG_LIBS) $(LIBS)

%.o: %.cpp
	$(CXX) $(CPPSTD) $(OPTS) -o $@ -c $< $(DEFS) $(INCS) $(PKG_CONFIG_CFLAGS) $(CFLAGS)

%.o: %.cc
	$(CXX) $(CPPSTD) $(OPTS) -o $@ -c $< $(DEFS) $(INCS) $(PKG_CONFIG_CFLAGS) $(CFLAGS)

%.o: %.c
	$(CC) $(CSTD) $(OPTS) -o $@ -c $< $(DEFS) $(INCS) $(PKG_CONFIG_CFLAGS) $(CFLAGS)

depend: .depend

.depend: $(BINARY_SRCS)
	$(RM) ./.depend
	$(CXX) $(CPPSTD) $(DEFS) $(INCS) $(CFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(BINARY_OBJS) $(BINARY)
	$(RM) -fv *~ .depend core *.out *.bak
	$(RM) -fv *.o *.a *~
	$(RM) -fv */*.o */*.a */*~

include .depend

.PHONY: all depend clean
