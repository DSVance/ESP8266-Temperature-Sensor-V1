// =============================================================================
//
// NAME:       TemperatureData
//
// PURPOSE:    Receive and display temperature data over a web-socket connection. 
//
// LANGUAGE:   Javascript
//
// NOTES:      -  Sensor web interface version 2.
//
//             -  This source file in conjunction with SensorData.html implements 
//                recieving temperature data messages over a web-socket connection. 
// 
//             -  When connected to a NodeMCU/ESP8266 running a websocket server 
//                that transmits temperature data, the web page will automatically 
//                update the displayed data value in real time, without polling.
//             
//             -  WARNING: The web socket created in this function needs an IP 
//                address and port number for the web socket server it should 
//                connect to.  To allow this code to be generic and work on 
//                multiple sensor devices, the IP address in this code is just a 
//                place holder of the form  "ws://w.x.y.z:p".  That text will be 
//                replaced with the real IP address of the sensor, by the web 
//                server on the sensor, before this javascript file is returned 
//                to the browser/client.
//
// HISTORY:
// ========= =========== = =====================================================
//
// =============================================================================

function Connect()
{
   if ( ! ( 'WebSocket' in window ) )
   {
      alert ( "ERROR: Web sockets are required for this application, but are not supported." );
   } 
   
   else 
   {
      var status = google.charts.load('current', {'packages':['gauge']});
      

      google.charts.setOnLoadCallback ( drawChart );

      function drawChart( t = 0 ) 
      {
         var GuageData = google.visualization.arrayToDataTable([
            ['Label', 'Value'],
            ['Temp', t],
         ]);

         var GuageOptions =
         {
            width:      350,
            height:     350,
            min:          0,
            max:        120,
            yellowFrom:  90,
            yellowTo:   105,
            redFrom:    105,
            redTo:      120,
            majorTicks: ["0", "20", "40", "60", "80", "100", "120"],
            minorTicks:   4
         };

         var chart = new google.visualization.Gauge(document.getElementById('chart_div'));

         chart.draw ( GuageData, GuageOptions );
      }



      // Get references to elements on the page.
      var form          = document.getElementById('action-form');
      var DataField     = document.getElementById('data');
      var SocketStatus  = document.getElementById('status');
      var CloseButton   = document.getElementById('close');
      var ConnectButton = document.getElementById('connect');
   
      var ws = new WebSocket ( "ws://w.x.y.z:p" );

   
      //
      // Show a connected message when the WebSocket is opened.
      //
      ws.onopen = function ( event )
      {
         console.log ( 'Connecting to '+ event.currentTarget.url  );
         SocketStatus.innerHTML = 'Connecting to ' + event.currentTarget.url;

         var State = ws.readyState;
                  
         console.log ( 'Socket ready state = ' + State );

         if ( State == 1 )
         {         
            SocketStatus.className = 'open';
            console.log ( 'Connected to '+ event.currentTarget.url  );
            SocketStatus.innerHTML = 'Connected to ' + event.currentTarget.url;
         }
      };
   
   
      //
      // Handle any errors that occur.
      //
      ws.onerror = function ( error )
      {
         console.log ( 'WebSocket Error: ' + error.message );
         SocketStatus.innerHTML = 'Error: ' + error.message;
         DataField.innerHTML = 'No Reading ...';
         drawChart ();
      }
   
   
   
      //
      // Handle messages sent by the server.
      //
      ws.onmessage = function ( event )
      {
         console.log ( 'Data = ' + event.data );
         var JSONData = JSON.parse ( event.data );
         JSONData.Value =  ( Math.round ( JSONData.Value * 10 ) / 10 );
         DataField.innerHTML = JSONData.Value + " &deg;" + JSONData.Units;
         drawChart ( JSONData.Value );

      };
   
   
      //
      // Show a disconnected message when the WebSocket is closed.
      //
      ws.onclose = function ( event )
      {
         console.log ( 'Connection to '+ event.currentTarget.url + ' closed'  );
         SocketStatus.innerHTML = 'WebSocket ' + event.currentTarget.url + ' disconnected';
         SocketStatus.className = 'closed';
         DataField.innerHTML = 'No Reading ...';
         drawChart ();
      };
   
   
      //
      // Close the WebSocket connection when the close button is clicked.
      //
      CloseButton.onclick = function ( event )
      {
         event.preventDefault();
   
         // Close the WebSocket.
         ws.close();
   
         return false;
      };
   
   
      ConnectButton.onclick = function ( event )
      {
         event.preventDefault();
         
         Connect();
   
         return false;
      };
   }
};


//
// An error message for instances where the content of a javascript tag does
// not load as expected.  Invoke this function with an 'onerror' handler:
// <script language="javascript" src="nonexist.js" onerror="NoScript();"></script>   
//

function NoScript ()
{
   alert ( "ERROR: Failed to load the necessary javascript objects.  An internet "
         + "connection is required for this capability and one does not appear "
         + "to be available.  This functionality will not work until an internet " 
         + "connection can be established."
         );
}
