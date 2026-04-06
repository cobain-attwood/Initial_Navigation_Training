//custom i2c handler
#include "i2c_handler.h"

//custom UDP client handler
#include "UDP_client.h"

//for handling time
#include "timer.h"

//for maths 
#include <numbers>

#define _USE_MATH_DEFINES

//Defines
//Temp Sensor
#define TEMP_SENSITIVITY 0.00390625 //256 LSB/degrees C
#define TEMP_OFF_SET 25

//Linear Acceleration
#define ACCL_SENSITIVITY 0.061 //not garunteed to be correct - go through calibration if needed 
#define GRAVITY 9.80665 //in m/s^2

//Angular Rate
#define ANGL_SENSITIVITY 8.75 //not garunteed to be correct - go through calibration if needed 

//time interval
#define SAMPLING_INTERVAL 0.01 //in seconds
#define SCALING 100 // dependent on SAMPLING_INTERVAL

//IMU register addresses
#define PITCH_GYRO_REG 0x22
#define ROLL_GYRO_REG 0x24
#define YAW_GYRO_REG 0x26

#define X_AXIS_REG 0x28
#define Y_AXIS_REG 0x2A
#define Z_AXIS_REG 0x2C

/**
 * @brief takes a two byte buffer containing a value and transfers it to a 16 bit variable 
 * @param buffer pointer to the buffer containing two byte value
 * @return single 16 bbit value
 */
int16_t process_16_bit_register_content(unsigned char* buffer)
{
	int16_t value = 0;
	
	//stitch the two halves together
	value = (int16_t)buffer[1];
	value = value << 8;
	value |= (int16_t)buffer[0];
	
	return value;
}

//FEFACTOR TO BE NEATER 
int16_t get_data_amount(i2c_handler i2c)
{
	uint8_t buffer[2] = {0};
	
	buffer[0] = 0x3a;
	i2c.write_i2c_bus(buffer, 1);
	i2c.read_i2c_bus(buffer, 2);
	
	//getting rid of top six bits
	buffer[1] &= 0x3;
	
	int16_t both_bytes = process_16_bit_register_content(buffer);
	
	//std::cout << "FIFO_STATUS: " << (int)both_bytes << std::endl;
	return both_bytes;
}

/**
 * @brief sets up the IMU by setting control registers with the desired values (hardcoded) 
 * @param i2c i2c handler class to allow for sending data to IMU
 */
void setup_IMU(i2c_handler i2c)
{
	uint8_t buffer[2] = {0};
	
	//set the CTRL1_XL register
	buffer[0] = 0x10;
	buffer[1] = 0x80;
	i2c.write_i2c_bus(buffer, 2);
	
	//set the INT1_CTRL register
	buffer[0] = 0x0D;
	buffer[1] = 0x03;
	i2c.write_i2c_bus(buffer, 2);
	
	//set the CTRL2_G register
	buffer[0] = 0x11;
	buffer[1] = 0x80;
	i2c.write_i2c_bus(buffer, 2);
	
	//set the FIFO_CTRL3 register
	buffer[0] = 0x09;
	buffer[1] = 0x66;
	i2c.write_i2c_bus(buffer, 2);
	
	//set the FIFO_CTRL4 register
	buffer[0] = 0x0A;
	buffer[1] = 0x06;
	i2c.write_i2c_bus(buffer, 2);
}

void read_status_register(i2c_handler i2c)
{
	uint8_t buffer[2] = {0};
	uint8_t result_buffer[2] = {0};
	
	//Reading the status register
	buffer[0] = 0x1E;	
	i2c.write_i2c_bus(buffer, 1);		
	i2c.read_i2c_bus(result_buffer, 1);
	std::cout << "STATUS_REG read: " << std::hex << (int)result_buffer[0] << std::dec << std::endl;
}

/**
 * @brief turn raw accleration data from milli-G to m/s^2 to make it more human readable
 * @param raw_data unprocessed data in milli-Gs
 * @return acceleration data in m/s^2
 */
