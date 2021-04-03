#include <iostream>
#include <fstream>
/*#include <sstream>
#include <string>
#include <vector>
#include <deque>
#include <list>
#include <iterator>
#include <thread>
#include <mutex>
//#include <shared_mutex>
#include <condition_variable>
#include <utility>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "utils.hpp"
#include "mac.hpp"
//#include "test2.cpp"

using namespace std;

int res = 10;
mutex res_lock;
condition_variable res_ring;

//thread t1([](){unique_lock<mutex> lck(res_lock);res_ring.wait(lck,[&res](){return (res == 11);});cout << 111 << endl;});

void call_thread(void)
{
	res_ring.notify_all();
}

class A
{
	public:
	A(void){cout << 111 << endl;};
	A(int aaa):A(){cout << aaa << endl;};
	~A(){cout << 333 << endl;};

	int x;
};

class B
{
	public:
	B(void):A1(222){};

	A A1;
};*/

using namespace std;

int main(int argc,char** argv)
{
	fstream fp("./ttt");
	fp << 111 << endl;
	fp.close();

	return 0;
}

