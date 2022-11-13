<<<<<<< HEAD

/**************************************************************************************************
 * Test program to figure out if we can use EEPROM to store variables for setup and managmement
 * Would like to have a tool that allows us to set EEPROM for each stepper motor, so we don't have to 
 * use the SD cared to store config settings.   EEPROM, once written, does not reset until it is over written or erased by 
 * writing to EEPROM again.   It may be a good way to set configuration on automation tools, and would use this program to set them up
 * 
 * once they are set up, EEPROM.get and EEPROM.read can be used to set the config necessary during run time, or doing lookups
 * 
 * mega 2560 has 4096 bytes
 * uno  has 1024
 * nano has 1024 bytes
 * 
 * set the boardMemory variable to the appropriate size , and the memory will be cleared then written 
 * May need to modify code for the right program and initializing - but the concpet is here, 
 * 
 * reading memory is based on sizeof(type)... that will advance the reading point
 * can write to various points of EEPROM by using put, instead of write.   Can create structures and write entire structure
 *    instead of writing variable by variable
 *    use last read address += sizeof(variable type) to advance to te next read
 *    
 *    thinking of using this for initializing a bunch of setup on a nano, and not having to use the SD card for memory management
 **********************************************************************************/


#include <EEPROM.h>
#include "SdFat.h"
            
 #define debug(x) Serial.print(x)
 #define debugLn(x) Serial.println(x)

#define SD_WRITE 53                 //  CS pin of the SD card
#define SET_ADDRESS 0
#define HEIGHT_ADDRESS 50
 
        uint8_t    directionPin = 40;             //  pin that will go to the D+ on the stepper controller  have to declare here, as the stepper is a global class
        uint8_t    sledDirPin = 38;               //  pin that will go to the D+ on the sled stepper controller.   Have to declare here as the stepper is a global class
        uint8_t    stepPin = 34;                  //  pin that will wire to the P+ (pulse) on the stepper controlloer   have to declare here as the stepper is a global class
        uint8_t    sledStepPin = 36;              //  pin that will wire to the P+ (pulse) on the Fence stepper controlloer   have to declare here as the stepper is a global class
        uint8_t    enablePin = 35;                 //  pin that will wire to the E+ on the stepper controller.   Turns the controller on and off  LOW turns motor on, HIGH turns motor off
        uint8_t    sledEnablePin = 39;            //  pin that will wire to the E+ on the fence stepper controller.   Turns the controller on and off  LOW turns motor on, HIGH turns motor off
        int     initSpeed = 4000;              //  sets the initial speed of the motor.   Don't think we need acceleration in the router, but will find out
        long    maxMotorSpeed = 32000;         //  as defined - maximum speed motor will run.   Sets the 100% on the speed slider
        int     maxAcceleration = 2000;        //  maximum number of steps for acceleration
        long    workingMotorSpeed = 4000;      //  active working speed, set by the slider, and will be somewhere between 0 and 100%
        int     stepsPerRevolution = 1600;     //  number of steps required for 1 revolution of leadscrew
        int     microPerStep =   8;            //  number of microSteps.   Set on the TB600 controller by dip switch   Off | On | Off
        float   distPerStep = 0.00024606;      //  inches per step with calculated off <microPerStep> = 8
        int     pulseWidthMicros = 20;         //  miliseconds  -- delay for the steps.   the higher the number the slower the lead screw will turn
        int     millisBetweenSteps = 20;       //  milliseconds - or try 1000 for slower steps   delay between pulses sent to the controller
        byte    RIGHT_LIMIT = 46;
        byte    LEFT_LIMIT = 44;
        byte    FRONT_LIMIT=45;
        byte    BACK_LIMIT=23; 
        byte    SAW_ZERO = 13;
        byte    jPin = 9;
        SdFat   sdCard;                        //  pointer to the SD card reader.   Used in storing config and memory files
        int     eeAddress =500;
  
   struct inchCacl {
          int    index;
          char   inches[6];
          float  decimal;
          float  steps;
          char   label[30];
        }  preSetLookup; 
  
