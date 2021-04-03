#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <deque>
#include <vector>
#include <chrono>
#include <functional>
#include <iterator>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <cstring>
#include <cmath>
#include <fcntl.h>
#include <unistd.h>
#include "lora.hpp"
#include "global.hpp"
#include "uart.hpp"
#include "gpio.hpp"
#include "utils.hpp"

using namespace std;

extern Config global_config;

extern ofstream global_log;

const char set_to_flash_ins[] = "\xC0";
const char set_to_ram_ins[] = "\xC2";
const char set_encryption_ins[] = "\xC6";
const char get_configurations_ins[] = "\xC1\xC1\xC1";
const char reset_ins[] = "\xC4\xC4\xC4";
const char ok_res[] = "OK";
const char error_res[] = "ERROR";


LoRa::LoRa(function<void(vector<unsigned char>&)> recv_callback):
	con2_port(global_config.uart_lora_dev,bind(&LoRa::Recv,this,placeholders::_1)),
	employer(recv_callback)
{
	try
	{
		//Open UART && GPIO port.
		this->md0_port.Open(GPIOH2_0);
		this->md1_port.Open(GPIOH2_1);
		this->aux_port.Open(GPIOH3_2);

		//Set default configurations to the device.
		LoRaConfig to_set;
		to_set.local_addr = global_config.lora_addr;
		to_set.parity = LoRaTypes::Parity::none;
		to_set.baud = LoRaTypes::Baud::b115200;
		to_set.air_speed = LoRaTypes::AirSpeed::as19200;
		to_set.local_chan = LoRaTypes::Channel::ch23_433mhz;
		to_set.transmit = LoRaTypes::Transmit::fixed;
		to_set.io_drive = LoRaTypes::IODrive::drive2;
		to_set.wake_time = LoRaTypes::WakeTime::wt250;
		to_set.fec = LoRaTypes::FEC::off;
		to_set.send_power = LoRaTypes::SendPower::sp30;

		this->ChangeMode(LoRaTypes::Mode::sleep);
		this->Reset();
		this->SetConfig(to_set,LoRaTypes::SetConfigTo::ram);
		this->SyncConfig();
		this->ChangeMode(LoRaTypes::Mode::normal);

		//Device send test.
		string str("LoRa onlined.\r\n");
		vector<unsigned char> temp(str.begin(),str.end());
		this->Send(temp,0xffff,LoRaTypes::Channel::ch23_433mhz);
	}
	catch(...)
	{
		global_log << "[ERROR][LORA] " << "LoRa initialization failed." << endl;
		throw ;
	}
}



LoRa::~LoRa()
{
	try
	{
		//
		this->ChangeMode(LoRaTypes::Mode::normal);
		string str("LoRa offlined.\r\n");
		vector<unsigned char> temp(str.begin(),str.end());
		this->Send(temp,0xffff,LoRaTypes::Channel::ch23_433mhz);

		//Close UART && GPIO port.
		this->md0_port.Close();
		this->md1_port.Close();
		this->aux_port.Close();
	}
	catch(...)
	{
		global_log << "[ERROR][LORA] " << "LoRa is not closed." << endl;
		throw ;
	}
}



void LoRa::Send(vector<unsigned char>& to_send,LoRaTypes::Address remote_addr,LoRaTypes::Channel remote_chan)
{
	for(unsigned packet_count = 0;packet_count * LORA_PACKET_SIZE < to_send.size();packet_count++)
	{
		//packet container.
		vector<unsigned char> temp;

		//Check if the device is busy.
		this->aux_port.Listen(GPIO::high,2 * LORA_PACKET_SIZE / 37.5);

		//Add a prefix before data if it's in fixed-transmit mode.
		if(this->Config.transmit == LoRaTypes::Transmit::fixed && this->mode != LoRaTypes::Mode::sleep)
		{
			temp.push_back((unsigned char)(remote_addr >> 8));
			temp.push_back((unsigned char)(remote_addr & 0xFF));
			temp.push_back((unsigned char)remote_chan);
		}

		//Send bytes.
		if(to_send.size() - LORA_PACKET_SIZE * packet_count > LORA_PACKET_SIZE)
		{
			temp.insert(	temp.end()							,	
					to_send.begin()	+ LORA_PACKET_SIZE * packet_count		,
					to_send.begin() + (LORA_PACKET_SIZE * (packet_count + 1))	);
			con2_port.Send(temp);
		}
		else
		{
			temp.insert(temp.end(),to_send.begin() + LORA_PACKET_SIZE * packet_count,to_send.end());
			con2_port.Send(temp);
		}

		//Check if the device is finished to send.
		this->aux_port.Listen(GPIO::high,2 * LORA_PACKET_SIZE / 37.5);
	}

	//Status and return.
	global_log << "[STATUS][LORA] " << "Send " << to_send.size() << " Bytes in total." << endl;
	show_bytes(to_send.data(),to_send.size());
}



