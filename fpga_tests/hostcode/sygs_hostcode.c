#include "utils/sptl_rocc.h"
#include "utils/sygs_utils.h"
#include "utils/helepers_uart.h"

#include "generated/template_constants.h"
#include "generated/data_allocation_global.h"

#define SEND_DATA 0
#define CHECK_DATA 1
#define PRINT_CHECK_DATA 0
#define PRINT_FREQ 10 //print every (1 / PRINT_FREQ) of the total iterations are done

void checkResult(int num_rows, char* buff, int* iterations) {

    read_variables(var_data, num_rows, 0);

    kputs("CHECK_result");
    for(int i = 0; i < num_rows; i++)
        send_long(var_data[i]);
    kputs("END_result\n");

    kread(buff, 12);

    if(buff[0] == 'q')
        *iterations = 0;
    else if(buff[0] == 'e')
        *iterations = atoi(buff + 1);
    else
        kputs("Unknown command");
}

int main(void) {

    REG32(uart, UART_REG_TXCTRL) = UART_TXEN;
    REG32(uart, UART_REG_RXCTRL) = UART_RXEN;
    char start_cmd[1] = "s";

    while(1) {

        char start_buff[6];
        kread(start_buff, 6);

        int iterations;

        if (start_buff[0] == start_cmd[0]) {

            kputs("Starting");

            iterations = atoi(start_buff + 1);

#include "generated/data_allocation.h"

#if SEND_DATA
            kputs("SEND_var_data");
            for(int i = 0; i < num_rows; i++)
                send_long(var_data[i]);
            kputs("END_var_data");

            kputs("SEND_var_off");
            for(int i = 0; i < NUM_VAR_BANKS; i++)
                send_int(var_off[i]);
            kputs("END_var_off");

            kputs("SEND_acc_data");
            for(int i = 0; i < acc_data_size; i++)
                send_long(acc_data[i]);
            kputs("END_acc_data");

            kputs("SEND_acc_off");
            for(int i = 0; i < NUM_ACC; i++)
                send_int(acc_off[i]);
            kputs("END_acc_off");
#endif
            kputs("Data obtained");

            int num_elems;

            kputs("Writing data on var banks...");

            write_variables(var_data, num_rows);

#if CHECK_DATA
            read_variables(var_data, num_rows, 1);
#endif

#if PRINT_CHECK_DATA
            kputs("CHECK_var_data");
            for(int i = 0; i < num_rows; i++)
                send_long(var_data[i]);
            kputs("END_var_data");
#endif

            kputs("Writing data on acc banks...");

            for(int i = 0; i < NUM_ACC; i++) {

                if(i < NUM_ACC - 1) num_elems = acc_off[i + 1] - acc_off[i];
                else num_elems = acc_data_size - acc_off[i];

                write_chunk(acc_data, acc_off[i], 0, num_elems, i + 1);

#if CHECK_DATA
                read_chunk(acc_data, 0, acc_off[i], num_elems, i + 1, 1);
#endif
            }

#if PRINT_CHECK_DATA
            kputs("CHECK_acc_data");
            for(int i = 0; i < acc_data_size; i++)
                send_long(acc_data[i]);
            kputs("END_acc_data");
#endif

            kputs("Data written on bram banks!");

            unsigned long doneCycles = 0;
            unsigned long doneIterations = 0;

            int iter_before_print;
            int iter_blocks;
            int last_block_iterations;
            char buff[10];

            //DEBUG
            //iterations = 1;
            //-----

            while(iterations > 0) {

                iter_before_print = iterations / PRINT_FREQ;

                if(iter_before_print == 0) iter_blocks = 0;
                else iter_blocks = PRINT_FREQ;

                last_block_iterations = iterations % PRINT_FREQ;

                kwrite("Starting with iterations: ", 26);
                send_int(iterations);
                kputs("");

                for(int n = 0; n < iter_blocks; n++) {

                    for(int i = 0; i < iter_before_print; i++) {

                        /*kwrite("\n", 1);

                        kwrite("Forward iteration ", 18);
                        send_int(i);*/

                        doneCycles += start_exec(0);
                        /*kwrite("Cycles: ", 8);
                        send_long(cycles);*/

                        /*kwrite("Backwards iteration ", 20);
                        send_int(i);*/

                        doneCycles += start_exec(1);
                        /*kwrite("Cycles: ", 8);
                        send_long(cycles);*/
                    }

                    doneIterations += iter_before_print;

                    kwrite("Iteration block ", 16);
                    send_int(n);
                    kwrite("Iterations: ", 12);
                    send_long(doneIterations);
                    kwrite("Cycles: ", 8);
                    send_long(doneCycles);

                    doneIterations = 0;
                    doneCycles = 0;
                }

                if(last_block_iterations > 0) {

                    for(int i = 0; i < last_block_iterations; i++) {

                        /*kwrite("\n", 1);

                        kwrite("Forward iteration ", 18);
                        send_int(i);*/

                        doneCycles += start_exec(0);
                        /*kwrite("Cycles: ", 8);
                        send_long(cycles);*/

                        /*kwrite("Backwards iteration ", 20);
                        send_int(i);*/

                        doneCycles += start_exec(1);
                        /*kwrite("Cycles: ", 8);
                        send_long(cycles);*/
                    }

                    kputs("Last iteration block");
                    kwrite("Iterations: ", 12);
                    send_long(last_block_iterations);
                    kwrite("Cycles: ", 8);
                    send_long(doneCycles);

                    doneCycles = 0;
                }

                checkResult(num_rows, buff, &iterations);
            }

            return 0;
        }
        else kputs("Unknown command");
    }

    return 0;
}
