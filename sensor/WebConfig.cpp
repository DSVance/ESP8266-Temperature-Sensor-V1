// -----------------------------------------------------------------------------
// ---------------------------------------------------------< WebConfig.cpp >---
// -----------------------------------------------------------------------------
//
// PROJECT: Arduino development for the ESP8266 custom temperature sensor board.
//
// PURPOSE: Routines to handle web server events.
//
// AUTHOR:  Scott Vance
//
// NOTES:
//
// HISTORY:
// --------- ----------- - -----------------------------------------------------
// 30Nov2018 Scott Vance - Initial development.
//
// -----------------------------------------------------------------------------



#include "WebConfig.h"
#include <FS.h>                  // SPIFFS file system
#include <assert.h>              // Assert statements
#include <Schedule.h>            // Scheduled function ability


// POPPYCOCK DSV: Can probably move this into HandleFileUpload()
// and just declare it 'static'.
File UploadFileHandle;


os_timer_t  RestartTimer;


static bool HandleFileRequest (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah,
   String            FilePath
);

static void HandleFileUpload (
   ESP8266WebServer* WebServerh
);

static void HandleSensorConfigGet (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah,
   const char*       FilePath
);

static void HandleSensorConfigPost (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
);

static void HandleWifiConfigGet (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah,
   const char*       FilePath
);

static void HandleWifiConfigPost (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
);

static void HandleSensorDataJS (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah,
   const char*       FilePath
);

static void RestartSystem ( void* Args );

static void HandleRestart (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah,
   const char*       FilePath
);

static boolean ConfigProbe (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
);

static boolean ConfigRelay (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
);

static void ConfigLowTemp (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
);

static void ConfigHighTemp (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
);

static boolean ConfigSensorLabel (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
);

static void ConfigWaitInterval (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
);

static boolean ConfigTempUnits (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
);

static boolean ConfigDebug (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
);

static void ConfigWifiEnable (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
);

static void ConfigSSID (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
);

static void ConfigPassword (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
);

static void ConfigAccessIP (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
);

static void ConfigAPNetmask (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
);

static void ConfigAPGateway (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
);

static void ConfigBaud (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
);

static void ConfigWebPort (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
);

static void ConfigDataPort (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
);

static void ReplaceSensorName (
   PConfig_t*  ConfigDatah,
   String&     HTMLFile
);

static void GetWifiNetworks ( 
   PConfig_t*  ConfigDatah,
   String&     HTMLFile
);

static void GetContentType (
   String   Extension,
   String&  Type
);

static void GetWebMethodText (
   ESP8266WebServer* WebServerh,
   String&           MethodText
);

static void Send404 (
   ESP8266WebServer* WebServerh
);

int GetFileSize (
   fs::FS&     FileSys,
   const char* FilePath
);

int LoadFile (
   fs::FS&     FileSys,
   const char* FilePath,
   String&     FileContent
);



// -----------------------------------------------------------------------------
// -------------------------------------------------------------< WebEvents >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Define handler functions for web server events.
//
// PARAMETERS: WebServerh - Handle to the web server.
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

void WebEvents (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah,
   SSDPClass*        SSDPh
)
{
   // Most page requests are handled generically below, but handle a
   // GET request for the "upload" page individually so that the server
   // can respond differently when it is a POST request instead.
   WebServerh->on ( "/UploadFile.html", HTTP_GET, [ WebServerh, ConfigDatah ]()
   {
      // Return the page if it exists, else return an error.
      HandleFileRequest ( WebServerh, ConfigDatah, "/UploadFile.html" );
   });


   // Most page requests are handled generically below, but handle a
   // POST request for the "upload" page individually so that the server
   // can respond differently when it is a GET request instead.
   WebServerh->on
   (
      "/UploadFile.html", HTTP_POST, [ WebServerh ]()
   {
      // A web client POSTED to the file-upload page so send status
      // 200 (OK) to tell the client the server is ready to receive
      // the data and and save the file to SPIFFS.

      // 200 - OK
      WebServerh->send ( 200 );
   },
   [ WebServerh ]()
   {
      HandleFileUpload ( WebServerh );
   }
   );

   WebServerh->on ( "/description.xml", HTTP_GET, [ WebServerh, SSDPh ]()
   {
      SSDPh->schema ( WebServerh->client() );
   });

   WebServerh->on ( "/SensorConfig.html", HTTP_GET, [ WebServerh, ConfigDatah ]()
   {
      HandleSensorConfigGet ( WebServerh, ConfigDatah, "/SensorConfig.html" );
   });

   WebServerh->on ( "/SensorConfig.html", HTTP_POST, [ WebServerh, ConfigDatah ]()
   {
      HandleSensorConfigPost ( WebServerh, ConfigDatah );
   });

   WebServerh->on ( "/WifiConfig.html", HTTP_GET, [ WebServerh, ConfigDatah ]()
   {
      HandleWifiConfigGet ( WebServerh, ConfigDatah, "/WifiConfig.html" );
   });

   WebServerh->on ( "/WifiConfig.html", HTTP_POST, [ WebServerh, ConfigDatah ]()
   {
      HandleWifiConfigPost ( WebServerh, ConfigDatah );
   });

   WebServerh->on ( "/TemperatureData.js", [ WebServerh, ConfigDatah ]()
   {
      HandleSensorDataJS ( WebServerh, ConfigDatah, "/TemperatureData.js" );
   });

   WebServerh->on ( "/RESTART", HTTP_POST, [ WebServerh, ConfigDatah ]()
   {
      HandleRestart ( WebServerh, ConfigDatah, "/Restarting.html" );
   });

   WebServerh->onNotFound ( [ WebServerh, ConfigDatah ]()
   {
      // For those pages that do not have a registered handler, they either
      // don't exist, or they don't require special processing and can be
      // returned in generic fashion, as is.  This routine called below
      // returns the requested page, or returns an error.

      HandleFileRequest ( WebServerh, ConfigDatah, WebServerh->uri() );
   });
}

// ------------------------------------------------------------< /WebEvents >---



// -----------------------------------------------------------------------------
// -----------------------------------------------------< HandleFileRequest >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    A web server event handler to return a specified page from SPIFFS
//             storage to the web client.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
//
//             FilePath - The fully qualified file name to be returned.
//
// RETURNS:    bool == 'true' when the file was successfully returned.
//                  == 'false' if the file send failed or file doesn't exist.
//
// NOTES:      -  If the specified file cannot be found or sent back to the
//                client, a 404 error message is automatcally returned.
//
//             -  If the sensor configuration contains a label, this routine
//                performs an automatic text substitution on all uncompressed
//                files (substitution is not done on compressed files):
//                   The first occurance of this text:
//                      <span name="sensor_label"></span>
//                   Is replaced by this text:
//                      <span name="sensor_label">#Label#</span>
//                   Where #Label# is the configuration value:
//                      ConfigDatah->Label
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------

static bool HandleFileRequest (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah,
   String            FilePath
)
{
   bool     SentFileStatus = false;
   bool     Compressed = false;
   String   ContentType;

   // If the specified file exists, return its content.

   if ( FilePath.endsWith ( "/" ) )
   {
      // If the request is for a folder, send the index file.
      FilePath += "index.html";
   }

   // Get the MIME type based on the file's extension.
   GetContentType( FilePath, ContentType );

   if ( SPIFFS.exists ( FilePath + ".gz" ) )
   {
      // Modify the request to use the compressed version.
      FilePath += ".gz";
      Compressed = true;
   }

   if ( SPIFFS.exists ( FilePath ) )
   {
      if ( ConfigDatah->LabelLength > 0 && Compressed == false )
      {
         String   FileContent;
         int      FileSize = 0;

         // Load the web page into memory so it can be modified.
         FileSize = LoadFile ( SPIFFS, FilePath.c_str(), FileContent );

         if ( FileSize > 0 )
         {
            ReplaceSensorName ( ConfigDatah, FileContent );

            // Send the updated web page to the web client.
            WebServerh->sendContent ( FileContent );
            SentFileStatus = true;
         }
      }

      else
      {
         // If the file exists, either as a compressed archive, or normal
         File FileHandle = SPIFFS.open ( FilePath, "r" );
         size_t Sent = WebServerh->streamFile ( FileHandle, ContentType.c_str() );
         FileHandle.close();
         SentFileStatus = true;
      }
   }

   if ( SentFileStatus == true )
   {
      Serial.printf ( "HandleFileRequest - Sent file \"%s\" \n", FilePath.c_str() );
   }

   else
   {
      Serial.printf ( "HandleFileRequest - File Not Found - \"%s\" \n", FilePath.c_str() );
      Send404 ( WebServerh );
   }

   return SentFileStatus;
}

