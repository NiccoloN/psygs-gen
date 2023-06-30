//like preprocess but using floats

#include <string>
#include <fstream>
#include "include/float_hpcg_matrix.hpp"
#include "include/IntVectorArray.hpp"
#include "include/FloatVector.hpp"
#include "float_graph_coloring.cpp"

void assign_rows(HPCGMatrix & A, IntVectorArray & colors, IntVectorArray result[], int n_acc);
int generate_addr(HPCGMatrix & A, IntVectorArray divided_colors[], FloatVector & b, FloatVector & x, void * & mem_data, int * & mem_off, void * & acc_data, int * & acc_off, int n_acc, int n_ban);
void print_divided_colors(IntVectorArray divided_colors[], int n_acc);
void print_data_allocation(int num_rows, int acc_data_size, int n_acc, int n_ban);

//mem_data contains all data about the vector x
//mem_off contains offsets for access to mem_data, an offset for each memory bank
//acc_data contains all data for accumulators
//acc_off contains offsets for access to acc_data, an offset for each accumulator
int main(int argc, char* argv[]) {
    int n_ban;
    int n_acc;
    char matrix_path[1000];
    HPCGMatrix A;
    FloatVector b;
    if(argc == 6) {
        strcpy(matrix_path, argv[1]);
        n_ban = atoi(argv[2]);
        n_acc = atoi(argv[3]);
        float maxValue = InitializeHPCGMatrix(A, matrix_path);
        if(strcmp(argv[4], "rand") == 0) {
            InitializeFloatVector(b, A.numberOfRows);
            FillRandomFloatVector(b, 0, maxValue);
            ExportFloatVector(b, argv[5]);
        }
        else {
            InitializeFloatVectorFromPath(b, argv[5], A.numberOfColumns);
        }
    } else {
        printf("Not enough arguments!\n");
        return 0;
    }

    FloatVector x;
    IntVectorArray colors;
    IntVector bad_colors;
    IntVectorArray divided_colors[n_acc];
    void* mem_data;
    void* acc_data;
    int* mem_off;
    int* acc_off;
    
    /*
    for(int i=0; i<b.length; i++) {
        printf("%f ", b.values[i]);
    }
    printf("\n");
    */

    InitializeFloatVector(x, A.numberOfRows);
    ComputeColors(A, bad_colors, colors);
    /*
    for(int i=0; i<colors.length; i++) {
        printf("color %d\n", i);
        for(int j=0; j<colors.values[i].length; j++) {
            printf("%d ", colors.values[i].values[j]);
        }
        printf("\n");
    }
    print_divided_colors(divided_colors, n_acc);
    */

    assign_rows(A, colors, divided_colors, n_acc);

    //print for debugging how rows are devided by accumulator
    /*
    for(int i=0; i<n_acc; i++) {
        printf("\n\naccumulator %d:", i);
        for(int j=0; j<divided_colors[i].length; j++) {
            printf("\ncolor %d:\n", j);
            for(int k=0; k<divided_colors[i].values[j].length; k++) {
                printf("%d ", divided_colors[i].values[j].values[k]);
            }
        }
    }
    */

    int acc_data_size = generate_addr(A, divided_colors, b, x, mem_data, mem_off, acc_data, acc_off, n_acc, n_ban);

    /*
    printf("Accumulator 0:\n");
    printf("Bank 0:");
    int curBank = 0;
    for(int i=0; i<A.numberOfRows; i++) {
        if(curBank < n_ban-1 && i >= mem_off[curBank+1]) {
            curBank++;
            printf("\nBank %d:", curBank);
        }
        printf(" %f", ((float*) mem_data)[i]);
    }
    */

    print_data_allocation(A.numberOfRows, acc_data_size, n_acc, n_ban);

    //writing data to file
    std::ofstream outfile;
    outfile.open("data.txt");

    outfile << A.numberOfRows;
    for(int i = 0; i < A.numberOfRows; i++) outfile << " " << *((float*)mem_data + i);
    outfile << "\n";

    outfile << n_ban;
    for(int i = 0; i < n_ban; i++) outfile << " " << *((int*)mem_off + i);
    outfile << "\n";

    outfile << acc_data_size;
    for(int i = 0; i < acc_data_size; i++) outfile << " " << *((int*)acc_data + i);
    outfile << "\n";

    outfile << n_acc;
    for(int i = 0; i < n_acc; i++) outfile << " " << *((int*)acc_off + i);

    outfile.close();
}

void initialize_vector(FloatVector & b, int start_addr, int num_rows) {
    float* addr = (float*) start_addr;
    InitializeFloatVector(b, num_rows);
    for(int i=0; i<num_rows; i++) {
        b.values[i] = *(addr+i*sizeof(float));
    }
}

