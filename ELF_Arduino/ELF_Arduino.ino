//
// EPS NXT/Arduino LaserTag
// MK, GM
//All spellings shall reflect standard American English
//This version has been commented excessively to help new people who may or may not have programming experience.

//#include <Wire.h>  //Relic from when we were using the NXT.
#include <Servo.h>

#define STATE_NEUTRAL               0    //This program is a state machine. That means that it has several states. The state that it is in is controlled by the variable g_iState. The behavior is determined by the state. These are the possible states.
#define COMMAND_REPORT_HITS         1    //If you go to "Tools" -> "Serial Monitor" and input "1", this will report all values.
#define COMMAND_FIRE_LASER          2    //As advertized.
#define COMMAND_TURN_LASER_LEFT     3    //Often goes center then left. No idea why.
#define COMMAND_TURN_LASER_CENTER   4    //As advertised.
#define COMMAND_TURN_LASER_RIGHT    5    //As advertised.
#define STATE_REPORT_FIRED          6    //Used only internally.
#define COMMAND_FIRE_LASER_LATER    7    //As advertised.
#define COMMAND_REPORT_CALIBRATION  8    //As advertised.
#define COMMAND_REPORT_VALUE        9    //As advertised.
#define STATE_ERROR                 255

#define THRESHOLD_OFFSET   100

#define ELF_MSG_HEADER     199  //Unlikely special value to msg header id

int g_iState;
int g_iCurrentHitCount;
unsigned long g_uLastHit;
int g_iCalibrationValue;
Servo g_sLaserCannon;
int g_iServoDegrees;

void setup()    //Runs when it is turned on.
{
  pinMode (7, OUTPUT);    //Sets pin 7 to output mode.
  delay(1000);
  analogWrite(7, 0);      //Turns off laser.
  Serial.begin(9600);    //9600 is the baudrate. Shouldn't ever need to change it.
  g_iState = STATE_NEUTRAL;
  g_iCurrentHitCount = 0;
  g_uLastHit = 0;
    // initialize gun turret servo
  g_sLaserCannon.attach(12);                //the laser cannon's rotation is controlled by pin 12
  g_iServoDegrees = 90;                    //the rotational position, in degrees, of the laser cannon
  g_sLaserCannon.write(g_iServoDegrees);
  delay(300);
    // calibrate hit detection
  unsigned long uTotal = 0;
  for (int i=0; i < 200 ;i++)
  {
    uTotal += analogRead(0);  //A0
    delay (1);
  }
  g_iCalibrationValue = (int) ((uTotal / 200) + THRESHOLD_OFFSET); 
//  Wire.begin(0x0A);               // join i2c bus with slave address #2.   //Relic from when we were using the NXT.
//  Wire.onRequest(requestEvent);   // Event handler, responds by sending information back to the NXT
//  Wire.onReceive(receiveEvent);   // Event handler, receiving information
  delay(1000);
}

void loop()    //After the setup runs, the loop will run again and again.
{
  readSerial();    //Checks commands from the computer/phone.
  switch (g_iState)
    {
    case COMMAND_REPORT_HITS:
      Serial.print(g_iCurrentHitCount);
      Serial.print(" ");
      Serial.print(analogRead(0));
      Serial.print(" ");
      Serial.println(g_iCalibrationValue);
      g_iState = STATE_NEUTRAL;      
      break;
    case COMMAND_FIRE_LASER: // normal case
      g_iState = STATE_REPORT_FIRED;
      analogWrite(7, 255);  // turn on laser for a quarter second
      delay(1000);
      analogWrite(7, 0);  
      break;
    case COMMAND_TURN_LASER_LEFT:      //turns laser left 15 degrees
      g_iServoDegrees -= 15;
      g_sLaserCannon.write(g_iServoDegrees);
      delay(50);
      g_iState = STATE_NEUTRAL;
      break;      
    case COMMAND_TURN_LASER_CENTER:    //turns laser to the center
      g_iServoDegrees = 90;
      g_sLaserCannon.write(g_iServoDegrees);
      delay(300);
      g_iState = STATE_NEUTRAL;
      break;    
    case COMMAND_TURN_LASER_RIGHT:     //turns laser right 15 degrees
      g_iServoDegrees += 15;
      g_sLaserCannon.write(g_iServoDegrees);
      delay(50);
      g_iState = STATE_NEUTRAL;
      break;    
    case STATE_REPORT_FIRED:
      delay(250);
      g_iState = STATE_NEUTRAL;
      break;
    case COMMAND_FIRE_LASER_LATER: // awkward case were we already reported that we fired
      g_iState = STATE_NEUTRAL;    // don't report after this
      delay(1000);
      analogWrite(7, 255);        // turn on laser for a quarter second
      delay(1000);
      analogWrite(7, 0);
      break;
    case COMMAND_REPORT_CALIBRATION:
      Serial.println(g_iCalibrationValue);
      g_iState = STATE_NEUTRAL;      
      break;
    case COMMAND_REPORT_VALUE:    // leave state untouched, event handlers will reset it
      Serial.print(analogRead(0));
      g_iState = STATE_NEUTRAL;
      break;
    case STATE_ERROR:              
      g_iState = STATE_NEUTRAL;    
      for (int i = 0; i<5; i++)
      {
        analogWrite(7, 255);  //new. turns on laser
        delay(100);
        analogWrite(7, 0);  //new. turns off laser
        delay(100);
      }
     break;
    default:
      // not sure how we got here, but reset
      g_iState = STATE_NEUTRAL;
   }   
  detectHits();
  delay (1);
}

