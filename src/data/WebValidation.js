// =============================================================================
//
// NAME:       WebValidation 
//
// PURPOSE:    Data validation for the ESP8266 configuration pages.
//
// LANGUAGE:   Javascript
//
// NOTES:      -  Sensor web interface version 2.
//
// HISTORY:
// ========= =========== =======================================================
// 12Nov2018 Scott Vance - Initial development.
//
// =============================================================================


function CheckPort ( SelectedItem )
{
   if ( SelectedItem.value != 80 )
   {
      alert ( 'WARNING: Be careful about using anything other than the standard port 80 '
            + 'for the web server!  This configuration page may not work properly on that '
            + 'port, so you may not be able to change it again.  In that case, the system '
            + 'firmware will have to be reflashed to correct the problem.'
            );
   }
}


function FocusTo ( Item ) 
{
   Item.focus();
   return;
}



function ValidateIP ( SelectedItem )
{
   if ( SelectedItem.id == "ap_0" || SelectedItem.id == "gw_0" )
   {
      if ( SelectedItem.value  !=  10
         && SelectedItem.value != 172
         && SelectedItem.value != 192
         )
      {
         alert ( 'ERROR: The access point and gateway IP addresses must be in a range used for private '
               + 'networks.  The private network addresses must start with 10, 172, or 192.'
               );
      }

      else if ( SelectedItem.value == 192 )
      {
         if ( SelectedItem.id == "ap_0" )
         {
            document.getElementById("ap_1").value = 168;
         }
         
         else if ( SelectedItem.id == "gw_0" )
         {
            document.getElementById("gw_1").value = 168;
         }
      }
   }

   else 
   {
      var SubNet0;
      
      if ( SelectedItem.id == "ap_1" )
      {         
         SubNet0 = document.getElementById("ap_0").value;
      }
      
      else
      {
         SubNet0 = document.getElementById("gw_0").value;         
      }
      
      if (  ( SubNet0 == 172 )
         && ( SelectedItem.value < 16 || SelectedItem.value > 31 )
         )

      {
         alert ( 'ERROR: The second subnet for an access point IP addresses starting '
               + 'with 172 must be between 16 and 31 inclusive.'
               );
      }

      else if (  ( SubNet0 == 192 )
              && ( SelectedItem.value != 168 )
              )
      {
         alert ( 'ERROR: The second subnet for an access point IP addresses starting '
               + 'with 192 must be 168.'
               );
      }
   }
}


function ValidateLabel ( SelectedItem )
{
   if ( SelectedItem.textLength > 31 )
   {
      alert ( 'ERROR: The maximum label length is 31 characters.' );
      return false;
   }
}
