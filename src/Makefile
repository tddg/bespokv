CC=gcc
CPP=g++
MAKE = make -f
MAKE_NO_FORCE = make
MAKE_CLEAN = make clean
CONFIGURE = ./configure
CP = cp
MV = mv
AR = ar
RM = rm -f
RANLIB = ranlib
TOUCH = touch
INC =

DEBUG = 
OPTS =
COPTS = -Wall -Wno-deprecated
CPPOPTS = -Wno-literal-suffix -Wall -Wno-deprecated
LDFLAGS = -L/root/packages/double-conversion -L./kafka/src-cpp -L./kafka/src
CFLAGS = -c $(INC) $(COPTS) $(OPTS) $(DEBUG) -D_LARGEFILE_SOURCE -fPIC \
	-D_FILE_OFFSET_BITS=64 -D_LARGE_FILE_API -D_LARGEFILE64_SOURCE \
	-D_GNU_SOURCE -D__STDC_FORMAT_MACROS -fstrict-aliasing -Wall 
CPPFLAGS = -c -std=c++11 $(INC) $(CPPOPTS) $(OPTS) $(DEBUG) -D_LARGEFILE_SOURCE \
	-D_FILE_OFFSET_BITS=64 -D_LARGE_FILE_API -D_LARGEFILE64_SOURCE \
	-D_GNU_SOURCE -D__STDC_FORMAT_MACROS -fstrict-aliasing \
	-I./redlock-cpp -I./redlock-cpp/redlock-cpp -I./redlock-cpp/hiredis \
	-I./kafka/src-cpp -I./kafka/src
#CPPFLAGS = -c -std=c++11 $(INC) $(OPTS) $(DEBUG) -D_LARGEFILE_SOURCE \
	-D_FILE_OFFSET_BITS=64 -D_LARGE_FILE_API -D_LARGEFILE64_SOURCE \
	-D_GNU_SOURCE -D__STDC_FORMAT_MACROS -fstrict-aliasing \
	-I/usr/local/include/double-conversion 
	
OBJS = fde.o co_ketama.o link.o msg.o net.o proxy_util.o server.o resp.o co_zk.o \
       zk_error.o spec.pb.o ckv_proto.pb.o co_kafka.o
TARGET = conproxy
UTIL_OBJS = ./util/log.o ./util/config.o ./util/bytes.o ./util/sorted_set.o ./util/app.o
UTIL_LIB = ./util/libutil.a
REDLOCK_LIB = ./redlock-cpp/bin/libredlock.a
HIREDIS_LIB = ./redlock-cpp/hiredis/libhiredis.so
#RDKAFKAPP_LIB = ./kafka/src-cpp/librdkafka++.a ./kafka/src/librdkafka.a
RDKAFKA_LIB = ./kafka/src-cpp/librdkafka++.a ./kafka/src/librdkafka.a

MD5_OBJS = md5.o
MD5_LIB = md5.a
#KETAMA_OBJS = ./libketama/md5.o ./libketama/ketama.o 
#KETAMA_LIB = ./libketama/libketama.a
LIBS = -lrt -lm -lfolly -lglog -lgflags -lpthread -lboost_thread -lboost_regex \
       -lboost_system -lboost_filesystem -ldouble-conversion -lzookeeper_mt -lprotobuf -lhiredis -luuid -lrdkafka++ -lrdkafka

dbg: OPTS += -O0
dbg: LDFLAGS +=
#dbg: DEBUG += -D_UTEST_ -g -ggdb
dbg: DEBUG += -g -ggdb
dbg: all

opti: OPTS += -O2
opti: LDFLAGS += 
#opti: CPPFLAGS += -Wall
opti: all stripall

#INCLUDE2 = -I./redlock-cpp/redlock-cpp/ -I./redlock-cpp/hiredis
LIB_HIREDIS_FLAGS = -I./redlock-cpp/redlock-cpp/ -I./redlock-cpp -I./redlock-cpp/hiredis -L./redlock-cpp/hiredis -lhiredis -luuid
LIB_RDKAFKA_FLAGS = -I./kafka/src-cpp/ -I./kafka/src/ -lstdc++

#opt: LDFLAGS += -static-libgcc -static-libstdc++ -static -lstdc++ -Wl,-Bstatic 
#g++ -Wall -o unit_test -Wl,-Bstatic unit_test.o libtraceutils.a -lm \
	-lglog -lgflags -lfolly -lboost_regex -levent -ldouble-conversion \
	-lunwind -lunwind-x86_64 -llzma -Wl,-Bdynamic -lgtest -lpthread -Wl,--as-needed

all: $(OBJS) $(UTIL_LIB) $(HIREDIS_LIB) $(REDLOCK_LIB) $(TARGET) 

stripall: $(TARGET)
	strip $(TARGET)
	mv -t bin/ $(TARGET) 

$(UTIL_LIB) : $(UTIL_OBJS)
	$(RM) $(UTIL_LIB)        
	$(AR) cq $(UTIL_LIB) $(UTIL_OBJS)      
	$(RANLIB) $(UTIL_LIB) 

./util/app.o: ./util/app.cpp ./util/app.h
	${CPP} ${CPPFLAGS} -c -o $@ $<

./util/log.o: ./util/log.cpp ./util/log.h
	${CPP} ${CPPFLAGS} -c -o $@ $<

./util/config.o: ./util/config.cpp ./util/config.h
	${CPP} ${CPPFLAGS} -c -o $@ $<

./util/bytes.o: ./util/bytes.cpp ./util/bytes.h link.h
	${CPP} ${CPPFLAGS} -c -o $@ $<

./util/sorted_set.o: ./util/sorted_set.cpp ./util/sorted_set.h
	${CPP} ${CPPFLAGS} -c -o $@ $<

