// -----------------------------------------------------------------------------
// ------------------------------------------------------------< sensor.ino >---
// -----------------------------------------------------------------------------
//
// PURPOSE: Pull values from a DS18B20 temperature sensor and display them.
//
// MACHINE: ESP8266
//
// NOTES:   -  The ESP8266 has two watchdog timers (one software, one hardware)
//             that guard against a hung system or infinite loop by restarting
//             the system if either of the timers goes off.  The software timer
//             is set for ~3200ms while the hardware timer is set for ~8000ms.
//             As it executes, functions in the ESP8266 core periodically reset
//             the timers (returns the elapsed time to zero), but user level
//             code (sketch code) typically does not directly reset the timers
//             (though it could).  This means that in practice, sketch code
//             should not continuously execute for longer than ~32000ms or it
//             runs the risk of causing a WDT system restart.
//
//                NOTE: Calling delay() or yeild() in sketch code temporarily
//                returns control to the system, causing the watch dog timers
//                to reset and avoiding the undesirable WDT system restart.
//
//          -  The ESP8266 is a single threaded system so multiple tasks do not
//             execute simultaneously - they must share the processor and take
//             turns executing.  This can be a problem when a sketch takes a
//             long time to execute because it has a lot to do, or it needs to
//             wait for extended periods of time (like this sketch), but it also
//             uses services that need time to execute in the background - some
//             system services, a web server, or web socket for example.  If the
//             sketch consumes too much time and the services don't get a chance
//             to run, the system can become unresponsive and not behave as
//             expected.
//
//             One technique used here to avoid unresponsiveness is to set an
//             interrupt timer in setup() that fires an event at a specified
//             interval, causing a specified function to execute when the event
//             fires.  This avoids having long delays inside the loop()
//             function which gives the various services time to execute.
//
//             WARNING: An issue with interrupt handling functions is that they
//             cannot do anything that would cause another interrupt to be
//             triggered such as interacting with a hardware interface that uses
//             interrupts, and includes things like calling delay() or yield().
//             Those things will typically lead to a system crash or restart.
//             One way to get around that restriction is to have the handler do
//             nothing but schedule another function to execute the next time
//             the loop() function returns.  That is done by calling the
//             schedule_function() routine in the interrupt handler.
//
//          -  Temperature sensor:  DS18B20
//
//                Red  --> 3 - 5vdc
//                Blue/Black --> Ground
//                Yellow/White --> Data
//
//          -  Display screen:  SSD1306 128x64 OLED using I2C for comm.
//
//                NodeMCU GPIO5  (D1)  -->  OLED SCL
//                NodeMCU GPIO4  (D2)  -->  OLED SDA
//                NodeMCU +3.3   (3V3) -->  VCC
//                NodeMCU Ground (GND) -->  GND
//
//             A degree symbol can be entered with the keyboard using the
//             numeric key pad.  While holding tje 'Alt' key enter:  Alt+248
//             or Alt+0176.  The symbol displays correct in the serial window
//             but does not display correctly on the SSD1306 screen.  :(
//
//             Use (char)247 for the degree symbol on the SSD1306 OLED screen.
//
// TODO:    - Check for DEVICE_DISCONNECTED after reading temp from sensor.
//          - Set high and low alarm values on sensor object in setup.
//          - Create an alarm handler function.
//          - Set the alarm handler function on sensor object in setup.
//          - Check for sensor alarms in loop.
//          - Enable a relay in loop if there are no alarms.
//
// HISTORY:
// --------- --- - -------------------------------------------------------------
// 25Oct2018 DSV - Initial development.
//
// -----------------------------------------------------------------------------


#include <assert.h>              // Assert statements
#include <SPI.h>                 // Serial Peripheral Interface communication
#include <Wire.h>                // Two Wire Interface communication
#include <FS.h>                  // SPIFFS file system

#include "Sensor.h"              // Definitions common to whole Sensor sketch
#include "EEPROMConfig.h"        // Read a write configuration to FLASH storage
#include "WebConfig.h"           // Web server event handlers

#include <Schedule.h>            // Scheduled function ability

#include <Adafruit_GFX.h>        // Basic graphics library for screen drivers
#include <Adafruit_SSD1306.h>    // Screen driver for SSD1306 OLED screen

#include <ESP8266WiFi.h>         // Wifi operations on ESP8266
#include <ESP8266WebServer.h>    // Simple web server
#include <WebSocketsServer.h>    // Accept web socket connections from client
#include <ESP8266SSDP.h>         // Simple service discovery

#include <WiFiClient.h>          // Define a wifi client connection
#include <WifiConnect.h>         // Wifi connection credentials

#include <WiFiUdp.h>             // Over-the-air software updates
#include <ArduinoOTA.h>          // Over-the-air software updates

#include <OneWire.h>             // One-wire communication bus
#include <DallasTemperature.h>   // DS18B20 temperature ICs

#include <ArduinoJson.h>         // Send and recieve JSON encoded messages
// WARNING: Use version 5, not version 6.



// Specify the NodeMCU pin number connected to data of DS18B20 temp sensor.
#define ONE_WIRE_BUS  D4

// Definitions for the 'Services' flag field.
#define  SERIAL_CONNECTED        0x0001
#define  WIFI_STATION_CONNECTED  0x0002
#define  WIFI_ACCESS_CONNECTED   0x0004
#define  WEB_SERVER_STARTED      0x0008
#define  SSDP_STARTED            0x0010
#define  SPIFFS_STARTED          0x0020

#define  WIFI_CONNECTED          ( WIFI_STATION_CONNECTED | WIFI_ACCESS_CONNECTED )

