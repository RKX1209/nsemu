CXX				:= g++
SRC_FILES := $(wildcard *.cpp ARMv8/*.cpp)
OBJ_FILES := $(SRC_FILES:.cpp=.o)
CXX_FLAGS	:= -std=c++11 -Iinclude
LD_FLAGS	:= -llz4 -lpthread

all: nsemu
nsemu: $(OBJ_FILES)
	$(CXX) -o nsemu $(OBJ_FILES) $(LD_FLAGS)
%.o: %.cpp
	$(CXX) $(CXX_FLAGS) -c -g -o $@ $<
clean:
	rm -f *.o */*.o
	rm -f nsemu
