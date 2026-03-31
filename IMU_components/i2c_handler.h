//for i2c usage
#include "linux/i2c-dev.h"
#include "unistd.h"
#include "fcntl.h"
#include "sys/ioctl.h"
#include "math.h"

//for errno usage
#include <cerrno>
#include <cstring>
#include <iostream>
#include <iomanip>

class i2c_handler
{
	private:
		int file_i2c;
		int address;
		const char *file;
		
	public:
		i2c_handler(std::string filename, int addr)
		{
			file = filename.c_str();
			address = addr;
		}
		
		int open_bus();
		void read_i2c_bus(unsigned char* buffer, int length);
		void write_i2c_bus(unsigned char* buffer, int length);
		int get_file_i2c();
};
