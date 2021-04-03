//Test4: Function of GNSS module.

#include <iostream>
#include <string>
#include "gnss.hpp"
#include "nmea0183.hpp"

using namespace std;

void recv(GnssMsg& new_msg)
{
	new_msg.Print();
}

int main(int argc,char* argv[])
{
	string gnss_dev_addr(argv[1]);
	GNSS G1(gnss_dev_addr,recv);

	while(true);

	return 0;
}

