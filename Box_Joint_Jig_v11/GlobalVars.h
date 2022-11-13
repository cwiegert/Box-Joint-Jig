/**********************************************************************************
 *    Program:    Global varialbes for a 2 axis stepper motor program.   
 *                2 sets of limit switchs
 *                2 stepper motor drivers
 *                and a structure for setting the steps per inch for moving from selected distance of travel
 *                
 *    Author:     Cory D. Wiegert        
 *    Date:       Sept 5, 2021
 *    Use:        for use with Arduino code, not for reslae, use as is.   Typically included with Nextion UI
 *    version:    v 1.0
 *    
 *    
 *********************************************************/


/*#define FRONT_SLED 4000 
#define BACK_SLED  4005 
#define RIGHT_CAR  4010
#define LEFT_CAR   4015*/

      

    const PROGMEM int FRONT_SLED = 4000; 
    const PROGMEM int BACK_SLED = 4005;
    const PROGMEM int RIGHT_CAR = 4010;
    const PROGMEM int LEFT_CAR = 4015;

    const PROGMEM uint8_t   directionPin = 40;             //  pin that will go to the D+ on the stepper controller  have to declare here, as the stepper is a global class
    const PROGMEM uint8_t    sledDirPin = 38;               //  pin that will go to the D+ on the sled stepper controller.   Have to declare here as the stepper is a global class
    const PROGMEM uint8_t    stepPin = 34;                  //  pin that will wire to the P+ (pulse) on the stepper controlloer   have to declare here as the stepper is a global class
    const PROGMEM uint8_t    sledStepPin = 36;              //  pin that will wire to the P+ (pulse) on the Fence stepper controlloer   have to declare here as the stepper is a global class
    const PROGMEM uint8_t    enablePin = 35;                 //  pin that will wire to the E+ on the stepper controller.   Turns the controller on and off  LOW turns motor on, HIGH turns motor off
    const PROGMEM uint8_t    sledEnablePin = 39;            //  pin that will wire to the E+ on the fence stepper controller.   Turns the controller on and off  LOW turns motor on, HIGH turns motor off
    const PROGMEM int        SD_WRITE = 53;                      //  CS pin of the SD card
    const PROGMEM int        nCutDelay = 200;                   //  microseconds between sled cycles.   Pauses the autocut for a tiny bit so we aren't hammering the motor
    const PROGMEM int        xPin = A2;
    const PROGMEM int        yPin = A1;
    
    /*_______________________________________________________________________________________________________________*/
        
    const PROGMEM byte  RIGHT_EDGE = 0;       //  right edge of a finger
    const PROGMEM byte  LEFT_EDGE = 1;        //  left edge of a finger
    const PROGMEM byte  HOG_OUT = 2;          //  cleaning out the slot
    /*_______________________________________________________________________________________________________________*/
    
    
    byte    RIGHT_LIMIT;
    byte    LEFT_LIMIT;
    byte    FRONT_LIMIT;
    byte    BACK_LIMIT; 
    byte    SAW_ZERO;
    int     iStopPin = 3;           // PIn  for the interupt to stop everything.    must be 2 or 3 on the NANO, 2,3,18,19.on the Mega
    int     initSpeed ;              //  sets the initial speed of the motor.   Don't think we need acceleration in the router, but will find out
   
    long   maxMotorSpeed;                 //  as defined - maximum speed motor will run.   Sets the 100% on the speed slider
    int    maxAcceleration;               //  maximum number of steps for acceleration
    long   workingMotorSpeed;             //  active working speed, set by the slider, and will be somewhere between 0 and 100%
    int    stepsPerRevolution;     //  number of steps required for 1 revolution of leadscrew
    int    microPerStep;              //  number of microSteps.   Set on the TB600 controller by dip switch   Off | On | Off
    float  distPerStep;      //  inches per step with calculated off <microPerStep> = 8
    int    pulseWidthMicros;          //  miliseconds  -- delay for the steps.   the higher the number the slower the lead screw will turn
    int    millisBetweenSteps;        //  milliseconds - or try 1000 for slower steps   delay between pulses sent to the controller
    
    /*_______________________________________________________________________________________________________________*/
    
    long   rightOfBlade;                 //  Right side of blade, used to 0 out carraige prior to starting the cut
    long   leftLimit;                    //  step value when the lower limit switch is hit
    long   rightLimit;                   //  step value when the upper limit switch is hit
    long   backLimit;                    //   step value when the back limit switch is hit
    long   frontLimit;                   //   step value when the front limit switch is hit  (pushed into the saw)
    /*_______________________________________________________________________________________________________________*/
    
    float   kerfSteps;                    //  width of the saw kerf in steps
    float   fingerSteps;                  //  width of the finger in steps
    float   stockSteps;                   //  number of steps for the width of the stock
    float   totFingers;                   //  number of fingers to cut in the stock
    float   stockWidth;                   //  width of the wood stock being cut
    float   firstCutRightSaw;             //  step position of the first cut of a finger;   Used to assess when done hogging flot
    float   zeroPosition;                 //  right edge of stock, used to calculate positioning carraige against blade
    int    whereSaw;                     //  where in the cut process are we?   valid values:  RIGHT_EDGE, LEFT_EDGE, HOG_OUT
    boolean bCarrMoved;                   //  flag to test whether to cycle the sled.   Initialized in NextCut();
    int     currFinger;                   //  current finger that is being cut
    int     fingerCounter = 0;            //  counter for comparing to currFinger
    int    moveCarriage = 22;            //  mechanical button to manually tell carriage to move to next cut    
    uint32_t     bInverted = 0;
    char    errorTxt[200] = {'\0'};
    
    /*_______________________________________________________________________________________________________________*/
    
    long   curPos;                        //  return value from CurrentPosition() call on the stepper
    int   DIRECTION;                     //  tests the direction the motor should be turning
    int   HOME_MOTOR = 1;                //  Used with the swFence switch to tell which motor to use when zeroing and resetting
    int   SET_MOTOR = 1;                 //  Used on the Settings Screen to tell which motor to use when resetting calibrations
    int    stepSize = 3;                  //  used in the moving of the router to set precision on how large the steps should be for the runToPosition() function
    char   CR = '\n';                     //  Carraige return constant, used for the writeDebug function to add a new line at the end of the function
    uint32_t    bStop = 0;                //  used as a flag to test if the stop button has been pressed.
    int     eeAddress;
    int     DEBUG = 1;
    byte    jPin;
    int     xVal;
    int     yVal;
    int     aveRead_x;
    int     aveRead_y;
    byte    jONOff = HIGH;               // joystick power setting   LOW = off, HIGH = on
    int     xAxisLock = 1;
    int     yAxisLock = 1;

    
    /**********************
    My easy way to write to the Serial debugger screen
 **********************/        
    void writeDebug (String toWrite, byte LF)
        {
          if (LF)
          {
            Serial.println(toWrite);
          }
          else
          {
            Serial.print(toWrite);
          }
        
        }

    /**************************************************
          long  calcInches (float  numOfSteps)
               returns length in inches when passed a number of steps.   4wire motor
               and 2 phase stepper.   microsteps / 2 becuse of the 2 motors
               gives 1600 steps per revolution
     *************************************************/
    float calcInches (float numOfSteps)
      {
        return numOfSteps / (microPerStep / 2 ) * distPerStep;
      }
   
    /********************************************************
            float   stepsFromValue ( float fvalue)
    
            will give the number of steps required to move the distance
            passed into the function.   used to convert screen value
            decimals into a number of steps to move the stepper
      ____________________________________________________________*   
    float  stespFromValue ( float fValue)
      {    
        int  bContinue = 1;
      
        eeAddress = 1000;
        while (bContinue)
          {
            EEPROM.get(eeAddress, preSetLookup);
            if (fValue == preSetLookup.decimal)
              {
                return preSetLookup.steps;
              }
            else
              {
                eeAddress += sizeof(preSetLookup);
                if (eeAddress > 4096 )
                  {
                    bContinue = LOW;
                    return 0;
                  }
              }
          }
      }
*/
   /*****************************************************************
           void unPadSpace (char *source)
    
            will remove all spaces from a char array.  The source will be overwritten
            and the contents of the passed char array will be the same characters
            with all spaces stripped.    if you need to retain the value of the char array
            copy the original value to another char array prior to calling this function
     *****************************************************************/
    void unPadSpace (char *source)
      {
      
        char  sRemove[80] = {'\0'};
        int   i = 0;
        int   r = 0;
        int   iLeng = 0;

        while ( source[i] != '\0')
        {
          if (source[i] != ' ')
          {
            sRemove[r] = source[i];
            r++;
          }
          i++;
        }
      
        i = 0;
        while (i < r)
          {
            source[i] = sRemove[i];
            if (DEBUG)
              Serial.println(sRemove[i]);
            i++;
          }
      
        source[i] = '\0';   //null terminate the string to remove residual characters
      }
    
    /****************************************************
         long   calcSteps (float   inchValue)
             returns the number of steps to travel, given the value of inches passed to the function
             4wire motor and 2 phase stepper.   microsteps / 2 becuse of the 2 motors (4 microsteps per step)
             gives 1600 steps per revolution
    ***************************************************/
    float calcSteps (float inchValue) 
      {
        return inchValue / distPerStep * (microPerStep / 2 );
      
      }
    /********************************************************************
     *    void bounceMotorOffLimit (AccelStepper *motor, int iLimit, int iDirection)
     *        function used to bounce the motor off the limit switch, instead of writing reversing
     *        code everywhere, this function will be called
     *        
     *              *motor --- which motor is to be moved
     *              iLimit --- which limit switch is being hit
     *              iDirection -- which direction should it be moved     
     * **********************************************************************/
    void bounceMotorOffLimit (AccelStepper* motor, int iLimit, int iDirection)
        {
          int speed_ ;
          int moveTo_ ; 
          
          if (iDirection)   // moving in a + direction
            {
              speed_ = maxMotorSpeed * .5;
              moveTo_ = 1000;
            }
          else  
            {
              speed_ = -maxMotorSpeed *.5;
              moveTo_ = -1000;
            }
          motor->moveTo (motor->currentPosition() + moveTo_ );
          motor->setSpeed (speed_);
          while (digitalRead(iLimit))
            motor->runSpeed();
        }

    
 