// Define the size of the SSD1306 screen.
#define SCREEN_WIDTH      128       // Width in pixels
#define SCREEN_HEIGHT      64       // Height in pixels
#define SCREEN_RESET_PIN   -1       // Reset pin not used



//
// Global variables
//

// Instantiate a OneWire instance to communicate with devices.
OneWire OneWireComm ( ONE_WIRE_BUS );

// Instantiate a temperature device instance for the DS18B20 sensor
// and pass the OneWire reference to it.
DallasTemperature  Sensors ( &OneWireComm );

// Instantiate a web server.  This specifies standard http port 80, but
// that can be changed if a different port is specified to the 'begin'
// method (from the stored config data) when the server is started.
ESP8266WebServer  WebServer ( 80 );

// Instantiate a web socket server on port 81.
WebSocketsServer  WebSocket = WebSocketsServer ( 81 );

// Instantiate a SSD1306 display object.
// NOTE: The TwoWire instance 'Wire' referenced here was declared in Wire.h
Adafruit_SSD1306 Screen ( SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, SCREEN_RESET_PIN );



PConfig_t   ConfigData = { 0 };
uint16_t    Services = 0;
char        ChipUUID[ SSDP_UUID_SIZE ] = { 0 };
os_timer_t  TemperatureTimer;


typedef struct SENSOR_DATA
{
  const char*    Type;
  const char*    Name;
  const char*    Units;
  float          Value;
  uint32_t       Time;
  uint32_t       Interval;
} SData_t ;

#define SENSORDATA_JSON_SIZE ( JSON_OBJECT_SIZE ( 6 ) )


// Convert celsius to fahrenheit.
#define C2F(c) ( ((float)c * (9.0/5.0)) + 32.0 )

// Convert fahrenheit to celsius.
#define F2C(f) ( ((float)f - 32.0 ) * (5.0/9.0) )



//
// Function prototypes
//

void InitDisplay (
  Adafruit_SSD1306* Screen
);

void UpdateDisplay (
  Adafruit_SSD1306* Screen,
  float             SensorValueF,
  bool              DeviceState
);

void ClearLine (
  Adafruit_SSD1306* Screen,
  uint8_t           TopRow,
  uint8_t           TextSize
);

void SensorAction (
  void*    Args
);

void WebSocketEvent (
  uint8_t  num,
  WStype_t type,
  uint8_t* payload,
  size_t   payload_length
);

void MakeUUID (
  char*    Result
);

bool DeserializeJSON (
  SData_t&    Data,
  char*       JSONBuffer
);

size_t SerializeJSON (
  const SData_t& Data,
  char*          JSONBuffer,
  size_t         MaxSize,
  boolean        Pretty = false
);

void Deblank ( 
   char*    Value, 
   uint8_t  ValueLength, 
   char*    NewValue, 
   uint8_t* NewValueLength,
   char     BlankReplacement
);



// -----------------------------------------------------------------------------
// -----------------------------------------------------------------< setup >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Called only once when the sketch starts, to initialize global
//             variables, pin modes, start using libraries, etc.
//
// PARAMETERS: None
//
// RETURNS:    void
//
// NOTES:      -  This sketch uses a timer that fires at a set interval and
//                invokes a specified function when it does.  The sequence of
//                routines that executes when the timer fires cause the sensor
//                data to be updated and transmitted to the connected clients.
//
// HISTORY:
// --------- --- - -------------------------------------------------------------
// 25Oct2018 DSV - Initial development.
//
// -----------------------------------------------------------------------------

