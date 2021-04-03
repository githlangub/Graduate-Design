#ifndef __APP_HPP__
#define __APP_HPP__

#include <thread>
#include "mac.hpp"
#include "gnss.hpp"
#include "nmea0183.hpp"

class APP
{
	public:
		enum class UserState:unsigned char
		{
			local		= 0,
			onlined		= 1,
			offlined	= 2
		};

		struct UserInfo
		{
			unsigned short	user_id;
			unsigned long	last_time;

			double		longitude;
			double		latitude;

			UserState	state;

			UserInfo(unsigned short ,unsigned long ,double ,double ,UserState);
		};

	public:
		APP(void);
		~APP();

		void Send(vector<unsigned char>& );
		void Recv(vector<unsigned char>& );
		void RecvGNSS(GnssMsg& );

		void Client(void);
		void UserTimer(void);
		void Exec(void);
	
	private:
		deque<UserInfo> user_list;
		mutex user_list_lock;

		MAC employee;

		GNSS gnss_reciver;

		bool run_flag;
		thread app_exec;
		thread user_timer;
};

#endif