void LoRa::Recv(vector<unsigned char>& recived)
{
	//Status.
	global_log << "[STATUS][LORA] " << "Recive " << recived.size() << " bytes." << endl;
	show_bytes(recived.data(),recived.size());

	//Recive bytes.
	if(this->mode == LoRaTypes::Mode::sleep)
	{
		this->in_buffer_lock.lock();
		this->IBuff.insert(this->IBuff.end(),recived.begin(),recived.end());
		this->in_buffer_lock.unlock();
		this->in_buffer_ring.notify_all();
	}
	else
	{
		this->employer(recived);
	}
}



void LoRa::ChangeMode(LoRaTypes::Mode mode)
{
	try
	{
		//Check if it is in this mode.
		if(mode == this->mode)
		{
			throw 1;
		}

		//Check if the device is busy.
		this->aux_port.Listen(GPIO::high,2 * LORA_PACKET_SIZE / 37.5);

		//Switch device via GPIO.
		this->md0_port = (GPIO::Value_t)((unsigned char)mode & 0x1);
		this->md1_port = (GPIO::Value_t)((unsigned char)mode & 0x2);

		//Check if device is switched succeccfully
		usleep(50000);
		this->aux_port.Listen(GPIO::high,10 * 10.3 * 1e-3);

		//Match the baud rate between the device and the UART port.
		if(mode == LoRaTypes::Mode::sleep)
		{
			con2_port(UART::Baud::b9600);
			con2_port.Sync(UART::sync_now);
		}
		else
		{
			switch(this->Config.baud)
			{
				case LoRaTypes::Baud::b1200: con2_port(UART::Baud::b1200);break;
				case LoRaTypes::Baud::b2400: con2_port(UART::Baud::b2400);break;
				case LoRaTypes::Baud::b4800: con2_port(UART::Baud::b4800);break;
				case LoRaTypes::Baud::b9600: con2_port(UART::Baud::b9600);break;
				case LoRaTypes::Baud::b19200: con2_port(UART::Baud::b19200);break;
				case LoRaTypes::Baud::b38400: con2_port(UART::Baud::b38400);break;
				case LoRaTypes::Baud::b57600: con2_port(UART::Baud::b57600);break;
				case LoRaTypes::Baud::b115200: con2_port(UART::Baud::b115200);break;
			}
			con2_port.Sync(UART::sync_now);
		}

		//Status and return.
		this->mode = mode;
		global_log << "[STATUS][LORA] " << "Switched LoRa into mode " << (int)this->mode << endl;
		return;
	}
	catch(int exp)
	{
		if(exp == 1)
		{
			global_log << "[WARNING][LORA] " << "Device is currently in mode " << (int)mode;
			global_log << " , no need to switch."<< endl;
		}
		return;
	}
	catch(...)
	{
			global_log << "[STATUS][LORA] " << "Unable to switch LoRa into mode " << (int)mode << endl;
			//throw ;
	}
}



void LoRa::GetConfig(LoRaConfig& Config)
{
	Config = this->Config;
}



