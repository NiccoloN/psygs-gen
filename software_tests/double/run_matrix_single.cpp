//edited version of runMatrix for testing purposes, not used to get results

#include <fstream>
#include <iostream>
#include <cstdlib>
using std::endl;

#include <vector>
#include "../../fpga_tests/preprocessing/include/double_hpcg_matrix.hpp"
#include "../../fpga_tests/preprocessing/include/DoubleVector.hpp"
#include "ComputeSYMGS_ref.hpp"
#include "ComputeSYMGS_ref.cpp"
#include "ComputeSYMGS_GC_no_omp.hpp"
#include "ComputeSYMGS_GC_no_omp.cpp"
#include "../../fpga_tests/preprocessing/double_graph_coloring.cpp"
#include "../../fpga_tests/preprocessing/include/IntVectorArray.hpp"
#include <tgmath.h>

#define THRESHOLD 1.0E-11
#define CHECK_FREQ 1000
#define ESTIMATE 1
#define MAX_ITERATIONS 10000000
#define BLOCK_SIZE 2

int VerifyResult(HPCGMatrix & A, DoubleVector & b, double threshold, int checkFreq);
int VerifyResultColors(HPCGMatrix & A, DoubleVector & b, double threshold, int checkFreq, IntVectorArray colors);
int VerifyResultBlocks(HPCGMatrix & A, DoubleVector & b, double threshold, int checkFreq, IntVectorArray colors, int block_size);
double ComputeParallelizationInfo(HPCGMatrix & A, IntVectorArray color_nodes, int num_threads);
double ComputeParallelizationInfoBlocks(HPCGMatrix & A, IntVectorArray color_nodes, int block_size, int num_threads);
int estimateIterations(double old_remainder, double remainder, double target, int checkFreq, int old_expected_iter);
void assign_blocks(HPCGMatrix & A, IntVectorArray & colors, IntVectorArray result[], int n_acc, int block_size);

//old main: runs Gauss Seidel on one matrix specifed in argv
//argv[1]: matrix filename, argv[2]: seed (optional)
int main(int argc, char * argv[]) {
  HPCGMatrix A;
  DoubleVector v;
  double maxValue;
  IntVector node_colors;
  IntVectorArray color_nodes;

  if(argc != 3) {
    printf("Incorrect number of arguments!");
    return 0;
  }

  printf("Initializing matrix: %s\n", argv[1]);
  maxValue = InitializeHPCGMatrix(A, argv[1]);
  printf("Matrix initialized, number of rows: %d, number of nonzeros: %d\n", A.numberOfRows, A.numberOfNonzeros);
  fflush(stdout);
  InitializeDoubleVector(v, A.numberOfColumns);
  InitializeDoubleVectorFromPath(v, argv[2], A.numberOfColumns);

  printf("Initializing colors vectors\n");
  ComputeBlockColoring(A, BLOCK_SIZE, node_colors, color_nodes);


  /*
  for(int i=0; i<color_nodes.length; i++) {
    printf("color %d:\n",i);
    for(int j=0; j<color_nodes.values[i].length; j++) {
      for(int k=0; k<BLOCK_SIZE; k++) {
        printf("%d ", color_nodes.values[i].values[j]*BLOCK_SIZE+k);
      }
    }
    printf("\n");
  }
  if(A.numberOfRows % BLOCK_SIZE != 0) {
    printf("color %d:\n",color_nodes.length);
    for(int i=0; i<A.numberOfRows % BLOCK_SIZE; i++) {
      printf("%d ", BLOCK_SIZE*(A.numberOfRows / BLOCK_SIZE)+i);
    }
    printf("\n");
  }
  */

  /*
  for(int i=0; i<color_nodes.length; i++) {
    for(int j=0; j<color_nodes.values[i].length; j++) {
      for(int k=0; k<BLOCK_SIZE; k++) {
        int row = color_nodes.values[i].values[j]*BLOCK_SIZE+k;
        for(int l=0; l<A.nonzerosInRow[row]; l++) {
          printf("%ld\n", *((long*) &(A.matrixValues[row][l])));
        }
      }
    }
  }
  if(A.numberOfRows % BLOCK_SIZE != 0) {
    for(int i=0; i<A.numberOfRows % BLOCK_SIZE; i++) {
      int row = BLOCK_SIZE*(A.numberOfRows / BLOCK_SIZE)+i;
      for(int l=0; l<A.nonzerosInRow[row]; l++) {
        printf("%ld\n", *((long*) &(A.matrixValues[row][l])));
      }
    }
  }
  */

  IntVectorArray divided_colors[1];
  assign_blocks(A, color_nodes, divided_colors, 1, BLOCK_SIZE);

  printf("Colors vectors initialized\n");
  fflush(stdout);

  //int iter = VerifyResultBlocks(A, v, THRESHOLD, CHECK_FREQ, color_nodes, BLOCK_SIZE);

  int iter = VerifyResultColors(A, v, THRESHOLD, CHECK_FREQ, divided_colors[0]);

  printf("Test passed. Number of iterations: %d\n", iter);
  return 0;
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
    if(ESTIMATE) {
      //remainder is estimated to decrease exponentially
      old_expected_iter = expected_iter;
      expected_iter = estimateIterations(old_remainder, remainder, target, checkFreq, old_expected_iter);
      if(expected_iter > MAX_ITERATIONS-counter) {
        expected_iter = MAX_ITERATIONS-counter;
      }
      old_remainder = remainder;
    } else {
      expected_iter = 1;
    }

    //printf("Current iterations: %d, remainder: %lf, target: %lf, expected iter: %d\n", counter, remainder, target, expected_iter);
    //fflush(stdout);
  }
  printf("Test failed.\n");
  return -1;
}

