// -----------------------------------------------------------------------------
// ------------------------------------------------------< EEPROMConfig.cpp >---
// -----------------------------------------------------------------------------
//
// PROJECT: Arduino development for the ESP8266 custom temperature sensor board.
//
// PURPOSE: Routines to store and return program configuration values from the 
//          EEPROM emulation on the ESP8266.
//
// AUTHOR:  Scott Vance
//
// NOTES:   -  ESP8266 is little-endian so the bytes in the hex values for the 
//             IP addresses are written in reverse order.
//
// HISTORY:
// --------- ----------- - ----------------------------------------------------
// 19Nov2018 Scott Vance - Initial development.
//
// -----------------------------------------------------------------------------



#include <stddef.h>           // For offsetof function
#include "EEPROMConfig.h"



// -----------------------------------------------------------------------------
// --------------------------------------------------------------< ClearROM >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Set a specified number of bytes in the ROM storage area to zero.
//
// PARAMETERS: Size - The number of bytes to be set to zero.
//
// RETURNS:    void
//
// NOTES:      -  WARNING: By default, the offset value is set to zero, so the 
//                zeroing of bytes starts at the beginning of the ROM storage 
//                area.  The caller must provide a non-zero starting offset 
//                argument to start zeroing at some other location.
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------

void ClearROM ( 
   uint32_t Size, 
   uint32_t Offset
)
{
   for ( int i = Offset; i < Size; i++ ) 
   {
      EEPROM.write ( i, 0 );
   }
   EEPROM.commit();
}

// -------------------------------------------------------------< /ClearROM >---



// -----------------------------------------------------------------------------
// --------------------------------------------------------< SetROMDefaults >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Set default values in the ROM storage area.
//
// PARAMETERS: ConfigDatah - Pointer to the configuration data structure.
//
// RETURNS:    void
//
// NOTES:      -  A new processor presents a chicken-and-egg problem:  It needs
//                to be configured, but it can't start the services necessary to
//                get the configuration values without some key configuration 
//                values.  This routine provides a simple way to boot-strap a 
//                new processor and put some default values into the ROM area.
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------

void SetROMDefaults ( PConfig_t* ConfigDatah )
{
   if ( ConfigDatah != NULL )
   {
      ConfigDatah->Size                 = sizeof ( PConfig_t );
      ConfigDatah->Version              = PCONFIG_VERSION;

      ConfigDatah->Flags                = ( CONFIG_VALUES_INITIALIZED
                                         | CONFIG_TEMP_PROBE_CONNECTED
                                         | CONFIG_DEVICE_RELAY_CONNECTED
                                         | CONFIG_TEMP_DISPLAY_FAHRENHEIT
                                         | CONFIG_WIFI_STATION_ENABLED
                                         );

      ConfigDatah->WifiSSIDLength       = 0;
      memset ( ConfigDatah->WifiSSID, 0, sizeof ( ConfigDatah->WifiSSID ) );
      
      ConfigDatah->WifiPasswordLength   = 0;
      memset ( ConfigDatah->WifiPassword, 0, sizeof ( ConfigDatah->WifiPassword ) );
      
      ConfigDatah->StationIP[ 0 ]       = 0;
      ConfigDatah->StationIP[ 1 ]       = 0;
      ConfigDatah->StationIP[ 2 ]       = 0;
      ConfigDatah->StationIP[ 3 ]       = 0;

      ConfigDatah->AccessIP[ 0 ]        = 192;
      ConfigDatah->AccessIP[ 1 ]        = 168;
      ConfigDatah->AccessIP[ 2 ]        = 0;
      ConfigDatah->AccessIP[ 3 ]        = 10;
      
      ConfigDatah->NetMask [ 0 ]        = 255;
      ConfigDatah->NetMask [ 1 ]        = 255;
      ConfigDatah->NetMask [ 2 ]        = 255;
      ConfigDatah->NetMask [ 3 ]        = 0;
      
      ConfigDatah->Gateway[ 0 ]         = 192;
      ConfigDatah->Gateway[ 1 ]         = 168;
      ConfigDatah->Gateway[ 2 ]         = 0;
      ConfigDatah->Gateway[ 3 ]         = 62;

      ConfigDatah->SerialBaud           = DEFAULT_BAUD;

      ConfigDatah->WebServerPort        = 80;
      ConfigDatah->WebSocketServerPort  = 81;

      ConfigDatah->SensorWaitTime       = 15 * 1000;
      ConfigDatah->TempHighLimit        = 85;
      ConfigDatah->TempLowLimit         = 35;

      ConfigDatah->LabelLength          = 0;
      memset ( ConfigDatah->Label, 0, sizeof ( ConfigDatah->Label ) );

      EEPROM.put ( PCONFIG_OFFSET, *ConfigDatah );
      EEPROM.commit();
   }
}

// -------------------------------------------------------< /SetROMDefaults >---



// -----------------------------------------------------------------------------
// ---------------------------------------------------------< ShowROMValues >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Show the data values stored in ROM on the serial port.
//
// PARAMETERS: ConfigDatah - Pointer to the configuration data structure.
//
//             Label - Text string providing context about the routine's caller.
//
// RETURNS:    void
//
// NOTES:
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------

