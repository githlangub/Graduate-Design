#ifndef __MAC_HPP__
#define __MAC_HPP__

#include <vector>
#include <deque>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>
#include "mac.hpp"
#include "frame.hpp"
#include "lora.hpp"
#include "utils.hpp"

#define MTU 100

using namespace std;

struct ThreadControl
{
	enum{run,stop,pause} 	status;
	unsigned		operation;
	thread::id		id;

	ThreadControl(void):status(run),operation(0){};
};



struct ThreadCtl
{
	bool			run_flag;
	bool			pause_flag;
	bool			reset_flag;
	thread::id		id;
	//mutex			lock;
	condition_variable	ring;

	ThreadCtl(void):run_flag(true),pause_flag(false),reset_flag(false){};
};


class MAC;
class UserList
{
	public:
		struct UserInfo
		{
			unsigned short 	user_addr;
			unsigned long	last_time;

			thread::id	daemon;

			UserInfo(unsigned short user_addr,unsigned long time_stamp):
				user_addr(user_addr),last_time(time_stamp){};

			inline void Print(void);
		};

	public:
		UserList(unsigned short ,MAC& );
		~UserList();

		void New(unsigned short ,unsigned long );
		void Delete(unsigned short );
		void Update(unsigned short,unsigned long );
		void Print(void);
		void Clear(void);
		int Find(unsigned short );
		vector<UserInfo> Info(unsigned short = 0);

		void UserManager(void);

	private:
		deque<UserInfo> user_list;
		mutex lock;

		const unsigned short local_addr;

		//ThreadPool user_daemon;
		deque<ThreadCtl*> daemon_pool;
		mutex daemon_pool_lock;

		MAC& interface;
};



struct ThreadCtl1
{
	bool			run_flag;
	bool			pause_flag;
	bool			reset_flag;
	bool			manual_network_flag;
	bool			user_timeout_flag;
	unsigned short		timeout_user;
	bool			heart_beat_flag;
	thread::id		id;
	condition_variable	ring;

	ThreadCtl1(void):run_flag(true),pause_flag(false),reset_flag(false){};
};



struct ThreadCtl2
{
	bool			run_flag;
	bool			pause_flag;
	bool			reset_flag;
	bool			heart_beat_flag;
	bool			gateway_timeout_flag;
	thread::id		id;
	condition_variable	ring;

	ThreadCtl2(void):run_flag(true),pause_flag(false),reset_flag(false){};
};



struct ThreadCtl3
{
	bool			run_flag;
	bool			pause_flag;
	bool			reset_flag;
	bool			heart_beat_flag;
	thread::id		id;
	condition_variable	ring;

	ThreadCtl3(void):run_flag(true),pause_flag(false),reset_flag(false){};
};



class MAC
{
	public:
		/*struct UserInfo
		{
			unsigned short 	user_addr;
			unsigned long	last_time;

			ThreadControl	daemon_ctl;
		};*/

		enum class Mode:unsigned char
		{
			gateway = 0,
			terminal = 1,
			netless = 2,
		};

	public:
		MAC(function<void(vector<unsigned char>&)>);
		~MAC();

		void Send(vector<unsigned char>& );
		void Recv(vector<unsigned char>& );

		void ChangeChannelUser(unsigned short );
		void NetRequest(void);
		void SwitchMode(Mode );
		
		void ManualNetwork(void);

		//void SpeedTestClient(unsigned ,LoRaConfig& );
		void SpeedTestServer(void);

	private:
		void PushUp(void);
		void PutDown(void);

		void PeriodControl(void);
		void GatewayRun(void);
		void TerminalRun(void);
		//void NetMaster(void);
		//void NetSlave(void);

		void NetController_gateway(void);
		void NetController_terminal(void);
		void NetController_netless(void);

		void HeartBeat(void);
		void NetworkInfo(void);
		void InviteToNetwork(void);
		void InviteAcknowledgement(void);
		void UserQuery(unsigned short );

		void ReciveNotify(void);
		bool ReciveListen(void);

		void SpeedTest_Server(void);
		void SpeedTest_Client(int ,unsigned short );

	//private:
	public:
		unsigned short mac_addr;

		/*vector<UserInfo> user_list;
		mutex user_list_lock;
		condition_variable user_list_ring;
		thread::id user_list_ring_id;*/

		UserList user_list;

		unsigned user_seq;

		//chrono::monotonic_clock::time_point time_stamp;

		//chrono::milliseconds com_period;

		//unsigned time_stamp;

		//unsigned com_period;

		deque<Frame> 		in_buffer;
		mutex			in_buffer_lock;
		condition_variable	in_buffer_ring;

		deque<Frame>		out_buffer;
		mutex			out_buffer_lock;
		condition_variable	out_buffer_ring;
		bool urgent_send_flag;
		
		deque<Frame>		net_buffer;
		mutex			net_buffer_lock;
		condition_variable	net_buffer_ring;
		unsigned		net_buffer_head_pos;

		deque<Frame>		speed_buffer;
		mutex			speed_buffer_lock;
		condition_variable	speed_buffer_ring;

		thread			push_up_daemon;
		thread			put_down_daemon;
		bool			run_flag;

		Mode			mode;
		mutex			node_type_lock1;
		condition_variable	node_type_ring;

		unsigned short		channel_user;
		mutex			channel_user_lock;
		condition_variable	channel_user_ring;


		condition_variable	recive_ring;
		
		bool			local_nrq_flag;
		mutex			local_nrq_flag_lock;
		condition_variable	local_nrq_flag_ring;

		mutex			net_daemon_lock;
		condition_variable	net_daemon_ring;

		thread			net_gateway_daemon;
		ThreadCtl1		net_gateway_daemon_ctl;
		thread			net_terminal_daemon;
		ThreadCtl2		net_terminal_daemon_ctl;
		thread			net_netless_daemon;
		ThreadCtl3		net_netless_daemon_ctl;

		//The entrance of the physical layer.
		LoRa employee;

		//The entrance of the application layer.
		function<void(vector<unsigned char>&)> employer;
};



void UserList::UserInfo::Print(void)
{
	cout << "\tUserInfo" << endl;
	cout << "\t{" << endl;
	cout << "\t\tuser_addr = " << this->user_addr << endl;
	cout << "\t\tlast_time = ";
	show_time(this->last_time);
	cout << "\t}" << endl;
}


#endif