// ----------------------------------------------------< /HandleFileRequest >---



// -----------------------------------------------------------------------------
// ------------------------------------------------------< HandleFileUpload >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    A web server event handler to manage the uploading of a file to
//             to the web server from the web client, and storage in SPIFFS.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
// RETURNS:    void
//
// NOTES:      -  During an upload, this routine is called at least three times.
//                One time each with a status of:
//                   UPLOAD_FILE_START   followed by
//                   UPLOAD_FILE_WRITE   followed by
//                   UPLOAD_FILE_END
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------

static void HandleFileUpload (
   ESP8266WebServer* WebServerh
)
{
   HTTPUpload& upload = WebServerh->upload();

   if ( upload.status == UPLOAD_FILE_START )
   {
      String FileName = upload.filename;
      if ( ! FileName.startsWith ( "/" ) )
      {
         // A file must have a path so prepend the root file system delimiter.
         FileName = "/" + FileName;
      }
      Serial.printf ( "HandleFileUpload - Starting upload of file \"%s\" \n", FileName.c_str() );

      // Open the file for writing in SPIFFS (creating if it doesn't exist).
      UploadFileHandle = SPIFFS.open ( FileName, "w" );
      FileName = String();
   }

   else if ( upload.status == UPLOAD_FILE_WRITE )
   {
      if ( UploadFileHandle )
      {
         // Write the received bytes to the file.
         UploadFileHandle.write ( upload.buf, upload.currentSize );
      }
   }

   else if ( upload.status == UPLOAD_FILE_END )
   {
      if ( UploadFileHandle )
      {
         String FileName = upload.filename;

         UploadFileHandle.close();

         Serial.printf ( "HandleFileUpload - Finished upload of file \"%s\" (%d bytes) \n",
                         FileName.c_str(),
                         upload.totalSize
                       );

         // Redirect the client to the success page
         WebServerh->sendHeader ( "Location", "/UploadSuccess.html" );
         // 303 - See other (redirect).
         WebServerh->send ( 303 );
      }

      else
      {
         // 500 - Internal Server Error.
         WebServerh->send ( 500, "text/plain", "500: Coult not create the file" );
      }
   }

   else if ( upload.status == UPLOAD_FILE_ABORTED )
   {
      // 500 - Internal Server Error.
      WebServerh->send ( 500, "text/plain", "500: File upload was aborted" );
   }
}

// -----------------------------------------------------< /HandleFileUpload >---



// -----------------------------------------------------------------------------
// -------------------------------------------------< HandleSensorConfigGet >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    A web server event handler to populate a sensor configuration web
//             form with the current stored sensor settings.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
//
//             FilePath - The .html file that holds the wifi configuration page.
//
// RETURNS:    void
//
// NOTES:      -  The configuraiton web page has built-in place holders for
//                the current values.  This routine replaces those place holders
//                with the actual stored values before the page is returned to
//                the client.
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------

static void HandleSensorConfigGet (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah,
   const char*       FilePath
)
{
   String   FileContent;
   int      FileSize = 0;

   // Load the web page into memory so it can be modified.
   FileSize = LoadFile ( SPIFFS, FilePath, FileContent );

   if ( FileSize > 0 )
   {
      if ( ConfigDatah->LabelLength > 0 )
      {
         ReplaceSensorName ( ConfigDatah, FileContent );
      }

      // Replace the sensor temp probe radio place holder.
      if ( ConfigDatah->Flags & CONFIG_TEMP_PROBE_CONNECTED )
      {
         FileContent.replace ( "\"set_probe_Y\"", "\"Y\" checked" );
         FileContent.replace ( "\"set_probe_N\"", "\"N\"" );
      }

      else
      {
         FileContent.replace ( "\"set_probe_Y\"", "\"Y\"" );
         FileContent.replace ( "\"set_probe_N\"", "\"N\" checked" );
      }

      // Replace the sensor relay radio place holder.
      if ( ConfigDatah->Flags & CONFIG_DEVICE_RELAY_CONNECTED )
      {
         FileContent.replace ( "\"set_relay_Y\"", "\"Y\" checked" );
         FileContent.replace ( "\"set_relay_N\"", "\"N\"" );
      }

      else
      {
         FileContent.replace ( "\"set_relay_Y\"", "\"Y\"" );
         FileContent.replace ( "\"set_relay_N\"", "\"N\" checked" );
      }

      FileContent.replace ( "set_lowtemp",  String ( ConfigDatah->TempLowLimit ) );
      FileContent.replace ( "set_hightemp", String ( ConfigDatah->TempHighLimit ) );
      FileContent.replace ( "set_label",    ConfigDatah->Label );
      FileContent.replace ( "set_interval", String ( ConfigDatah->SensorWaitTime / 1000 ) );

      // Replace the fahrenheit radio place holder.
      if ( ConfigDatah->Flags & CONFIG_TEMP_DISPLAY_FAHRENHEIT )
      {
         FileContent.replace ( "\"set_units_F\"", "\"F\" checked" );
         FileContent.replace ( "\"set_units_C\"", "\"C\"" );
      }

      else
      {
         FileContent.replace ( "\"set_units_F\"", "\"F\"" );
         FileContent.replace ( "\"set_units_C\"", "\"C\" checked" );
      }

      // Replace the debug message radio place holder.
      if ( ConfigDatah->Flags & CONFIG_DEBUG_MESSAGE_ENABLED )
      {
         FileContent.replace ( "\"set_debug_Y\"", "\"Y\" checked" );
         FileContent.replace ( "\"set_debug_N\"", "\"N\"" );
      }

      else
      {
         FileContent.replace ( "\"set_debug_Y\"", "\"Y\"" );
         FileContent.replace ( "\"set_debug_N\"", "\"N\" checked" );
      }


      // Send the updated web page to the web client.
      WebServerh->sendContent ( FileContent );

      Serial.printf ( "HandleSensorConfigGet - Sent file \"%s\" \n", FilePath );
   }

   else
   {
      Serial.printf ( "HandleSensorConfigGet - File Not Found - \"%s\" \n", FilePath );

      // File Not Found
      Send404 ( WebServerh );
   }
}

// ------------------------------------------------< /HandleSensorConfigGet >---



// -----------------------------------------------------------------------------
// ------------------------------------------------< HandleSensorConfigPost >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    A web server event handler to manage retrieval of sensor settings
//             from a sensor configuration web page.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
//
// RETURNS:    void
//
// NOTES:      -  WARNING: The serial log message and the returned HTML page
//                both say that the new settings will take effect after a system
//                restart, and that is true for all changes to take effect, but
//                some new settings will take effect immediately because the
//                values in the configuration structure have already been
//                updated, and much of the system operates by directly examining
//                those values.
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------

static void HandleSensorConfigPost (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
)
{
   String Message;
   boolean BitsChanged;
   boolean FlagsChanged = false;


   if ( ConfigDatah->Flags & CONFIG_DEBUG_MESSAGE_ENABLED )
   {
      Serial.printf ( "DEBUG HandleSensorConfigPost: Arg count is %d \n", WebServerh->args() );

      for ( int i = 0; i < WebServerh->args(); i++ )
      {
         Serial.println ( "   " + WebServerh->argName ( i ) + ": " + WebServerh->arg ( i ) );
      }
   }

   if (  WebServerh->hasArg ( "sensor_probe" )
         || ConfigDatah->Flags & CONFIG_TEMP_PROBE_CONNECTED
      )
   {
      BitsChanged = ConfigProbe ( WebServerh, ConfigDatah );
      FlagsChanged = FlagsChanged || BitsChanged;
   }

   if (  WebServerh->hasArg ( "sensor_relay" )
         || ConfigDatah->Flags & CONFIG_DEVICE_RELAY_CONNECTED
      )
   {
      BitsChanged = ConfigRelay ( WebServerh, ConfigDatah );
      FlagsChanged = FlagsChanged || BitsChanged;
   }


   if ( WebServerh->hasArg ( "sensor_lowtemp" ) )
   {
      ConfigLowTemp ( WebServerh, ConfigDatah );
   }


   if ( WebServerh->hasArg ( "sensor_hightemp" ) )
   {
      ConfigHighTemp ( WebServerh, ConfigDatah );
   }


   if ( WebServerh->hasArg ( "sensor_label" ) )
   {
      ConfigSensorLabel ( WebServerh, ConfigDatah );
   }


   if ( WebServerh->hasArg ( "sensor_interval" ) )
   {
      ConfigWaitInterval ( WebServerh, ConfigDatah );
   }


   if (  WebServerh->hasArg ( "sensor_units" )
         || ConfigDatah->Flags & CONFIG_TEMP_DISPLAY_FAHRENHEIT
      )
   {
      BitsChanged = ConfigTempUnits ( WebServerh, ConfigDatah );
      FlagsChanged = FlagsChanged || BitsChanged;
   }


   if (  WebServerh->hasArg ( "sensor_debug" )
         || ConfigDatah->Flags & CONFIG_DEBUG_MESSAGE_ENABLED
      )
   {
      BitsChanged = ConfigDebug ( WebServerh, ConfigDatah );
      FlagsChanged = FlagsChanged || BitsChanged;
   }


   // If any flags changed from the reconfiguration, update the EEPROM field.
   if ( FlagsChanged == true )
   {
      SetROMValue ( PCONFIG_OFFSET_FLAGS,
                    (uint8_t*) &ConfigDatah->Flags,
                    sizeof ( ConfigDatah->Flags )
                  );
   }


   // The values in the configuration structure should already be up to date
   // with any changes, but re-read them again anyway just to be sure.
   //
   // Display the new configuration settings on the serial port log.

   EEPROM.get ( PCONFIG_OFFSET, *ConfigDatah );
   ShowROMValues ( ConfigDatah, "After HandleSensorConfigPost:" );
   Serial.println ( "   New settings will take effect after restart" );


   // TODO: Set a run-time flag that tells the system to refresh the static parts
   //       of the screen display in case the new settings changed the set points.

   // TODO: Make this a static page in SPIFFS.


   // Redirect the client to the success page
   WebServerh->sendHeader ( "Location", "/UpdateSuccess.html" );
   // 303 - See other (redirect).
   WebServerh->send ( 303 );
}

