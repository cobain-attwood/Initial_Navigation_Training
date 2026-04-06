#include "i2c_handler.h"

/**
 * @brief Sets up and opens a i2c bus. 
 * 
 * @return 0 on success and -1 on failure
 */
int i2c_handler::open_bus()
{
	if ((file_i2c = open(file, O_RDWR)) < 0)
	{
		//ERROR HANDLING: you can check errno to see what went wrong
		std::cout << "Failed to open the i2c bus" << std::endl;
		std::cout << "errno is: " << errno << std::endl;
		std::cout << strerror(errno) << std::endl;
		return -1;
	}

	if (ioctl(file_i2c, I2C_SLAVE, address) < 0)
	{	
		//ERROR HANDLING; you can check errno to see what went wrong
		std::cout << "Failed to acquire bus access and/or talk to slave." << std::endl;
		std::cout << "errno is: " << errno << std::endl;
		std::cout << strerror(errno) << std::endl;
		return -1;
	}
	return 0;
}

/**
 * @brief Reads from i2c bus. 
 * @param buffer pointer to the buffer read data will be stored in
 * @param length number of bytes to be read
 */
void i2c_handler::read_i2c_bus(unsigned char* buffer, int length)
{
	//read() returns the number of bytes actually read, if it doesn't match then an error occurred
	if (read(file_i2c, buffer, length) != length)		
	{
		//ERROR HANDLING: i2c transaction failed
		std::cout << "Failed to read from the i2c bus." << std::endl;
		std::cout << "errno is: " << errno << std::endl;
		std::cout << strerror(errno) << std::endl;
	}
}

/**
 * @brief Writes to i2c bus. 
 * @param buffer pointer to the buffer data will be written to
 * @param length number of bytes to be written
 */
void i2c_handler::write_i2c_bus(unsigned char* buffer, int length)
{
	//write() returns the number of bytes actually written, if it doesn't match then an error occurred
	if (write(file_i2c, buffer, length) != length)		
	{
		/* ERROR HANDLING: i2c transaction failed */
		std::cout << "Failed to write to the i2c bus." << std::endl;
		std::cout << "errno is: " << errno << std::endl;
		std::cout << strerror(errno) << std::endl;
	}
}

/**
 * @brief Returns file handler
 * @return file_i2c 
 */
int i2c_handler::get_file_i2c()
{
	return file_i2c;
}

