#ifndef __MC60_H__
#define __MC60_H__

#include "Serial_Command_Handler.h"

typedef enum
{
    NOT_REGISTERED = 0,
    REGISTERED = 1,
    SEARCHING = 2,
    REGISTRATION_DENIED = 3,
    UNKNOWN = 4,
    REGISTERED_ROAMING = 5,
    INVALID_CODE = 6
} registation_codes;

/**************************************************************************/
/*!
    @brief The MC60 Class
*/
/**************************************************************************/
class MC60 : public Serial_Command_Handler
{
public:
    uint8_t begin(uint32_t baud, uint8_t pin);
    bool initialize(uint32_t mc60_baud, bool autoBaud = false);
    bool initializeSMS();
    bool initializeGPS();

#ifdef USE_SW_SERIAL
    MC60(SoftwareSerial *ser);
#endif
    MC60(HardwareSerial *ser);
    MC60();
    ~MC60();

    bool powerUp(uint8_t pin);
    bool powerDown(bool urgent = false, uint8_t pin = 255);

    String getManufacturerID(void);
    String getModule(void);
    String getVersion(void);
    registation_codes getNetworkRegistration(void);
    registation_codes getGPRSRegistration(void);
    String getOperatorName(void);
    String getIMSI(void);
    String getICCID(void);

    bool sendSMS(const char *number, const char *message);
    bool sendSMS(String number, const char *message);
    bool sendSMS(const char *number, String message);
    bool sendSMS(String number, String message);

    bool readGPS(bool signedCoordinates = false);
    String getGGASentence();

    bool gpsFix();

    uint8_t hour; ///< Hour in 24-hour format
    uint8_t minute; ///< Minute
    uint8_t second; ///< Second
    uint8_t millisecond; ///< Millisecond
    int8_t latitude_degrees; ///< Latitude in degrees (integer)
    uint8_t latitude_minutes; ///< Latitude in minutes (integer)
    double latitude_seconds; ///< Latitude in seconds (double)
    char latitude_direction; ///< Latitude (N/S)
    int16_t longitude_degrees; ///< Longitude in degrees (integer)
    uint8_t longitude_minutes; ///< Longitude in minutes (integer)
    double longitude_seconds; ///< Longitude in seconds (double)
    char longitude_direction; ///< Longitude (E/W)
    uint8_t fix_type; ///< Fix type (0 = No fix, 1 = GPS fix, 2 = Differential GPS fix, 4 = RTK Fix, 5 = RTK Float, 6 = Dead reckoning, 7 = Manual input, 8 = Simulator)
    uint8_t number_of_satellites; ///< Number of satellites being tracked
    float horizontal_dilution; ///< Horizontal dilution
    float altitude; ///< Altitude in meters
    float geoidal_separation; ///< Geoidal separation in meters
    uint32_t age_of_differential; ///< Age of differential GPS data (seconds)
    uint32_t differential_reference_station_id; ///< Differential reference station ID

private:
    String getValue(String data, char separator, int index);

    bool began = false;
    bool connected = false;
    uint8_t initialization = 0;

    bool smsInitialized = false;
    bool gpsInitialized = false;
};

#endif