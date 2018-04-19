CXX				:= g++
SRC_FILES := $(wildcard *.cpp ARMv8/*.cpp Service/*.cpp)
OBJ_FILES := $(SRC_FILES:.cpp=.o)
CXX_FLAGS	:= -std=gnu++1z -Iinclude -O3
LD_FLAGS	:= -llz4 -lpthread
MAKE := make

all: nsemu
nsemu: $(OBJ_FILES)
	$(CXX) -o nsemu $(OBJ_FILES) $(LD_FLAGS)
%.o: %.cpp
	$(CXX) $(CXX_FLAGS) -c -g -o $@ $<
testall:
	$(MAKE) -C test/
distclean:
	$(MAKE) clean -C test/
	$(MAKE)	clean
	rm -f include/IpcStubs.h
clean:
	rm -f *.o */*.o
	rm -f nsemu
