#ifndef __NMEA0183_HPP__
#define __NMEA0183_HPP__

#include <string>
#include <vector>
#include <deque>

using namespace std;

struct GnssMsg
{
	//
	//long time_stamp;
	double longitude;
	double latitude;

	//
	GnssMsg(void);
	GnssMsg(double ,double );
	GnssMsg(GnssMsg& );
	GnssMsg(GnssMsg&& );

	void operator=(GnssMsg& );
	void operator=(GnssMsg&& );
	void Print(void);
};



class NMEA0183
{
	public:
		GnssMsg operator()(deque<unsigned char>::iterator ,deque<unsigned char>::iterator );

	private:
		GnssMsg ResolveGNGGA(vector<string>& );
};

#endif

