#ifndef __MESSAGE_HPP__
#define __MESSAGE_HPP__

#include <vector>

using namespace std;

class Message
{
	public:
		//Constructor.
		Message(vector<unsigned char>& ,unsigned short ,double ,double );
		Message(vector<unsigned char>& );

		void Encapsulate(void);
		void DeEncapsulate(vector<unsigned char>& );

		void Print(void);

	public:
		vector<unsigned char> data;

		unsigned short	user_id;
		double		longitude;
		double		latitude;
};

#endif

