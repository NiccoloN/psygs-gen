//computes coloring and rearranges matrix, rhs and vector to be usable by the DSA

#include <string>
#include <fstream>
#include "include/double_hpcg_matrix.hpp"
#include "include/IntVectorArray.hpp"
#include "include/DoubleVector.hpp"
#include "double_graph_coloring.cpp"
#include <iostream>

//default block size if it is used
#define BLOCK_SIZE 128

void assign_rows(HPCGMatrix & A, IntVectorArray & colors, IntVectorArray result[], int n_acc);
void assign_blocks(HPCGMatrix & A, IntVectorArray & colors, IntVectorArray result[], int n_acc, int block_size);
int generate_addr(HPCGMatrix & A, IntVectorArray divided_colors[], DoubleVector & b, DoubleVector & x, void * & mem_data, int * & mem_off, void * & acc_data, int * & acc_off, int n_acc, int n_ban);
void print_divided_colors(IntVectorArray divided_colors[], int n_acc);
void print_data_allocation(int num_rows, int acc_data_size, int n_acc, int n_ban);


//mem_data contains all data about the vector x
//mem_off contains offsets for access to mem_data, an offset for each memory bank
//acc_data contains all data for PEs
//acc_off contains offsets for access to acc_data, an offset for each PE

//arguments:
//1: path of the mtx file for the matrix
//2: number of memory banks
//3: numbr of PEs
//4: if "rand", the rhs is randomly generated an written to argv[5], otherwise it is read from argv[5]
//5: path of the rhs file
//6: type of coloring: 1: multi-coloring, 2: block multi-coloring, 3: dependency graph
//7: only relevant if using block multi-coloring, size of the block, if not set the default one is used
int main(int argc, char* argv[]) {
    int n_ban;
    int n_acc;
    char matrix_path[1000];
    HPCGMatrix A;
    DoubleVector b;
    int mode = 1;
    int block_size;
    if(argc == 6 || argc == 7 || argc == 8) {
        strcpy(matrix_path, argv[1]);
        n_ban = atoi(argv[2]);
        n_acc = atoi(argv[3]);
        double maxValue = InitializeHPCGMatrix(A, matrix_path);
        if(strcmp(argv[4], "rand") == 0) {
            InitializeDoubleVector(b, A.numberOfRows);
            FillRandomDoubleVector(b, 0, maxValue);
            ExportDoubleVector(b, argv[5]);
        }
        else {
            InitializeDoubleVectorFromPath(b, argv[5], A.numberOfColumns);
        }
        if(argc >= 7) {
            block_size = std::min(BLOCK_SIZE, A.numberOfRows/64);
            if(block_size == 0) block_size = 2;
            //std::cout << "Blocks size: " << block_size << "\n";
            mode = atoi(argv[6]);
            if(argc == 8) {
                block_size = atoi(argv[7]);
            }
            std::cout << "Blocks size: " << block_size << "\n";
        }
    } else {
        printf("Wrong number of arguments!\n");
        return 0;
    }

    DoubleVector x;
    IntVectorArray colors;
    IntVector bad_colors;
    IntVectorArray divided_colors[n_acc];
    void* mem_data;
    void* acc_data;
    int* mem_off;
    int* acc_off;
    
    /*
    for(int i=0; i<b.length; i++) {
        printf("%lf ", b.values[i]);
    }
    printf("\n");
    */

    InitializeDoubleVector(x, A.numberOfRows);
    if(mode == 1) {
        ComputeColors(A, bad_colors, colors);
        assign_rows(A, colors, divided_colors, n_acc);
    } else if(mode == 2) {
        ComputeBlockColoring(A, block_size, bad_colors, colors);
        assign_blocks(A, colors, divided_colors, n_acc, block_size);
    } else if(mode == 3) {
        ComputeColorsDepGraph(A, bad_colors, colors);
        assign_rows(A, colors, divided_colors, n_acc);
    } else {
        printf("Wrong mode: %d\n", mode);
        return 0;
    }
    
    /*
    for(int i=0; i<colors.length; i++) {
        printf("color %d\n", i);
        for(int j=0; j<colors.values[i].length; j++) {
            printf("%d ", colors.values[i].values[j]);
        }
        printf("\n");
    }
    */
    
    print_divided_colors(divided_colors, n_acc);

    /*
    for(int i=0; i<n_acc; i++) {
        printf("%d: ", divided_colors[i].length);
        for(int j=0; j<divided_colors[i].length; j++) {
            printf("%d, ", divided_colors[i].values[j].length);
        }
        printf("\n");
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
        printf(" %lf", ((double*) mem_data)[i]);
    }
    */

    print_data_allocation(A.numberOfRows, acc_data_size, n_acc, n_ban);

    //writing data to file
    std::ofstream outfile;
    outfile.open("data.txt");

    outfile << A.numberOfRows;
    for(int i = 0; i < A.numberOfRows; i++) outfile << " " << *((double*)mem_data + i);
    outfile << "\n";

    outfile << n_ban;
    for(int i = 0; i < n_ban; i++) outfile << " " << *((int*)mem_off + i);
    outfile << "\n";

    outfile << acc_data_size;
    for(int i = 0; i < acc_data_size; i++) outfile << " " << *((long*)acc_data + i);
    outfile << "\n";

    outfile << n_acc;
    for(int i = 0; i < n_acc; i++) outfile << " " << *((int*)acc_off + i);

    outfile.close();
}

