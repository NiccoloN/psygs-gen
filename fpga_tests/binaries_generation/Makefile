CXX=g++
CXXFLAGS+=-std=c++11 -O3 -w

build: gen

gen := generate_binary_data
$(gen): generate_binary_data.cpp
	$(CXX) $(CXXFLAGS) -o $@ generate_binary_data.cpp
.PHONY: gen
gen: $(gen)

clean:
	rm -rf generate_binary_data