int  boardMemory = 4096;     // This is for a MEGA

 void clearEEProm ( int boardSize)
  {
    int lastx = 0;
    debugLn("clearing the EEPROM");
    for (int x = 0; x< boardSize; x++)
      {
        if (x == lastx + 124)
          {
            debugLn(".");
            lastx = x;
          }
        else
          Serial.print(".");
        EEPROM.write(x, 255);
      }
     debugLn("");
     debugLn("EEPROM Cleared");
  }
  
  
  /****************************************************
  int stepsFromDistance( int move_go, int index)

     Returns the  index of the structure that matches with the value from the drop down
     This is typically used after the drop down selection for travel distance
     it will check the move/goto button to figure out if we are moving a set distance
     or setting the height from a fixed position.  If a fixed position, the 0 value of that
     position should be the "table" height
*********************************************/
        
int loadHeightsFromFile(int move_go) {

        
          int     i = 0;
          SdFile  fSeek;
          char    fileLine[60] = {'\0'};
          char    divider[2] = {',', '\0'};
          char    *token;
          char    checker;
          int     limit;
          int     bStop=1;

 
          if (!fSeek.open("heights.txt"))
            {
              //ErrorDialog( 108 ); 
              debugLn ("could not open heights.txt") ; 
              return -1;
            }
          fSeek.rewind(); 
          checker = fSeek.read();
          if (move_go == 1)
            while (checker != '^')            // find the end of the header section, start populating the structure with preset configs
                checker = fSeek.read();
          else if (move_go == 0)
            while (checker != char(160) )  // find the break in heights, then start reading special bit definitions 
                checker = fSeek.read();  
          fSeek.read();
          if (move_go == 1)
            limit = 4;
          else if ( move_go == LOW)
            limit = 5;
          debugLn("\n\n\t\tWriting heights to the EEPROM.");
          while (fSeek.available())
            {
              fSeek.fgets(fileLine, sizeof(fileLine));
              if (fileLine[0] == '*')
                {
                  fSeek.close();
                  //debugLn("");
                  //debugLn("Finished writing Heights to EEPROM");
                  return eeAddress;
                }
              token = strtok(fileLine, divider);   
              for (i=1; i<= limit; i++)
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
                        preSetLookup.steps = atof(token);
                        break;
                      case 5:
                        strcpy (preSetLookup.label, token);
                        break;
                    }
                  }
                EEPROM.put(eeAddress, preSetLookup);
                Serial.print(".");
                eeAddress += sizeof(preSetLookup);
 
   /*           if (move_go == HIGH)
                { 
                  if (preSetLookup.index == preSetNum)      // found the right line from the file, move to end of file
                    fSeek.seekEnd();
                }
              else if (move_go == LOW)
                {
                  if ( preSetLookup.index == index)         // found the right custom bit in the heights file, move to the end of the file
                  fSeek.seekEnd();
                }   */
              
                  
            }
          fSeek.close();
          debugLn("");
          debugLn("Finished writing Heights to EEPROM");
          return eeAddress;
        }

void setup() 
  {
    Serial.begin(500000);
      delay (1000);
   pinMode(SD_WRITE, OUTPUT);       // define the SD card writing pin
        
         if (!sdCard.begin(SD_WRITE))
            debugLn("Could not open the SD card");
  }

