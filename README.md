# TemperatureServer
c++ code for a digital thermometer and web server on a Digilent MX7 Development Board
using MPIDE/ArduinoIDE code structure.<br>

This program uses the a MX7 developement board, a TC1 temperature sensor and SSD to create a
Digital thermometer.  It then displays the reading of the thermometer on a html webpage that can be 
seen by any computer on the same network as the device.  The ip of the server is originally set to 10.0.0.97 but can be changed in the
source code as needed.

The URL for the webpage is set to the ip address of the server.<br>
Example: http://10.0.0.97

This server can be paired with the Temperature Monitor Application to constantly send temperature data to an device iOS device.

## Materials Required
Digilent MX7 Development Board<br>
TC1 Sensor<br>
Seven Segment Display<br>
