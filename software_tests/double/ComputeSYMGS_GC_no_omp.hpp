#ifndef COMPUTESYMGS_REF_GC_NOOMP_HPP
#define COMPUTESYMGS_REF_GC_NOOMP_HPP
#include "../../fpga_tests/preprocessing/include/double_hpcg_matrix.hpp"
#include "../../fpga_tests/preprocessing/include/DoubleVector.hpp"
#include "../../fpga_tests/preprocessing/include/IntVectorArray.hpp"

int ComputeSYMGS_GC_NoOMP( const HPCGMatrix  & A, const DoubleVector& r, DoubleVector& x, IntVectorArray & colors);
int ComputeSYMGS_BC_NoOMP( const HPCGMatrix  & A, const DoubleVector& r, DoubleVector& x, IntVectorArray & colors, int block_size);

#endif