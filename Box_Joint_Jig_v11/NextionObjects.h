#include <Nextion.h>
  
  NexPage     Home  =          NexPage        (0, 0, "Home");
  NexButton   bSetScrn =       NexButton      (0, 1, "bSetScrn");
  NexButton   bSledLimits =    NexButton      (0, 4, "bSledLim");         //   button to home and set the ball screw limits for the sled
  NexButton   bCarrLimits  =   NexButton      (0, 3, "bCarLim");          //  button to set the carriage end stop limits
  NexDSButton btOnOff     =    NexDSButton    (0, 34, "btOnOff");
  NexButton   bZeroSaw  =      NexButton      (0, 29, "bZeroSaw");        //  button used to set the 0 edge of the saw and reset bottom left limit
  NexText     tKerf   =        NexText        (0, 11, "tKerf");           //  text field holding the width of the saw kerf  RESET THE # OFF THE NEXTION
  NexText     tFinger  =       NexText        (0, 10, "tFinger");         //  text field holding the width to cut each finger  REST THE # OFF TEH NEXTION
  NexText     tStockWid =      NexText        (0, 9, "tStockWid");        //  text field holding the width of the wood stock  RESET THE # OFF THE NEXTION
  NexButton   bSetParams =     NexButton      (0, 31, "bSetParams");
  NexButton   bClear =         NexButton      (0, 35, "bClear");          //  button to clear the parameter fields.   values set in Nextion UI IDE
  NexButton   bStartAuto  =    NexButton      (0, 27, "bStartAuto");      //  button for starting the automatic sled, carriage model
  NexButton   bZero        =   NexButton      (0, 32, "bZero");           //  return carriage back to the 0 edge of the blade after completing cuts
  NexButton   bNextCut  =      NexButton      (0, 28, "bNextCut");        //  Nextion screen button to move to next cut
  NexText     tCurPos  =       NexText        (0, 33, "tCurPos");         //  text value to display where the saw is during cuts
  NexProgressBar  jProgress=   NexProgressBar (0, 2, "jProgress");        //  keep track of the job, and progress to the end 
  NexDSButton btInvert    =    NexDSButton    (0, 21, "btInvert");        //  button to set whether to cut tounge or groove first
  NexButton   bSledStop   =    NexButton      (0, 43, "bSledStop");       //  Button to set the slide position stop point to not have to cycle all the way to front limit
  NexText     tSawStop     =   NexText        (0, 44, "tSawStop");        //  Field to hold the value of the saw stop position from Sled Stop button
  NexButton   bLeft        =   NexButton      (0, 42, "bLeft");           //  Button to move carriage to the left (like a joystick jog)
  NexButton   bUp          =   NexButton      (0, 39, "bUp");             //  Button to move the sled forward, similar to joystick move
  NexButton   bRight       =   NexButton      (0, 41, "bRight");          //  Button to move the carriage to the right
  NexButton   bDown        =   NexButton      (0, 40, "bDown");           //  Button to move sled to the back, like a joystick jog
  NexPage     Settings =       NexPage        (2,0,"Settings");
  NexNumber   nRightLimit =    NexNumber      (2, 2, "nRightLimit");      //  PIN defined for the right end limit switch on carriage
  NexButton   bHome =          NexButton      (2, 1, "bHome");
  NexDSButton bDebug =         NexDSButton    (2, 40, "bDebug");          //  Button to set the debug byte in EEPROM, to allow for output to the Serial monitor
  NexButton   bRefresh =       NexButton      (2, 42, "bRefresh");        //   button to refresh the Settings screen from EEPROM
  NexNumber   nLeftLimit =     NexNumber      (2, 4, "nLeftLimit");       //  PIN defined as the Left end limit switch on carriage
  NexNumber   nFrontLimit =    NexNumber      (2, 6, "nFrontLimit");      //  PIN defined as the forward motion limit switch for sled
  NexNumber   nBackLimit =     NexNumber      (2, 8, "nBackLimit");       //  PIN defined as the back limit switch for the sled.  
  NexNumber   nSDWrite =       NexNumber      (2, 10, "nSDWrite");        //  PIN where the SD_Write CS activation.   
  NexNumber   nStepPin =       NexNumber      (2, 16, "nStepPin");        //  PIN for the stepper pin on the carriage stepper motor controller
  NexNumber   nSledStepPin =   NexNumber      (2, 22, "nSledStepPin");    //  PIN for the stepper pin on the sled stepper motor controller
  NexNumber   nEnablePin =     NexNumber      (2, 18, "nEnablePin");      //  PIN for the enable E+ pin on the carriage stepper motor controller
  NexNumber   nDirectionPin =  NexNumber      (2, 14, "nDirectionPin");   //  PIN for the Direction D+ pin on the carriage stepper motor controller
  NexNumber   nSledDirPin =    NexNumber      (2, 12, "nSledDirPin");     //  PIN for the direction D+ pin on the sled stepper motor controller
  NexNumber   nStpsPerRev =    NexNumber      (2, 25, "nStpsPerRev");     //  number of steps per revolution of the stepper motor
  NexNumber   nWorkMtrSpd =    NexNumber      (2, 35, "nWorkMtrSpd");     //  working speed of the motor
  NexNumber   nSledEblPin =    NexNumber      (2, 20, "nSledEblPin");     //  PIN to enable E+ the sled stepper motor controller
  NexNumber   nPlseWdtMicro =  NexNumber      (2, 29, "nPlseWdtMicro");   //  number of microseconds to send power to the stepper motors
  NexNumber   nMaxAccel =      NexNumber      (2, 37, "nMaxAccel");       //  maximum accelearation for th estepper motors
  NexNumber   nMillisBtwn =    NexNumber      (2, 27, "nMillisBtwn");     //  number of millisecons between the pulses to the stepper motor controllers
   
  NexText   tMaxMotorSpeed  =  NexText        (2, 38, "tMaxMotoSpeed");   //  maximum motor speed - will be upper limit
  NexText     tDistPerStep =   NexText        (2, 36, "tDistPerStep");    //  distance traveled for each pulse step 
  NexText     tInitSpeed =     NexText        (2, 39, "tInitSpeed");      //  Initial speed for the motor on startup
  NexPage     Error     =      NexPage        (3, 0, "Error");             //  error dialog with the OK button on it
 
   
  NexTouch *nex_listen_list[]  = {
  &bSetScrn,
  &bHome,
  &bDebug, 
  &bRefresh,
  &bSledLimits,
  &bCarrLimits,
  &btOnOff,
  &bZeroSaw,
  &bSetParams,
  &bZero,
  &bNextCut,
  &bStartAuto,
  &btInvert,
  &bClear,
  &bSledStop,
  &bLeft,
  &bRight,
  &bUp,
  &bDown,
  NULL
}; 
 
/***************************************
    void Flushbuffer()
        when serial write to the Nextion, need to send final command
 ***********************************/
    void FlushBuffer() 
      {
        nexSerial.write(0xff);
        nexSerial.write(0xff);
        nexSerial.write(0xff);
      }

   