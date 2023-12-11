
// -----------------------------------------------------------------------------
// ---------------------------------------------------< WifiConnect.cpp >-------
// -----------------------------------------------------------------------------
//
// PROJECT: Arduino development for the ESP8266 micro controller board.
//
// PURPOSE: Routines to return the SSID of the local wifi network and the 
//          connection password for that network.
//
// AUTHOR:  Scott Vance
//
// NOTES:
//
// HISTORY:
// --------- ----------- - ----------------------------------------------------
// 23Aug2018 Scott Vance - Initial development.
//
// -----------------------------------------------------------------------------


static const char* OurSSID = "Wifi_SSID_Here";
static const char* OurPW   = "Wifi_Password_Here"; 
static const char* OurHost = "Wifi_Host_Here"; 


const char* GetWifiSSID()
{
   return OurSSID;
}


const char* GetWifiPW()
{
   return OurPW;
}


const char* GetWifiWebHost()
{
   return OurHost;
}
