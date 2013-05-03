#include <Servo.h>
#include <PS2X_lib.h>

#define PHOTOR A0
#define WINCHSERVO 2
#define STRETCHERANGLESERVO 3
#define FRONTPINGSENSOR 4 //hooked up to a servo pin
#define SIDEPINGSENSOR 5 //hooked up to a servo pin
#define LEFTSERVO 6
#define RIGHTSERVO 7
#define STRETCHERMOTOR 8
#define SIDEPIN 12
#define LED 13

//REMAINING EMPTY PINS:
//0,1    : dont use if possible, for main TX/RX
//8      : servo connection
//10,11  : nothing special
//13     : nothing special
//A5-A7  : analog

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

//amount the light value has to change before robot considers LED to be lit
#define LIGHTCHANGETHRESHOLD 180

//adjust the rubber band stretcher angle
#define STRETCHERBACK 120
#define STRETCHEROUT 40

//motor stretching controls
#define MOTOROUT 1000
#define MOTORIN 2000
#define MOTORSTOP 1500

//#################################################
//CHANGE THIS VALUE
boolean leftSide = false;
//################################################


int relativeLightValue;
int PS2PINS[]  = {A4,A2,A3,A1}; //{CLOCK=blue, COMMAND=orange, ATTENTION=yellow, DATA=brown}
PS2X ps2x; // create PS2 Controller Class

Servo leftWheel;
Servo rightWheel;
Servo winch;
Servo stretcherAngle;
Servo stretcherMotor;

