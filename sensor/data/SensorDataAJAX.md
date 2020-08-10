# WARNING - OBSOLETE

Any external web pages that display the sensor data are now required to used web sockets to get the data from the sensor's web server.  Prior to that, javascript in the web page was executed periodically based on a timer, and used AJAX to request that the data be returned to it (a polling arrangement). 

This page exists simply to preserve that old AJAX code in case it should be needed again for some reason, however unlikely - web sockets are great!


# Using AJAX to retrieve the current temperature value

## Getting the current temperature

There is more than one way to retrieve the current temperature value from the sensor system.
* It can be seen on a web page provided by the web server running on the sensor system.
* It can be retrieved with AJAX on a web page surfaced by an external web server and displayed as desired.

## Javascript & AJAX

This is the javascript to retrieve the sensor temperature value using AJAX, by requesting the web server to send "temperature.txt", which doesn't actually exist on the server.

```html

   <script>
      var x = setInterval ( function(){ loadData ( "temperature.txt", updateData ) }, 10000 );

      function loadData ( url, callback )
      {
         var xhttp = new XMLHttpRequest();
         xhttp.onreadystatechange = function()
         {
            if ( this.readyState == 4 && this.status == 200 )
            {
               callback.apply ( xhttp );
            }
         };
         xhttp.open ( "GET", url, true );
         xhttp.send();
      }

      function updateData()
      {
         document.getElementById ( "SensorValue" ).innerHTML = this.responseText;
      }
   </script>

```

The sensor system web server was configured with an event that was triggered to fire when the client requested "temperature.txt".  That event caused an internal text string containing the current temperature value to be returned instead of the requested file:

```C++

   WebServerh->on ( "/temperature.txt", [ WebServerh ]()
   {
      // Send the temperature result string.
      WebServerh->send ( 200, "text/html", SensorValueString );
   });

```

The internal text string holding the temperature data was a global variable that was populated in the "SensorAction" function.  The temperature probe was read and the string filled out with the latest data.  It is this same funciton that now broadcasts the data to any connected web sockets.  In fact, prior to dropping the AJAX support, the routine did both.

   char  SensorValueString[16] = { 0 };
   int   SensorValueStringLength;

   void SensorAction ( void* Args )
   {
      float SensorValueC;
      float SensorValueF;
      bool  DeviceState;

      ...

      SensorValueStringLength = sprintf ( SensorValueString, "%.1f °F", SensorValueF );

      ...
   }



Here's the entire former content of the SensorData.html file running on an external web server:

```html

<!DOCTYPE html>
<html>
<head>
   <title>ESP8266 Sensor Data</title>
   <meta http-equiv="Content-Type" content="text/html; charset=US-ASCII">
   <meta name="viewport" content="width=device-width, initial-scale=1.0"/>
   <link rel="StyleSheet" href="ESP8266.css" type="text/css" media="screen">
   <script>
      var x = setInterval ( function(){ loadData ( "temperature.txt", updateData ) }, 10000 );
     
      function loadData ( url, callback )
      {
         var xhttp = new XMLHttpRequest();
         xhttp.onreadystatechange = function()
         {
            if ( this.readyState == 4 && this.status == 200 )
            {
               callback.apply ( xhttp );
            }
         };
         xhttp.open ( "GET", url, true );
         xhttp.send();
      }

      function updateData()
      {
         document.getElementById ( "SensorValue" ).innerHTML = this.responseText;
      }
   </script>

</head>

<body>

   <center>

   <hr>
   <h2>ESP8266 Sensor Data</h2>
   <hr>

   <br><br>

   <table border="0" cellspacing="0" cellpadding="3" style="width:550px" >

      <tr><td class="section"><br>Temperature Reading<br><br></td></tr>

      <tr><td><b>Data: </b> <span id="SensorValue">No reading ...</span></td></tr>
   </table>

   <br><br>

   <hr>
   <script language="javascript" src="Footer.js"></script>
   </center>

</body>

</html>

```
