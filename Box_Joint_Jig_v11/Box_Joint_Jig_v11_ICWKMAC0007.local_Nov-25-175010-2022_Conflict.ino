#define BLYNK_TEMPLATE_ID "TMPLCMSbrrOs"
#define BLYNK_DEVICE_NAME "Box Joint Jig"
#define BLYNK_AUTH_TOKEN "1l5eObE7e-_gO4IdX3W7cqEIBBLfbFqU"

/*********************************************************************
    This code is for a project to automate a Box Joint jig on a tablesaw.   The jig will have a sled to move the wood tenons on the x axis to
    cut the toungs and the groves, and a z axis to move the sled through the saw.

    The original version of the code was written by Cory D. Wiegert and can be found at <fill in git repository>

      Will need to know:
        1)   width of saw blade
        2)   width of the box joint
        3)   width of the material
        4)   Automatic cut or manual cut

     
  NEW VERSION - Cory Wiegert   08/29/2021
    v         Don't know how I am going to write the code yet - but let's get started

    11/12/2022  v. 2.0  -- Added the wifi shield and BLYNK app to have a joystick from a smartphone.   A
                        -- Added logic in the RIGHT_EDGE section of nextCut(), as I don't think there was a move
                            after HOG_OUT was finished
                        -- fixed speed and direction issues after building sled.   The sled motor is not directly 
                            connected to the ball screw.   Instead, it's connected with a set of GT2 pulleys.   All 
                            directions are inverted because the motor turns same direction as ball screw (saves space on the sled)
                        -- added debounce when each of the ball screws hit their end limits
                        -- tuned timing of the cycling of the sled (delayMillisecond () between cuts)  Set global variable for this
                        -- added function for bumping off the limit switches
                        -- averaged out the reading of the X & Y joystick signals (A1,A2)
                        -- READY FOR LIVE TEST
  11/19/2022    CDW     -- LIVE and merge back into main line in git.   it's in production

****************************************************************************************************************************************/
/* Comment the BLYNK_PRINT if you are not in debug mode*/
#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG 
#include <AccelStepper.h>
#include "NextionObjects.h"
#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>
#include <EEPROM.h>
#include "GlobalVars.h"

/****************************************************************************
      HIGH -- arduino constant for 1.   Used for turning the digital pins on and off.  Motor state, HIGH = ON
      LOW  -- arduino constant for 0    Used for turning the digital pins on and off   Motor state, LOW = OFF
               no need to define them here, they are already defined in libraries
*********************************************************************************
   Decare the AccelStepper object and use the FULL2WIRE as a type of stepper motor.
   connecting to the TB6600, it is a 2 wire control.

   The pins must be defined and configured here, they can not be part of the Setup - because sCarriage is set as a global variable
   it must have the buttons defined, prior to the setup
 *************************************************************************/
  
  AccelStepper  sSled (AccelStepper::FULL2WIRE, sledStepPin, sledDirPin, HIGH);
  AccelStepper  sCarriage (AccelStepper::FULL2WIRE, stepPin, directionPin, HIGH);

/***********************************************
     if Blynk capabilities are to be added to the boxjoint program
     the 3 lines below have to be uncommented, and the ESP / blynk include libraries
     need to be uncommented above.
 ********************************************/
/*Hardware Serial on Mega, Leonardo, Micro...*/
#define EspSerial Serial1
ESP8266     wifi(&EspSerial);
BlynkTimer  timer;


