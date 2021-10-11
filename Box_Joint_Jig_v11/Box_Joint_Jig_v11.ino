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

****************************************************************************************************************************************/
#include <AccelStepper.h>
#include "NextionObjects.h"
//#include <ESP8266_Lib.h>
//#include <BlynkSimpleShieldEsp8266.h>
//#include <SdFat.h>
#include <EEPROM.h>
#include "GlobalVars.h"

/****************************************************************************
      HIGH -- arduino constant for 1.   Used for turning the digital pins on and off.  Motor state, HIGH = ON
      LOW  -- arduino constant for 0    Used for turning the digital pins on and off   Motor state, LOW = OFF
               no need to define them here, they are already defined in libraries
*********************************************************************************/
//#define FILE_WRITE (O_WRITE | O_READ | O_CREAT)   // redefine the file write function based on this thread   https://forum.arduino.cc/index.php?topic=616594.0
/****************************************************************************************
   Decare the AccelStepper object and use the FULL2WIRE as a type of stepper motor.
   connecting to the TB6600, it is a 2 wire control.

   The pins must be defined and configured here, they can not be part of the Setup - because sRouter is set as a global variable
   it must have the buttons defined, prior to the setup
 *************************************************************************/
  AccelStepper  sSled (AccelStepper:: FULL2WIRE, sledStepPin, sledDirPin);
  AccelStepper  sCarriage (AccelStepper::FULL2WIRE, stepPin, directionPin);
/***********************************************
     if Blynk capabilities are to be added to the boxjoint program
     the 3 lines below have to be uncommented, and the ESP / blynk include libraries
     need to be uncommented above.
 ********************************************/