void detectHits()
{
  int iLightValue;
  iLightValue = analogRead(0);  //A0
  if (iLightValue > g_iCalibrationValue)    //If the light value is 100 greater than the calibration value
  {
    if ((millis() - g_uLastHit) > 500)    //And it has been at least a half second
    {
      g_iCurrentHitCount++;    //Then we got hit :(
      g_uLastHit = millis();
    }
  }  
}


void readSerial()
{
  if (Serial.available());    
  {
    int input = Serial.read();
    int availableCommands[10] = {'0','1','2','3','4','5','6','7','8','9'};
    for(int i = 0; i < 10; i++)
      if (input == availableCommands[i])
      {
        g_iState = i;
      }
    //Serial.println(g_iState);
  }
}



/*    From back when we used the NXT
//
// When data is received from NXT, this function is called.
//
void receiveEvent(int iBytesIn)
{
  int iCommand;
  
  //
  // process commands only if 1) right number of bytes, 2) our special msg header and 3) checksum correct
  //
  
  if (iBytesIn == 4) // 3 bytes for our message plus an added "0" of padding
  {
    if (Wire.read() == ELF_MSG_HEADER)
    {
      iCommand = Wire.read();
      if (((Wire.read() + iCommand)%256) == 0)
      {
        //
        // the switch here is overkill, but I am doing that so we can be more specific in the future
        //
        switch (iCommand)
        {
          case COMMAND_FIRE_LASER:
            g_iState = COMMAND_FIRE_LASER;
            break;
            
          case COMMAND_TURN_LASER_LEFT:
          case COMMAND_TURN_LASER_CENTER:
          case COMMAND_TURN_LASER_RIGHT:
            g_iState = iCommand;
            break;

          case COMMAND_REPORT_HITS:
          case COMMAND_REPORT_CALIBRATION:
          case COMMAND_REPORT_VALUE:
            g_iState = iCommand;
            break;
            
          default:
            g_iState = STATE_ERROR;
            break;
        }
      }
    }
  }
    
      
  //
  // Don't need any more data, but process it anyway (in error case)
  // should only be the one padding/trailing/end byte
  //
    
  while (Wire.available())
  {
    Wire.read();
  }
}

//
// Macro splits integer into fields 1 and 2, and puts checksum in 3
//

#define PUT_INT_BYTES(s,i) {s[1] = (byte)i;s[2] = (byte) (((unsigned int)i)>>8); s[3] = (byte) 0-(s[1]+s[2]);}

//
//
void requestEvent()
{
  char s[4];
  int x;
 
  s[0] = ELF_MSG_HEADER;   // special value to denote header for ELF message    

  switch (g_iState)
  {
    
    case COMMAND_REPORT_HITS:
      PUT_INT_BYTES(s, g_iCurrentHitCount); 
      g_iCurrentHitCount = 0;
      g_iState = STATE_NEUTRAL;
      break;

    case COMMAND_REPORT_VALUE:
      x = analogRead(0); 
      PUT_INT_BYTES(s, x);
      g_iState = STATE_NEUTRAL;
      break;

    case COMMAND_REPORT_CALIBRATION:
      PUT_INT_BYTES (s, g_iCalibrationValue); 
      g_iState = STATE_NEUTRAL;
      break;

    case STATE_REPORT_FIRED:
      PUT_INT_BYTES(s,255)
      g_iState = STATE_NEUTRAL;
      break;

    case COMMAND_FIRE_LASER:
      // This is a bit awkward - we are in this handler too early, can't reset the command. That's why we have an alternate command
      PUT_INT_BYTES(s,254)
      g_iState = COMMAND_FIRE_LASER_LATER;
      break;

    //
    // This was the error case, but with the addition of turning I don't
    // want to add more states just to respond properly, so all the NXT will
    // get back is 0.
    //
    default:
      PUT_INT_BYTES(s,0)  
      break;
  }
  //
  // don't forget to send it
  //
  Wire.write(s, 4);
}
*/

