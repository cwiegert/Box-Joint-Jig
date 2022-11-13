# Box-Joint-Jig
to set the initial configuration - use the EEPROM writer.   It will set the values from the globvars.h in the EEPROM library into EEPROM.   The boxJoint will read that config from EEPROM to set the stepper motor controllers and pin configruations.

Once you have the settings loaded into EEPROM, you can load the box joint firmware onto your MEGA... and load the Nextion UI, and you should be set to go.   I've added a joystick for fine movement control.   The buttons in the UI will automate movement to limit stops, but will not job the carraige or the sled.   