/*Hardware Serial on Mega, Leonardo, Micro...
//#define EspSerial Serial2
//ESP8266     wifi(&EspSerial);
//BlynkTimer  timer;
*/

    /******************************************************
      void  getSettingsScreen(char *verString)
          Function to load all the settings fro the pins and files.   be very careful with this section
          We will have a tokenized read from the config file, and the values should only  be changed in the
          these are the PINS on the arduio where all the digital communication happens
    ********************************************************/
    void   putSettingsScreen( char *verString)
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
    
      setCheck = EEPROM[4095];
      bDebug.setValue(EEPROM[4094]);
      DEBUG = EEPROM[4094];
      if (setCheck != 0)
      {
    
        eeAddress = 0;
        EEPROM.get(eeAddress, directionPin);
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
          displaySawPosition (sCarriage.currentPosition(),1);
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
       void bSetScrnPushCallback(void *ptr)
    
             Nothing implemented on this slider button yet
    *************************************************************/
    void bSetScrnPopCallback(void *ptr) 
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
       void bHomePopCallback(void *ptr)
    
             Nothing implemented on this slider button yet
    *************************************************************/
    void bHomePopCallback(void *ptr) 
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
              setLimitFields();       //  set the displayed limit fields and set color to SET
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
     ************************************************************/
    void bSetParamsPushCallback (void *ptr)
    {
      char    readTxt[10] = {'\0'};
    
      //clear the varaiables prior to reading the screen.   will check the values when time to do the calcs
      fingerSteps = 0;
      kerfSteps = 0;
      stockWidth = 0;
    
      tKerf.getText (readTxt, sizeof(readTxt));
      if (readTxt[0] != '<')
          kerfSteps = atof(readTxt);
      memset (readTxt, '\0', sizeof(readTxt));
      tFinger.getText (readTxt, sizeof(readTxt));
      if (readTxt[0] != '<')
        fingerSteps = atof(readTxt);
      memset(readTxt, '\0', sizeof(readTxt));
      tStockWid.getText (readTxt, sizeof(readTxt));
      if (readTxt[0] != '<')
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
    void bZeroSawPopCallback(void *ptr)
    {
      long  lNewZero;
    
      if (!bStop)
      {
        setLimitFields(1);   
        lNewZero = zeroToBlade();     //zero edge of the working pieces to the right side of the blade, then reset the end limits
        rightLimit = rightLimit- lNewZero;
        leftLimit = leftLimit - lNewZero;
        EEPROM.put(RIGHT_CAR, rightLimit);
        EEPROM.put(LEFT_CAR ,leftLimit);
        sCarriage.setCurrentPosition (0);
        setLimitFields();
      }
    }
   
    /*************************************************
     *  void  bZeroReturnPushCallback (void *ptr)
     *      button used to return the carriage back to the 0 mark of the blade
     *************************************************/
    void bZeroReturnPushCallback (void *ptr)
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
            displaySawPosition (sCarriage.currentPosition(),0); 
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
    void btInvertPopCallback (NexDSButton *ptr)
        {
            ptr->getValue(&bInverted);
        }
 
    /***********************************************************
          void btOnOffPopCallback (void *ptr)
     ***********************************************************/
    byte btOnOffPushCallback(NexDSButton *ptr)
      {
        ptr->getValue(&bStop);
        if (bStop)
          {
            digitalWrite(enablePin, HIGH);
            digitalWrite(sledEnablePin, HIGH);
            return bStop;
          }
        else
          {
            digitalWrite(enablePin, LOW);
            digitalWrite(sledEnablePin, LOW);
            return bStop;
          }
      }
   
    /*************************************************************
    *   void  btJoyPushCallback (void *ptr)
    *     button call back to turn the joystick on and off
    *     when off - the joystick will not respond.   Turn it off so
    *     it is not used in the middle of automatic cutting
    * ***********************************************************/
    byte btJoyPushCallback (NexDSButton *ptr)
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
    
             Nothing implemented on this slider button yet
    *************************************************************/
    void bRefreshPushCallback(void *ptr) 
      {
        readEEPROMSettings();
        putSettingsScreen("");
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
                  while (nextCut() && !btOnOffPushCallback(&btOnOff))
                    {
                      delayMicroseconds(1000);
                      if (bCarrMoved)
                        cycleSled();
                      delayMicroseconds(1000);
                    }
                  bZeroReturnPushCallback(&bZeroReturn);
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
                  bZeroReturnPushCallback(&bZeroReturn);
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
          }
        else
          Serial.end();
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
          delay(500);
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
          sCarriage.moveTo(sCarriage.currentPosition() + 50000); 
          while (!digitalRead(LEFT_LIMIT) && !bStop)
            sCarriage.run();       
          if (bStop)
            {
              stopMotor();
              return LOW;
            }
          sCarriage.moveTo(sCarriage.currentPosition() - 2000);
          sCarriage.setSpeed(-200);
          while (digitalRead(LEFT_LIMIT) && !bStop)
            sCarriage.runSpeed();          
          leftLimit = 0;
          EEPROM.put(LEFT_CAR,leftLimit);
          sCarriage.setCurrentPosition (0);
          if (bStop)
            {
              stopMotor();
              return 0;
            }
            
          // Now run to the other end of the ball screw to set the right limit of the sled
          delay(500);
          sCarriage.moveTo(sCarriage.currentPosition() - 100000);
          while (!digitalRead(RIGHT_LIMIT) && !bStop)
            sCarriage.run();             
          if (bStop)
          {
            stopMotor();
            return LOW;
          }
          sCarriage.move(sCarriage.currentPosition() + 1000);
          sCarriage.setSpeed(300);
          while (digitalRead(RIGHT_LIMIT) && !bStop)
            sCarriage.runSpeed();          
          if (bStop)
          {
            stopMotor();
            return LOW;
          }
          rightLimit = sCarriage.currentPosition();
          EEPROM.put(RIGHT_CAR, rightLimit);
          delay(500);
          // Move the sled back to the 0 position on the left limit switch
          sCarriage.moveTo(leftLimit);
          while ((sCarriage.currentPosition() < leftLimit) && !bStop)
            sCarriage.run();         
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
          sSled.moveTo(sSled.currentPosition() - 50000);
          while (!digitalRead(BACK_LIMIT) && !bStop)
            sSled.run();          
          if (bStop)
            {
              stopMotor();
              return LOW;
            } 
          sSled.move(sSled.currentPosition() + 20000);
          sSled.setSpeed(200);
          while (digitalRead(BACK_LIMIT))
            sSled.runSpeed(); 
          backLimit = 0;
          EEPROM.put(BACK_SLED,backLimit);
          Serial.println(EEPROM[BACK_SLED]);
          sSled.setCurrentPosition (0);
          // Now run to the other end of the ball screw to set the front limit of the sled
          delay(500);
          sSled.moveTo(sSled.currentPosition() + 150000);
          while (!digitalRead(FRONT_LIMIT) && !bStop)
            sSled.run();         
          if (bStop)
            {
              stopMotor();
              return LOW;
            }
          sSled.move(sSled.currentPosition() - 1000);
          sSled.setSpeed(-200);
          while (digitalRead(FRONT_SLED))
            sSled.runSpeed();
          frontLimit = sSled.currentPosition();
          EEPROM.put(FRONT_SLED, frontLimit);
          // Move the sled back to the 0 position on the left limit switch
          sSled.moveTo(backLimit);
          while (sSled.currentPosition() > backLimit)
            sSled.run();       
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
          if(!bStop)
            {
              sSled.moveTo (frontLimit);
              sSled.setSpeed(maxMotorSpeed *.75);
              do                                //  run sled through saw
                  {
                    sSled.runSpeed();
                  }
              while (!digitalRead(FRONT_LIMIT) && sSled.currentPosition() != sSled.targetPosition());

              sSled.moveTo(backLimit);
              sSled.setSpeed(-(maxMotorSpeed));
              do                               // bring sled back to starting position
                {
                  sSled.runSpeed();
                }
              while (!digitalRead(BACK_LIMIT) && sSled.currentPosition() != sSled.targetPosition());
              sSled.setSpeed(1000);
              while (digitalRead(BACK_LIMIT))      // bounce it off the back limit switch
                sSled.runSpeed();
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
       if (sCarriage.currentPosition() >= stockSteps)//   beyond the edge of the stock, don't need to cut anything more
          return LOW;
       switch (whereSaw)
         {
            case RIGHT_EDGE:                      // check that we haven't finished, move to new finger
              {
                if (fingerCounter == 0)                                            // FIRST CUT WHEN STARTING 
                    if (!bInverted)
                      {
                        whereSaw = LEFT_EDGE;
                        firstCutRightSaw = sCarriage.currentPosition();
                        break;
                      }                        
                    else
                      {
                        sCarriage.moveTo(fingerSteps);
                        fingerCounter = fingerCounter +1;
                        bCarrMoved = true;
                      }
                else                                              //May need logic here to but the last little remnant from left side of stock  
                  {
                    sCarriage.moveTo (fingerCounter*fingerSteps);
                    
                  }  
                  if (sCarriage.currentPosition() <= stockSteps)            
                    {
                      sCarriage.setSpeed(maxMotorSpeed);
                      while (sCarriage.currentPosition() != sCarriage.targetPosition())
                        sCarriage.runSpeedToPosition();
                      whereSaw = LEFT_EDGE; 
                      bCarrMoved = true;
                    }
                
                firstCutRightSaw = sCarriage.currentPosition();
              //  displaySawPosition (sCarriage.currentPosition(), 0);
                break;
              }
            case LEFT_EDGE:               // cut the outer edge of the finger, set the status to hog out the middle
              {
                 if (sCarriage.currentPosition() >= stockSteps)//   beyond the edge of the stock, don't need to cut anything more
                  {
                    displaySawPosition (sCarriage.currentPosition(), 0);
                    return LOW;
                  }
   
                sCarriage.moveTo (sCarriage.currentPosition() + (fingerSteps - kerfSteps));
                sCarriage.setSpeed(maxMotorSpeed);
                if (sCarriage.currentPosition() == sCarriage.targetPosition())
                  {
                    bCarrMoved = false;
                    whereSaw = HOG_OUT;
                    break;
                  }
                while (sCarriage.currentPosition() != sCarriage.targetPosition())
                  sCarriage.runSpeedToPosition();
                  
                whereSaw = HOG_OUT;
                bCarrMoved = true;
                //displaySawPosition (sCarriage.currentPosition(),0);
                break;
              }
            case HOG_OUT:         // check to see if we are goign to go past the end of the right edge, if not, start hogging
                                  //   if so, move the blade to just past the start of the kerf on the right edge and finish hogging
              {
                if (sCarriage.currentPosition() - (.75 * kerfSteps) > firstCutRightSaw)
                  {
                    sCarriage.moveTo (sCarriage.currentPosition() - (.875 * kerfSteps));
                    sCarriage.setSpeed(maxMotorSpeed);
                    while (sCarriage.currentPosition() != sCarriage.targetPosition())
                      sCarriage.runSpeedToPosition();
                    bCarrMoved = true;
                    break;
                  }
                if (sCarriage.currentPosition() - (.75 * kerfSteps) < firstCutRightSaw)
                  {
                    whereSaw = RIGHT_EDGE;
                    bCarrMoved = false;
                    fingerCounter = fingerCounter + 2;
                    break;
                  }
              }  
         }
       displaySawPosition (sCarriage.currentPosition(), 0);
       return HIGH;                 
     }

    /*int stepsFromDistance(int move_go, int index) {
    
      int     i = 0;
      SdFile  fSeek;
      char    fileLine[60] = {'\0'};
      char    divider[2] = {',', '\0'};
      char    *token;
      char    checker;
      int     limit;
      if (!fSeek.open("heights.txt"))
      {
        // ErrorDialog( 108 );
        return 0;
      }
      fSeek.rewind();
      checker = fSeek.read();
      if (move_go == HIGH)
        while (checker != '^')            // find the end of the header section, start populating the structure with preset configs
          checker = fSeek.read();
      else if (move_go == LOW)
        while (checker != char(160) )  // find the break in heights, then start reading special bit definitions
          checker = fSeek.read();
      if (move_go == HIGH)
        limit = 4;
      else if ( move_go == LOW)
        limit = 5;
      checker = fSeek.read();
      while (fSeek.available())
      {
        fSeek.fgets(fileLine, sizeof(fileLine));
        token = strtok(fileLine, divider);
        for (i = 1; i <= limit; i++)
        {
          token = strtok(NULL, divider);
          switch (i)
          {
            case 1:
              preSetLookup.index = atoi(token);
              break;
            case 2:
              strcpy(preSetLookup.inches, token);
              break;
            case 3:
              preSetLookup.decimal = atof(token);
              break;
            case 4:
              preSetLookup.steps = atoi(token);
              break;
            case 5:
              strcpy (preSetLookup.label, token);
              break;
          }
        }
        if (move_go == HIGH)
        {
          if (preSetLookup.index == preSetNum)      // found the right line from the file, move to end of file
            fSeek.seekEnd();
        }
        else if (move_go == LOW)
        {
          if ( preSetLookup.index == index)         // found the right custom bit in the heights file, move to the end of the file
            fSeek.seekEnd();
        }
    
      }
      fSeek.close();
      return preSetLookup.steps;
      }*/
    
    /************************************************
     *    void  displaySawPosition (long sawPos)
     *        setting the display on the UI for the current saw position 
     *        will set it in steps
     ************************************************/
    void  displaySawPosition(long sawPos, int howTo)
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
          memset(sCommand, '\0', sizeof(sCommand));
          float  val = calcInches(sawPos)/calcInches(stockSteps);
          if (int(val*100) < 100)
            sprintf(sCommand, "jProgress.val=%d", int(val*100));
          else
            sprintf(sCommand, "jProgress.val=100");
          nexSerial.write(sCommand);
          FlushBuffer();
       }
//SETUP    
    void setup() {
      if (DEBUG)
        {
          Serial.begin (250000);
          delay(500);
        }
      nexInit(250000);
      
      //EEPROM[4094] = 0;  // setting the debug flag.   If 0, no debug, if 1, debugging enabled
      readEEPROMSettings();
      putSettingsScreen("");
    
      bSetScrn.attachPush(bSetScrnPushCallback, &bSetScrn);
      bSetScrn.attachPop(bSetScrnPopCallback, &bSetScrn);
      bHome.attachPush(bHomePushCallback, &bHome);
      bHome.attachPop(bHomePopCallback, &bHome);
      bDebug.attachPop(bDebugPopCallback, &bDebug);
      bRefresh.attachPush(bRefreshPushCallback, &bRefresh);
      bSledLimits.attachPush(bSledLimitsPushCallback, &bSledLimits);
      bCarrLimits.attachPop(bCarrLimitsPopCallback, &bCarrLimits);
      bZeroSaw.attachPop(bZeroSawPopCallback, &bZeroSaw);
      btOnOff.attachPush(btOnOffPushCallback, &btOnOff);
      bSetParams.attachPush (bSetParamsPushCallback, &bSetParams);
      bClear.attachPop (bClearPopCallback, &bClear);
      bZeroReturn.attachPush(bZeroReturnPushCallback, &bZeroReturn);
      bNextCut.attachPush(bNextCutPushCallback, &bNextCut);
      bStartAuto.attachPush(bStartAutoPushCallback, &bStartAuto);
      btInvert.attachPop(btInvertPopCallback, &btInvert);
      btJoy.attachPush(btJoyPushCallback, &btJoy);     
    /*-----------------------------------------------------*/
      pinMode (sledEnablePin, OUTPUT);
      pinMode (sledStepPin, OUTPUT);
      pinMode (sledDirPin, OUTPUT);
    /*-----------------------------------------------------*/
      pinMode (enablePin, OUTPUT);
      pinMode(directionPin, OUTPUT);
      pinMode(stepPin, OUTPUT);
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
      pinMode (jPin, INPUT);
    /*-----------------------------------------------------*/  
      attachInterrupt (digitalPinToInterrupt(iStopPin), stopMotor, RISING);   //hard stop button - kill switch pin to stop all movement of the motors
      
      stopMotor();
      btJoy.setText("Joy");
      btJoy.setValue(0); 
      bStop = LOW;
      jProgress.setValue(0);
      setLimitFields();
    /*-----------------------------------------------------*/  
      sSled.setAcceleration (maxAcceleration);
      sSled.setMaxSpeed(maxMotorSpeed);
      sSled.setMinPulseWidth(pulseWidthMicros);
    /*-----------------------------------------------------*/
      sCarriage.setMinPulseWidth(pulseWidthMicros);
      sCarriage.setMaxSpeed(maxMotorSpeed);
      sCarriage.setAcceleration(maxAcceleration);
    }
    
    void loop() {
  
      nexLoop(nex_listen_list);           // Nextion screen object listener
      if (digitalRead(moveCarriage))     // move to next cut position
        if (!bStop)
          nextCut();
      if (jONOff)                         // if the Joystick is enabled, move sled or carriage
        {
          xVal = analogRead(xPin);
          yVal = analogRead (yPin); 
  /* using "if's" in the next statements allows all 4 axis of movement.  While's will block until
  //  they the movement is finished and the joystick is back to center
  //  with if's you can move both motors at simeltaneously*/
          if (xVal > 523)               //  allow for a little jitter, value is around 518
            {
              int speed_ = map (xVal, 523, 1023, 10, 2000);
              sCarriage.move(1);
              sCarriage.setSpeed (speed_);
              sCarriage.runSpeed();
             // xVal = analogRead(xPin);
            }
          if (xVal < 500)               //  allow for a little jitter, value is around 518
            {
              int speed_ = map(xVal, 500, 0, -10, -2000);
              sCarriage.move(1);
              sCarriage.setSpeed(speed_);
              sCarriage.runSpeed();
             // xVal = analogRead(xPin);
            }
          if (yVal > 523)               //  allow for a little jitter, value is around 518
            {
              int speed_ = map (yVal, 523, 1023, 10, 4000);
              sSled.move(1);
              sSled.setSpeed (speed_);
              sSled.runSpeed();
              //yVal = analogRead(yPin);
            }
          if (yVal < 500)               //  allow for a little jitter, value is around 518
            {
              int speed_ = map(yVal, 500, 0, -10, -4000);
              sSled.move(1);
              sSled.setSpeed(speed_);
              sSled.runSpeed();
             // yVal = analogRead(yPin);
            }
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
      bStop = HIGH;
      btOnOff.setValue(HIGH);
      btOnOff.setText("Off");
      digitalWrite (enablePin, HIGH);
      digitalWrite (sledEnablePin, HIGH);
    }
  
    