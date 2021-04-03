#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include "gpio.hpp"
#include "global.hpp"
#include "utils.hpp"

using namespace std;

extern ofstream global_log;

GPIO::GPIO(const char* gpio_id)
{
	//Allocate space.
	try
	{
		this->gpio_id = new char[4];
	}
	catch(...)
	{
		global_log << "[ERROR][GPIO] " << "Unable to allocate space for ";
		global_log << '\"' << "GPIO.gpio_id" << '\"' << endl;
		//throw ;
	}
	
	try
	{
		this->direction_addr = new char[40];
	}
	catch(...)
	{
		global_log << "[ERROR][GPIO] " << "Unable to allocate space for ";
		global_log << '\"' << "GPIO.direction_addr" << '\"' << endl;
		//throw ;
	}

	try
	{
		this->value_addr = new char[30];
	}
	catch(...)
	{
		global_log << "[ERROR][GPIO] " << "Unable to allocate space for ";
		global_log << '\"' << "GPIO.value_addr" << '\"' << endl;
		//throw ;
	}

	try
	{
		this->edge_addr = new char[30];
	}
	catch(...)
	{
		global_log << "[ERROR][GPIO] " << "Unable to allocate space for ";
		global_log << '\"' << "GPIO.edge_addr" << '\"' << endl;
		//throw ;
	}

	//Open GPIO
	this->Open(gpio_id);

	global_log << "[STATUS][GPIO] " << "GPIO was allocated space." << endl;
}



GPIO::GPIO(void)
{
	//Allocate space.
	try
	{
		this->gpio_id = new char[4];
	}
	catch(...)
	{
		global_log << "[ERROR][GPIO] " << "Unable to allocate space for ";
		global_log << '\"' << "GPIO.gpio_id" << '\"' << endl;
		//throw ;
	}
	
	try
	{
		this->direction_addr = new char[40];
	}
	catch(...)
	{
		global_log << "[ERROR][GPIO] " << "Unable to allocate space for ";
		global_log << '\"' << "GPIO.direction_addr" << '\"' << endl;
		//throw ;
	}

	try
	{
		this->value_addr = new char[30];
	}
	catch(...)
	{
		global_log << "[ERROR][GPIO] " << "Unable to allocate space for ";
		global_log << '\"' << "GPIO.value_addr" << '\"' << endl;
		//throw ;
	}
	
	try
	{
		this->edge_addr = new char[30];
	}
	catch(...)
	{
		global_log << "[ERROR][GPIO] " << "Unable to allocate space for ";
		global_log << '\"' << "GPIO.edge_addr" << '\"' << endl;
		//throw ;
	}

	//GPIO is not open.
	this->open_flag = false;

	global_log << "[STATUS][GPIO] " << "GPIO was allocated space." << endl;
}



GPIO::~GPIO()
{
	//Close GPIO port.
	if(this->open_flag)
	{
		this->Close();
	}

	//Free space
	delete this->gpio_id;
	delete this->direction_addr;
	delete this->value_addr;

	global_log << "[STATUS][GPIO] " << "Space for GPIO was free." << endl;
}



void GPIO::Open(const char* gpio_id)
{
	//gpio_id initializtion
	strcpy(this->gpio_id,gpio_id);
	if(!this->gpio_id)
	{
		global_log << "[ERROR][GPIO] " << '\"' << "GPIO.gpio_id" << '\"' << " initialization failed." << endl;
		this->open_flag = false;
		//throw ;
	}
	else
	{
		global_log << "[STATUS][GPIO] " << "GPIO.gpio_id = " << '\"' << this->gpio_id << '\"' << endl;
	}
	
	//direction_addr initialization.
	strcpy(this->direction_addr,"/sys/class/gpio/gpio");
	strcat(this->direction_addr,gpio_id);
	strcat(this->direction_addr,"/direction");
	if(!this->direction_addr)
	{
		global_log << "[ERROR][GPIO] " << '\"' << "GPIO.direction_addr" << '\"' << " initialization failed." << endl;
		this->open_flag = false;
		//throw ;
	}
	else
	{
		global_log << "[STATUS][GPIO] " << "GPIO.direction_addr = " << '\"' << this->direction_addr << '\"' << endl;
	}

	//value_addr initialization.
	strcpy(this->value_addr,"/sys/class/gpio/gpio");
	strcat(this->value_addr,gpio_id);
	strcat(this->value_addr,"/value");
	if(!this->value_addr)
	{
		global_log << "[ERROR][GPIO] " << '\"' << "GPIO.value_addr" << '\"' << " initialization failed." << endl;
		this->open_flag = false;
		//throw ;
	}
	else
	{
		global_log << "[STATUS][GPIO] " << "GPIO.value_addr = " << '\"' << this->value_addr << '\"' << endl;
	}
	
	//edge_addr initialization.
	strcpy(this->edge_addr,"/sys/class/gpio/gpio");
	strcat(this->edge_addr,gpio_id);
	strcat(this->edge_addr,"/edge");
	if(!this->edge_addr)
	{
		global_log << "[ERROR][GPIO] " << '\"' << "GPIO.edge_addr" << '\"' << " initialization failed." << endl;
		this->open_flag = false;
		//throw ;
	}
	else
	{
		global_log << "[STATUS][GPIO] " << "GPIO.edge_addr = " << '\"' << this->edge_addr << '\"' << endl;
	}
	
	//export GPIO
	ofstream fp("/sys/class/gpio/export",ios::out);
	if(!fp.is_open())
	{
		global_log << "[ERROR][GPIO] " << "/sys/class/gpio/export" << " open failed." << endl;
		this->open_flag = false;
		//throw ;
	}
	fp << this->gpio_id;
	if(0/*check if gpio is occupied*/)
	{
		global_log << "[ERROR][GPIO] " << "GPIO export failed." << endl;
		this->open_flag = false;
	}
	else
	{
		global_log << "[STATUS][GPIO] " << "GPIO exported." << endl;
	}
	fp.close();

	//Normal return.
	this->open_flag = true;
	global_log << "[STATUS][GPIO] " << "GPIO was opened." << endl;
}



