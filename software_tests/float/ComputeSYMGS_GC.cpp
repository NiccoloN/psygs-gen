#include "ComputeSYMGS_GC.hpp"
#include <cassert>
#include <omp.h>

/*!
  Computes one step of symmetric Gauss-Seidel:

  Assumption about the structure of matrix A:
  - Each row 'i' of the matrix has nonzero diagonal value whose address is matrixDiagonal[i]
  - Entries in row 'i' are ordered such that:
       - lower triangular terms are stored before the diagonal element.
       - upper triangular terms are stored after the diagonal element.
       - No other assumptions are made about entry ordering.

  Symmetric Gauss-Seidel notes:
  - We use the input FloatVector x as the RHS and start with an initial guess for y of all zeros.
  - We perform one forward sweep.  x should be initially zero on the first GS sweep, but we do not attempt to exploit this fact.
  - We then perform one back sweep.
  - For simplicity we include the diagonal contribution in the for-j loop, then correct the sum after

  @param[in] A the known system matrix
  @param[in] r the input vector
  @param[inout] x On entry, x should contain relevant values, on exit x contains the result of one symmetric GS sweep with r as the RHS.


  @warning Early versions of this kernel (Version 1.1 and earlier) had the r and x arguments in reverse order, and out of sync with other kernels.

  @return returns 0 upon success and non-zero otherwise

  @see ComputeSYMGS
*/
int ComputeSYMGS_GC_OMP( const HPCGMatrix & A, const FloatVector & r, FloatVector & x, IntVectorArray & colors, int num_threads) {
  omp_set_num_threads(num_threads);
  omp_set_dynamic(0);
  assert(x.length==A.numberOfColumns); // Make sure x contain space for halo values
  const int nrow = A.numberOfRows;
  float ** matrixDiagonal = A.matrixDiagonal;  // An array of pointers to the diagonal entries A.matrixValues
  const float * const rv = r.values;
  float * const xv = x.values;
  float ** matrixValues = A.matrixValues;
  int ** mtxInd = A.mtxInd;
  int * nonzerosInRow = A.nonzerosInRow;

  #pragma omp parallel firstprivate(xv, matrixValues, mtxInd, nonzerosInRow, matrixDiagonal, rv)
  {
    for (int color=0; color<colors.length; color++) {
      IntVector rows = colors.values[color];
      #pragma omp for
      for (int i=0; i<rows.length; i++) {
        int row = rows.values[i];
        const float * const currentValues = A.matrixValues[row];
        const int * const currentColIndices = A.mtxInd[row];
        const int currentNumberOfNonzeros = A.nonzerosInRow[row];
        const float  currentDiagonal = matrixDiagonal[row][0]; // Current diagonal value
        float sum = rv[row]; // RHS value

        for (int j=0; j<currentNumberOfNonzeros; j++) {
          int curCol = currentColIndices[j];
          sum -= currentValues[j] * xv[curCol];
        }
        sum += xv[row]*currentDiagonal; // Remove diagonal contribution from previous loop

        xv[row] = sum/currentDiagonal;
      }
    }

    // Now the back sweep.
    for(int color=colors.length-1; color>=0; color--) {
      IntVector rows = colors.values[color];
      #pragma omp for
      for (int i=rows.length-1; i>=0; i--) {
        int row = rows.values[i];
        const float * const currentValues = A.matrixValues[row];
        const int * const currentColIndices = A.mtxInd[row];
        const int currentNumberOfNonzeros = A.nonzerosInRow[row];
        const float  currentDiagonal = matrixDiagonal[row][0]; // Current diagonal value
        float sum = rv[row]; // RHS value

        for (int j = 0; j< currentNumberOfNonzeros; j++) {
          int curCol = currentColIndices[j];
          sum -= currentValues[j]*xv[curCol];
        }
        sum += xv[row]*currentDiagonal; // Remove diagonal contribution from previous loop

        xv[row] = sum/currentDiagonal;
      }
    }
  }

  return 0;
}

