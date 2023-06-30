//gets data for all matrices in a directory, more info in the prints

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <string>
#include <filesystem>  
using std::endl;
namespace fs = std::filesystem;
#include "run_matrix_test.cpp"

#define NUM_ITER 1

int main() {
    std::string directory;
    printf("results for each method (%d iterations):\n", NUM_ITER);
    printf("standard GS: average time in us, number of iterations\n");
    printf("other methods: for 1 to 8 threads: number of threads, average time in us, average iterations, average speedup, average theoretical speedup, average weighted overhead, average clock cycles, average frequency\n");
    directory = "/home/francezco/Desktop/progetto_xohw/symmetric-gauss-seidel-src/resources/temp_mtx";
    for(const auto & entry : fs::directory_iterator(directory)) {
        std::string path = (std::string) entry.path();
        std::string mtx_name = (std::string) entry.path().filename().replace_extension("");
        std::string mtx_path = "/home/francezco/Desktop/progetto_xohw/symmetric-gauss-seidel-src/resources/temp_mtx/" + mtx_name + ".mtx";
        std::string vec_path = "/home/francezco/Desktop/progetto_xohw/symmetric-gauss-seidel-src/resources/temp_vec/" + mtx_name + ".vec";
        printf("%s\n", &(mtx_path[0]));
        double std_res[2] = {0.0, 0.0};
        double* gc_res[4];
        double* bc_res[4];
        double* dg_res[4];
        double iter_std_res[2];
        double* iter_gc_res[4];
        double* iter_bc_res[4];
        double* iter_dg_res[4];
        for(int i=0; i<4; i++) {
            gc_res[i] = (double*) malloc(sizeof(double)*8);
            bc_res[i] = (double*) malloc(sizeof(double)*8);
            dg_res[i] = (double*) malloc(sizeof(double)*8);
            iter_gc_res[i] = (double*) malloc(sizeof(double)*8);
            iter_bc_res[i] = (double*) malloc(sizeof(double)*8);
            iter_dg_res[i] = (double*) malloc(sizeof(double)*8);
            for(int j=0; j<8; j++) {
                gc_res[i][j] = 0;
                bc_res[i][j] = 0;
                dg_res[i][j] = 0;
            }
        }

        for(int i=0; i<NUM_ITER; i++) {
            run_matrix_test(&(mtx_path[0]), &(vec_path[0]), iter_std_res, iter_gc_res, iter_bc_res, iter_dg_res);
            for(int i=0; i<2; i++) {
                std_res[i] += iter_std_res[i];
            }
            for(int i=0; i<4; i++) {
                for(int j=0; j<8; j++) {
                    gc_res[i][j] += iter_gc_res[i][j];
                    bc_res[i][j] += iter_bc_res[i][j];
                    dg_res[i][j] += iter_dg_res[i][j];
                }
            }
        }

        for(int i=0; i<2; i++) {
            std_res[i] /= NUM_ITER;
        }
        for(int i=0; i<4; i++) {
            for(int j=0; j<8; j++) {
                gc_res[i][j] /= NUM_ITER;
                bc_res[i][j] /= NUM_ITER;
                dg_res[i][j] /= NUM_ITER;
            }
        }
        

        for(int i=0; i<2; i++) {
            printf("%lf ", std_res[i]);
        }
        printf("\n\n");
        for(int i=0; i<8; i++) {
            for(int j=0; j<4; j++) {
                printf("%lf ", gc_res[j][i]);
            }
            printf("\n");
        }
        printf("\n\n");
        for(int i=0; i<8; i++) {
            for(int j=0; j<4; j++) {
                printf("%lf ", bc_res[j][i]);
            }
            printf("\n");
        }
        printf("\n\n");
        for(int i=0; i<8; i++) {
            for(int j=0; j<4; j++) {
                printf("%lf ", dg_res[j][i]);
            }
            printf("\n");
        }
        printf("\n\n\n");
        fflush(stdout);
    }
    return 0;
}