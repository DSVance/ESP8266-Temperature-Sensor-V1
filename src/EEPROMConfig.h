#ifndef EEPROM_CONFIG_CONTROL
#define EEPROM_CONFIG_CONTROL

// -----------------------------------------------------------------------------
// --------------------------------------------------------< EEPROMConfig.h >---
// -----------------------------------------------------------------------------
//
// PROJECT: Arduino development for the ESP8266 custom temperature sensor board.
//
// PURPOSE: Prototypes for routines to store and return program configuration
//          values from the EEPROM emulation on the ESP8266.
//
// AUTHOR:  Scott Vance
//
// NOTES:
//
// HISTORY:
// --------- ----------- - ----------------------------------------------------
// 19Nov2018 Scott Vance - Initial development.
//
// -----------------------------------------------------------------------------


#include <HardwareSerial.h>
#include <EEPROM.h>              // Read a write to permanent FLASH storage
#include "Sensor.h"              // Definitions common to whole Sensor sketch



// -----------------------------------------------------------------------------
// ---------------------------------------------------< PROGRAM_CONFIG_DATA >---
// -----------------------------------------------------------------------------
//
// PURPOSE: Structure of software configuration values stored in the EEPROM.
//
// FIELDS:  Size - The size in bytes of the stored structure.
//
//          Version - The version number of the stored structure.
//
//          Flags - A bit field to hold configuration boolean values.
//
//          WifiSSID - The SSID of the wifi network to connect to.
//
//          WifiPassword - The password for the wifi network to connect to.
//
//          StationIP - The IP address of the ESP8266 in STATION mode.
//
//          AccessIP - The IP address of the ESP8266 in ACCESS_POINT mode.
//
//          NetMask - Mask that defines which subnet the ESP8266 is on.
//
//          Gateway - The IP address of device providing internet connection.
//
//          SerialBaud - The baud rate when sending data to the serial port.
//
//             Valid values:   9600   14400   19200   28800  38400
//                            57600  115200  230400  460800
//
//          WebServerPort - The port the web server is running on.
//
//          WebSocketServerPort - The port the web socket server if running on.
//
//          SensorWaitTime - How many MILLISECONDS elapse between sensor reads.
//
//          TempHighLimit - Relay is turned OFF when temperature is HIGHER.
//
//          TempLowLimit - Relay is turned OFF when temperature is LOWER.
//
//          Spare - Room to expand without changing total stored size.
//
//          WifiSSIDLength - Length in bytes of the stored SSID string.
//
//          WifiPasswordLength - Length in bytes of the stored wifi password.
//
//          Label - User assigned descriptive label for the sensor.
//
//          Label - Length in bytes of the stored user assigned label.
//
// NOTES:   -  WARNING: Since EEPROM values are written and read as a sequence
//             of single bytes (without data types), care must be taken to
//             ensure that the values are properly aligned when read into memory
//             (RAM) to avoid memory-bus errors when accessing the values as
//             specific data types.  Structures must start on an 8 byte memory
//             boundary, and each inherent data type must be aligned on a memory
//             boundary that matches its size:
//
//                struct types -> offset modulo 8 == 0
//                8-byte types -> offset modulo 8 == 0
//                4-byte types -> offset modulo 4 == 0
//                2-byte types -> offset modulo 2 == 0
//                1-byte types -> offset modulo 1 == 0  (no alignment required)
//
//             Note that an 8-byte boundary is also a valid 4, 2, and 1 byte
//             boundary, and a 4-byte boundary is also a valid 2 and 1 byte
//             boundary, but the reverse is not true.  A valid 4-byte boundary
//             may or may not fall on an 8-byte boundary.
//
//          -  The simplest way to honor memory alignment requirements is to
//             store the values in already-aligned form so no adjustments need
//             to be made as the values are read into working memory.
//
// HISTORY:
// --------- ----------- - -----------------------------------------------------
// 19Nov2018 Scott Vance - Initial development.
//
// -----------------------------------------------------------------------------

#define PCONFIG_MAX_SSID      31
#define PCONFIG_MAX_PASSWORD  31
#define PCONFIG_MAX_LABEL     31