void LoRa::SetConfig(LoRaConfig& Config,LoRaTypes::SetConfigTo set_to)
{	
	//
	if(Config.local_addr == 0x00) Config.local_addr = this->Config.local_addr;
	if(Config.parity == LoRaTypes::Parity::unchanged) Config.parity = this->Config.parity;
	if(Config.baud == LoRaTypes::Baud::unchanged) Config.baud = this->Config.baud;
	if(Config.air_speed == LoRaTypes::AirSpeed::unchanged) Config.air_speed = this->Config.air_speed;
	if(Config.local_chan == LoRaTypes::Channel::unchanged) Config.local_chan = this->Config.local_chan;
	if(Config.transmit == LoRaTypes::Transmit::unchanged) Config.transmit = this->Config.transmit;
	if(Config.io_drive == LoRaTypes::IODrive::unchanged) Config.io_drive = this->Config.io_drive;
	if(Config.wake_time == LoRaTypes::WakeTime::unchanged) Config.wake_time = this->Config.wake_time;
	if(Config.fec == LoRaTypes::FEC::unchanged) Config.fec = this->Config.fec;
	if(Config.send_power == LoRaTypes::SendPower::unchanged) Config.send_power = this->Config.send_power;

	//
	unsigned char config_ins[6] = {	(	(set_to == LoRaTypes::SetConfigTo::ram)?
						(unsigned char)set_to_ram_ins[0]:
						(unsigned char)set_to_flash_ins[0])	,
					(unsigned char)(Config.local_addr >> 8)			,
					(unsigned char)(Config.local_addr & 0xFF)		,
					/*(	(unsigned char)Config.parity |
						(unsigned char)Config.baud | 
						(unsigned char)Config.air_speed)*/0		,
					(unsigned char)Config.local_chan			,
					/*(	(unsigned char)Config.transmit |
						(unsigned char)Config.io_drive |
						(unsigned char)Config.wake_time |
						(unsigned char)Config.fec |
						(unsigned char)Config.send_power)*/0		};
	config_ins[3] = (unsigned char)Config.parity;
	unsigned char temp = (char)Config.baud;
	config_ins[3] |= temp;
	temp = (unsigned char)Config.air_speed;
	config_ins[3] |= temp;
	config_ins[5] = (unsigned char)Config.transmit;
	temp = (unsigned char)Config.io_drive;
	config_ins [5] |= temp;
	temp = (unsigned char)Config.wake_time;
	config_ins [5] |= temp;
	temp = (unsigned char)Config.fec ;
	config_ins [5] |= temp;
	temp = (unsigned char)Config.send_power;
	config_ins [5] |= temp;		//ARM g++(4.4.6/4.5.1) internal error: segmentation default, 
					//if don't do this.

	vector<unsigned char> config_ins_(config_ins,config_ins + 6);

	for(unsigned i = 1;i <= 3;i++)
	{
		try
		{
			//Send instrutions to set configurations.
			this->Flush();
			con2_port.Send(config_ins_);

			//Check if AUX is high.
			aux_port.Listen(GPIO::high,10 * 140 * 1e-3);

			//
			unique_lock<mutex> lock(this->in_buffer_lock);
			function<bool(void)> pred
			(
				[this]()->bool
				{
					return (this->IBuff.size() >= 4);
				}
			);
			if(!this->in_buffer_ring.wait_for(	lock			,
								chrono::seconds(1)	,
								pred			))
			{
				throw 80;
			}

			//Get respond from device.
			string respond(this->IBuff.begin(),this->IBuff.end());

			//
			if(respond.find(ok_res) != string::npos)
			{
				//If got an "OK" respond.
				this->Flush();
				this->Config = Config;
				Config.Print();

				//Status and return.
				global_log << "[STATUS][LORA] " << "Set configuraions to device:" << endl;
				return;
			}
			else if(respond.find(error_res) != string::npos)
			{
				//If got an "ERROR" respond.
				throw 1;
			}
			else
			{
				//If got other responds.
				throw 2;
			}
		}
		catch(int exp)
		{
			if(exp == 1)
			{
				//If got an "ERROR" respond.
				global_log << "[STATUS][LORA] ";
				global_log << "Got ERROR respond while setting configurations to device." << endl;
			}
			else if(exp == 80 || exp == 2)
			{
				//If got other responds.
				global_log << "[STATUS][LORA] ";
				global_log << "No respond while setting configurations to device." << endl;
			}

			if(i != 3)
			{
				global_log << "[STATUS][LORA] ";
				global_log << "Unable to set configurations to device, resending ... ";
				global_log << i << endl;
			}
			else
			{
				global_log << "[WARNING][LORA] ";
				global_log << "Unable to set configurations to device after 3 trials." << endl;
				//throw ;
			}
		}
		catch(...)
		{
			if(i != 3)
			{
				global_log << "[STATUS][LORA] ";
				global_log << "Unable to set configurations to device, resending ... ";
				global_log << i << endl;
			}
			else
			{
				global_log << "[WARNING][LORA] ";
				global_log << "Unable to set configurations to device after 3 trials." << endl;
				//throw ;
			}
		}
	}
}



