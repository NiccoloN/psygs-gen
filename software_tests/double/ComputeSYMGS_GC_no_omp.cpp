//like the NoOMP versions in ComputeSYMGS_GC, but without importing OMP and made to exactly simulate execution on the DSA

#include "ComputeSYMGS_GC_no_omp.hpp"
#include <cassert>

int ComputeSYMGS_GC_NoOMP( const HPCGMatrix & A, const DoubleVector & r, DoubleVector & x, IntVectorArray & colors) {

  assert(x.length==A.numberOfColumns); // Make sure x contain space for halo values

  const int nrow = A.numberOfRows;
  double ** matrixDiagonal = A.matrixDiagonal;  // An array of pointers to the diagonal entries A.matrixValues
  const double * const rv = r.values;
  double * const xv = x.values;

  for (int color=0; color<colors.length; color++) {
    for (int i=0; i<colors.values[color].length; i++) {
      int row = colors.values[color].values[i];
      const double * const currentValues = A.matrixValues[row];
      const int * const currentColIndices = A.mtxInd[row];
      const int currentNumberOfNonzeros = A.nonzerosInRow[row];
      const double  currentDiagonal = matrixDiagonal[row][0]; // Current diagonal value
      double sum1 = rv[row]; // RHS value
      double sum2 = 0.0;
      int firstSum = 1;
      double sum;

      for (int j=0; j< currentNumberOfNonzeros; j++) {
        int curCol = currentColIndices[j];
        if(curCol != row) {
          if(firstSum) {
            sum1 -= currentValues[j] * xv[curCol];
            firstSum = 0;
          } else {
            sum2 -= currentValues[j] * xv[curCol];
            firstSum = 1;
          }
        }
      }
      sum = sum1 + sum2;
      double invert = 1/currentDiagonal;
      xv[row] = sum*invert;
    }
  }


  // Now the back sweep.
  for(int color=colors.length-1; color>=0; color--) {
    for (int i=colors.values[color].length-1; i >= 0; i--) {
      int row = colors.values[color].values[i];
      const double * const currentValues = A.matrixValues[row];
      const int * const currentColIndices = A.mtxInd[row];
      const int currentNumberOfNonzeros = A.nonzerosInRow[row];
      const double  currentDiagonal = matrixDiagonal[row][0]; // Current diagonal value
      double sum1 = rv[row]; // RHS value
      double sum2 = 0.0;
      int firstSum = 1;
      double sum;

      for (int j = currentNumberOfNonzeros-1; j>=0; j--) {
        int curCol = currentColIndices[j];
        if(curCol != row) {
          if(firstSum) {
            sum1 -= currentValues[j] * xv[curCol];
            firstSum = 0;

          } else {
            sum2 -= currentValues[j] * xv[curCol];
            firstSum = 1;
          }
        }
      }
      sum = sum1 + sum2;
      double invert = 1/currentDiagonal;
      xv[row] = sum*invert;
    }
  }

  return 0;
}

