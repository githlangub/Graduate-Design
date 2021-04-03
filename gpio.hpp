#ifndef __GPIO_HPP__
#define __GPIO_HPP__

#define GPIOH2_0	"146"
#define GPIOH2_1	"147"
#define GPIOH2_2	"148"
#define GPIOH2_3	"149"
#define GPIOH3_0	"155"
#define GPIOH3_1	"156"
#define GPIOH3_2	"157"
#define	GPIOH3_3	"158"

class GPIO
{

	public:
		enum Direction_t
		{
			in = 1,
			out = 0
		};

		enum Value_t
		{
			high = 1,
			low = 0
		};

		GPIO(void);
		GPIO(const char* );
		~GPIO();

		void Open(const char* );
		void Close(void);
		void Listen(const Value_t ,double = 1,double = 1 * 1e-3);

		int operator=(const Value_t );
		int operator~(void);
		int operator==(const Value_t );
		int operator!=(const Value_t );
		//int Config(const bool ,const bool );
		bool Is_Open(void);

	private:
		char* gpio_id;
		char* direction_addr;
		char* value_addr;
		char* edge_addr;
		bool open_flag;
};

#endif