void LoRa::SyncConfig(void)
{
	for(int i = 1;i <= 3;i++)
	{
		try
		{
			//Send instructions.
			this->Flush();
			vector<unsigned char> ins(	(unsigned char*)get_configurations_ins,
							(unsigned char*)get_configurations_ins + sizeof(get_configurations_ins));
			con2_port.Send(ins);

			//
			deque<unsigned char>::iterator it;
			unique_lock<mutex> lock(this->in_buffer_lock);
			function<bool(void)> pred
			(
				[this,&it]()->bool
				{
					it = find(this->IBuff.begin(),this->IBuff.end(),'\xC0');
					return (distance(it,this->IBuff.end()) >= 6) ? true : false;
				}
			);
			if(!this->in_buffer_ring.wait_for(	lock			,
								chrono::seconds(1)	,
								pred			))
			{
				throw 80;
			}

			//Second and Third bytes: local address.
			this->Config.local_addr = 0;
			this->Config.local_addr = *(it + 1);
			this->Config.local_addr <<= 8;
			this->Config.local_addr |= *(it + 2);

			//Fourth byte: parity check, baud rate and air speed.
			this->Config.parity = (LoRaTypes::Parity)(*(it + 3) & 0xC0);
			this->Config.baud = (LoRaTypes::Baud)(*(it + 3) & 0x38);
			this->Config.air_speed = (LoRaTypes::AirSpeed)(*(it + 3) & 0x07);

			//Fifth byte: local channel.
			this->Config.local_chan = (LoRaTypes::Channel)*(it + 4);

			//Sixth byte: options.
			this->Config.transmit = (LoRaTypes::Transmit)(*(it + 5) & 0x80);
			this->Config.io_drive = (LoRaTypes::IODrive)(*(it + 5) & 0x40);
			this->Config.wake_time = (LoRaTypes::WakeTime)(*(it + 5) & 0x38);
			this->Config.fec = (LoRaTypes::FEC)(*(it + 5) & 0x04);
			this->Config.send_power = (LoRaTypes::SendPower)(*(it + 5) & 0x03);

			//
			aux_port.Listen(GPIO::high,10 * 2.93 * 1e-3);
			this->Flush();

			//Status and return.
			global_log << "[STATUS][LORA] " << "Syncronize configurations from device:" << endl;
			this->Config.Print();
			return;
		}
		catch(int exp)
		{
			if(i != 3)
			{
				global_log << "[STATUS][LORA] ";
				global_log << "No respond for syncronizing configurations, resending ... ";
				global_log << i << endl;
			}
			else
			{
				global_log << "[WARNING][LORA] ";
				global_log << "Unable to synconize configurations after 3 trials." << endl;
				//throw ;
			}
		}
	}
}