void loop() 
  {

    char   serGet;
    int     nTemp = 0;
    debugLn ("");
    debugLn ("enter an <0> if you want to clear the EEPROM");
    debugLn ("enter an <1> if you want to load the settings to EEPROM");
    debugLn ("enter an <2> if you want to get the settings from EEPROM");
    debugLn ("enter an <3> if you what the Heights file loaded");
    debugLn ("enter an <4> if you want to read the first heights values from EEPROM");
    debugLn ("    enter an <5> if you want to read the next Heights values");
    debugLn ("enter <6> if you want size of structure");
    debugLn ("enter <7> if you want to see what's in EEPROM");

    while (Serial.available()==0);
    serGet = Serial.read();
    
    switch (serGet)
      {
          case '0':
            {
              clearEEProm(boardMemory);
              break;
            }
          case '1':
            {
                eeAddress = 0;
                debugLn("");
                debugLn("going to write a bunch of data to the EEPROM, the filled byte indicates it hasn't been written");
                EEPROM.put (eeAddress, directionPin);
                eeAddress += sizeof(directionPin);
                EEPROM.put(eeAddress, sledDirPin);
                eeAddress += sizeof(sledDirPin);
                EEPROM.put(eeAddress, stepPin);
                eeAddress += sizeof(stepPin);
                EEPROM.put(eeAddress, sledStepPin);
                eeAddress += sizeof(sledStepPin);
                EEPROM.put(eeAddress, enablePin);
                eeAddress += sizeof(enablePin);
                EEPROM.put(eeAddress, sledEnablePin);
                eeAddress += sizeof(sledEnablePin);
                EEPROM.put(eeAddress, initSpeed);
                eeAddress += sizeof(initSpeed);
                EEPROM.put(eeAddress, stepsPerRevolution);
                eeAddress += sizeof(stepsPerRevolution);
                EEPROM.put(eeAddress, microPerStep);
                eeAddress += sizeof(microPerStep);  
                EEPROM.put(eeAddress, pulseWidthMicros);
                eeAddress += sizeof(pulseWidthMicros);
                EEPROM.put(eeAddress, millisBetweenSteps);
                eeAddress += sizeof (millisBetweenSteps);
                EEPROM.put(eeAddress, RIGHT_LIMIT);
                eeAddress += sizeof (RIGHT_LIMIT);
                EEPROM.put(eeAddress, LEFT_LIMIT);
                eeAddress += sizeof (LEFT_LIMIT);
                EEPROM.put(eeAddress, FRONT_LIMIT);
                eeAddress += sizeof (FRONT_LIMIT);
                EEPROM.put(eeAddress, BACK_LIMIT);
                eeAddress += sizeof (BACK_LIMIT);
                EEPROM.put(eeAddress,SAW_ZERO);
                eeAddress += sizeof(SAW_ZERO);
                EEPROM.put(eeAddress, maxAcceleration);
                eeAddress += sizeof(maxAcceleration);
                EEPROM.put (eeAddress, distPerStep);
                eeAddress += sizeof(distPerStep);   
                EEPROM.put(eeAddress, maxMotorSpeed);
                eeAddress += sizeof(maxMotorSpeed);
                EEPROM.put(eeAddress, workingMotorSpeed);
                eeAddress += sizeof(workingMotorSpeed);
                EEPROM.put(eeAddress, jPin);
                eeAddress += sizeof(jPin);
                
                nTemp = EEPROM[boardMemory - 1];
                if ((nTemp < eeAddress) || nTemp == 255)
                  {
                    EEPROM[boardMemory - 1] = eeAddress;
                  } 
                break;
            }
          case '2':
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

        
                debugLn("_________________________________________________________________________________________________________________________");
                Serial.print ("directionPin ==> ");
                debugLn(byte(directionPin));
                debug ("sledDirPin ==> ");
                debugLn(sledDirPin);
                debug("stepPin ==> ");
                debugLn(stepPin);
                debug ("sledStepPin ==> ");
                debugLn(sledStepPin);
                debug ("enablePin ==> ");
                debugLn(enablePin);
                debug("sledEnablePin ==> ");
                debugLn(sledEnablePin);
                debug ("initSpeed ==> ");
                debugLn (initSpeed);       
                debug("stepsPerRevolution ==> ");
                debugLn(stepsPerRevolution);
                debug("microPerStep ==> ");
                debugLn(microPerStep);       
                debug("pulseWidthMicros ==> ");
                debugLn(pulseWidthMicros);
                debug ("millisBetweenSteps ==> ");
                debugLn(millisBetweenSteps);
                debug ("RIGHT_LIMIT ==> ");
                debugLn(RIGHT_LIMIT);
                debug("LEFT_LIMIT ==> ");
                debugLn(LEFT_LIMIT);
                debug("FRONT_LIMIT ==>");
                debugLn (FRONT_LIMIT);
                debug ("BACK_LIMIT ==> ");
                debugLn(BACK_LIMIT);
                debug("SAW_ZERO ==> ");
                debugLn(SAW_ZERO);
                debug ("maxAcceleartion ==> ");
                debugLn(maxAcceleration);
                debug("distPerStep ==> ");
                Serial.println(distPerStep,8);
                debug("maxMotorSpeed == > ");
                debugLn(maxMotorSpeed);
                debug("workingMotorSpeed ==> ");
                debugLn(workingMotorSpeed);
                debug("Joystick Power Pin ==> ");
                debugLn(jPin);
                break;
            }  
         case '3':
          {
            eeAddress = HEIGHT_ADDRESS;
            eeAddress = loadHeightsFromFile(1);
            EEPROM.put(boardMemory - 3, eeAddress);
            break;
          }  
          case '6':
            {
              debug ("Size of the preset lookup structure ==> ");
              debugLn(sizeof(preSetLookup));
              debugLn("");
              break;
            }
          case '7':
            for (int x = 0; x< boardMemory; x++)
              {
                debug (x);
                debug (" ==> ");
                debugLn (EEPROM.read(x));
              }
            break;
          case '4':
            {
            // eeAddress = sizeof(preSetLookup);
              eeAddress = HEIGHT_ADDRESS;
              EEPROM.get(eeAddress,preSetLookup);
              debugLn(preSetLookup.index);
              debugLn(preSetLookup.inches);
              Serial.println(preSetLookup.decimal, 6);
              Serial.println(preSetLookup.steps, 6);
              debugLn("");
        //      eeAddress += sizeof(preSetLookup);
              break;
            }
          case '5':
            {
              eeAddress += sizeof(preSetLookup);
              EEPROM.get(eeAddress,preSetLookup);
              debugLn(preSetLookup.index);
              debugLn(preSetLookup.inches);
              Serial.println(preSetLookup.decimal, 6);
              Serial.println(preSetLookup.steps, 6);
              debugLn("");
              break;
            }
      
      }
}
=======
/**************************************************************************************************
 * Test program to figure out if we can use EEPROM to store variables for setup and managmement
 * Would like to have a tool that allows us to set EEPROM for each stepper motor, so we don't have to 
 * use the SD cared to store config settings.   EEPROM, once written, does not reset until it is over written or erased by 
 * writing to EEPROM again.   It may be a good way to set configuration on automation tools, and would use this program to set them up
 * 
 * once they are set up, EEPROM.get and EEPROM.read can be used to set the config necessary during run time, or doing lookups
 * 
 * mega 2560 has 4096 bytes
 * uno  has 1024
 * nano has 1024 bytes
 * 
 * set the boardMemory variable to the appropriate size , and the memory will be cleared then written 
 * May need to modify code for the right program and initializing - but the concpet is here, 
 * 
 * reading memory is based on sizeof(type)... that will advance the reading point
 * can write to various points of EEPROM by using put, instead of write.   Can create structures and write entire structure
 *    instead of writing variable by variable
 *    use last read address += sizeof(variable type) to advance to te next read
 *    
 *    thinking of using this for initializing a bunch of setup on a nano, and not having to use the SD card for memory management
 **********************************************************************************/


