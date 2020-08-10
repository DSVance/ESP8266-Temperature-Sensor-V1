#ifndef WEB_CONFIG
#define WEB_CONFIG

// -----------------------------------------------------------------------------
// -----------------------------------------------------------< WebConfig.h >---
// -----------------------------------------------------------------------------
//
// PROJECT: Arduino development for the ESP8266 custom temperature sensor board.
//
// PURPOSE: Prototypes and definitions for routines to handle web server events.
//
// AUTHOR:  Scott Vance
//
// NOTES:
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------



#include <ESP8266WebServer.h>    // Simple web server
#include <ESP8266SSDP.h>         // Simple service discovery
#include "Sensor.h"              // Definitions common to whole Sensor sketch
#include "EEPROMConfig.h"        // Read a write configuration to FLASH storage



void WebEvents (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah,
   SSDPClass*        SSDPh
);

int LoadFile (
   fs::FS&     FileSys,
   const char* FilePath,
   char*       FileContent
);



// -----------------------------------------------------------------------------
// --------------------------------------------------------------< BaudList >---
// -----------------------------------------------------------------------------
//
// PURPOSE: A static array of permitted serial baud rate values.
//
// NOTES:   -  Notice that this is a static array.
//
// -----------------------------------------------------------------------------

#define BAUD_LIST_SIZE  10

static int BaudList[ BAUD_LIST_SIZE ] = 
{   
     100,  9600,  14400,  19200,  28800,
   38400, 57600, 115200, 230400, 460800 
};

// -------------------------------------------------------------< /BaudList >---



#endif   // WEB_CONFIG
