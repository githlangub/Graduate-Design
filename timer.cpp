#include <iostream>
#include <sys/time.h>
#include <unistd.h>

using namespace std;

int main()
{
	timeval start,stop;
	unsigned count = 0;
	long total_rounds = 0;
	double total_times = 0;
	double total_avr = 0;

	unsigned a,b;
	cin>>a>>b;
	for(unsigned long i = a;i <= b;i *= 10)
	{
		for(unsigned long j = 1;j <= 9;j++)
		{
			unsigned long round = i * j;
			unsigned long k = round;
			total_rounds += round;
			cerr << "Result " << ++count << ": " << round << " rounds ";
			{
				gettimeofday(&start,NULL);
				while(k--);
				gettimeofday(&stop,NULL);
			}
			long gap = (stop.tv_sec - start.tv_sec) * 1000000 + (stop.tv_usec - start.tv_usec);
			total_times += gap;
			total_avr += (round / ((double)(gap) / 1000));
			cerr << (double)(gap) / 1000 << " ms ";
			cerr << round / ((double)(gap) / 1000) << " rounds/ms" << endl;
		}
	}
	cerr << "Average: " << total_rounds / (total_times / 1000) << " rounds/ms" << endl;
	cerr << "Average: " << total_avr / count << " rounds/ms" << endl;
}

