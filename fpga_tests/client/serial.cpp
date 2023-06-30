#include <cstdio>
#include <unistd.h>
#include <fcntl.h> //文件控制定义
#include <termios.h>//终端控制定义
#include <stdint.h>
#include <tgmath.h>
#include <iostream>
#include <iomanip>

#include "../preprocessing/include/DoubleVector.hpp"
#include "../preprocessing/include/double_hpcg_matrix.hpp"

#define DEVICE "/dev/ttyUSB0"
#define S_TIMEOUT 1

int serial_fd = 0;

//打开串口并初始化设置
int init_serial(char *device)
{
    serial_fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (serial_fd < 0) {
        perror("open");
        return -1;
    }

    //串口主要设置结构体termios <termios.h>
    struct termios options;

    /**1. tcgetattr函数用于获取与终端相关的参数。
    *参数fd为终端的文件描述符，返回的结果保存在termios结构体中
    */

    tcgetattr(serial_fd, &options);
    /**2. 修改所获得的参数*/
    options.c_cflag |= (CLOCAL | CREAD);   //设置控制模式状态，本地连接，接收使能
    options.c_cflag &= ~CSIZE;             //字符长度，设置数据位之前一定要屏掉这个位
    options.c_cflag &= ~CRTSCTS;           //无硬件流控
    options.c_cflag |= CS8;                //8位数据长度
    options.c_cflag &= ~CSTOPB;            //1位停止位
    options.c_iflag |= IGNPAR;             //无奇偶检验位
    options.c_oflag = 0;                   //输出模式
    options.c_lflag = 0;                   //不激活终端模式
    cfsetospeed(&options, B115200);        //设置波特率

    /**3. 设置新属性，TCSANOW：所有改变立即生效*/
    tcflush(serial_fd, TCIFLUSH);          //溢出数据可以接收，但不读
    tcsetattr(serial_fd, TCSANOW, &options);

    return 0;
}

/**
*串口发送数据
*@fd:串口描述符
*@data:待发送数据
*@datalen:数据长度
*/
unsigned int total_send = 0 ;
int uart_send(int fd, uint8_t *data, int datalen)
{
    int len = 0;
    len = write(fd, data, datalen);//实际写入的长度
    if(len == datalen) {
        total_send += len;
        return len;
    } else {
        tcflush(fd, TCOFLUSH);//TCOFLUSH刷新写入的数据但不传送
        return -1;
    }
    return 0;
}

/**
*串口接收数据
*要求启动后，在pc端发送ascii文件
*/
unsigned int total_length = 0 ;
int uart_recv(int fd, uint8_t *data, int datalen)
{
    int len=0, ret = 0;
    fd_set fs_read;
    struct timeval tv_timeout;

    FD_ZERO(&fs_read);
    FD_SET(fd, &fs_read);

#ifdef S_TIMEOUT
    tv_timeout.tv_sec = (10*20/115200+2);
    tv_timeout.tv_usec = 0;
    ret = select(fd+1, &fs_read, NULL, NULL, NULL);
#elif
    ret = select(fd+1, &fs_read, NULL, NULL, tv_timeout);
#endif

    //如果返回0，代表在描述符状态改变前已超过timeout时间,错误返回-1

    if (FD_ISSET(fd, &fs_read)) {
        len = read(fd, data, datalen);
        total_length += len ;
        return len;
    } else {
        perror("select");
        return -1;
    }

    return 0;
}

void readline(char *p)
{
    char *q = p;
    do {
        uart_recv(serial_fd, (uint8_t *)q, sizeof(uint8_t));
    } while (*q++ != '\n');
    *q = '\0';
}

double precision = 0;
double old_remainder = -1;
int old_expected_iter = -1;
int checkFreq = 1000;
int maxIter = 10000000;

//usage:
//old_remainder: remainder the last time it was checked, if this is the first iteration set it to -1
//remainder: current remainder
//target: desired remainder
//checkFreq: minimum number of iterations before checking again (usually set to 100 or 1000)
//old_expected_iter: previous result of this function call, not checked if old_remainder is -1
int estimateIterations(double old_remainder, double remainder, double target, int checkFreq, int old_expected_iter) {
    if(old_remainder == -1 || old_remainder <= remainder)
        return checkFreq;

    int result = log(remainder/target)/log(old_remainder/remainder)*old_expected_iter;

    if(result < 0 || result > maxIter)
        return maxIter;

    if(result == 0)
        return checkFreq;

    return result;
}

int submitExecutions(HPCGMatrix A, DoubleVector x, DoubleVector b) {

    DoubleVector mulRes, diffRes;
    InitializeDoubleVector(mulRes, A.numberOfColumns);
    InitializeDoubleVector(diffRes, A.numberOfColumns);

    double remainder;
    double target;

    target = precision * Abs(b);
    SpMV(A, x, mulRes);
    Diff(mulRes, b, diffRes);
    remainder = Abs(diffRes);

    auto oldPrecision = std::cout.precision();
    std::cout << std::setprecision(53) << std::scientific;
    std::cout << "\nRemainder: " << remainder << "\n";
    std::cout << "Target:    " << target << "\n";
    std::cout << std::setprecision(oldPrecision) << std::defaultfloat;

    /*for(int i = 0; i < mulRes.length; i++) {

        double max = std::max<double>(b.values[i], mulRes.values[i]) == 0 ? 1 : std::max<double>(b.values[i], mulRes.values[i]);
        double err = diffRes.values[i] / max;
        std::cout << "Relative error: " << err << (std::max<double>(b.values[i], mulRes.values[i]) == 0 ? " abs" : "") << "\n";
    }*/

    if (remainder < target || remainder == old_remainder) {

        uart_send(serial_fd, ((uint8_t*) ("q")), 12);
        printf("\nComputation ended\n");
        return 0;
    }
    else {

        old_expected_iter = estimateIterations(old_remainder, remainder, target, checkFreq, old_expected_iter);
        old_remainder = remainder;

        uart_send(serial_fd, ((uint8_t*) (std::string("e") + std::to_string(old_expected_iter)).c_str()), 12);
        std::cout << "\nNew execution command sent: " << old_expected_iter << " iterations";
        return 1;
    }
}