//result is an array with color and rows data for each accumulator: each element is an IntVectorArray with an IntVector of rows per color assigned to the accumulator
//rows are assigned in a greedy way based on the number of nonzeros
void assign_rows(HPCGMatrix & A, IntVectorArray & colors, IntVectorArray result[], int n_acc) {
    for(int acc=0; acc<n_acc; acc++) {
        InitializeIntVectorArr(result[acc], colors.length);
    }
    for(int color=0; color<colors.length; color++) {
        int nnz_acc[n_acc]; //number of nonzeros of this color for each accumulator
        int num_rows_acc[n_acc]; //number of rows of this color for each accumulator
        int* rows_acc[n_acc]; //array of rows of this color for each accumulator (temporary)
        for(int i=0; i<n_acc; i++) {
            nnz_acc[i] = 0;
            rows_acc[i] = (int*) malloc(colors.values[color].length*sizeof(int));
            num_rows_acc[i] = 0;
        }
        for(int row=0; row<colors.values[color].length; row++) {
            int min_nnz = A.numberOfNonzeros;
            int acc_min_nnz = 0;
            for(int i=0; i<n_acc; i++) {
                if(nnz_acc[i] < min_nnz) {
                    min_nnz = nnz_acc[i];
                    acc_min_nnz = i;
                }
            }
            int real_row = colors.values[color].values[row];
            nnz_acc[acc_min_nnz] += A.nonzerosInRow[real_row];
            rows_acc[acc_min_nnz][num_rows_acc[acc_min_nnz]] = real_row;
            num_rows_acc[acc_min_nnz]++;
        }
        for(int acc=0; acc<n_acc; acc++) {
            InitializeIntVector(result[acc].values[color], num_rows_acc[acc]);
            for(int row=0; row<num_rows_acc[acc]; row++) {
                result[acc].values[color].values[row] = rows_acc[acc][row];
            }
            free(rows_acc[acc]);
        }
    }
}

