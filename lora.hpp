#ifndef __LORA_HPP__
#define __LORA_HPP__

#include <deque>
#include <vector>
#include <condition_variable>
#include <functional>
#include "uart.hpp"
#include "gpio.hpp"
//#include "frame.hpp"

#define LORA_PACKET_SIZE 224
#define MAX_PACKET_SIZE 224

using namespace std;

struct LoRaTypes
{
	public:
		enum class Options:unsigned char
		{
			//flush_before_read = 0x1,
			set_read_time = 1,

			none = 0x0
		};

		typedef unsigned short Address;

		enum class Mode:unsigned char
		{
			normal = 0,
			low_power = 1,
			wake_up = 2,
			sleep = 3
		};

		enum class Parity:unsigned char
		{
			none = 0x0,
			odd = 0x40,
			even = 0x80,

			unchanged = 0xff
		};

		enum class Baud:unsigned char
		{
			b1200 = 0x0,
			b2400 = 0x08,
			b4800 = 0x10,
			b9600 = 0x18,
			b19200 = 0x20,
			b38400 = 0x28,
			b57600 = 0x30,
			b115200 = 0x38,

			unchanged = 0xff
		};

		enum class AirSpeed:unsigned char
		{
			as300 = 0x0,
			as1200 = 0x01,
			as2400 = 0x02,
			as4800 = 0x03,
			as9600 = 0x04,
			as19200 = 0x05,

			unchanged = 0xff
		};

		enum class Channel:unsigned char
		{
			ch0_410mhz = 0,
			ch1_411mhz = 0x1,
			ch2_412mhz    ,
			ch3_413mhz    ,
			ch4_414mhz    ,
			ch5_415mhz    ,
			ch6_416mhz    ,
			ch7_417mhz    ,
			ch8_418mhz    ,
			ch9_419mhz    ,
			ch10_420mhz    ,
			ch11_421mhz    ,
			ch12_422mhz    ,
			ch13_423mhz    ,
			ch14_424mhz    ,
			ch15_425mhz    ,
			ch16_426mhz    ,
			ch17_427mhz    ,
			ch18_428mhz    ,
			ch19_429mhz    ,
			ch20_430mhz    ,
			ch21_431mhz    ,
			ch22_432mhz    ,
			ch23_433mhz    ,
			ch24_434mhz    ,
			ch25_435mhz    ,
			ch26_436mhz    ,
			ch27_437mhz    ,
			ch28_438mhz    ,
			ch29_439mhz    ,
			ch30_440mhz    ,
			ch31_441mhz    ,

			unchanged = 0xff
		};

		enum class Transmit:unsigned char
		{
			transparent = 0x0,
			fixed = 0x80,

			unchanged = 0xff
		};

		enum class IODrive:unsigned char
		{
			drive1 = 0x0,
			drive2 = 0x40,

			unchanged = 0xff
		};

		enum class WakeTime:unsigned char
		{
			wt250 = 0x0,
			wt500 = 0x08,
			wt750 = 0x10,
			wt1000 = 0x18,
			wt1250 = 0x20,
			wt1500 = 0x28,
			wt1750 = 0x30,
			wt2000 = 0x38,

			unchanged = 0xff
		};

		enum class FEC:unsigned char
		{
			on = 0x04,
			off = 0x0,

			unchanged = 0xff
		};

		enum class SendPower:unsigned char
		{
			sp30 = 0x0,
			sp27 = 0x01,
			sp24 = 0x02,
			sp21 = 0x03,

			unchanged = 0xff
		};

		enum class SetConfigTo:unsigned char
		{
			ram = 0,
			flash = 1
		};
};

struct LoRaConfig
{
	public:
		LoRaTypes::Address	local_addr;
		LoRaTypes::Parity 	parity;
		LoRaTypes::Baud 	baud;
		LoRaTypes::AirSpeed 	air_speed;
		LoRaTypes::Channel 	local_chan;
		LoRaTypes::Transmit 	transmit;
		LoRaTypes::IODrive 	io_drive;
		LoRaTypes::WakeTime 	wake_time;
		LoRaTypes::FEC 		fec;
		LoRaTypes::SendPower 	send_power;

	public:
		LoRaConfig(void);
		/*LoRaConfig(	LoRaTypes::Address = 0x00,
		 		LoRaTypes::Parity = LoRaTypes::Parity::unchanged,
				LoRaTypes::Baud = LoRaTypes::Baud::unchanged,
				LoRaTypes::AirSpeed = LoRaTypes::AirSpeed::unchanged,
				LoRaTypes::Channel = LoRaTypes::Channel::unchanged,
				LoRaTypes::Transmit = LoRaTypes::Transmit::unchanged,
				LoRaTypes::IODrive = LoRaTypes::IODrive::unchanged,
				LoRaTypes::WakeTime = LoRaTypes::WakeTime::unchanged,
				LoRaTypes::FEC = LoRaTypes::FEC::unchanged,
				LoRaTypes::SendPower = LoRaTypes::SendPower::unchanged);*/

		void Print(void);
		void operator=(LoRaConfig& );
};



class LoRa
{
	public:
		LoRa(function<void(vector<unsigned char>&)> );
		~LoRa();

		void Send(	vector<unsigned char>& 					,
				LoRaTypes::Address = 0xffff				,
				LoRaTypes::Channel = LoRaTypes::Channel::ch23_433mhz	);
		void Recv(vector<unsigned char>& );

		void ChangeMode(LoRaTypes::Mode );
		void GetConfig(LoRaConfig& );
		void SyncConfig(void);
		void SetConfig(LoRaConfig& ,LoRaTypes::SetConfigTo = LoRaTypes::SetConfigTo::ram);
		void Reset(void);

		void Flush(void);
	
	private:
		LoRaTypes::Mode 	mode;

		LoRaConfig		Config;

		deque<unsigned char> 	IBuff;
		condition_variable 	in_buffer_ring;
		mutex 			in_buffer_lock;
		//deque<unsigned char> OBuff;

		UART con2_port;

		GPIO md0_port;
		GPIO md1_port;
		GPIO aux_port;

		//The entrance of the data-link layer.
		function<void(vector<unsigned char>&)> employer;
};

#endif

