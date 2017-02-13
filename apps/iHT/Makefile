CC=gcc
CPP=g++

OPTS =
DEBUG =
COPTS = -Wall -Wno-deprecated

CFLAGS = -c $(COPTS) ${OPTS} ${DEBUG} -D_LARGEFILE_SOURCE \
	-D_FILE_OFFSET_BITS=64 -D_LARGE_FILE_API -D_LARGEFILE64_SOURCE \
	-D_GNU_SOURCE -D__STDC_FORMAT_MACROS -fstrict-aliasing -Wall
CPPFLAGS = -c -std=c++11 $(COPTS) ${OPTS} ${DEBUG} -D_LARGEFILE_SOURCE \
	  -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE_API -D_LARGEFILE64_SOURCE \
	  -D_GNU_SOURCE -D__STDC_FORMAT_MACROS -fstrict-aliasing -fpermissive

TARGET = conkv
OBJS = conkv.o link.o str.o ckv_proto.pb.o

LIBS = -lpthread -levent -lglog -lprotobuf

dbg: OPTS += -O0
dbg: DEBUG += -g -ggdb
dbg: all

opti: OPTS += -O2
opti: all stripall


all: $(OBJS) $(TARGET)

stripall: $(TARGET)
	strip $(TARGET)

conkv: $(OBJS) 
	$(CPP) $(OBJS) -o $@ $(LIBS) 

link.o: link.cpp link.h
	$(CPP) $(CPPFLAGS) $<

str.o: str.cpp str.h
	$(CPP) $(CPPFLAGS) $<

ckv_proto.pb.cc: ckv_proto.proto
	protoc --cpp_out=. $<

ckv_proto.pb.o: ckv_proto.pb.cc
	$(CPP) $(CPPFLAGS) $<

conkv.o: conkv.cpp link.h ckv_proto.pb.cc conkv.h 
	$(CPP) $(CPPFLAGS) $<

clean:
	rm -fr *.o *.a $(TARGET) 