void setup()
{
   uint16_t    Tries;
   boolean     Status;
   IPAddress   ConnectedIP;
   IPAddress   ConnectedNetMask;
   IPAddress   ConnectedGateway;



  // Set the built-in LED to output and blink it to show program start.
  pinMode ( LED_BUILTIN, OUTPUT );
  digitalWrite ( LED_BUILTIN, LOW );
  delay ( 250 );
  digitalWrite ( LED_BUILTIN, HIGH );


  // Configure GPIO/D-pins for output and turn them off.
  pinMode ( D6, OUTPUT );
  pinMode ( D7, OUTPUT );
  digitalWrite ( D6, LOW );
  digitalWrite ( D7, LOW );


  EEPROM.begin ( sizeof ( PConfig_t ) );

  // Set all ROM values to zero the first time the EEPROM is used or whenever a
  // return to the initial defaults is desired, otherwise leave commented out.
  //
  // ClearROM ( sizeof ( PConfig_t ) );


  EEPROM.get ( PCONFIG_OFFSET, ConfigData );
  if ( ! ( ConfigData.Flags & CONFIG_VALUES_INITIALIZED ) )
  {
    SetROMDefaults ( &ConfigData );
    EEPROM.get ( PCONFIG_OFFSET, ConfigData );
  }

  Status = false;
  for ( int i = 0; i < BAUD_LIST_SIZE && Status == false; i++ )
  {
    if ( ConfigData.SerialBaud == BaudList[ i ] )
    {
      Status = true;
    }
  }

  if ( Status == false )
  {
    ConfigData.SerialBaud = DEFAULT_BAUD;
  }

  // Initialize serial output and wait for it to complete intialization.
  Serial.begin ( ConfigData.SerialBaud );

  Tries = 0;
  while ( !Serial && Tries < 30 )
  {
    Tries++;
    delay ( 500 );
  }

  if ( Serial )
  {
    Services |= SERIAL_CONNECTED;
  }
  Serial.println ( "\nDS18B20 temperature sensor sketch started ..." );

  Serial.printf ( "Software version %d.%d compiled %s \n", 
                  SENSOR_VERSION_MAJOR, 
                  SENSOR_VERSION_MINOR, 
                  SENSOR_VERSION_DATE 
                );

  // Now that Serial is up an running, show the configuration info.
  ShowROMValues ( &ConfigData, "Initial configuration values:" );

  if ( ConfigData.Version != PCONFIG_VERSION )
  {
     // WARNING: The version number of the config structure compiled into the 
     // software does not match the version number stored in the ROM.  This 
     // could be a problem if the sizes or member layouts don't match!
     Serial.printf ( "WARNING: Configuration structure version difference: \n" );
     Serial.printf ( "         ROM version      = 0x%04x \n", ConfigData.Version );
     Serial.printf ( "         Compiled version = 0x%04x \n", PCONFIG_VERSION  );
  }

  
  // Initialize output for screen at address 0x3C.
  Screen.begin ( SSD1306_SWITCHCAPVCC, 0x3C );

  // Display the IP address to the screen.
  Screen.clearDisplay();
  Screen.setTextColor ( WHITE );
  Screen.setTextSize ( 2 );
  Screen.setCursor ( 0, 0 );
  Screen.println ( "Connecting" );
  Screen.display();


   // Connect to the wifi network.
   Screen.setTextSize ( 1 );
   Screen.setCursor ( 0, 30 );


   if ( ConfigData.Flags & CONFIG_WIFI_STATION_ENABLED )
   {
      WiFi.mode ( WIFI_STA );

      if ( ConfigData.LabelLength > 0 && ConfigData.LabelLength < 32 )
      {
         char     StationName[ PCONFIG_MAX_LABEL + 1 ];
         uint8_t  StationNameLength =  PCONFIG_MAX_LABEL + 1;

         Deblank ( ConfigData.Label,
                   ConfigData.LabelLength,
                   StationName,
                   &StationNameLength,
                   '_'
                 );

         WiFi.hostname ( StationName );        
      }

      WiFi.begin ( ConfigData.WifiSSID, ConfigData.WifiPassword );

      if ( WiFi.waitForConnectResult() != WL_CONNECTED )
      {
         Serial.println ( "ERROR: Wifi station failed to start" );
      }

      else
      {
         Services |= WIFI_STATION_CONNECTED;

         ConnectedIP = WiFi.localIP();
         ConnectedNetMask = WiFi.subnetMask();
         ConnectedGateway = WiFi.gatewayIP();
      }
   }

   if ( ! ( Services & WIFI_STATION_CONNECTED ) )
   {
      char  APName[ PCONFIG_MAX_SSID + 1 ];
      int   APNameLength;

      APNameLength = sprintf ( APName, "ESP%d", ESP.getChipId() );
      APName[ APNameLength ] = 0;

      Serial.printf ( "Starting wifi soft access point \"%.*s\" \n", APNameLength, APName );

      WiFi.mode ( WIFI_AP );

      WiFi.softAPConfig ( ConfigData.AccessIP, ConfigData.Gateway, ConfigData.NetMask );

      Status = WiFi.softAP ( APName );
      if ( Status == false )
      {
         Serial.println ( "Wifi AP mode failed to start!" );
         Screen.println ( "Wifi not connected" );
      }

      else
      {
         Services |= WIFI_ACCESS_CONNECTED;

         Serial.println ( "Wifi AP mode ready for connections" );

         ConnectedIP = WiFi.softAPIP();
         ConnectedNetMask = WiFi.subnetMask();
         ConnectedGateway = WiFi.gatewayIP();

         //
         // Report the AP name on the top line of the screen.
         //
         ClearLine ( &Screen, 0, 2 );
         Screen.setTextSize ( 2 );
         Screen.setCursor ( 0, 0 );
         Screen.print ( APName );
      }
   }

   if (  Services & WIFI_CONNECTED )
   {
      //
      // Report the connection parameter to the serial log and screen.
      //
      Serial.print   ( "Wifi connected to SSID " );
      Serial.println ( WiFi.SSID() );
      Serial.print   ( "Wifi IP address " );
      Serial.println ( ConnectedIP );
      Serial.print   ( "Netmask: " );
      Serial.println ( ConnectedNetMask );
      Serial.print   ( "Gateway: " );
      Serial.println ( ConnectedGateway );

      Screen.setTextSize ( 1 );
      Screen.setCursor ( 0, 30 );
      Screen.print   ( "IP: " );
      Screen.println ( ConnectedIP );
      Screen.display();


      //
      // Update the stored IP address to the one currently in use.
      // NOTE: Doing this will allow the sensor data page to work
      // correctly even while the web server is in AP mode.
      //
      IPAddress   StoredIP ( ConfigData.StationIP[ 0 ],
                             ConfigData.StationIP[ 1 ],
                             ConfigData.StationIP[ 2 ],
                             ConfigData.StationIP[ 3 ]
                           );
      if ( ConnectedIP != StoredIP )
      {
         for ( int i = 0; i < 4; i++ )
         {
           SetROMValue ( PCONFIG_OFFSET_STATIONIP + i, &ConnectedIP [ i ], 1 );
           ConfigData.StationIP[ i ] = ConnectedIP[ i ];
         }
      }

      //
      // Define the functions necessary for over-the-air software updates.
      //
      ArduinoOTA.onStart ( []()
      {
         //
         // The update command is either U_FLASH or U_SPIFFS.
         // Apparently U_SPIFFS was replaced by U_FS in newer release.
         //
         Serial.printf ( "Started update of %s \n",
                         ( ArduinoOTA.getCommand() == U_FLASH )
                         ? "sketch"
                         : "file system"
                       );

         if ( ArduinoOTA.getCommand() == U_FS )
         {
            // Unmount the file system prior to updating it.
            SPIFFS.end();
         }

         //
         // Disable services (like timers) that might interfere with the update.
         //
         os_timer_disarm ( &TemperatureTimer );


         Screen.clearDisplay();
         Screen.setTextSize ( 2 );
         Screen.setCursor ( 0, 0 );
         Screen.println ( " Updating " );
         Screen.display();
         delay ( 1000 );
      } );

      ArduinoOTA.onEnd ( []()
      {
         Serial.println ( "Update complete" );

         Screen.clearDisplay();
         Screen.setTextSize ( 2 );
         Screen.setCursor ( 0, 0 );
         Screen.println ( " Finished " );
         Screen.display();

         // Reset the processor after a short delay.
         delay ( 1000 );
      } );

      ArduinoOTA.onProgress ( [] ( unsigned int progress, unsigned int total )
      {
         Serial.printf ( "Progress: %u of %u = %u%%\r", progress, total, ( progress / ( total / 100 ) ) );

         Screen.setTextSize ( 2 );
         ClearLine ( &Screen, 25, 2 );
         Screen.setCursor ( 5, 25 );
         Screen.printf ( "   %u%%", ( progress / ( total / 100 ) ) );

         Screen.setTextSize ( 1 );
         ClearLine ( &Screen, 50, 1 );
         Screen.setCursor ( 10, 50 );
         Screen.printf ( "%u of %u", progress, total );

         Screen.display();
      } );

      ArduinoOTA.onError ( [] ( ota_error_t error )
      {
         Serial.printf  ( "Update error[%u]: ", error );
         Serial.println ( ( error == OTA_AUTH_ERROR )    ? "Auth Failed"    :
                          ( error == OTA_BEGIN_ERROR )   ? "Begin Failed"   :
                          ( error == OTA_CONNECT_ERROR ) ? "Connect Failed" :
                          ( error == OTA_RECEIVE_ERROR ) ? "Receive Failed" :
                          ( error == OTA_END_ERROR )     ? "End Failed"     :
                                                        "Unknown"
                        );

         Screen.setTextSize ( 1 );
         Screen.setCursor ( 2, 40 );
         Screen.printf  ( "Update error[%u]:", error );
         Screen.setCursor ( 2, 50 );
         Screen.println ( ( error == OTA_AUTH_ERROR )    ? "Auth Failed"    :
                          ( error == OTA_BEGIN_ERROR )   ? "Begin Failed"   :
                          ( error == OTA_CONNECT_ERROR ) ? "Connect Failed" :
                          ( error == OTA_RECEIVE_ERROR ) ? "Receive Failed" :
                          ( error == OTA_END_ERROR )     ? "End Failed"     :
                                                           "Unknown"
                        );

         Screen.display();
      } );

      ArduinoOTA.begin();


      // Mount the file system
      if ( !SPIFFS.begin() )
      {
         // Serious problem
         Serial.println ( "ERROR: SPIFFS mount failed" );
         Services |= SPIFFS_STARTED;
      }

      else
      {
         Serial.println ( "SPIFFS mount succesfull" );
      }

      // Setup handlers for web server events.
      WebEvents ( &WebServer, &ConfigData, &SSDP );


      // Start the web server running on the specified port.
      WebServer.begin ( ConfigData.WebServerPort );

      Serial.printf ( "Web server started on port %d \n", ConfigData.WebServerPort );
      Screen.setCursor ( 2, 40 );
      Screen.print   ( "Web server port " );
      Screen.println ( ConfigData.WebServerPort );

      // Configure the Simple Service Discovery Protocol values.
      SSDP.setSchemaURL ( "description.xml" );
      SSDP.setHTTPPort ( ConfigData.WebServerPort );
      SSDP.setName ( "ESP8266 Temp Sensor" );
      SSDP.setSerialNumber ( "001788102201" );
      SSDP.setURL ( "/" );
      SSDP.setModelName ( "Vance ESP8266 Temp Sensor 1.0" );
      SSDP.setModelNumber ( "929000226503" );
      SSDP.setModelURL ( "http://enterprise.youandmetx.us/Experiments/TemperatureData.html" );
      SSDP.setManufacturer ( "D S Vance" );
      SSDP.setManufacturerURL ( "http://enterprise.youandmetx.us" );
      // SSDP.setDeviceType ( "upnp:rootdevice" );
      SSDP.setDeviceType ( "urn:schemas-upnp-org:device:SensorManagement:1" );

      MakeUUID ( ChipUUID );
      Serial.printf  ( "UUID = %s \n", ChipUUID );
      SSDP.setUUID ( ChipUUID );

      SSDP.begin();
      Serial.println ( "SSDP started" );


      // Start the web-socket server running, passing an event
      // handler function to take care of incoming messages.
      WebSocket.onEvent ( WebSocketEvent );
      WebSocket.begin();
      Serial.println ( "Web socket server started" );
      Screen.setCursor ( 2, 50 );
      Screen.println ( "Web socket started" );
   }

   else
   {
      Serial.println ( "No web server started" );
      Serial.println ( "No web socket server started" );
      Screen.setCursor ( 2, 40 );
      Screen.println ( "Web server not started" );
      Screen.setCursor ( 2, 50 );
      Screen.println ( "Web socket not started" );
   }


   Screen.display();


   if ( ConfigData.Flags & CONFIG_TEMP_PROBE_CONNECTED )
   {
      DEBUG_PRINTF ( &ConfigData, "DEBUG: DS18B20 temperature sensor is in use \n" );

      // Start the sensor communications.
      Sensors.begin ();

      // Set the global sensor resolution (9..12).
      Sensors.setResolution ( 10 );

      if ( ConfigData.Flags & CONFIG_DEBUG_MESSAGE_ENABLED )
      {
         uint8_t OWDeviceCount = Sensors.getDeviceCount();
         Serial.printf ( "DEBUG: Found %u OneWire device%s \n",
                         OWDeviceCount,
                         ( OWDeviceCount > 1 ) ? "s" : " "
                       );

         for ( uint8_t i = 0; i < OWDeviceCount; i++ )
         {
            uint8_t OWDeviceAddress = 0;
            Sensors.getAddress ( &OWDeviceAddress, i );
            Serial.printf ( "   Device #%d address is: 0x%08x \n", i, OWDeviceAddress );
         }
      }
   }

   else
   {
      Serial.println ( "Using randomly generated fake data values" );

      // In the absence of a temp sensor use randomly generated data.
      // Prime the RNG with garbage value from the analog pin.
      randomSeed ( analogRead ( 0 ) );
   }

   // Ensure the intial info has time to be seen.
   delay ( 5000 );

   // Display the start-up screen content.
   InitDisplay ( &Screen );

   DEBUG_PRINTF ( &ConfigData, "DEBUG: Sensor timer is set to %d milliseconds \n", ConfigData.SensorWaitTime );

   std::string ActionID = "Setup";

   // Take an initial temperature reading before handing control to timer.
   SensorAction ( (void*) ActionID.c_str() );

   // Enable the timer that transmits data to web socket client.
   os_timer_setfn ( &TemperatureTimer, SensorTimerISR, NULL );
   os_timer_arm ( &TemperatureTimer, ConfigData.SensorWaitTime, true );

   return;
}

