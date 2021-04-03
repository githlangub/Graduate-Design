#ifndef __GNSS_HPP__
#define __GNSS_HPP__

#include <vector>
#include <mutex>
#include <functional>
#include "nmea0183.hpp"
#include "uart.hpp"

using namespace std;

class GNSS
{
	public:
		GNSS(string& ,function<void(GnssMsg&)> );
		~GNSS();

		GnssMsg operator*(void);

	private:
		void Recv(vector<unsigned char>& );

	private:
		GnssMsg local_state;
		mutex lock;

		UART uart_port;

		NMEA0183 gnss_msg_resolver;

		function<void(GnssMsg&)> gnss_msg_callback;
};

#endif

