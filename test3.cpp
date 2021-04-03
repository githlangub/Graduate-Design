//Test3: Function of ./uart.cpp.

#include <iostream>
#include <string>
#include <vector>
#include "uart.hpp"

using namespace std;

void uart_recv(vector<unsigned char>& recived)
{
	//
	cout << "Recived " << recived.size() << " bytes: ";
	for(auto it = recived.begin();it != recived.end();it++)
	{
		cout << *it;
	}
	cout << endl;
}


int main(int argc,char* argv[])
{
	string uart_dev_addr(argv[1]);
	UART U1(uart_dev_addr,uart_recv);
	U1(UART::Baud::b38400);
	U1.Sync(UART::sync_now);

	while(true)
	{
		string temp;
		cin >> temp;
		vector<unsigned char> to_send(temp.begin(),temp.end());
		to_send.push_back('\x0d');
		U1.Send(to_send);
	};

	return 0;
}