void LoRa::Reset(void)
{
	for(unsigned i = 1;i <= 3;i++)
	{
		try
		{
			//Send instrutions to reset device.
			this->Flush();
			vector<unsigned char> ins(	(unsigned char*)reset_ins,
							(unsigned char*)reset_ins + sizeof(reset_ins));
			con2_port.Send(ins);

			//Check if the device is successfully reset
			aux_port.Listen(GPIO::low);
			aux_port.Listen(GPIO::high);

			//
			unique_lock<mutex> lock(this->in_buffer_lock);
			function<bool(void)> pred
			(
				[this]()->bool
				{
					return (this->IBuff.size() >= 4);
				}
			);
			if(!this->in_buffer_ring.wait_for(	lock			,
								chrono::seconds(1)	,
								pred			))
			{
				throw 80;
			}

			//Get respond from device.
			string respond(this->IBuff.begin(),this->IBuff.end());
			if(respond.find(ok_res) != string::npos)
			{
				//If got an "OK" respond.
				this->Flush();

				//Status and return.
				global_log << "[STATUS][LORA] " << "Reset device." << endl;
				return;
			}
			else if(respond.find(error_res) != string::npos)
			{
				throw 1;
			}
			else
			{
				throw 80;
			}
		}
		catch(int exp)
		{
			if(exp == 1)
			{
				//If got an "ERROR" respond.
				global_log << "[STATUS][LORA] ";
				global_log << "Got ERROR respond while resetting device." << endl;
			}
			else if(exp == 80)
			{
				//If got other responds.
				global_log << "[STATUS][LORA] ";
				global_log << "No respond while resetting device, waiting ... " << endl;
			}

			//
			if(i != 3)
			{
				global_log << "[STATUS][LORA] ";
				global_log << "Unable to reset device, resending ... ";
				global_log << i << endl;
			}
			else
			{
				global_log << "[WARNING][LORA] ";
				global_log << "Unable to reset device after 3 trials." << endl;
				//throw ;
			}
		}
		catch(...)
		{
			if(i != 3)
			{
				global_log << "[STATUS][LORA] ";
				global_log << "Unable to reset device, resending ... ";
				global_log << i << endl;
			}
			else
			{
				global_log << "[WARNING][LORA] ";
				global_log << "Unable to reset device after 3 trials." << endl;
				//throw ;
			}
		}
	}
}



void LoRa::Flush(void)
{
	//Flush UART and IO Buffer.
	con2_port.Flush();
	this->IBuff.clear();
//	this->OBuff.clear();

	global_log << "[STATUS][LORA] " << "IO Buffer flushed." << endl;
}



LoRaConfig::LoRaConfig(void)
{
	this->local_addr = 0xffff;
	this->parity = LoRaTypes::Parity::unchanged;
	this->baud = LoRaTypes::Baud::unchanged;
	this->air_speed = LoRaTypes::AirSpeed::unchanged;
	this->local_chan = LoRaTypes::Channel::unchanged;
	this->transmit = LoRaTypes::Transmit::unchanged;
	this->io_drive = LoRaTypes::IODrive::unchanged;
	this->wake_time = LoRaTypes::WakeTime::unchanged;
	this->fec = LoRaTypes::FEC::unchanged;
	this->send_power = LoRaTypes::SendPower::unchanged;
}



void LoRaConfig::Print(void)
{
	global_log << "\tLoRaConfig" << endl;
	global_log << "\t{" << endl;
	global_log << "\t\tlocal_addr = " << "0x" << hex << this->local_addr << dec << endl;
	global_log << "\t\tparity = " << "0x" << hex << (int)this->parity << dec << endl;
	global_log << "\t\tbaud = " << "0x" << hex << (int)this->baud << dec << endl;
	global_log << "\t\tair_speed = " << "0x" << hex << (int)this->air_speed << dec << endl;
	global_log << "\t\tlocal_chan = " << "0x" << hex << (int)this->local_chan << dec << endl;
	global_log << "\t\ttransmit = " << "0x" << hex << (int)this->transmit << dec << endl;
	global_log << "\t\tio_drive = " << "0x" << hex << (int)this->io_drive << dec << endl;
	global_log << "\t\twake_time = " << "0x" << hex << (int)this->wake_time << dec << endl;
	global_log << "\t\tfec = " << "0x" << hex << (int)this->fec << dec << endl;
	global_log << "\t\tsend_power = " << "0x" << hex << (int)this->send_power << dec << endl;
	global_log << "\t};" << endl;
}



void LoRaConfig::operator=(LoRaConfig& that)
{
	this->local_addr = that.local_addr;
	this->parity = that.parity;
	this->baud = that.baud;
	this->air_speed = that.air_speed;
	this->local_chan = that.local_chan;
	this->transmit = that.transmit;
	this->io_drive = that.io_drive;
	this->wake_time = that.wake_time;
	this->fec = that.fec;
	this->send_power = that.send_power;
}