// -----------------------------------------------< /HandleSensorConfigPost >---



// -----------------------------------------------------------------------------
// ---------------------------------------------------< HandleWifiConfigGet >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    A web server event handler to populate wifi configuration web
//             form with the current stored wifi settings.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
//
//             FilePath - The .html file that holds the wifi configuration page.
//
// RETURNS:    void
//
// NOTES:      -  The wifi configuraiton web page has built-in place holders for
//                the current values.  This routine replaces those place holders
//                with the actual stored values before the page is returned to
//                the we client.
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------

static void HandleWifiConfigGet (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah,
   const char*       FilePath
)
{
   String   FileContent;
   int      FileSize = 0;

   // Load the web page into memory so it can be modified.
   FileSize = LoadFile ( SPIFFS, FilePath, FileContent );

   if ( FileSize > 0 )
   {
      if ( ConfigDatah->LabelLength > 0 )
      {
         ReplaceSensorName ( ConfigDatah, FileContent );
      }

      GetWifiNetworks( ConfigDatah, FileContent );

      // Replaces the wifi station radio place holder.
      if ( ConfigDatah->Flags & CONFIG_WIFI_STATION_ENABLED )
      {
         FileContent.replace ( "\"set_wifi_Y\"", "\"Y\" checked" );
         FileContent.replace ( "\"set_wifi_N\"", "\"N\"" );
      }

      else
      {
         FileContent.replace ( "\"set_wifi_Y\"", "\"Y\"" );
         FileContent.replace ( "\"set_wifi_N\"", "\"N\" checked" );
      }

      // Replaces the SSID name and password place holders.
      FileContent.replace ( "set_ssid", ConfigDatah->WifiSSID );
      FileContent.replace ( "set_pass", ConfigDatah->WifiPassword );

      // Replace the Access Point IP address place holders.
      FileContent.replace ( "set_ap0", String ( ConfigDatah->AccessIP [ 0 ] ) );
      FileContent.replace ( "set_ap1", String ( ConfigDatah->AccessIP [ 1 ] ) );
      FileContent.replace ( "set_ap2", String ( ConfigDatah->AccessIP [ 2 ] ) );
      FileContent.replace ( "set_ap3", String ( ConfigDatah->AccessIP [ 3 ] ) );

      // Replace the Access Point network mask place holders.
      FileContent.replace ( "set_nm0", String ( ConfigDatah->NetMask [ 0 ] ) );
      FileContent.replace ( "set_nm1", String ( ConfigDatah->NetMask [ 1 ] ) );
      FileContent.replace ( "set_nm2", String ( ConfigDatah->NetMask [ 2 ] ) );
      FileContent.replace ( "set_nm3", String ( ConfigDatah->NetMask [ 3 ] ) );

      // Replace the Access Point gateway IP address place holders.
      FileContent.replace ( "set_gw0", String ( ConfigDatah->Gateway [ 0 ] ) );
      FileContent.replace ( "set_gw1", String ( ConfigDatah->Gateway [ 1 ] ) );
      FileContent.replace ( "set_gw2", String ( ConfigDatah->Gateway [ 2 ] ) );
      FileContent.replace ( "set_gw3", String ( ConfigDatah->Gateway [ 3 ] ) );

      // Replace the Serial Baud rate place holders and set the "selected"
      // property on the one that matches the current stored value.
      for ( int i = 0; i < BAUD_LIST_SIZE; i++ )
      {
         String FieldID;
         String IDValue;

         FieldID  = "\"set_";
         FieldID += BaudList[ i ];
         FieldID += "\"";

         IDValue = "\"";
         IDValue += BaudList[ i ];
         IDValue += "\"" ;
         if ( ConfigDatah->SerialBaud == BaudList[ i ] )
         {
            IDValue += " selected";
         }

         FileContent.replace ( FieldID, IDValue );
      }

      // Replaces the HTML web server port place holder.
      FileContent.replace ( "set_webport", String ( ConfigDatah->WebServerPort ) );

      // Replaces the WS server port place holder.
      FileContent.replace ( "set_wsport", String ( ConfigDatah->WebSocketServerPort ) );

      // Send the updated web page to the web client.
      WebServerh->sendContent ( FileContent );

      Serial.printf ( "HandleWifiConfigGet - Sent file \"%s\" \n", FilePath );
   }

   else
   {
      Serial.printf ( "HandleWifiConfigGet - File Not Found - \"%s\" \n", FilePath );

      // File Not Found
      Send404 ( WebServerh );
   }
}

// --------------------------------------------------< /HandleWifiConfigGet >---



// -----------------------------------------------------------------------------
// --------------------------------------------------< HandleWifiConfigPost >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    A web server event handler to retrieve wifi settings from a wifi
//             configuration web page and update associated stored values.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
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

static void HandleWifiConfigPost (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
)
{
   String Message;

   if ( ConfigDatah->Flags & CONFIG_DEBUG_MESSAGE_ENABLED )
   {
      GetWebMethodText ( WebServerh, Message );
      Serial.println ( "DEBUG HandleWifiConfigPost: Method is " + Message );
      Serial.printf  ( "DEBUG HandleWifiConfigPost: Arg count is %d \n", WebServerh->args() );

      for ( int i = 0; i < WebServerh->args(); i++ )
      {
         Serial.println ( "   " + WebServerh->argName ( i ) + ": " + WebServerh->arg ( i ) );
      }
   }

   if ( WebServerh->hasArg ( "wifi_station" ) )
   {
      ConfigWifiEnable ( WebServerh, ConfigDatah );
   }

   if ( WebServerh->hasArg ( "ssid" ) )
   {
      ConfigSSID ( WebServerh, ConfigDatah );
   }


   if ( WebServerh->hasArg ( "password" ) )
   {
      ConfigPassword ( WebServerh, ConfigDatah );
   }

   if (  WebServerh->hasArg ( "ap_0" )
         && WebServerh->hasArg ( "ap_1" )
         && WebServerh->hasArg ( "ap_2" )
         && WebServerh->hasArg ( "ap_3" )
      )
   {
      ConfigAccessIP ( WebServerh, ConfigDatah );
   }


   if (  WebServerh->hasArg ( "nm_0" )
         && WebServerh->hasArg ( "nm_1" )
         && WebServerh->hasArg ( "nm_2" )
         && WebServerh->hasArg ( "nm_3" )
      )
   {
      ConfigAPNetmask ( WebServerh, ConfigDatah );
   }


   if (  WebServerh->hasArg ( "gw_0" )
         && WebServerh->hasArg ( "gw_1" )
         && WebServerh->hasArg ( "gw_2" )
         && WebServerh->hasArg ( "gw_3" )
      )
   {
      ConfigAPGateway ( WebServerh, ConfigDatah );
   }


   if ( WebServerh->hasArg ( "set_baud" ) )
   {
      ConfigBaud ( WebServerh, ConfigDatah );
   }

   if ( WebServerh->hasArg ( "webport" ) )
   {
      ConfigWebPort ( WebServerh, ConfigDatah );
   }

   if ( WebServerh->hasArg ( "wsport" ) )
   {
      ConfigDataPort ( WebServerh, ConfigDatah );
   }


   // The values in the configuration structure should already be up to date
   // with any changes, but re-read them again anyway just to be sure.
   //
   // Display the new configuration settings on the serial port log.

   EEPROM.get ( PCONFIG_OFFSET, *ConfigDatah );
   ShowROMValues ( ConfigDatah, "After HandleWifiConfigPost:" );
   Serial.println ( "   New settings will take effect after restart" );


   // Redirect the client to the success page.
   WebServerh->sendHeader ( "Location", "/UpdateSuccess.html" );
   // 303 - See other (redirect).
   WebServerh->send ( 303 );
}

