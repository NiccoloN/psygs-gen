#ifndef HELPER_UART_H
#define HELPER_UART_H

#include "../include/platform.h"
#include "../include/common.h"
#include "../include/serial.h"

static inline int itoa(int value, char *sp, int radix)
{
    char tmp[16]; // be careful with the length of the buffer
    char *tp = tmp;
    int i;
    unsigned v;

    int sign = (radix == 10 && value < 0);
    if (sign)
        v = -value;
    else
        v = (unsigned)value;

    while (v || tp == tmp)
    {
        i = v % radix;
        v /= radix;
        if (i < 10)
            *tp++ = i + '0';
        else
            *tp++ = i + 'a' - 10;
    }

    int len = tp - tmp;

    if (sign)
    {
        *sp++ = '-';
        len++;
    }

    while (tp > tmp)
        *sp++ = *--tp;

    return len;
}

static inline int atoi(char* str) {

    int res = 0;

    for(int i=0; str[i]!='\0'; i++)
        res = res * 10 + str[i] - '0';

    return res;
}

char *
__utoa(unsigned value,
       char *str,
       int base)
{
    const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    int i, j;
    unsigned remainder;
    char c;

    /* Check base is supported. */
    if ((base < 2) || (base > 36))
    {
        str[0] = '\0';
        return 0;
    }

    /* Convert to string. Digits are in reverse order.  */
    i = 0;
    do
    {
        remainder = value % base;
        str[i++] = digits[remainder];
        value = value / base;
    } while (value != 0);
    str[i] = '\0';

    /* Reverse string.  */
    for (j = 0, i--; j < i; j++, i--)
    {
        c = str[j];
        str[j] = str[i];
        str[i] = c;
    }

    return str;
}

char *
utoa(unsigned value,
     char *str,
     int base)
{
    return __utoa(value, str, base);
}

void reverse(char str[], int length)
{
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        end--;
        start++;
    }
}

int ltoa(long num, char* str, int base)
{
    int i = 0;
    int isNegative = 0;

    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return i;
    }

    if (num < 0 && base == 10) {
        isNegative = 1;
        num = -num;
    }

    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    if (isNegative)
        str[i++] = '-';

    str[i] = '\0';

    reverse(str, i);
    return i;
}

static inline void send_int(int v)
{
    char int_str[64];
    int len = itoa(v, int_str, 10);
    kwrite(int_str, len);
    kwrite("\n", 1);
}

static inline void put_int(int v)
{
    char int_str[64];
    int len = itoa(v, int_str, 10);
    kwrite(int_str, len);
    kwrite("\n", 1);
}

static inline void send_uint(unsigned v)
{
    char int_str[64];
    utoa(v, int_str, 10);
    int len = 0;
    while (int_str[len] != '\0')
        len++;
    kwrite(int_str, len);
    kwrite("\n", 1);
}

static inline void send_long(long v)
{
    char int_str[64];
    int len = ltoa(v, int_str, 10);
    kwrite(int_str, len);
    kwrite("\n", 1);
}

uint64_t get_mcycle()
{
    uint64_t mcycle = 0;

    asm volatile("csrr %0,mcycle"
                 : "=r"(mcycle));

    return mcycle;
}

static inline int buffer_cmp(int *buff_1, int *buff_2, int n, int start_1, int start_2)
{
    for (int i = 0; i < n; i++)
    {
        kwrite("b1 was ", 8);
        send_int(buff_1[start_1 + i]);
        kwrite("b2 was ", 8);
        send_int( buff_2[start_2 + i]);
        if (buff_1[start_1 + i] != buff_2[start_2 + i])
        {
            return 0;
        }
    }
    return 1;
}

static inline void send_buf_as_int(int* buff, int n){
    for (int i = 0; i < n; i++)
    {
        //kwrite(": ", 3);
        send_int(buff[i]);
    }
    return;
}

#endif //HELPER_UART_H