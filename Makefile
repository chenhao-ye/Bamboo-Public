CC=g++
# CFLAGS=-Wall -g -std=c++0x -fno-omit-frame-pointer
CFLAGS=-Wall -g -std=c++17 -fno-omit-frame-pointer

.SUFFIXES: .o .cpp .h

SRC_DIRS = ./ ./benchmarks/ ./concurrency_control/ ./storage/ ./system/
# INCLUDE = -I. -I./benchmarks -I./concurrency_control -I./storage -I./system
INCLUDE = -I. -I./benchmarks -I./concurrency_control -I./storage -I./system -I./nanolog

#CFLAGS += $(INCLUDE) -D NOGRAPHITE=1 -Wno-unused-variable #-Werror
CFLAGS += $(INCLUDE) -D NOGRAPHITE=1 -O3 -Wno-unused-variable #-Werror
#LDFLAGS = -Wall -L. -L./libs -pthread -g -lrt -std=c++0x -ljemalloc
#LDFLAGS = -Wall -L. -L./libs -pthread -g -lrt -std=c++0x -O3 -ljemalloc
LDFLAGS = -Wall -L. -L./libs -L./nanolog -lNanoLog -pthread -g -lrt -std=c++0x -O3 -ljemalloc
LDFLAGS += $(CFLAGS)

CPPS = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)*.cpp))
OBJS = $(CPPS:.cpp=.o)
DEPS = $(CPPS:.cpp=.d)

all:rundb

rundb : $(OBJS)
	$(CC) -no-pie -o $@ $^ $(LDFLAGS)
	#$(CC) -o $@ $^ $(LDFLAGS)

-include $(OBJS:%.o=%.d)

%.d: %.cpp
	$(CC) -MM -MT $*.o -MF $@ $(CFLAGS) $<

%.o: %.cpp
	$(CC) -c $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -f rundb $(OBJS) $(DEPS)

run: all
	./rundb
	./nanolog/decompressor decompress compressedLog > debug_sven/latest_log
	sed 's/2021-12-09[^\]*\]: //' debug_sven/latest_log | tail -n 100000 > debug_sven/latest_log_t