// -------------------------------------------------< /HandleWifiConfigPost >---



// -----------------------------------------------------------------------------
// ----------------------------------------------------< HandleSensorDataJS >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    A web server event handler to update a javascript file with the
//             current IP address of the sensor prior to returning to the client.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
//
//             FilePath - The javascript file to insert the IP address into.
//
// RETURNS:    void
//
// NOTES:      -  The javascript file has a built-in place holder for the IP
//                address and port number in the form of:  ws://w.x.y.z:p
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------

static void HandleSensorDataJS (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah,
   const char*       FilePath
)
{
   String   FileContent;
   int      FileSize = 0;

   // Load the web page into memory so it can be modified.
   FileSize = LoadFile ( SPIFFS, FilePath, FileContent );

   if ( FileSize > 0 )
   {
      String NewURL = String ( "ws://" );
      NewURL += String ( ConfigDatah->StationIP[ 0 ] ) + String ( "." );
      NewURL += String ( ConfigDatah->StationIP[ 1 ] ) + String ( "." );
      NewURL += String ( ConfigDatah->StationIP[ 2 ] ) + String ( "." );
      NewURL += String ( ConfigDatah->StationIP[ 3 ] ) + String ( ":" );
      NewURL += String ( ConfigDatah->WebSocketServerPort );

      // Replaces the javascript place holder for the wifi IP address.
      FileContent.replace ( "ws://w.x.y.z:p", NewURL );

      // Send the updated web page to the web client.
      WebServerh->sendContent ( FileContent );

      Serial.printf ( "HandleSensorDataJS - Sent file \"%s\" \n", FilePath );
   }

   else
   {
      Serial.printf ( "HandleSensorDataJS - File Not Found - \"%s\" \n", FilePath );

      // File Not Found
      Send404 ( WebServerh );
   }
}

// ---------------------------------------------------< /HandleSensorDataJS >---



// -----------------------------------------------------------------------------
// ---------------------------------------------------------< RestartSystem >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    A function that can be invoked by a timer to restart the system.
//
// PARAMETERS: void
//
// RETURNS:    void
//
// NOTES:      -  See the HandleRestart() function for timer setup.
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 31Jan2019 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------

static void RestartSystem ( void* Args )
{
   ESP.restart();
}

// --------------------------------------------------------< /RestartSystem >---



// -----------------------------------------------------------------------------
// ---------------------------------------------------------< HandleRestart >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    A web server event handler that causes the system to restart.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
//
//             FilePath - The .html file to display before the restart happens.
//
// RETURNS:    void
//
// NOTES:      -  WARNING: The call to restart the system does not happen here.  
//                A timer is set that will invoke the restart after a delay.  
//                The delay allows time for the system to return a status page
//                to the client, and allows this event handler to return thus
//                resuming normal operation before the restart.  Does resuming
//                normal operation really matter...maybe not but it feels right.
//
//             -  The time delay used here was determined experimetally as one 
//                that normally allows the status page to be returned before the 
//                reset occurs, but the system timing is a bit unpredictable.
//                It can happen that the system resets before the page gets 
//                displayed by the client, but since the page is already on the
//                wire, it does get displayed eventually.  It looks the same at
//                the client either way, it's just that the system may be reset
//                and running again before the client sees something change.
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 31Jan2019 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------

static void HandleRestart ( 
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah ,
   const char*       FilePath
)
{
   void* Args = NULL;

   // Send the client an acknowledgement and status page.
   HandleFileRequest ( WebServerh, ConfigDatah, String ( "/Restarting.html" ) );

   // Set a time that will invoke the restart after a delay.
   os_timer_setfn ( &RestartTimer, RestartSystem, Args );
   os_timer_arm ( &RestartTimer, 6000, true );
}

// --------------------------------------------------------< /HandleRestart >---



// -----------------------------------------------------------------------------
// -----------------------------------------------------------< ConfigProbe >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Update the stored PROBE ATTACHED setting from a web form entry.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
//
// RETURNS:    boolean == 'true' if any of the flag bits were changed.
//                     == 'false' if the flag bits were not changed.
//
// NOTES:      -  WARNING: Multiple configuration flags are used, so to avoid
//                repeatedly re-writing the field in EEPROM as various flags are
//                changed, this routine updates the flags in the configuration
//                structure, but does not update the EEPROM with the new
//                settings.  It is the callers responsibility to update EEPROM!
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------

static boolean ConfigProbe (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
)
{
   boolean BitsChanged = false;

   if (  ( WebServerh->arg("sensor_probe").equalsIgnoreCase(String("Y")) )
         && ( ! ( ConfigDatah->Flags & CONFIG_TEMP_PROBE_CONNECTED ) )
      )
   {
      // The web entry says that a temperature probe is connected, but the
      // configuration flags don't currently reflect one.  Set the flag.
      ConfigDatah->Flags |= CONFIG_TEMP_PROBE_CONNECTED;
      BitsChanged = true;
   }

   else if (  ( WebServerh->arg("sensor_probe").equalsIgnoreCase(String("N")) )
              && ( ConfigDatah->Flags & CONFIG_TEMP_PROBE_CONNECTED )
           )
   {
      // The web entry says that a temperature probe is NOT connected, but the
      // configuration flags show that one is connected.  Unset the flag.
      ConfigDatah->Flags &= ~CONFIG_TEMP_PROBE_CONNECTED;
      BitsChanged = true;
   }

   return BitsChanged;
}

// ----------------------------------------------------------< /ConfigProbe >---



// -----------------------------------------------------------------------------
// -----------------------------------------------------------< ConfigRelay >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Update the stored RELAY ATTACHED setting from a web form entry.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
//
// RETURNS:    boolean == 'true' if any of the flag bits were changed.
//                     == 'false' if the flag bits were not changed.
//
// NOTES:      -  WARNING: Multiple configuration flags are used, so to avoid
//                repeatedly re-writing the field in EEPROM as various flags are
//                changed, this routine updates the flags in the configuration
//                structure, but does not update the EEPROM with the new
//                settings.  It is the callers responsibility to update EEPROM!
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------

static boolean ConfigRelay (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
)
{
   boolean BitsChanged = false;

   if (  ( WebServerh->arg("sensor_relay").equalsIgnoreCase(String("Y")) )
         && ( ! ( ConfigDatah->Flags & CONFIG_DEVICE_RELAY_CONNECTED ) )
      )
   {
      // The web entry says that a device-relay is connected, but the
      // configuration flags don't currently reflect one.  Set the flag.
      ConfigDatah->Flags |= CONFIG_DEVICE_RELAY_CONNECTED;
      BitsChanged = true;
   }

   else if (  ( WebServerh->arg("sensor_relay").equalsIgnoreCase(String("N")) )
              && ( ConfigDatah->Flags & CONFIG_DEVICE_RELAY_CONNECTED )
           )
   {
      // The web entry says that a device-relay is NOT connected, but the
      // configuration flags show that one is connected.  Unset the flag.
      ConfigDatah->Flags &= ~CONFIG_DEVICE_RELAY_CONNECTED;
      BitsChanged = true;
   }

   return BitsChanged;
}

// ----------------------------------------------------------< /ConfigRelay >---



// -----------------------------------------------------------------------------
// ---------------------------------------------------------< ConfigLowTemp >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Update the stored LOW TEMP set point from a web form entry.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
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

