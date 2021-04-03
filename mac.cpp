#include <iostream>
	#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "mac.hpp"
#include "global.hpp"
#include "lora.hpp"
#include "frame.hpp"
#include "utils.hpp"

using namespace std;

extern Config global_config;

extern ofstream global_log;

MAC::MAC(function<void(vector<unsigned char>&)> recv_callback):
	mode(Mode::netless),
	mac_addr(global_config.mac_addr),
	user_list(this->mac_addr,*this),
	run_flag(true),
	push_up_daemon(&MAC::PushUp,this),
	put_down_daemon(&MAC::PutDown,this),
	/*net_gateway_daemon(&MAC::NetController_gateway,this),
	net_terminal_daemon(&MAC::NetController_terminal,this),
	net_netless_daemon(&MAC::NetController_netless,this),*/
	channel_user(0xffff),
	employee(bind(&MAC::Recv,this,placeholders::_1)),
	employer(recv_callback)
{
	//
	LoRaConfig config;
	config.local_addr = this->mac_addr;
	this->employee.ChangeMode(LoRaTypes::Mode::sleep);
	this->employee.SetConfig(config,LoRaTypes::SetConfigTo::flash);
	this->employee.ChangeMode(LoRaTypes::Mode::normal);
	
	//Status and return.
	global_log << "[STATUS][MAC] " << "MAC layer initialized." << endl;
}



MAC::~MAC()
{
	//Shut daemon threads.
	this->run_flag = false;
	this->in_buffer_ring.notify_all();
	this->out_buffer_ring.notify_all();
	this->push_up_daemon.join();
	this->put_down_daemon.join();

	/*this->net_gateway_daemon_ctl.run_flag = false;
	this->net_terminal_daemon_ctl.run_flag = false;
	this->net_netless_daemon_ctl.run_flag = false;
	this->net_daemon_ring.notify_all();
	this->net_gateway_daemon.join();
	this->net_terminal_daemon.join();
	this->net_netless_daemon.join();*/

	//Status and return.
	global_log << "[STATUS][MAC] " << "MAC layer closed." << endl;
}



void MAC::Send(vector<unsigned char>& to_send)
{
	//
	Frame frame;
	frame.data = to_send;
	frame.src_addr = this->mac_addr;
	frame.frame_flag = 0;

	//
	this->out_buffer_lock.lock();
	this->urgent_send_flag = true;
	this->out_buffer.push_back(frame);
	this->out_buffer_lock.unlock();
	this->out_buffer_ring.notify_all();
}



void MAC::Recv(vector<unsigned char>& recived)
{
	//
	this->ReciveNotify();

	//
	static deque<unsigned char> frame_fragment;
	frame_fragment.insert(frame_fragment.end(),recived.begin(),recived.end());
	unsigned total = 0;

	for(bool flag = true;frame_fragment.size() > 0 && flag;)
	{
		if(frame_fragment.front() == '\x81')
		{
			for(	deque<unsigned char>::iterator it = frame_fragment.begin() + 1	;
				it != frame_fragment.end()					;
				it++								)
			{
				//
				if(*it == '\xA5')
				{
					//
					this->ReciveNotify();
	
					//Frame dispatch.
					vector<unsigned char> temp(frame_fragment.begin(),it + 1);
					Frame frame(temp);
					if(frame.itn_flag)
					{
						this->net_buffer_lock.lock();
						this->net_buffer.push_back(frame);
						this->net_buffer_lock.unlock();
						this->net_daemon_ring.notify_all();
					}
					else
					{
						this->in_buffer_lock.lock();
						this->in_buffer.push_back(frame);
						this->in_buffer_lock.unlock();
					}

					//Update user list.
					if(this->user_list.Find(frame.src_addr) != -1)
					{
						this->user_list.Update(frame.src_addr,frame.time_stamp);
					}

					//
					total++;
	
					//
					global_log << "[STATUS][MAC] " << "Recive a frame." << endl;
					frame.Print();
					
					frame_fragment.erase(frame_fragment.begin(),it + 1);
					break;
				}
				else if(*it == '\x81')
				{
					frame_fragment.erase(frame_fragment.begin(),it);
					break;
				}
				else if(*it == '\x99' && it != frame_fragment.end() - 1)
				{
					it++;
				}

				//
				if(it == frame_fragment.end() - 1)
				{
					flag = false;
				}
			}
		}
		else if(frame_fragment.front() == '\x99')
		{
			frame_fragment.pop_front();
			frame_fragment.pop_front();
		}
		else
		{
			frame_fragment.pop_front();
		}
	}

	/*static vector<unsigned char> frame_fragment;
	vector<unsigned char>::iterator frame_begin = recived.end(),frame_end = recived.end();
	for(vector<unsigned char>::iterator it = recived.begin();it != recived.end();it++)
	{
		if(*it == '\x81')
		{
			frame_begin = it;
			frame_fragment.clear();
		}
		else if(*it == '\xA5')
		{
			if(frame_begin != recived.end())
			{
				//
				this->ReciveNotify();

				//
				vector<unsigned char> temp(frame_begin,it + 1);
				Frame frame(temp);
				this->in_buffer_lock.lock();
				this->in_buffer.push_back(frame);
				this->in_buffer_lock.unlock();
				frame_begin = recived.end();

				//
				total++;

				//
				global_log << "[STATUS][MAC] " << "Recive a frame." << endl;
				frame.Print();
			}
			else if(frame_fragment.size() > 0)
			{
				//
				this->ReciveNotify();

				frame_fragment.insert(frame_fragment.end(),recived.begin(),it + 1);
				Frame frame(frame_fragment);
				this->in_buffer_lock.lock();
				this->in_buffer.push_back(frame);
				this->in_buffer_lock.unlock();
				frame_fragment.clear();

				//
				total++;

				//
				global_log << "[STATUS][MAC] " << "Recive a frame." << endl;
				frame.Print();
			}
		}
		else
		{
			if(it == recived.end() - 1)
			{
				if(frame_begin != recived.end())
				{
					frame_fragment.assign(frame_begin,recived.end());
				}
				else if(frame_fragment.size() > 0)
				{
					frame_fragment.insert(frame_fragment.end(),recived.begin(),recived.end());
				}
			}
		}
	}*/

	//Status and return.
	global_log << "[STATUS][MAC] " << "Recive " << total << " frames in total." << endl;
	this->in_buffer_ring.notify_all();
	this->ReciveNotify();
}