int boardMemory = 4098;   // 4K of memory for the Mega board

    /******************************************************
      void  getSettingsScreen(char *verString)
          Function to load all the settings fro the pins and files.   be very careful with this section
          We will have a tokenized read from the config file, and the values should only  be changed in the
          these are the PINS on the arduio where all the digital communication happens
    ********************************************************/
    void   putSettingsScreen( )
    {
      char    tempWriter[60] = {'\0'};
      float   limTemp;
      int     i = 0;
      char    tempBuff[15] = {'\0'};
    
      memset(tempWriter, '\0', sizeof(tempWriter));
      sprintf(tempWriter, "Settings.nLeftLimit.val=%d\0", LEFT_LIMIT);
      nexSerial.write(tempWriter);
      FlushBuffer();
      sprintf(tempWriter, "Settings.nRightLimit.val=%d\0", RIGHT_LIMIT);
      nexSerial.write(tempWriter);
      FlushBuffer();
      sprintf(tempWriter, "Settings.nFrontLimit.val=%d\0", FRONT_LIMIT);
      nexSerial.write(tempWriter);
      FlushBuffer();
      sprintf(tempWriter, "Settings.nBackLimit.val=%d\0", BACK_LIMIT);
      nexSerial.write(tempWriter);
      FlushBuffer();
      sprintf(tempWriter, "Settings.nDirectionPin.val=%d\0", directionPin);
      nexSerial.write(tempWriter);
      FlushBuffer();
      sprintf(tempWriter, "Settings.nSledDirPin.val=%d\0", sledDirPin);
      nexSerial.write(tempWriter);
      FlushBuffer();
      sprintf(tempWriter, "Settings.nStepPin.val=%d\0", stepPin);
      nexSerial.write(tempWriter);
      FlushBuffer();
      sprintf(tempWriter, "Settings.nSledStepPin.val=%d\0", sledStepPin);
      nexSerial.write(tempWriter);
      FlushBuffer();
      sprintf(tempWriter, "Settings.nEnablePin.val=%d\0", enablePin);
      nexSerial.write(tempWriter);
      FlushBuffer();
      sprintf(tempWriter, "Settings.nSledEblPin.val=%d\0", sledEnablePin);
      nexSerial.write(tempWriter);
      FlushBuffer();
      sprintf(tempWriter, "Settings.nPlseWdtMicro.val=%d", pulseWidthMicros);
      nexSerial.write(tempWriter);
      FlushBuffer();
      dtostrf(maxMotorSpeed, 5, 0, tempBuff);
      sprintf(tempWriter, "Settings.tMaxMotorSpeed.txt=\"%s\"\0", tempBuff);
      nexSerial.write(tempWriter);
      FlushBuffer();
      sprintf(tempWriter, "Settings.nMaxAccel.val=%d\0", maxAcceleration);
      nexSerial.write(tempWriter);
      FlushBuffer();
      sprintf(tempWriter, "Settings.nWorkMtrSpd.val=%d\0", workingMotorSpeed);
      nexSerial.write(tempWriter);
      FlushBuffer();
      sprintf(tempWriter, "Settings.nSDWrite.val=%d\0", SD_WRITE);
      nexSerial.write(tempWriter);
      FlushBuffer();
      strcpy(tempBuff, "");
      dtostrf(distPerStep, 1, 8, tempBuff);
      sprintf(tempWriter, "Settings.tDistPerStep.txt=\"%s\"\0", tempBuff);
      nexSerial.write(tempWriter);
      FlushBuffer();
      sprintf(tempWriter, "Settings.nMilisBtwn.val=%d\0", millisBetweenSteps);
      nexSerial.write(tempWriter);
      FlushBuffer();
      dtostrf(initSpeed, 5, 0, tempBuff);
      if (DEBUG)
        Serial.println(tempBuff);
      unPadSpace(tempBuff);
      sprintf(tempWriter, "Settings.tInitSpeed.txt=\"%s\"", tempBuff);
      nexSerial.write(tempWriter);
      FlushBuffer();
      sprintf(tempWriter, "Settings.nStpsPerRev.val=%d", stepsPerRevolution);
      nexSerial.write(tempWriter);
      FlushBuffer();
      memset (tempWriter, '\0', sizeof(tempWriter));
      //     tVersion.setText (verString);
    }
    
    /***************************************************************
          void readEEPROMSettings ()
    
          Used to read the settings from arduino EEPROM.   Called at setup and can be triggered anytime from the UI
     ****************************************************************/
    void readEEPROMSettings()
    {
      int   setCheck = 0;
    
      setCheck = EEPROM[boardMemory - 3];
      bDebug.setValue(EEPROM[boardMemory - 4]);
      DEBUG = EEPROM[4094];
      if (setCheck != 0)
      {
    
        eeAddress = 0;
        EEPROM.get (eeAddress, directionPin);
        eeAddress += sizeof(directionPin);
        EEPROM.get(eeAddress, sledDirPin);
        eeAddress += sizeof(sledDirPin);
        EEPROM.get(eeAddress, stepPin);
        eeAddress += sizeof(stepPin);
        EEPROM.get(eeAddress, sledStepPin);
        eeAddress += sizeof(sledStepPin);
        EEPROM.get(eeAddress, enablePin);
        eeAddress += sizeof(enablePin);
        EEPROM.get(eeAddress, sledEnablePin);
        eeAddress += sizeof(sledEnablePin);
        EEPROM.get(eeAddress, initSpeed);
        eeAddress += sizeof(initSpeed);
        EEPROM.get(eeAddress, stepsPerRevolution);
        eeAddress += sizeof(stepsPerRevolution);
        EEPROM.get(eeAddress, microPerStep);
        eeAddress += sizeof(microPerStep);  
        EEPROM.get(eeAddress, pulseWidthMicros);
        eeAddress += sizeof(pulseWidthMicros);
        EEPROM.get(eeAddress, millisBetweenSteps);
        eeAddress += sizeof (millisBetweenSteps);
        EEPROM.get(eeAddress, RIGHT_LIMIT);
        eeAddress += sizeof (RIGHT_LIMIT);
        EEPROM.get(eeAddress, LEFT_LIMIT);
        eeAddress += sizeof (LEFT_LIMIT);
        EEPROM.get(eeAddress, FRONT_LIMIT);
        eeAddress += sizeof (FRONT_LIMIT);
        EEPROM.get(eeAddress, BACK_LIMIT);
        eeAddress += sizeof (BACK_LIMIT);
        EEPROM.get(eeAddress, SAW_ZERO);
        eeAddress += sizeof(SAW_ZERO);
        EEPROM.get(eeAddress, maxAcceleration);
        eeAddress += sizeof(maxAcceleration);
        EEPROM.get (eeAddress, distPerStep);
        eeAddress += sizeof(distPerStep);   
        EEPROM.get(eeAddress, maxMotorSpeed);
        eeAddress += sizeof(maxMotorSpeed);
        EEPROM.get(eeAddress, workingMotorSpeed);
        eeAddress += sizeof(workingMotorSpeed);
        EEPROM.get(eeAddress, jPin);
        eeAddress += sizeof(jPin);
        
        if ( DEBUG)
        {
          Serial.println("_________________________________________________________________________________________________________________________");
          Serial.print (F("directionPin ==> "));
          Serial.println(directionPin);
          Serial.print (F("sledDirPin ==> "));
          Serial.println(sledDirPin);
          Serial.print(F("stepPin ==> "));
          Serial.println(stepPin);
          Serial.print (F("sledStepPin ==> "));
          Serial.println(sledStepPin);
          Serial.print (F("enablePin ==> "));
          Serial.println(enablePin);
          Serial.print(F("sledEnablePin ==> "));
          Serial.println(sledEnablePin);
          Serial.print (F("initSpeed ==> "));
          Serial.println (initSpeed);
    
          Serial.print(F("stepsPerRevolution ==> "));
          Serial.println(stepsPerRevolution);
          Serial.print(F("microPerStep ==> "));
          Serial.println(microPerStep);
          Serial.print(F("pulseWidthMicros ==> "));
          Serial.println(pulseWidthMicros);
          Serial.print (F("millisBetweenSteps ==> "));
          Serial.println(millisBetweenSteps);
          Serial.print (F("RIGHT_LIMIT ==> "));
          Serial.println(RIGHT_LIMIT);
          Serial.print(F("LEFT_LIMIT ==> "));
          Serial.println(LEFT_LIMIT);
          Serial.print(F("FRONT_LIMIT ==>"));
          Serial.println (FRONT_LIMIT);
          Serial.print (F("BACK_LIMIT ==> "));
          Serial.println(BACK_LIMIT);
          Serial.print (F("SAW_ZERO ==> "));
          Serial.println(SAW_ZERO);
          Serial.print (F("maxAcceleartion ==> "));
          Serial.println(maxAcceleration);
          Serial.print(F("distPerStep ==> "));
          Serial.println(distPerStep, 8);
          Serial.print(F("maxMotorSpeed == > "));
          Serial.println(maxMotorSpeed);
          Serial.print(F("workingMotorSpeed ==> "));
          Serial.println(workingMotorSpeed);
          Serial.print(F("Joystick Power Pin ==> "));
          Serial.println(jPin);             
        }
    
      }
    }

    /******************************************************
     *   void  callError ()
     *      display error dialog and fill it with errorTxt
     * ****************************************************/
    void callError()
      {
        char  sCommand[200] = {'\0'};
        sprintf(sCommand, "Error.tErrorMsg.txt=\"%s\"", errorTxt);
        Error.show(); 
        nexSerial.write(sCommand);
        FlushBuffer();
      }
  
  /********************************************************
     *   byte  setLimitFields( byte  whereFrom)
     *      used to set the dispaly of the carriage and sled fields 
     *      used both when the homing is finished, and when the home screen
     *      is refreshed.   The fields will not reset automatically, as they are not
     *      stored anywhere except the temperal UI while it is active
     * *******************************************************/
    byte setLimitFields (int whereFrom = 0)
      {
        char  tString[80];
        long  setColor = 2016;
        long  notSetColor = 63584;
        long  eeVal;
        
        switch (whereFrom)  //  reset the colors for the carriage
          {
            case 1:         //  Carriage limits values to be cleared and color's changed
              {
                memset(tString, '\0', sizeof(tString));
                sprintf(tString, "Home.nLeftLim.pco=%ld\0", notSetColor);
                nexSerial.write(tString);
                FlushBuffer();
                memset(tString, '\0', sizeof(tString));
                sprintf(tString, "Home.nRighLim.pco=%ld\0", notSetColor);
                nexSerial.write(tString);
                FlushBuffer();
                sprintf(tString, "Home.RighLIm.val=0\0");
                nexSerial.write(tString);
                FlushBuffer();
                break;
              }
            case 2:         //  Sled limits values to be cleared and color change
              {
                memset(tString, '\0', sizeof(tString));
                sprintf(tString, "Home.nSledBkLim.pco=%ld\0", notSetColor);
                nexSerial.write(tString);
                FlushBuffer();
                memset(tString, '\0', sizeof(tString));
                sprintf(tString, "Home.nSledFrntLmt.pco=%ld\0", notSetColor);
                nexSerial.write(tString);
                FlushBuffer();
                break;
              }
          }
        if (!whereFrom)     // setting the values for carriage and sled limits from EEPROM
        {
          memset(tString, '\0', sizeof(tString));
          EEPROM.get(RIGHT_CAR, eeVal);
          sprintf(tString, "Home.nRighLim.val=%ld", eeVal);
          rightLimit = eeVal;
          nexSerial.write(tString);
          FlushBuffer();
          memset(tString, '\0', sizeof(tString));
          EEPROM.get(LEFT_CAR, eeVal);
          sprintf(tString, "Home.nLeftLim.val=%ld", eeVal);
          nexSerial.write(tString);
          leftLimit = eeVal;
          FlushBuffer();
          memset(tString, '\0', sizeof(tString));
          sprintf(tString, "Home.nLeftLim.pco=%ld\0", setColor);
          nexSerial.write(tString);
          FlushBuffer();
          memset(tString, '\0', sizeof(tString));
          sprintf(tString, "Home.nRighLim.pco=%ld\0", setColor);
          nexSerial.write(tString);
          FlushBuffer();
          displaySawPosition (sCarriage.currentPosition(),1, 0);
          memset (tString, '\0', sizeof(tString));
          EEPROM.get(BACK_SLED, eeVal);
          sprintf(tString, "Home.nSledBkLim.val=%ld\0", eeVal);
          nexSerial.write(tString);
          backLimit=eeVal;
          FlushBuffer();
          memset(tString, '\0', sizeof(tString));
          EEPROM.get(FRONT_SLED,eeVal);
          sprintf(tString, "Home.nSledFrntLmt.val=%ld", eeVal);
          nexSerial.write(tString);
          FlushBuffer();
          frontLimit = eeVal;
          memset(tString, '\0', sizeof(tString));
          sprintf(tString, "Home.nSledBkLim.pco=%ld\0", setColor);
          nexSerial.write(tString);
          FlushBuffer();
          memset(tString, '\0', sizeof(tString));
          sprintf(tString, "Home.nSledFrntLmt.pco=%ld\0", setColor);
          nexSerial.write(tString);
          FlushBuffer();
        }
    }

    /************************************************************
       void bSetScrnPushCallback(void *ptr)
    
             Nothing implemented on this slider button yet
    *************************************************************/
    void bSetScrnPushCallback(void *ptr)
      {

      }
    
    /************************************************************
       void bHomePushCallback(void *ptr)
    
             Nothing implemented on this slider button yet
    *************************************************************/
    void bHomePushCallback(void *ptr) 
      {
      }
 
    /************************************************************
       void bSledLimitsPopCallback(void *ptr)
    
         sets the limit switches for the into and back from saw sled
         finished coding CDW 9/13/2021
    
    *************************************************************/
    void bSledLimitsPushCallback(void *ptr) 
      {
        if (!bStop)
          {
            setLimitFields(2);        // clear values and set color to Not Set
            if (homeSled())
              setLimitFields(0);       //  set the displayed limit fields and set color to SET
          }
      }
 
    /************************************************************
       void bCarrLimitsPopCallback(void *ptr)
    
          sets the right and left limits for the moving carriage
          finished coding  CDW 9/13/2021
    
    *************************************************************/
    void bCarrLimitsPopCallback(void *ptr)
      {
        if (!bStop)
          {
            setLimitFields(1);
            if (homeCarriage())
              setLimitFields ();
          }
      }
    
    /************************************************************
         void  bSetParmsPushCallback(void *ptr)
    
           used to read the kerf, finger width and stock width from the screen
           and set the global variables which calculate the steps from the
           inch values entered in the screen   (finished coding for now ==> 9/13/2021)
          11_13_2022 -- may need a small adustment to the finger steps to loosen up the joint
              today - they are tight tight tight.. where the tounges and grooves are exact.
              will try changing the finger steps by just a small bit, which will shrink the toungs and 
              open the grooves.   will try 10 steps, no move.    Set as GLOBAL VARIABLE   jointAdjust;
     ************************************************************/
    void bSetParamsPushCallback (void *ptr)
    {
      char    readTxt[10] = {'\0'};
    
      //clear the varaiables prior to reading the screen.   will check the values when time to do the calcs
      fingerSteps = 0;
      kerfSteps = 0;
      stockWidth = 0;
    
      tKerf.getText (readTxt, sizeof(readTxt));
      if (readTxt[0] != 'W')
          kerfSteps = atof(readTxt);
      memset (readTxt, '\0', sizeof(readTxt));
      tFinger.getText (readTxt, sizeof(readTxt));
      if (readTxt[0] != 'W')
        fingerSteps = atof(readTxt);
      memset(readTxt, '\0', sizeof(readTxt));
      tStockWid.getText (readTxt, sizeof(readTxt));
      if (readTxt[0] != 'W')
        stockWidth = atof(readTxt);
    
      if (fingerSteps && kerfSteps && stockWidth )          // Need to set an error notice if the values are not set in the Nextion
        {
           totFingers = stockWidth / fingerSteps;     // may need to figure out how to find the remainder for the odd size 
           fingerSteps = calcSteps( fingerSteps );
           kerfSteps = calcSteps (kerfSteps);
           stockSteps = calcSteps ( stockWidth );
        }
       if (DEBUG)
        {
          Serial.println("");
          Serial.print (F("total fingers ==> " ));
          Serial.println(totFingers, 4);
          Serial.print (F("finger Steps ==> " ));
          Serial.println(fingerSteps, 4);
          Serial.print (F("kerf Steps ==> "));
          Serial.println(kerfSteps, 4);
          Serial.print (F("stock Steps ==> " ));
          Serial.println(stockSteps, 4);
        }
    }
    
    /************************************************************
     *   void bClearPopCallback (void *ptr)
     *      used to reset the Kerf, slot and stock width values 
     *      and all the variables used in setting the automatic cutting
     * **********************************************************/
    void bClearPopCallback (void *ptr)
      {
        kerfSteps = 0;
        stockWidth = 0;
        fingerCounter = 0;
        fingerSteps = 0;
        totFingers = 0;
        stockSteps = 0;
        whereSaw = 5;
      }
 
    /************************************************************
         void bZeroSawPopCallback(void *ptr)
    
            Sets the edge of the saw to the 0 point using a INPUT_PULLUP
            will reset the left and right limit based on where the 0 limits are set
            after the carraige is 0'd out, the left limit is set to the (-current position)
            right limit is set to the right limit - current position
            finished coding CDW  9/12/2021
     ************************************************************/
    void bZeroPushCallback(void *ptr)
    {
    
      if (!bStop)
      {
        rightLimit = rightLimit- sCarriage.currentPosition();
        leftLimit = leftLimit - sCarriage.currentPosition();
        EEPROM.put(RIGHT_CAR, rightLimit);
        EEPROM.put(LEFT_CAR ,leftLimit);
        sCarriage.setCurrentPosition (0);
        displaySawPosition (sCarriage.currentPosition(), 0, 1);
        setLimitFields();   // by not sending a parameter, the function will default to 0, and just set new limit fields
      }
    }
   
    /*************************************************
     *  void  bZeroReturnPushCallback (void *ptr)
     *      button used to return the carriage back to the 0 mark of the blade
     *************************************************/
    void bZeroSawPopCallback (void *ptr)
      {
        if (!bStop)
          {
            sCarriage.moveTo(0);  
            if (sCarriage.currentPosition() > 0)
              sCarriage.setSpeed (-maxMotorSpeed);
            else
              sCarriage.setSpeed(maxMotorSpeed);
            while (sCarriage.currentPosition() != sCarriage.targetPosition())
                sCarriage.runSpeed();
            displaySawPosition (sCarriage.currentPosition(),0, 1); 
            whereSaw = RIGHT_EDGE;  
            fingerCounter= 0;
          }
      }

    /***************************************************************
 *    void btInvertedPushCallback (void *ptr)
 *        select whether to cut a groove or tung first
 *        if Value == HIGH, there will be a tounge (inverted button pushed)
 *        if Value == LOW, there will be a groove first (inverted button not pushed)
 *        
 *        bInverted is initialized to 0
 * ******************************************************************/
    void btInvertPopCallback (void *ptr)
        {
            btInvert.getValue(&bInverted);
        }
 
    /***********************************************************
          void btOnOffPopCallback (void *ptr)
     ***********************************************************/
    void btOnOffPushCallback(void *ptr)
      {
        btOnOff.getValue(&bStop);
        if (bStop)
          stopMotor();
        else
          startMotor();
          
      }
   
    /*************************************************************
    *   void  btJoyPushCallback (void *ptr)
    *     button call back to turn the joystick on and off
    *     when off - the joystick will not respond.   Turn it off so
    *     it is not used in the middle of automatic cutting
    * ***********************************************************/
    void btJoyPushCallback (NexDSButton *ptr)
      {
        uint32_t read_;
        ptr->getValue(&read_);
        if (read_)
          jONOff = HIGH;
        else
          jONOff = LOW;
      }

    /************************************************************
       void bRefreshPushCallback(void *ptr)
          Read the EEPROM again, fill the settings screen
          return to the Home screen
    *************************************************************/
    void bRefreshPushCallback(void *ptr) 
      {
        readEEPROMSettings();
        putSettingsScreen();
      }
 
    /*******************************************************************
     *    void   bAutoStartPushCallback (void *ptr)
     *        only called when start button is visible, it should be disabled once the program starts
     *        only re-enabled if there is a stop in the program.    
     *        Calls nextCut() where all logic for moving the carriage
     *        then, upon success, will move the sled to front limit and back again
     *        
     *******************************************************************/
    void bStartAutoPushCallback (void *ptr)
       {
          int curRead = -1;
          if (!bStop)
            {           
              if (!kerfSteps)     // Parameters button has not set the visual parameters to code
                {
                  strcpy(errorTxt, "\n\n\n\tsetting the saw parameters");
                  callError ();
                  bSetParamsPushCallback (&bSetParams);
                }
              if (kerfSteps)
                {
                  if (sSled.currentPosition () != backLimit)
                    {
                      sSled.moveTo (backLimit);
                      sSled.setSpeed( maxMotorSpeed * .5);
                      while (!digitalRead (BACK_LIMIT))
                        sSled.runSpeed();
                      if (digitalRead (BACK_LIMIT))
                        bounceMotorOffLimit (&sSled, BACK_LIMIT, LOW);
                    }
                  int jTemp = jONOff; 
                  jONOff = LOW;
                  FlushBuffer();
                  zeroPosition = sCarriage.currentPosition();
                  while (nextCut()  && !bStop && sCarriage.currentPosition() < stockSteps)   
                    {
                      delayMicroseconds(nCutDelay);
                      if (bCarrMoved)
                        cycleSled();
                      delayMicroseconds(nCutDelay);
                      curRead = nexSerial.read();
                      if (curRead == 101)
                        {
                          curRead = nexSerial.read();     // read the page      
                          curRead = nexSerial.read();     // read the control
                          if (curRead == 34)             // pressed the stop button  
                            btOnOffPushCallback(&btOnOff); // process the stop button
                        }
                    }
                  bZeroSawPopCallback(&bZeroSaw);
                }
            }
       }
 
    /*********************************************************************
     *    void   bNextCutPushCallback (void *ptr)
     *        used to move to the next cut in the sequence.   Same as the mechanical button
     *        but this 1 can be used from the screen
     **********************************************************************/
    void bNextCutPushCallback (void *ptr)
      {
          if (!bStop)
            {
              if (!kerfSteps)
                {
                  strcpy(errorTxt, "\n\n\n\tsetting the saw parameters");
                  callError ();
                  bSetParamsPushCallback (&bSetParams); 
                }
              if (kerfSteps)
                if (!nextCut())
                  bZeroSawPopCallback(&bZeroSaw);
            }          
      }
    
    /************************************************************
       void bDebugPushCallback(void *ptr)
    
             Nothing implemented on this slider button yet
    *************************************************************/
    void bDebugPopCallback(void *ptr) 
      {
        uint32_t  setDebug;
      
        bDebug.getValue(&setDebug);
        EEPROM[4094] = setDebug;
        DEBUG = setDebug;
        if (DEBUG)
          {
            Serial.begin(250000);
            delay (500);
            Serial.println("Turned the debug on");
          }
        else
          {
            Serial.println("turning the debugger off");
            delay(500);
            Serial.end();
          }
      }
    
    /************************************************************
       void bSledStopPushCallbackvoid *ptr)
          button used to set the travel distance of the sled.   We don't want to cycle 
          all the way to the FRONT_LIMIT if we do not have to .   FRONT_LIMIT will 
          be the safety stop after we cut the clot.   This button will put a step value
          into the tSawStop adn that field will be used as the front limit
    *************************************************************/
    void bSledStopPushCallback(void *ptr) 
      {
        long  lSawSteps;
        char  sSteps[12] = {'\0'};

        sprintf(sSteps, "%ld", sSled.currentPosition());
        tSawStop.setText (sSteps);
        FlushBuffer();
      }
