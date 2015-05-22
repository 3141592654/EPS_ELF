//
// EPS NXT/Arduino LaserTag
// MK, GM
//

#include <Wire.h>
#include <Servo.h>

#define STATE_NEUTRAL               0
#define COMMAND_REPORT_HITS         1
#define COMMAND_FIRE_LASER          2
#define STATE_REPORT_FIRED          3
#define COMMAND_FIRE_LASER_LATER    4
#define COMMAND_REPORT_CALIBRATION  5
#define COMMAND_REPORT_VALUE        6
#define COMMAND_TURN_LASER_LEFT     7
#define COMMAND_TURN_LASER_CENTER   8
#define COMMAND_TURN_LASER_RIGHT    9
#define STATE_ERROR                 255

#define THRESHOLD_OFFSET   100

#define ELF_MSG_HEADER     199  // unlikely special value to msg header id

int g_iState;
int g_iCurrentHitCount;
unsigned long g_uLastHit;
int g_iCalibrationValue;
Servo g_sLaserCannon;
int g_iServoDegrees;

void setup()
{
  pinMode (12, OUTPUT);
  pinMode (13, OUTPUT);
  
  analogWrite(13, 255);    // turn on light
  analogWrite(12, 0);      // turn off laser


  g_iState = STATE_NEUTRAL;
  g_iCurrentHitCount = 0;
  g_uLastHit = 0;
  
  //
  // initialize gun turret servo
  //
  
  g_sLaserCannon.attach(6);                //the laser cannon comes out of pin six
  g_iServoDegrees = 90;                    //the rotational position, in degrees, of the laser cannon
  g_sLaserCannon.write(g_iServoDegrees);
  delay(300);

  
  //
  // calibrate hit detection
  //
  
  unsigned long uTotal = 0;
  for (int i=0; i < 200 ;i++)
  {
    uTotal += analogRead(0);
    delay (1);
  }
  g_iCalibrationValue = (int) ((uTotal / 200) + THRESHOLD_OFFSET);
  


  Wire.begin(0x0A);               // join i2c bus with slave address #2
  Wire.onRequest(requestEvent);   // Event handler, responds by sending information back to the NXT
  Wire.onReceive(receiveEvent);   // Event handler, receiving information

  delay(1000);
  analogWrite(13, 0);      // turn off light  
}

void loop()
  {
  int iLightValue;
  
  switch (g_iState)
    {
    case COMMAND_FIRE_LASER: // normal case
      g_iState = STATE_REPORT_FIRED;
      analogWrite(12, 255);  // turn on laser for a quarter second
      delay(250);
      analogWrite(12, 0);  
      break;

    case COMMAND_FIRE_LASER_LATER: // awkward case were we already reported that we fired
      g_iState = STATE_NEUTRAL;    // don't report after this
      analogWrite(12, 255);        // turn on laser for a quarter second
      delay(250);
      analogWrite(12, 0);
      break;

    case STATE_REPORT_FIRED:
    case COMMAND_REPORT_HITS:
    case COMMAND_REPORT_CALIBRATION:
    case COMMAND_REPORT_VALUE:
      // leave state untouched, event handlers will reset it
      break;
      
    case COMMAND_TURN_LASER_LEFT:      //turns laser left 5 degrees
      g_iServoDegrees -= 5;
      g_sLaserCannon.write(g_iServoDegrees);
      delay(50);
      break;
      
    case COMMAND_TURN_LASER_CENTER:    //turns laser to the centre
      g_iServoDegrees = 90;
      g_sLaserCannon.write(g_iServoDegrees);
      delay(300);
      break;    
      
    case COMMAND_TURN_LASER_RIGHT:     //turns laser right 5 degrees
      g_iServoDegrees += 5;
      g_sLaserCannon.write(g_iServoDegrees);
      delay(50);
      break;    
      

      
    case STATE_ERROR:              
      g_iState = STATE_NEUTRAL;    
      for (int i = 0; i<5; i++)
      {
        analogWrite(13, 255);      // turn on light for half a second
        delay(100);
        analogWrite(13, 0);
        delay(100);
      }
     break;

    default:
      // not sure how we got here, but reset
      g_iState = STATE_NEUTRAL;
   }
   
   
  //
  // Hit detection
  //
 
  iLightValue = analogRead(0);
  if (iLightValue > g_iCalibrationValue)
  {
    // currently under fire
    if ((millis() - g_uLastHit) > 500)
    {
      // can only get hit once every half second
      g_iCurrentHitCount++;
      g_uLastHit = millis();
    }
  }  
     
   
     
  delay (1);
}

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
