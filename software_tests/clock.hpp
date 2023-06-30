//Script to get the number of clock cycles elapsed.

#include <fstream>
#include <iostream>
#include <cstdlib>
using std::endl;

#include <stdint.h>

//  Windows
#ifdef _WIN32

#include <intrin.h>
uint64_t rdtsc(){
    return __rdtsc();
}

//  Linux/GCC
#else

uint64_t rdtsc(){
    unsigned int lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

#endif

long get_clock_count() {
  return rdtsc();
}