
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

#ifndef COMPUTESYMGS_REF_HPP
#define COMPUTESYMGS_REF_HPP
#include "../../fpga_tests/preprocessing/include/float_hpcg_matrix.hpp"
#include "../../fpga_tests/preprocessing/include/FloatVector.hpp"

int ComputeSYMGS_ref( const HPCGMatrix  & A, const FloatVector & r, FloatVector & x);

#endif // COMPUTESYMGS_REF_HPP