//the same as VerifyResult, but with a colors DoubleVector and with ComputeSYMGS_GC instead
int VerifyResultColors(HPCGMatrix & A, DoubleVector & b, double threshold, int checkFreq, IntVectorArray colors) {
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
      ComputeSYMGS_GC_NoOMP(A, b, gs, colors);
      counter++;
    }
    SpMV(A, gs, res);
    Diff(res, b, diff);
    remainder = Abs(diff);
    if (remainder < target) {
      //printf("Test passed. Number of iterations: %d\n", counter);
      /*
      for(int i=0; i<gs.length; i++) {
        printf("%.20e ", gs.values[i]);
      }
      printf("\n");
      for(int i=0; i<gs.length; i++) {
        double max_val = abs(gs.values[i]);
        if(abs(res.values[i]) > max_val) {
          max_val = abs(res.values[i]);
        }
        double rel_diff = abs(diff.values[i])/max_val;
        printf("%.20e ", rel_diff);
      }
      printf("\n");
      fflush(stdout);
      */
      printf("Current iterations: %d, remainder: %.53e, target: %lf, expected iter: %d\n", counter, remainder, target, expected_iter);
      fflush(stdout);
      return counter;
    }
    if(ESTIMATE) {
      //remainder is estimated to decrease exponentially
      old_expected_iter = expected_iter;
      expected_iter = estimateIterations(old_remainder, remainder, target, checkFreq, old_expected_iter);
      if(expected_iter > MAX_ITERATIONS-counter) {
        expected_iter = MAX_ITERATIONS-counter;
      }
      old_remainder = remainder;
    } else {
      expected_iter = 1;
    }

    /*
    printf("Current iterations: %d, remainder: %lf, target: %lf, expected iter: %d\n", counter, remainder, target, expected_iter);
    for(int i=0; i<gs.length; i++) {
      printf("%.20e ", gs.values[i]);
    }
    printf("\n");
    for(int i=0; i<gs.length; i++) {
      double max_val = abs(gs.values[i]);
      if(abs(res.values[i]) > max_val) {
        max_val = abs(res.values[i]);
      }
      double rel_diff = abs(diff.values[i])/max_val;
      printf("%.20e ", rel_diff);
    }
    printf("\n");
    fflush(stdout);
    */
    printf("Current iterations: %d, remainder: %.53e, target: %lf, expected iter: %d\n", counter, remainder, target, expected_iter);
    fflush(stdout);
  }
  printf("Test failed.\n");
  return -1;
}

