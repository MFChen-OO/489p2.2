CPP_CODE_FILES := $(sort $(wildcard *.cpp) $(wildcard *.cc))
CPP_OBJ_FILES := $(addsuffix .o,$(basename $(CPP_CODE_FILES)))
H_FILES := $(wildcard *.h)

.PHONY: all clean

all: test_lib

test_lib: $(CPP_OBJ_FILES) 
	g++ $(CPP_OBJ_FILES) -o test_lib

%.o: %.cpp $(H_FILES)
	g++ $< -I -ldl -std=c++11 -c -o $@ -g 

clean:
	-rm -rf *.o
	-rm test_lib