void ShowROMValues ( 
   PConfig_t*  ConfigDatah, 
   const char* Label 
)
{
   if ( ConfigDatah != NULL )
   {
      if ( Label != NULL )
      {
         Serial.printf ( "%s \n", Label );
      }
      Serial.printf ( "   Size ................. %u \n", ConfigDatah->Size );
      Serial.printf ( "   Version .............. 0x%04x \n", ConfigDatah->Version );
      Serial.printf ( "   Flags ................ 0x%04x \n", ConfigDatah->Flags );

      Serial.printf ( "      Initialized ....... %s \n", ( ConfigDatah->Flags & CONFIG_VALUES_INITIALIZED )      ? "TRUE" : "FALSE" );
      Serial.printf ( "      Probe connected ... %s \n", ( ConfigDatah->Flags & CONFIG_TEMP_PROBE_CONNECTED )    ? "TRUE" : "FALSE" );
      Serial.printf ( "      Relay connected ... %s \n", ( ConfigDatah->Flags & CONFIG_DEVICE_RELAY_CONNECTED )  ? "TRUE" : "FALSE" );
      Serial.printf ( "      Debug messages .... %s \n", ( ConfigDatah->Flags & CONFIG_DEBUG_MESSAGE_ENABLED )   ? "TRUE" : "FALSE" );
      Serial.printf ( "      Fahrenheit ........ %s \n", ( ConfigDatah->Flags & CONFIG_TEMP_DISPLAY_FAHRENHEIT ) ? "TRUE" : "FALSE" );
      Serial.printf ( "      Wifi enabled ...... %s \n", ( ConfigDatah->Flags & CONFIG_WIFI_STATION_ENABLED )    ? "TRUE" : "FALSE" );

      Serial.printf ( "   WifiSSIDLength ....... %u \n", ConfigDatah->WifiSSIDLength );
      Serial.printf ( "   WifiSSID ............. %.*s \n", ConfigDatah->WifiSSIDLength, ConfigDatah->WifiSSID );
     
      Serial.printf ( "   WifiPasswordLength ... %u \n", ConfigDatah->WifiPasswordLength );
      Serial.printf ( "   WifiPassword ......... %.*s \n", ConfigDatah->WifiPasswordLength, ConfigDatah->WifiPassword );

      Serial.printf ( "   StationIP ............ %u.%u.%u.%u \n", 
                      ConfigDatah->StationIP[ 0 ],
                      ConfigDatah->StationIP[ 1 ],
                      ConfigDatah->StationIP[ 2 ],
                      ConfigDatah->StationIP[ 3 ]
                    );

      Serial.printf ( "   AccessIP ............. %u.%u.%u.%u \n", 
                      ConfigDatah->AccessIP[ 0 ],
                      ConfigDatah->AccessIP[ 1 ],
                      ConfigDatah->AccessIP[ 2 ],
                      ConfigDatah->AccessIP[ 3 ]
                    );
                    
      Serial.printf ( "   NetMask .............. %u.%u.%u.%u \n", 
                      ConfigDatah->NetMask[ 0 ],
                      ConfigDatah->NetMask[ 1 ],
                      ConfigDatah->NetMask[ 2 ],
                      ConfigDatah->NetMask[ 3 ]
                    );

      Serial.printf ( "   Gateway .............. %u.%u.%u.%u \n", 
                      ConfigDatah->Gateway[ 0 ],
                      ConfigDatah->Gateway[ 1 ],
                      ConfigDatah->Gateway[ 2 ],
                      ConfigDatah->Gateway[ 3 ]
                    );

      Serial.printf ( "   SerialBaud ........... %u \n", ConfigDatah->SerialBaud );

      Serial.printf ( "   WebServerPort ........ %u \n", ConfigDatah->WebServerPort );
      Serial.printf ( "   WebSocketServerPort .. %u \n", ConfigDatah->WebSocketServerPort );

      Serial.printf ( "   SensorWaitTime ....... %u \n", ConfigDatah->SensorWaitTime );
      Serial.printf ( "   TempHighLimit ........ %d \n", ConfigDatah->TempHighLimit );
      Serial.printf ( "   TempLowLimit ......... %d \n", ConfigDatah->TempLowLimit );

      Serial.printf ( "   LabelLength .......... %u \n", ConfigDatah->LabelLength );
      Serial.printf ( "   Label ................ %.*s \n", ConfigDatah->LabelLength, ConfigDatah->Label );
   }
}

// --------------------------------------------------------< /ShowROMValues >---



// -----------------------------------------------------------------------------
// -----------------------------------------------------------< SetROMValue >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Store a data item in the ROM storage area of memory.
//
// PARAMETERS: Offset - The offset into the ROM area to store the data item.
//
//             Value - A byte array that contains the data item to store.
//
//             Size - The number of bytes the data item occupies.
//
// RETURNS:    void
//
// NOTES:
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------

void SetROMValue ( 
   int         Offset,
   uint8_t*    Value,
   int         Size
)
{
   if (  Offset >= PCONFIG_OFFSET 
      && Value != NULL 
      && Size >= 0 
      )
   {
      Serial.printf ( "SetROMValue: Offset = %d  Size = %d  Value = ", Offset, Size  );

      for ( int i = 0; i < Size; i++, Value++ )
      {
         EEPROM.write ( Offset + i, *Value );
         Serial.printf ( "%02x ", *Value );
      }
      EEPROM.commit();
      Serial.printf ( "\n" );
   }
}

// ----------------------------------------------------------< /SetROMValue >---