static void ConfigLowTemp (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
)
{
   assert ( sizeof ( ConfigDatah->TempLowLimit ) == sizeof ( int16_t ) );

   int16_t Value = (int16_t) WebServerh->arg("sensor_lowtemp").toInt();

   if ( Value == 0 )
   {
      // The value might be zero because that was what was entered in the web
      // page, or it might be because the web entry cannot be converted to int.
      // Figure out whether or not the web page entry was actually zero.
      if ( ! WebServerh->arg("sensor_lowtemp").equals(String('0')) )
      {
         // The web page entry was not zero so trick the code into ignoring the
         // entry and not updating the stored value.
         Value = ConfigDatah->TempLowLimit;
      }
   }

   if ( Value != ConfigDatah->TempLowLimit )
   {
      SetROMValue ( PCONFIG_OFFSET_TEMPLOWLIMIT,
                    (uint8_t*) &Value,
                    sizeof ( ConfigDatah->TempLowLimit )
                  );

      ConfigDatah->TempLowLimit = Value;
   }
}

// --------------------------------------------------------< /ConfigLowTemp >---



// -----------------------------------------------------------------------------
// --------------------------------------------------------< ConfigHighTemp >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Update the stored HIGH TEMP set point from a web form entry.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
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

static void ConfigHighTemp (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
)
{
   assert ( sizeof ( ConfigDatah->TempHighLimit ) == sizeof ( int16_t ) );

   int16_t Value = (int16_t) WebServerh->arg("sensor_hightemp").toInt();

   if ( Value == 0 )
   {
      // The value might be zero because that was what was entered in the web
      // page, or it might be because the web entry cannot be converted to int.
      // Figure out whether or not the web page entry was actually zero.
      if ( ! WebServerh->arg("sensor_hightemp").equals(String('0')) )
      {
         // The web page entry was not zero so trick the code into ignoring the
         // entry and not updating the stored value.
         Value = ConfigDatah->TempHighLimit;
      }
   }

   if ( Value != ConfigDatah->TempHighLimit )
   {
      SetROMValue ( PCONFIG_OFFSET_TEMPHIGHLIMIT,
                    (uint8_t*) &Value,
                    sizeof ( ConfigDatah->TempHighLimit )
                  );

      ConfigDatah->TempHighLimit = Value;
   }
}

// -------------------------------------------------------< /ConfigHighTemp >---



// -----------------------------------------------------------------------------
// -----------------------------------------------------< ConfigSensorLabel >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Update the SENSOR LABEL from a web form entry.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
//
// RETURNS:    void
//
// NOTES:      -  A label that is longer than the allowed maximum is reported
//                the serial log, but otherwise quietly ignored.
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------

static boolean ConfigSensorLabel (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
)
{
   assert ( sizeof ( ConfigDatah->LabelLength ) == sizeof ( uint8_t ) );

   uint8_t ValueLength = (uint8_t) WebServerh->arg("sensor_label").length();


   if ( ValueLength > PCONFIG_MAX_LABEL )
   {
      Serial.printf ( "ERROR: The specified sensor label is too long (%d)!  Ignoring setting. \n", ValueLength );
   }

   else
   {
      if ( ! ( WebServerh->arg("sensor_label").equals( String ( ConfigDatah->Label ) ) ) )
      {
         char Value[ PCONFIG_MAX_LABEL + 1 ];

         WebServerh->arg("sensor_label").toCharArray( Value, PCONFIG_MAX_LABEL );

         // Add a NULL terminator to the string value.
         Value[ ValueLength ] = 0;

         SetROMValue ( PCONFIG_OFFSET_LABEL,
                       (uint8_t*) &Value,
                       ValueLength + 1
                     );

         SetROMValue ( PCONFIG_OFFSET_LABELLENGTH,
                       (uint8_t*) &ValueLength,
                       sizeof ( ValueLength )
                     );

         ConfigDatah->LabelLength = ValueLength;
         memcpy ( Value, ConfigDatah->Label, ValueLength + 1 );
      }
   }
}

// ----------------------------------------------------< /ConfigSensorLabel >---



// -----------------------------------------------------------------------------
// ----------------------------------------------------< ConfigWaitInterval >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Update the WAIT TIME INTERVAL from a web form entry.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
//
// RETURNS:    void
//
// NOTES:      -  WARNING: A wait time of zero seconds is not allowed and if
//                specified, will quietly be ignored (noted on serial log).
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------

static void ConfigWaitInterval (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
)
{
   assert ( sizeof ( ConfigDatah->SensorWaitTime ) == sizeof ( int32_t ) );

   int32_t Value = (int32_t) WebServerh->arg("sensor_interval").toInt();

   if ( Value == 0 )
   {
      // Do not allow a zero interval.  Trick the code into
      // ignoring the entry and not updating the stored value.
      Serial.printf ( "ERROR: The sensor read interval cannot be zero!  Ignoring setting. \n" );
      Value = ConfigDatah->SensorWaitTime / 1000;
   }

   // Web page specification is for seconds, but stored value is milliseconds.
   Value *= 1000;

   if ( Value != ConfigDatah->SensorWaitTime )
   {
      SetROMValue ( PCONFIG_OFFSET_SENSORWAITTIME,
                    (uint8_t*) &Value,
                    sizeof ( ConfigDatah->SensorWaitTime )
                  );

      ConfigDatah->SensorWaitTime = Value;
   }
}

// ---------------------------------------------------< /ConfigWaitInterval >---



// -----------------------------------------------------------------------------
// -------------------------------------------------------< ConfigTempUnits >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Update the TEMPERATURE UNITS from a web form entry.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
//
// RETURNS:    boolean == 'true' if any of the flag bits were changed.
//                     == 'false' if the flag bits were not changed.
//
// NOTES:      -  The temperature units are represented by a bit in the 'Flags'
//                field of the configuration structure that is either on or off.
//                If the CONFIG_TEMP_DISPLAY_FAHRENHEIT bit is on then degrees
//                fahrenheit are displayed, otherwise degrees are in celcius.
//
//             -  WARNING: Multiple configuration flags are used, so to avoid
//                repeatedly re-writing the field in EEPROM as various flags are
//                changed, this routine updates the flags in the configuration
//                structure, but does not update the EEPROM with the new
//                settings.  It is the callers responsibility to update EEPROM!
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------

static boolean ConfigTempUnits (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
)
{
   boolean BitsChanged = false;

   if (  ( WebServerh->arg("sensor_units").equals(String('F')) )
         && ( ! ( ConfigDatah->Flags & CONFIG_TEMP_DISPLAY_FAHRENHEIT ) )
      )
   {
      // Fahrenheit specified but not currently set in flags.
      ConfigDatah->Flags |= CONFIG_TEMP_DISPLAY_FAHRENHEIT;
      BitsChanged = true;
   }

   else if (  ( WebServerh->arg("sensor_units").equals(String('C')) )
              && ( ConfigDatah->Flags & CONFIG_TEMP_DISPLAY_FAHRENHEIT )
           )
   {
      // Celcius specified but fahrenheit is currently set in flags.
      ConfigDatah->Flags &= ~CONFIG_TEMP_DISPLAY_FAHRENHEIT;
      BitsChanged = true;
   }

   return BitsChanged;
}

// ------------------------------------------------------< /ConfigTempUnits >---



// -----------------------------------------------------------------------------
// -----------------------------------------------------------< ConfigDebug >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Update the stored PROBE ATTACHED setting from a web form entry.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
//
// RETURNS:    boolean == 'true' if any of the flag bits were changed.
//                     == 'false' if the flag bits were not changed.
//
// NOTES:      -  WARNING: Multiple configuration flags are used, so to avoid
//                repeatedly re-writing the field in EEPROM as various flags are
//                changed, this routine updates the flags in the configuration
//                structure, but does not update the EEPROM with the new
//                settings.  It is the callers responsibility to update EEPROM!
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------

static boolean ConfigDebug (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
)
{
   boolean BitsChanged = false;

   if (  ( WebServerh->arg("sensor_debug").equalsIgnoreCase(String("Y")) )
         && ( ! ( ConfigDatah->Flags & CONFIG_DEBUG_MESSAGE_ENABLED ) )
      )
   {
      // The web entry says that debug messages should be enabled, but they are
      // not enabled in the configuration flags.  Set the flag.
      ConfigDatah->Flags |= CONFIG_DEBUG_MESSAGE_ENABLED;
      BitsChanged = true;
   }

   else if (  ( WebServerh->arg("sensor_debug").equalsIgnoreCase(String("N")) )
              && ( ConfigDatah->Flags & CONFIG_DEBUG_MESSAGE_ENABLED )
           )
   {
      // The web entry says that debug messages should be NOT enabled, but they
      // are enabled in the configuration flags.  Unset the flag.
      ConfigDatah->Flags &= ~CONFIG_DEBUG_MESSAGE_ENABLED;
      BitsChanged = true;
   }

   return BitsChanged;
}

// ----------------------------------------------------------< /ConfigDebug >---