// ----------------------------------------------------------------< /setup >---



// -----------------------------------------------------------------------------
// ------------------------------------------------------------------< loop >---
// -----------------------------------------------------------------------------
//
// PURPOSE:     Called repeatedly to respond to inputs, update variables, and
//              generate outputs as desired.
//
// PARAMETERS:  void
//
// RETURNS:     void
//
// NOTES:      -  This loop mostly just handles services and connected clients.
//
// HISTORY:
// --------- --- - -------------------------------------------------------------
// 25Oct2018 DSV - Initial development.
//
// -----------------------------------------------------------------------------

void loop ()
{
   if ( Services & WIFI_CONNECTED )
   {
      WebSocket.loop();
      WebServer.handleClient();
      ArduinoOTA.handle();
   }
}

// -----------------------------------------------------------------< /loop >---



// -----------------------------------------------------------------------------
// --------------------------------------------------------< SensorTimerISR >---
// -----------------------------------------------------------------------------
//
// PURPOSE:     An interrupt service routine (ISR) that is called when a timer
//              set in the setup() function fires.
//
// PARAMETERS:  Args - an argument or structure of arguments provided by the
//              routine that set up the timer that triggered this ISR to run.
//
// RETURNS:     void
//
// NOTES:      -  There are restrictions on what an interrupt handler can and
//                cannot safely do.  See the notes in the file header about it.
//                Because of those restrictions, all this routine does is
//                schedule a different funciton to be executed the next time
//                the loop() routine returns.
//
//             -  The timer that invoked the function is automatically reset to
//                fire again as long as it is "armed".
//
//             -  See os_timer_setfn() for timer setup.
//
// HISTORY:
// --------- --- - -------------------------------------------------------------
// 25Oct2018 DSV - Initial development.
//
// -----------------------------------------------------------------------------

