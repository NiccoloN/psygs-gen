#include <fstream>
#include <iostream>
#include <cstdlib>
using std::endl;

#include <vector>
#include "../../fpga_tests/preprocessing/include/float_hpcg_matrix.hpp"
#include "../../fpga_tests/preprocessing/include/FloatVector.hpp"
#include "ComputeSYMGS_ref.hpp"
#include "ComputeSYMGS_ref.cpp"
#include "ComputeSYMGS_GC.hpp"
#include "ComputeSYMGS_GC.cpp"
#include "../../fpga_tests/preprocessing/float_graph_coloring.cpp"
#include "../../fpga_tests/preprocessing/include/IntVectorArray.hpp"
#include <tgmath.h>
#include "chrono"

#define THRESHOLD 1.0E-3
#define CHECK_FREQ 1000
#define MAX_ITERATIONS 100000000000
#define BLOCK_SIZE 128

int VerifyResult(HPCGMatrix & A, FloatVector & b, float threshold, int checkFreq);
int VerifyResultColors(HPCGMatrix & A, FloatVector & b, float threshold, int checkFreq, IntVectorArray colors, bool useOMP, int num_threads);
int VerifyResultBlocks(HPCGMatrix & A, FloatVector & b, float threshold, int checkFreq, IntVectorArray colors, int block_size, bool useOMP, int num_threads);
float ComputeParallelizationInfo(HPCGMatrix & A, IntVectorArray color_nodes, int num_threads);
float ComputeParallelizationInfoBlocks(HPCGMatrix & A, IntVectorArray color_nodes, int block_size, int num_threads);
int estimateIterations(float old_remainder, float remainder, float target, int checkFreq, int old_expected_iter);

