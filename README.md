# GameCube-BLE-adaptor  
The Bluetooth adaptor you've always dreamed for your GameCube. An external adaptor that will allow you to connect a bluetooth controller to the NGC socket of your GameCube (or Wii), and enjoy the freedom of wireless.  
  
The code regarding the communication protocol of the GameCube controller has been reverse engineered and is available [online](https://www.arduino.cc/reference/en/libraries/nintendo/) [1][2][3]. The existing code was developed to convert a standard wired GameCube controller into a bluetooth one [4]. However, this mod only works for bluetooth compatible devices, rendering the new modded controller incompatible with the GameCube or Wii consoles.  
  
This project will be towards the development of a Bluetooth receiver for the NGC port of the GameCube and Wii consoles. Allowing bluetooth controller such as the "PowerA Bluetooth Controller for the Nintendo Switch - GameCube Style" to be compatible with these old consoles [5].  
  
# Discussion  
From the documentation, the Data pin of the NGC controller is active high 3.3V, but does not have an internal pull-up resistor [2]. Therefore, we can use a 1K resistor that bridge between the 3.3V rail and the Data pin. This in turn will be connected to the Arduino, which will sink the signal whenever a 'low' bit is required to be sent. The communication is initiated by the console to the controller with a 24-bit string, to which the controller reply with a 64-bit string containing button state and joystick data, at a rate of 3.95us per bit [2].  
  
"A 'Low' bit is signalled by a 3us 'Low' followed by 1us 'High'"  
"A 'High' bit is signalled by 1us 'Low' followed by 3us 'High'"  
Each strings of data is terminated with a single 'High' at the stop bit  
  
ref: [2]  
  
This communication protocole is already available as an Arduino library [3].  
  
# Abbreviations  
NGC : Nintendo GameCube  
  
# References  
[1] https://simplecontrollers.bigcartel.com/gamecube-protocol 
[2] http://www.int03.co.uk/crema/hardware/gamecube/gc-control.html  
[3] https://www.arduino.cc/reference/en/libraries/nintendo/
[4] https://github.com/NathanReeves/BlueCubeMod  
[5] https://www.amazon.ca/PowerA-Wireless-Controller-Nintendo-Switch/dp/B07GXLBCC3  
  
