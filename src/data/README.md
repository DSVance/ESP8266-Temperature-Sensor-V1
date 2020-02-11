# Web Interface

The ESP8266 micro controller that hosts the temperature sensor system also runs a web server that provides an external interface to the system.  The system is configured through the web interface and there is also a graphical display of the current temperature provided.

This collection of files (exclusing the .md files) is uploaded to an SPI Flash File System (SPIFFS) on the ESP8266 controller and surfaced by the system's web server.


The initial web page for the web interface looks like this

<img src="images/Home.png" alt="Home" width="400"/>


The current temperature display page looks like this

<img src="images/Data.png" alt="Home" width="400"/>