void SensorTimerISR ( void* Args )
{
   if ( Args != NULL )
   {
      DEBUG_PRINTF ( &ConfigData, "DEBUG: SensorTimerISR - Args = 0x%08x \n", Args );
   }

   // Schedule a function to be executed the next time loop() returns.
   schedule_function ( std::bind ( &SensorAction, Args ) );
}

// -------------------------------------------------------< /SensorTimerISR >---



// -----------------------------------------------------------------------------
// ----------------------------------------------------------< SensorAction >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Retrieve data from the attached sensor(s) and update any attached
//             displays and connected clients with the new values and status.
//
// PARAMETERS:  Args - Argument block passed to the function.
//
// RETURNS:     void
//
// NOTES:      -  The only value expected in the function argument (Args) is
//                a const char* to a character string label that provides some
//                context about the routine's caller.
//
// HISTORY:
// --------- --- - -------------------------------------------------------------
// 25Oct2018 DSV - Initial development.
//
// -----------------------------------------------------------------------------

void SensorAction ( void* Args )
{
   float    SensorValue;
   bool     DeviceState;
   char     Units[ 2 ];

   if ( Args != NULL )
   {
      DEBUG_PRINTF ( &ConfigData, "DEBUG: SensorAction - %s \n", (const char*) Args );
   }


   sprintf ( Units, "%s", ( ConfigData.Flags & CONFIG_TEMP_DISPLAY_FAHRENHEIT ) ? "F" : "C" );

   // Indicate temperature measurement in process (LED ON).
   digitalWrite ( D7, HIGH );

   if ( ConfigData.Flags & CONFIG_TEMP_PROBE_CONNECTED )
   {
      // Send a command for all devices on the bus
      // to perform a temperature conversion.
      Sensors.requestTemperatures();

      // Get a temperature in degrees C from device 0.
      // NOTE: The sensor's native return value is in celsius.
      SensorValue = Sensors.getTempCByIndex ( 0 );
   }

   else
   {
      // Simulate data with random numbers in abscense of a sensor.
      SensorValue = (float) random ( -199, 4999 );
      SensorValue /= 100;
   }

   if ( ConfigData.Flags & CONFIG_TEMP_DISPLAY_FAHRENHEIT )
   {
      SensorValue = C2F ( SensorValue );
   }

   // Determine the desired device state based on temperature value.
   DeviceState = ( SensorValue > ConfigData.TempLowLimit && SensorValue < ConfigData.TempHighLimit );

   DEBUG_PRINTF ( &ConfigData, "DEBUG: SensorAction - %.1f °%s  Device is %s \n",
                  SensorValue,
                  Units,
                  ( DeviceState == true ) ? "ON" : "OFF"
                );

   // Set the output pin HIGH to turn the device ON or LOW to turn it OFF.
   digitalWrite ( D6, ( DeviceState == true ) ? HIGH : LOW );

   // Indicate temperature measurement complete (LED OFF).
   digitalWrite ( D7, LOW );

   UpdateDisplay ( &Screen, SensorValue, DeviceState );

   if ( WebSocket.connectedClients ( false ) > 0 )
   {
      // There are clients connected to the web socket server.  Send the new
      // sensor data to the clients in a JSON format.


      // The maximum size of the JSON text that the tranmission buffer can hold.
      // If more values are added to the SData_t structure, this size will need
      // to be increased to accommodate the additional characters.
#define  JSON_MAX_TEXT  160

      char     JSONText[ JSON_MAX_TEXT ];
      size_t   JSONTextLength = 0;
      char     Type[] = { "Temperature" };

      // Initialize the const character pointers.
      SData_t  SensorData = { (const char*) Type,
                              (const char*) ConfigData.Label,
                              (const char*) Units,
                            };
      SensorData.Value    = SensorValue;
      SensorData.Time     = 0;
      SensorData.Interval = ConfigData.SensorWaitTime / 1000;


      if ( ConfigData.Flags & CONFIG_DEBUG_MESSAGE_ENABLED )
      {
         // Get the "pretty" version of the JSON for display.
         JSONTextLength = SerializeJSON ( SensorData, (char*) JSONText, JSON_MAX_TEXT, true );
         assert ( JSONTextLength < JSON_MAX_TEXT );
         // NULL termination is probably unnecessary, but just in case.
         JSONText[ JSONTextLength ] = 0;

         Serial.printf ( "JSON text length: %d \n  %.*s \n",
                         JSONTextLength,
                         JSONTextLength,
                         JSONText
                       );
         memset ( JSONText, 0, JSON_MAX_TEXT );
      }

      JSONTextLength = SerializeJSON ( SensorData, (char*) JSONText, JSON_MAX_TEXT );
      assert ( JSONTextLength < JSON_MAX_TEXT );
      // NULL termination is probably unnecessary, but just in case.
      JSONText[ JSONTextLength ] = 0;
      WebSocket.broadcastTXT ( JSONText, JSONTextLength );
   }
}

