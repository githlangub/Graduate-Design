#ifndef __GLOBAL_HPP__
#define __GLOBAL_HPP__

#include <string>

using namespace std;

struct Config
{
	unsigned short lora_addr;
	unsigned short mac_addr;
	unsigned short app_user_id;
	
	string uart_lora_dev;
	string uart_gnss_dev;
	string gpio_lora_md0_dev;
	string gpio_lora_md1_dev;
	string gpio_lora_aux_dev;
	string gpio_gnss_pps_dev;

	Config(void);

	void operator()(const char* config);
};

#endif