//old main: runs Gauss Seidel on one matrix specifed in argv
//argv[1]: matrix filename, argv[2]: seed (optional)
int main(int argc, char * argv[]) {
  HPCGMatrix A;
  FloatVector v;
  float maxValue;
  IntVector node_colors_1;
  IntVectorArray color_nodes_1;
  IntVector node_colors_2;
  IntVectorArray color_nodes_2;
  IntVector node_colors_3;
  IntVectorArray color_nodes_3;
  int block_size;
  auto t0 = std::chrono::high_resolution_clock::now();
  auto t1 = std::chrono::high_resolution_clock::now();
  std::chrono::duration< float > fs;
  std::chrono::microseconds d;
  long coloring_ovh;
  long block_coloring_ovh;
  long dep_graph_ovh;
  int num_threads;
  int std_iter;
  int gc_iter;
  int bc_iter;
  int dg_iter;
  float th_speedup;

  if(argc != 3) {
    printf("Incorrect number of arguments!");
    return 0;
  }

  printf("Initializing matrix: %s\n", argv[1]);
  maxValue = InitializeHPCGMatrix(A, argv[1]);
  printf("Matrix initialized, number of rows: %d, number of nonzeros: %d\n", A.numberOfRows, A.numberOfNonzeros);
  fflush(stdout);
  InitializeFloatVector(v, A.numberOfColumns);
  InitializeFloatVectorFromPath(v, argv[2], A.numberOfColumns);


  if(A.numberOfRows >= 32*BLOCK_SIZE) {
    block_size = BLOCK_SIZE;
  } else {
    block_size = A.numberOfRows/32;
  }


  printf("Initializing colors vectors\n");

  t0 = std::chrono::high_resolution_clock::now();
  ComputeColors(A, node_colors_1, color_nodes_1);
  t1 = std::chrono::high_resolution_clock::now();
  fs = t1 - t0;
  d = std::chrono::duration_cast< std::chrono::microseconds >( fs );
  coloring_ovh = d.count();

  t0 = std::chrono::high_resolution_clock::now();
  ComputeBlockColoring(A, block_size, node_colors_2, color_nodes_2);
  t1 = std::chrono::high_resolution_clock::now();
  fs = t1 - t0;
  d = std::chrono::duration_cast< std::chrono::microseconds >( fs );
  block_coloring_ovh = d.count();

  t0 = std::chrono::high_resolution_clock::now();
  ComputeColorsDepGraph(A, node_colors_3, color_nodes_3);
  t1 = std::chrono::high_resolution_clock::now();
  fs = t1 - t0;
  d = std::chrono::duration_cast< std::chrono::microseconds >( fs );
  dep_graph_ovh = d.count();

  printf("Colors vectors initialized\n");
  fflush(stdout);

  /*
  t0 = std::chrono::high_resolution_clock::now();
  VerifyResultColors(A, v, THRESHOLD, CHECK_FREQ, color_nodes_1, true, 8);
  t1 = std::chrono::high_resolution_clock::now();
  fs = t1 - t0;
  d = std::chrono::duration_cast< std::chrono::microseconds >( fs );
  long omp = d.count();
  printf("omp: %ld\n", omp);
  t0 = std::chrono::high_resolution_clock::now();
  VerifyResultColors(A, v, THRESHOLD, CHECK_FREQ, color_nodes_1, false, 8);
  t1 = std::chrono::high_resolution_clock::now();
  fs = t1 - t0;
  d = std::chrono::duration_cast< std::chrono::microseconds >( fs );
  long no_omp = d.count();
  printf("no omp: %ld\n", no_omp);
  printf("omp speedup: %f\n", ((float)no_omp)/((float)omp));
  fflush(stdout);
  */
  

  //printf("\nRunning Gauss-Seidel\n");
  t0 = std::chrono::high_resolution_clock::now();
  std_iter = VerifyResult(A, v, THRESHOLD, CHECK_FREQ);
  t1 = std::chrono::high_resolution_clock::now();
  fs = t1 - t0;
  d = std::chrono::duration_cast< std::chrono::microseconds >( fs );
  long normal_time = d.count();
  printf("normal: %ld us, %d iterations\n", normal_time, std_iter);
  fflush(stdout);
  
  num_threads = 1;
  for(int i=0; i<4; i++) {
    //printf("Running Gauss-Seidel with graph coloring\n");
    th_speedup = ComputeParallelizationInfo(A, color_nodes_1, num_threads);
    t0 = std::chrono::high_resolution_clock::now();
    gc_iter = VerifyResultColors(A, v, THRESHOLD, CHECK_FREQ, color_nodes_1, true, num_threads);
    t1 = std::chrono::high_resolution_clock::now();
    fs = t1 - t0;
    d = std::chrono::duration_cast< std::chrono::microseconds >( fs );
    long gc_time = d.count();
    printf("coloring with %d threads: %ld us, %d iterations, speedup: %f vs %f theoretical, overhead: %f\n", num_threads, gc_time, gc_iter, ((float)normal_time)/(gc_time+coloring_ovh), th_speedup, ((float) coloring_ovh)/normal_time);
    fflush(stdout);
    num_threads *= 2;
  }

  num_threads = 1;
  for(int i=0; i<4; i++) {
    //printf("Running Gauss-Seidel with block coloring\n");
    th_speedup = ComputeParallelizationInfoBlocks(A, color_nodes_2, block_size, num_threads);
    t0 = std::chrono::high_resolution_clock::now();
    bc_iter = VerifyResultBlocks(A, v, THRESHOLD, CHECK_FREQ, color_nodes_2, block_size, true, num_threads);
    t1 = std::chrono::high_resolution_clock::now();
    fs = t1 - t0;
    d = std::chrono::duration_cast< std::chrono::microseconds >( fs );
    long bc_time = d.count();
    printf("block coloring with %d threads with %d block size: %ld us, %d iterations, speedup: %f vs %f theoretical, overhead: %f\n", num_threads, block_size, bc_time, bc_iter, ((float)normal_time)/(bc_time+block_coloring_ovh), th_speedup, ((float) block_coloring_ovh)/normal_time);
    fflush(stdout);
    num_threads *= 2;
  }

  num_threads = 1;
  for(int i=0; i<4; i++) {
    //printf("Running Gauss-Seidel with dependency graph\n");
    th_speedup = ComputeParallelizationInfo(A, color_nodes_3, num_threads);
    t0 = std::chrono::high_resolution_clock::now();
    dg_iter = VerifyResultColors(A, v, THRESHOLD, CHECK_FREQ, color_nodes_3, true, num_threads);
    t1 = std::chrono::high_resolution_clock::now();
    fs = t1 - t0;
    d = std::chrono::duration_cast< std::chrono::microseconds >( fs );
    long dg_time = d.count();
    printf("dep graph with %d threads: %ld us, %d iterations, speedup: %f vs %f theoretical, overhead: %f\n", num_threads, dg_time, dg_iter, ((float)normal_time)/(dg_time+dep_graph_ovh), th_speedup, ((float) dep_graph_ovh)/normal_time);
    fflush(stdout);
    num_threads *= 2;
  }
  
  printf("\n\n\n");
  fflush(stdout);
  return 0;
}