void GPIO::Close(void)
{
	//Unexport GPIO
	ofstream fp("/sys/class/gpio/unexport",ios::out);
	if(!fp.is_open())
	{
		global_log << "[ERROR][GPIO] " << "Unable to open " << '\"' << "/sys/class/gpio/unexport" << '\"' << endl;
		global_log << "[ERROR][GPIO] " << "Unable to close GPIO" << this->gpio_id << endl;
		//throw ;
	}
	fp << this->gpio_id;
	if(0/*check if gpio is hidden*/)
	{
		global_log << "[ERROR][GPIO] " << "Unable to unexport GPIO" << this->gpio_id << "." << endl;
		global_log << "[ERROR][GPIO] " << "Unable to close GPIO" << this->gpio_id << endl;
		//throw ;
	}
	else
	{
		global_log << "[STATUS][GPIO] " << "GPIO" << this->gpio_id << " unexported." << endl;
	}
	fp.close();

	this->open_flag = false;
	global_log << "[STATUS][GPIO] " << "GPIO was closed." << endl;
}



void GPIO::Listen(const GPIO::Value_t value,double time_out,double time_wait)
{
	//Check if GPIO is opened.
	if(!this->open_flag)
	{
		global_log << "[ERROR][GPIO] " << "GPIO "  << this->gpio_id << " is not opened." << endl;
		global_log << "[ERROR][GPIO] " << "Unable to listen GPIO." << endl;
		//throw ;
	}

	//
	if(time_wait >= time_out)
	{
		global_log << "[ERROR][GPIO] " << "Parameter error:\"time_out\" is smaller than \"time_wait\"." << endl;
		global_log << "[ERROR][GPIO] " << "Unable to listen GPIO." << endl;
		//throw ;
	}
	
	//epoll()???
	
	double start = usec_clock(),stop = start;
	do
	{
		if(*this == value)
		{
			msec_delay(time_wait);
			if(*this == value)
			{
				global_log << "[STATUS][GPIO] " << "Listen GPIO = " << (bool)value;
				global_log << " for " << stop - start << "s." << endl;
				return;
			}
			else
			{
				continue;
			}
		}
		stop = usec_clock();
	}while(stop - start < time_out);

	//
	global_log << "[WARNING][GPIO] " << "Listen GPIO = " << (bool)value << " time out." << endl;
	throw 100;
}



int GPIO::operator=(const Value_t value)
{
	//Check if GPIO is occupied
	if(!this->open_flag)
	{
		global_log << "[ERROR][GPIO] " << "GPIO "  << this->gpio_id << " is not opened." << endl;
		global_log << "[WARNING][GPIO] " << "Unable to set GPIO value." << endl;
		//throw ;
		return -1;
	}
	
	//Set GPIO into output mode
	ofstream fp(this->direction_addr);
	if(!fp.is_open())
	{
		global_log << "[ERROR][GPIO] " << "Unable to open " << '\"' << this->direction_addr << '\"' << endl;
		global_log << "[WARNING][GPIO] " << "Unable to set GPIO value." << endl;
		//throw ;
		return -1;
	}
	fp << "out";
	fp.close();

	//Set GPIO value
	fp.open(this->value_addr);
	if(!fp.is_open())
	{
		global_log << "[ERROR][GPIO] " << "Unable to open " << '\"' << this->value_addr << '\"' << endl;
		global_log << "[WARNING][GPIO] " << "Unable to set GPIO value." << endl;
		//throw ;
		return -1;
	}
	fp << (bool)value;
	fp.close();

	//default return
	return (bool)value;

	global_log << "[STATUS][GPIO] " << "Set GPIO into " << '\"' << (bool)value << '\"' << endl;
}