// -----------------------------------------------------------------------------
// ------------------------------------------------------< ConfigWifiEnable >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Update the wifi-enabled flag from a web form entry.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
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

static void ConfigWifiEnable (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
)
{
   boolean BitsChanged = false;

   if (  ( WebServerh->arg("wifi_station").equalsIgnoreCase(String("Y")) )
         && ( ! ( ConfigDatah->Flags & CONFIG_WIFI_STATION_ENABLED ) )
      )
   {
      // The web entry says that wifi-station mode is enabled, but the
      // configuration flags don't currently reflect that.  Set the flag.
      ConfigDatah->Flags |= CONFIG_WIFI_STATION_ENABLED;
      BitsChanged = true;
   }

   else if (  ( WebServerh->arg("wifi_station").equalsIgnoreCase(String("N")) )
              && ( ConfigDatah->Flags & CONFIG_WIFI_STATION_ENABLED )
           )
   {
      // The web entry says that wifi-station mode is NOT enabled, but the
      // configuration flags show that it is.  Unset the flag.
      ConfigDatah->Flags &= ~CONFIG_WIFI_STATION_ENABLED;
      BitsChanged = true;
   }

   if ( BitsChanged == true )
   {
      SetROMValue ( PCONFIG_OFFSET_FLAGS,
                    (uint8_t*) &ConfigDatah->Flags,
                    sizeof ( ConfigDatah->Flags )
                  );
   }
}

// -----------------------------------------------------< /ConfigWifiEnable >---



// -----------------------------------------------------------------------------
// ------------------------------------------------------------< ConfigSSID >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Update the wifi SSID from a web form entry.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
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

static void ConfigSSID (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
)
{
   assert ( sizeof ( ConfigDatah->WifiSSIDLength ) == sizeof ( uint8_t ) );

   uint8_t ValueLength = (uint8_t) WebServerh->arg("ssid").length();

   if ( ValueLength > PCONFIG_MAX_SSID )
   {
      Serial.printf ( "ERROR: The specified SSID name is too long (%d)!  Ignoring setting. \n", ValueLength );
   }

   else
   {
      if ( ! ( WebServerh->arg("ssid").equals( String (ConfigDatah->WifiSSID) ) ) )
      {
         char  Value[ PCONFIG_MAX_SSID + 1 ];
         int   Remains;

         WebServerh->arg("ssid").toCharArray ( Value, PCONFIG_MAX_SSID );

         // Add a NULL terminator to the value string.
         Value[ ValueLength ] = 0;

         SetROMValue ( PCONFIG_OFFSET_WIFISSID,
                       (uint8_t*) &Value,
                       ValueLength + 1
                     );

         // Set any remaining bytes from the previous value to zeros.

         Remains = ConfigDatah->WifiSSIDLength - ValueLength;
         if ( Remains > 0 )
         {
            memset ( Value, 0, min ( Remains, (int) sizeof ( Value ) ) );
            SetROMValue ( PCONFIG_OFFSET_WIFISSID + ValueLength,
                          (uint8_t*) &Value,
                          Remains
                        );
         }

         SetROMValue ( PCONFIG_OFFSET_WIFISSIDLENGTH,
                       (uint8_t*) &ValueLength,
                       sizeof ( ValueLength )
                     );

         ConfigDatah->WifiSSIDLength = ValueLength;
         memcpy ( Value, ConfigDatah->WifiSSID, ValueLength + 1 );
      }
   }
}

// -----------------------------------------------------------< /ConfigSSID >---



// -----------------------------------------------------------------------------
// --------------------------------------------------------< ConfigPassword >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Update the wifi PASSWORD from a web form entry.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
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

static void ConfigPassword (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
)
{
   assert ( sizeof ( ConfigDatah->WifiPasswordLength ) == sizeof ( uint8_t ) );

   uint8_t ValueLength = (uint8_t) WebServerh->arg("password").length();

   if ( ValueLength > PCONFIG_MAX_PASSWORD )
   {
      Serial.printf ( "ERROR: The specified SSID password is too long (%d)!  Ignoring setting. \n", ValueLength );
   }

   else
   {
      if ( ! ( WebServerh->arg("password").equals( String ( ConfigDatah->WifiPassword ) ) ) )
      {
         char Value[ PCONFIG_MAX_PASSWORD + 1 ];

         WebServerh->arg("password").toCharArray ( Value, PCONFIG_MAX_PASSWORD );

         // Add a NULL terminator to the value string.
         Value[ ValueLength ] = 0;

         SetROMValue ( PCONFIG_OFFSET_WIFIPASSWORD,
                       (uint8_t*) &Value,
                       ValueLength + 1
                     );

         SetROMValue ( PCONFIG_OFFSET_WIFIPASSWORDLENGTH,
                       (uint8_t*) &ValueLength,
                       sizeof ( ValueLength )
                     );

         ConfigDatah->WifiPasswordLength = ValueLength;
         memcpy ( Value, ConfigDatah->WifiPassword, ValueLength + 1 );
      }
   }
}

// -------------------------------------------------------< /ConfigPassword >---



// -----------------------------------------------------------------------------
// --------------------------------------------------------< ConfigAccessIP >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Update the ACCESS POINT IP address from a web form entry.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
//
// RETURNS:    void
//
// NOTES:      -  Private IPV4 addresses have specific allowable ranges:
//
//                      10.0.0.0 –  10.255.255.255
//                    172.16.0.0 –  172.31.255.255
//                   192.168.0.0 – 192.168.255.255
//
//                This routine should only allows addresses with those ranges.
//                Any invalid address is invalid and ignored.
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------

static void ConfigAccessIP (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
)
{
   uint8_t  NewIP[ 4 ];
   boolean  Update = true;
   String   FieldID;


   assert ( sizeof ( ConfigDatah->AccessIP[ 0 ] ) == sizeof ( uint8_t ) );

   for ( int i = 0; i < 4 && Update == true; i++ )
   {
      FieldID  = "ap_";
      FieldID += String ( i );

      NewIP[ i ] = (uint8_t) WebServerh->arg(FieldID).toInt();

      if ( NewIP[ i ] > 255 )
      {
         Serial.printf ( "ERROR: Access point IP address segment value of %u " \
                         "is not in the range of 0 to 255!  Ignoring setting. \n",
                         NewIP[ i ]
                       );
         Update = false;
      }

      if ( i == 0 )
      {
         if ( NewIP[ i ] != 10 && NewIP[ i ] != 172 && NewIP[ i ] != 192 )
         {
            Serial.printf ( "ERROR: Access point IP addresses must start with 10, 172, or 192.  Ignoring setting. \n" );
            Update = false;
         }
      }

      else if ( i == 1 )
      {
         if ( NewIP[ 0 ] == 172 )
         {
            if ( NewIP[ i ] < 16 || NewIP[ i ] > 31 )
            {
               Serial.printf ( "ERROR: Access point IP addresses starting with 172 must have " \
                               "a second segment between 16 and 31.  Ignoring setting. \n"
                             );
               Update = false;
            }
         }

         else if ( NewIP[ 0 ] == 192 )
         {
            if ( NewIP[ i ] != 168 )
            {
               Serial.printf ( "ERROR: Access point IP addresses starting with 192 must have " \
                               "a second segment equal to 168.  Ignoring setting. \n"
                             );
               Update = false;
            }
         }
      }
   }

   if ( Update == true )
   {
      for ( int i = 0; i < 4; i++ )
      {
         if ( NewIP[ i ] != ConfigDatah->AccessIP[ i ] )
         {
            SetROMValue ( PCONFIG_OFFSET_ACCESSIP + i,
                          (uint8_t*) &NewIP[ i ],
                          sizeof ( uint8_t )
                        );
            ConfigDatah->AccessIP[ i ] = NewIP[ i ];
         }
      }
   }
}

// -------------------------------------------------------< /ConfigAccessIP >---



// -----------------------------------------------------------------------------
// -------------------------------------------------------< ConfigAPNetmask >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Update the access point NETMASK address from a web form entry.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
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

static void ConfigAPNetmask (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
)
{
   uint8_t  NewIP[ 4 ];
   boolean  Update = true;
   String   FieldID;


   assert ( sizeof ( ConfigDatah->NetMask[ 0 ] ) == sizeof ( uint8_t ) );

   for ( int i = 0; i < 4 && Update == true; i++ )
   {
      FieldID  = "nm_";
      FieldID += String ( i );

      NewIP[ i ] = (uint8_t) WebServerh->arg(FieldID).toInt();

      if ( NewIP[ i ] > 255 )
      {
         Serial.printf ( "ERROR: The network mask address segment value of %u " \
                         "is not in the range of 0 to 255!  Ignoring setting. \n",
                         NewIP[ i ]
                       );
         Update = false;
      }
   }

   if ( Update == true )
   {
      for ( int i = 0; i < 4; i++ )
      {
         if ( NewIP[ i ] != ConfigDatah->NetMask[ i ] )
         {
            SetROMValue ( PCONFIG_OFFSET_NETMASK + i,
                          (uint8_t*) &NewIP[ i ],
                          sizeof ( uint8_t )
                        );
            ConfigDatah->NetMask[ i ] = NewIP[ i ];
         }
      }
   }
}

