dbg: OPTS += -O0
dbg: all

opti: OPTS += -O2
opti: all 

all: bench_trace_gen bench_client

bench_trace_gen: bench_trace_gen.cpp
	g++ -std=c++0x -o bench_trace_gen bench_trace_gen.cpp -lssl -lcrypto -lz

bench_client: bench_client.o
	gcc -std=gnu99 ${OPTS} -ggdb -o bench_client bench_client.o -lpthread -lmemcached -lckv -lstdc++ -lprotobuf \
		-L/usr/lib -L/usr/local/lib \
		-L../apps/clibs/libckv/build -L/usr/lib64 

bench_client.o: bench_client.c
	gcc -std=gnu99 ${OPTS} -ggdb -c bench_client.c -I/usr/include -I/usr/include \
		-I../apps/clibs/libckv/include

clean:
	-rm *.o bench_client bench_trace_gen
