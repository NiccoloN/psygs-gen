//like double_hpcg_matrix but using floats

#ifndef HPCGMATRIX_HPP
#define HPCGMATRIX_HPP

#include <cassert>
#include <cstdlib>
#include <cmath>
#include <vector>
#include "FloatVector.hpp"
extern "C" {
  #include "../libs/mmio.c"
}

struct HPCGMatrix_STRUCT {
  int numberOfRows; //!< number of rows
  int numberOfColumns;  //!< number of columns
  int numberOfNonzeros;  //!< number of nonzeros
  //array che nell'i-esima posizione contiene il numero di non-zeri nella riga i
  int * nonzerosInRow;
  //matrice che nell'(i-j)-esima posizione contiene l'indice della colonna del j-esimo elemento non-zero della riga i
  int ** mtxInd; //!< matrix indices
  //matrice che nell'(i-j)-esima posizione contiene il valore dell j-esimo elemento non-zero della riga i
  float ** matrixValues; //!< values of matrix entries
  //matrice che nella prima colonna contiene il valore della diagonale, il resto è inutile
  float ** matrixDiagonal; //!< values of matrix diagonal entries
};
typedef struct HPCGMatrix_STRUCT HPCGMatrix;

inline void MakeSymmetricHPCG(HPCGMatrix & A);

inline float InitializeHPCGMatrix(HPCGMatrix & A, const char* filename) {
  FILE * file;
  MM_typecode matcode;
  int lastInd = 0;
  int lastRow = 0;
  int currNumNon0 = 0;
  float lastValue;
  int curInd;
  float maxValue = 0;
  bool symmetric = false;

  //parse first two lines
  file = fopen(filename, "r");

  mm_read_banner(file, &matcode);
  if(((char*) matcode)[3] == 'S') {
    symmetric = true;
  }
  mm_read_mtx_crd_size(file, &A.numberOfRows, &A.numberOfColumns, &A.numberOfNonzeros);

  //allocate structures that do not depend on the maximum number of nonzeros
  A.nonzerosInRow = (int*) malloc(A.numberOfRows*sizeof(int));
  A.mtxInd = (int**) malloc(A.numberOfRows*sizeof(int*));
  A.matrixValues = (float**) malloc(A.numberOfRows*sizeof(float*));
  A.matrixDiagonal = (float**) malloc(A.numberOfRows*sizeof(float*));
  for(int i=0; i<A.numberOfRows; i++) {
    A.matrixDiagonal[i] = (float*) malloc(sizeof(float));
  }

  //find the largest number of nonzeros per line
  int* cur_num_nonzeros = (int*) malloc(A.numberOfRows*sizeof(int));
  for(int i=0; i<A.numberOfRows; i++) {
    cur_num_nonzeros[i] = 0;
  }

  while (true) {
    if (fscanf(file, "%d", &lastRow) == -1) {
      break;
    }
    cur_num_nonzeros[lastRow-1]++;
    fscanf(file, "%d", &lastInd);
    fscanf(file, "%f", &lastValue);
  }
  int max_num_nonzeros = 0;
  for(int i=0; i<A.numberOfRows; i++) {
    if (max_num_nonzeros < cur_num_nonzeros[i]) {
      max_num_nonzeros = cur_num_nonzeros[i];
    }
    A.nonzerosInRow[i] = cur_num_nonzeros[i];
  }
  fclose(file);
  file = fopen(filename, "r");
  
  //allocate remaining structures
  for(int i=0; i<A.numberOfRows; i++) {
    A.mtxInd[i] = (int*) malloc(max_num_nonzeros*sizeof(int));
  }
  for(int i=0; i<A.numberOfRows; i++) {
    A.matrixValues[i] = (float*) malloc(max_num_nonzeros*sizeof(float));
  }

  //skip the first two lines of the input
  mm_read_banner(file, &matcode);
  mm_read_mtx_crd_size(file, &A.numberOfRows, &A.numberOfColumns, &A.numberOfNonzeros);

  //reinitialize the number of nonzeros per line
  for(int i=0; i<A.numberOfRows; i++) {
    cur_num_nonzeros[i] = 0;
  }

  //parse the rest of the matrix
  while (true) {
    if (fscanf(file, "%d", &lastRow) == -1) {
      break;
    }
    lastRow--;
    cur_num_nonzeros[lastRow]++;
    fscanf(file, "%d", &lastInd);
    curInd = cur_num_nonzeros[lastRow]-2;
    while(curInd >= 0 && A.mtxInd[lastRow][curInd] > lastInd-1) {
      A.mtxInd[lastRow][curInd+1] = A.mtxInd[lastRow][curInd];
      A.matrixValues[lastRow][curInd+1] = A.matrixValues[lastRow][curInd];
      curInd--;
    }
    A.mtxInd[lastRow][curInd+1] = lastInd-1;
    fscanf(file, "%f", &lastValue);
    A.matrixValues[lastRow][curInd+1] = lastValue;
    if(lastValue > maxValue) {
      maxValue = lastValue;
    } else if(lastValue < -maxValue) {
      maxValue = -lastValue;
    }
    if (lastInd-1 == lastRow) {
      A.matrixDiagonal[lastRow][0] = lastValue;
    }
  }

  if(symmetric) {
    MakeSymmetricHPCG(A);
  }

  return maxValue;
}

