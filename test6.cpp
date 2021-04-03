//	main.cpp
//
//	??

#include <iostream>
#include <thread>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include "lora.hpp"
#include "uart.hpp"
#include "gpio.hpp"
#include "frame.hpp"
#include "mac.hpp"

using namespace std;

bool run_flag = true;
bool int_flag = false;

int leds_fd;

LoRa* recv_entrance = NULL;

void buttons_poll(void)
{
	int buttons_fd = open("/dev/buttons",0);
	if(buttons_fd < 0)
	{
		cout << "buttons failed" << endl;
	}
	while(run_flag)
	{
		char buttons[9] = {0};
		if(read(buttons_fd,buttons,8) == 8)
		{
			int_flag = true;
			cout << buttons << endl;
		}
	}
	close(buttons_fd);
}

void recv_sig_handle(int signum)
{
	ioctl(leds_fd,1,3);
	recv_entrance->Recive();
	ioctl(leds_fd,0,3);
}

void buttons_sig_hadle(int signum)
{
}

void int_sig_handle(int signum)
{
	run_flag = false;
	ioctl(leds_fd,0,0);
	ioctl(leds_fd,0,1);
	ioctl(leds_fd,0,2);
	ioctl(leds_fd,0,3);
	close(leds_fd);
	exit(-1);
}

int main(int argc,char* argv[])
{
	//thread Tt(buttons_poll);
	signal(SIGINT,int_sig_handle);
	signal(SIGUSR1,recv_sig_handle);

	if(argc < 2)
	{
		cerr<<"UART device is needed!"<<endl;
		cerr<<"Usage: "<<endl;
		cerr<<"      ?????"<<endl;
		return -1;
	}

	leds_fd = open("/dev/leds",0);
	if(leds_fd < 0) cout << "Leds open failed." << endl;
	ioctl(leds_fd,1,0);

	UART U1(argv[1]);
	LoRa L1(U1);
	MAC M1(L1);

	L1.ChangeMode(LoRaTypes::Mode::sleep);
	LoRaConfig Config;
	Config.local_addr = 0x02;
	Config.parity = LoRaTypes::Parity::none;
	Config.baud = LoRaTypes::Baud::b115200;
	Config.air_speed = LoRaTypes::AirSpeed::as19200;
	Config.local_chan = LoRaTypes::Channel::ch23_433mhz;
	Config.transmit = LoRaTypes::Transmit::fixed;
	Config.io_drive = LoRaTypes::IODrive::drive2;
	Config.wake_time = LoRaTypes::WakeTime::wt250;
	Config.fec = LoRaTypes::FEC::off;
	Config.send_power = LoRaTypes::SendPower::sp30;
	L1.SetConfig(Config,LoRaTypes::SetConfigTo::flash);

	L1.ChangeMode(LoRaTypes::Mode::normal);

	vector<unsigned char> temp;
	while(run_flag)
	{
		if(argv[2][0] == 'w')
		{
			string str;
			getline(cin, str);
			temp.assign(str.begin(),str.end());
			temp.push_back('\x0D');
			temp.push_back('\x0A');
			//L1.Send(temp);
			M1.Send(temp);
		}
		else if(argv[2][0] == 'c')
		{
			unsigned air_speed;
			cout << "Get air_speed: ";
			cin >> air_speed;
			unsigned send_power;
			cout << "Get send_power: ";
			cin >> send_power;
			unsigned bytes;
			cout << "Get bytes: ";
			cin >> bytes;

			LoRaConfig LC1;
			LC1.air_speed = (LoRaTypes::AirSpeed)air_speed;
			LC1.send_power = (LoRaTypes::SendPower)send_power;
			M1.SpeedTestClient(bytes,LC1);
		}
		else if(argv[2][0] == 's')
		{	
			L1.Flush();
			M1.SpeedTestServer();
		}
		else if(argv[2][0] == 't')
		{
			for(int i = 0;i < 10;i++)
			{
				temp.assign(10,(unsigned char)(i + 16 * i));
				M1.Send(temp);
			}
		}
	}

	ioctl(leds_fd,0,0);
	ioctl(leds_fd,0,1);
	ioctl(leds_fd,0,2);
	ioctl(leds_fd,0,3);
	close(leds_fd);
	return 0;
}

