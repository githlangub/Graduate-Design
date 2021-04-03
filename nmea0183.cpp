#include <iostream>
	#include <fstream>
#include <iomanip>
#include <string>
#include <deque>
#include <mutex>
#include <chrono>
#include <functional>
#include <cmath>
#include "gnss.hpp"
#include "utils.hpp"

using namespace std;

extern ofstream global_log;

GnssMsg NMEA0183::operator()(deque<unsigned char>::iterator head,deque<unsigned char>::iterator tail)
{
	//
	if(*head != '$' || *(tail - 1) != '*')
	{
		global_log << "[ERROR][NMEA0183] " << "Invalid GNSS message." << endl;
		throw ;
	}

	//Take out all fields of this message.
	vector<string> fields_list;
	for(	auto it = head + 1	,
		last_break = head	;
		it != tail		;
		it++			)
	{
		if(*it == ',' || *it == '*')
		{
			string temp(last_break + 1,it);
			//fields_list.push_back(move(temp));
			fields_list.push_back(temp);
			last_break = it;
		}
	}
	
	//
	if(fields_list.size() > 0)
	{
		if(fields_list.at(0) == "GNGGA")
		{
			return this->ResolveGNGGA(fields_list);
		}
		else
		{
			global_log << "[WARNING][NMEA0183] ";
			global_log << "Unable to resolve, unknown message ID." << endl;
			throw 107;
		}
	}
}



GnssMsg NMEA0183::ResolveGNGGA(vector<string>& fields_list)
{
	//
	if(fields_list.at(0) != "GNGGA")
	{
		throw 107;
	}

	GnssMsg new_msg;

	//Get UTC.
	if(fields_list.at(1).size() > 0)
	{
	}
	else
	{
	}
	
	//Get latitude.
	if(fields_list.at(2).size() > 0 && fields_list.at(3).size() > 0)
	{
		//degree`minute' format.
		double degree =	(fields_list.at(2).at(0) - 0x30) * 10	+
				(fields_list.at(2).at(1) - 0x30)	;
		double minute =	(fields_list.at(2).at(2) - 0x30) * 10	+
				(fields_list.at(2).at(3) - 0x30)	;
		for(int i = 5;i < fields_list.at(2).size();i++)
		{
			minute += ((fields_list.at(2).at(i) - 0x30) * pow(10,4 - i));
		}
	
		//degree` format.
		new_msg.latitude = ddmmss2dd(degree,minute,0);

		//Latitue N or S.
		if(fields_list.at(3) == "S")
		{
			new_msg.latitude = -new_msg.latitude;
		}
	}
	else
	{
	}
	
	//Get longitude.
	if(fields_list.at(4).size() > 0 && fields_list.at(5).size() > 0)
	{
		//degree`minute' format.
		double degree =	(fields_list.at(4).at(0) - 0x30) * 100	+
				(fields_list.at(4).at(1) - 0x30) * 10 	+
				(fields_list.at(4).at(2) - 0x30)	;
		double minute =	(fields_list.at(4).at(3) - 0x30) * 10	+
				(fields_list.at(4).at(4) - 0x30)	;
		for(int i = 6;i < fields_list.at(4).size();i++)
		{
			minute += ((fields_list.at(4).at(i) - 0x30) * pow(10,5 - i));
		}
	
		//degree` format.
		new_msg.longitude = ddmmss2dd(degree,minute,0);

		//Latitue N or S.
		if(fields_list.at(5) == "W")
		{
			new_msg.longitude = -new_msg.longitude;
		}
	}
	else
	{
	}

	//Status & return.
	global_log << "[STATUS][NMEA0183] ";
	global_log << "Resolving a " << fields_list.at(0) << " message." << endl;
	//new_msg.Print();
	return new_msg;
}



GnssMsg::GnssMsg(void):
	longitude(0),
	latitude(0)
{
}



GnssMsg::GnssMsg(double longitude,double latitude):
	longitude(longitude),
	latitude(latitude)
{
}



GnssMsg::GnssMsg(GnssMsg& to_assign):
	longitude(to_assign.longitude),
	latitude(to_assign.latitude)
{
}



GnssMsg::GnssMsg(GnssMsg&& to_assign):
	longitude(to_assign.longitude),
	latitude(to_assign.latitude)
{
}



void GnssMsg::operator=(GnssMsg& to_assign)
{
	this->longitude = to_assign.longitude;
	this->latitude = to_assign.latitude;
}



void GnssMsg::operator=(GnssMsg&& to_assign)
{
	this->longitude = to_assign.longitude;
	this->latitude = to_assign.latitude;
}



void GnssMsg::Print(void)
{
	global_log.precision(10);

	global_log << "\tGnssMsg" << endl;
	global_log << "\t{" << endl;
	global_log << "\t\tlongitude = " << this->longitude << endl;
	global_log << "\t\tlatitude = " << this->latitude << endl;
	global_log << "\t}" << endl;
}