$(MD5_LIB) : $(MD5_OBJS)
	$(RM) $(MD5_LIB)
	$(AR) cq $(MD5_LIB) $(MD5_OBJS)
	$(RANLIB) $(MD5_LIB)

md5.o: md5.c md5.h
	${CC} ${CFLAGS} -c -o $@ $<

#./libketama/md5.o: ./libketama/md5.c ./libketama/md5.h
#	${CPP} ${CPPFLAGS} -c -o $@ $<

#./libketama/ketama.o: ./libketama/ketama.c ./libketama/ketama.h
#	${CPP} ${CPPFLAGS} -c -o $@ $<


#./zookeeper/authentication.o: ./zookeeper/authentication.cpp ./zookeeper/authentication.hpp
#	${CPP} ${CPPFLAGS} -c -o $@ $<

#./zookeeper/contender.o: ./zookeeper/contender.cpp ./zookeeper/contender.hpp
#	${CPP} ${CPPFLAGS} -c -o $@ $<

#./zookeeper/detector.o: ./zookeeper/detector.cpp ./zookeeper/detector.hpp
#	${CPP} ${CPPFLAGS} -c -o $@ $<

#./zookeeper/group.o: ./zookeeper/group.cpp ./zookeeper/group.hpp ./zookeeper/url.hpp ./zookeeper/authentication.hpp
#	${CPP} ${CPPFLAGS} -c -o $@ $<

#./zookeeper/zookeeper.o: ./zookeeper/zookeeper.cpp ./zookeeper/zookeeper.hpp
#	${CPP} ${CPPFLAGS} -c -o $@ $<


#REDLOCKLIB: -L./redlock/bin -lredlock -L./redlock/hiredis -lhiredis

$(HIREDIS_LIB):
	cd ./redlock-cpp/hiredis; $(MAKE_CLEAN)
	cd ./redlock-cpp/hiredis; ${MAKE_NO_FORCE}
	cd ./redlock-cpp/hiredis; cp libhiredis.so libhiredis.so.0.11 

$(REDLOCK_LIB):
	cd ./redlock-cpp; $(MAKE_CLEAN); ${MAKE_NO_FORCE}

$(RDKAFKA_LIB):
	cd ./kafka; $(CONFIGURE); ${MAKE_NO_FORCE}
	
conproxy: $(OBJS) $(UTIL_LIB) $(HIREDIS_LIB) $(REDLOCK_LIB) $(MD5_LIB) $(RDKAFKA_LIB)
	$(CPP) $(LDFLAGS) $(OBJS) -o $@ $(LIBS) $(UTIL_LIB) $(MD5_LIB) $(HIREDIS_LIB) $(REDLOCK_LIB) $(LIB_HIREDIS_FLAGS) $(LIB_RDKAFKA_FLAGS)

fde.o: fde.cpp fde.h fde_epoll.cpp fde_select.cpp
	$(CPP) $(CPPFLAGS) $<

co_ketama.o: co_ketama.cpp co_ketama.h 
	$(CPP) $(CPPFLAGS) $<

co_zk.o: co_zk.cpp co_zk.h
	$(CPP) $(CPPFLAGS) $<

co_kafka.o: kafka/src-cpp/librdkafka++.a kafka/src/librdkafka.a co_kafka.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) co_kafka.cpp 

#-o $@ $(LDFLAGS) kafka/src-cpp/librdkafka++.a kafka/src/librdkafka.a $(LIBS) -lstdc++

zk_error.o: zk_error.cpp zk_error.h
	$(CPP) $(CPPFLAGS) $<

spec.pb.cc: spec.proto
	protoc --cpp_out=. $<

spec.pb.o: spec.pb.cc
	$(CPP) $(CPPFLAGS) $<

ckv_proto.pb.cc: ckv_proto.proto
	protoc --cpp_out=. $<

ckv_proto.pb.o: ckv_proto.pb.cc
	$(CPP) $(CPPFLAGS) $<

link.o: link.cpp link.h msg.h link_redis.cpp link_redis.h ckv_proto.pb.cc
	$(CPP) $(CPPFLAGS) $<

msg.o: msg.cpp spec.pb.cc link.h server.h
	$(CPP) $(CPPFLAGS) $< 

net.o: net.cpp spec.pb.cc net.h server.h
	$(CPP) $(CPPFLAGS) $< 

proxy_util.o: proxy_util.cpp proxy_util.h server.h
	$(CPP) $(CPPFLAGS) $< 

server.o: server.cpp net.h proxy_util.h
	$(CPP) $(CPPFLAGS) $<

resp.o: resp.cpp resp.h
	$(CPP) $(CPPFLAGS) $<


#valgrind:
#	G_DEBUG=gc-friendly G_SLICE=always-malloc valgrind --track-origins=yes --leak-resolution=high --leak-check=full --show-reachable=yes  ./unit_test

#cleanlog:
#	rm -fr /tmp/cachesim.* /tmp/unit_test.*
#test:
#	./unit_test
#
#cscope:
#	cscope -Rbkq

#clean:
#	rm -fr *.o *.a 

clean:
	rm -fr *.o *.a spec.pb.* ckv_proto.pb.* $(TARGET) bin/$(TARGET) ./util/*.o ./util/*.a ./redlock-cpp/hiredis/*.so ./redlock-cpp/bin/*.a

#clean:
#	rm -fr *.o *.a spec.pb.* ckv_proto.pb.* $(TARGET) bin/$(TARGET) ./util/*.o ./util/*.a ./redlock-cpp/hiredis/*.so ./redlock-cpp/bin/*.a ./kafka/src/*.o ./kafka/src/*.a ./kafka/src/*.so