typedef struct PROGRAM_CONFIG_DATA
{                                                        // Offset
   uint16_t Size;                                        //     0
   uint16_t Version;                                     //     2
   uint32_t Flags;                                       //     4

   char     WifiSSID[ PCONFIG_MAX_SSID + 1 ];            //     8
   char     WifiPassword[ PCONFIG_MAX_PASSWORD + 1 ];    //    40
   uint8_t  StationIP[ 4 ];                              //    72
   uint8_t  AccessIP[ 4 ];                               //    76
   uint8_t  NetMask[ 4 ];                                //    80
   uint8_t  Gateway[ 4 ];                                //    84

   uint32_t SerialBaud;                                  //    88

   uint16_t WebServerPort;                               //    92
   uint16_t WebSocketServerPort;                         //    94

   uint32_t SensorWaitTime;                              //    96
   int16_t  TempHighLimit;                               //   100
   int16_t  TempLowLimit;                                //   102

   uint8_t  WifiSSIDLength;                              //   104
   uint8_t  WifiPasswordLength;                          //   105

   char     Label[ PCONFIG_MAX_LABEL + 1 ];              //   106
   uint8_t  LabelLength;                                 //   138

   uint8_t  Spare[ 37 ];                                 //   139

}  PConfig_t;

//
// Bit definitions for the configuration 'Flags' field.
//
#define  CONFIG_VALUES_INITIALIZED        0x00000001     // Have EEPROM values been initialized
#define  CONFIG_TEMP_PROBE_CONNECTED      0x00000002     // Is there a temperature probe connected
#define  CONFIG_DEVICE_RELAY_CONNECTED    0x00000004     // Is there a device relay in the circuit
#define  CONFIG_DEBUG_MESSAGE_ENABLED     0x00000008     // Are debug messages enabled
#define  CONFIG_TEMP_DISPLAY_FAHRENHEIT   0X00000010     // Is the temperature display in fahrenheit
#define  CONFIG_WIFI_STATION_ENABLED      0X00000020     // Is a wifi station connection *desired*



//
// 1st byte == major version, 2nd byte == minor version
//
#define  PCONFIG_VERSION   0x0101

//
// This doesn't have to be zero but there is no reason for it not to be.
// WARNING: The offset is ASSUMED to be on an 8-byte boundary!
//
#define  PCONFIG_OFFSET    0

//
// WARNING: These definitions are NOT absolute offsets into the EEPROM, but are
// rather RELATIVE to the overall structure offset as defined by PCONFIG_OFFSET.
//
#define  PCONFIG_OFFSET_SIZE                    ( PCONFIG_OFFSET +   0 )
#define  PCONFIG_OFFSET_VERSION                 ( PCONFIG_OFFSET +   2 )
#define  PCONFIG_OFFSET_FLAGS                   ( PCONFIG_OFFSET +   4 )
#define  PCONFIG_OFFSET_WIFISSID                ( PCONFIG_OFFSET +   8 )
#define  PCONFIG_OFFSET_WIFIPASSWORD            ( PCONFIG_OFFSET +  40 )
#define  PCONFIG_OFFSET_STATIONIP               ( PCONFIG_OFFSET +  72 )
#define  PCONFIG_OFFSET_ACCESSIP                ( PCONFIG_OFFSET +  76 )
#define  PCONFIG_OFFSET_NETMASK                 ( PCONFIG_OFFSET +  80 )
#define  PCONFIG_OFFSET_GATEWAY                 ( PCONFIG_OFFSET +  84 )
#define  PCONFIG_OFFSET_SERIALBAUD              ( PCONFIG_OFFSET +  88 )
#define  PCONFIG_OFFSET_WEBSERVERPORT           ( PCONFIG_OFFSET +  92 )
#define  PCONFIG_OFFSET_WEBSOCKETSERVERPORT     ( PCONFIG_OFFSET +  94 )
#define  PCONFIG_OFFSET_SENSORWAITTIME          ( PCONFIG_OFFSET +  96 )
#define  PCONFIG_OFFSET_TEMPHIGHLIMIT           ( PCONFIG_OFFSET + 100 )
#define  PCONFIG_OFFSET_TEMPLOWLIMIT            ( PCONFIG_OFFSET + 102 )
#define  PCONFIG_OFFSET_WIFISSIDLENGTH          ( PCONFIG_OFFSET + 104 )
#define  PCONFIG_OFFSET_WIFIPASSWORDLENGTH      ( PCONFIG_OFFSET + 105 )
#define  PCONFIG_OFFSET_LABEL                   ( PCONFIG_OFFSET + 106 )
#define  PCONFIG_OFFSET_LABELLENGTH             ( PCONFIG_OFFSET + 138 )

// --------------------------------------------------< /PROGRAM_CONFIG_DATA >---



void ClearROM ( 
   uint32_t Size,
   uint32_t Offset = 0   
);

void SetROMDefaults ( PConfig_t* ConfigData );

void ShowROMValues (
   PConfig_t*  ConfigData,
   const char* Label
);

void SetROMValue (
   int         Offset,
   uint8_t*    Value,
   int         Size
);



#endif   // EEPROM_CONFIG_CONTROL
