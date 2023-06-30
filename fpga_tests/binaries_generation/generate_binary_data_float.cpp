//like generate_binary_data, but using int and float instead of long and double

#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>

void get_data(void* & mem_data, void* & mem_off, void* & acc_data, void* & acc_off, int & num_rows, int & acc_data_size, int & n_acc, int & n_ban);
void write_binary_data(char* matrix_name, int addresses[], void* mem_data, void* mem_off, void* acc_data, void* acc_off, int num_rows, int acc_data_size, int n_acc, int n_ban);

int main(int argc, char* argv[]) {
    char matrix_name[1000];
    int addresses[4];
    if(argc == 6) {
        strcpy(matrix_name, argv[1]);
        for(int i=0; i<4; i++) {
            addresses[i] = atoi(argv[2+i]);
        }
    } else {
        printf("Not enough arguments!");
        return 0;
    }

    void* mem_data;
    void* mem_off;
    void* acc_data;
    void* acc_off;
    int num_rows;
    int acc_data_size;
    int n_acc;
    int n_ban;
    
    get_data(mem_data, mem_off, acc_data, acc_off, num_rows, acc_data_size, n_acc, n_ban);

    /*
    for(int i=0; i<num_rows; i++) {
        printf("%f ", ((float*) mem_data)[i]);
    }
    printf("\n");
    for(int i=0; i<n_ban; i++) {
        printf("%d ", ((int*) mem_off)[i]);
    }
    printf("\n");
    for(int i=0; i<acc_data_size; i++) {
        printf("%d ", ((int*) acc_data)[i]);
    }
    printf("\n");
    for(int i=0; i<n_acc; i++) {
        printf("%d ", ((int*) acc_off)[i]);
    }
    printf("\n");
    */

    write_binary_data(matrix_name, addresses, mem_data, mem_off, acc_data, acc_off, num_rows, acc_data_size, n_acc, n_ban);
}

void get_data(void* & mem_data, void* & mem_off, void* & acc_data, void* & acc_off, int & num_rows, int & acc_data_size, int & n_acc, int & n_ban) {
    std::ifstream infile;
    char temp;
    infile.open("../preprocessing/data.txt");

    infile >> num_rows;
    mem_data = malloc(sizeof(float)*num_rows);
    for(int i = 0; i < num_rows; i++) infile >> *((float*)mem_data + i);

    infile >> n_ban;
    mem_off = malloc(sizeof(int)*n_ban);
    for(int i = 0; i < n_ban; i++) infile >> *((int*)mem_off + i);

    infile >> acc_data_size;
    acc_data = malloc(sizeof(int)*acc_data_size);
    for(int i = 0; i < acc_data_size; i++) infile >> *((int*)acc_data + i);

    infile >> n_acc;
    acc_off = malloc(sizeof(int)*n_acc);
    for(int i = 0; i < n_acc; i++) infile >> *((int*)acc_off + i);

    infile.close();
}

void write_binary_data(char* matrix_name, int addresses[], void* mem_data, void* mem_off, void* acc_data, void* acc_off, int num_rows, int acc_data_size, int n_acc, int n_ban) {
    std::string binary_path = "binaries/" + std::string(matrix_name) + "-data.bin";
    FILE* file = fopen(&(binary_path[0]), "wb");

    /*for(int i = 0; i < acc_data_size; i++)
        printf("%d\n", *((int*)(acc_data + i*sizeof(int))));*/

    int bres;
    for(int i=0; i<4; i++) {
        int num_before;
        int cur_index;

        //write mem_data
        num_before = 0;
        cur_index = 0;
        for(int j=0; j<4; j++) {
            if(addresses[j] < addresses[cur_index]) {
                num_before++;
            }
        }
        if(num_before == i) {
            fwrite(mem_data, num_rows*sizeof(float), 1, file);
            bres = (num_rows * 4) % 8;
        }

        //write mem_off
        num_before = 0;
        cur_index = 1;
        for(int j=0; j<4; j++) {
            if(addresses[j] < addresses[cur_index]) {
                num_before++;
            }
        }
        if(num_before == i) {
            fwrite(mem_off, n_ban*sizeof(int), 1, file);
            bres = (n_ban * 4) % 8;
        }

        //write acc_data
        num_before = 0;
        cur_index = 2;
        for(int j=0; j<4; j++) {
            if(addresses[j] < addresses[cur_index]) {
                num_before++;
            }
        }
        if(num_before == i) {
            fwrite(acc_data, acc_data_size*sizeof(int), 1, file);
            bres = (acc_data_size * 4) % 8;
        }

        //write acc_off
        num_before = 0;
        cur_index = 3;
        for(int j=0; j<4; j++) {
            if(addresses[j] < addresses[cur_index]) {
                num_before++;
            }
        }
        if(num_before == i) {
            fwrite(acc_off, n_acc*sizeof(int), 1, file);
            bres = (n_acc * 4) % 8;
        }


        for(int j=0; j<bres; j++) {
            fputc(0, file);
        }
    }
}