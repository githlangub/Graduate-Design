#ifndef __UART_HPP__
#define __UART_HPP__

#include <termios.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>

using namespace std;

class UART
{
	public:
		enum Option_t
		{
			baud,
			wait_time,
			none
		};

		enum class Baud:int
		{
			/*b0 = 0000000,
			b1200 = 0000011,
			b2400 = 0000013,
			b4800 = 0000014,
			b9600 = 0000015,
			b19200 = 0000016,
			b38400 = 0000017,
			b57600 = 0010001,
			b115200 = 0010002*/
			b0 = B0,
			b1200 = B1200,
			b2400 = B2400,
			b4800 = B4800,
			b9600 = B9600,
			b19200 = B19200,
			b38400 = B38400,
			b57600 = B57600,
			b115200 = B115200
		};

		enum Sync_t
		{
			sync_now = 0,
			sync_after_output = 1,
			sync_flush_input = 2
		};

	public:
		UART(string& ,function<void(vector<unsigned char>&)> );
		~UART();

		void	Open(char* );
		void	Close(void);
		void	Send(vector<unsigned char>& );
		//void	Send(unsigned char* ,unsigned );
		void	Recv(void);
		void	Flush(void);
		void	Sync(Sync_t );

		UART& operator()(Option_t ,int );

		UART& operator()(UART::Baud );
	
	private:
		termios 	uart_opt;
		string 		uart_dev;
		int 		uart_fd;

		thread 		uart_daemon;
		atomic<bool> 	run_flag;
		mutex 		uart_lock;

		function<void(vector<unsigned char>&)> recv_callback;
};

#endif