int ComputeSYMGS_BC_NoOMP( const HPCGMatrix & A, const DoubleVector & r, DoubleVector & x, IntVectorArray & colors, int block_size) {
  assert(x.length==A.numberOfColumns); // Make sure x contain space for halo values

  const int nrow = A.numberOfRows;
  double ** matrixDiagonal = A.matrixDiagonal;  // An array of pointers to the diagonal entries A.matrixValues
  const double * const rv = r.values;
  double * const xv = x.values;

  for (int color=0; color<colors.length; color++) {
    IntVector blocks = colors.values[color];
    for (int i=0; i<blocks.length; i++) {
      int block = blocks.values[i];
      for(int row=block*block_size; row<(block+1)*block_size; row++) {
        const double * const currentValues = A.matrixValues[row];
        const int * const currentColIndices = A.mtxInd[row];
        const int currentNumberOfNonzeros = A.nonzerosInRow[row];
        const double  currentDiagonal = matrixDiagonal[row][0]; // Current diagonal value
        double sum1 = rv[row]; // RHS value
        double sum2 = 0.0;
        int firstSum = 1;
        double sum;

        for (int j=0; j< currentNumberOfNonzeros; j++) {
          int curCol = currentColIndices[j];
          if(curCol != row) {
            if(firstSum) {
              sum1 -= currentValues[j] * xv[curCol];
              firstSum = 0;
            } else {
              sum2 -= currentValues[j] * xv[curCol];
              firstSum = 1;
            }
          }
        }
        sum = sum1 + sum2;
        double invert = 1/currentDiagonal;
        xv[row] = sum*invert;
      }
    }
  }

  //do remaining rows forward
  for(int row=(A.numberOfColumns/block_size)*block_size; row<A.numberOfColumns; row++) {
    const double * const currentValues = A.matrixValues[row];
    const int * const currentColIndices = A.mtxInd[row];
    const int currentNumberOfNonzeros = A.nonzerosInRow[row];
    const double  currentDiagonal = matrixDiagonal[row][0]; // Current diagonal value
    double sum1 = rv[row]; // RHS value
    double sum2 = 0.0;
    int firstSum = 1;
    double sum;

    for (int j=0; j< currentNumberOfNonzeros; j++) {
      int curCol = currentColIndices[j];
      if(curCol != row) {
        if(firstSum) {
          sum1 -= currentValues[j] * xv[curCol];
          firstSum = 0;
        } else {
          sum2 -= currentValues[j] * xv[curCol];
          firstSum = 1;
        }
      }
    }
    sum = sum1 + sum2;
    double invert = 1/currentDiagonal;
    xv[row] = sum*invert;
  }

  //do remaining rows back
  for(int row = A.numberOfColumns-1; row >= (A.numberOfColumns/block_size)*block_size; row--) {
    const double * const currentValues = A.matrixValues[row];
    const int * const currentColIndices = A.mtxInd[row];
    const int currentNumberOfNonzeros = A.nonzerosInRow[row];
    const double  currentDiagonal = matrixDiagonal[row][0]; // Current diagonal value
    double sum1 = rv[row]; // RHS value
    double sum2 = 0.0;
    int firstSum = 1;
    double sum;

    for (int j=currentNumberOfNonzeros-1; j >= 0; j--) {
      int curCol = currentColIndices[j];
      if(curCol != row) {
        if(firstSum) {
          sum1 -= currentValues[j] * xv[curCol];
          firstSum = 0;

        } else {
          sum2 -= currentValues[j] * xv[curCol];
          firstSum = 1;
        }
      }
    }
    sum = sum1 + sum2;
    double invert = 1/currentDiagonal;
    xv[row] = sum*invert;
  }

  // Now the back sweep.
  for(int color=colors.length-1; color>=0; color--) {
    IntVector blocks = colors.values[color];
    for (int i=blocks.length-1; i>=0; i--) {
      int block = blocks.values[i];
      for(int row = (block+1)*block_size-1; row>=block*block_size; row--) {
        const double * const currentValues = A.matrixValues[row];
        const int * const currentColIndices = A.mtxInd[row];
        const int currentNumberOfNonzeros = A.nonzerosInRow[row];
        const double  currentDiagonal = matrixDiagonal[row][0]; // Current diagonal value
        double sum1 = rv[row]; // RHS value
        double sum2 = 0.0;
        int firstSum = 1;
        double sum;

        for (int j = currentNumberOfNonzeros-1; j >= 0; j--) {
          int curCol = currentColIndices[j];
          if(curCol != row) {
            if(firstSum) {
              sum1 -= currentValues[j] * xv[curCol];
              firstSum = 0;
            } else {
              sum2 -= currentValues[j] * xv[curCol];
              firstSum = 1;
            }
          }
        }
        sum = sum1 + sum2;
        double invert = 1/currentDiagonal;
        xv[row] = sum*invert;
      }
    }
  }

  return 0;
}