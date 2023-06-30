CXX=g++
CXXFLAGS+=-std=c++11 -O3 -w

build: pre

pre := preprocess
$(pre): preprocess.cpp
	$(CXX) $(CXXFLAGS) -o $@ preprocess.cpp
.PHONY: pre
pre: $(pre)

clean:
	rm -rf preprocess
