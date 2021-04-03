#include <iostream>
#include <vector>
#include <sys/time.h>
#include "message.hpp"
#include "utils.hpp"

using namespace std;

Message::Message(vector<unsigned char>& data,unsigned short user_id,double longitude,double latitude):
	data(data),
	user_id(user_id),
	longitude(longitude),
	latitude(latitude)
{
}



Message::Message(vector<unsigned char>& raw_bit)
{
	//
	this->DeEncapsulate(raw_bit);
}



void Message::Encapsulate(void)
{
	//Set gnss information.
	for(int i = 0;i < 8;i++)
	{
		/*unsigned long* temp = (unsigned long*)(&this->latitude);
		this->data.insert(this->data.begin(),((*temp) >> (i * 8)) & 0xFF);*/
		this->data.insert(this->data.begin(),*((unsigned char*)(&this->latitude) + i));
	}
	for(int i = 0;i < 8;i++)
	{
		/*unsigned long* temp = (unsigned long*)(&this->longitude);
		this->data.insert(this->data.begin(),((*temp) >> (i * 8)) & 0xFF);*/
		this->data.insert(this->data.begin(),*((unsigned char*)(&this->longitude) + i));
	}

	//Set user ID.
	this->data.insert(this->data.begin(),(unsigned char)(this->user_id & 0xFF));
	this->data.insert(this->data.begin(),(unsigned char)(this->user_id >> 8));
}



void Message::DeEncapsulate(vector<unsigned char>& raw_bit)
{
	if(raw_bit.size() < 18)
	{
		cerr << "[ERROR][APP] " << "DeEncapsulate failed: too short, ";
		cerr << "the size is " << raw_bit.size() << endl;
		throw 90;
		//return;
	}

	//Get user id.
	this->user_id = ((unsigned short)raw_bit.at(0) << 8) | raw_bit.at(1);

	//Get gnss information.
	unsigned char* temp = (unsigned char*)(&this->longitude);
	for(int i = 0;i < 8;i++)
	{
		//*temp |= ((unsigned long)raw_bit.at(2 + i) << ((7 - i) * 8));
		*(temp + (7 - i)) = raw_bit.at(2 + i);
	}

	temp = (unsigned char*)(&this->latitude);
	for(int i = 0;i < 8;i++)
	{
		//*temp |= ((unsigned long)raw_bit.at(10 + i) << ((7 - i) * 8));
		*(temp + (7 - i)) = raw_bit.at(10 + i);
	}

	//Get short message.
	this->data.assign(raw_bit.begin() + 18,raw_bit.end());
}



void Message::Print(void)
{
	cerr << "\tMessage" << endl;
	cerr << "\t{" << endl;
	//cerr << "\t\tdata = " << endl;
	//show_bytes(this->data.data(),this->data.size(),3);
	cerr << "\t\tuser_id = 0x" << hex << this->user_id << dec << endl;
	cerr << "\t\tlongitude = " << this->longitude << endl;
	cerr << "\t\tlatitude = " << this->latitude << endl;
	cerr << "\t}" << endl;
}

