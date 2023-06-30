
//@HEADER
// ***************************************************
//
// HPCG: High Performance Conjugate Gradient Benchmark
//
// Contact:
// Michael A. Heroux ( maherou@sandia.gov)
// Jack Dongarra     (dongarra@eecs.utk.edu)
// Piotr Luszczek    (luszczek@eecs.utk.edu)
//
// ***************************************************
//@HEADER

#ifndef COMPUTESYMGS_REF_GC_HPP
#define COMPUTESYMGS_REF_GC_HPP
#include "../../fpga_tests/preprocessing/include/double_hpcg_matrix.hpp"
#include "../../fpga_tests/preprocessing/include/DoubleVector.hpp"
#include "../../fpga_tests/preprocessing/include/IntVectorArray.hpp"

int ComputeSYMGS_GC( const HPCGMatrix  & A, const DoubleVector& r, DoubleVector& x, IntVectorArray & colors, bool useOMP, int num_threads);
int ComputeSYMGS_GC_OMP( const HPCGMatrix  & A, const DoubleVector& r, DoubleVector& x, IntVectorArray & colors, int num_threads);
int ComputeSYMGS_GC_NoOMP( const HPCGMatrix  & A, const DoubleVector& r, DoubleVector& x, IntVectorArray & colors);
int ComputeSYMGS_BC( const HPCGMatrix  & A, const DoubleVector& r, DoubleVector& x, IntVectorArray & colors, int block_size, bool useOMP, int num_threads);
int ComputeSYMGS_BC_OMP( const HPCGMatrix  & A, const DoubleVector& r, DoubleVector& x, IntVectorArray & colors, int block_size, int num_threads);
int ComputeSYMGS_BC_NoOMP( const HPCGMatrix  & A, const DoubleVector& r, DoubleVector& x, IntVectorArray & colors, int block_size);

#endif // COMPUTESYMGS_REF_HPP
