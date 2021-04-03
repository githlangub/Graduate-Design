//Test if read() and write() can be called at the same time or not.

#include <iostream>
#include <thread>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

void uart_read(int uart_fd)
{
	//
	cout << "Ready to read." << endl;

	char buff[1024] = {0};
	int read_bytes = read(uart_fd,buff,sizeof(buff));

	cout << "Read " << read_bytes << " bytes: " << buff << endl;
}



int main(int argc,char* argv[])
{
	//
	int uart_fd = open(argv[1],O_RDWR);
	if(uart_fd < 0)
	{
		cerr << "UART " << argv[1] << "open failed." << endl;
		return -1;
	}

	termios uart_opt;
	tcgetattr(uart_fd,&uart_opt);
	cfmakeraw(&uart_opt);
	cfsetspeed(&uart_opt,B115200);
	uart_opt.c_cc[VMIN] = 0;
	uart_opt.c_cc[VTIME] = 100;
	tcsetattr(uart_fd,TCSANOW,&uart_opt);
	tcflush(uart_fd,TCIOFLUSH);

	//
	thread t1(uart_read,uart_fd);
	sleep(5);

	//
	cout << "Ready to write." << endl;

	char buff[1024] = {"Test1: uart read & write at the same time."};
	int write_bytes = write(uart_fd,buff,sizeof(buff));

	cout << "Write " << write_bytes << " bytes: " << buff << endl;

	//
	t1.join();
	
	close(uart_fd);

	return 0;
}

