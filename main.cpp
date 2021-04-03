//	main.cpp
//
//	??

#include <iostream>
#include <fstream>
#include <atomic>
#include <cstring>
#include <sys/signal.h>
#include "global.hpp"
#include "app.hpp"
#include "message.hpp"

using namespace std;

Config global_config;

ofstream global_log("/root/sharer.log",ofstream::out);

atomic<bool> run_flag(true);

void int_handler(int signum)
{
	if(signum == SIGINT)
	{
		run_flag = false;
	}
}

int main(int argc,char* argv[])
{
	//Resolve "^C" signal.
	signal(SIGINT,int_handler);

	//Resolve options led by "-".
	for(int i = 1;i < argc;i++)
	{
		if(argv[i][0] == '-')
		{
			if(argv[i][1] == 'c' && strlen(argv[i]) == 2)
			{
				if(i != argc) global_config(argv[i + 1]);
			}
			else if(argv[i][1] == 'l' && strlen(argv[i]) == 2)
			{
				//if(i != argc) global_log(argv[i + 1]);
			}
			else
			{
				global_log << "[WARNING][MAIN] ";
				global_log << "Unknowned option \"" << argv[i] << "\"" << endl;
			}
		}
	}

	//Core function.
	APP app;

	while(run_flag)
	{
		int option = 0;
		cout << "Option: ";
		cin >> option;
		if(option == 1)
		{
			string str("ABC");
			cin >> str;
			vector<unsigned char> to_send(str.begin(),str.end());
			app.Send(to_send);
			usleep(1000);
		}
		else if(option == 2)
		{
			//Layer2.ManualNetwork();
		}
		else if(option == 3)
		{
			vector<unsigned char> sms;
			Message to_send(sms,22,108.836,34.131);
			to_send.Encapsulate();
			app.Send(to_send.data);
		}
		else if(option == 4)
		{
			app.Client();
		}
		else
		{
			run_flag = false;
		}
	}

	return 0;
}