int ComputeSYMGS_BC_OMP( const HPCGMatrix & A, const FloatVector & r, FloatVector & x, IntVectorArray & colors, int block_size, int num_threads) {
  omp_set_num_threads(num_threads);
  omp_set_dynamic(0);

  assert(x.length==A.numberOfColumns); // Make sure x contain space for halo values

  const int nrow = A.numberOfRows;
  float ** matrixDiagonal = A.matrixDiagonal;  // An array of pointers to the diagonal entries A.matrixValues
  const float * const rv = r.values;
  float * const xv = x.values;
  float ** matrixValues = A.matrixValues;
  int ** mtxInd = A.mtxInd;
  int * nonzerosInRow = A.nonzerosInRow;

  #pragma omp parallel firstprivate(xv, matrixValues, mtxInd, nonzerosInRow, matrixDiagonal, rv, block_size)
  {
    for (int color=0; color<colors.length; color++) {
      IntVector blocks = colors.values[color];
      #pragma omp for 
      for (int i=0; i<blocks.length; i++) {
        int block = blocks.values[i];
        for(int row=block*block_size; row<(block+1)*block_size; row++) {
          const float * const currentValues = A.matrixValues[row];
          const int * const currentColIndices = A.mtxInd[row];
          const int currentNumberOfNonzeros = A.nonzerosInRow[row];
          const float  currentDiagonal = matrixDiagonal[row][0]; // Current diagonal value
          float sum = rv[row]; // RHS value

          for (int j=0; j< currentNumberOfNonzeros; j++) {
            int curCol = currentColIndices[j];
            sum -= currentValues[j] * xv[curCol];
          }
          sum += xv[row]*currentDiagonal; // Remove diagonal contribution from previous loop

          xv[row] = sum/currentDiagonal;
        }
      }
    }

    //do remaining rows forward
    for(int row=(A.numberOfColumns/block_size)*block_size; row<A.numberOfColumns; row++) {
      const float * const currentValues = A.matrixValues[row];
      const int * const currentColIndices = A.mtxInd[row];
      const int currentNumberOfNonzeros = A.nonzerosInRow[row];
      const float  currentDiagonal = matrixDiagonal[row][0]; // Current diagonal value
      float sum = rv[row]; // RHS value

      for (int j=0; j< currentNumberOfNonzeros; j++) {
        int curCol = currentColIndices[j];
        sum -= currentValues[j] * xv[curCol];
      }
      sum += xv[row]*currentDiagonal; // Remove diagonal contribution from previous loop

      xv[row] = sum/currentDiagonal;
    }
    //do remaining rows back
    for(int row = A.numberOfColumns-1; row >= (A.numberOfColumns/block_size)*block_size; row--) {
      const float * const currentValues = A.matrixValues[row];
      const int * const currentColIndices = A.mtxInd[row];
      const int currentNumberOfNonzeros = A.nonzerosInRow[row];
      const float  currentDiagonal = matrixDiagonal[row][0]; // Current diagonal value
      float sum = rv[row]; // RHS value

      for (int j=0; j< currentNumberOfNonzeros; j++) {
        int curCol = currentColIndices[j];
        sum -= currentValues[j] * xv[curCol];
      }
      sum += xv[row]*currentDiagonal; // Remove diagonal contribution from previous loop

      xv[row] = sum/currentDiagonal;
    }

    // Now the back sweep.
    for(int color=colors.length-1; color>=0; color--) {
      IntVector blocks = colors.values[color];
      #pragma omp for 
      for (int i=blocks.length-1; i>=0; i--) {
        int block = blocks.values[i];
        for(int row = (block+1)*block_size-1; row>=block*block_size; row--) {
          const float * const currentValues = A.matrixValues[row];
          const int * const currentColIndices = A.mtxInd[row];
          const int currentNumberOfNonzeros = A.nonzerosInRow[row];
          const float  currentDiagonal = matrixDiagonal[row][0]; // Current diagonal value
          float sum = rv[row]; // RHS value

          for (int j = 0; j< currentNumberOfNonzeros; j++) {
            int curCol = currentColIndices[j];
            sum -= currentValues[j]*xv[curCol];
          }
          sum += xv[row]*currentDiagonal; // Remove diagonal contribution from previous loop

          xv[row] = sum/currentDiagonal;
        }
      }
    }
  }

  return 0;
}

int ComputeSYMGS_GC_NoOMP( const HPCGMatrix & A, const FloatVector & r, FloatVector & x, IntVectorArray & colors) {

  assert(x.length==A.numberOfColumns); // Make sure x contain space for halo values

  const int nrow = A.numberOfRows;
  float ** matrixDiagonal = A.matrixDiagonal;  // An array of pointers to the diagonal entries A.matrixValues
  const float * const rv = r.values;
  float * const xv = x.values;

  for (int color=0; color<colors.length; color++) {
    for (int i=0; i<colors.values[color].length; i++) {
      int row = colors.values[color].values[i];
      const float * const currentValues = A.matrixValues[row];
      const int * const currentColIndices = A.mtxInd[row];
      const int currentNumberOfNonzeros = A.nonzerosInRow[row];
      const float  currentDiagonal = matrixDiagonal[row][0]; // Current diagonal value
      float sum = rv[row]; // RHS value

      for (int j=0; j< currentNumberOfNonzeros; j++) {
        int curCol = currentColIndices[j];
        sum -= currentValues[j] * xv[curCol];
      }
      sum += xv[row]*currentDiagonal; // Remove diagonal contribution from previous loop

      xv[row] = sum/currentDiagonal;
    }
  }


  // Now the back sweep.
  for(int color=colors.length-1; color>=0; color--) {
    for (int i=colors.values[color].length-1; i >= 0; i--) {
      int row = colors.values[color].values[i];
      const float * const currentValues = A.matrixValues[row];
      const int * const currentColIndices = A.mtxInd[row];
      const int currentNumberOfNonzeros = A.nonzerosInRow[row];
      const float  currentDiagonal = matrixDiagonal[row][0]; // Current diagonal value
      float sum = rv[row]; // RHS value

      for (int j = 0; j< currentNumberOfNonzeros; j++) {
        int curCol = currentColIndices[j];
        sum -= currentValues[j]*xv[curCol];
      }
      sum += xv[row]*currentDiagonal; // Remove diagonal contribution from previous loop

      xv[row] = sum/currentDiagonal;
    }
  }

  return 0;
}