#include <EEPROM.h>
#include "SdFat.h"
            
 #define debug(x) Serial.print(x)
 #define debugLn(x) Serial.println(x)

#define SD_WRITE 53                 //  CS pin of the SD card
#define SET_ADDRESS 0
#define HEIGHT_ADDRESS 50
 
        byte    directionPin = 32;             //  pin that will go to the D+ on the stepper controller  have to declare here, as the stepper is a global class
        byte    sledDirPin = 12;               //  pin that will go to the D+ on the sled stepper controller.   Have to declare here as the stepper is a global class
        byte    stepPin = 13;                  //  pin that will wire to the P+ (pulse) on the stepper controlloer   have to declare here as the stepper is a global class
        byte    sledStepPin = 33;              //  pin that will wire to the P+ (pulse) on the Fence stepper controlloer   have to declare here as the stepper is a global class
        byte    enablePin = 6;                 //  pin that will wire to the E+ on the stepper controller.   Turns the controller on and off  LOW turns motor on, HIGH turns motor off
        byte    sledEnablePin = 36;            //  pin that will wire to the E+ on the fence stepper controller.   Turns the controller on and off  LOW turns motor on, HIGH turns motor off
        int     initSpeed = 4000;              //  sets the initial speed of the motor.   Don't think we need acceleration in the router, but will find out
        long    maxMotorSpeed = 20000;         //  as defined - maximum speed motor will run.   Sets the 100% on the speed slider
        int     maxAcceleration = 2000;        //  maximum number of steps for acceleration
        long    workingMotorSpeed = 4000;      //  active working speed, set by the slider, and will be somewhere between 0 and 100%
        int     stepsPerRevolution = 1600;     //  number of steps required for 1 revolution of leadscrew
        int     microPerStep =   8;            //  number of microSteps.   Set on the TB600 controller by dip switch   Off | On | Off
        float   distPerStep = 0.00024606;      //  inches per step with calculated off <microPerStep> = 8
        int     pulseWidthMicros = 20;         //  miliseconds  -- delay for the steps.   the higher the number the slower the lead screw will turn
        int     millisBetweenSteps = 20;       //  milliseconds - or try 1000 for slower steps   delay between pulses sent to the controller
        byte    RIGHT_LIMIT = 35;
        byte    LEFT_LIMIT = 34;
        byte    FRONT_LIMIT=40;
        byte    BACK_LIMIT=41; 
        byte    SAW_ZERO = 44;
        SdFat   sdCard;                        //  pointer to the SD card reader.   Used in storing config and memory files
        int     eeAddress =500;
  
   struct inchCacl {
          int    index;
          char   inches[6];
          float  decimal;
          float  steps;
          char   label[30];
        }  preSetLookup; 
  
 int  boardMemory = 4096;     // This is for a MEGA

 void clearEEProm ( int boardSize)
  {
    int lastx = 0;
    debugLn("clearing the EEPROM");
    for (int x = 0; x< boardSize; x++)
      {
        if (x == lastx + 124)
          {
            debugLn(".");
            lastx = x;
          }
        else
          Serial.print(".");
        EEPROM.write(x, 255);
      }
     debugLn("");
     debugLn("EEPROM Cleared");
  }
  
  
  /****************************************************
  int stepsFromDistance( int move_go, int index)

     Returns the  index of the structure that matches with the value from the drop down
     This is typically used after the drop down selection for travel distance
     it will check the move/goto button to figure out if we are moving a set distance
     or setting the height from a fixed position.  If a fixed position, the 0 value of that
     position should be the "table" height
*********************************************/
        