//result is an array with color and rows data for each PE: each element is an IntVectorArray with an IntVector of rows per color assigned to the PE
//rows are assigned in a greedy way based on the number of nonzeros
void assign_rows(HPCGMatrix & A, IntVectorArray & colors, IntVectorArray result[], int n_acc) {
    for(int acc=0; acc<n_acc; acc++) {
        InitializeIntVectorArr(result[acc], colors.length);
    }
    for(int color=0; color<colors.length; color++) {
        int nnz_acc[n_acc]; //number of nonzeros of this color for each PE
        int num_rows_acc[n_acc]; //number of rows of this color for each PE
        int* rows_acc[n_acc]; //array of rows of this color for each PE (temporary)
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

//like assign_rows but each block is in the same PE
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
        int nnz_acc[n_acc]; //number of nonzeros of this color for each PE
        int num_rows_acc[n_acc]; //number of rows of this color for each PE
        int* rows_acc[n_acc]; //array of rows of this color for each PE (temporary)
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
    //the remaining values are put in the first PE
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

//reorders the data structures and puts data in mem_data, mem_off, acc_data, acc_off
int generate_addr(HPCGMatrix & A, IntVectorArray divided_colors[], DoubleVector & b, DoubleVector & x, void * & mem_data, int * & mem_off, void * & acc_data, int * & acc_off, int n_acc, int n_ban) {
    //memory banks (x values are assigned in order changing bank after each value)
    int total_size = 0;
    mem_off = (int*) malloc(sizeof(int)*n_ban);
    double* bank_x[n_ban];
    int sizes[n_ban];
    for(int bank=0; bank<n_ban; bank++) {
        int size = A.numberOfRows/n_ban;
        if(A.numberOfRows % n_ban > bank) {
            size++;
        }
        bank_x[bank] = (double*) malloc(sizeof(double)*size);
        sizes[bank] = size;
        mem_off[bank] = total_size;
        total_size += size;
        for(int i=0; i<size; i++) {
            bank_x[bank][i] = x.values[i*n_ban+bank];
        }
    }
    mem_data = malloc(total_size*sizeof(double));
    for(int bank=0; bank<n_ban; bank++) {
        memcpy(&(((double*) mem_data)[mem_off[bank]]), bank_x[bank], sizeof(double)*sizes[bank]);
    }

    //PEs
    int acc_sizes[n_acc];
    total_size = 0;
    acc_off = (int*) malloc(sizeof(int)*n_acc);
    void* acc_data_temp[n_acc];
    for(int acc=0; acc<n_acc; acc++) {
        long nnz = 0;
        long num_rows=0;
        for(int color=0; color<divided_colors[acc].length; color++) {
            for(int row=0; row<divided_colors[acc].values[color].length; row++) {
                num_rows++;
                nnz += A.nonzerosInRow[divided_colors[acc].values[color].values[row]];
            }
        }
        acc_sizes[acc] = 3+divided_colors[acc].length+3*num_rows+2*nnz;
        acc_off[acc] = total_size;
        total_size += acc_sizes[acc];
        acc_data_temp[acc] = (void*) malloc(sizeof(double)*(3+divided_colors[acc].length+3*num_rows+2*nnz));
        memcpy(&(((long**) acc_data_temp)[acc][0]), &nnz, sizeof(long)); //number of nonzeros for this PE
        memcpy(&(((long**) acc_data_temp)[acc][1]), &num_rows, sizeof(long)); //number of rows for this PE
        long long_length = (long) divided_colors[acc].length;
        memcpy(&(((long**) acc_data_temp)[acc][2]), &(long_length), sizeof(long)); //number of colors for this PE
        //printf("%ld %ld %ld\n", ((long**) acc_data_temp)[acc][0], ((long**) acc_data_temp)[acc][1], ((long**) acc_data_temp)[acc][2]);
        for(int color=0; color<divided_colors[acc].length; color++) {
            long value = divided_colors[acc].values[color].length; //number of rows for each color for this PE
            memcpy(&(((long**) acc_data_temp)[acc][3+color]), &value, sizeof(long));
        }
        int index = 3+divided_colors[acc].length;
        for(int color=0; color<divided_colors[acc].length; color++) {
            for(int ind=0; ind<divided_colors[acc].values[color].length; ind++) { //for each row for this PE
                int row = divided_colors[acc].values[color].values[ind];
                long nnz_row = A.nonzerosInRow[row];
                memcpy(&(((long**) acc_data_temp)[acc][index]), &(nnz_row), sizeof(long)); //number of nonzeros in said row
                long diag_index;
                for(int i=0; i<A.nonzerosInRow[row]; i++) {
                    if(A.mtxInd[row][i] == row) {
                        diag_index = i;
                        break;
                    }
                }
                memcpy(&(((long**) acc_data_temp)[acc][index+1]), &diag_index, sizeof(long)); //index of diagonal value of said row
                double value = b.values[row];
                memcpy(&(((double**) acc_data_temp)[acc][index+2]), &value, sizeof(double)); //value of said row of the RHS
                //printf("%ld %ld %ld %lf\n", row, ((long**) acc_data_temp)[acc][index], ((long**) acc_data_temp)[acc][index+1], ((double**) acc_data_temp)[acc][index+2]);
                index += 3;
            }
        }
        for(int color=0; color<divided_colors[acc].length; color++) {
            for(int ind=0; ind<divided_colors[acc].values[color].length; ind++) {
                int row = divided_colors[acc].values[color].values[ind];
                for(int i=0; i<A.nonzerosInRow[row]; i++) {
                    double value = A.matrixValues[row][i];
                    memcpy(&(((double**) acc_data_temp)[acc][index]), &value, sizeof(double)); //values of each nonzero of each row for this accelerator
                    //printf("%d %lf     ", row, ((double**) acc_data_temp)[acc][index]);
                    index++;
                }
            }
        }
        //printf("\n");
        for(int color=0; color<divided_colors[acc].length; color++) {
            for(int ind=0; ind<divided_colors[acc].values[color].length; ind++) {
                int row = divided_colors[acc].values[color].values[ind];
                for(int i=0; i<A.nonzerosInRow[row]; i++) {
                    long value = A.mtxInd[row][i];
                    memcpy(&(((long**) acc_data_temp)[acc][index]), &value, sizeof(long)); //indices of each nonzero of each row for this accelerator
                    //printf("%ld %ld %ld      ", row, value, ((long**) acc_data_temp)[acc][index]);
                    index++;
                }
            }
        }
        //printf("\n");
    }
    acc_data = malloc(total_size*sizeof(double));
    for(int acc=0; acc<n_acc; acc++) {
        memcpy((char*)acc_data+sizeof(double)*acc_off[acc], acc_data_temp[acc], sizeof(double)*acc_sizes[acc]);
    }

    for(int acc=0; acc<n_acc; acc++) {
        free(acc_data_temp[acc]);
    }
    /*
    for(int i=0; i<total_size; i++) {
        printf("%d ", ((int*) acc_data)[i]);
        printf("%lf\n", ((double*) acc_data)[i]);
    }
    for(int i=0; i<n_acc; i++) {
        printf("%d\n", acc_off[i]);
    }
    */
    return total_size;
}

//debug function
void print_divided_colors(IntVectorArray divided_colors[], int n_acc) {
    for(int acc=0; acc<n_acc; acc++) {
        printf("PE %d:\n", acc);
        for(int i=0; i<divided_colors[acc].length; i++) {
            printf("color %d:\n", i);
            for(int j=0; j<divided_colors[acc].values[i].length; j++) {
                printf("%d ", divided_colors[acc].values[i].values[j]);
            }
            printf("\n");
        }
    }
}

//writes headers specifying the size of the arrays.
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
    fprintf(file, "long var_data[%d];\n", num_rows);
    fprintf(file, "int var_off[%d];\n", n_ban);
    fprintf(file, "long acc_data[%d];\n", acc_data_size);
    fprintf(file, "int acc_off[%d];\n", n_acc);
    fputs("#endif //CSR_DATA_ALLOCATION_H\n", file);
    fclose(file);
}
