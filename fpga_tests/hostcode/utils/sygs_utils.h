#include <stdio.h>
#include <math.h>
#include "helepers_uart.h"
#include "../generated/template_constants.h"

static inline long write_to_accel_mem(long val, unsigned long addr, unsigned interface) {
    // |lmem_id| addr| val | 1  | 1   |  1   |ack_reg| CUSTOM_1 |
    if((interface == 0 && addr > VAR_BANK_SIZE * NUM_VAR_BANKS) ||
       (interface > 0 && (addr > ACC_BANK_SIZE))) {

        kputs("!!!");
        kputs("ERROR: Mem address violation at: ");
        kwrite("    Interface: ",15);
        send_int(interface);
        kwrite("    Address: ",13);
        send_long(addr);
        kputs("!!!");
    }
    long resp_val = 0;
    switch(interface) {
        case 0:
            ROCC_INSTRUCTION_DS(0, resp_val, addr, 0)
            break;
        case 1:
            ROCC_INSTRUCTION_DS(0, resp_val, addr, 1)
            break;
        case 2:
            ROCC_INSTRUCTION_DS(0, resp_val, addr, 2)
            break;
        case 3:
            ROCC_INSTRUCTION_DS(0, resp_val, addr, 3)
            break;
        case 4:
            ROCC_INSTRUCTION_DS(0, resp_val, addr, 4)
            break;
        case 5:
            ROCC_INSTRUCTION_DS(0, resp_val, addr, 5)
            break;
        case 6:
            ROCC_INSTRUCTION_DS(0, resp_val, addr, 6)
            break;
        case 7:
            ROCC_INSTRUCTION_DS(0, resp_val, addr, 7)
            break;
        case 8:
            ROCC_INSTRUCTION_DS(0, resp_val, addr, 8)
            break;
        case 9:
            ROCC_INSTRUCTION_DS(0, resp_val, addr, 9)
            break;
        case 10:
            ROCC_INSTRUCTION_DS(0, resp_val, addr, 10)
            break;
        case 11:
            ROCC_INSTRUCTION_DS(0, resp_val, addr, 11)
            break;
        case 12:
            ROCC_INSTRUCTION_DS(0, resp_val, addr, 12)
            break;
        case 13:
            ROCC_INSTRUCTION_DS(0, resp_val, addr, 13)
            break;
        case 14:
            ROCC_INSTRUCTION_DS(0, resp_val, addr, 14)
            break;
        case 15:
            ROCC_INSTRUCTION_DS(0, resp_val, addr, 15)
            break;
        default:
            break;
    }
    /*kputs("writing");
    send_long(*((int*)&val + 1));
    send_long(*((int*)&val));*/
    ROCC_INSTRUCTION_DSS(1, resp_val, *((int*)&val + 1), *((int*)&val), 0) //funct = dontCare
    return resp_val;
}

static inline long read_from_accel_mem(unsigned long addr, unsigned interface) {
    // |lmem_id| addr| -   | 1  | 1   |  0   |  res  | CUSTOM_2 |
    if((interface == 0 && addr > VAR_BANK_SIZE * NUM_VAR_BANKS) ||
       (interface > 0 && (addr > ACC_BANK_SIZE))) {

        kputs("!!!");
        kputs("ERROR: Mem address violation at: ");
        kwrite("    Interface: ",15);
        send_int(interface);
        kwrite("    Address: ",13);
        send_long(addr);
        kputs("!!!");
    }
    long read_val = 0;
    long data64 = 0;
    switch(interface) {
        case 0:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 0, 0)
            break;
        case 1:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 0, 1)
            break;
        case 2:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 0, 2)
            break;
        case 3:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 0, 3)
            break;
        case 4:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 0, 4)
            break;
        case 5:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 0, 5)
            break;
        case 6:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 0, 6)
            break;
        case 7:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 0, 7)
            break;
        case 8:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 0, 8)
            break;
        case 9:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 0, 9)
            break;
        case 10:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 0, 10)
            break;
        case 11:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 0, 11)
            break;
        case 12:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 0, 12)
            break;
        case 13:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 0, 13)
            break;
        case 14:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 0, 14)
            break;
        case 15:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 0, 15)
            break;
        default:
            break;
    }
    *((int*)&data64 + 1) = read_val;
    /*kputs("reading");
    send_long(read_val);
    send_long(data64);*/

    switch(interface) {
        case 0:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 1, 0)
            break;
        case 1:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 1, 1)
            break;
        case 2:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 1, 2)
            break;
        case 3:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 1, 3)
            break;
        case 4:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 1, 4)
            break;
        case 5:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 1, 5)
            break;
        case 6:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 1, 6)
            break;
        case 7:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 1, 7)
            break;
        case 8:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 1, 8)
            break;
        case 9:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 1, 9)
            break;
        case 10:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 1, 10)
            break;
        case 11:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 1, 11)
            break;
        case 12:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 1, 12)
            break;
        case 13:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 1, 13)
            break;
        case 14:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 1, 14)
            break;
        case 15:
            ROCC_INSTRUCTION_DSS(2, read_val, addr, 1, 15)
            break;
        default:
            break;
    }
    *((int*)&data64) = read_val;
    /*send_long(read_val);
    send_long(data64);*/
    return data64;
}

