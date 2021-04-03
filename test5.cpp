#include<iostream>
#include<cstdlib>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include <sys/ioctl.h>
#include<termios.h>
#include<unistd.h>
#include<cstring>
#include <sys/epoll.h>
#include <signal.h>
#include"debug.c"
//#include "uart.hpp"

using namespace std;

int fpid = 0;

int lock = 0;

//UART U1;

/*void sig_pa(int signum)
{
	if(signum == SIGINT)
	{
		cout << "ByeBye! " << getpid() << endl;
		kill(fpid,SIGINT);
		exit(0);
	}
}

void sig_ch(int signum)
{
	if(signum == SIGINT)
	{
		cout << "ByeBye! " << getpid() << endl;
		exit(0);
	}
	else if(signum == SIGALRM)
	{
		unsigned char buff[1024] = {0};
		int read_bytes = U1.Read(buff,1024);
		if(read_bytes > 0)
		cout<<buff<<endl;
	}
}*/

int main(int argc,char* argv[])
{
	int fd_sp = open(argv[1],O_RDWR | O_NONBLOCK);
	if(fd_sp == -1)
	{
		cerr<<"Can't Open Serial port!"<<endl;
		return -1;
	}

	termios Opt;
	tcgetattr(fd_sp,&Opt);
	/*Opt.c_iflag = Opt.c_oflag = Opt.c_cflag = Opt.c_lflag = 0;
	Opt.c_iflag &= ~(PARMRK | INPCK);
	Opt.c_iflag |= IGNPAR;
	Opt.c_cflag &= ~(CBAUD | CSIZE | PARODD | PARENB | CSTOPB);
	Opt.c_cflag |= (B115200 | CS8);*/
	cfmakeraw(&Opt);
	if(cfsetspeed(&Opt,B115200) == -1)
	{
		cerr<<"Can't set speed!"<<endl;
	}
	Opt.c_cc[VMIN] = 0;
	Opt.c_cc[VTIME] = 10;
	tcsetattr(fd_sp,TCSANOW,&Opt);

	tcflush(fd_sp,TCIFLUSH);

	/*int fd_ep = epoll_create(1);
	if(fd_ep == -1)
	{
		cerr << "Epoll create failed!" << endl;
	}
	struct epoll_event event_sp = {EPOLLIN,0};
	epoll_ctl(fd_ep,EPOLL_CTL_ADD,fd_sp,&event_sp);
	close(fd_sp);
	close(fd_ep);*/
	
	if(argv[2][0] == 'r')
	{
		cout << "read" << endl;
		do{
			int b = 0;
			ioctl(fd_sp,FIONREAD,&b);
			if(b) cout << b <<endl;
			char buff[1024] = {0};
			int readByte = read(fd_sp,buff,sizeof(buff));
			if(readByte != -1)
			{
				cout<<readByte<<" Bytes read: "<<buff<<endl;
				usleep(10000);
			}
		}while(1);
	}
	else if(argv[2][0] == 'w')
	{
		cout << "write" << endl;
		do{
			char buff[1024] = {0};
			cin>>buff;
			int writeByte = write(fd_sp,buff,strlen(buff));
			cout<<writeByte<<" Bytes written: "<<buff<<endl;
		}while(1);
	}
	/*else if(argv[2][0] == 'e')
	{
		while(1)
		{
			if(epoll_wait(fd_ep,&event_sp,1,10000) != 0)
			{
				char buff[1024] = {0};
				int read_bytes = read(fd_sp,buff,1024);
				cout<<read_bytes<<" Bytes read: "<<buff<<endl;
			}
		}
	}
	else if(argv[2][0] == 'i')
	{
		U1.Open(argv[1]);
		U1(UART::Baud::b115200);
		U1.Sync(UART::sync_now);
		fpid = fork();
		if(fpid > 0)
		{
			signal(SIGINT,sig_pa);
			while(1)
			{
				if(U1.Listen(10) == 0)
				{
					kill(fpid,SIGALRM);
				}
			}
		}
		else if(fpid == 0)
		{
			signal(SIGINT,sig_ch);
			signal(SIGALRM,sig_ch);
			while(1)
			{
				unsigned char buff[1024] = {0};
				cin >> buff;
				buff[strlen((const char*)buff)] = '\x0d';
				U1.Write(buff,strlen((const char*)buff));
			}
			cout << "I'm children." << endl;
		}
		else
		{
			cerr << "Fuck!" << endl;
		}
	}
	else
	{
		cerr << "Fuck!" << endl;
	}*/

	if(close(fd_sp)  == -1)
	{
		cerr<<"Can't close serial port!"<<endl;
	}

	return 0;
}