void MAC::PushUp(void)
{
	global_log << "[STATUS][MAC] " << "Thread \"push_up_daemon\" start." << endl;

	//Definition.
	unique_lock<mutex> lck(this->in_buffer_lock);

	//Run daemon thread.
	while(this->run_flag)
	{
		//Wait for notification.
		this->in_buffer_ring.wait(lck);

		while(this->in_buffer.size() > 0)
		{
			this->employer(this->in_buffer.front().data);

			this->in_buffer.pop_front();
		}
	}

	global_log << "[STATUS][MAC] " << "Thread \"push_up_daemon\" stop." << endl;
}



void MAC::PutDown(void)
{
	global_log << "[STATUS][MAC] " << "Thread \"put_down_daemon\" start." << endl;

	//Definition.
	static vector<unsigned char> frame_fragment;
	unique_lock<mutex> lck(this->out_buffer_lock);

	//Run daemon thread.
	while(this->run_flag)
	{
		//Wait for notification.
		this->out_buffer_ring.wait(lck);

		//
		unsigned total = 0;
		if(this->urgent_send_flag)
		{
			//
			while(this->out_buffer.size() > 0)
			{
				//
				global_log << "[STATUS][MAC] " << "Send a frame:" << endl;
				this->out_buffer.front().Print();
	
				//
				while(this->ReciveListen())
				{
					unsigned delay_time = random(10) * 10;
					this_thread::sleep_for(chrono::milliseconds(delay_time));

					global_log << "[STATUS][MAC] " << "Delay " << delay_time << "ms." << endl;
				}
				this->out_buffer.front().Encapsulate();
				this->employee.Send(this->out_buffer.front().data);

				//Update user list.
				if(this->user_list.Find(this->out_buffer.front().src_addr) != -1)
				{
					this->user_list.Update(	this->out_buffer.front().src_addr	,
								this->out_buffer.front().time_stamp	);
				}

				//
				this->out_buffer.pop_front();
				total++;
			}

			this->urgent_send_flag = false;
		}
		else
		{
		}
		
		//Status and return.
		global_log << "[STATUS][MAC] " << "Send " << total << " frames in totals." << endl;
	}

	global_log << "[STATUS][MAC] " << "Thread \"put_down_daemon\" stop." << endl;
}



void MAC::PeriodControl(void)
{
	global_log << "[STATUS][MAC] " << "Thread \"period_control_daemon\" start." << endl;

	global_log << "[STATUS][MAC] " << "Thread \"period_control_daemon\" stop." << endl;
}



