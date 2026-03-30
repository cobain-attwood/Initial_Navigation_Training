//libaries
#include <NewPing.h>

//GLOBAL VARIABLES.
bool obsticle_detected_front = false;
bool obsticle_detected_left = false;
bool obsticle_detected_right = false;
bool obsticle_detected_rear = false;

//Initialize an ultrasonic device, trigger pin, echo pin, and optional maximum distance you wish to sensor to measure (default = 500cm).
NewPing sonar_front(3, 2, 200);
NewPing sonar_left(9, 10, 200);
NewPing sonar_right(11, 12, 200);
NewPing sonar_rear(4, 13, 200);

//varibles to control the motors. m1 = left motor m2 = right motor
int E1 = 6; //M1 Speed Control
int E2 = 5; //M2 Speed Control
int M1 = 8; //M1 Direction Control
int M2 = 7; //M2 Direction Control

//FUNCTIONS: mainly individual behaviours.

//reads data from selected sonar and sends that data over serial
void readSonar(NewPing sonar) {
  //delay(1000);
  int distance = sonar.ping_cm(); //new max distance can be sent as a parameter
  Serial.println(distance);
}

//detects if an obsticle is within 30cm of selected sensor
bool detectObsticle(NewPing sonar) {
  int distance = sonar.ping_cm();
  if (distance < 30 && distance > 0) {
    Stop();
    return true;
  } else {
    //move_forward();
    return false;
  }
}

//sets the motors to the stop position
void Stop() {
  analogWrite (E1, 0);
  digitalWrite(M1, LOW);
  analogWrite (E2, 0);
  digitalWrite(M2, LOW);
}

//moving robot forward (at max speed).
void move_forward() {
  analogWrite (E1, 255);
  digitalWrite(M1, LOW);
  analogWrite (E2, 255);
  digitalWrite(M2, LOW);
}

//moving robot backwards (at max speed).
void move_backwards() {
  analogWrite (E1, 255);
  digitalWrite(M1, HIGH);
  analogWrite (E2, 255);
  digitalWrite(M2, HIGH);
}

//turn the robot on its axis to the right 10 degrees.
void turn_right() {

  //drives left track forward
  analogWrite (E1, 255);
  digitalWrite(M1, LOW);

  //drives right track backwards
  analogWrite (E2, 255);
  digitalWrite(M2, HIGH);

  delay(200);

  Stop();
}

//turn the robot on its axis to the left 10 degrees.
void turn_left() {

  //drives left track backwards
  analogWrite (E1, 255);
  digitalWrite(M1, HIGH);

  //drives right track forwards
  analogWrite (E2, 255);
  digitalWrite(M2, LOW);

  delay(200);

  Stop();
}

//sets up the sensors and motors for the rest of the program to use.(runs once)
void setup() {
  int i;
  Serial.begin(9600);

  //setting up motor pinmodes - DONT TOUCH UNTIL YOU UNDERSTAND EXACTLY WHAT THIS DOES AGAIN!!!
  for (i = 5; i <= 8; i++) {
    pinMode(i, OUTPUT);
  }
}

//handles the execution of behaviours. (runs continually)
void loop() {

  //sonar debug
  //readSonar(sonar3);

  obsticle_detected_front = detectObsticle(sonar_front);
  obsticle_detected_left = detectObsticle(sonar_left);
  obsticle_detected_right = detectObsticle(sonar_right);
  obsticle_detected_rear = detectObsticle(sonar_rear);
  
  if (obsticle_detected_front) 
  {
    Stop();
    if (obsticle_detected_left) 
    {
      if (!obsticle_detected_right) 
      {
        turn_right();
      }
    } else if (!obsticle_detected_left) 
      {
        turn_left();
      } else 
        {
          if (!obsticle_detected_rear) 
          {
            move_backwards();
          }
        }

  } else if(obsticle_detected_left)
    {
      turn_right();
    } else if(obsticle_detected_right)
      {
        turn_left();
      } else 
        {
          move_forward();
        }
  delay(200);
}

//INFORMATION ON SENSORS AND MOTORS
//HOW THEY ARE USED AND WHAT THE VALUES USED IN THE CODE MEAN.

//INFORMATION FOR MOTORS:
//maximum speed is set with a value of 255
//minimum speed is set with a value of 1
//stop value is 0
//setting the motors LOW moves the robot forward.
//setting the motors HIGH moves the robot backwards.
