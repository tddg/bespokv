# generic makefile

# the module names
MODULES = confs.cpp

LIB = `pkg-config fuse --libs`
INCLUDE = `pkg-config fuse --cflags`

MODE = debug
# release or debug

ifeq ($(MODE), release)
FLAGS += -O2
else
FLAGS += -O0 -g
endif

# Everything after this is generic, no need to edit

OBJS := ${MODULES:%.cpp=%.o}

.PHONY: all run clean
  
all: $(OBJS)
	g++ -o confs $(OBJS) $(LIB) 
	
clean:
	rm -f confs confs.o

%.o : %.cpp 
	g++ -c $< -o $@ $(FLAGS) $(INCLUDE)