/*void MAC::ChannelControl(void)
{
	global_log << "[STATUS][MAC] " << "Thread \"channel_control_daemon\" start." << endl;

	unique_lock<mutex> lck(this->in_buffer_ring);
	while(this->run_flag)
	{
		//
		this->chan_crq_ring.wait(lck);

		while(remote_crq_count || local_crq_flag)
		{
			if(this->node_type == MAC::NodeTypes::gateway)
			{
				//
				if(this->local_crq_flag)
				{
					//
					Frame frame;
					frame.src_addr = this->mac_addr;
					frame.crq_flag = true;
					frame.ack_flag = true;
					frame.data[0] = (channel_user >> 8);
					frame.data[1] = (channel_user & 0xFF);
			
					//
					this->out_buffer_lock.lock();
					this->urgent_send_flag = true;
					this->out_buffer.push_front(frame);
					this->out_buffer_lock.unlock();
					this->out_buffer_ring.notify_all();

					//
					this->channel_user_lock.lock();
					this->channel_user == this->mac_addr;
					this->channel_user_lock.unlock();

					//
					this->local_crq_flag == false;
					this->out_buffer_ring.notify_all();

					//
					mutex lock;
					unique_lock<mutex> lck(lock);
					function pred = [this]()->bool
					{
						return this->local_fin_flag;
					}
					this->crq_fin_ring.wait(lck,pred);
					this->local_fin_flag = false;

					//
					Frame frame2;
					frame2.src_addr = this->mac_addr;
					frame2.crq_flag = true;
					frame2.ack_flag = true;
					frame2.data[0] = 0xff;
					frame2.data[1] = 0xff;

					//
					this->out_buffer_lock.lock();
					this->urgent_send_flag = true;
					this->out_buffer.push_front(frame);
					this->out_buffer_lock.unlock();
					this->out_buffer_ring.notify_all();

					//
					this->channel_user_lock.lock();
					this->channel_user == 0xffff;
					this->channel_user_lock.unlock();
				}

				//
				if(this->remote_crq_count > 0)
				{
					//
					this->in_buffer_lock.lock();

					//
					deque<unsigned char>::iterator it;
					for(it = this->in_buffer.begin();it != this->in_buffer.end();it++)
					{
						if(*it.crq_flag)
						{
							break;
						}
					}

					//
					if(*(now - *it.time_stamp) < time_out*true)
					{
						this->ChangeChannelUser(*it.src_addr);
						this->out_buffer_ring.notify_all();
					}

					//
					this->in_buffer.erase(it);
					this->remote_crq_count--;

					//
					this->in_buffer_lock.lock();

					//
					unique_lock<mutex> lck(this->crq_fin_lock);
					this->crq_fin_ring.wait(lck);
					this->ChangeChannelUser(0xffff);
				}
			}
			else
			{
				//
				if(this->remote_crq_count > 0)
				{
					//
					this->in_buffer_lock.lock();

					//
					deque<unsigned char>::iterator it;
					for(it = this->in_buffer.begin();it != this->in_buffer.end();it++)
					{
						if(*it.crq_flag)
						{
							break;
						}
					}

					//
					if(*it.crq_flag && *it.ack_flag)
					{
						unsigned short channel_user = *it.data.at(0);
						channel_user <<= 8;
						channel_user |= *it.data.at(1);
						this->ChangeChannelUser(channel_user);
					}

					//
					this->in_buffer.erase(it);
					this->remote_crq_count--;

					//
					this->in_buffer_lock.unlock();

					//
					unique_lock<mutex> lck(this->crq_fin_lock);
					this->crq_fin_ring.wait(lck);
					this->ChangeChannelUser(0xffff);
				}

				//
				if(this->local_crq_flag)
				{
					//
					this->ChangeChannelUser(this->mac_addr);
					this->local_crq_flag == false;
					this->out_buffer_ring.notify_all();

					//
					unique_lock<mutex> lck(this->crq_fin_lock);
					this->crq_fin_ring.wait(lck);
					this->ChangeChannelUser(0xffff);
				}
			}
		}
	}

	global_log << "[STATUS][MAC] " << "Thread \"channel_control_daemon\" stop." << endl;
}



void MAC::GatewayRun(void)
{
	global_log << "[STATUS][MAC] " << "Thread \"gateway_daemon\" start." << endl;

	unique_lock lck(this->in_buffer_ring);
	while(this->run_flag)
	{
		this->node_type_lock.lock();

		if(this->node_type == NodeTypes::gateway)
		{
			global_log << "[STATUS][MAC] " << "Run as a gateway." << endl;
			
			//
			deque<unsigned char>::iterator it;
			function pred1 = [this,&it]->bool
			{
				for(it = this->in_buffer.begin();it != this->in_buffer.end();it++)
				{
					if(*it.crq_flag || *it.ack_flag)
					{
						return true;
					}
				}
				return false;
			}
			this->in_buffer_ring.wait(lck,pred1);

			//
			if(*it.crq_flag && channel_user == 0xffff)
			{
				this->ChangeChannelUser(*it.src_addr);
			}
		}

		this->node_type_lock.unlock();
	}

	global_log << "[STATUS][MAC] " << "Thread \"gateway_control_daemon\" stop." << endl;
}



void MAC::TerminalRun(void)
{
	global_log << "[STATUS][MAC] " << "Thread \"terminal_daemon\" start." << endl;

	global_log << "[STATUS][MAC] " << "Thread \"terminal_control_daemon\" stop." << endl;
}



void MAC::ChangeChannelUser(unsigned short channel_user)
{
	this->channel_user_lock.lock();

	//
	if(this->node_type == MAC::NodeTypes::gateway)
	{
	}
	else
	{
		//
		Frame frame;
		frame.src_addr = this->mac_addr;
		frame.crq_flag = true;

		//
		this->out_buffer_lock.lock();
		this->urgent_send_flag = true;
		this->out_buffer.push_front(frame);
		this->out_buffer_lock.unlock();
		this->out_buffer_ring.notify_all();

		//
		mutex lock;
		unique_lock<mutex> lck(lock);
		this->crq_ack_ring.wait_for(lck,chrono::seconds(1));
	}

	this->channel_user_lock.unlock();
}*/



void MAC::ReciveNotify(void)
{
	this->recive_ring.notify_all();
}



bool MAC::ReciveListen(void)
{
	mutex lock;
	unique_lock<mutex> lck(lock);

	//
	return (this->recive_ring.wait_for(lck,chrono::milliseconds(100)) == cv_status::no_timeout);
}



void MAC::NetController_gateway(void)
{
	//Thread management.
	ThreadCtl1& ctl = this->net_gateway_daemon_ctl;
	ctl.id = this_thread::get_id();

	//
	global_log << "[STATUS][MAC] " << "Thread \"net_gateway_daemon\" start." << endl;

	unique_lock<mutex> lck(this->net_daemon_lock);
	function<bool(void)> pred = [this,&ctl]()->bool
	{
		if(!ctl.run_flag)
		{
			return true;
		}
		if(ctl.pause_flag)
		{
			return false;
		}
		if(this->mode != Mode::gateway)
		{
			return false;
		}
 		if(ctl.manual_network_flag)
		{
			return true;
		}
		else if(this->net_buffer.size() > 0)
		{
			return true;
		}
		else if(ctl.heart_beat_flag)
		{
			return true;
		}
		else
		{
			return false;
		}
	};

	while(ctl.run_flag)
	{
		//ctl.ring.wait(lck,pred);
		this->net_daemon_ring.wait(lck,pred);

		//Maunal network.
		if(ctl.manual_network_flag)
		{
			//
			this->InviteToNetwork();

			global_log << "[STATUS][MAC] " << "Waiting for respond..." << endl;
			//ctl.ring.wait_for(lck,?);
			this_thread::sleep_for(chrono::seconds(5));

			ctl.heart_beat_flag = true;

			//
			ctl.manual_network_flag = false;
		}

		//Deal with frames.
		this->net_buffer_lock.lock();

		while(this->net_buffer.size() > 0)
		{
			if(this->net_buffer.front().itn_flag && this->net_buffer.front().ack_flag)
			{
				this->user_list.New(	this->net_buffer.front().src_addr	,
							this->net_buffer.front().time_stamp	);

				global_log << "[STATUS][MAC] " << "Got a new network entering acknowledgement." << endl;

				this->net_buffer.pop_front();
			}
			else
			{
				global_log << "[STATUS][MAC] " << "Drop an invalid frame." << endl;

				this->net_buffer.pop_front();
			}
		}

		this->net_buffer_lock.unlock();
this->user_list.Print();

		//Heart beat.
		if(ctl.heart_beat_flag)
		{
			//
			this->NetworkInfo();
			
			//
			ctl.heart_beat_flag = false;
		}

		//User time out.
		if(ctl.user_timeout_flag)
		{
			for(int i = 1;i <= 3;i++)
			{
				this->UserQuery(ctl.timeout_user);
				global_log << "[STATUS][MAC] ";
				global_log << "User 0x" << ctl.timeout_user << " lost, querying..." << i << endl;
				this_thread::sleep_for(chrono::seconds(1));

				vector<UserList::UserInfo> user_info = this->user_list.Info(ctl.timeout_user);
				if(user_info.at(0).last_time - time(NULL) < 20)
				{
					global_log << "[STATUS][MAC] ";
					global_log << "User 0x" << ctl.timeout_user << " reconnected." << endl;

					break;
				}
				if(i == 3)
				{
					global_log << "[STATUS][MAC] ";
					global_log << "No respond for querying,user 0x" << ctl.timeout_user;
					global_log << " lost" << endl;
					this->user_list.Delete(ctl.timeout_user);
				}
			}

			ctl.user_timeout_flag = false;
		}

		//
		vector<UserList::UserInfo> user_info = this->user_list.Info();
		//vector<UserList::UserInfo>&& user_info = move(this->user_list.Info());
		if(user_info.size() == 1)
		{
			if(user_info.at(0).user_addr == this->mac_addr)
			{
				this->user_list.Clear();
				this->SwitchMode(Mode::netless);
			}
			else
			{
				throw;
			}
		}
	}

	global_log << "[STATUS][MAC] " << "Thread \"net_gateway_daemon\" stop." << endl;
}