int GPIO::operator~(void)
{
	//Check if GPIO is occupied
	if(!this->open_flag)
	{
		global_log << "[ERROR][GPIO] " << "GPIO "  << this->gpio_id << " is not opened." << endl;
		global_log << "[WARNING][GPIO] " << "Unable to set GPIO value." << endl;
		//throw ;
		return -1;
	}

	//Get GPIO value
	ifstream fp(this->value_addr);
	if(!fp.is_open())
	{
		global_log << "[ERROR][GPIO] " << "Unable to open " << '\"' << this->value_addr << '\"' << endl;
		global_log << "[WARNING][GPIO] " << "Unable to invert GPIO value." << endl;
		//throw ;
		return -1;
	}
	bool value;
	fp >> value;
	fp.close();

	//Inverse GPIO value
	if(value)
	{
		*this = low;
	}
	else
	{
		*this = high;
	}

	//default return
	return (!(bool)value);

	global_log << "[STATUS][GPIO] " << "Invert GPIO into " << '\"' << !(bool)value << '\"' << endl;
}



int GPIO::operator==(const Value_t that_value)
{
	//Check if GPIO occupied
	if(!this->open_flag)
	{
		global_log << "[ERROR][GPIO] " << "GPIO "  << this->gpio_id << " is not opened." << endl;
		global_log << "[WARNING][GPIO] " << "Unable to get GPIO value." << endl;
		//throw ;
		return -1;
	}

	//Set GPIO into input mode
	fstream fp(this->direction_addr,ios::out);
	if(!fp.is_open())
	{
		global_log << "[ERROR][GPIO] " << "Unable to open " << '\"' << this->direction_addr << '\"' << endl;
		global_log << "[WARNING][GPIO] " << "Unable to get GPIO value." << endl;
		//throw ;
		return -1;
	}
	fp << "in";
	fp.close();

	//Set GPIO edge
	fp.open(this->edge_addr,ios::out);
	if(!fp.is_open())
	{
		global_log << "[ERROR][GPIO] " << "Unable to open " << '\"' << this->edge_addr << '\"' << endl;
		global_log << "[WARNING][GPIO] " << "Unable to get GPIO value." << endl;
		//throw ;
		return -1;
	}
	fp << "both";
	fp.close();

	//Get this GPIO value
	fp.open(this->value_addr,ios::in);
	if(!fp.is_open())
	{
		global_log << "[ERROR][GPIO] " << "Unable to open " << '\"' << this->value_addr << '\"' << endl;
		global_log << "[WARNING][GPIO] " << "Unable to get GPIO value." << endl;
		//throw ;
		return -1;
	}
	bool this_value;
	fp >> this_value;
	fp.close();

	//
	return ((Value_t)this_value == that_value)?true:false;
}



int GPIO::operator!=(const Value_t that_value)
{
	//Check if GPIO occupied
	if(!this->open_flag)
	{
		global_log << "[ERROR][GPIO] " << "GPIO "  << this->gpio_id << " is not opened." << endl;
		global_log << "[WARNING][GPIO] " << "Unable to get GPIO value." << endl;
		//throw ;
		return -1;
	}

	//Set GPIO into input mode
	fstream fp(this->direction_addr,ios::out);
	if(!fp.is_open())
	{
		global_log << "[ERROR][GPIO] " << "Unable to open " << '\"' << this->direction_addr << '\"' << endl;
		global_log << "[WARNING][GPIO] " << "Unable to get GPIO value." << endl;
		//throw ;
		return -1;
	}
	fp << "in";
	fp.close();

	//Get this GPIO value
	fp.open(this->value_addr,ios::in);
	if(!fp.is_open())
	{
		global_log << "[ERROR][GPIO] " << "Unable to open " << '\"' << this->value_addr << '\"' << endl;
		global_log << "[WARNING][GPIO] " << "Unable to get GPIO value." << endl;
		//throw ;
		return -1;
	}
	bool this_value;
	fp >> this_value;
	fp.close();

	//
	return ((Value_t)this_value != that_value)?true:false;
}



bool GPIO::Is_Open(void)
{
	return this->open_flag;
}

