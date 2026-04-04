% defineBusTypes.m
% Defines Simulink Bus objects for all MicroMouse C modules.
% Run this script (or add it to the model PreLoadFcn) before opening any model.
%
% IMPORTANT: Bus variable names MUST match C typedef names exactly so that
% Simulink code generation emits the correct C type in generated headers.
%
% Buses defined:
%   INA219_t    - Power monitor (INA219.h)
%   IMU_t       - Inertial measurement unit (IMU.h)
%   VL53L0_t    - Single VL53L0X sensor result (VL53L0X.h)
%   Motor_t     - Single motor state (Motors.h)
%   LED_t       - LED outputs (LEDs.h)
%   SW_t        - Switch/button inputs (Buttons.h)
%   SSD1306_t   - OLED display cursor state (SSD1306.h)

%% ====================================================================
%  INA219_t  —  mirrors INA219_t in INA219.h
%% ====================================================================
clear elems;
elems(1)  = Simulink.BusElement; elems(1).Name  = 'busVoltage_mV';      elems(1).DataType  = 'uint16';
elems(2)  = Simulink.BusElement; elems(2).Name  = 'shuntVoltage_mV';    elems(2).DataType  = 'int16';
elems(3)  = Simulink.BusElement; elems(3).Name  = 'current_mA';         elems(3).DataType  = 'int16';
elems(4)  = Simulink.BusElement; elems(4).Name  = 'power_mW';           elems(4).DataType  = 'uint16';
elems(5)  = Simulink.BusElement; elems(5).Name  = 'batteryLife';        elems(5).DataType  = 'int8';
elems(6)  = Simulink.BusElement; elems(6).Name  = 'calibrationValue';   elems(6).DataType  = 'uint16';
elems(7)  = Simulink.BusElement; elems(7).Name  = 'currentDivider_mA';  elems(7).DataType  = 'int16';
elems(8)  = Simulink.BusElement; elems(8).Name  = 'powerMultiplier_mW'; elems(8).DataType  = 'int16';
elems(9)  = Simulink.BusElement; elems(9).Name  = 'totalPowerUsed';     elems(9).DataType  = 'single';
elems(10) = Simulink.BusElement; elems(10).Name = 'initialized';        elems(10).DataType = 'uint8';

INA219_t = Simulink.Bus;
INA219_t.Elements   = elems;
INA219_t.HeaderFile = 'INA219.h';
INA219_t.Description = 'INA219 power monitor sensor data';
assignin('base', 'INA219_t', INA219_t);


%% ====================================================================
%  IMU_t  —  mirrors IMU_t in IMU.h
%% ====================================================================
clear elems;
elems(1)  = Simulink.BusElement; elems(1).Name  = 'Accel_X';    elems(1).DataType = 'single';
elems(2)  = Simulink.BusElement; elems(2).Name  = 'Accel_Y';    elems(2).DataType = 'single';
elems(3)  = Simulink.BusElement; elems(3).Name  = 'Accel_Z';    elems(3).DataType = 'single';
elems(4)  = Simulink.BusElement; elems(4).Name  = 'Gyro_X';     elems(4).DataType = 'single';  % rad/s
elems(5)  = Simulink.BusElement; elems(5).Name  = 'Gyro_Y';     elems(5).DataType = 'single';
elems(6)  = Simulink.BusElement; elems(6).Name  = 'Gyro_Z';     elems(6).DataType = 'single';
elems(7)  = Simulink.BusElement; elems(7).Name  = 'Gyro_DPS_X'; elems(7).DataType = 'single';  % deg/s
elems(8)  = Simulink.BusElement; elems(8).Name  = 'Gyro_DPS_Y'; elems(8).DataType = 'single';
elems(9)  = Simulink.BusElement; elems(9).Name  = 'Gyro_DPS_Z'; elems(9).DataType = 'single';
elems(10) = Simulink.BusElement; elems(10).Name = 'Temp_C';     elems(10).DataType = 'single';

IMU_t = Simulink.Bus;
IMU_t.Elements   = elems;
IMU_t.HeaderFile = 'IMU.h';
IMU_t.Description = 'IMU accelerometer, gyroscope and temperature data';
assignin('base', 'IMU_t', IMU_t);


%% ====================================================================
%  VL53L0_t  —  mirrors VL53L0_t in VL53L0X.h (one sensor instance)
%% ====================================================================
clear elems;
elems(1) = Simulink.BusElement; elems(1).Name = 'Distance';    elems(1).DataType = 'uint32';  % mm
elems(2) = Simulink.BusElement; elems(2).Name = 'rawDistance'; elems(2).DataType = 'uint16';  % mm uncorrected
elems(3) = Simulink.BusElement; elems(3).Name = 'Status';      elems(3).DataType = 'uint32';  % 0 = OK
elems(4) = Simulink.BusElement; elems(4).Name = 'Ambient';     elems(4).DataType = 'uint16';  % kcps/spad fix9.7
elems(5) = Simulink.BusElement; elems(5).Name = 'Signal';      elems(5).DataType = 'uint16';  % kcps/spad fix9.7
elems(6) = Simulink.BusElement; elems(6).Name = 'spadCnt';     elems(6).DataType = 'uint16';  % fix8.8
elems(7) = Simulink.BusElement; elems(7).Name = 'rangeStatus'; elems(7).DataType = 'uint8';
elems(8) = Simulink.BusElement; elems(8).Name = 'initialized'; elems(8).DataType = 'uint8';