void MAC::NetController_terminal(void)
{
	//Thread management.
	ThreadCtl2& ctl = this->net_terminal_daemon_ctl;
	ctl.id = this_thread::get_id();

	//
	global_log << "[STATUS][MAC] " << "Thread \"net_terminal_daemon\" start." << endl;

	unique_lock<mutex> lck(this->net_daemon_lock);
	function<bool(void)> pred = [this,&ctl]()->bool
	{
		if(!ctl.run_flag)
		{
			return true;
		}
		if(ctl.pause_flag)
		{
			return false;
		}
		if(this->mode != Mode::terminal)
		{
			return false;
		}
		if(this->net_buffer.size() > 0)
		{
			return true;
		}
		else if(ctl.heart_beat_flag)
		{
			return true;
		}
		else
		{
			return false;
		}
	};

	//
	while(ctl.run_flag)
	{
		//
		this->net_daemon_ring.wait(lck,pred);

		//
		this->net_buffer_lock.lock();

		while(this->net_buffer.size() > 0)
		{
			if(	this->net_buffer.front().itn_flag	&& 
				!this->net_buffer.front().ack_flag	&&
				!this->net_buffer.front().fin_flag	&&
				this->net_buffer.front().syn_flag	)
			{
				//
				unsigned short gateway_addr = this->net_buffer.front().data.at(0);
				gateway_addr <<= 8;
				gateway_addr |= (this->net_buffer.front().data.at(1));
				if(gateway_addr == this->user_list.Info().at(0).user_addr)
				{
					//this->user_list.Clear();
					bool flag = false;
					for(int i = 0;i < this->net_buffer.front().data.size() / 2;i++)
					{
						unsigned short user_addr = this->net_buffer.front().data.at(i * 2);
						user_addr <<= 8;
						user_addr |= (this->net_buffer.front().data.at(i * 2 + 1));
						if(user_addr == this->mac_addr) flag = true;
					}
					if(flag)
					{
						global_log << "[STATUS][MAC] ";
						global_log << "Got a network synchronizing frame." << endl;

						//Time sync.
						timeval to_sync;
						to_sync.tv_sec = this->net_buffer.front().time_stamp;
						if(settimeofday(&to_sync,NULL) == -1)
						{
							global_log << "[WARNING][MAC] ";
							global_log << "Synchronizing time from gateway failed." << endl;
						}
						else
						{
							global_log << "[STATUS][MAC] ";
							global_log << "Synchronizing time from gateway: ";
							show_time(to_sync.tv_sec);
						}

						//
						for(int i = 0;i < this->net_buffer.front().data.size() / 2;i++)
						{
							unsigned short user_addr = this->net_buffer.front().data.at(i * 2);
							user_addr <<= 8;
							user_addr |= (this->net_buffer.front().data.at(i * 2 + 1));
							if(this->user_list.Find(user_addr) == -1)
							{
								this->user_list.New(user_addr,time(NULL));
							}
						}
						this->user_list.Print();
					}
					else
					{
						global_log << "[STATUS][MAC] ";
						global_log << "Got an invalid network synchronizing frame,";
						global_log << "local address not match." << endl;
					}
				}
				else
				{
					global_log << "[STATUS][MAC] ";
					global_log << "Got an invalid network synchronizing frame,";
					global_log << "gatway address not match." << endl;
				}

				this->net_buffer.pop_front();
			}
			else if(!this->net_buffer.front().itn_flag	&& 
				this->net_buffer.front().qry_flag	&&
				!this->net_buffer.front().ack_flag	&&
				!this->net_buffer.front().fin_flag	&&
				!this->net_buffer.front().syn_flag	)
			{
				unsigned short addr = this->net_buffer.front().data.at(0);
				addr <<= 8;
				addr |= (this->net_buffer.front().data.at(1));
				if(addr == this->mac_addr)
				{
					global_log << "[STATUS][MAC] ";
					global_log << "Got a querying frame and respond." << endl;

					this->HeartBeat();
				}
				else
				{
					global_log << "[STATUS][MAC] ";
					global_log << "Got an invalid querying frame,";
					global_log << "local address not match." << endl;
				}
			}
			else
			{
				global_log << "[STATUS][MAC] " << "Drop an invalid frame." << endl;

				this->net_buffer.pop_front();
			}
		}

		this->net_buffer_lock.unlock();

		//Heart beat.
		if(ctl.heart_beat_flag)
		{
			//
			this->HeartBeat();
			
			//
			ctl.heart_beat_flag = false;
		}

		//Gateway lost.
		if(ctl.gateway_timeout_flag)
		{
			this->user_list.Clear();

			global_log << "[STATUS][MAC] " << "Gateway lost." << endl;

			ctl.gateway_timeout_flag = false;
		}

		//Exit condition.
		vector<UserList::UserInfo> user_info = this->user_list.Info();
		//vector<UserList::UserInfo>&& user_info = move(this->user_list.Info());
		if(user_info.size() == 0)
		{
			this->user_list.Clear();
			this->SwitchMode(Mode::netless);
		}
	}

	global_log << "[STATUS][MAC] " << "Thread \"net_terminal_daemon\" stop." << endl;
}