inline void PrintHPCGMatrix(HPCGMatrix & A) {
  for(int i=0; i<A.numberOfRows; i++) {
    for(int j=0; j<A.nonzerosInRow[i]; j++) {
      printf("%d:%d:%f ", i, A.mtxInd[i][j], A.matrixValues[i][j]);
    }
    printf("\n");
  }
  printf("\n");
}

inline void DeleteHPCGMatrix(HPCGMatrix & A) {
  free(A.nonzerosInRow);
  for(int i=0; i<A.numberOfRows; i++) {
    free(A.matrixDiagonal[i]);
    free(A.matrixValues[i]);
    free(A.mtxInd[i]);
  }
  free(A.matrixDiagonal);
  free(A.matrixValues);
  free(A.mtxInd);
  return;
}

inline void CopyHPCGMatrix(HPCGMatrix & original, HPCGMatrix & copy) {
  //allocate space
  copy.nonzerosInRow = (int*) malloc(original.numberOfRows*sizeof(int));
  copy.matrixDiagonal = (float**) malloc(original.numberOfRows*sizeof(float*));
  copy.mtxInd = (int**) malloc(original.numberOfRows*sizeof(int*));
  copy.matrixValues = (float**) malloc(original.numberOfRows*sizeof(float*));
  for(int i=0; i<original.numberOfRows; i++) {
    copy.mtxInd[i] = (int*) malloc(original.nonzerosInRow[i]*sizeof(int));
    copy.matrixValues[i] = (float*) malloc(original.nonzerosInRow[i]*sizeof(float));
    copy.matrixDiagonal[i] = (float*) malloc(1*sizeof(float));
  }

  //copy
  copy.numberOfRows = original.numberOfRows;
  copy.numberOfColumns = original.numberOfColumns;
  copy.numberOfNonzeros = original.numberOfNonzeros;

  for(int i=0; i<original.numberOfRows; i++) {
    copy.nonzerosInRow[i] = original.nonzerosInRow[i];
    copy.matrixDiagonal[i][0] = original.matrixDiagonal[i][0];
    for(int j=0; j<original.nonzerosInRow[i]; j++) {
      copy.mtxInd[i][j] = original.mtxInd[i][j];
      copy.matrixValues[i][j] = original.matrixValues[i][j];
    }
  }
}

//assumes A is a lower triangular matrix and copies its element above the diagonal
inline void MakeSymmetricHPCG(HPCGMatrix & A) {
  //calculates the real number of nonzeros
  std::vector<int> nonzerosInRow(A.numberOfRows);
  for(int i=0; i<A.numberOfRows; i++) {
    nonzerosInRow[i] = A.nonzerosInRow[i] - 1;
    for(int j=0; j<A.nonzerosInRow[i]; j++) {
      nonzerosInRow[A.mtxInd[i][j]]++;
    }
  }
  //stores a copy of the matrix and reallocates memory
  HPCGMatrix B;
  CopyHPCGMatrix(A, B);
  for(int i=0; i<A.numberOfRows; i++) {
    free(A.matrixValues[i]);
    free(A.mtxInd[i]);
    A.matrixValues[i] = (float*) malloc(nonzerosInRow[i]*sizeof(float));
    A.mtxInd[i] = (int*) malloc(nonzerosInRow[i]*sizeof(int));
    A.nonzerosInRow[i] = nonzerosInRow[i];
  }
  A.numberOfNonzeros = B.numberOfNonzeros*2-A.numberOfRows;
  //copies the original nonzeros in A
  std::vector<int> currNonzerosRow(A.numberOfRows, 0);
  for(int i=0; i<B.numberOfRows; i++) {
    for(int j=0; j<B.nonzerosInRow[i]; j++) {
      A.mtxInd[i][currNonzerosRow[i]] = B.mtxInd[i][j];
      A.matrixValues[i][currNonzerosRow[i]] = B.matrixValues[i][j];
      currNonzerosRow[i]++;
    }
  }
  //adds the transposed nonzeros to A
  for(int i=0; i<B.numberOfRows; i++) {
    for(int j=0; j<B.nonzerosInRow[i]; j++) {
      if(B.mtxInd[i][j] != i) {
        A.mtxInd[B.mtxInd[i][j]][currNonzerosRow[B.mtxInd[i][j]]] = i;
        A.matrixValues[B.mtxInd[i][j]][currNonzerosRow[B.mtxInd[i][j]]] = B.matrixValues[i][j];
        currNonzerosRow[B.mtxInd[i][j]]++;
      }
    }
  }
}

//computes A*v and stores the result in res
void SpMV(HPCGMatrix & A, FloatVector & v, FloatVector & res) {
  for(int i=0; i<res.length; i++) {
    res.values[i] = 0;
  }
  for(int i=0; i<res.length; i++) {
    for(int j=0; j<A.nonzerosInRow[i]; j++) {
      res.values[i] += A.matrixValues[i][j]*(v.values[A.mtxInd[i][j]]);
    }
  }
}

#endif // SPARSEMATRIX_HPP