VL53L0_t = Simulink.Bus;
VL53L0_t.Elements   = elems;
VL53L0_t.HeaderFile = 'VL53L0X.h';
VL53L0_t.Description = 'Single VL53L0X ToF sensor result';
assignin('base', 'VL53L0_t', VL53L0_t);


%% ====================================================================
%  Motor_t  —  mirrors Motor_t in Motors.h (single motor)
%% ====================================================================
clear elems;
elems(1) = Simulink.BusElement; elems(1).Name = 'magnitude';   elems(1).DataType = 'int16';  % -100 to +100
elems(2) = Simulink.BusElement; elems(2).Name = 'encoderRate'; elems(2).DataType = 'int16';  % ticks/s

Motor_t = Simulink.Bus;
Motor_t.Elements   = elems;
Motor_t.HeaderFile = 'Motors.h';
Motor_t.Description = 'Single motor: signed speed magnitude and encoder rate';
assignin('base', 'Motor_t', Motor_t);


%% ====================================================================
%  LED_t  —  mirrors LED_t in LEDs.h  (instance: LEDS)
%% ====================================================================
clear elems;
elems(1) = Simulink.BusElement; elems(1).Name = 'LED0'; elems(1).DataType = 'uint8';
elems(2) = Simulink.BusElement; elems(2).Name = 'LED1'; elems(2).DataType = 'uint8';
elems(3) = Simulink.BusElement; elems(3).Name = 'LED2'; elems(3).DataType = 'uint8';

LED_t = Simulink.Bus;
LED_t.Elements   = elems;
LED_t.HeaderFile = 'LEDs.h';
LED_t.Description = 'LED output states (0 = off, 1 = on)';
assignin('base', 'LED_t', LED_t);


%% ====================================================================
%  SW_t  —  mirrors SW_t in Buttons.h  (instance: SWS)
%% ====================================================================
clear elems;
elems(1) = Simulink.BusElement; elems(1).Name = 'SW0'; elems(1).DataType = 'uint8';
elems(2) = Simulink.BusElement; elems(2).Name = 'SW1'; elems(2).DataType = 'uint8';

SW_t = Simulink.Bus;
SW_t.Elements   = elems;
SW_t.HeaderFile = 'Buttons.h';
SW_t.Description = 'Switch/button states (0 = released, 1 = pressed)';
assignin('base', 'SW_t', SW_t);


%% ====================================================================
%  SSD1306_t  —  mirrors SSD1306_t in SSD1306.h
%% ====================================================================
clear elems;
elems(1) = Simulink.BusElement; elems(1).Name = 'CurrentX';      elems(1).DataType = 'uint16';
elems(2) = Simulink.BusElement; elems(2).Name = 'CurrentY';      elems(2).DataType = 'uint16';
elems(3) = Simulink.BusElement; elems(3).Name = 'Inverted';      elems(3).DataType = 'uint8';
elems(4) = Simulink.BusElement; elems(4).Name = 'Initialized';   elems(4).DataType = 'uint8';
elems(5) = Simulink.BusElement; elems(5).Name = 'oled_string1';  elems(5).DataType = 'uint8';  elems(5).Dimensions = [1 18];
elems(6) = Simulink.BusElement; elems(6).Name = 'oled_string2';  elems(6).DataType = 'uint8';  elems(6).Dimensions = [1 18];
elems(7) = Simulink.BusElement; elems(7).Name = 'oled_string3';  elems(7).DataType = 'uint8';  elems(7).Dimensions = [1 18];
elems(8) = Simulink.BusElement; elems(8).Name = 'oled_string4';  elems(8).DataType = 'uint8';  elems(8).Dimensions = [1 18];
elems(9) = Simulink.BusElement; elems(9).Name = 'oled_string5';  elems(9).DataType = 'uint8';  elems(9).Dimensions = [1 18];

SSD1306_t = Simulink.Bus;
SSD1306_t.Elements   = elems;
SSD1306_t.HeaderFile = 'SSD1306.h';
SSD1306_t.Description = 'SSD1306 OLED display state with text buffers';
assignin('base', 'SSD1306_t', SSD1306_t);


%% Done
disp('MicroMouse bus types defined: INA219_t, IMU_t, VL53L0_t, Motor_t, LED_t, SW_t, SSD1306_t');