int generate_addr(HPCGMatrix & A, IntVectorArray divided_colors[], FloatVector & b, FloatVector & x, void * & mem_data, int * & mem_off, void * & acc_data, int * & acc_off, int n_acc, int n_ban) {
    //accumulator 0 (x values are assigned in order changing bank after each value)
    int total_size = 0;
    mem_off = (int*) malloc(sizeof(int)*n_ban);
    float* bank_x[n_ban];
    int sizes[n_ban];
    for(int bank=0; bank<n_ban; bank++) {
        int size = A.numberOfRows/n_ban;
        if(A.numberOfRows % n_ban > bank) {
            size++;
        }
        bank_x[bank] = (float*) malloc(sizeof(float)*size);
        sizes[bank] = size;
        mem_off[bank] = total_size;
        total_size += size;
        for(int i=0; i<size; i++) {
            bank_x[bank][i] = x.values[i*n_ban+bank];
        }
    }
    mem_data = malloc(total_size*sizeof(float));
    for(int bank=0; bank<n_ban; bank++) {
        memcpy(&(((float*) mem_data)[mem_off[bank]]), bank_x[bank], sizeof(float)*sizes[bank]);
    }

    //other accumulators
    int acc_sizes[n_acc];
    total_size = 0;
    acc_off = (int*) malloc(sizeof(int)*n_acc);
    void* acc_data_temp[n_acc];
    for(int acc=0; acc<n_acc; acc++) {
        int nnz = 0;
        int num_rows=0;
        for(int color=0; color<divided_colors[acc].length; color++) {
            for(int row=0; row<divided_colors[acc].values[color].length; row++) {
                num_rows++;
                nnz += A.nonzerosInRow[divided_colors[acc].values[color].values[row]];
            }
        }
        acc_sizes[acc] = 3+divided_colors[acc].length+3*num_rows+2*nnz;
        acc_off[acc] = total_size;
        total_size += acc_sizes[acc];
        acc_data_temp[acc] = (void*) malloc(sizeof(float)*(3+divided_colors[acc].length+3*num_rows+2*nnz));
        memcpy(&(((int**) acc_data_temp)[acc][0]), &nnz, sizeof(int)); //number of nonzeros for this accumulator
        memcpy(&(((int**) acc_data_temp)[acc][1]), &num_rows, sizeof(int)); //number of rows for this accumulator
        memcpy(&(((int**) acc_data_temp)[acc][2]), &(divided_colors[acc].length), sizeof(int)); //number of colors for this accumulator
        //printf("%d %d %d\n", ((int**) acc_data_temp)[acc][0], ((int**) acc_data_temp)[acc][1], ((int**) acc_data_temp)[acc][2]);
        for(int color=0; color<divided_colors[acc].length; color++) {
            int value = divided_colors[acc].values[color].length; //number of rows for each color for this accumulator
            memcpy(&(((int**) acc_data_temp)[acc][3+color]), &value, sizeof(int));
        }
        int index = 3+divided_colors[acc].length;
        for(int color=0; color<divided_colors[acc].length; color++) {
            for(int ind=0; ind<divided_colors[acc].values[color].length; ind++) { //for each row for this accumulator
                int row = divided_colors[acc].values[color].values[ind];
                memcpy(&(((int**) acc_data_temp)[acc][index]), &(A.nonzerosInRow[row]), sizeof(int)); //number of nonzeros in said row
                int diag_index;
                for(int i=0; i<A.nonzerosInRow[row]; i++) {
                    if(A.mtxInd[row][i] == row) {
                        diag_index = i;
                        break;
                    }
                }
                memcpy(&(((int**) acc_data_temp)[acc][index+1]), &diag_index, sizeof(int)); //index of diagonal value of said row
                float value = b.values[row];
                memcpy(&(((float**) acc_data_temp)[acc][index+2]), &value, sizeof(float)); //value of said row of the RHS
                //printf("%d %d %d %f\n", row, ((int**) acc_data_temp)[acc][index], ((int**) acc_data_temp)[acc][index+1], ((float**) acc_data_temp)[acc][index+2]);
                index += 3;
            }
        }
        for(int color=0; color<divided_colors[acc].length; color++) {
            for(int ind=0; ind<divided_colors[acc].values[color].length; ind++) {
                int row = divided_colors[acc].values[color].values[ind];
                for(int i=0; i<A.nonzerosInRow[row]; i++) {
                    float value = A.matrixValues[row][i];
                    memcpy(&(((float**) acc_data_temp)[acc][index]), &value, sizeof(float)); //values of each nonzero of each row for this accelerator
                    //printf("%d %f     ", row, ((float**) acc_data_temp)[acc][index]);
                    index++;
                }
            }
        }
        //printf("\n");
        for(int color=0; color<divided_colors[acc].length; color++) {
            for(int ind=0; ind<divided_colors[acc].values[color].length; ind++) {
                int row = divided_colors[acc].values[color].values[ind];
                for(int i=0; i<A.nonzerosInRow[row]; i++) {
                    int value = A.mtxInd[row][i];
                    memcpy(&(((int**) acc_data_temp)[acc][index]), &value, sizeof(int)); //indices of each nonzero of each row for this accelerator
                    //printf("%d %d %d      ", row, value, ((int**) acc_data_temp)[acc][index]);
                    index++;
                }
            }
        }
        //printf("\n");
    }
    acc_data = malloc(total_size*sizeof(float));
    for(int acc=0; acc<n_acc; acc++) {
        memcpy((char*)acc_data+sizeof(float)*acc_off[acc], acc_data_temp[acc], sizeof(float)*acc_sizes[acc]);
    }

    for(int acc=0; acc<n_acc; acc++) {
        free(acc_data_temp[acc]);
    }
    /*
    for(int i=0; i<total_size; i++) {
        printf("%d ", ((int*) acc_data)[i]);
        printf("%f\n", ((float*) acc_data)[i]);
    }
    for(int i=0; i<n_acc; i++) {
        printf("%d\n", acc_off[i]);
    }
    */
    return total_size;
}

void print_divided_colors(IntVectorArray divided_colors[], int n_acc) {
    for(int acc=0; acc<n_acc; acc++) {
        printf("accumulator %d:\n", acc);
        for(int i=0; i<divided_colors[acc].length; i++) {
            printf("color %d:\n", i);
            for(int j=0; j<divided_colors[acc].values[i].length; j++) {
                printf("%d ", divided_colors[acc].values[i].values[j]);
            }
            printf("\n");
        }
    }
}

void print_data_allocation(int num_rows, int acc_data_size, int n_acc, int n_ban) {
    //local
    FILE* file;
    file = fopen("../hostcode/generated/data_allocation.h", "w");
    fputs("#ifndef CSR_DATA_ALLOCATION_LOCAL_H\n", file);
    fputs("#define CSR_DATA_ALLOCATION_LOCAL_H\n", file);
    fprintf(file, "int num_rows = %d;\n", num_rows);
    fprintf(file, "int acc_data_size = %d;\n", acc_data_size);
    fputs("#endif //CSR_DATA_ALLOCATION_LOCAL_H\n", file);
    fclose(file);

    //global
    file = fopen("../hostcode/generated/data_allocation_global.h", "w");
    fputs("#ifndef CSR_DATA_ALLOCATION_H\n", file);
    fputs("#define CSR_DATA_ALLOCATION_H\n", file);
    fprintf(file, "int var_data[%d];\n", num_rows);
    fprintf(file, "int var_off[%d];\n", n_ban);
    fprintf(file, "int acc_data[%d];\n", acc_data_size);
    fprintf(file, "int acc_off[%d];\n", n_acc);
    fputs("#endif //CSR_DATA_ALLOCATION_H\n", file);
    fclose(file);
}