void readResult(DoubleVector x, char* buff) {

    int i = 0;
    long tmp;
    char* c;
    while(1) {

        c = buff;
        do {

            uart_recv(serial_fd, (uint8_t *) c, sizeof(uint8_t));
            //printf("%c", *c);
        } while(*c++ != '\n');
        *c = '\0';

        if(!strncmp("END_result", buff, 10))
            break;

        tmp = atol(buff);
        //printf("%ld\n", tmp);
        //printf("%.100e\n", *((double*) &tmp));
        x.values[i] = *((double*) &tmp);
        i++;

        memset(buff, '\0', 256);
    }

    memset(buff, '\0', 256);
}

#define DPRINT 0
char msg[256];

int main(int argc, char *argv[]) {

    std::string junk;
    int template_freq = 0;

    std::ifstream infile;
    infile.open("../sygs_template/configurations.txt");
    if(!infile.is_open()) {

        std::cout << "Failed to open configurations file";
        exit(1);
    }
    //skip first 6 words
    infile >> junk >> junk >> junk >> junk >> junk >> junk;
    infile >> template_freq;
    infile >> precision;
    infile.close();

    if (init_serial(argv[1]) != 0) {

        printf("Failed to open serial\n");
        exit(-1);
    }
    printf("Serial opened successfully\n");

    char buff[256];
    char cmd[6];
    memcpy(cmd, argv[2], 5);
    cmd[5] = '\0';

    std::string matrix = argv[3];
    std::string matrix_path = "../../resources/matrices/" + matrix + ".mtx";
    std::string vector_path = "../../resources/vectors/" + matrix + ".vec";
    std::string coloring = argv[4];

    HPCGMatrix A;
    InitializeHPCGMatrix(A, matrix_path.c_str());

    DoubleVector b;
    InitializeDoubleVectorFromPath(b, vector_path.c_str(), A.numberOfColumns);

    DoubleVector x;
    InitializeDoubleVector(x, A.numberOfColumns);
    
    uart_send(serial_fd, ((uint8_t*) (cmd)), 6);

    printf("Command sent\n");

    int started = 0;
    while(!started) {

    	readline(buff);
        if(buff[0] == 'S') {

            started = 1;
            printf("Host started\n");
            printf("%s\n", buff);
        }
    }

    long long totIterations = 0;
    long long totCycles = 0;
    int redText = 0;

    printf("Start reading UART\n");
    char* c;
    while(1) {

        c = buff;
        do {

    	    uart_recv(serial_fd, (uint8_t *) c, sizeof(uint8_t));
	        //printf("%c", *c);
        } while(*c++ != '\n');
        *c = '\0';

        if(!strncmp("Iterations: ", buff, 12)) {

            totIterations += atoll(buff + 12);
            c--;
            *c = ' ';
            std::cout << "Iterations:               " << buff + 12 << "\n";
        }
        else if(!strncmp("Cycles: ", buff, 8)) {

            totCycles += atoll(buff + 8);
            c--;
            *c = ' ';
            std::cout << "Cycles:                   " << buff + 8 << "\n";
            std::cout << "Cumulative iterations:    " << totIterations << "\n";
            std::cout << "Cumulative cycles:        " << totCycles << "\n\n";
        }
        else if(!strncmp("CHECK_result", buff, 12)) {

            std::cout << "\u001b[33mChecking result:\u001b[0m\n";
            readResult(x, buff);
            if (!submitExecutions(A, x, b))
                break;
        }
        else if(!strncmp("!!!", buff, 3)) {

            if(redText) std::cout << buff << "\u001b[0m";
            else std::cout << "\u001b[31m" << buff;
            redText = !redText;
        }
        else std::cout << buff;

	    memset(buff, '\0', 256);
    }
	
    close(serial_fd);

    std::cout << "\nMatrix: " << matrix << "\n";
    std::cout << "Coloring: " << coloring << "\n";
    if(!strncmp("block_coloring", coloring.c_str(), 14)) {

        int matrix_dim = 0;

        infile.open(matrix_path);
        if(!infile.is_open()) {

            std::cout << "Failed to open matrix file\n";
            exit(1);
        }
        //skip until the nuber of rows
        std::string line;
        do {
            getline(infile, line);
        } while(line.at(0) == '%');
        matrix_dim = std::stoi(line.substr(0, line.find(' ')));
        infile.close();

        int block_size = std::min(128, matrix_dim/64);
        if(block_size == 0) block_size = 2;

        std::cout << "Blocks size: " << block_size << "\n";
    }
    std::cout << "\n";

    std::cout << "Template frequency:       " << template_freq / 1000000.0 << " MHz\n";
    std::cout << "Precision:                " << precision << "\n\n";

    std::cout << "Total iterations:         " << totIterations << "\n";
    std::cout << "Total cycles:             " << totCycles << "\n";
    std::cout << "Total execution time:     " << (double) totCycles / template_freq << " s\n\n";

    return 0;
}
