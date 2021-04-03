#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <sys/time.h>
#include <unistd.h>
#include "utils.hpp"
#include "global.hpp"

using namespace std;

extern ofstream global_log;

double usec_clock(void)
{	
	timeval tv;
	gettimeofday(&tv,NULL);
	return (double)(tv.tv_sec + tv.tv_usec * 1e-6);
}



void show_bytes(unsigned char* data,unsigned size,unsigned tab_num)
{
	for(int i = 0;i < size / 16;i++)
        {
		for(int j = 1;j <= tab_num;j++)
		{
			global_log << '\t';
		}
        	global_log << "0x" << setw(4) << setfill('0') << hex << i * 16 << dec << "  ";
		for(int j = 0;j < 16;j++)
		{
			int byte = *(data + 16 * i + j);
			global_log << setw(2) << setfill('0') << hex << byte << dec << ' ';
		}
		global_log << "  ";
		for(int j = 0;j < 16;j++)
		{
			char byte = *(data + 16 * i + j);
			if(byte > 0x20 && byte < 0x7f)
			{
				global_log << byte;
			}
			else
			{
				global_log << '.';
			}
		}
		global_log << endl;
	}

	//
	if(size % 16)
	{
		int i = size / 16;

		//
		for(int j = 1;j <= tab_num;j++)
		{
			global_log << '\t';
		}
        	global_log << "0x" << setw(4) << setfill('0') << hex << i * 16 << dec << "  ";

		//
		for(int j = 0;j < 16;j++)
		{
			if(j < size % 16)
			{
				int byte = *(data + 16 * i + j);
				global_log << setw(2) << setfill('0') << hex << byte << dec << ' ';
			}
			else
			{
				global_log << "  " << ' ';
			}
		}
		global_log << "  ";

		//
		for(int j = 0;j < 16;j++)
		{
			if(j < size % 16)
			{
				char byte = *(data + 16 * i + j);
				if(byte > 0x20 && byte < 0x7f)
				{
					global_log << byte;
				}
				else
				{
					global_log << '.';
				}
			}
			else
			{
				global_log << ' ';
			}
		}
		global_log << endl;
	}
}



void msec_delay(double delay)
{
	//Normally,Calculate it.
	//Specially,98000000 round/s on ARM platform.
	static const unsigned delay_unit = 98000000;

	//
	if(delay < 1 * 1e-3)
	{
		global_log << "[ERROR][UTILS] " << "\"msec_delay()\" parameter error: precise is 1ms." << endl;
		//throw ;
	}

	//
	unsigned i = delay * delay_unit;
	while(i--);
}



unsigned random(unsigned short max)
{
	//
	static bool seed_flag = false;
	if(!seed_flag)
	{
		srand(time(NULL));
		seed_flag = true;
	}

	//
	return (rand() % max);
}



void show_time(long to_show)
{
	//
	tm* time = localtime(&to_show);

	//
	cout << 1900 + time->tm_year << "-" << 1 + time->tm_mon << "-" << time->tm_mday << " ";
	cout << time->tm_hour << ":" << time->tm_min << ":" << time->tm_sec << endl;
}



double ddmmss2dd(double dd,double mm,double ss)
{
	//Check if positive.
	if(dd < 0 || mm < 0 || ss < 0)
	{
		global_log << "[ERROR][UTILS]" << "Degree, minutes or seconds is negative." << endl;
		throw 98;
	}

	//Change.
	return (dd + mm / 60 + ss / 60 / 60);
}



int timer(void)
{
	timeval start,stop;
	unsigned count = 0;
	long total_rounds = 0;
	double total_times = 0;
	double total_avr = 0;

	unsigned a,b;
	cin>>a>>b;
	for(unsigned long i = a;i <= b;i *= 10)
	{
		for(unsigned long j = 1;j <= 9;j++)
		{
			unsigned long round = i * j;
			unsigned long k = round;
			total_rounds += round;
			cerr << "Result " << ++count << ": " << round << " rounds ";
			{
				gettimeofday(&start,NULL);
				while(k--);
				gettimeofday(&stop,NULL);
			}
			long gap = (stop.tv_sec - start.tv_sec) * 1000000 + (stop.tv_usec - start.tv_usec);
			total_times += gap;
			total_avr += (round / ((double)(gap) / 1000));
			cerr << (double)(gap) / 1000 << " ms ";
			cerr << round / ((double)(gap) / 1000) << " rounds/ms" << endl;
		}
	}
	cerr << "Average: " << total_rounds / (total_times / 1000) << " rounds/ms" << endl;
	cerr << "Average: " << total_avr / count << " rounds/ms" << endl;
}