/************************************************************
       void bLeftPushCallbackvoid *ptr)
          Button used to jog the carriage left, similar to the joystick
    *************************************************************/
    void bLeftPushCallback(void *ptr) 
      {
          int cBuff = -1;

          sCarriage.move (sCarriage.currentPosition() -500000);
          sCarriage.setSpeed (-maxMotorSpeed *.8);        
          while (!digitalRead(LEFT_LIMIT) && cBuff == -1 && !bStop)
            {     
              sCarriage.runSpeed();
              cBuff = nexSerial.read ();
            }
          if (digitalRead(LEFT_LIMIT))
            bounceMotorOffLimit (&sCarriage, LEFT_LIMIT, HIGH);
          displaySawPosition (sCarriage.currentPosition(), 0, 0);
      }
 
/************************************************************
       void bUpPushCallbackvoid *ptr)
          Button used to jog the carriage left, similar to the joystick
    *************************************************************/
    void bUpPushCallback(void *ptr) 
      {
        int cBuff = -1;
     
          sSled.move (sSled.currentPosition() -500000);
          sSled.setSpeed (-maxMotorSpeed *.8);
          while (!digitalRead(FRONT_LIMIT) && cBuff == -1 && !bStop)
          {     
            sSled.runSpeed();
            cBuff = nexSerial.read ();
          }
         if (digitalRead(FRONT_LIMIT) && cBuff == -1)    // sled has hit the front limit and I need to bounce it
            bounceMotorOffLimit (&sSled, FRONT_LIMIT, HIGH);
      }