void MAC::NetController_netless(void)
{
	//Thread management.
	ThreadCtl3& ctl = this->net_netless_daemon_ctl;
	ctl.id = this_thread::get_id();

	//
	global_log << "[STATUS][MAC] " << "Thread \"net_netless_daemon\" start." << endl;

	unique_lock<mutex> lck(this->net_daemon_lock);
	function<bool(void)> pred = [this,&ctl]()->bool
	{
		if(!ctl.run_flag)
		{
			return true;
		}
		if(ctl.pause_flag)
		{
			return false;
		}
		if(this->mode != Mode::netless)
		{
			return false;
		}
		if(this->net_buffer.size() > 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	};

	//
	while(ctl.run_flag)
	{
		//
		this->net_daemon_ring.wait(lck,pred);

		//
		while(this->net_buffer.size() > 0)
		{
			if(this->net_buffer.front().itn_flag && !this->net_buffer.front().ack_flag)
			{
				//
				this->SwitchMode(Mode::terminal);
				this->user_list.New(	this->net_buffer.front().src_addr	,
							this->net_buffer.front().time_stamp	);
				this->user_list.New(this->mac_addr,time(NULL));

				this->InviteAcknowledgement();

				this->net_buffer.pop_front();
			}
			else
			{
				global_log << "[STATUS][MAC] " << "Drop an invalid frame." << endl;

				this->net_buffer.pop_front();
			}
		}
this->user_list.Print();
	}

	global_log << "[STATUS][MAC] " << "Thread \"net_netless_daemon\" stop." << endl;
}



void MAC::HeartBeat(void)
{
	Frame frame;
	frame.src_addr = this->mac_addr;

	this->out_buffer_lock.lock();
	this->urgent_send_flag = true;
	this->out_buffer.push_front(frame);
	this->out_buffer_lock.unlock();
	this->out_buffer_ring.notify_all();
}



void MAC::NetworkInfo(void)
{
	Frame frame;
	frame.src_addr = this->mac_addr;
	frame.itn_flag = true;
	frame.syn_flag = true;

	vector<UserList::UserInfo> net_user = this->user_list.Info();
	for(int i = 0;i < net_user.size();i++)
	{
		frame.data.push_back(net_user.at(i).user_addr >> 8);
		frame.data.push_back(net_user.at(i).user_addr & 0xff);
	}

	this->out_buffer_lock.lock();
	this->urgent_send_flag = true;
	this->out_buffer.push_front(frame);
	this->out_buffer_lock.unlock();
	this->out_buffer_ring.notify_all();
}



void MAC::ManualNetwork(void)
{
	if(this->mode != Mode::gateway)
	{
		this->SwitchMode(Mode::gateway);
		this->user_list.New(this->mac_addr,0);
	}
	this->net_gateway_daemon_ctl.manual_network_flag = true;
	this->net_daemon_ring.notify_all();
}



void MAC::InviteToNetwork(void)
{
	//
	Frame net_request;
	net_request.src_addr = this->mac_addr;
	net_request.itn_flag = true;

	this->out_buffer_lock.lock();
	this->urgent_send_flag = true;
	this->out_buffer.push_front(net_request);
	this->out_buffer_lock.unlock();
	this->out_buffer_ring.notify_all();

	//Status & return.
	global_log << "[STATUS][MAC] " << "Send \"ITN = 1\" signal." << endl;
}



void MAC::InviteAcknowledgement(void)
{
	//Acknowledge an invitation.
	Frame net_ack;
	net_ack.src_addr = this->mac_addr;
	net_ack.itn_flag = true;
	net_ack.ack_flag = true;

	this->out_buffer_lock.lock();
	this->urgent_send_flag = true;
	this->out_buffer.push_front(net_ack);
	this->out_buffer_lock.unlock();
	this->out_buffer_ring.notify_all();
}



void MAC::UserQuery(unsigned short user_addr)
{
	Frame frame;
	frame.src_addr = this->mac_addr;
	frame.qry_flag = true;
	frame.data.push_back(user_addr >> 8);
	frame.data.push_back(user_addr & 0xff);

	this->out_buffer_lock.lock();
	this->urgent_send_flag = true;
	this->out_buffer.push_front(frame);
	this->out_buffer_lock.unlock();
	this->out_buffer_ring.notify_all();
}



void MAC::SwitchMode(MAC::Mode to_switch)
{

	if(this->mode == to_switch)
	{
		global_log << "[STATUS][MAC] " << "It's currently in mode " << (int)to_switch;
		global_log << ",no need to switch." << endl;
	}
	else
	{
		this->mode = to_switch;
		
		global_log << "[STATUS][MAC] " << "Switched into mode " << (int)to_switch << endl;
	}
	//this->net_daemon_ring.notify_all();
}



UserList::UserList(unsigned short local_addr,MAC& interface):local_addr(local_addr),interface(interface)
{
	;
}



UserList::~UserList()
{
	this->Clear();
}



void UserList::New(unsigned short user_addr,unsigned long time_stamp)
{
	//Check if exist.
	if(this->Find(user_addr) != -1)
	{
		global_log << "[WARNING][MAC] " << "User to new exist." << endl;
		return ;
	}

	//Create a new user daemon.
	unique_lock<mutex> lck(this->lock);

	UserInfo new_user(user_addr,time_stamp);
	thread user_daemon(&UserList::UserManager,this);
	new_user.daemon = user_daemon.get_id();

	this->user_list.push_back(new_user);

	//Status & return.
	global_log << "[STATUS][MAC] " << "User enter:" << endl;
	new_user.Print();
	lck.unlock();

	user_daemon.detach();
}



void UserList::Delete(unsigned short user_addr)
{
	//
	unique_lock<mutex> lck1(this->lock);

	int user_seq = this->Find(user_addr);
	if(user_seq >= 0)
	{
		unique_lock<mutex> lck2(this->daemon_pool_lock);

		for(	deque<ThreadCtl*>::iterator it = this->daemon_pool.begin()	;
			it != this->daemon_pool.end()					;
			it++								)
		{
			if((*it)->id == this->user_list.at(user_seq).daemon)
			{
				//Status.
				global_log << "[STATUS][MAC] " << "User exit:" << endl;
				this->user_list.at(user_seq).Print();
				this->user_list.erase(this->user_list.begin() + user_seq);
				lck1.unlock();

				//Stop user daemon.
				(*it)->run_flag = false;
				(*it)->ring.notify_one();
				lck2.unlock();

				//Return.
				return ;
			}
		}
	}
	else
	{
		//If user to delete not exist.
		global_log << "[WARNING][MAC] " << "User to delete not exist." << endl;
	}
}



void UserList::Update(unsigned short user_addr,unsigned long time_stamp)
{
	//Update time stamp.
	unique_lock<mutex> lck1(this->lock);

	int user_seq = this->Find(user_addr);
	if(user_seq >= 0)
	{
		unique_lock<mutex> lck2(this->daemon_pool_lock);

		for(	deque<ThreadCtl*>::iterator it = this->daemon_pool.begin()	;
			it != this->daemon_pool.end()					;
			it++								)
		{
			if((*it)->id == this->user_list.at(user_seq).daemon)
			{
				//Status.
				global_log << "[STATUS][MAC] " << "User infomation update:" << endl;
				this->user_list.at(user_seq).last_time = time_stamp;
				this->user_list.at(user_seq).Print();
				lck1.unlock();

				//Reset user timer.
				(*it)->reset_flag = true;
				(*it)->ring.notify_one();
				lck2.unlock();

				//Return.
				return ;
			}
		}
	}
	else
	{
		//If user to delete not exist.
		global_log << "[WARNING][MAC] " << "User to update not exist." << endl;
	}
}



int UserList::Find(unsigned short user_addr)
{
	for(int i = 0;i < this->user_list.size();i++)
	{
		if(this->user_list.at(i).user_addr == user_addr)
		{
			return i;
		}
	}
	return -1;
}



void UserList::Print(void)
{
	unique_lock<mutex> lck(this->lock);

	cout << "\tUserList" << endl;
	cout << "\t{" << endl;
	cout << "\t\tAddress" << "\tLast talk" << endl;
	for(	deque<UserInfo>::iterator it = this->user_list.begin()	;
		it != this->user_list.end()				;
		it++							)
	{
		cout << "\t\t0x" << (*it).user_addr << "\t";
		show_time((*it).last_time);
	}
	cout << "\t}" << endl;
}



void UserList::Clear(void)
{
	//
	for(	deque<ThreadCtl*>::iterator it = this->daemon_pool.begin()	;
		it != this->daemon_pool.end()					;
		it++								)
	{
		//(*it)->lock.lock();
		(*it)->run_flag = false;
		//(*it)->lock.unlock();
		(*it)->ring.notify_one();
		//this_thread::yield();
	}
	this_thread::sleep_for(chrono::milliseconds(100));

	unique_lock<mutex> lck1(this->daemon_pool_lock);
	unique_lock<mutex> lck2(this->lock);

	this->daemon_pool.clear();
	this->user_list.clear();
}



vector<UserList::UserInfo> UserList::Info(unsigned short user_addr)
{
	unique_lock<mutex> lck(this->lock);

	if(user_addr == 0)
	{
		vector<UserInfo> user_info(this->user_list.begin(),this->user_list.end());
		return user_info;
	}
	else
	{
		if(this->Find(user_addr) == -1)
		{
			global_log << "[ERROR][MAC] " << "User not exist." << endl;
			throw ;
		}

		for(	deque<UserInfo>::iterator it = this->user_list.begin()	;
			it != this->user_list.end()				;
			it++							)
		{
			if((*it).user_addr == user_addr)
			{
				vector<UserInfo> user_info(it,it + 1);
				return user_info;
			}
		}
	}
}



void UserList::UserManager(void)
{
	//Create control interface.
	unique_lock<mutex> lck1(this->daemon_pool_lock);

	this->daemon_pool.push_back(new ThreadCtl);
	this->daemon_pool.back()->id = this_thread::get_id();

	function<unsigned(void)> thread_seq = [this]()->unsigned
	{
		for(unsigned i = 0;i < this->daemon_pool.size();i++)
		{
			if(this->daemon_pool.at(i)->id == this_thread::get_id())
			{
				return i;
			}
		}
	};

	//lck1.unlock();
	
	//
	function<bool(void)> pred = [this,&thread_seq]()->bool
	{
		if(!this->daemon_pool.at(thread_seq())->run_flag)
		{
			return true;
		}
		if(this->daemon_pool.at(thread_seq())->pause_flag)
		{
			return false;
		}
		if(this->daemon_pool.at(thread_seq())->reset_flag)
		{
			return true;
		}
	};

	//	
	while(this->daemon_pool.at(thread_seq())->run_flag)
	{
		//
		int period = 0;
		if(this->user_list.at(thread_seq()).user_addr == this->local_addr)
		{
			period = 20 - ((time(NULL) - this->user_list.at(thread_seq()).last_time) % 20);
		}
		else
		{
			period = 30 - ((time(NULL) - this->user_list.at(thread_seq()).last_time) % 30);
		}

		//Wait for notification.
		bool timeout_flag = this->daemon_pool.at(thread_seq())->ring.wait_for(lck1,chrono::seconds(period),pred);

		//Thread stop.
		if(!this->daemon_pool.at(thread_seq())->run_flag)
		{
			break;
		}
		//Thread pause.
		else if(this->daemon_pool.at(thread_seq())->pause_flag)
		{
			continue;
		}
		//Reset timer.
		else if(this->daemon_pool.at(thread_seq())->reset_flag)
		{
			this->daemon_pool.at(thread_seq())->reset_flag = false;
		}
		//Action of timing.
		else if(!timeout_flag)
		{
			global_log << "[STATUS][MAC] " << "Timing action at ";
			show_time(time(NULL));

			this->lock.lock();
			if(this->user_list.at(thread_seq()).user_addr == this->local_addr)
			{
				switch(this->interface.mode)
				{
					case MAC::Mode::gateway:
					{
						this->interface.net_gateway_daemon_ctl.heart_beat_flag = true;
						this->interface.net_daemon_ring.notify_all();

						break;
					}
					case MAC::Mode::terminal:
					{
						this->interface.net_terminal_daemon_ctl.heart_beat_flag = true;
						this->interface.net_daemon_ring.notify_all();

						break;
					}
					case MAC::Mode::netless:
					{
						this->interface.net_netless_daemon_ctl.heart_beat_flag = true;
						this->interface.net_daemon_ring.notify_all();

						break;
					}
				}
			}
			else
			{
				switch(this->interface.mode)
				{
					case MAC::Mode::gateway:
					{
						this->interface.net_gateway_daemon_ctl.user_timeout_flag = true;
						this->interface.net_gateway_daemon_ctl.timeout_user = this->user_list.at(thread_seq()).user_addr;
						this->interface.net_daemon_ring.notify_all();

						break;
					}
					case MAC::Mode::terminal:
					{
						if(thread_seq() == 0)
						{
							this->interface.net_terminal_daemon_ctl.gateway_timeout_flag = true;
							this->interface.net_daemon_ring.notify_all();

							break;
						}
					}
				}
			}
			this->lock.unlock();
		}
	}

	//Remove control interface.
	lck1.unlock();
	lck1.lock();
	
	for(	deque<ThreadCtl*>::iterator it = this->daemon_pool.begin()	;
		it != this->daemon_pool.end()					;
		it++								)
	{
		if((*it)->id == this_thread::get_id())
		{
			delete (*it);
			this->daemon_pool.erase(it);

			break;
		}
	}

	lck1.unlock();
}



void MAC::SpeedTest_Server(void)
{
	unique_lock<mutex> lck(this->speed_buffer_lock);
	unsigned short speedtest_addr = 0x0;
	bool proc1_flag = false,proc2_flag = false,proc3_flag = false,proc4_flag = false,proc5_flag = false;
	int recv_frame_count = 0,bytes_count = 0,total_bytes_count = 0;
	function<Frame(int)> create_result_frame = [this,recv_frame_count,total_bytes_count](int proc)->Frame
	{
		Frame frame;
		time_t time_to_sync = time(NULL);
		for(int i = 0;i < sizeof(time_to_sync);i++)
		{
			frame.data.push_back((unsigned char)time_to_sync);
			time_to_sync >>= 8;
		}
		if(proc == 3 || proc == 4 || proc == 5)
		{
			for(int i = 0;i < sizeof(total_bytes_count);i++)
			{
				frame.data.push_back((unsigned char)total_bytes_count);
				time_to_sync >>= 8;
			}
			for(int i = 0;i < sizeof(recv_frame_count);i++)
			{
				frame.data.push_back((unsigned char)recv_frame_count);
				time_to_sync >>= 8;
			}
		}
		frame.src_addr = this->mac_addr;
		frame.spd_flag = true;
		frame.ack_flag = true;
		if(proc == 2)
		{
			frame.syn_flag = true;
		}
		else if(proc == 4 || proc == 5)
		{
			frame.fin_flag = true;
		}

		return frame;
	};

	while(this->run_flag)
	{
		proc1_flag = proc2_flag = proc3_flag = proc4_flag = proc5_flag = false;

		if(this->speed_buffer_ring.wait_for(lck,chrono::seconds(1)) == cv_status::no_timeout)
		{
			proc1_flag = true;
		}

		if(proc1_flag)
		{
			while(this->speed_buffer.size() > 0)
			{	
				if(	this->speed_buffer.front().syn_flag	&&
					!this->speed_buffer.front().ack_flag	&&
					!this->speed_buffer.front().fin_flag	&&
					!this->speed_buffer.front().qry_flag	)
				{
					speedtest_addr = this->speed_buffer.front().src_addr;
					recv_frame_count = bytes_count = total_bytes_count = 0;
					proc2_flag = true;
					this->speed_buffer.clear();
				}
				else
				{
					this->speed_buffer.pop_front();
				}
			}
		}

		//
		if(proc2_flag)
		{
			this->out_buffer_lock.lock();
			this->urgent_send_flag = true;
			this->out_buffer.push_back(create_result_frame(2));
			this->out_buffer_lock.unlock();
			this->out_buffer_ring.notify_all();

			proc3_flag = true;
		}
	
		if(proc3_flag)
		{
			while(this->speed_buffer_ring.wait_for(lck,chrono::seconds(5)) == cv_status::no_timeout)
			{
				while(this->speed_buffer.size() > 0)
				{
					if(	this->speed_buffer.front().src_addr != speedtest_addr	|| 
						this->speed_buffer.front().syn_flag 			|| 
						this->speed_buffer.front().ack_flag			||
						this->speed_buffer.front().qry_flag			)
					{
						this->speed_buffer.pop_front();
					}
					else if(this->speed_buffer.front().fin_flag)
					{
						this->speed_buffer.clear();
						goto on_exit_1;
					}
					else if(bytes_count >= 1000)
					{
						this->out_buffer_lock.lock();
						this->urgent_send_flag = true;
						this->out_buffer.push_back(create_result_frame(3));
						this->out_buffer_lock.unlock();
						this->out_buffer_ring.notify_all();
						
						bytes_count = 0;
					}
					else
					{
						recv_frame_count++;
						bytes_count += this->speed_buffer.front().data.size();
						total_bytes_count += this->speed_buffer.front().data.size();

						this->speed_buffer.pop_front();
					}
				}
			}

			on_exit_1:
			proc4_flag = true;
		}

		if(proc4_flag)
		{
			this->out_buffer_lock.lock();
			this->urgent_send_flag = true;
			this->out_buffer.push_back(create_result_frame(4));
			this->out_buffer_lock.unlock();
			this->out_buffer_ring.notify_all();

			proc5_flag = true;
		}

		if(proc5_flag)
		{
			while(this->speed_buffer_ring.wait_for(lck,chrono::seconds(1)) == cv_status::no_timeout)
			{
				while(this->speed_buffer.size() > 0)
				{
					if(	this->speed_buffer.front().src_addr == speedtest_addr	&&
						!this->speed_buffer.front().syn_flag 			&& 
						this->speed_buffer.front().fin_flag			&&
						this->speed_buffer.front().ack_flag			&&
						this->speed_buffer.front().qry_flag			)
					{
						this->out_buffer_lock.lock();
						this->urgent_send_flag = true;
						this->out_buffer.push_back(create_result_frame(5));
						this->out_buffer_lock.unlock();
						this->out_buffer_ring.notify_all();
					}

					this->speed_buffer.pop_front();
				}
			}
		}
	}
}



void MAC::SpeedTest_Client(int send_frame,unsigned short dst_addr)
{
	unique_lock<mutex> lck(this->speed_buffer_lock);
	int recv_frame = 0,recv_bytes = 0;
	time_t begin_time = 0,end_time = 0;
	double speed = 0,frame_lost = 0;
	bool proc3_flag = false,proc5_flag = false;
	function<void(Frame&)> result = [&](Frame& result_frame)
	{
		for(int j = 0,end_time = 0;j < sizeof(end_time);j++)
		{
			end_time |= ((time_t)result_frame.data.at(j) << (8 * j));
		}
		for(int j = 8,recv_bytes = 0;j < sizeof(recv_bytes);j++)
		{
			recv_bytes |= ((time_t)result_frame.data.at(j) << (8 * j));
		}
		for(int j = 0,recv_frame = 0;j < sizeof(recv_frame);j++)
		{
			recv_frame |= ((time_t)result_frame.data.at(j) << (8 * j));
		}

		//speed = recv_bytes / (double)(end_time - begin_time);
		//frame_lost = (double)recv_frame / send_frame;
	};

	for(int i = 0;i < 3 && !proc3_flag;i++)
	{
		//
		Frame frame;
		frame.src_addr = this->mac_addr;
		frame.spd_flag = true;
		frame.syn_flag = true;
	
		this->out_buffer_lock.lock();
		this->urgent_send_flag = true;
		this->out_buffer.push_back(frame);
		this->out_buffer_lock.unlock();
		this->out_buffer_ring.notify_all();

		if(this->speed_buffer_ring.wait_for(lck,chrono::seconds(1)) == cv_status::no_timeout)
		{
			while(this->speed_buffer.size() > 0)
			{
				if(	//this->speed_buffer.front().src_addr != dst_addr	&&
					this->speed_buffer.front().syn_flag		&&
					!this->speed_buffer.front().fin_flag		&&
					this->speed_buffer.front().ack_flag		&&
					!this->speed_buffer.front().qry_flag		)
				{
					dst_addr = this->speed_buffer.front().src_addr;	//Brocast mode only.
					timeval to_sync;
					to_sync.tv_sec = 0;
					for(int j = 0;j < sizeof(to_sync);j++)
					{
						to_sync.tv_sec |= ((time_t)this->speed_buffer.front().data.at(j) << (8 * j));
					}
					settimeofday(&to_sync,NULL);
	
					this->speed_buffer.clear();
					proc3_flag = true;
				}
				else
				{
					this->speed_buffer.pop_front();
				}
			}
		}
	}

	if(proc3_flag)
	{
		Frame frame;
		frame.src_addr = this->mac_addr;
		frame.spd_flag = true;
		for(int j = 0;j < 200;j++)
		{
			frame.data.push_back(0);
		}

		Frame fin_frame;
		fin_frame.src_addr = this->mac_addr;
		frame.spd_flag = true;
		frame.fin_flag = true;

		begin_time = time(NULL);
	
		this->out_buffer_lock.lock();
		this->urgent_send_flag = true;
		for(int i = 0;i < send_frame;i++)
		{
			this->out_buffer.push_back(frame);
		}
		this->out_buffer.push_back(fin_frame);
		this->out_buffer_lock.unlock();
		this->out_buffer_ring.notify_all();

		this_thread::yield();

		proc5_flag = true;
		if(this->speed_buffer_ring.wait_for(lck,chrono::seconds(5)) == cv_status::no_timeout)
		{
			while(this->speed_buffer.size() > 0)
			{
				if(	this->speed_buffer.front().src_addr = dst_addr	&&
					!this->speed_buffer.front().syn_flag		&&
					this->speed_buffer.front().ack_flag		&&
					!this->speed_buffer.front().qry_flag		)
				{
					result(this->speed_buffer.front());

					if(this->speed_buffer.front().fin_flag)
					{
						proc5_flag = false;
					}
				}

				this->speed_buffer.pop_front();
			}
		}
	}

	if(proc5_flag)
	{
		cout << "Speed test failed!";
	}
}

