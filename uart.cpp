//	uart.cpp
//	
//	The file to config.

#include <iostream>
	#include <fstream>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <cstring>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include "uart.hpp"
#include "global.hpp"
#include "utils.hpp"

using namespace std;

extern ofstream global_log;



UART::UART(string& uart_dev,function<void(vector<unsigned char>&)> recv_callback):
	uart_dev(uart_dev),
	recv_callback(recv_callback),
	run_flag(true),
	uart_daemon(&UART::Recv,this)
{
	//Open UART device.
	this->uart_fd = open((char*)this->uart_dev.c_str(),O_RDWR | O_NONBLOCK);
	if(this->uart_fd < 0)
	{
		global_log << "[ERROR][UART] " << "Unable to open " << '\"' << this->uart_dev << '\"' << endl;
		global_log << "[ERROR][UART] " << "Unable to open UART." << endl;
		throw ;
	}

	//Get attributions from UART.
	if(tcgetattr(this->uart_fd,&this->uart_opt) < 0)
	{
		global_log << "[ERROR][UART] " << "Unable to get attributions from UART." << endl;
		global_log << "[ERROR][UART] " << "Unable to open UART." << endl;
		throw ;
	}

	//UART configurations.
	this->uart_opt.c_iflag = 0;
	this->uart_opt.c_iflag &= ~(PARMRK | INPCK);
	this->uart_opt.c_iflag |= IGNPAR;

	this->uart_opt.c_oflag = 0;

	this->uart_opt.c_cflag = 0;
	this->uart_opt.c_cflag &= ~(CBAUD | CSIZE | PARODD | PARENB | CSTOPB);
	this->uart_opt.c_cflag |= (B9600 | CS8);

	this->uart_opt.c_lflag = 0;
	//this->uart_opt.c_lflag |= CRTSCTS;

	//this->uart_opt.c_cc[VMIN] = 0;
	//this->uart_opt.c_cc[VTIME] = 1;

	//Set attributions to UART.
	if(tcsetattr(this->uart_fd,TCSANOW,&this->uart_opt) < 0)
	{
		global_log << "[ERROR][UART] " << "Unable to set attributions to UART." << endl;
		global_log << "[ERROR][UART] " << "Unable to open UART." << endl;
		throw ;
	}

	//Flush the buffer of UART.
	if(tcflush(this->uart_fd,TCIOFLUSH) < 0)
	{
		global_log << "[ERROR][UART] " << "Unable to flush the buffer of UART!" << endl;
		global_log << "[ERROR][UART] " << "Unable to open UART." << endl;
		throw ;
	}

	//Save the status of UART.
	global_log << "[STATUS][UART] " << "UART.uart_dev = " << this->uart_dev << endl;
	global_log << "[STATUS][UART] " << "UART.uart_fd = " << this->uart_fd << endl;
	global_log << "[STATUS][UART] " << "UART was opened." << endl;
}



UART::~UART()
{
	//Stop the daemon thread when the process stop.
	this->run_flag = false;
	uart_daemon.join();

	//Close UART.
	if(close(this->uart_fd))
	{
		global_log << "[ERROR][UART] " << "Unable to close " << '\"' << this->uart_dev << '\"' << "." << endl;
		global_log << "[ERROR][UART] " << "Unable to close UART." << endl;
		throw ;
	}

	//Status.
	global_log << "[STATUS][UART] " << "UART was closed." << endl;
}



void UART::Send(vector<unsigned char>& to_send)
{
	//Lock before writing.
	this->uart_lock.lock();

	//Send bytes.
	int write_bytes = write(this->uart_fd,to_send.data(),to_send.size());
	if(write_bytes < 0)
	{
		global_log << "[ERROR][UART] " << "Unable to write data to UART." << endl;
		throw 37;
	}
	else
	{
		global_log << "[STATUS][UART] " << "Send " << to_send.size() << " bytes: " << endl;
		show_bytes(to_send.data(),to_send.size());
	}
	
	//Unlock after writing.
	this->uart_lock.unlock();
}



