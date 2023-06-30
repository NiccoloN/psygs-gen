//like DoubleVector, but using floats

#ifndef FLOAT_VECTOR_HPP
#define FLOAT_VECTOR_HPP
#include <cassert>
#include <cstdlib>
#include <cmath>
#include <fstream>

#define SIZE_RANDOM 10

struct FloatVector_STRUCT {
  int length;  //!< length of the vector
  float * values;          //!< array of values
};
typedef struct FloatVector_STRUCT FloatVector;

/*!
  Initializes input vector.

  @param[in] v
  @param[in] length Length of input vector
 */
inline void InitializeFloatVector(FloatVector & v, int length) {
  v.length = length;
  v.values = new float[length];
  for(int i=0; i<length; i++) {
    v.values[i] = 0.0;
  }
  return;
}

inline void FillRandomFloatVector(FloatVector & v, unsigned seed, double maxValue) {
  srand(seed);
  int length = v.length;
  float * vv = v.values;
  for (int i=0; i<length; ++i) vv[i] = ((rand() / (float)(RAND_MAX))*2.0 - 1.0)*maxValue*SIZE_RANDOM;
  return;
}

inline void InitializeFloatVectorFromPath(FloatVector & v, const char* path, int length) {
  std::ifstream infile;
  infile.open(path);

  long x;
  std::string junk;

  InitializeFloatVector(v, length);
  for(int i = 0; i < length; i++)  {
    infile >> x >> junk;
    v.values[i] = *(float*) &x;
  }

  infile.close();
}

inline void ExportFloatVector(FloatVector & v, char* path) {
  std::ofstream outfile;
  outfile.open(path);

  for(int i = 0; i < v.length; i++)  {
    outfile << *(int*) (v.values + i) << " " << v.values[i];
    outfile << "\n";
  }

  outfile.close();
}

inline void Diff(const FloatVector & v1, const FloatVector & v2, const FloatVector & res) {
  for(int i=0; i<v1.length; i++) {
    res.values[i] = v1.values[i] - v2.values[i];
  }
}

inline float Abs(const FloatVector & v) {
  float res = 0;
  for(int i=0; i<v.length; i++) {
    res += v.values[i]*v.values[i];
  }
  return sqrt(res);
}

#endif
