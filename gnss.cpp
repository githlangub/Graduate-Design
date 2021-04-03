#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <functional>
#include <cmath>
#include "nmea0183.hpp"
#include "gnss.hpp"
#include "utils.hpp"

using namespace std;

extern ofstream global_log;

GNSS::GNSS(string& uart_dev,function<void(GnssMsg&)> gnss_msg_callback):
	uart_port(uart_dev,bind(&GNSS::Recv,this,placeholders::_1)),
	gnss_msg_callback(gnss_msg_callback)
{
	//Set baud rate of UART.
	this->uart_port(UART::Baud::b38400);
	this->uart_port.Sync(UART::sync_now);

	//Status.
	global_log << "[STATUS][GNSS] " << "GNSS module opened." << endl;
}



GNSS::~GNSS()
{
	global_log << "[STATUS][GNSS] " << "GNSS module closed." << endl;
}



GnssMsg GNSS::operator*(void)
{
	unique_lock<mutex> lck(this->lock);

	return this->local_state;
}



void GNSS::Recv(vector<unsigned char>& recived)
{
	//Buffer of new data.
	static deque<unsigned char> recv_buff;
	recv_buff.insert(recv_buff.end(),recived.begin(),recived.end());

	//Take out an entire and valid GNSS message.
	auto msg_prefix = recv_buff.end(),checksum_prefix = recv_buff.end();
	unsigned char checksum = 0,std_checksum = 0;
	for(auto it = recv_buff.begin();it != recv_buff.end();it++)
	{
		if(*it == '$')
		{
			msg_prefix = it;
			checksum_prefix = recv_buff.end();
		}
		else if(*it == '*')
		{
			if(msg_prefix != recv_buff.end())
			{
				checksum_prefix = it;
			}
		}
		else if(*it == '\x0d')
		{
			if(msg_prefix != recv_buff.end() && checksum_prefix != recv_buff.end())
			{
				//Calculate the standard checksum.
				std_checksum = 0;
				int n = 0;
				for(auto jt = it - 1;jt != checksum_prefix;jt--,n++)
				{
					//Hexadecimal char to int.
					int num = 0;
					if(*jt >= '0' && *jt <= '9')
					{
						num = *jt - 0x30;
					}
					else if(*jt >= 'A' && *jt <= 'F')
					{
						num = *jt - 0x37;
					}
					else if(*jt >= 'a' && *jt <= 'f')
					{
						num = *jt - 0x57;
					}
					else
					{
						global_log << "[ERROR][GNSS] ";
						global_log << "Invalid character in standard checksum." << endl;
						throw ;
					}

					//calculate the sum.
					std_checksum += (num * pow(16,n));
				}

				//Recive a valid message and resolve it.
				if(checksum == std_checksum)
				{
					//Valid checksum.
					global_log << "[STATUS][GNSS] ";
					global_log << "Error checking succeed, ";
					global_log << "checksum = " << hex << (int)checksum << dec;
					global_log << ", std_checksum = " << hex << (int)std_checksum << dec << endl;

					//Resolve.
					try
					{
						//Flush local state.
						GnssMsg new_msg = this->gnss_msg_resolver(msg_prefix,checksum_prefix + 1);
						this->gnss_msg_callback(new_msg);

						//Status.
						global_log << "[STATUS][GNSS]" << "Recived new GNSS data." << endl;
						this->lock.lock();
						this->local_state = new_msg;
						this->local_state.Print();
						this->lock.unlock();
					}
					catch(int err)
					{
					}
				}
				else
				{
					//Invalid checksum.
					global_log << "[WARNING][GNSS] ";
					global_log << "Recive an invalid GNSS message, checksum incorrect, ";
					global_log << "checksum = " << hex << (int)checksum << dec;
					global_log << ", std_checksum = " << hex << (int)std_checksum << dec << endl;
				}

				//Finish to resolve a GNSS message, reset the varieties.
				msg_prefix = checksum_prefix = recv_buff.end();
				checksum = 0;
			}
		}
		else
		{
			if(msg_prefix != recv_buff.end() && checksum_prefix == recv_buff.end())
			{
				//Calculate the checksum.
				if(it == msg_prefix + 1)
				{
					checksum = *it;
				}
				else
				{
					checksum ^= *it;
				}
			}
		}
	}

	//Clear all GNSS message that has been resolved from the buffer.
	recv_buff.erase(recv_buff.begin(),msg_prefix);
}



/*void GPS::Listen(void)
{
	while(this->run_flag)
	{
		this_thread::sleep_for(chrono::seconds(3));

		unique_lock<mutex> lck(this->lock);

		this->current_position.longitude = random(100000);
		this->current_position.latitude = random(100000);

		global_log << "[STATUS][GPS] " << "GPS position flushed: ";
		global_log << this->current_position.longitude << " , ";
		global_log << this->current_position.latitude << " , " <<  endl;
	}
}*/

