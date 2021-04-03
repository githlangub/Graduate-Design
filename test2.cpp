//Test <functional> & bind().

#include <iostream>
#include <functional>

using namespace std;

class A
{
	public:
		void a(int num)
		{
			cout << "A = " << num << endl;
		}
};



class B
{
	public:
		void b(int num)
		{
			cout << "B = " << num << endl;
		}
};



void func1(int num)
{
	cout << "func1() >> " << num << endl;
}



void func2(function<void(int)> to_call)
{
	to_call(10);
}



int main()
{
	A A1;
	B B1;

	func2(func1);
	func2(bind(&A::a,&A1,100));
	func2(bind(&B::b,&B1,1000));

	return 0;
}

