#include <Servo.h>
#include <PS2X_lib.h>  //for v1.6

#define PHOTOR A0
#define WINCHSERVO 2
#define STRETCHERANGLESERVO 3
#define FRONTPINGSENSOR 4 //hooked up to a servo pin
#define SIDEPINGSENSOR 5 //hooked up to a servo pin
#define LEFTSERVO 6
#define RIGHTSERVO 7
#define motorControl1 10
#define motorControl2 11
#define LED 13

//REMAINING PINS:
//0,1 : dont use if possible
//8,9 : servo connections
//12  : nothing special
//A5-7: analog

//winch constants
#define W_UP 0
#define W_DOWN 180
#define W_STOP 70

//drive constants
#define FORWARD_L 170
#define FORWARD_R 0
#define BACKWARDS_L 0
#define BACKWARDS_R 180
#define LEFTZERO 98
#define RIGHTZERO 95
#define LIGHTCHANGETHRESHOLD 180
int relativeLightValue;
int PS2PINS[]  = {A4,A2,A3,A1}; //{CLOCK=blue, COMMAND=orange, ATTENTION=yellow, DATA=brown}

Servo leftWheel;
Servo rightWheel;
Servo winch;
Servo stretcherAngle;

void setup() {
  //set up servos
  leftWheel.attach(LEFTSERVO);
  rightWheel.attach(RIGHTSERVO);
  winch.attach(WINCHSERVO);
  stretcherAngle.attach(STRETCHERANGLESERVO);
  
  pinMode(LED, OUTPUT);
  pinMode(PHOTOR, INPUT);
  Serial.begin(9600);
  delay(100);
  relativeLightValue = analogRead(PHOTOR);
  stretcherAngle.write(120);
}

void loop() {
  waitForLight();
  winch.write(W_STOP);
  forward();
  delay(800);
  spinRight();
  delay(1800);
  followEdge(12, 35);
  hardBackLeft();
  delay(1000);
  spinRight();
  delay(2100);
  backwards();
  while(pingDistanceCM(FRONTPINGSENSOR) < 44)
  {
    //do nothing. Ping already has a delay 100ms built in
  }
  stopDriving();
  liftWrench();
  //enterRCmode();
  delay(10000000);
}

void waitForLight()
{
  double lightValue = analogRead(PHOTOR);
  while(!(abs(lightValue-relativeLightValue) > LIGHTCHANGETHRESHOLD))
  {
    lightValue = analogRead(PHOTOR);
    delay(10);
  }
}

void followEdge(int distFromEdge, int endDist)
{
  Serial.println("entered followEdge");
  int lastValue = pingDistanceCM(SIDEPINGSENSOR);
  delay(100);
  int change = 0;
  forward();
  while(pingDistanceCM(FRONTPINGSENSOR) > endDist)
  {
    int currentValue = pingDistanceCM(SIDEPINGSENSOR);
    Serial.println("currentValue: " + String(currentValue));
    change = lastValue - currentValue;
    Serial.println("change: " + String(change));
    if(change > 0)
    {//we are getting close to wall   
      if(currentValue - change < distFromEdge)
      {//the next timestep should take us slightly too close. slow down outside wheel
        Serial.println("will get too close. turn halfLeft");
        halfLeft();
      }
      else
      {//next timestep will not take us too close, but we are getting closer. keep going straight
        Serial.println("getting closer, wont get too close. go straight");
        forward();
      }
    }
    else if(change < 0)
    {//we are getting further from wall
      if(currentValue - change < distFromEdge)
      {//the next timestep should take us slightly too far. slow down outside wheel
        Serial.println("getting farther. will get too far. turn halfRight");
        halfRight();
      }
      else
      {//next timestep will not take us too far, but we are getting farther. keep going straight
        Serial.println("getting farther, wont get too far. go straight");
        forward();
      }
    }
    else
    {//same distance from wall.
      Serial.println("good distance. go straight");
      forward();
    }
    lastValue = currentValue;
    delay(400);
  }
  Serial.println("left followEdge");
}

void liftWrench()
{
  //run winch
  winch.write(W_DOWN);
  delay(2500);
  winch.write((W_STOP-W_UP)/2); //go up half speed
  delay(3500);
  winch.write(W_STOP);
}

int pingDistanceCM(int pinNum)
{
  //send a ping
  pinMode(pinNum, OUTPUT);
  digitalWrite(pinNum, LOW);
  delayMicroseconds(2);
  digitalWrite(pinNum, HIGH);
  delayMicroseconds(5);
  digitalWrite(pinNum, LOW); 
  //wait for return
  pinMode(pinNum, INPUT);
  long duration = pulseIn(pinNum, HIGH);
  //convert to cm
  Serial.println((int)duration / 58);
  delay(100); //help prevent interference
  return (int)duration / 58; //its a magic number for converting
}

void forward()
{
  rightWheel.write(FORWARD_R);
  leftWheel.write(FORWARD_L);
}

void backwards()
{
  rightWheel.write(BACKWARDS_R);
  leftWheel.write(BACKWARDS_L);
}

void spinLeft()
{
  rightWheel.write(FORWARD_R);
  leftWheel.write(BACKWARDS_L);
}

void spinRight()
{
  rightWheel.write(BACKWARDS_R);
  leftWheel.write(FORWARD_L);
}

void hardLeft()
{
  rightWheel.write(FORWARD_R);
  leftWheel.write(LEFTZERO);
}

void hardRight()
{
  rightWheel.write(RIGHTZERO);
  leftWheel.write(FORWARD_L);
}
void hardBackLeft()
{
  rightWheel.write(BACKWARDS_R);
  leftWheel.write(LEFTZERO);
}

void hardBackRight()
{
  rightWheel.write(RIGHTZERO);
  leftWheel.write(BACKWARDS_L);
}

void halfLeft()
{
  rightWheel.write(FORWARD_R);
  leftWheel.write((4.5*LEFTZERO+1.0*FORWARD_L)/5.5);
}

void halfRight()
{
  rightWheel.write((4.5*RIGHTZERO+1.0*FORWARD_R)/5.5);
  leftWheel.write(FORWARD_L);
}

void stopDriving()
{
  rightWheel.write(RIGHTZERO);
  leftWheel.write(LEFTZERO);
}

void openStretcher()
{
  digitalWrite(motorControl1, HIGH);
  digitalWrite(motorControl2, LOW);
}

void closeStretcher()
{
  digitalWrite(motorControl1, LOW);
  digitalWrite(motorControl2, HIGH);
}

void stopStretcher()
{
  digitalWrite(motorControl1, LOW);
  digitalWrite(motorControl2, LOW);
}

void enterRCmode()
{
  pinMode(LEFTSERVO, INPUT);
  pinMode(RIGHTSERVO, INPUT);
  pinMode(WINCHSERVO, INPUT);
}
