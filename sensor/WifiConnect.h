
#ifndef WIFI_CONNECT

// -----------------------------------------------------------------------------
// -----------------------------------------------------< WifiConnect.h >-------
// -----------------------------------------------------------------------------
//
// PROJECT: Arduino development for the ESP8266 micro controller board.
//
// PURPOSE: Prototypes for routines to return the SSID of the local wifi network
//          and the connection password for that network.
//
// AUTHOR:  Scott Vance
//
// NOTES:   -  Providing the network credentials this way does two things:
//
//                1) It adds a little security since the password is not visible 
//                   in open code, and access to these source files can be 
//                   locked down.
//                   
//                2) It provides a single place to change should the SSID or the 
//                   password for the local wifi network change.
//
//          -  To use, include this header and invoke the desired function to 
//             populate a local variable of type const char* like this:
//
//                const char* ssid      = GetWifiSSID();
//                const char* password  = GetWifiPW();
//                const char* host      = GetWifiWebHost();
//
// HISTORY:
// --------- ----------- - ----------------------------------------------------
// 23Aug2018 Scott Vance - Initial development.
//
// -----------------------------------------------------------------------------


#define WIFI_CONNECT


char* GetWifiSSID();
char* GetWifiPW();
char* GetWifiWebHost();


#endif   // WIFI_CONNECT