// ------------------------------------------------------< /ConfigAPNetmask >---



// -----------------------------------------------------------------------------
// -------------------------------------------------------< ConfigAPGateway >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Update the access point GATEWAY address from a web form entry.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
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

static void ConfigAPGateway (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
)
{
   uint8_t  NewIP[ 4 ];
   boolean  Update = true;
   String   FieldID;


   assert ( sizeof ( ConfigDatah->Gateway[ 0 ] ) == sizeof ( uint8_t ) );

   for ( int i = 0; i < 4 && Update == true; i++ )
   {
      FieldID  = "gw_";
      FieldID += String ( i );

      NewIP[ i ] = (uint8_t) WebServerh->arg(FieldID).toInt();

      if ( NewIP[ i ] > 255 )
      {
         Serial.printf ( "ERROR: The access point gateway address segment value of %u " \
                         "is not in the range of 0 to 255!  Ignoring setting. \n",
                         NewIP[ i ]
                       );
         Update = false;
      }
   }

   if ( Update == true )
   {
      for ( int i = 0; i < 4; i++ )
      {
         if ( NewIP[ i ] != ConfigDatah->Gateway[ i ] )
         {
            SetROMValue ( PCONFIG_OFFSET_GATEWAY + i,
                          (uint8_t*) &NewIP[ i ],
                          sizeof ( uint8_t )
                        );
            ConfigDatah->Gateway[ i ] = NewIP[ i ];
         }
      }
   }
}

// ------------------------------------------------------< /ConfigAPGateway >---



// -----------------------------------------------------------------------------
// ------------------------------------------------------------< ConfigBaud >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Update the SERIAL BAUD RATE from a web form entry.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
//
// RETURNS:    void
//
// NOTES:      -  WARNING: Only the more modern and standard serial port baud
//                rates are allowed.  Some older ports support lower rates, but
//                that's unnecessary in modern times, and some modern ports may
//                well support higher rates, but they are largely non-standard
//                and also unnecessary, so to avoid problems, keep it simple.
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------

static void ConfigBaud (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
)
{
   assert ( sizeof ( ConfigDatah->SerialBaud ) == sizeof ( uint32_t ) );


   uint32_t Value = (uint32_t) WebServerh->arg("set_baud").toInt();
   bool     InList = false;

   for ( int i = 0; i < BAUD_LIST_SIZE; i++ )
   {
      if ( Value == BaudList[ i ] )
      {
         InList = true;
      }
   }

   if ( InList == false )
   {
      // Only the standard serial port baud rates are allowed.  Trick the code
      // into ignoring the entry and not updating the stored value.
      Serial.printf ( "ERROR: The specified serial baud rate of \"%u\" is not supported!  Ignoring setting. \n", Value );
      Value = ConfigDatah->SerialBaud;
   }


   if ( Value != ConfigDatah->SerialBaud )
   {
      SetROMValue ( PCONFIG_OFFSET_SERIALBAUD,
                    (uint8_t*) &Value,
                    sizeof ( ConfigDatah->SerialBaud )
                  );

      ConfigDatah->SerialBaud = Value;
   }
}

// -----------------------------------------------------------< /ConfigBaud >---



// -----------------------------------------------------------------------------
// ---------------------------------------------------------< ConfigWebPort >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Update the HTML web server port from a web form entry.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
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

static void ConfigWebPort (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
)

{
   assert ( sizeof ( ConfigDatah->WebServerPort ) == sizeof ( uint16_t ) );

   uint16_t Value = (uint16_t) WebServerh->arg("webport").toInt();

   if ( Value == 0 )
   {
      // The value might be zero because that was what was entered in the web
      // page, or it might be because the web entry cannot be converted to int.
      // Figure out whether or not the web page entry was actually zero.
      if ( ! WebServerh->arg("webport").equals(String('0')) )
      {
         // The web page entry was not zero so trick the code into ignoring the
         // entry and not updating the stored value.
         Value = ConfigDatah->WebServerPort;
      }
   }

   if ( Value != ConfigDatah->WebServerPort )
   {
      SetROMValue ( PCONFIG_OFFSET_WEBSERVERPORT,
                    (uint8_t*) &Value,
                    sizeof ( ConfigDatah->WebServerPort )
                  );

      ConfigDatah->WebServerPort = Value;
   }
}

// --------------------------------------------------------< /ConfigWebPort >---



// -----------------------------------------------------------------------------
// --------------------------------------------------------< ConfigDataPort >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Update the web-socket server port from a web form entry.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             ConfigDatah - Pointer to the configuration data structure.
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

static void ConfigDataPort (
   ESP8266WebServer* WebServerh,
   PConfig_t*        ConfigDatah
)

{
   assert ( sizeof ( ConfigDatah->WebSocketServerPort ) == sizeof ( uint16_t ) );

   uint16_t Value = (uint16_t) WebServerh->arg("wsport").toInt();

   if ( Value == 0 )
   {
      // The value might be zero because that was what was entered in the web
      // page, or it might be because the web entry cannot be converted to int.
      // Figure out whether or not the web page entry was actually zero.
      if ( ! WebServerh->arg("wsport").equals(String('0')) )
      {
         // The web page entry was not zero so trick the code into ignoring the
         // entry and not updating the stored value.
         Value = ConfigDatah->WebSocketServerPort;
      }
   }

   if ( Value != ConfigDatah->WebSocketServerPort )
   {
      SetROMValue ( PCONFIG_OFFSET_WEBSOCKETSERVERPORT,
                    (uint8_t*) &Value,
                    sizeof ( ConfigDatah->WebSocketServerPort )
                  );

      ConfigDatah->WebSocketServerPort = Value;
   }
}

// -------------------------------------------------------< /ConfigDataPort >---



// -----------------------------------------------------------------------------
// -----------------------------------------------------< ReplaceSensorName >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    
//
// PARAMETERS: ConfigDatah - Pointer to the configuration data structure.
//
//             HTMLFile - The contents of the HTML source to be updated.
//
// RETURNS:    bool == 'true' when the content was updated.
//                  == 'false' if the file was not updated (no sensor label).
//
// NOTES:      -  If the sensor configuration contains a label and there is a
//                place-holder in the HTML for it, a text substitution is used
//                to insert the label into the content of the page.
//                   The first occurance of this text:
//                      <span name="sensor_name"></span>
//                   Is replaced by this text:
//                      <span name="sensor_name">#Label#</span>
//                   Where #Label# is the configuration value:
//                      ConfigDatah->Label
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 02Feb2019 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------

static void ReplaceSensorName (
   PConfig_t*  ConfigDatah,
   String&     HTMLFile
)
{
   if ( ConfigDatah->LabelLength > 0 )
   {
      HTMLFile.replace ( String ( "<span name=\"sensor_name\"></span>" ),
                         String ( "<span name=\"sensor_name\">" )
                         + String ( "<a href=\"/\">" )
                         + String ( ConfigDatah->Label )
                         + String ( "</a></span>" )
                       );   
   }
}

// ----------------------------------------------------< /ReplaceSensorName >---



// -----------------------------------------------------------------------------
// -------------------------------------------------------< GetWifiNetworks >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Add a list of wifi networks (SSID) to the content of a web page.
//
// PARAMETERS: ConfigDatah - Pointer to the configuration data structure.
//
//             HTMLFile - The contents of the HTML source to be updated.
//
// RETURNS:    void
//
// NOTES:      -  The list of networks will be formated as a a set of HTML 
//                <option></option> tags, and that set of tags will replace
//                text in the HTML source of:  <span name="set_netlist"/>
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 31Jan2019 DSVance    - Initial development.
//
// -----------------------------------------------------------------------------

static void GetWifiNetworks ( 
   PConfig_t*  ConfigDatah,
   String&     HTMLFile
)