int loadHeightsFromFile(int move_go) {

        
          int     i = 0;
          SdFile  fSeek;
          char    fileLine[60] = {'\0'};
          char    divider[2] = {',', '\0'};
          char    *token;
          char    checker;
          int     limit;
          int     bStop=1;

 
          if (!fSeek.open("heights.txt"))
            {
              //ErrorDialog( 108 ); 
              debugLn ("could not open heights.txt") ; 
              return -1;
            }
          fSeek.rewind(); 
          checker = fSeek.read();
          if (move_go == 1)
            while (checker != '^')            // find the end of the header section, start populating the structure with preset configs
                checker = fSeek.read();
          else if (move_go == 0)
            while (checker != char(160) )  // find the break in heights, then start reading special bit definitions 
                checker = fSeek.read();  
          fSeek.read();
          if (move_go == 1)
            limit = 4;
          else if ( move_go == LOW)
            limit = 5;
          debugLn("\n\n\t\tWriting heights to the EEPROM.");
          while (fSeek.available())
            {
              fSeek.fgets(fileLine, sizeof(fileLine));
              if (fileLine[0] == '*')
                {
                  fSeek.close();
                  //debugLn("");
                  //debugLn("Finished writing Heights to EEPROM");
                  return eeAddress;
                }
              token = strtok(fileLine, divider);   
              for (i=1; i<= limit; i++)
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
                        preSetLookup.steps = atof(token);
                        break;
                      case 5:
                        strcpy (preSetLookup.label, token);
                        break;
                    }
                  }
                EEPROM.put(eeAddress, preSetLookup);
                Serial.print(".");
                eeAddress += sizeof(preSetLookup);
 
   /*           if (move_go == HIGH)
                { 
                  if (preSetLookup.index == preSetNum)      // found the right line from the file, move to end of file
                    fSeek.seekEnd();
                }
              else if (move_go == LOW)
                {
                  if ( preSetLookup.index == index)         // found the right custom bit in the heights file, move to the end of the file
                  fSeek.seekEnd();
                }   */
              
                  
            }
          fSeek.close();
          debugLn("");
          debugLn("Finished writing Heights to EEPROM");
          return eeAddress;
        }