void setup() {
  //set up servos
  leftWheel.attach(LEFTSERVO);
  rightWheel.attach(RIGHTSERVO);
  winch.attach(WINCHSERVO);
  stretcherAngle.attach(STRETCHERANGLESERVO);
  stretcherMotor.attach(STRETCHERMOTOR);
  
  pinMode(LEFTSERVO, OUTPUT);
  pinMode(RIGHTSERVO, OUTPUT);
  pinMode(WINCHSERVO, OUTPUT);
  pinMode(STRETCHERANGLESERVO, OUTPUT);
  pinMode(STRETCHERMOTOR, OUTPUT);
  pinMode(SIDEPIN, INPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  Serial.begin(57600);
  delay(100);
  relativeLightValue = analogRead(PHOTOR);
  stretcherAngle.write(STRETCHERBACK);
  stretcherMotor.writeMicroseconds(MOTORSTOP);
  setupPS2controller();
  digitalWrite(LED, HIGH);
}

void loop() {
  waitForLight();
  stretcherMotor.writeMicroseconds(MOTOROUT);
  if(leftSide == true)
  {
    getWrenchLeft(); //autonomously drive and attempt to get wrench
  }
  else
  {
    getWrenchRight();
  }
  stretcherMotor.writeMicroseconds(MOTORSTOP);
  PS2control();
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

void getWrenchLeft()
{
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
}

void getWrenchRight()
{
  forward();
  delay(800);
  spinRight();
  delay(2000);
  backwards();
  delay(14000); //10 seconds
  //now we should be against the far wall
  forward();
  delay(1800);
  spinRight();
  delay(1700);
  backwards();
  while(pingDistanceCM(FRONTPINGSENSOR) < 34)
  {
    //do nothing. Ping already has a delay 100ms built in
  }
  stopDriving();
  liftWrench();
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

void driveCustom(int left, int right)
{
  leftWheel.write(left);
  rightWheel.write(right);
}

void mapAndDrive(byte left, byte right)
{
  //Serial.println("left:" + (String)left + " right:" + (String)right);
  int l;
  int r;
  if(left < 50)
  {
    l = FORWARD_L;
  }
  else if(left > 205)
  {
    l = BACKWARDS_L;
  }
  else// if(left <
  {
    l = LEFTZERO;
  }

  if(right < 50)
  {
    r = FORWARD_R;
  }
  else if (right > 205)
  {
    r = BACKWARDS_R;
  }
  else
  {
    r = RIGHTZERO;
  }
  driveCustom(l, r);
}

void stopDriving()
{
  rightWheel.write(RIGHTZERO);
  leftWheel.write(LEFTZERO);
}


void enterRCmode()
{
  pinMode(LEFTSERVO, INPUT);
  pinMode(RIGHTSERVO, INPUT);
  pinMode(WINCHSERVO, INPUT);
}

void setupPS2controller()
{
  int error;
  byte type;
  
  //config_gamepad(clock, command, attention, data, Pressures?, Rumble?)
  error = ps2x.config_gamepad(PS2PINS[0],PS2PINS[1],PS2PINS[2],PS2PINS[3], false, false);
  
  if(error == 0){
    Serial.println("Found Controller, configured successful");
  }
  else if(error == 1)
    Serial.println("No controller found, check wiring, see readme.txt to enable debug. visit www.billporter.info for troubleshooting tips");
   
  else if(error == 2)
    Serial.println("Controller found but not accepting commands. see readme.txt to enable debug. Visit www.billporter.info for troubleshooting tips");

   //Serial.print(ps2x.Analog(1), HEX);
   
   type = ps2x.readType(); 
   switch(type) {
     case 0:
      Serial.println("Unknown Controller type");
      setupPS2controller();
      break;
     case 1:
      Serial.println("DualShock Controller Found");
     break;
   }  
   ps2x.read_gamepad();
   if(ps2x.Analog(PSS_LY) == 255)
   {
     Serial.println("value was 255 (AKA controller not set up correctly, re-running setup");
     setupPS2controller();
   }
}


void PS2control()
{
  int stretcherAnglePos = (STRETCHERBACK + STRETCHEROUT)/2.0;
  while(true) //run this forever
  {
    ps2x.read_gamepad();          //read controller. changes are only recognized here
    
    /*
    //INFORMATION FOR WRITING ADDITIONAL FUNCTIONS
    
    //BUTTON NAMES
    PSB_L1
    PSB_L2
    PSB_L3
    PSB_R1
    PSB_R2
    PSB_R3
    
    PSB_PAD_LEFT
    PSB_PAD_RIGHT
    PSB_PAD_UP
    PSB_PAD_DOWN
    
    PSB_START
    PSB_SELECT
    
    PSAB_BLUE
    PSB_GREEN
    PSB_PINK
    PSB_RED
    
    //BUTTON FUNCTIONS
    ps2x.ButtonPressed(button)    //true if just pressed
    ps2x.Button(button)           //true if currently pressed
    ps2x.NewButtonState)(button)  //true if state just changed
    
    
    
    //ANALOG NAMES
    PSS_LY
    PSS_LX
    PSS_RY
    PSS_RX
    
    //ANALOG FUNCTIONS 
    ps2x.Analog(button/stick) //byte, pressure
    */
    
    //wrench lift
    if(ps2x.Button(PSB_R2))
    {
      Serial.println("Triangle pressed");
      winch.write(W_DOWN);
      
    }
    else if(ps2x.Button(PSB_R1))
    {
      Serial.println("Circle pressed");
      winch.write((W_STOP-W_UP)/2); //go up half speed
    }
    else
    {
      winch.write(W_STOP);
    }
    
    
    //stretcher
    if(ps2x.Button(PSB_L2))
    {
      Serial.println("Left dpad pressed");
      stretcherMotor.writeMicroseconds(MOTOROUT);
      
    }
    else if(ps2x.Button(PSB_L1))
    {
      Serial.println("Right dpad pressed");
      stretcherMotor.writeMicroseconds(MOTORIN);
    }
    else
    {
      stretcherMotor.writeMicroseconds(MOTORSTOP);
    }
    
    //stretcher angle
    if(ps2x.Button(PSB_PAD_UP))
    {
      Serial.println("up dpad pressed");
      if(stretcherAnglePos > STRETCHEROUT)
      {
        stretcherAnglePos -= 1;
      }
    }
    else if(ps2x.Button(PSB_PAD_DOWN))
    {
      Serial.println("down dpad pressed");
      if(stretcherAnglePos < STRETCHERBACK)
      {
        stretcherAnglePos += 1;
      }
    }
    stretcherAngle.write(stretcherAnglePos);
    
    mapAndDrive(ps2x.Analog(PSS_LY), ps2x.Analog(PSS_RY));
    delay(10);
  }
}
