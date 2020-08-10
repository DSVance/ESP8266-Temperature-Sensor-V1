// =============================================================================
//
// NAME:       PageSaveDate
//
// PURPOSE:    Displays the day of the week and the calendar date that the current
//             page was last saved, in standard US full-date format.
//
// LANGUAGE:   Javascript
//
// NOTES:
//
// EXAMPLE:    Put a line similar to the following one, anywhere in your HTML
//             docuemnt that you want the "Last updated" message to be displayed.
//
//             <SCRIPT LANGUAGE="JAVASCRIPT" SRC="PageSaveDate.js"> </SCRIPT>
//
//             Last updated on Tuesday, November 10, 1998
//
// HISTORY:
// ========= =========== = =====================================================
// 21Nov2000 Scott Vance - Added special case suffix handler for 21st, 22nd, 23rd, 31st.
// 18Jul2008 Scott Vance - Added time stamp to last update display.
//
// =============================================================================

   MonthName = new Array ( 'January',
                           'February',
                           'March',
                           'April',
                           'May',
                           'June',
                           'July',
                           'August',
                           'September',
                           'October',
                           'November',
                           'December'
                         );

   DayName = new Array ( 'Sunday',
                         'Monday',
                         'Tuesday',
                         'Wednesday',
                         'Thursday',
                         'Friday',
                         'Saturday'
                       );

   DateSuffix = new Array ( 'st',
                            'nd',
                            'rd',
                            'th'
                          );


   var MessageText = "Last Updated on ";

   var Result;


   // Get the file's save date from the operating system
   // and store the individual date componenets in an array.
   var DateSplit = new Array();
   var LMDate = new Date (document.lastModified);
   DateSplit = document.lastModified.split (" ");

   // Extract or construct the individual date elements.
   WeekDay  = DayName[LMDate.getDay(DateSplit)];
   Month    = MonthName[LMDate.getMonth (DateSplit)];
   MonthDay = LMDate.getDate (DateSplit);
   Year     = LMDate.getYear (DateSplit);
   Hours    = '0' + LMDate.getHours (DateSplit);
   Minutes  = '0' + LMDate.getMinutes (DateSplit);


   // alert ( "DEBUG PageSaveDate: \nWeekDay = " + WeekDay + "\nMonth = " + Month + "\nMonthDay = " + MonthDay + "\nYear = " + Year );

   // Determine the month day's suffix.
   if (MonthDay > 0 && MonthDay < 4)
   {
       // Special cases: day is 1st, 2nd or 3rd
       Suffix = DateSuffix[MonthDay-1];
   }

   else if (MonthDay != 30 && MonthDay > 20 && MonthDay%10 < 4)
   {
       // Special cases: day is 21st, 22nd, 23rd, or 31st
       Suffix = DateSuffix[(MonthDay%10)-1];
   }

   else
   {
       // All other days not handled above end in 'th'
       Suffix = DateSuffix[3];
   }

   // alert ( "DEBUG PageSaveDate: Suffix = " + Suffix );

   // Compensate for the various formats that different machines return the
   // year value of the date in.  Some return 2 digits only, some a value
   // referenced from 1900, and some a full four digit year value.
   //
   // If the year value
   //    is < 0 it must be an error.
   //    is > 1900 it must already be a four digit year.
   //    is < 50 it must be a two digit year in the 2000's
   //    is < 1900 and > 50 it must be referenced from 1900's.

   if (Year > 0)
   {
      if ( Year < 1900 )
      {
         // The value is not already in a four digit format.

         if ( Year < 50 )
         {
            // The year value only has two digits, but it is
            // less that 50, so assume it to be in the 2000s.
            Year += 2000;
         }

         else
         {
            // The year is value is either a two digit year so assume it to be
            // in the 1900s, or it is already be Y2K compensated, and is a value
            // referenced from 1900.  In either case, add 1900 to the value.
            //
               Year += 1900;
         }
      }
   }

   else
   {
       // The year value is negative! Not likely!
       // Must be an error so simply blank-out the year value.
       Year = "";
   }

   // alert ( "DEBUG PageSaveDate: Year = " + Year );

   // Display the message text along with the date in standard US format.
   Result = ( "<div class=\"savedate\">" +
              MessageText +
              WeekDay + ", " +
              Month + " " +
              MonthDay +
              Suffix + ", " +
              Year + " " +
              Hours.slice ( -2 ) + ":" + Minutes.slice ( -2 ) +
              "</div>"
            );

   // Display the message text along with the date in standard US format.
 document.write ( Result );

// === End of JavaScript Applet ================================================