//computes Gauss-Seidel with matrix A and known FloatVector b.
//checks convergence every checkFreq interations.
//convergerce is considered reached if the module of the remainder is less then threshold times the module of b
int VerifyResult(HPCGMatrix & A, FloatVector & b, float threshold, int checkFreq) {
  FloatVector gs; //partial result for x
  FloatVector res; //A*x
  FloatVector diff; //Ax - rhs
  InitializeFloatVector(gs, A.numberOfColumns);
  InitializeFloatVector(res, A.numberOfColumns);
  InitializeFloatVector(diff, A.numberOfColumns);
  float remainder;
  float old_remainder = -1;
  int counter = 0;
  int expected_iter = checkFreq;
  int old_expected_iter;
  float target = threshold*Abs(b);
  while(counter < MAX_ITERATIONS) {
    for(int i=0; i<expected_iter; i++) {
      ComputeSYMGS_ref(A, b, gs);
      counter++;
    }
    SpMV(A, gs, res);
    Diff(res, b, diff);
    remainder = Abs(diff);
    if (remainder < target) {
      //printf("Test passed. Number of iterations: %d\n", counter);
      return counter;
    }
    //remainder is estimated to decrease exponentially
    old_expected_iter = expected_iter;
    expected_iter = estimateIterations(old_remainder, remainder, target, checkFreq, old_expected_iter);
    if(expected_iter > MAX_ITERATIONS-counter) {
      expected_iter = MAX_ITERATIONS-counter;
    }
    old_remainder = remainder;
    //printf("Current iterations: %d, remainder: %f, target: %f, expected iter: %d\n", counter, remainder, target, expected_iter);
    fflush(stdout);
  }
  printf("Test failed.\n");
  return -1;
}

//the same as VerifyResult, but with a colors FloatVector and with ComputeSYMGS_GC instead
int VerifyResultColors(HPCGMatrix & A, FloatVector & b, float threshold, int checkFreq, IntVectorArray colors, bool useOMP, int num_threads) {
  FloatVector gs; //partial result for x
  FloatVector res; //A*x
  FloatVector diff; //Ax - rhs
  InitializeFloatVector(gs, A.numberOfColumns);
  InitializeFloatVector(res, A.numberOfColumns);
  InitializeFloatVector(diff, A.numberOfColumns);
  float remainder;
  float old_remainder = -1;
  int counter = 0;
  int expected_iter = checkFreq;
  int old_expected_iter;
  float target = threshold*Abs(b);
  while(counter < MAX_ITERATIONS) {
    for(int i=0; i<expected_iter; i++) {
      ComputeSYMGS_GC(A, b, gs, colors, useOMP, num_threads);
      counter++;
    }
    SpMV(A, gs, res);
    Diff(res, b, diff);
    remainder = Abs(diff);
    if (remainder < target) {
      //printf("Test passed. Number of iterations: %d\n", counter);
      return counter;
    }
    //remainder is estimated to decrease exponentially
    old_expected_iter = expected_iter;
    expected_iter = estimateIterations(old_remainder, remainder, target, checkFreq, old_expected_iter);
    if(expected_iter > MAX_ITERATIONS-counter) {
      expected_iter = MAX_ITERATIONS-counter;
    }
    old_remainder = remainder;
    //printf("Current iterations: %d, remainder: %f, target: %f, expected iter: %d\n", counter, remainder, target, expected_iter);
    //fflush(stdout);
  }
  printf("Test failed.\n");
  return -1;
}

