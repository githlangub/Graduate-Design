#include <iostream>
#include <fstream>
#include "global.hpp"

using namespace std;

Config::Config(void):
	lora_addr(0x00),
	mac_addr(lora_addr),
	app_user_id(lora_addr),
	uart_lora_dev("/dev/ttySAC2"),
	uart_gnss_dev("/dev/ttySAC3")
{
}



void Config::operator()(const char* config)
{
	fstream fp(config,ios::in | ios::out);
 
	fp >> this->lora_addr;
	fp >> mac_addr;
	fp >> app_user_id;
	fp >> uart_lora_dev;
	fp >> uart_gnss_dev;
};

