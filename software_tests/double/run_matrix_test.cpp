//gets matrix data for software tests for one matrix

#include <fstream>
#include <iostream>
#include <cstdlib>
using std::endl;

#include <vector>
#include "../../fpga_tests/preprocessing/include/double_hpcg_matrix.hpp"
#include "../../fpga_tests/preprocessing/include/DoubleVector.hpp"
#include "ComputeSYMGS_ref.hpp"
#include "ComputeSYMGS_ref.cpp"
#include "ComputeSYMGS_GC.hpp"
#include "ComputeSYMGS_GC.cpp"
#include "../../fpga_tests/preprocessing/double_graph_coloring.cpp"
#include "../../fpga_tests/preprocessing/include/IntVectorArray.hpp"
#include <tgmath.h>
#include "chrono"
#include "../clock.hpp"

#define THRESHOLD 1.0E-11
#define CHECK_FREQ 100
#define MAX_ITERATIONS 100000000000
#define BLOCK_SIZE 128

int VerifyResult(HPCGMatrix & A, DoubleVector & b, double threshold, int checkFreq);
int VerifyResultColors(HPCGMatrix & A, DoubleVector & b, double threshold, int checkFreq, IntVectorArray colors, bool useOMP, int num_threads);
int VerifyResultBlocks(HPCGMatrix & A, DoubleVector & b, double threshold, int checkFreq, IntVectorArray colors, int block_size, bool useOMP, int num_threads);
double ComputeParallelizationInfo(HPCGMatrix & A, IntVectorArray color_nodes, int num_threads);
double ComputeParallelizationInfoBlocks(HPCGMatrix & A, IntVectorArray color_nodes, int block_size, int num_threads);
int estimateIterations(double old_remainder, double remainder, double target, int checkFreq, int old_expected_iter);

void run_matrix_test(char* mtx_path, char* vec_path, double * std_res, double * gc_res[], double * bc_res[], double * dg_res[]) {
    HPCGMatrix A;
    DoubleVector v;
    double maxValue;
    IntVector node_colors_1;
    IntVectorArray color_nodes_1;
    IntVector node_colors_2;
    IntVectorArray color_nodes_2;
    IntVector node_colors_3;
    IntVectorArray color_nodes_3;
    int block_size;
    auto t0 = std::chrono::high_resolution_clock::now();
    auto t1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration< double > fs;
    std::chrono::microseconds d;
    long coloring_ovh;
    long block_coloring_ovh;
    long dep_graph_ovh;
    int num_threads;
    int std_iter;
    int gc_iter;
    int bc_iter;
    int dg_iter;
    double th_speedup;

    maxValue = InitializeHPCGMatrix(A, mtx_path);

    if(A.numberOfRows >= 64*BLOCK_SIZE) {
        block_size = BLOCK_SIZE;
    } else {
        block_size = A.numberOfRows/64;
    }

    InitializeDoubleVector(v, A.numberOfColumns);
    InitializeDoubleVectorFromPath(v, vec_path, A.numberOfColumns);

    //printf("Initializing colors vectors\n");

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

    //printf("\nRunning Gauss-Seidel\n");
    t0 = std::chrono::high_resolution_clock::now();
    std_iter = VerifyResult(A, v, THRESHOLD, CHECK_FREQ);
    t1 = std::chrono::high_resolution_clock::now();
    fs = t1 - t0;
    d = std::chrono::duration_cast< std::chrono::microseconds >( fs );
    long normal_time = d.count();
    std_res[0] = normal_time;
    std_res[1] = std_iter;

    num_threads = 1;
    for(int i=0; i<4; i++) {
        //printf("Running Gauss-Seidel with graph coloring\n");
        th_speedup = ComputeParallelizationInfo(A, color_nodes_1, num_threads);
        long before_clock = get_clock_count();
        t0 = std::chrono::high_resolution_clock::now();
        gc_iter = VerifyResultColors(A, v, THRESHOLD, CHECK_FREQ, color_nodes_1, true, num_threads);
        t1 = std::chrono::high_resolution_clock::now();
        long after_clock = get_clock_count();
        long diff_clock = after_clock - before_clock;
        fs = t1 - t0;
        d = std::chrono::duration_cast< std::chrono::microseconds >( fs );
        long gc_time = d.count();
        gc_res[i][0] = num_threads;
        gc_res[i][1] = gc_time;
        gc_res[i][2] = gc_iter;
        gc_res[i][3] = ((double)normal_time)/(gc_time+coloring_ovh);
        gc_res[i][4] = th_speedup;
        gc_res[i][5] = ((double) coloring_ovh)/normal_time;
        gc_res[i][6] = diff_clock;
        gc_res[i][7] = ((double) diff_clock)/gc_time;
        num_threads *= 2;
    }

    num_threads = 1;
    for(int i=0; i<4; i++) {
        //printf("Running Gauss-Seidel with block coloring\n");
        th_speedup = ComputeParallelizationInfoBlocks(A, color_nodes_2, block_size, num_threads);
        long before_clock = get_clock_count();
        t0 = std::chrono::high_resolution_clock::now();
        bc_iter = VerifyResultBlocks(A, v, THRESHOLD, CHECK_FREQ, color_nodes_2, block_size, true, num_threads);
        t1 = std::chrono::high_resolution_clock::now();
        long after_clock = get_clock_count();
        long diff_clock = after_clock - before_clock;
        fs = t1 - t0;
        d = std::chrono::duration_cast< std::chrono::microseconds >( fs );
        long bc_time = d.count();
        bc_res[i][0] = num_threads;
        bc_res[i][1] = bc_time;
        bc_res[i][2] = bc_iter;
        bc_res[i][3] = ((double)normal_time)/(bc_time+block_coloring_ovh);
        bc_res[i][4] = th_speedup;
        bc_res[i][5] = ((double) block_coloring_ovh)/normal_time;
        bc_res[i][6] = diff_clock;
        bc_res[i][7] = ((double) diff_clock)/bc_time;
        num_threads *= 2;
    }

    num_threads = 1;
    for(int i=0; i<4; i++) {
        //printf("Running Gauss-Seidel with dependency graph\n");
        th_speedup = ComputeParallelizationInfo(A, color_nodes_3, num_threads);
        long before_clock = get_clock_count();
        t0 = std::chrono::high_resolution_clock::now();
        dg_iter = VerifyResultColors(A, v, THRESHOLD, CHECK_FREQ, color_nodes_3, true, num_threads);
        t1 = std::chrono::high_resolution_clock::now();
        long after_clock = get_clock_count();
        long diff_clock = after_clock - before_clock;
        fs = t1 - t0;
        d = std::chrono::duration_cast< std::chrono::microseconds >( fs );
        long dg_time = d.count();
        dg_res[i][0] = num_threads;
        dg_res[i][1] = dg_time;
        dg_res[i][2] = dg_iter;
        dg_res[i][3] = ((double)normal_time)/(dg_time+dep_graph_ovh);
        dg_res[i][4] = th_speedup;
        dg_res[i][5] = ((double) dep_graph_ovh)/normal_time;
        dg_res[i][6] = diff_clock;
        dg_res[i][7] = ((double) diff_clock)/dg_time;
        num_threads *= 2;
    }
}

