
// GLOVE!!!!!!!!!!!
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "Hand.pb.h"

#include <errno.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <mcp3004.h>
#include <LSM9DS1.h>
#include <LSM9DS1_Types.h>
#include <softPwm.h>

const int BASE = 100;
const int SPI_CHAN = 0;

const int max_data_size = 4096;

using std::cout;
using std::endl;
using std::cerr;

int sock , n;
struct sockaddr_in server, from;
demo::Hand hand_data;
//char buffer[max_data_size] = {0};
unsigned int length;
float fArray [5]= {1.23f,2.23f,3.23f,4.23f,5.23f};
float aArray [3]= {1.23f,2.23f,3.23f};
float gArray [3]= {1.23f,2.23f,3.23f};
float mArray [3]= {1.23f,2.23f,3.23f};
float press_array [5];

//LSM9DS1 imu(IMU_MODE_I2C, 0x6b, 0x1e);

float R_DIV = 47000.000f;
const float STR_R[5]= {8500.00f,8500.00f,12000.00f,8000.00f,8000.00f};
//			 pinky ring middle  index  thumb
const float BEND_R[5] = {18000.00f,21000.00f,40000.00f,21000.00f,16000.00f};

const int PWM[5] = {25,24,23,22,21};
float flex_data[5];
float flex_voltage[5];
float resistance[5];
float buzzer_val[5];
int servo_val[5];


void error(const char *msg){
      perror(msg);
     exit(0);
}

void sever_setup(void){
    int port = 1024;
    struct hostent *hp;
    length=sizeof(struct sockaddr_in);
    sock= socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) error("socket");

    server.sin_family = AF_INET;
    hp = gethostbyname("10.16.4.131");
    bcopy((char *)hp->h_addr, 
            (char *)&server.sin_addr,
            hp->h_length);
    server.sin_port = htons(port);
}
void hand_setup(void){
    for(float i =0.00f; i <5.00f; i = i + 1.00f ){
        hand_data.add_flex_resistor(i);
    }
    for(float i =0.00f; i <3.00f; i = i + 1.00f ){
        hand_data.add_imu_accel(i);
    }
    
    for(float i =0.00f; i <3.00f; i = i + 1.00f ){
        hand_data.add_imu_gyro(i);
    }
    
    for(float i =0.00f; i <3.00f; i = i + 1.00f ){
        hand_data.add_imu_mag(i);
    }
}

void all_print(void){
    printf("Flex resistor\n");
    for(int i =0; i <5; i++){
        printf("%f\n", hand_data.flex_resistor(i));
    }
    printf("accel\n");
    for(int i =0; i<3;i++){
        printf("%f\n", hand_data.imu_accel(i));
    }
    printf("gryo\n");
    for(int i =0; i<3;i++){
        printf("%f\n", hand_data.imu_gyro(i));
    }
    printf("mag\n");
    for(int i =0; i<3;i++){
        printf("%f\n", hand_data.imu_mag(i));
    }
}

void send_data(void){
    char buffer[max_data_size] = {0};
    for(int i =0; i < 5; i++){
        hand_data.set_flex_resistor(i,fArray[i]+0.01f);
    }
    for(int i=0;i<3;i++){
        hand_data.set_imu_accel(i,aArray[i]+0.01f);
    }
    for(int i=0;i<3;i++){
        hand_data.set_imu_gyro(i,gArray[i]+0.01f);
    }
    for(int i=0;i<3;i++){
        hand_data.set_imu_mag(i,mArray[i]+0.01f);
    }
    all_print();
    std::string data;
    hand_data.SerializeToString(&data);
    sprintf(buffer, "%s", data.c_str());
    n=sendto(sock,buffer,
            strlen(buffer),0,(const struct sockaddr *)&server,length);
    if (n < 0) error("Sendto");
}
void receive(){
    char buffer[max_data_size] = {0};
    n = recvfrom(sock,buffer,max_data_size,0,(struct sockaddr *)&server,&length);
    if (n < 0) error("recvfrom");
    printf("receive glove_data  \n");
    std::string a = buffer;
    demo::Glove glove_data;
    glove_data.ParseFromString(a);
    printf("before for loop\n");
    for(int i =0; i < 5; i++){
        printf("%d = %f\n",i, press_array[i]);
        press_array[i] = glove_data.pressure_sensor(i);
        printf("%d = %f\n",i, press_array[i]);
    }
    printf("after for loop\n");
}
void pressure_sensor_print(void){
    printf("Pressure resistor\n");
    for(int i =0; i <5; i++){
        printf("%f\n", press_array[i]);
    }
}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void servo_setup(int size) {
	for (int i=0; i<size; i++) {
		pinMode(PWM[i],OUTPUT);
		softPwmCreate(PWM[i],0,50);
	}
}

void servo_write(int size) {
	for (int i=0; i<size; i++) {
		softPwmWrite(PWM[i],servo_val[i]);
	}
}

void flex_read(int base) {
	for (int i=0; i<5; i++) {
		flex_data[i] = (float)analogRead(base+i);
	}
}

void calc_voltage(int size) {
	for (int i=0; i<size; i++) {
		flex_voltage[i] = (float)((flex_data[i]*(5.0f)/1023.0f));
	}
}

void calc_resistance(int size) {
	for (int i=0; i<size; i++) {
		resistance[i] = R_DIV*(5.0f/flex_voltage[i] - 1.0f);
	}
}

void calc_angle(int size) {
	for (int i=0; i<size; i++) {
		fArray[i] = map(resistance[i],STR_R[i],BEND_R[i],0,90.0f);	
	}
}

void calc_all(int size) {
	for (int i=0; i<size; i++) {
		flex_voltage[i] = (float)(flex_data[i]*(5.0f)/1023.0f)+ 0.01f;
		resistance[i] = (float)(R_DIV*(5.0f/flex_voltage[i] - 1.0f))+ 0.01f;
		fArray[i] = resistance[i];	
	}
}
/*
void imu_read() {
		while (!imu.gyroAvailable()) ;
        imu.readGyro();
		gArray[0] = imu.calcGyro(imu.gx);
		gArray[1] = imu.calcGyro(imu.gy);
		gArray[2] = imu.calcGyro(imu.gz);
        while(!imu.accelAvailable()) ;
        imu.readAccel();
		aArray[0] = imu.calcAccel(imu.ax);
		aArray[1] = imu.calcAccel(imu.ay);
		aArray[2] = imu.calcAccel(imu.az);
        while(!imu.magAvailable()) ;
        imu.readMag();
		mArray[0] = imu.calcMag(imu.mx);
		mArray[1] = imu.calcMag(imu.my);
		mArray[2] = imu.calcMag(imu.mz);
}
*/

int main(void){
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	sever_setup();
	hand_setup();
	wiringPiSetup();
	int check;
	check = mcp3004Setup(BASE,SPI_CHAN);
	if (check == -1) {
		fprintf(stderr, "Failed to communicate with ADC_Chip.\n");
        	exit(EXIT_FAILURE);
	}
    printf("forloop\n");
	for (int i=0; i<10; i++) {
		flex_read(BASE);
		calc_all(5);
        //all_print();
		send_data();
		
		receive();
		pressure_sensor_print();
		
		
		delay(1000);
	}
	close(sock);
	printf("client finish\n");
    /*
    sever_setup();
    hand_setup();
    fResistor_set();
    send_data();
    receive();
    pressure_sensor_print();
    close(sock);*/
    printf("client finish\n");
    
   	return 0;   
}








    