{
   int      NetworkCount = 0;
   boolean  ScanAsynch = false;
   boolean  ScanHidden = true;

   
   NetworkCount = WiFi.scanNetworks ( ScanAsynch, ScanHidden );

   DEBUG_PRINTF ( ConfigDatah, "GetWifiNetworks: Found %d wifi networks \n", NetworkCount );

   if ( NetworkCount > 0 )
   {
      String   NWName;
      int      NWChannel;
      int      NWSignal;
      int      NWEncryptID;
      String   NWEncryptType;
      String   NWOption;
      String   NWList;
      
      for ( int i = 0; i < NetworkCount; i++ )
      {
         // SSID - service set identifier 
         // RSSI - Received Signal Strength Indication

         NWName      = WiFi.SSID ( i );
         NWChannel   = WiFi.channel ( i );
         NWSignal    = WiFi.RSSI ( i );
         NWEncryptID = WiFi.encryptionType ( i );        
         NWEncryptType = ( NWEncryptID == ENC_TYPE_NONE ) ? "Open"         // == 7 
                       : ( NWEncryptID == ENC_TYPE_WEP )  ? "WEP"          // == 5
                       : ( NWEncryptID == ENC_TYPE_TKIP ) ? "WPA/PSK"      // == 2
                       : ( NWEncryptID == ENC_TYPE_CCMP ) ? "WPA2/PSK"     // == 4                       
                       : ( NWEncryptID == ENC_TYPE_AUTO ) ? "Auto"         // == 8
                       :                                    "Unknown"
                       ;

         NWOption = (String)"<option value=\"" + NWName + (String)"\">" 
                  + NWName + (String)" (Ch " 
                  + (String)NWChannel + (String) ", " 
                  + (String)NWSignal + " dBm, "
                  + NWEncryptType 
                  + (String)") </option>\n"; 

         NWList += NWOption;
      }

      DEBUG_PRINTLN ( ConfigDatah, NWList );

      HTMLFile.replace ( "<span name=\"set_netlist\"/>", NWList );   
   }
}

// ------------------------------------------------------< /GetWifiNetworks >---



// -----------------------------------------------------------------------------
// --------------------------------------------------------< GetContentType >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Return the MIME type string for a specified file extension.
//
// PARAMETERS: Extension - The file extension for a type of file.
//
//             Type - The MIME type string for the specified extension.
//
// RETURNS:    void
//
// NOTES:      -  If the file extension is not recognized, a default value of
//                "text/plain" is returned.
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSV - Lifted from somewhere on the interwebs.
//
// -----------------------------------------------------------------------------

static void GetContentType (
   String   Extension,
   String&  Type
)
{
   if (      Extension.endsWith ( ".htm"  ) ) Type = "text/html";
   else if ( Extension.endsWith ( ".html" ) ) Type = "text/html";
   else if ( Extension.endsWith ( ".css"  ) ) Type = "text/css";
   else if ( Extension.endsWith ( ".js"   ) ) Type = "application/javascript";
   else if ( Extension.endsWith ( ".png"  ) ) Type = "image/png";
   else if ( Extension.endsWith ( ".gif"  ) ) Type = "image/gif";
   else if ( Extension.endsWith ( ".jpg"  ) ) Type = "image/jpeg";
   else if ( Extension.endsWith ( ".ico"  ) ) Type = "image/x-icon";
   else if ( Extension.endsWith ( ".xml"  ) ) Type = "text/xml";
   else if ( Extension.endsWith ( ".pdf"  ) ) Type = "application/x-pdf";
   else if ( Extension.endsWith ( ".zip"  ) ) Type = "application/x-zip";
   else if ( Extension.endsWith ( ".gz"   ) ) Type = "application/x-gzip";
   // Unrecognized file extension.  Return a default value.
   else                                       Type = "text/plain";
}

// -------------------------------------------------------< /GetContentType >---



// -----------------------------------------------------------------------------
// ------------------------------------------------------< GetWebMethodText >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Return a text representation of the current web server method.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
//             MethodText - A reference to the String to store method text in.
//
// RETURNS:    void
//
// NOTES:      -  HTTP request methods other than those listed below are
//                possible, but the ESP8266 web server only has a method ID
//                for those, so any others are simply shown as unhandled.  In
//                any case, only 'GET' and 'POST' are really expected.
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSV - Initial implementation.
//
// -----------------------------------------------------------------------------

static void GetWebMethodText (
   ESP8266WebServer* WebServerh,
   String&           MethodText
)
{
   HTTPMethod  WebMethod = WebServerh->method();

   MethodText = WebMethod == HTTP_GET     ? "GET"       :
                WebMethod == HTTP_POST    ? "POST"      :
                WebMethod == HTTP_PUT     ? "PUT"       :
                WebMethod == HTTP_PATCH   ? "PATCH"     :
                WebMethod == HTTP_DELETE  ? "DELETE"    :
                WebMethod == HTTP_OPTIONS ? "OPTIONS"   :
                "UNHANDLED" ;
}

// -----------------------------------------------------< /GetWebMethodText >---



// -----------------------------------------------------------------------------
// ---------------------------------------------------------------< Send404 >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Return 404 (not found)  error page to the web server client.
//
// PARAMETERS: WebServerh - Handle to the web server.
//
// RETURNS:    void
//
// NOTES:      -  HTTP request methods other than those listed below are
//                possible, but the ESP8266 web server only has a method ID
//                for those, so any others are simply shown as unhandled.  In
//                any case, only 'GET' and 'POST' are really expected.
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSV - Initial implementation.
//
// -----------------------------------------------------------------------------

static void Send404 ( ESP8266WebServer* WebServerh )
{
   // Send a standard 404 (Not Found) error message to web server client.

   String      Message;
   String      Method;
   int         ArgCount = WebServerh->args();

   GetWebMethodText ( WebServerh, Method );

   Message  = "<html> \n";
   Message += "<head>\n<title>404: File not found</title>\n</head> \n";
   Message += "<body> \n";
   Message += "<h1>404: File not found</h1>\n<hr><br> \n";
   Message += "The requested URL \"" + WebServerh->uri() + "\" was not found on this server. <br> \n",
              Message += "Method was ";
   Message += Method;
   Message += " with " + (String) ArgCount + " arguments <br> \n";
   for ( int i = 0; i < ArgCount; i++ )
   {
      Message += "&nbsp;&nbsp;&nbsp;" + (String) i + ") " + WebServerh->argName( i ) + " = " + WebServerh->arg( i ) + "<br> \n";
   }
   Message += "</body> \n";
   Message += "</html> \n";
   WebServerh->send ( 404, "text/html", Message );
}

// --------------------------------------------------------------< /Send404 >---



// -----------------------------------------------------------------------------
// -----------------------------------------------------------< GetFileSize >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Return the number of bytes in a specified SPIFFS file.
//
// PARAMETERS: FileSys - A reference to the file system handle.
//
//             FilePath - The path and name of the file to be get the size of.
//
// RETURNS:    int >= 0 : The number of bytes read an loaded into memory variable.
//                 <  0 : The specified file does not exist.
//
// NOTES:
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSV - Initial implementation.
//
// -----------------------------------------------------------------------------

int GetFileSize (
   fs::FS&     FileSys,
   const char* FilePath
)
{
   int FileSize = -1;

   if ( FileSys.exists ( FilePath ) )
   {
      File FileHandle = FileSys.open ( FilePath, "r" );
      FileSize = FileHandle.size();
      FileHandle.close();
   }
   return FileSize;
}

// ----------------------------------------------------------< /GetFileSize >---



// -----------------------------------------------------------------------------
// --------------------------------------------------------------< LoadFile >---
// -----------------------------------------------------------------------------
//
// PURPOSE:    Read a SPIFFS file and store its contents in a memory variable.
//
// PARAMETERS: FileSys - A reference to the file system handle.
//
//             FilePath - The path and name of the file to be loaded.
//
//             FileContent - A reference to the memory variable to load into.
//
// RETURNS:    int == The number of bytes read an loaded into memory variable.
//
// NOTES:
//
// HISTORY:
// --------- ---------- - ------------------------------------------------------
// 30Nov2018 DSV - Initial implementation.
//
// -----------------------------------------------------------------------------

int LoadFile (
   fs::FS&     FileSys,
   const char* FilePath,
   String&     FileContent
)
{
   int ReadSize = 0;

   if ( FileSys.exists ( FilePath ) )
   {
      File FileHandle = FileSys.open ( FilePath, "r" );

      if ( FileHandle )
      {
         int FileSize = FileHandle.size();

         if ( FileSize > 0 )
         {
            while ( FileHandle.available() )
            {
               FileContent += char ( FileHandle.read() );
               ReadSize++;
            }
         }

         FileHandle.close();
      }
   }

   return ReadSize;
}

// -------------------------------------------------------------< /LoadFile >---