//computes Gauss-Seidel with matrix A and known DoubleVector b.
//checks convergence every checkFreq interations.
//convergerce is considered reached if the module of the remainder is less then threshold times the module of b
int VerifyResult(HPCGMatrix & A, DoubleVector & b, double threshold, int checkFreq) {
  DoubleVector gs; //partial result for x
  DoubleVector res; //A*x
  DoubleVector diff; //Ax - rhs
  InitializeDoubleVector(gs, A.numberOfColumns);
  InitializeDoubleVector(res, A.numberOfColumns);
  InitializeDoubleVector(diff, A.numberOfColumns);
  double remainder;
  double old_remainder = -1;
  int counter = 0;
  int expected_iter = checkFreq;
  int old_expected_iter;
  double target = threshold*Abs(b);
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
    //printf("Current iterations: %d, remainder: %lf, target: %lf, expected iter: %d\n", counter, remainder, target, expected_iter);
    fflush(stdout);
  }
  printf("Test failed.\n");
  return -1;
}

//the same as VerifyResult, but with a colors DoubleVector and with ComputeSYMGS_GC instead
int VerifyResultColors(HPCGMatrix & A, DoubleVector & b, double threshold, int checkFreq, IntVectorArray colors, bool useOMP, int num_threads) {
  DoubleVector gs; //partial result for x
  DoubleVector res; //A*x
  DoubleVector diff; //Ax - rhs
  InitializeDoubleVector(gs, A.numberOfColumns);
  InitializeDoubleVector(res, A.numberOfColumns);
  InitializeDoubleVector(diff, A.numberOfColumns);
  double remainder;
  double old_remainder = -1;
  int counter = 0;
  int expected_iter = checkFreq;
  int old_expected_iter;
  double target = threshold*Abs(b);
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
    //printf("Current iterations: %d, remainder: %lf, target: %lf, expected iter: %d\n", counter, remainder, target, expected_iter);
    //fflush(stdout);
  }
  printf("Test failed.\n");
  return -1;
}

//the same as VerifyResult, but with a colors DoubleVector and with ComputeSYMGS_BC instead
int VerifyResultBlocks(HPCGMatrix & A, DoubleVector & b, double threshold, int checkFreq, IntVectorArray colors, int block_size, bool useOMP, int num_threads) {
  DoubleVector gs; //partial result for x
  DoubleVector res; //A*x
  DoubleVector diff; //Ax - rhs
  InitializeDoubleVector(gs, A.numberOfColumns);
  InitializeDoubleVector(res, A.numberOfColumns);
  InitializeDoubleVector(diff, A.numberOfColumns);
  double remainder;
  double old_remainder = -1;
  int counter = 0;
  int expected_iter = checkFreq;
  int old_expected_iter;
  double target = threshold*Abs(b);
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
    //printf("Current iterations: %d, remainder: %lf, target: %lf, expected iter: %d\n", counter, remainder, target, expected_iter);
    //fflush(stdout);
  }
  printf("Test failed.\n");
  return -1;
}

//calculates best possible speedup assuming splitting rows in a greedy way
double ComputeParallelizationInfo(HPCGMatrix & A, IntVectorArray color_nodes, int num_threads) {
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

  return ((double)total_nonzeros)/partial_max_nonzeros;
}

//calculates best possible speedup assuming splitting blocks in a greedy way
double ComputeParallelizationInfoBlocks(HPCGMatrix & A, IntVectorArray color_nodes, int block_size, int num_threads) {
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

  return ((double)total_nonzeros)/partial_max_nonzeros;
}

//estimates the number of iterations for convergence
int estimateIterations(double old_remainder, double remainder, double target, int checkFreq, int old_expected_iter) {
  if(old_remainder == -1 || old_remainder <= remainder) {
    return checkFreq;
  }
  int result = (int) (log(remainder/target)/log(old_remainder/remainder)*old_expected_iter);
  if(result == 0) {
    return 1;
  }
  return result;
}