//the same as VerifyResult, but with a colors FloatVector and with ComputeSYMGS_BC instead
int VerifyResultBlocks(HPCGMatrix & A, FloatVector & b, float threshold, int checkFreq, IntVectorArray colors, int block_size, bool useOMP, int num_threads) {
  FloatVector gs; //partial result for x
  FloatVector res; //A*x
  FloatVector diff; //Ax - rhs
  InitializeFloatVector(gs, A.numberOfColumns);
  InitializeFloatVector(res, A.numberOfColumns);
  InitializeFloatVector(diff, A.numberOfColumns);
  float remainder;
  float old_remainder = -1;
  int counter = 0;
  int expected_iter = checkFreq;
  int old_expected_iter;
  float target = threshold*Abs(b);
  while(counter < MAX_ITERATIONS) {
    for(int i=0; i<expected_iter; i++) {
      ComputeSYMGS_BC(A, b, gs, colors, block_size, useOMP, num_threads);
      counter++;
    }
    SpMV(A, gs, res);
    Diff(res, b, diff);
    remainder = Abs(diff);
    if (remainder < target) {
      //printf("Test passed. Number of iterations: %d\n", counter);
      return counter;
    }
    //remainder is estimated to decrease exponentially
    old_expected_iter = expected_iter;
    expected_iter = estimateIterations(old_remainder, remainder, target, checkFreq, old_expected_iter);
    if(expected_iter > MAX_ITERATIONS-counter) {
      expected_iter = MAX_ITERATIONS-counter;
    }
    old_remainder = remainder;
    //printf("Current iterations: %d, remainder: %f, target: %f, expected iter: %d\n", counter, remainder, target, expected_iter);
    //fflush(stdout);
  }
  printf("Test failed.\n");
  return -1;
}

float ComputeParallelizationInfo(HPCGMatrix & A, IntVectorArray color_nodes, int num_threads) {
  int total_nonzeros = A.numberOfNonzeros;
  int nonzeros_per_thread[num_threads];
  int partial_max_nonzeros = 0;
  int min_nonzeros;
  int min_index;
  int max_nonzeros;
  int max_index;

  for(int i=0; i<color_nodes.length; i++) {
    for(int k=0; k<num_threads; k++) {
      nonzeros_per_thread[k] = 0;
    }
    for(int j=0; j<color_nodes.values[i].length; j++) {
      min_nonzeros = nonzeros_per_thread[0];
      min_index = 0;
      for(int k=0; k<num_threads; k++) {
        if(nonzeros_per_thread[k] < min_nonzeros) {
          min_nonzeros = nonzeros_per_thread[k];
          min_index = k;
        }
      }
      nonzeros_per_thread[min_index] += A.nonzerosInRow[color_nodes.values[i].values[j]];
    }
    max_nonzeros = nonzeros_per_thread[0];
    max_index = 0;
    for(int k=0; k<num_threads; k++) {
      if(nonzeros_per_thread[k] > max_nonzeros) {
        max_nonzeros = nonzeros_per_thread[k];
        max_index = k;
      }
    }
    partial_max_nonzeros += nonzeros_per_thread[max_index];
  }

  return ((float)total_nonzeros)/partial_max_nonzeros;
}

float ComputeParallelizationInfoBlocks(HPCGMatrix & A, IntVectorArray color_nodes, int block_size, int num_threads) {
  int total_nonzeros = A.numberOfNonzeros;
  int nonzeros_per_thread[num_threads];
  int partial_max_nonzeros = 0;
  int min_nonzeros;
  int min_index;
  int max_nonzeros;
  int max_index;

  for(int i=0; i<color_nodes.length; i++) {
    for(int k=0; k<num_threads; k++) {
      nonzeros_per_thread[k] = 0;
    }
    for(int j=0; j<color_nodes.values[i].length; j++) {
      min_nonzeros = nonzeros_per_thread[0];
      min_index = 0;
      for(int k=0; k<num_threads; k++) {
        if(nonzeros_per_thread[k] < min_nonzeros) {
          min_nonzeros = nonzeros_per_thread[k];
          min_index = k;
        }
      }
      for(int k=0; k<block_size; k++) {
        nonzeros_per_thread[min_index] += A.nonzerosInRow[(color_nodes.values[i].values[j])*block_size+k];
      }
    }
    max_nonzeros = nonzeros_per_thread[0];
    max_index = 0;
    for(int k=0; k<num_threads; k++) {
      if(nonzeros_per_thread[k] > max_nonzeros) {
        max_nonzeros = nonzeros_per_thread[k];
        max_index = k;
      }
    }
    partial_max_nonzeros += nonzeros_per_thread[max_index];
  }

  for(int k=block_size*(A.numberOfRows/block_size); k<A.numberOfRows; k++) {
    partial_max_nonzeros += A.nonzerosInRow[k];
  }

  return ((float)total_nonzeros)/partial_max_nonzeros;
}

int estimateIterations(float old_remainder, float remainder, float target, int checkFreq, int old_expected_iter) {
  if(old_remainder == -1 || old_remainder <= remainder) {
    return checkFreq;
  }
  int result = (int) (log(remainder/target)/log(old_remainder/remainder)*old_expected_iter);
  if(result == 0) {
    return 1;
  }
  return result;
}
