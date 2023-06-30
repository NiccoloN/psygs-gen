#include <fstream>
#include <iostream>
#include <cstdlib>
#include <string>
#include <filesystem>  
using std::endl;
namespace fs = std::filesystem;
#include "run_matrix_test.cpp"

#define NUM_ITER 4

int main() {
    std::string directory;
    printf("results for each method (4 iterations):\n");
    printf("standard GS: average time in us, number of iterations\n");
    printf("other methods: for 1 to 8 threads: number of threads, average time in us, average iterations, average speedup, average theoretical speedup, average weighted overhead\n");
    directory = "/home/francezco/Desktop/progetto_xohw/symmetric-gauss-seidel-src/resources/temp";
    for(const auto & entry : fs::directory_iterator(directory)) {
        std::string path = (std::string) entry.path();
        std::string mtx_name = (std::string) entry.path().filename().replace_extension("");
        std::string mtx_path = "/home/francezco/Desktop/progetto_xohw/symmetric-gauss-seidel-src/resources/matrices/" + mtx_name + ".mtx";
        std::string vec_path = "/home/francezco/Desktop/progetto_xohw/symmetric-gauss-seidel-src/resources/vectors/" + mtx_name + ".vec";
        printf("%s\n", &(mtx_path[0]));
        float std_res[2] = {0.0, 0.0};
        float* gc_res[4];
        float* bc_res[4];
        float* dg_res[4];
        float iter_std_res[2];
        float* iter_gc_res[4];
        float* iter_bc_res[4];
        float* iter_dg_res[4];
        for(int i=0; i<4; i++) {
            gc_res[i] = (float*) malloc(sizeof(float)*6);
            bc_res[i] = (float*) malloc(sizeof(float)*6);
            dg_res[i] = (float*) malloc(sizeof(float)*6);
            iter_gc_res[i] = (float*) malloc(sizeof(float)*6);
            iter_bc_res[i] = (float*) malloc(sizeof(float)*6);
            iter_dg_res[i] = (float*) malloc(sizeof(float)*6);
            for(int j=0; j<6; j++) {
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
                for(int j=0; j<6; j++) {
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
            for(int j=0; j<6; j++) {
                gc_res[i][j] /= NUM_ITER;
                bc_res[i][j] /= NUM_ITER;
                dg_res[i][j] /= NUM_ITER;
            }
        }
        

        for(int i=0; i<2; i++) {
            printf("%f ", std_res[i]);
        }
        printf("\n\n");
        for(int i=0; i<6; i++) {
            for(int j=0; j<4; j++) {
                printf("%f ", gc_res[j][i]);
            }
            printf("\n");
        }
        printf("\n\n");
        for(int i=0; i<6; i++) {
            for(int j=0; j<4; j++) {
                printf("%f ", bc_res[j][i]);
            }
            printf("\n");
        }
        printf("\n\n");
        for(int i=0; i<6; i++) {
            for(int j=0; j<4; j++) {
                printf("%f ", dg_res[j][i]);
            }
            printf("\n");
        }
        printf("\n\n\n");
        fflush(stdout);
    }
    return 0;
}