//the same as VerifyResult, but with a colors DoubleVector and with ComputeSYMGS_BC instead
int VerifyResultBlocks(HPCGMatrix & A, DoubleVector & b, double threshold, int checkFreq, IntVectorArray colors, int block_size) {
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
      ComputeSYMGS_BC_NoOMP(A, b, gs, colors, block_size);
      counter++;
    }
    SpMV(A, gs, res);
    Diff(res, b, diff);
    remainder = Abs(diff);

    /*
    for(int i=0; i<gs.length; i++) {
      printf("%lf\n", gs.values[i]);
    }
    */

    if (remainder < target) {
      printf("Current iterations: %d, remainder: %.53e, target: %lf, expected iter: %d\n", counter, remainder, target, expected_iter);
      fflush(stdout);
      //printf("Test passed. Number of iterations: %d\n", counter);
      return counter;
    }
    if(ESTIMATE) {
      //remainder is estimated to decrease exponentially
      old_expected_iter = expected_iter;
      expected_iter = estimateIterations(old_remainder, remainder, target, checkFreq, old_expected_iter);
      if(expected_iter > MAX_ITERATIONS-counter) {
        expected_iter = MAX_ITERATIONS-counter;
      }
      old_remainder = remainder;
    } else {
      expected_iter = CHECK_FREQ;
    }

    printf("Current iterations: %d, remainder: %.53e, target: %lf, expected iter: %d\n", counter, remainder, target, expected_iter);
    fflush(stdout);
  }
  printf("Test failed.\n");
  return -1;
}

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

int estimateIterations(double old_remainder, double remainder, double target, int checkFreq, int old_expected_iter) {
  if(old_remainder == -1 || old_remainder <= remainder) {
    return checkFreq;
  }
  int result = (int) (log(remainder/target)/log(old_remainder/remainder)*old_expected_iter);

  if(result < 0 || result > MAX_ITERATIONS) {
    return MAX_ITERATIONS;
  }

  if(result == 0) {
    return checkFreq;
  }

  return result;
}

//copied from preprocessing, used to check if it is working correctly
void assign_blocks(HPCGMatrix & A, IntVectorArray & colors, IntVectorArray result[], int n_acc, int block_size) {
    int num_colors = colors.length;
    //account for block sizes that do not cover the entire matrix
    if(A.numberOfColumns % block_size != 0) {
        num_colors++;
    }
    for(int acc=0; acc<n_acc; acc++) {
        InitializeIntVectorArr(result[acc], num_colors);
    }
    for(int color=0; color<colors.length; color++) {
        int nnz_acc[n_acc]; //number of nonzeros of this color for each accumulator
        int num_rows_acc[n_acc]; //number of rows of this color for each accumulator
        int* rows_acc[n_acc]; //array of rows of this color for each accumulator (temporary)
        for(int i=0; i<n_acc; i++) {
            nnz_acc[i] = 0;
            rows_acc[i] = (int*) malloc(colors.values[color].length*sizeof(int)*block_size);
            num_rows_acc[i] = 0;
        }
        for(int idx=0; idx<colors.values[color].length; idx++) {
            int min_nnz = A.numberOfNonzeros;
            int acc_min_nnz = 0;
            for(int i=0; i<n_acc; i++) {
                if(nnz_acc[i] < min_nnz) {
                    min_nnz = nnz_acc[i];
                    acc_min_nnz = i;
                }
            }
            int block = colors.values[color].values[idx];
            int nnz_block = 0;
            for(int i=block*block_size; i<(block+1)*block_size; i++) {
                nnz_block += A.nonzerosInRow[i];
            }
            nnz_acc[acc_min_nnz] += nnz_block;
            for(int i=0; i<block_size; i++) {
                rows_acc[acc_min_nnz][num_rows_acc[acc_min_nnz]+i] = block*block_size+i;
            }
            num_rows_acc[acc_min_nnz] += block_size;
        }
        for(int acc=0; acc<n_acc; acc++) {
            InitializeIntVector(result[acc].values[color], num_rows_acc[acc]);
            for(int row=0; row<num_rows_acc[acc]; row++) {
                result[acc].values[color].values[row] = rows_acc[acc][row];
            }
            free(rows_acc[acc]);
        }
    }
    //the remaining values are put in the first accumulator
    if(A.numberOfColumns % block_size != 0) {
        InitializeIntVector(result[0].values[num_colors-1], A.numberOfColumns % block_size);
        int counter = 0;
        for(int row = (A.numberOfColumns / block_size)*block_size; row < A.numberOfColumns; row++) {
            result[0].values[num_colors-1].values[counter] = row;
            counter++;
        }
        for(int i=1; i<n_acc; i++) {
            InitializeIntVector(result[i].values[num_colors-1], 0);
        }
    }
}