float process_accleration_data(int16_t raw_data)
{
	//gives us acceleration in milli-G
	float processed_data = (raw_data * ACCL_SENSITIVITY);
	
	//convert from milli-G to G
	processed_data = processed_data / 1000; 
	
	//convert from G to m/s^2
	processed_data = processed_data * GRAVITY;
	
	return processed_data;
}

/**
 * @brief turns micro-seconds into seconds 
 * @param time_us time in micro-seconds
 * @return time in seconds
 */
float micro_to_sec(double time_us)
{
	return time_us / 1000000;
}


float get_velocity(float velocity, float acceleration, double time)
{
	velocity = velocity + (acceleration * time);
	
	return velocity;
}

//determine distance traveled in metres
float determine_distance_traveled(float velocity, double time)
{
	return velocity * time;
}

//data filtered and averaged using interquartile range
float filter_and_calculate_mean_average(std::vector<float> sample_range)
{
	int UQ = ((3 * (sample_range.size() + 1)) / 4); //formula for upper quartile 
	int LQ = ((sample_range.size() + 1) / 4); //formula for lower quartile 
	//int IQR = UQ - LQ; //kept for reference
	
	//std::cout << "Sample size: " << sample_range.size() << std::endl;
	//std::cout << "Upper quarterile: " << UQ << std::endl;
	//std::cout << "Lower quarterile: " << LQ << std::endl;
	
	float mean_average = 0;
	
	//filter out noise
	int sample_size = 0;
	for(int i = LQ; i < UQ; i++)
	{
		if(i == sample_range.size())
		{
			//stop program aborting
			break;
		}
		//the data being already sorted at this stage is an assumption
		mean_average += sample_range.at(i); /// SCALING);
		sample_size++;
	}
	
	mean_average = mean_average / sample_size;
	//std::cout << "Mean average: " << mean_average << std::endl;
	//std::cout << "\x1B[4A" << std::flush;  //xA to go back
	return mean_average; // / sample_size;
}

//angular velocity in dps and time in seconds
float calculate_angle_change(float angular_velocity, double time)
{
	return (angular_velocity * time);
}

/**
 * @brief turns value in milli degrees per second into degrees per seconds 
 * @param mdps milli degrees per second
 * @return degrees per seconds
 */
float milli_dps_to_dps(float mdps)
{
	return mdps / 1000;
}

/**
 * @brief turns value in degrees into radians
 * @param degrees value in degrees
 * @return value in radians
 */
float degrees_to_radians(float degrees)
{
	return degrees * (M_PI / 180);
}

/**
 * @brief Calculates the acceleration due to gravity in each axis based on the orientation of the IMU. Results can be taken off acceleration data
 * to neutralise gravity from acceleromoter readings.
 * @param roll angle in in the roll axis
 * @param pitch angle in in the pitch axis
 * @param yaw  angle in in the yaw axis
 * @return a vector containing three offset values, one for each axis
 */
std::vector<float> calculate_gravity_offset(float roll, float pitch, float yaw)
{
	std::vector<float> gravity_offset;

	float r_roll = degrees_to_radians(roll);
	float r_pitch = degrees_to_radians(pitch);
	float r_yaw = degrees_to_radians(yaw);

	//do calculations in the orientation matrix
	float xx_by_xx = cos(r_yaw) * cos(r_pitch);
	float xx_by_xy = (cos(r_yaw) * sin(r_pitch) * sin(r_roll)) - (sin(r_yaw) * cos(r_roll));
	float xx_by_xz = (sin(r_yaw) * sin(r_roll)) + (cos(r_yaw) * sin(r_pitch) * cos(r_roll));  
	
	float yx_by_xx = sin(r_yaw) * cos(r_pitch);
	float yx_by_xy = (cos(r_yaw) * cos(r_roll)) + (sin(r_yaw) * sin(r_pitch) * sin(r_roll));
	float yx_by_xz = (sin(r_yaw) * sin(r_pitch) * cos(r_roll)) - (cos(r_yaw) * sin(r_roll));

	float zx_by_xx = -sin(r_pitch);
	float zx_by_xy = cos(r_pitch) * sin(r_roll);
	float zx_by_xz = cos(r_pitch) * cos(r_roll);
	
	//calculate the offsets for each direction
	gravity_offset.push_back((xx_by_xx * 0) + (xx_by_xy * 0) + (xx_by_xz * GRAVITY)); //x axis offset
	gravity_offset.push_back((yx_by_xx * 0) + (yx_by_xy * 0) + (yx_by_xz * GRAVITY)); //y axis offset
	gravity_offset.push_back((zx_by_xx * 0) + (zx_by_xy * 0) + (zx_by_xz * GRAVITY)); //z axis offset
	
	return gravity_offset; 
}