// ---------------------------------------------------------< /SensorAction >---



// -----------------------------------------------------------------------------
// -----------------------------------------------------------< InitDisplay >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Set the initial values displayed on the screen until the sensors
//             are read and the values can be updated as needed.
//
// PARAMETERS: Screen - A pointer to the screen object operate on.
//
// RETURNS:    void
//
// NOTES:
//
// HISTORY:
// --------- --- - -------------------------------------------------------------
// 25Oct2018 DSV - Initial development.
//
// -----------------------------------------------------------------------------

void InitDisplay (
   Adafruit_SSD1306* Screen
)
{
   Screen->clearDisplay();

   Screen->setTextSize ( 2 );
   Screen->setCursor ( 5, 0 );
   Screen->println ( "Starting" );

   Screen->setCursor ( 20, 23 );
   Screen->println ( "Wait..." );

   Screen->setTextSize ( 1 );
   Screen->setCursor ( 10, 45 );
   Screen->println ( "Low temp trip:  " + (String) ConfigData.TempLowLimit );
   DEBUG_PRINTF ( &ConfigData, "DEBUG: Low temp trip: %d \n", ConfigData.TempLowLimit );

   Screen->setCursor ( 10, 55 );
   Screen->println ( "High temp trip: " + (String) ConfigData.TempHighLimit );
   DEBUG_PRINTF ( &ConfigData, "DEBUG: High temp trip: %d \n", ConfigData.TempHighLimit );

   // Leave the screen set to the temp value display size.
   Screen->setTextSize ( 2 );
   Screen->display();
}

// ----------------------------------------------------------< /InitDisplay >---



// -----------------------------------------------------------------------------
// ---------------------------------------------------------< UpdateDisplay >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Display the temperature sensor value and the associated device
//             status to an attached display screen.
//
// PARAMETERS: Screen - A pointer to the screen object operate on.
//
//             SensorValueF - Temperature sensor value in degrees farenheit.
//
//             DeviceState - Whether the device in ON (true) or OFF (false).
//
// RETURNS:    void
//
// NOTES:
//
// HISTORY:
// --------- --- - -------------------------------------------------------------
// 25Oct2018 DSV - Initial development.
//
// -----------------------------------------------------------------------------

void UpdateDisplay (
   Adafruit_SSD1306* Screen,
   float             SensorValueF,
   bool              DeviceState
)
{
   ClearLine ( Screen, 23, 2 );
   Screen->setCursor ( 20, 23 );

   // NOTE: Character 247 is degree symbol for screen display.
   // The one created with Alt-248 doesn't work for the SSD1306.
   Screen->printf ( "%.1f %cF", SensorValueF, (char)247 );
   Screen->display();


   ClearLine ( Screen, 0, 2 );
   Screen->setCursor ( 0, 0 );
   Screen->println ( ( DeviceState == true ) ? "Device ON" : "Device OFF" );
   Screen->display();
}

// --------------------------------------------------------< /UpdateDisplay >---