/*void UART::Send(unsigned char* data,unsigned size)
{
	//Lock before writing.
	this->uart_lock.lock();

	if(!this->open_flag)
	{
		global_log << "[ERROR][UART] " << "UART is not opened." << endl;
		global_log << "[ERROR][UART] " << "Unable to write data to UART." << endl;
		//throw ;
	}

	//Write data to UART.
	int write_bytes = write(this->uart_fd,data,size);
	if(write_bytes < 0)
	{
		global_log << "[ERROR][UART] " << "Unable to write data to UART." << endl;
		throw ;
	}
	else
	{
		global_log << "[STATUS][UART] " << "Write " << write_bytes << " bytes to UART: " << endl;
	}
	
	//Unlock after writing.
	this->uart_lock.unlock();
}*/



void UART::Recv(void)
{
	//
	this_thread::yield();
	
	//
	while(this->run_flag)
	{
		//Examin if uart is readable.
		int bytes = 0;
		ioctl(this->uart_fd,FIONREAD,&bytes);

		//Produce an interupt.
		if(bytes > 0)
		{
			//Read data.
			unsigned char recived[1024] = {0};
			int read_bytes = read(this->uart_fd,recived,sizeof(recived));
			if(read_bytes > 0)
			{
				global_log << "[STATUS][UART] " << "Read " << read_bytes << " bytes: " << endl;
				show_bytes(recived,read_bytes);
			}
			else
			{
				global_log << "[ERROR][UART] " << "Unable to read data from UART." << endl;
				throw 31;
			}

			//Call for the interupt handler to deal with the data recived.
			vector<unsigned char> temp(recived,recived + read_bytes);
			this->recv_callback(temp);
		}

		//This thread sleep for 100ms.
		this_thread::sleep_for(chrono::milliseconds(10));
	}
}



void UART::Flush(void)
{
	if(tcflush(uart_fd,TCIOFLUSH) < 0)
	{
		global_log << "[ERROR][UART] " << "Unable to flush the buffer of UART!" << endl;
		global_log << "[ERROR][UART] " << "Unable to open UART." << endl;
		//throw ;
	}
	else
	{
		global_log << "[STATUS][UART] " << "Flushed IO Buffer." << endl;
		return;
	}
}



void UART::Sync(UART::Sync_t action)
{
	if(tcsetattr(this->uart_fd,(int)action,&this->uart_opt) < 0)
	{
		if(tcgetattr(this->uart_fd,&this->uart_opt) < 0)
		{
			global_log << "[ERROR][UART] " << "Unable to synchronize configurations to UART." << endl;
			throw ;
		}
		else
		{
			global_log << "[WARNING] " << "Unable to synchronize the configurations to UART, ";
			global_log << "with original values unchanged." << endl;
		}
	}

	global_log << "[STATUS][UART] " << "UART configurations synchronized." << endl;
}



UART& UART::operator()(UART::Option_t option,int value)
{
	switch(option)
	{
		case wait_time:
		{	
			if(value < 0)
			{
				global_log << "[WARNING][UART] "; 
				global_log << "The value of \"wait_time\" for UART must be positive, ";
				global_log << "with the original value " << this->uart_opt.c_cc[VTIME] << " unchanged.";
				global_log << endl;
			}
			else
			{
				this->uart_opt.c_cc[VTIME] = value;
				global_log << "[STATUS][UART] " << "Set UART wait time into " << value * 0.1 << "s." << endl;
			}
			break;
		}
	}

	return *this;
}



UART& UART::operator()(UART::Baud baud)
{
	this->uart_opt.c_cflag &= ~CBAUD;
	this->uart_opt.c_cflag |= (unsigned)baud;
	global_log << "[STATUS][UART] " << "Set UART baud rate into ";
	global_log << '\"' << oct << (unsigned)baud << dec << '\"' << endl;

	return *this;
}