void setup() 
  {
    Serial.begin(250000);
      delay (1000);
   pinMode(SD_WRITE, OUTPUT);       // define the SD card writing pin
        
         if (!sdCard.begin(SD_WRITE))
            debugLn("Could not open the SD card");
  }

void loop() 
  {

    char   serGet;
    int     nTemp = 0;
    debugLn ("");
    debugLn ("enter an <X> if you want to clear the EEPROM");
    debugLn ("enter an <S> if you want to load the settings to EEPROM");
    debugLn ("enter an <I> if you want to get the settings from EEPROM");
    debugLn ("enter an <H> if you what the Heights file loaded");
    debugLn ("enter an <R> if you want to read the first heights values from EEPROM");
    debugLn ("    enter an <N> if you want to read the next Heights values");
    debugLn ("enter <C> if you want size of structure");
    debugLn ("enter <L> if you want to see what's in EEPROM");

    while (Serial.available()==0);
    serGet = Serial.read();
    
    switch (serGet)
      {
          case 'X':
            {
              clearEEProm(boardMemory);
              break;
            }
          case 'S':
            {
                eeAddress = 0;
                debugLn("");
                debugLn("going to write a bunch of data to the EEPROM, the filled byte indicates it hasn't been written");
                EEPROM.put (eeAddress, directionPin);
                eeAddress += sizeof(directionPin);
                EEPROM.put(eeAddress, sledDirPin);
                eeAddress += sizeof(sledDirPin);
                EEPROM.put(eeAddress, stepPin);
                eeAddress += sizeof(stepPin);
                EEPROM.put(eeAddress, sledStepPin);
                eeAddress += sizeof(sledStepPin);
                EEPROM.put(eeAddress, enablePin);
                eeAddress += sizeof(enablePin);
                EEPROM.put(eeAddress, sledEnablePin);
                eeAddress += sizeof(sledEnablePin);
                EEPROM.put(eeAddress, initSpeed);
                eeAddress += sizeof(initSpeed);
                EEPROM.put(eeAddress, stepsPerRevolution);
                eeAddress += sizeof(stepsPerRevolution);
                EEPROM.put(eeAddress, microPerStep);
                eeAddress += sizeof(microPerStep);  
                EEPROM.put(eeAddress, pulseWidthMicros);
                eeAddress += sizeof(pulseWidthMicros);
                EEPROM.put(eeAddress, millisBetweenSteps);
                eeAddress += sizeof (millisBetweenSteps);
                EEPROM.put(eeAddress, RIGHT_LIMIT);
                eeAddress += sizeof (RIGHT_LIMIT);
                EEPROM.put(eeAddress, LEFT_LIMIT);
                eeAddress += sizeof (LEFT_LIMIT);
                EEPROM.put(eeAddress, FRONT_LIMIT);
                eeAddress += sizeof (FRONT_LIMIT);
                EEPROM.put(eeAddress, BACK_LIMIT);
                eeAddress += sizeof (BACK_LIMIT);
                EEPROM.put(eeAddress,SAW_ZERO);
                eeAddress += sizeof(SAW_ZERO);
                EEPROM.put(eeAddress, maxAcceleration);
                eeAddress += sizeof(maxAcceleration);
                EEPROM.put (eeAddress, distPerStep);
                eeAddress += sizeof(distPerStep);   
                EEPROM.put(eeAddress, maxMotorSpeed);
                eeAddress += sizeof(maxMotorSpeed);
                EEPROM.put(eeAddress, workingMotorSpeed);
                eeAddress += sizeof(workingMotorSpeed);
                nTemp = EEPROM[boardMemory - 1];
                if ((nTemp < eeAddress) || nTemp == 255)
                  {
                    EEPROM[boardMemory - 1] = eeAddress;
                  } 
                break;
            }
          case 'I':
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
                SAW_ZERO = EEPROM[eeAddress];
                eeAddress += sizeof(SAW_ZERO);
                EEPROM.get(eeAddress, maxAcceleration);
                eeAddress += sizeof(maxAcceleration);
                EEPROM.get (eeAddress, distPerStep);
                eeAddress += sizeof(distPerStep);   
                EEPROM.get(eeAddress, maxMotorSpeed);
                eeAddress += sizeof(maxMotorSpeed);
                EEPROM.get(eeAddress, workingMotorSpeed);
                eeAddress += sizeof(workingMotorSpeed);
        
                debugLn("_________________________________________________________________________________________________________________________");
                Serial.print ("directionPin ==> ");
                debugLn(byte(directionPin));
                debug ("sledDirPin ==> ");
                debugLn(sledDirPin);
                debug("stepPin ==> ");
                debugLn(stepPin);
                debug ("sledStepPin ==> ");
                debugLn(sledStepPin);
                debug ("enablePin ==> ");
                debugLn(enablePin);
                debug("sledEnablePin ==> ");
                debugLn(sledEnablePin);
                debug ("initSpeed ==> ");
                debugLn (initSpeed);       
                debug("stepsPerRevolution ==> ");
                debugLn(stepsPerRevolution);
                debug("microPerStep ==> ");
                debugLn(microPerStep);       
                debug("pulseWidthMicros ==> ");
                debugLn(pulseWidthMicros);
                debug ("millisBetweenSteps ==> ");
                debugLn(millisBetweenSteps);
                debug ("RIGHT_LIMIT ==> ");
                debugLn(RIGHT_LIMIT);
                debug("LEFT_LIMIT ==> ");
                debugLn(LEFT_LIMIT);
                debug("FRONT_LIMIT ==>");
                debugLn (FRONT_LIMIT);
                debug ("BACK_LIMIT ==> ");
                debugLn(BACK_LIMIT);
                debug("SAW_ZERO ==> ");
                debugLn(SAW_ZERO);
                debug ("maxAcceleartion ==> ");
                debugLn(maxAcceleration);
                debug("distPerStep ==> ");
                Serial.println(distPerStep,8);
                debug("maxMotorSpeed == > ");
                debugLn(maxMotorSpeed);
                debug("workingMotorSpeed ==> ");
                debugLn(workingMotorSpeed);
                break;
            }  
         case 'H':
          {
            eeAddress = HEIGHT_ADDRESS;
            eeAddress = loadHeightsFromFile(1);
            EEPROM.put(4095, eeAddress);
            break;
          }  
          case 'C':
            {
              debug ("Size of the preset lookup structure ==> ");
              debugLn(sizeof(preSetLookup));
              debugLn("");
              break;
            }
          case 'L':
            for (int x = 0; x< boardMemory; x++)
              {
                debug (x);
                debug (" ==> ");
                debugLn (EEPROM.read(x));
              }
            break;
          case 'R':
            {
            // eeAddress = sizeof(preSetLookup);
              eeAddress = HEIGHT_ADDRESS;
              EEPROM.get(eeAddress,preSetLookup);
              debugLn(preSetLookup.index);
              debugLn(preSetLookup.inches);
              Serial.println(preSetLookup.decimal, 6);
              Serial.println(preSetLookup.steps, 6);
              debugLn("");
        //      eeAddress += sizeof(preSetLookup);
              break;
            }
          case 'N':
            {
              eeAddress += sizeof(preSetLookup);
              EEPROM.get(eeAddress,preSetLookup);
              debugLn(preSetLookup.index);
              debugLn(preSetLookup.inches);
              Serial.println(preSetLookup.decimal, 6);
              Serial.println(preSetLookup.steps, 6);
              debugLn("");
              break;
            }
      
      }
}
>>>>>>> parent of c163956 (version 2  - production testing)