int ComputeSYMGS_BC_NoOMP( const HPCGMatrix & A, const FloatVector & r, FloatVector & x, IntVectorArray & colors, int block_size) {
  assert(x.length==A.numberOfColumns); // Make sure x contain space for halo values

  const int nrow = A.numberOfRows;
  float ** matrixDiagonal = A.matrixDiagonal;  // An array of pointers to the diagonal entries A.matrixValues
  const float * const rv = r.values;
  float * const xv = x.values;

  for (int color=0; color<colors.length; color++) {
    IntVector blocks = colors.values[color];
    for (int i=0; i<blocks.length; i++) {
      int block = blocks.values[i];
      for(int row=block*block_size; row<(block+1)*block_size; row++) {
        const float * const currentValues = A.matrixValues[row];
        const int * const currentColIndices = A.mtxInd[row];
        const int currentNumberOfNonzeros = A.nonzerosInRow[row];
        const float  currentDiagonal = matrixDiagonal[row][0]; // Current diagonal value
        float sum = rv[row]; // RHS value

        for (int j=0; j< currentNumberOfNonzeros; j++) {
          int curCol = currentColIndices[j];
          sum -= currentValues[j] * xv[curCol];
        }
        sum += xv[row]*currentDiagonal; // Remove diagonal contribution from previous loop

        xv[row] = sum/currentDiagonal;
      }
    }
  }

  //do remaining rows forward
  for(int row=(A.numberOfColumns/block_size)*block_size; row<A.numberOfColumns; row++) {
    const float * const currentValues = A.matrixValues[row];
    const int * const currentColIndices = A.mtxInd[row];
    const int currentNumberOfNonzeros = A.nonzerosInRow[row];
    const float  currentDiagonal = matrixDiagonal[row][0]; // Current diagonal value
    float sum = rv[row]; // RHS value

    for (int j=0; j< currentNumberOfNonzeros; j++) {
      int curCol = currentColIndices[j];
      sum -= currentValues[j] * xv[curCol];
    }
    sum += xv[row]*currentDiagonal; // Remove diagonal contribution from previous loop

    xv[row] = sum/currentDiagonal;
  }
  //do remaining rows back
  for(int row = A.numberOfColumns-1; row >= (A.numberOfColumns/block_size)*block_size; row--) {
    const float * const currentValues = A.matrixValues[row];
    const int * const currentColIndices = A.mtxInd[row];
    const int currentNumberOfNonzeros = A.nonzerosInRow[row];
    const float  currentDiagonal = matrixDiagonal[row][0]; // Current diagonal value
    float sum = rv[row]; // RHS value

    for (int j=0; j< currentNumberOfNonzeros; j++) {
      int curCol = currentColIndices[j];
      sum -= currentValues[j] * xv[curCol];
    }
    sum += xv[row]*currentDiagonal; // Remove diagonal contribution from previous loop

    xv[row] = sum/currentDiagonal;
  }


  // Now the back sweep.
  for(int color=colors.length-1; color>=0; color--) {
    IntVector blocks = colors.values[color];
    for (int i=blocks.length-1; i>=0; i--) {
      int block = blocks.values[i];
      for(int row = (block+1)*block_size-1; row>=block*block_size; row--) {
        const float * const currentValues = A.matrixValues[row];
        const int * const currentColIndices = A.mtxInd[row];
        const int currentNumberOfNonzeros = A.nonzerosInRow[row];
        const float  currentDiagonal = matrixDiagonal[row][0]; // Current diagonal value
        float sum = rv[row]; // RHS value

        for (int j = 0; j< currentNumberOfNonzeros; j++) {
          int curCol = currentColIndices[j];
          sum -= currentValues[j]*xv[curCol];
        }
        sum += xv[row]*currentDiagonal; // Remove diagonal contribution from previous loop

        xv[row] = sum/currentDiagonal;
      }
    }
  }

  return 0;
}

int ComputeSYMGS_GC( const HPCGMatrix  & A, const FloatVector & r, FloatVector & x, IntVectorArray & colors, bool useOMP, int num_threads) {
  if(useOMP) {
    return ComputeSYMGS_GC_OMP(A, r, x, colors, num_threads);
  } else {
    return ComputeSYMGS_GC_NoOMP(A, r, x, colors);
  }
}

int ComputeSYMGS_BC( const HPCGMatrix  & A, const FloatVector & r, FloatVector & x, IntVectorArray & colors, int block_size, bool useOMP, int num_threads) {
  if(useOMP) {
    return ComputeSYMGS_BC_OMP(A, r, x, colors, block_size, num_threads);
  } else {
    return ComputeSYMGS_BC_NoOMP(A, r, x, colors, block_size);
  }
}
