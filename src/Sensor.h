
#ifndef SENSOR_COMMON_DEFS

// -----------------------------------------------------------------------------
// ----------------------------------------------------------< Sensor.h >-------
// -----------------------------------------------------------------------------
//
// PROJECT: Arduino development for the ESP8266 custom temperature sensor board.
//
// PURPOSE: Commonly used general purpose definitions in the Sensor sketch.
//
// AUTHOR:  Scott Vance
//
// NOTES:   
//
// HISTORY:
// --------- ----------- - ----------------------------------------------------
// 13Oct2018 Scott Vance - Initial development.
//
// -----------------------------------------------------------------------------


#define SENSOR_VERSION_MAJOR     1
#define SENSOR_VERSION_MINOR     5
#define SENSOR_VERSION_DATE      __DATE__



#define DEBUG_PRINTF(f,...)                                             \
        if ( ((PConfig_t*)(f))->Flags & CONFIG_DEBUG_MESSAGE_ENABLED )  \
        { Serial.printf ( __VA_ARGS__ ); }

#define DEBUG_PRINTLN(f,...)                                            \
        if ( ((PConfig_t*)(f))->Flags & CONFIG_DEBUG_MESSAGE_ENABLED )  \
        { Serial.println ( __VA_ARGS__ ); }


#define DEFAULT_BAUD    115200


#endif   // SENSOR_COMMON_DEFS