int16_t read_gyro(int address, i2c_handler handler)
{
	uint8_t buffer[2] = {0};
	uint8_t result_buffer[2] = {0};
	
	buffer[0] = address;	
	handler.write_i2c_bus(buffer, 1);		
	handler.read_i2c_bus(result_buffer, 2);
	
	return process_16_bit_register_content(result_buffer);
}

int16_t read_accelerometer(int address, i2c_handler handler)
{
	uint8_t buffer[2] = {0};
	uint8_t result_buffer[2] = {0};
	
	buffer[0] = address;	
	handler.write_i2c_bus(buffer, 1);		
	handler.read_i2c_bus(result_buffer, 2);
	
	return process_16_bit_register_content(result_buffer);
}

//get the angle of the IMU in a cetain axis from raw data
void get_angle(std::vector<float> angle_data, float* angle)
{
	//filter the data using it's interquartile range to remove noise
	std::sort(angle_data.begin(), angle_data.end());
	float average_anglur_velocity = filter_and_calculate_mean_average(angle_data); //angular velocity in dps
	//std::cout << "average_anglur_velocity: " << average_anglur_velocity << std::endl;

	*angle += calculate_angle_change(average_anglur_velocity, SAMPLING_INTERVAL);
	
	*angle += average_anglur_velocity;
	
	if(*angle >= 360)
	{
		*angle -= 360;
	}else if(*angle <= -360)
	{
		*angle += 360;
	}
}

float filter_acceleration(std::vector<float> acceleration_data, float offset)
{
	std::sort(acceleration_data.begin(), acceleration_data.end());
	for(int i = 0; i < acceleration_data.size() ;i++)
	{
		acceleration_data[i] -= offset; //removing the affects of gravity
	}
	return filter_and_calculate_mean_average(acceleration_data);	
}

float process_angle_data(uint8_t buffer[2])
{
	int16_t value = process_16_bit_register_content(buffer);
								
	float human_readable_value = (value * ANGL_SENSITIVITY);
	return  milli_dps_to_dps(human_readable_value);
}