/************************************************************
       void bRightCallbackvoid *ptr)
          Button used to jog the carriage Right, similar to the joystick
    *************************************************************/
    void bRightPushCallback(void *ptr) 
      {
          int cBuff = -1;
        
          sCarriage.moveTo (sCarriage.currentPosition() +500000);
          sCarriage.setSpeed (maxMotorSpeed *.8 );
          while (!digitalRead(RIGHT_LIMIT) && cBuff == -1 && !bStop)
            {     
              sCarriage.runSpeed();
              cBuff = nexSerial.read ();
            }
          if (digitalRead(RIGHT_LIMIT))
            bounceMotorOffLimit (&sCarriage, RIGHT_LIMIT, LOW);
          displaySawPosition (sCarriage.currentPosition(), 0, 0);  
      }

/************************************************************
       void bDownCallbackvoid *ptr)
          Button used to jog the carriage Down, similar to the joystick
    *************************************************************/
    void bDownPushCallback(void *ptr) 
      {
          int cBuff = -1;    
          
          sSled.move (sSled.currentPosition() +500000);
          sSled.setSpeed (maxMotorSpeed *.8);
          while (!digitalRead(BACK_LIMIT) && cBuff == -1 && !bStop)
            {     
              sSled.runSpeed();
              cBuff = nexSerial.read ();
            }
          if (digitalRead(BACK_LIMIT))
            bounceMotorOffLimit (&sSled, BACK_LIMIT, LOW);
      }
   
    /**************************************************
          long zeroToBlade();
             used to set the working 0 for the left side of the saw blad.   Used with a INPUT_PULLUP
             conductor to stop the carriage from moving left when the marker hits the blade.
             RETURNS==>  the posiiton of the Carriage ball screw as steps from the 0 position.
             For the funciton to work, the homeCarriage function had to be called to set the left limit if the Carriage
             ball screw
    
     **************************************************/
    long  zeroToBlade()
      {
        //   test whether or not we should call homeCarriage when setting the 0.
        if (!bStop)
        {
          sCarriage.move(leftLimit - 2000);
          while (!digitalRead(LEFT_LIMIT) && !bStop && digitalRead(RIGHT_LIMIT) && sCarriage.currentPosition() != leftLimit - 2000)
            sCarriage.run();
          sCarriage.move(sCarriage.currentPosition() + 200);
          sCarriage.setSpeed(400);
          while (!digitalRead(SAW_ZERO) && !bStop)
            sCarriage.runSpeed();
          fingerCounter=0;
          whereSaw = RIGHT_EDGE;
          return sCarriage.currentPosition();
        }
      }
     
    /**************************************************
          void homeCarriage()
             set the left and right limits for the travel on the ball screw for the carriage
     **************************************************/
    int homeCarriage()
      {
        if (!bStop)
        {
          
          sCarriage.moveTo(sCarriage.currentPosition() - 200000); 
          sCarriage.setSpeed(-maxMotorSpeed);
          while (!digitalRead(LEFT_LIMIT) && !bStop)
            sCarriage.runSpeed();  
          if (bStop)
            {
              stopMotor();
              return LOW;
            }
      //Bounce off the limit switch
          if (digitalRead(LEFT_LIMIT))
            bounceMotorOffLimit (&sCarriage, LEFT_LIMIT, HIGH);
              
          leftLimit = 0;
          EEPROM.put(LEFT_CAR,leftLimit);     //  write the limit to EEPROM, store for next time
          sCarriage.setCurrentPosition (0);
          if (bStop)
            {
              stopMotor();
              return 0;
            }
            
          // Now run to the other end of the ball screw to set the right limit of the sled
          delayMicroseconds(nCutDelay);
          sCarriage.moveTo( 300000);
          sCarriage.setSpeed((maxMotorSpeed));
          //sCarriage.setAcceleration (2000);
          while (!digitalRead(RIGHT_LIMIT) && !bStop)
            {
              sCarriage.runSpeed(); 
            }      
 
          if (bStop)
          {
            stopMotor();
            return LOW;
          }
      //Bounce the carriage off the limit stop    
          if (digitalRead(RIGHT_LIMIT))
            bounceMotorOffLimit (&sCarriage, RIGHT_LIMIT, LOW);
          if (bStop)
            {
              stopMotor();
              return LOW;
            }
          rightLimit = sCarriage.currentPosition();
          EEPROM.put(RIGHT_CAR, rightLimit);
          // Move the sled back to the 0 position on the left limit switch
          sCarriage.moveTo(leftLimit - 100);
          sCarriage.setSpeed ((-maxMotorSpeed *.9));
          while ((!digitalRead(LEFT_LIMIT)) && !bStop)
            sCarriage.runSpeed();  
          if (digitalRead(LEFT_LIMIT))
            bounceMotorOffLimit (&sCarriage, LEFT_LIMIT, HIGH);
  
          if (bStop)
          {
            stopMotor();
            return LOW;
          }
          return HIGH;
        }
        return LOW;
      }
    
    /**************************************************
          void homeSled()
             set the back and front limitss for the travel on the ball screw for the sled
     **************************************************/
    int homeSled()
    {
      btOnOff.getValue(&bStop);
      if (!bStop)
        {
          
          sSled.moveTo(sSled.currentPosition() + 500000);
          sSled.setSpeed (maxMotorSpeed *.8);

          while (!digitalRead(BACK_LIMIT) && !bStop)
            sSled.runSpeed();          
          
          if (bStop)
            {
              stopMotor();
              return LOW;
            } 
          if (digitalRead(BACK_LIMIT))
            bounceMotorOffLimit (&sSled, BACK_LIMIT, LOW);
    
          backLimit = 0;
          EEPROM.put(BACK_SLED,backLimit);
          sSled.setCurrentPosition (0);
          // Now run to the other end of the ball screw to set the front limit of the sled
          sSled.moveTo(sSled.currentPosition() -300000);
          sSled.setSpeed (-maxMotorSpeed *.8 );

          while (!digitalRead(FRONT_LIMIT) && !bStop)
            sSled.runSpeed();                 //  run sled through saw          
      
          if (bStop)
            {
              stopMotor();
              return LOW;
            }
          if (digitalRead(FRONT_LIMIT))
            bounceMotorOffLimit (&sSled, FRONT_LIMIT, HIGH);
          frontLimit = sSled.currentPosition();
          EEPROM.put(FRONT_SLED, frontLimit);
          
          sSled.moveTo (backLimit + 50);
          sSled.setSpeed (maxMotorSpeed * .8);
          // Move the sled back to the 0 position on the left limit switch
          while (!digitalRead(BACK_LIMIT))
            sSled.runSpeed();
              
          if (digitalRead(BACK_LIMIT))
            bounceMotorOffLimit (&sSled, BACK_LIMIT, LOW);
  
          if (bStop)
          {
            stopMotor();
            return LOW;
          }
          return HIGH;
        }
      return LOW;
    }
      
    /**************************************************
     *     bool   cycleSled ()
     *          move the sled to the front limit, then back to the back limit 
     *          1 cycle, will be 1 cut through saw
     *          bounce off back limit switch
     ***************************************************/
    bool  cycleSled()
       {
          long  lSledLimit;
          char  sSledLimit[12] = {'\0'};
          bool  bLim;

          tSawStop.getText (sSledLimit, sizeof(sSledLimit));
      
          lSledLimit = atol ( sSledLimit);
          bLim = HIGH;
          if(!bStop)
            {
              if (!bLim)
                sSled.moveTo (frontLimit);
              else
                sSled.moveTo (lSledLimit);
              sSled.setSpeed(-maxMotorSpeed *.8);
              do                                //  run sled through saw
                {
                  sSled.runSpeed();
                }
              while (!digitalRead(FRONT_LIMIT) && sSled.currentPosition() != sSled.targetPosition());

              sSled.moveTo(backLimit);
              delayMicroseconds(nCutDelay);
              sSled.setSpeed((maxMotorSpeed *.8));
              do                               // bring sled back to starting position
                {
                  sSled.runSpeed();
                }  while (!digitalRead(BACK_LIMIT));
              
              if (digitalRead(BACK_LIMIT))
                bounceMotorOffLimit (&sSled, BACK_LIMIT, LOW);
              }
       }
 
    /*********************************************************************
          bool   nextCut( )
    
              logic in this function to figure out the next step of the figer hog out.
              Need to keep track of where we are in the cutting -- right, left edges or hogging out the space
    
              RETURNS:  TRUE or FALSE, if finished, will return FALSE
     **********************************************************************/
    bool   nextCut()
    {  
      bCarrMoved = false;
  
       if (sCarriage.currentPosition()  >= stockSteps)//   beyond the edge of the stock, don't need to cut anything more
         {  
          return LOW;
         }
            
       switch (whereSaw)
         {
            case RIGHT_EDGE:                      // check that we haven't finished, move to new finger
              {
                if (fingerCounter == 0)                                            // FIRST CUT WHEN STARTING 
                  {  
                     if (bInverted)
                      {
                        // .  Do I want to move KerfSteps + fingerSteps here?
                        sCarriage.moveTo (sCarriage.currentPosition() +   fingerSteps + (kerfSteps));
                        sCarriage.setSpeed (maxMotorSpeed);
                        while (sCarriage.currentPosition() != sCarriage.targetPosition())
                          sCarriage.runSpeed();
                        whereSaw = LEFT_EDGE;
                        firstCutRightSaw = sCarriage.currentPosition();
                        movingRightEdge = firstCutRightSaw;
                        bCarrMoved = true;
                        fingerCounter++;
                        break;
                      }                        
                     else
                      {
                        firstCutRightSaw = sCarriage.currentPosition();
                        movingRightEdge = firstCutRightSaw;
                        sCarriage.moveTo(kerfSteps);
                        sCarriage.setSpeed (maxMotorSpeed);
                        while (sCarriage.currentPosition() != sCarriage.targetPosition())
                            sCarriage.runSpeedToPosition();
                        fingerCounter = fingerCounter +1;
                        bCarrMoved = true;
                        whereSaw = LEFT_EDGE;
                        
 
                       break;
                      }
                  }
                else                                              //May need logic here to but the last little remnant from left side of stock  
                  {
                    sCarriage.moveTo (firstCutRightSaw + kerfSteps + (fingerCounter*fingerSteps));

                  }  
                if (sCarriage.currentPosition() <= stockSteps)            
                    {                
                      // the distance to move was set in the previous If/Else statement
                      sCarriage.setSpeed(maxMotorSpeed);
                      while (sCarriage.currentPosition() != sCarriage.targetPosition())
                        sCarriage.runSpeedToPosition();
                      movingRightEdge = sCarriage.currentPosition();
                      whereSaw = LEFT_EDGE; 
                      bCarrMoved = true;
                    }
              //  displaySawPosition (sCarriage.currentPosition(), 0);
                break;
              }
            case LEFT_EDGE:               // cut the outer edge of the finger, set the status to hog out the middle
              {
                 if (sCarriage.currentPosition() - kerfSteps >= stockSteps)//   beyond the edge of the stock, don't need to cut anything more
                  {
                    displaySawPosition (sCarriage.currentPosition(), 0, 1);
                    return LOW;
                  }
                if (fingerCounter == 1)
                  if (!bInverted)
                    sCarriage.moveTo ( firstCutRightSaw + (fingerCounter  * fingerSteps) + jointAdjust);
                  else
                    sCarriage.moveTo ((firstCutRightSaw - kerfSteps) + (fingerCounter  * fingerSteps) + jointAdjust);
                else
                  sCarriage.moveTo ( firstCutRightSaw + (fingerCounter +1) * fingerSteps + jointAdjust);

                sCarriage.setSpeed(maxMotorSpeed);
                while (sCarriage.currentPosition() != sCarriage.targetPosition() && !digitalRead(RIGHT_LIMIT))
                  {
       //             writeDebug ("current Position==> " + String(sCarriage.currentPosition()) + " targetPosition ==> " + String (sCarriage.targetPosition()), 1);
                    sCarriage.runSpeed();
                  }
                displaySawPosition (sCarriage.currentPosition(), 0, 1);
                bCarrMoved = true;
                whereSaw = HOG_OUT;
               break;
                
              }
            case HOG_OUT:         // check to see if we are goign to go past the end of the right edge, if not, start hogging
                                  //   if so, move the blade to just past the start of the kerf on the right edge and finish hogging
              {                  
     
                if (sCarriage.currentPosition() - (.75 * kerfSteps) > movingRightEdge)
                  {
                    sCarriage.moveTo (sCarriage.currentPosition() - (.75 * kerfSteps));
                    sCarriage.setSpeed(-maxMotorSpeed );
                   // sCarriage.runSpeedToPosition();
                    while (sCarriage.currentPosition() != sCarriage.targetPosition())
                      sCarriage.runSpeed();
                    bCarrMoved = true;
                    break;
                  }
                if (bInverted && fingerCounter == 1)
                  movingRightEdge += kerfSteps;  
                if (sCarriage.currentPosition() - (.75 * kerfSteps) < movingRightEdge)
                  {
                    whereSaw = RIGHT_EDGE;
                    bCarrMoved = false;
                    if (bInverted && fingerCounter == 1)
                      {
                        movingRightEdge = movingRightEdge - (2 * kerfSteps);
                        firstCutRightSaw = movingRightEdge;
                      }
                    //fingerCounter = fingerCounter + 2;
                    if (fingerCounter == 1)
                      fingerCounter +=1;
                    else
                      fingerCounter +=2 ;
                    break;
                  }
              }  
         }
       displaySawPosition (sCarriage.currentPosition(), 0, 1);
       return HIGH;                 
     }

    /************************************************
     *    void  displaySawPosition (long sawPos)
     *        setting the display on the UI for the current saw position 
     *        will set it in steps
     ************************************************/
    void  displaySawPosition(long sawPos, int howTo, int progBar)
       {
          char  sCommand[40] = {'\0'};
          char  s[9] = {'\0'};
    
          dtostrf(calcInches(sawPos), 4, 4, s);
          if (!sawPos)
            sprintf(sCommand, "tCurPos.txt=\"0\"");
          else
            sprintf(sCommand, "tCurPos.txt=\"%s\"", s);
          nexSerial.write(sCommand);
              
          FlushBuffer();
         if (progBar) 
          {
            memset(sCommand, '\0', sizeof(sCommand));
            float  val = calcInches(sawPos)/calcInches(stockSteps);
            if (int(val*100) < 100)
              sprintf(sCommand, "jProgress.val=%d", int(val*100));
            else
              sprintf(sCommand, "jProgress.val=100");
            nexSerial.write(sCommand);
            FlushBuffer();
          }
       }

   /***************************************************************
     *    void stopMotor()
     *      turns the motor off and resets the buttons
     *      by turning off the enablePins, the motors will hard stop
     *      and have to be re-enabled by another button on the UI
     * ****************************************************************/
    void stopMotor ()         
      {
        char  sTemp [25];
        bStop = HIGH;
        btOnOff.setValue(HIGH);
        memset( sTemp, '\0', sizeof(sTemp));
        sprintf(sTemp, "btOnOff.val=1");
        nexSerial.write (sTemp);
        FlushBuffer();

        digitalWrite (enablePin, HIGH);
        digitalWrite (sledEnablePin, HIGH);
        
      }
    /***************************************************************
     *    void startMotor()
     *      turns the motor off and resets the buttons
     *      by turning off the enablePins, the motors will hard stop
     *      and have to be re-enabled by another button on the UI
     * ****************************************************************/
    void startMotor ()         
    {
      char  sTemp [25];
      memset( sTemp, '\0', sizeof(sTemp));
      sprintf(sTemp, "btOnOff.val=0");
      nexSerial.write (sTemp);
      FlushBuffer();
    
      bStop = LOW;
      btOnOff.setValue(LOW);
      //btOnOff.setText("Motors\r On");
      digitalWrite (enablePin, LOW);
      digitalWrite (sledEnablePin, LOW);  
    }
    
    BLYNK_WRITE(V10)      // Joystick input
  { 
    int xAxis = param[0].asInt();
    int yAxis = param[1].asInt(); 
    int Xspeed_ = 0;
    int Yspeed_ = 0;

    //  Center of the joystick is 128, 128   
    if (xAxis > 128 && !xAxisLock)      // move Right       
      {
        Xspeed_ = map(xAxis, 128, 255, 0 , 5000);
        moveMotor (&sCarriage, Xspeed_, 100);

      }
    else if (xAxis < 110 && !xAxisLock )    // move Left
      {
        Xspeed_ = map(xAxis, 0, 128, -5000, 0);
        moveMotor (&sCarriage, Xspeed_, 100);

      }
    if (yAxis > 128 && !yAxisLock)        // move Forward
      {
        Yspeed_ = map (yAxis, 128, 255,0, 5000);
        moveMotor (&sSled, Yspeed_, 100);
      }
    else if (yAxis < 128 && !yAxisLock)   // move Back
      {
        Yspeed_ = map (yAxis, 0, 128, -5000, 0);
        moveMotor (&sSled, Yspeed_, 100);

      }
  }

    BLYNK_WRITE (V6)      // Lock the x Axis
    {
      xAxisLock = param.asInt();
    }
  
    BLYNK_WRITE(V7)       // Lock the Y Axis
    {
      yAxisLock = param.asInt();
    }

    
    BLYNK_WRITE(V5)
      {
        int status;
        status = param.asInt();
        if (status)
          digitalWrite(enablePin, LOW);
        else
          digitalWrite(enablePin, HIGH);
      } 

    BLYNK_WRITE(V9)
      {
        jONOff = param.asInt();
      }
  
  /**************************************************************
     *   void moveMotor (AccelStepper *, int)
     *      used as a single functiont to move either motor.  Using the pointer
     *      to call out the specific motor 
     *      is used by the Blynk joystick control, to move a step per call
     * *************************************************************/
    void moveMotor (AccelStepper *motor, long howFast, int howMany)
      { 
         motor->move(howMany);
         motor->setSpeed (howFast);
         //while (!digitalRead(FRONT_LIMIT) && !digitalRead(BACK_LIMIT) && !digitalRead(LEFT_LIMIT) && !digitalRead(RIGHT_LIMIT))
            motor->runSpeedToPosition();
      }
  
  /*************************************************************************
     *      void setJoyAverage ()
     *        setting the average analog signal for the joystick.  Doing this will 
     *        stop the jitter from raw reads.    The logic in the loop will only allow the motors to 
     *        move if the delta from the average is large enough
     ***************************************************************************/
    void  setJoyAverage()
        {
            float xAv_ = 0;
            float yAv_ = 0;

            for (int x=0; x< 50; x++)
              {
                xAv_  += analogRead(xPin);
                yAv_  += analogRead(yPin);
                delayMicroseconds (50);
              }
            aveRead_x = xAv_ / 50;
            aveRead_y = yAv_ / 50;
        }
    
 //SETUP   *********************************** 
    void setup() {
      
      DEBUG = HIGH;
      if (DEBUG)
        {
          Serial.begin (1000000);
          delay(500);
        }

      nexInit(250000);        // start the serial port for the Nextion HMI 
      readEEPROMSettings();
      putSettingsScreen();

/*-------------------------------------------------------------*/
      bSetScrn.attachPush(bSetScrnPushCallback, &bSetScrn);
      bHome.attachPush(bHomePushCallback, &bHome);
      bDebug.attachPop(bDebugPopCallback, &bDebug);
      bRefresh.attachPush(bRefreshPushCallback, &bRefresh);
      bSledLimits.attachPush(bSledLimitsPushCallback, &bSledLimits);
      bCarrLimits.attachPop(bCarrLimitsPopCallback, &bCarrLimits);
      bZeroSaw.attachPop(bZeroSawPopCallback, &bZeroSaw);
      btOnOff.attachPush(btOnOffPushCallback, &btOnOff);
      bSetParams.attachPush (bSetParamsPushCallback, &bSetParams);
      bClear.attachPop (bClearPopCallback, &bClear);
      bZero.attachPush(bZeroPushCallback, &bZero);
      bNextCut.attachPush(bNextCutPushCallback, &bNextCut);
      bStartAuto.attachPush(bStartAutoPushCallback, &bStartAuto);
      btInvert.attachPop(btInvertPopCallback, &btInvert);
      bSledStop.attachPush(bSledStopPushCallback, &bSledStop);   
    /*-----------------------------------------------------*/  
      bLeft.attachPush(bLeftPushCallback, &bLeft);  
      bRight.attachPush(bRightPushCallback, &bRight);  
      bUp.attachPush(bUpPushCallback, &bUp);  
      bDown.attachPush(bDownPushCallback, &bDown);  
    /*-----------------------------------------------------*/
      pinMode (sledEnablePin, OUTPUT);
      pinMode (sledStepPin, OUTPUT);
      pinMode (sledDirPin, OUTPUT); 
      sSled.enableOutputs();
      pinMode (enablePin, OUTPUT);
      pinMode(directionPin, OUTPUT);
      pinMode(stepPin, OUTPUT);
      sCarriage.enableOutputs();
    /*-----------------------------------------------------*/
      pinMode (BACK_LIMIT, INPUT_PULLUP);             // activate the pin for the back of the saw side of the sled 
      pinMode (FRONT_LIMIT, INPUT_PULLUP);            // activate the pin for the saw side of the sled
      pinMode (LEFT_LIMIT, INPUT_PULLUP);             // activate the pin for the left limit switch of the carriage
      pinMode (RIGHT_LIMIT, INPUT_PULLUP);            // activate the pin for the right limit switch of the carriage
      pinMode (iStopPin, INPUT_PULLUP);               // activate the pin used for the interrupt and emergency kill switch
      pinMode (SAW_ZERO, INPUT_PULLUP);               // activate the pin for the wand to set the carriage to 0 point agains the saw
      pinMode (moveCarriage, INPUT_PULLUP);           // activate the pin for the mechanical button that moves the carriage to next position
      pinMode (xPin, INPUT);
      pinMode (yPin, INPUT);
    //  pinMode (jPin, INPUT);
      
    /*-----------------------------------------------------*/  
      sSled.setAcceleration (maxAcceleration);
      sSled.setMaxSpeed(maxMotorSpeed);
      sSled.setMinPulseWidth(pulseWidthMicros);
    /*-----------------------------------------------------*/
      sCarriage.setMinPulseWidth(pulseWidthMicros);
      sCarriage.setMaxSpeed(maxMotorSpeed);
      sCarriage.setAcceleration(maxAcceleration);
      sCarriage.setCurrentPosition (0);
      /*-----------------------------------------------------*/  

      stopMotor();
      bStop = HIGH;
      jProgress.setValue(0);
      setLimitFields(0);
    
   
      jONOff = LOW;
      
     // setJoyAverage();              // adjust for jitter in the analog reading.   Set the average voltage
    
      EspSerial.begin(115200);      // Start the wifi shield and connect to local wifi
      delay(20);
      //Blynk.begin(BLYNK_AUTH_TOKEN, wifi, "Everest 2.4G", "************");
     // Blynk.begin(BLYNK_AUTH_TOKEN, wifi, "Everest", "67NorseSk!");
      //Blynk.virtualWrite(V6, xAxisLock);
      //Blynk.virtualWrite(V7, yAxisLock);
      //Blynk.virtualWrite(V9, jONOff);
      //Blynk.syncAll(); 
      tSawStop.setText ("--READY--");
      FlushBuffer();
      
    }   //   END of SETUP

 // RUN THE LOOP  
    void loop() 
      {
       int x = 0;
  //   jONOff = HIGH;
          
     // if (jONOff)                         // if the Joystick is enabled, move sled or carriage
     //   {
       /*  xVal = analogRead(A2);
          yVal = analogRead (A1); 
 
          if((xVal-aveRead_x)< -100)      // move LEFT, with (-) motor speed
            {
              digitalWrite(directionPin, LOW);
              sCarriage.move (sCarriage.currentPosition () - 100);
              sCarriage.setSpeed (-maxMotorSpeed);
              while (!digitalRead (LEFT_LIMIT) &&  x < 100)
                {
                  sCarriage.runSpeed();
                  x++;
                }
              if (digitalRead(LEFT_LIMIT))
                bounceMotorOffLimit (&sCarriage, LEFT_LIMIT, HIGH);
            }
        
          if((xVal-aveRead_x) > 100)      //  move RIGHT with (+) motor speed
            {
              sCarriage.move (sCarriage.currentPosition () + 100);
              sCarriage.setSpeed (maxMotorSpeed);
              
              while (!digitalRead (RIGHT_LIMIT) &&  x < 100)
                {
                  sCarriage.runSpeed();
                  x++;
                }
              if (digitalRead(RIGHT_LIMIT))
                bounceMotorOffLimit (&sCarriage, RIGHT_LIMIT, LOW);
                              // xVal = analogRead(xPin);
            }
          if ((yVal - aveRead_y) > 100)               //  move FORWARD with (-) motor speed
            {  
              sSled.setSpeed(-maxMotorSpeed);
              sSled.move(sSled.currentPosition() - 100);            
              while (!digitalRead (FRONT_LIMIT) &&  x < 100)
                {
                  /*digitalWrite (sledStepPin, LOW);
                  delayMicroseconds(150);
                  digitalWrite (sledStepPin, HIGH);
                  delayMicroseconds(20);
                  x++;
                  sSled.runSpeed();
                  x++;
                  
                }
              if (digitalRead(FRONT_LIMIT))
                bounceMotorOffLimit (&sSled, FRONT_LIMIT, HIGH);
                              }
          if ((yVal - aveRead_y ) < -100)               //  move BACKWARD with (+) motor speed
            {
              //digitalWrite(sledDirPin, LOW);
               sSled.move(sSled.currentPosition() + 100);  
              sSled.setSpeed(maxMotorSpeed);             
              while (!digitalRead (BACK_LIMIT) &&  x < 100)
                {
                  /*digitalWrite (sledStepPin, LOW);
                  delayMicroseconds(150);
                  digitalWrite (sledStepPin, HIGH);
                  delayMicroseconds(20);
                  sSled.runSpeed();
                  x++;
                }

              if (digitalRead(BACK_LIMIT))
                bounceMotorOffLimit (&sSled, BACK_LIMIT, LOW);
                 
            } */
       // }  

          nexLoop(nex_listen_list);           // Nextion screen object listener
        //  Blynk.run();
      }
    
      