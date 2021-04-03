#include <iostream>
#include <unistd.h>
#include <sys/time.h>

using namespace std;

int main()
{
	struct timeval old_time,new_time;
	cout << sizeof(timeval) << "," << sizeof(old_time.tv_sec) << "," << sizeof(old_time.tv_usec) << endl;

	if(gettimeofday(&old_time,NULL) == -1)
	{
		cerr << "Get system time failed!" << endl;
	}
	cout << old_time.tv_sec << "s + " << old_time.tv_sec << "us." << endl;

	new_time.tv_sec = 123456789;
	new_time.tv_sec = 987654321;
	if(settimeofday(&new_time,NULL) == -1)
	{
		cerr << "Set system time failed!" << endl;
	}
	cout << new_time.tv_sec << "s + " << new_time.tv_sec << "us." << endl;

	settimeofday(&old_time,NULL);

	return 0;
}

