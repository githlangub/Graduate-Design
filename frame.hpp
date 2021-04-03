#ifndef __FRAME_HPP__
#define __FRAME_HPP__

#include <vector>
#include <sys/time.h>

using namespace std;

class Frame
{
	public:
		Frame(void);
		Frame(vector<unsigned char>& );

		void Encapsulate(void);
		void DeEncapsulate(vector<unsigned char>& );
		//void Clear(void);
		void Print(void);

	public:
		vector<unsigned char> data;

		unsigned long	time_stamp;
		unsigned short	src_addr;
		bool		spd_flag;
		bool		itn_flag;
		bool		crq_flag;
		bool		qry_flag;
		bool		syn_flag;
		bool		fin_flag;
		bool		ack_flag;

		unsigned char	front_delimiter;
		unsigned char	back_delimiter;
		unsigned char	escape_character;

		unsigned short	dst_addr;
		unsigned char	dst_chan;

	unsigned char frame_flag;
};

#endif

