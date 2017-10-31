CXX				:= g++
SRC_FILES := $(wildcard *.cpp)
OBJ_FILES := $(SRC_FILES:.cpp=.o)
CXX_FLAGS	:= -Iinclude
LD_FLAGS	:= -llz4

all: nsemu
nsemu: $(OBJ_FILES)
	$(CXX) -o nsemu $(LD_FLAGS) $(OBJ_FILES)
%.o: %.cpp
	$(CXX) $(CXX_FLAGS) -c -g -o $@ $<
clean:
	rm -f *.o
	rm -f nsemu