// -----------------------------------------------------------------------------
// -------------------------------------------------------------< ClearLine >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Clear a line of text of a given size at a given screen position.
//
// PARAMETERS: Screen - A pointer to the screen object operate on.
//
//             TopRow - Vertical screen position of the top of the text line.
//
//              TextSize - The size of the text on the line being cleared.
//
// NOTES:     - Clearing the line of text turns off all of the pixels in the
//              line by overwriting each row of pixels in BLACK.
//
//            - TextSize is a magnification value applied to the default
//              character size:
//                1 = 1 x (6 x 8) =  6 x  8 (default)
//                2 = 2 x (6 x 8) = 12 x 16
//                3 = 3 x (6 x 8) = 18 x 24
//                4 = 4 x (6 x 8) = 24 x 32
//
// HISTORY:
// --------- --- - -------------------------------------------------------------
// 25Oct2018 DSV - Initial development.
//
// -----------------------------------------------------------------------------

void ClearLine (
   Adafruit_SSD1306* Screen,
   uint8_t           TopRow,
   uint8_t           TextSize
)
{
   if ( Screen != NULL )
   {
      int BottomRow = TopRow + (TextSize * (8 - 1));

      if ( BottomRow > SCREEN_HEIGHT )
      {
         BottomRow = SCREEN_HEIGHT;
      }

      for ( int i = TopRow; i < BottomRow; i++ )
      {
         Screen->drawFastHLine ( 0, i, SCREEN_WIDTH, BLACK );
      }
   }
}

// ------------------------------------------------------------< /ClearLine >---



// -----------------------------------------------------------------------------
// --------------------------------------------------------< WebSocketEvent >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    An event handler for a web-socket that handles incoming messages.
//
// PARAMETERS: num - ID number of the web socket that triggered the event.
//
//             type - The type of event that has occurred.
//
//             payload - The message or data associated with the event.
//
//             payload_length - The length of the event's message or data.
//
// RETURNS:    void
//
// NOTES:      -  The timer that invoked the function is automatically reset to
//                fire again as long as it is "armed".
//
// HISTORY:
// --------- --- - -------------------------------------------------------------
// 25Oct2018 DSV - Initial development.
//
// -----------------------------------------------------------------------------

void WebSocketEvent (
   uint8_t   num,
   WStype_t  type,
   uint8_t*  payload,
   size_t    payload_length
)
{
#define PAYLOAD ( ( payload != NULL ) ? payload : (unsigned char*) " " )

   Serial.print ( "Web Socket Event - " );

   switch ( type )
   {
      case WStype_ERROR:
      {
         Serial.printf ( "ERROR [%u]: %s \n", num, PAYLOAD );
         break;
      }

      case WStype_DISCONNECTED:
      {
         Serial.printf ( "DISCONNECTED [%u] \n", num );
         break;
      }

      case WStype_CONNECTED:
      {
         // NOTE: The payload argument carries the URL requested.
         IPAddress ip = WebSocket.remoteIP ( num );
         Serial.printf ( "CONNECTED [%u] from %d.%d.%d.%d  url: %s \n",
                         num,
                         ip[0],
                         ip[1],
                         ip[2],
                         ip[3],
                         PAYLOAD
                       );
         // Take an initial temperature reading so the client isn't left without
         // any response until the next timer event occurs.
         SensorAction ( NULL );

         break;
      }

      case WStype_TEXT:
      {
         Serial.printf ( "TEXT [%u] %s \n", num, PAYLOAD );
         break;
      }

      default:
      {
         // WARNING: Should never happen!
         Serial.printf ( "UNKNOWN [%u] %s \n", num, PAYLOAD );
         break;
      }
   }
}

// -------------------------------------------------------< /WebSocketEvent >---



// -----------------------------------------------------------------------------
// --------------------------------------------------------------< MakeUUID >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Return a UUID value based on the current processor ID.
//
// PARAMETERS: Result - Memory to store the returned UUID into.
//
// RETURNS:    void
//
// NOTES:      -  A universally unique identifier (UUID) is a 128-bit number
//                used to identify information in a computer, that if generated
//                correctly, has a negligible probability of being duplicated.
//
//             -  The value of a UUID is represented in string form as 32
//                hexadecimal digits divided into five hyphen separated groups
//                in the form 8-4-4-4-12, for a total of 36 characters not
//                including any NULL terminator.
//
//                -  In version 1 UUID format, the last six bytes were known as
//                   the "node ID", which is how this routine justifies swapping
//                   out the last six bytes with bytes from the ESP8266 chip ID.
//
//             -  When used as a Uniform Resource Name (URN), a UUID value
//                takes form:  urn:uuid:dab6cd98-dcd3-4709-9566-fc03e77e8b66
//
// HISTORY:
// --------- --- - -------------------------------------------------------------
// 25Oct2018 DSV - Initial development.
//
// -----------------------------------------------------------------------------

void MakeUUID ( char* Result )
{
   uint32_t ChipID = ESP.getChipId();

   Serial.printf ( "Chip ID = %d (0x%08x) \n", ChipID, ChipID );
   //
   // UUID generated online at https://www.uuidgenerator.net/:
   // dab6cd98-dcd3-4709-9566-fc03e77e8b66
   // Replace the last six characters with the ESP8266 chip ID.
   //
   sprintf ( Result, "dab6cd98-dcd3-4709-9566-fc03e7%02x%02x%02x",
             (uint16_t) ((ChipID >> 16) & 0xff),
             (uint16_t) ((ChipID >>  8) & 0xff),
             (uint16_t) ChipID & 0xff
           );
}

// --------------------------------------------------------------< MakeUUID >---