static inline long start_exec(int backwards) {
    // |   -   |  -  | -   | 1  | 0   |  0   |ack_reg| CUSTOM_3 |
    long resp_val;
    switch(backwards) {
        case 0:
            ROCC_INSTRUCTION_D(3, resp_val, 0)
            break;
        case 1:
            ROCC_INSTRUCTION_D(3, resp_val, 1)
            break;
        default:
            break;
    }
    return resp_val;
}

static inline void write_chunk(long* src, unsigned long src_addr, unsigned long dest_addr, unsigned long num_elems, int interface) {

    for(int i = 0; i < num_elems; i++) {

        write_to_accel_mem(src[src_addr+i], dest_addr+i, interface);
        /*kwrite("written: ", 10);
        send_long(src[src_addr+i]);
        kwrite("on interface: ", 14);
        send_int(interface);
        kwrite("dest addr: ", 11);
        send_long(dest_addr+i);
        kputs("");*/
    }
}

static inline void read_chunk(long* buf, unsigned long src_addr, unsigned long dest_addr, unsigned long num_elems, int interface, int check) {

    for(int i = 0; i < num_elems; i++) {

        long read = read_from_accel_mem(src_addr+i, interface);
        if(check && buf[dest_addr+i] != read) {

            kputs("!!!");
            kputs("ERROR: value read at:");
            kwrite("    Interface: ", 15);
            send_int(interface);
            kwrite("    Address: ", 13);
            send_long(src_addr + i);
            kwrite("Different from value in acc_data at address: ", 45);
            send_long(dest_addr + i);
            kputs("!!!");
        }
        buf[dest_addr+i] = read;
        /*kwrite("read: ", 7);
        send_long(buf[src_addr+i]);
        kwrite("on interface: ", 14);
        send_int(interface);
        kwrite("src addr: ", 10);
        send_long(src_addr+i);
        kwrite("dest addr: ", 11);
        send_long(dest_addr+i);
        kputs("");*/
    }
}

static inline void write_variables(long* src, unsigned long num_elems) {

    for(int i = 0; i < num_elems; i++) {

        write_to_accel_mem(src[i], i % NUM_VAR_BANKS * VAR_BANK_SIZE + i / NUM_VAR_BANKS, 0);
    }
}

static inline void read_variables(long* buf, unsigned long num_elems, int check) {

    unsigned long src_addr;
    for(int i = 0; i < num_elems; i++) {

        src_addr = i % NUM_VAR_BANKS * VAR_BANK_SIZE + i / NUM_VAR_BANKS;
        long read = read_from_accel_mem(src_addr, 0);
        if(check && buf[i] != read) {

            kputs("!!!");
            kputs("ERROR: value read at:");
            kputs("    Interface: 0");
            kwrite("    Address: ", 13);
            send_long(src_addr + i);
            kwrite("Different from value in var_data at address: ", 45);
            send_long(i);
            kputs("!!!");
        }
        buf[i] = read;
    }
}