int main()
{
	auto start_time = std::chrono::high_resolution_clock::now();
	
	//put here for the UDP functionality
	//uint8_t buffer[2] = {0};
	//uint8_t result_buffer[2] = {0};
	
	uint8_t fifo_result_buffer[4000] = {0};
	
	//set up i2c handler class
	i2c_handler i2c("/dev/i2c-1", 0x6b);
	
	//set up UDP server
	UDP_client client;
	if(client.client_setup() != 0)
	{
		return -1;
	}
	
	//open the i2c bus
	if(i2c.open_bus() != 0)
	{
		return -1;
	}
	else
	{
		setup_IMU(i2c);
		
		float x_velocity = 0; //m/s
		float y_velocity = 0; //m/s
		float z_velocity = 0; //m/s
		
		float position_in_x = 0; //metres
		float position_in_y = 0; //metres
		float position_in_z = 0; //metres 6 bytes
		
		float pitch_angle = 0; //degrees
		float roll_angle = 0; //degrees
		float yaw_angle = 0; //degrees
		
		//acceleration data
		std::vector<float> x_acceleration_data;
		std::vector<float> y_acceleration_data;
		std::vector<float> z_acceleration_data;
		
		//angle data
		std::vector<float> p_angle_data;
		std::vector<float> r_angle_data;
		std::vector<float> y_angle_data;
		
		Timer timer;
		while(1)
		{
			timer.start();
			while(timer.elapsedSeconds() < SAMPLING_INTERVAL)
			{
				//allow for data to fill IMU FIFO
			}
			timer.stop();
			
			uint8_t fifo_buffer = 0x78;
			
			int fifo_status_1 = get_data_amount(i2c);
			int fifo_data_length = fifo_status_1 * 7;
			//std::cout << "number of tags: " << fifo_status_1 << std::endl;	
			//std::cout << "fifo length: " << fifo_data_length << std::endl;
			
			//std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() << "ms" << std::endl;
				
			i2c.write_i2c_bus(&fifo_buffer, 1);
			if(fifo_data_length)
			{
				i2c.read_i2c_bus(fifo_result_buffer, fifo_data_length);
				
				for(int i = 0; i < fifo_data_length ;i++)
				{
					if(i%7 == 0)
					{
						uint8_t buffer[2] = {0};
						int16_t value = 0;
						switch(fifo_result_buffer[i])
						{
							case 0x0c:
								//std::cout << "TAG: Gyro" << std::endl;
								
								//pitch
								buffer[0] = fifo_result_buffer[i+1];
								buffer[1] = fifo_result_buffer[i+2];	
						
								p_angle_data.push_back(process_angle_data(buffer));
								//std::cout << std::dec << "Pitch: " << p_angle_data.back() << " dps" << std::endl;
								
								//roll
								buffer[0] = fifo_result_buffer[i+3];
								buffer[1] = fifo_result_buffer[i+4];
								
								r_angle_data.push_back(process_angle_data(buffer));
								//std::cout << std::dec << "Roll: " << r_angle_data.back() << " dps" << std::endl;
								
								//yaw
								buffer[0] = fifo_result_buffer[i+5];
								buffer[1] = fifo_result_buffer[i+6];
								
								y_angle_data.push_back(process_angle_data(buffer));
								//std::cout << std::dec << "Yaw: " << y_angle_data.back() << " dps" << std::endl;
								
								i+= 6; //moves us over the data we have just extracted
								break;
							case 0x0a:
								//std::cout << "TAG: Gyro" << std::endl;
								
								//pitch
								buffer[0] = fifo_result_buffer[i+1];
								buffer[1] = fifo_result_buffer[i+2];	
						
								p_angle_data.push_back(process_angle_data(buffer));
								//std::cout << std::dec << "Pitch: " << p_angle_data.back() << " dps" << std::endl;
								
								//roll
								buffer[0] = fifo_result_buffer[i+3];
								buffer[1] = fifo_result_buffer[i+4];
								
								r_angle_data.push_back(process_angle_data(buffer));
								//std::cout << std::dec << "Roll: " << r_angle_data.back() << " dps" << std::endl;
								
								//yaw
								buffer[0] = fifo_result_buffer[i+5];
								buffer[1] = fifo_result_buffer[i+6];
								
								y_angle_data.push_back(process_angle_data(buffer));
								//std::cout << std::dec << "Yaw: " << y_angle_data.back() << " dps" << std::endl;
								
								i+= 6; //moves us over the data we have just extracted
								break;
								
							case 0x0f:
								//std::cout << "TAG: Gyro" << std::endl;
								
								//pitch
								buffer[0] = fifo_result_buffer[i+1];
								buffer[1] = fifo_result_buffer[i+2];	
						
								p_angle_data.push_back(process_angle_data(buffer));
								//std::cout << std::dec << "Pitch: " << p_angle_data.back() << " dps" << std::endl;
								
								//roll
								buffer[0] = fifo_result_buffer[i+3];
								buffer[1] = fifo_result_buffer[i+4];
								
								r_angle_data.push_back(process_angle_data(buffer));
								//std::cout << std::dec << "Roll: " << r_angle_data.back() << " dps" << std::endl;
								
								//yaw
								buffer[0] = fifo_result_buffer[i+5];
								buffer[1] = fifo_result_buffer[i+6];
								
								y_angle_data.push_back(process_angle_data(buffer));
								//std::cout << std::dec << "Yaw: " << y_angle_data.back() << " dps" << std::endl;
								
								i+= 6; //moves us over the data we have just extracted
								break;	
								
							case 0x08:
								//std::cout << "TAG: Gyro" << std::endl;
								
								//pitch
								buffer[0] = fifo_result_buffer[i+1];
								buffer[1] = fifo_result_buffer[i+2];	
						
								p_angle_data.push_back(process_angle_data(buffer));
								//std::cout << std::dec << "Pitch: " << p_angle_data.back() << " dps" << std::endl;
								
								//roll
								buffer[0] = fifo_result_buffer[i+3];
								buffer[1] = fifo_result_buffer[i+4];
								
								r_angle_data.push_back(process_angle_data(buffer));
								//std::cout << std::dec << "Roll: " << r_angle_data.back() << " dps" << std::endl;
								
								//yaw
								buffer[0] = fifo_result_buffer[i+5];
								buffer[1] = fifo_result_buffer[i+6];
								
								y_angle_data.push_back(process_angle_data(buffer));
								//std::cout << std::dec << "Yaw: " << y_angle_data.back() << " dps" << std::endl;
								
								i+= 6; //moves us over the data we have just extracted
								break;	
								
							case 0x09:
								//std::cout << "TAG: Gyro" << std::endl;
								
								//pitch
								buffer[0] = fifo_result_buffer[i+1];
								buffer[1] = fifo_result_buffer[i+2];	
						
								p_angle_data.push_back(process_angle_data(buffer));
								//std::cout << std::dec << "Pitch: " << p_angle_data.back() << " dps" << std::endl;
								
								//roll
								buffer[0] = fifo_result_buffer[i+3];
								buffer[1] = fifo_result_buffer[i+4];
								
								r_angle_data.push_back(process_angle_data(buffer));
								//std::cout << std::dec << "Roll: " << r_angle_data.back() << " dps" << std::endl;
								
								//yaw
								buffer[0] = fifo_result_buffer[i+5];
								buffer[1] = fifo_result_buffer[i+6];
								
								y_angle_data.push_back(process_angle_data(buffer));
								//std::cout << std::dec << "Yaw: " << y_angle_data.back() << " dps" << std::endl;
								
								i+= 6; //moves us over the data we have just extracted
								break;	
								
							case 0x10:
								//std::cout << "TAG: Accel" << std::endl;
								
								//X-axis
								buffer[0] = fifo_result_buffer[i+1];
								buffer[1] = fifo_result_buffer[i+2];	
						
								value = process_16_bit_register_content(buffer);
								x_acceleration_data.push_back(process_accleration_data(value));
								//std::cout << std::dec << "X-axis: " << x_acceleration_data.back() << " m/s^2" << std::endl;
								
								//Y-axis
								buffer[0] = fifo_result_buffer[i+3];
								buffer[1] = fifo_result_buffer[i+4];	
						
								value = process_16_bit_register_content(buffer);
								y_acceleration_data.push_back(process_accleration_data(value));
								//std::cout << std::dec << "Y-axis: " << y_acceleration_data.back() << " m/s^2" << std::endl;
							
								//Z-axis
								buffer[0] = fifo_result_buffer[i+5];
								buffer[1] = fifo_result_buffer[i+6];	
						
								value = process_16_bit_register_content(buffer);
								z_acceleration_data.push_back(process_accleration_data(value));
								//std::cout << std::dec << "Z-axis: " << z_acceleration_data.back() << " m/s^2" << std::endl;
								
								i+= 6; //moves us over the data we have just extracted
								break;	
								
							case 0x12:
								//std::cout << "TAG: Accel" << std::endl;
								
								//X-axis
								buffer[0] = fifo_result_buffer[i+1];
								buffer[1] = fifo_result_buffer[i+2];	
						
								value = process_16_bit_register_content(buffer);
								x_acceleration_data.push_back(process_accleration_data(value));
								//std::cout << std::dec << "X-axis: " << x_acceleration_data.back() << " m/s^2" << std::endl;
								
								//Y-axis
								buffer[0] = fifo_result_buffer[i+3];
								buffer[1] = fifo_result_buffer[i+4];	
						
								value = process_16_bit_register_content(buffer);
								y_acceleration_data.push_back(process_accleration_data(value));
								//std::cout << std::dec << "Y-axis: " << y_acceleration_data.back() << " m/s^2" << std::endl;
							
								//Z-axis
								buffer[0] = fifo_result_buffer[i+5];
								buffer[1] = fifo_result_buffer[i+6];	
						
								value = process_16_bit_register_content(buffer);
								z_acceleration_data.push_back(process_accleration_data(value));
								//std::cout << std::dec << "Z-axis: " << z_acceleration_data.back() << " m/s^2" << std::endl;
								
								i+= 6; //moves us over the data we have just extracted
								break;
								
							case 0x14:
								//std::cout << "TAG: Accel" << std::endl;
								
								//X-axis
								buffer[0] = fifo_result_buffer[i+1];
								buffer[1] = fifo_result_buffer[i+2];	
						
								value = process_16_bit_register_content(buffer);
								x_acceleration_data.push_back(process_accleration_data(value));
								//std::cout << std::dec << "X-axis: " << x_acceleration_data.back() << " m/s^2" << std::endl;
								
								//Y-axis
								buffer[0] = fifo_result_buffer[i+3];
								buffer[1] = fifo_result_buffer[i+4];	
						
								value = process_16_bit_register_content(buffer);
								y_acceleration_data.push_back(process_accleration_data(value));
								//std::cout << std::dec << "Y-axis: " << y_acceleration_data.back() << " m/s^2" << std::endl;
							
								//Z-axis
								buffer[0] = fifo_result_buffer[i+5];
								buffer[1] = fifo_result_buffer[i+6];	
						
								value = process_16_bit_register_content(buffer);
								z_acceleration_data.push_back(process_accleration_data(value));
								//std::cout << std::dec << "Z-axis: " << z_acceleration_data.back() << " m/s^2" << std::endl;
								
								i+= 6; //moves us over the data we have just extracted
								break;
								
							case 0x17:
								//std::cout << "TAG: Accel" << std::endl;
								
								//X-axis
								buffer[0] = fifo_result_buffer[i+1];
								buffer[1] = fifo_result_buffer[i+2];	
						
								value = process_16_bit_register_content(buffer);
								x_acceleration_data.push_back(process_accleration_data(value));
								//std::cout << std::dec << "X-axis: " << x_acceleration_data.back() << " m/s^2" << std::endl;
								
								//Y-axis
								buffer[0] = fifo_result_buffer[i+3];
								buffer[1] = fifo_result_buffer[i+4];	
						
								value = process_16_bit_register_content(buffer);
								y_acceleration_data.push_back(process_accleration_data(value));
								//std::cout << std::dec << "Y-axis: " << y_acceleration_data.back() << " m/s^2" << std::endl;
							
								//Z-axis
								buffer[0] = fifo_result_buffer[i+5];
								buffer[1] = fifo_result_buffer[i+6];	
						
								value = process_16_bit_register_content(buffer);
								z_acceleration_data.push_back(process_accleration_data(value));
								//std::cout << std::dec << "Z-axis: " << z_acceleration_data.back() << " m/s^2" << std::endl;
								
								i+= 6; //moves us over the data we have just extracted
								break;
								
							case 0x11:
								//std::cout << "TAG: Accel" << std::endl;
								
								//X-axis
								buffer[0] = fifo_result_buffer[i+1];
								buffer[1] = fifo_result_buffer[i+2];	
						
								value = process_16_bit_register_content(buffer);
								x_acceleration_data.push_back(process_accleration_data(value));
								//std::cout << std::dec << "X-axis: " << x_acceleration_data.back() << " m/s^2" << std::endl;
								
								//Y-axis
								buffer[0] = fifo_result_buffer[i+3];
								buffer[1] = fifo_result_buffer[i+4];	
						
								value = process_16_bit_register_content(buffer);
								y_acceleration_data.push_back(process_accleration_data(value));
								//std::cout << std::dec << "Y-axis: " << y_acceleration_data.back() << " m/s^2" << std::endl;
							
								//Z-axis
								buffer[0] = fifo_result_buffer[i+5];
								buffer[1] = fifo_result_buffer[i+6];	
						
								value = process_16_bit_register_content(buffer);
								z_acceleration_data.push_back(process_accleration_data(value));
								//std::cout << std::dec << "Z-axis: " << z_acceleration_data.back() << " m/s^2" << std::endl;
								
								i+= 6; //moves us over the data we have just extracted
								break;
								
							default:
								std::cout << "TAG: other" << std::endl;
						}
					}
					else
					{
						std::cout << "Data misalignment has occured!!!" << std::endl;
					}
				}
			}
			
			//process angles
			//PITCH
			get_angle(p_angle_data, &pitch_angle);

			//ROLL
			get_angle(r_angle_data, &roll_angle);
			
			//YAW
			get_angle(y_angle_data, &yaw_angle);

			//clear angle data ready for next set of samples
			p_angle_data.clear();
			r_angle_data.clear();
			y_angle_data.clear();
			
			//get gravity offsets
			std::vector<float> offsets = calculate_gravity_offset(roll_angle, pitch_angle, yaw_angle);
			
			std::cout << "\x1B[2K" << "X offset: " << offsets[0] << std::endl;
			std::cout << "\x1B[2K" << "Y offset: " << offsets[1] << std::endl;
			std::cout << "\x1B[2K" << "Z offset: " << offsets[2] << std::endl;
		
			//process acceleration
			float x_avg_acceleration = filter_acceleration(x_acceleration_data, offsets[0]);
			float y_avg_acceleration = filter_acceleration(y_acceleration_data, offsets[1]);
			float z_avg_acceleration = filter_acceleration(y_acceleration_data, offsets[2]);
			
			//clear acceleration data ready for next set of samples
			x_acceleration_data.clear();
			y_acceleration_data.clear();
			z_acceleration_data.clear();
			
			
			//output for analysis
			auto current_time = std::chrono::high_resolution_clock::now();
			
			std::cout << "\x1B[2K" << std::setprecision(3) << "Acceleration:" << std::endl;
			std::cout << "\x1B[2K" << "X axis: " << x_avg_acceleration << " m/s^2" << std::endl;
			std::cout << "\x1B[2K" << "Y axis: " << y_avg_acceleration << " m/s^2" << std::endl;
			std::cout << "\x1B[2K" << "Z axis: " << z_avg_acceleration << " m/s^2" << std::endl;
		
			std::cout << "Program has been running for: " << std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count() << " seconds" << std::endl;
			std::cout << "\x1B[2K" << std::setprecision(3) << "orientation:" << std::endl;
			std::cout << "\x1B[2K" << "Pitch: " << pitch_angle << " degrees" << std::endl;
			std::cout << "\x1B[2K" << "Roll: " << roll_angle << " degrees" << std::endl;
			std::cout << "\x1B[2K" << "Yaw: " << yaw_angle << " degrees" << std::endl;
			
			std::cout << "\x1B[12A" << std::flush;  //xA to go back x lines
			
			
			/*
			//calculate velocity in X-axis
			/*x_velocity = get_velocity(x_velocity, avg_acceleration, SAMPLING_INTERVAL);
			std::cout << "\x1B[2K" << "X-axis velocity: " << x_velocity << " m/s" << std::endl; //for local print out
			
			//show final position in X-axis reletive to start position of 0
			position_in_x += determine_distance_traveled(x_velocity, SAMPLING_INTERVAL);
			std::cout << "\x1B[2K" << "position in X-axis: " << position_in_x << " metres" << std::endl;
			
			//print out data
			char x_data[32] = {0};
			sprintf(x_data, "X-axis data: %f m/s^2",  avg_acceleration);
			//client.send(x_data);
			std::cout << "\x1B[2K" << x_data << std::endl; //for local print out*/
					
			
			//KEEP - HAS UDP CODE THAT NEEDS REFACTORING
			/*
			//read Y axis
			buffer[0] = 0x2A;	
			i2c.write_i2c_bus(buffer, 1);		
			i2c.read_i2c_bus(result_buffer, 2);
			int16_t y_value_raw = process_16_bit_register_content(result_buffer);
			float y_value = (y_value_raw * ACCL_SENSITIVITY);
			y_value = process_accleration_data(y_value);
			char y_data[32] = {0};
			sprintf(y_data, "Y-axis data: %f m/s^2", y_value);
			client.send(y_data);
			std::cout << "\x1B[2K" << y_data << std::endl; //for local print out
			
			//read Z axis
			buffer[0] = 0x2C;	
			i2c.write_i2c_bus(buffer, 1);		
			i2c.read_i2c_bus(result_buffer, 2);
			int16_t z_value_raw = process_16_bit_register_content(result_buffer);
			float z_value = (z_value_raw * ACCL_SENSITIVITY);
			z_value = process_accleration_data(z_value);		
			char z_data[32] = {0};
			sprintf(z_data, "Z-axis data: %f m/s^2", z_value);
			client.send(z_data);
			
			std::cout << "\x1B[2K" << z_data << std::endl; //for local print out
			
			//read pitch axis 
			buffer[0] = 0x22;	
			i2c.write_i2c_bus(buffer, 1);		
			i2c.read_i2c_bus(result_buffer, 2);
			int16_t pitch_value_raw = process_16_bit_register_content(result_buffer);
			float pitch_value = (pitch_value_raw * ANGL_SENSITIVITY);
			char p_data[32] = {0};
			sprintf(p_data, "Pitch data: %f", pitch_value);
			client.send(p_data);
			
			
			std::cout << "\x1B[2K" << "Pitch data: " << pitch_value << std::endl; //for local print out
			
			//read roll axis 
			buffer[0] = 0x24;	
			i2c.write_i2c_bus(buffer, 1);		
			i2c.read_i2c_bus(result_buffer, 2);
			int16_t roll_value_raw = process_16_bit_register_content(result_buffer);
			float roll_value = (roll_value_raw * ANGL_SENSITIVITY);
			char r_data[32] = {0};
			sprintf(r_data, "Roll data: %f", roll_value);
			client.send(r_data);
			std::cout << "\x1B[2K" << "Roll data: " << roll_value << std::endl; //for local print out
			
			//read yaw axis 
			buffer[0] = 0x26;	
			i2c.write_i2c_bus(buffer, 1);		
			i2c.read_i2c_bus(result_buffer, 2);
			int16_t yaw_value_raw = process_16_bit_register_content(result_buffer);
			float yaw_value = (yaw_value_raw * ANGL_SENSITIVITY);
			char yaw_data[32] = {0};
			sprintf(yaw_data, "Yaw data: %f", yaw_value);
			client.send(yaw_data);
			std::cout << "\x1B[2K" << "Yaw data: " << yaw_value << std::endl; //for local print out
			
			//read temperature axis 
			buffer[0] = 0x20;	
			i2c.write_i2c_bus(buffer, 1);		
			i2c.read_i2c_bus(result_buffer, 2);
			int16_t temp_value_raw = process_16_bit_register_content(result_buffer);
			float temp_value = (temp_value_raw * TEMP_SENSITIVITY) + TEMP_OFF_SET;
			char t_data[32] = {0};
			sprintf(t_data, "Temperature data: %f", temp_value);
			client.send(t_data);
			std::cout << "\x1B[2K" << "Temperature data: " << temp_value << std::endl; //for local print out*/
			
			//std::cout << "\x1B[3A" << std::flush;  //xA to go back x lines
			
		}
		
		
	}
	
	return 0;
}