// -----------------------------------------------------------------------------
// -------------------------------------------------------< DeserializeJSON >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Parse a JSON text buffer and populate a data structure for sensor
//             data from the extracted values.
//
// PARAMETERS: Data - The data structure to be populated with extracted values.
//
//             JSONBuffer - The JSON text buffer to parse and extract data from.
//
// RETURNS:    bool == true:  The JSON buffer was successfully parsed.
//                  == false: Parsing of the JSON buffer failed.
//
// NOTES:
//
// HISTORY:
// --------- --- - -------------------------------------------------------------
// 25Oct2018 DSV - Initial development.
//
// -----------------------------------------------------------------------------

bool DeserializeJSON (
   SData_t&    Data,
   char*       JSONBuffer
)
{
   StaticJsonBuffer<SENSORDATA_JSON_SIZE> jsonBuffer;
   JsonObject& root = jsonBuffer.parseObject ( JSONBuffer );

   Data.Type     = root[ "Type" ];
   Data.Name     = root[ "Name" ];
   Data.Units    = root[ "Units" ];
   Data.Value    = root[ "Value" ];
   Data.Time     = root[ "Time" ];
   Data.Interval = root[ "Interval" ];

   return root.success();
}

// ------------------------------------------------------< /DeserializeJSON >---



// -----------------------------------------------------------------------------
// ---------------------------------------------------------< SerializeJSON >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Fill a buffer with the JSON representation of a data structure
//             holding sensor data.
//
// PARAMETERS: Data - The sensor data structure to be converted into JSON form.
//
//             JSONBuffer - The buffer to store the JSON formatted values into.
//
//             MaxSize - The size of the JSON buffer.
//
//             Pretty -  Should the JSON be formatted for display and easy
//             reading (true), or for compactness (false).
//
// RETURNS:    size_t - The number of bytes of JSON text returned in the buffer.
//
// NOTES:
//
// HISTORY:
// --------- --- - -------------------------------------------------------------
// 25Oct2018 DSV - Initial development.
//
// -----------------------------------------------------------------------------

size_t SerializeJSON (
   const SData_t& Data,
   char*          JSONBuffer,
   size_t         MaxSize,
   boolean        Pretty
)
{
   size_t   Length = 0;
   StaticJsonBuffer<SENSORDATA_JSON_SIZE> jsonBuffer;
   JsonObject& root = jsonBuffer.createObject();

   root[ "Type" ]     = Data.Type;
   root[ "Name" ]     = Data.Name;
   root[ "Units" ]    = Data.Units;
   root[ "Value" ]    = Data.Value;
   root[ "Time" ]     = Data.Time;
   root[ "Interval" ] = Data.Interval;

   if ( Pretty == true )
   {
      root.prettyPrintTo ( JSONBuffer, MaxSize );
      Length = root.measurePrettyLength();
   }

   else
   {
      root.printTo ( JSONBuffer, MaxSize );
      Length = root.measureLength();
   }

   return Length;
}

// --------------------------------------------------------< /SerializeJSON >---


// -----------------------------------------------------------------------------
// ---------------------------------------------------------------< Deblank >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Eliminate blanks from a character array, dropping leading and 
//             trailing blanks, and replacing inner blanks with another char.
//
// PARAMETERS: Value - The input character array to remove blanks from.
//
//             ValueLength - The length of the input/original character array.
//
//             NewValue - The output character array with blanks removed.
//
//             NewValueLength - The length of the output/deblanked array.

// RETURNS:    void
//
// NOTES:      -  WARNING: The NewValueLength argument value should not be zero
//                when the routine is called.  When called the value should be
//                maximum result size (size of the NewValue buffer).  Upon 
//                return is will be the length of the NewValue string.
//
// HISTORY:
// --------- ----------- - -----------------------------------------------------
// 29Dec2019 Scott Vance - Initial development.
//
// -----------------------------------------------------------------------------

void Deblank ( 
   char*    Value, 
   uint8_t  ValueLength, 
   char*    NewValue, 
   uint8_t* NewValueLength,
   char     BlankReplacement
)

{
   if (  Value != NULL  
      && ValueLength > 0 
      && NewValue != NULL 
      && NewValueLength != NULL
      && *NewValueLength >= ( ValueLength + 1 )
      )
   {
      int i;

      *NewValueLength = 0;

      //
      // Don't continue if the original string is all blanks.
      //
      for ( i = 0; i < ValueLength; i++ )
      {
         if ( Value[ i ] != ' ' )
         {
            break;
         }
      }

      if ( i != ValueLength )
      {
         char* ValuePtr = Value;

         *NewValueLength = ValueLength;

         //
         // Trim all of the leading blanks from the string
         //
         for ( i = 0; i < ValueLength; i++ )
         {
            if ( Value[ i ] == ' ' )
            {
               ValuePtr++;
               *NewValueLength -= 1;
            }
   
            else
            {
               memcpy ( NewValue, ValuePtr, min ( *NewValueLength, ValueLength ) ); 
               break;
            }
         }

         //
         // Trim all of the trailing blanks from the string
         //
         for ( i = *NewValueLength - 1; i > 0; i-- )
         {
            if ( NewValue[ i ] == ' ' )
            {
               *NewValueLength -= 1;
            }

            else
            {
               break;
            }
         }


         //
         // Replace all remaining blanks with the specified character.
         //
         for ( i = 0; i < *NewValueLength; i++ )
         {
            if ( NewValue[ i ] == ' ' )
            {
               NewValue[ i ] = BlankReplacement;
            }
         }        

         NewValue[ *NewValueLength ] = 0;
      }
   }
}

// --------------------------------------------------------------< /Deblank >---
