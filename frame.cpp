#include <iostream>
#include <vector>
#include <sys/time.h>
#include "frame.hpp"
#include "utils.hpp"

using namespace std;

Frame::Frame(void):
	src_addr(0x00),
	spd_flag(false),
	itn_flag(false),
	crq_flag(false),
	qry_flag(false),
	syn_flag(false),
	fin_flag(false),
	ack_flag(false),
	front_delimiter('\x81'),
	back_delimiter('\xA5'),
	escape_character('\x99'),
	dst_addr(0xffff),
	dst_chan(0x00)
{
	timeval now;
	if(gettimeofday(&now,NULL) < 0)
	{
		cerr << "[STATUS][MAC] " << "Unable to get time stamp,encapsulation failed." << endl;
		throw 99;
	}
	this->time_stamp = now.tv_sec;
}



Frame::Frame(vector<unsigned char>& raw_bit)//:Frame()
{
	this->time_stamp = 0;
	this->src_addr = 0x00;
	this->spd_flag = false;
	this->itn_flag = false;
	this->crq_flag = false;
	this->fin_flag = false;
	this->ack_flag = false;
	this->front_delimiter = '\x81';
	this->back_delimiter = '\xA5';
	this->escape_character = '\x99';

	//
	this->DeEncapsulate(raw_bit);
}



void Frame::Encapsulate(void)
{
	//Set flags of frame.
	unsigned char frame_flag = (itn_flag << 6) | (itn_flag << 5) | (crq_flag << 4) | (qry_flag << 3) | (syn_flag << 2) | (fin_flag << 1) | ack_flag;
	this->data.insert(this->data.begin(),(unsigned char)frame_flag);
	
	//Set source address.
	this->data.insert(this->data.begin(),(unsigned char)(this->src_addr & 0xFF));
	this->data.insert(this->data.begin(),(unsigned char)(this->src_addr >> 8));

	//Set time stamp.
	timeval now;
	if(gettimeofday(&now,NULL) < 0)
	{
		cerr << "[STATUS][MAC] " << "Unable to get time stamp,encapsulation failed." << endl;
		throw 99;
	}
	this->time_stamp = now.tv_sec;
	for(int i = 0;i < 8;i++)
	{
		this->data.insert(this->data.begin(),(unsigned char)((this->time_stamp >> (i * 8)) & 0xFF));
	}

	//Escape sequence.
	for(int i = 0;i < this->data.size();i++)
	{
		if(	this->data.at(i) == this->front_delimiter	||
			this->data.at(i) == this->back_delimiter	||
			this->data.at(i) == this->escape_character	)
		{
			this->data.insert(this->data.begin() + i,this->escape_character);
			i++;
		}
	}

	//Set front & back delimiter.
	this->data.insert(this->data.begin(),this->front_delimiter);
	this->data.insert(this->data.end(),this->back_delimiter);
}



void Frame::DeEncapsulate(vector<unsigned char>& raw_bit)
{
	//Delete escape char.
	for(int i = 0;i < raw_bit.size();i++)
	{
		if(raw_bit.at(i) == this->escape_character)
		{
			raw_bit.erase(raw_bit.begin() + i);
			if(	raw_bit.at(i) != this->front_delimiter	&&
				raw_bit.at(i) != this->back_delimiter	&&
				raw_bit.at(i) != this->escape_character	)
			{
				cerr << "[ERROR][MAC] " << "DeEncapsulate failed: ";
				cerr << "escape character position error." << endl;
				throw 61;
			}
		}
	}

	//
	if(raw_bit.size() < 13)
	{
		cerr << "[ERROR][MAC] " << "DeEncapsulate failed: too short, ";
		cerr << "the size is " << raw_bit.size() << endl;
		throw 60;
	}
	if(raw_bit.front() != this->front_delimiter)
	{
		cerr << "[ERROR][MAC] " << "DeEncapsulate failed: front_delimiter not match.";
		cerr << "the first bytes is 0x" << hex << (unsigned)raw_bit.front() << dec << endl;
		throw 60;
	}
	if(raw_bit.back() != this->back_delimiter)
	{
		cerr << "[ERROR][MAC] " << "DeEncapsulate failed: back_delimiter not match, ";
		cerr << "the back bytes is 0x" << hex << (unsigned)raw_bit.back() << dec << endl;
		throw 60;
	}

	//Set time stamp.
	this->time_stamp = 0;
	for(int i = 0;i < 8;i++)
	{
		this->time_stamp |= ((unsigned long)raw_bit.at(1 + i) << ((7 - i) * 8));
	}

	//Set source address.
	this->src_addr = ((unsigned short)raw_bit.at(9) << 8) | raw_bit.at(10);

	//Set flags of the frame.
	this->spd_flag = (bool)(raw_bit.at(11) & 0x20);
	this->itn_flag = (bool)(raw_bit.at(11) & 0x20);
	this->crq_flag = (bool)(raw_bit.at(11) & 0x10);
	this->qry_flag = (bool)(raw_bit.at(11) & 0x08);
	this->syn_flag = (bool)(raw_bit.at(11) & 0x04);
	this->fin_flag = (bool)(raw_bit.at(11) & 0x02);
	this->ack_flag = (bool)(raw_bit.at(11) & 0x01);

	//Set data.
	this->data.assign(raw_bit.begin() + 12,raw_bit.end() - 1);
}



/*void Frame::Clear(void)
{
	if(this->body_size > 0)
	{
		frame.erase(this->frame.begin() + this->head_size ,this->frame.end() - this->tail_size);
	}

	//Status and return.
	this->body_size = 0;
	this->size = this->head_size + this->body_size + this->tail_size;
}*/



void Frame::Print(void)
{
	cerr << "\tFrame" << endl;
	cerr << "\t{" << endl;
	cerr << "\t\tdata = " << endl;
	show_bytes(this->data.data(),this->data.size(),3);
	cerr << "\t\ttime_stamp = " << this->time_stamp << endl;
	cerr << "\t\tsrc_addr = 0x" << hex << this->src_addr << dec << endl;
	cerr << "\t\tspd_flag = " << this->spd_flag << endl;
	cerr << "\t\titn_flag = " << this->itn_flag << endl;
	cerr << "\t\tcrq_flag = " << this->crq_flag << endl;
	cerr << "\t\tqry_flag = " << this->qry_flag << endl;
	cerr << "\t\tsyn_flag = " << this->syn_flag << endl;
	cerr << "\t\tfin_flag = " << this->fin_flag << endl;
	cerr << "\t\tack_flag = " << this->ack_flag << endl;
	cerr << "\t\tfront_delimiter = 0x" << hex << (unsigned)this->front_delimiter << dec << endl;
	cerr << "\t\tback_delimiter = 0x" << hex << (unsigned)this->back_delimiter << dec << endl;
	cerr << "\t\tescape_character = 0x" << hex << (unsigned)this->escape_character << dec << endl;
	cerr << endl;
	cerr << "\t\tSend at ";
	show_time(this->time_stamp);
	cerr << endl;
	cerr << "\t\tNote: " << "?" << " bytes in total,which includes " << this->data.size() << " of data." << endl;
	cerr << "\t}" << endl;
}

