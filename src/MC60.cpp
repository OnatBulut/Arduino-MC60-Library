#include "MC60.h"

/**************************************************************************/
/*!
    @brief Start the HW or SW serial port, power up and initialize MC60
    @param baud Baud rate for serial communication
    @param pin Pin to control power to MC60
    @returns 0 on failure, 1 on power up without initialization, 2 on complete success
*/
/**************************************************************************/
uint8_t MC60::begin(uint32_t baud, uint8_t pin)
{
    if (!began)
    {
        Serial_Command_Handler::begin(baud);
        pinMode(pin, OUTPUT);

        if (powerUp(pin))
        {
            began = true;
            if (initialize(baud))
                return 2;
            else
                return 1;
        }
    }
    else
    {
        if (initialize(baud))
            return 2;
        else
            return 1;
    }

    return 0;
}

/**************************************************************************/
/*!
    @brief Initialize MC60
    @param mcbaud Baud rate for serial communication
    @param autoBaud Set baud rate to auto (optional, default = false)
    @returns False on failure, true on success
*/
/**************************************************************************/
bool MC60::initialize(uint32_t mc60Baud, bool autoBaud)
{
    initialization = 0;

    if (!autoBaud)
    {
        char cmd[20];
        sprintf(cmd, "AT+IPR=%lu\r", mc60Baud);

        initialization += sendCommandWaitOK(cmd); ///< Set baud rate
    }
    else
        initialization += sendCommandWaitOK("AT+IPR=0\r"); ///< Set baud rate to auto

    initialization += sendCommandWaitOK("ATE1\r");       ///< Turn on echo
    initialization += sendCommandWaitOK("AT+IFC=2,2\r"); ///< Set flow control to hardware

    return initialization == 3; ///< If all commands were successful, return true
}

/**************************************************************************/
/*!
    @brief Initialize SMS
    @returns False on failure, true on success
*/
/**************************************************************************/
bool MC60::initializeSMS()
{
    if (smsInitialized)
        return true;

    initialization = 0;
    initialization += sendCommandWait("AT+CPIN?\r", "+CPIN: READY\r\n\r\nOK"); ///< Check SIM card
    initialization += sendCommandWaitOK("AT+CMGF=1\r");                        ///< Set SMS to text mode
    initialization += sendCommandWaitOK("AT+CSCS=\"GSM\"\r");                  ///< Set SMS to GSM mode

    return smsInitialized = initialization == 3; ///< If all commands were successful, return true
}

/**************************************************************************/
/*!
    @brief Initialize GPS
    @returns False on failure, true on success
*/
/**************************************************************************/
bool MC60::initializeGPS()
{
    if (gpsInitialized)
        return true;

    if (sendCommandWait("AT+QGNSSC?\r", "+QGNSSC: 0\r\n\r\nOK"))    ///< Check if GNSS is disabled
        return gpsInitialized = sendCommandWaitOK("AT+QGNSSC=1\r"); ///< Enable GNSS
    else
        return gpsInitialized = sendCommandWait("AT+QGNSSC?\r", "+QGNSSC: 1\r\n\r\nOK"); ///< Check if GNSS is enabled
}

/**************************************************************************/
/*!
    @brief Constructor when using SoftwareSerial
    @param ser Pointer to SoftwareSerial device
*/
/**************************************************************************/
#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
MC60::MC60(SoftwareSerial *ser) : Serial_Command_Handler(ser)
{
}
#endif

/**************************************************************************/
/*!
    @brief Constructor when using HardwareSerial
    @param ser Pointer to a HardwareSerial object
*/
/**************************************************************************/
MC60::MC60(HardwareSerial *ser) : Serial_Command_Handler(ser) {}

/**************************************************************************/
/*!
    @brief Constructor when there are no communications attached
*/
/**************************************************************************/
MC60::MC60() : Serial_Command_Handler() {}

MC60::~MC60() = default;

/**************************************************************************/
/*!
    @brief Power up the module
    @param pin PWRKEY pin
    @returns True on success, False on failure
*/
/**************************************************************************/
bool MC60::powerUp(uint8_t pin)
{
    if (sendAT(1000)) ///< Wait for AT response
        return true;

    digitalWrite(pin, HIGH);
    delay(1200);
    digitalWrite(pin, LOW);
    delay(800);

    for (int i = 0; i < 10; i++) ///< Try to connect 10 times
        if (sendAT(500))         ///< Wait for AT response
            return true;

    return false;
}

/**************************************************************************/
/*!
    @brief Power down the module
    @param urgent Power down immediately if true (optional, default = false)
    @param pin PWRKEY pin, used instead of software shutdown if used (optional, default = 255)
    @returns True on success, False on failure or if the module is already off
*/
/**************************************************************************/
bool MC60::powerDown(bool urgent, uint8_t pin)
{
    if (!connected)
        return false;

    if (pin != 255)
    {
        digitalWrite(pin, HIGH);
        delay(1200);
        digitalWrite(pin, LOW);
        if (waitForResponse("NORMAL POWER DOWN"))
        {
            connected = false;
            return true;
        }
    }

    if (urgent)
        if (sendCommandWaitOK("AT+QPOWD=0\r", 300))
        {
            connected = false;
            return true;
        }
        else if (sendCommandWait("AT+QPOWD=1\r", "NORMAL POWER DOWN", 300))
        {
            connected = false;
            return true;
        }

    return false;
}

/**************************************************************************/
/*!
    @brief Get manufacturer ID
    @returns Manufacturer ID
*/
/**************************************************************************/
String MC60::getManufacturerID(void)
{
    if (sendCommandWait("ATI\r", "ATI\r\r\n", 300))
    {
        String manufacturer_ID = readline();
        (void)waitForOK();
        return manufacturer_ID;
    }
    return "";
}

/**************************************************************************/
/*!
    @brief Get module name
    @returns Module name
*/
/**************************************************************************/
String MC60::getModule(void)
{
    if (sendCommandWait("ATI\r", "ATI\r\r\n", 300))
    {
        (void)readline();
        String module = readline();
        (void)waitForOK();
        return module;
    }
    return "";
}

/**************************************************************************/
/*!
    @brief Get firmware version
    @returns Firmware version
*/
/**************************************************************************/
String MC60::getVersion(void)
{
    if (sendCommandWait("ATI\r", "ATI\r\r\n", 300))
    {
        (void)readline();
        (void)readline();
        (void)waitForResponse("Revision: ");
        String version = readline();
        (void)waitForOK();
        return version;
    }
    return "";
}

/**************************************************************************/
/*!
    @brief Get network registration status
    @returns Network registration status
*/
/**************************************************************************/
registation_codes MC60::getNetworkRegistration(void)
{
    if (sendCommandWait("AT+CREG?\r", "+CREG: ", 300))
    {
        char *rb = readbetween(',', '\r');
        (void)waitForOK();
        return (abs(rb[0] - 48) <= 5 ? (registation_codes)(rb[0] - 48) : registation_codes::INVALID_CODE);
    }
    return registation_codes::INVALID_CODE;
}

/**************************************************************************/
/*!
    @brief Get GPRS registration status
    @returns GPRS registration status
*/
/**************************************************************************/
registation_codes MC60::getGPRSRegistration(void)
{
    if (sendCommandWait("AT+CGREG?\r", "+CGREG: ", 300))
    {
        char *rb = readbetween(',', '\r');
        (void)waitForOK();
        return (abs(rb[0] - 48) <= 5 ? (registation_codes)(rb[0] - 48) : registation_codes::INVALID_CODE);
    }
    return registation_codes::INVALID_CODE;
}

/**************************************************************************/
/*!
    @brief Get operator name
    @returns Operator name
*/
/**************************************************************************/
String MC60::getOperatorName(void)
{
    if (sendCommandWait("AT+COPS?\r", "+COPS: ", 300))
    {
        String operator_name = readbetween('"', '"');
        (void)waitForOK();
        return operator_name;
    }
    return "";
}

/**************************************************************************/
/*!
    @brief Get IMSI
    @returns IMSI
*/
/**************************************************************************/
String MC60::getIMSI(void)
{
    if (sendCommandWait("AT+CIMI\r", "AT+CIMI\r\r\n", 300))
    {
        String IMSI = readline();
        (void)waitForOK();
        return IMSI;
    }
    return "";
}

/**************************************************************************/
/*!
    @brief Get ICCID
    @returns ICCID
*/
/**************************************************************************/
String MC60::getICCID(void)
{
    if (sendCommandWait("AT+QCCID\r", "AT+QCCID\r\r\n", 300))
    {
        String ICCID = readline();
        (void)waitForOK();
        return ICCID;
    }
    return "";
}

/**************************************************************************/
/*!
    @brief Send SMS
    @param number Phone number to send SMS to
    @param message Message to send
    @returns True on successful send, False on failure
*/
/**************************************************************************/
bool MC60::sendSMS(const char *number, const char *message)
{
    if (!initializeSMS())
        return false;

    char cmd[20];
    sprintf(cmd, "AT+CMGS=\"%s\"\r", number);

    if (sendCommandWait(cmd, "> ", 300))
    {
        write(message);
        sendEndMarker();
        (void)waitForResponse("+CMGS: ");
        (void)waitForOK();
    }
    else
        return false;

    return true;
}

/**************************************************************************/
/*!
    @brief Send SMS
    @param number Phone number to send SMS to
    @param message Message to send
    @returns True on successful send, False on failure
*/
/**************************************************************************/
bool MC60::sendSMS(String number, const char *message)
{
    return sendSMS(number.c_str(), message);
}

/**************************************************************************/
/*!
    @brief Send SMS
    @param number Phone number to send SMS to
    @param message Message to send
    @returns True on successful send, False on failure
*/
/**************************************************************************/
bool MC60::sendSMS(const char *number, String message)
{
    return sendSMS(number, message.c_str());
}

/**************************************************************************/
/*!
    @brief Send SMS
    @param number Phone number to send SMS to
    @param message Message to send
    @returns True on successful send, False on failure
*/
/**************************************************************************/
bool MC60::sendSMS(String number, String message)
{
    return sendSMS(number.c_str(), message.c_str());
}

// TODO: Read NMEA/GGA parse and write to a struct

// > AT+QGNSSRD="NMEA/GGA"
// +QGNSSRD: $GNGGA,000654.095,,,,,0,0,,,M,,M,,*5D

// OK

/**************************************************************************/
/*!
    @brief Get GPS parameters
    @param signedCoordinates Set to true to get signed coordinates, false to use N-S / E-W instead (optional, default = false)
    @returns True on successful read, False on failure
*/
/**************************************************************************/
bool MC60::readGPS(bool signedCoordinates)
{
    if (!initializeGPS())
        return false;

    if (sendCommandWait("AT+QGNSSRD=\"NMEA/GGA\"\r", "+QGNSSRD: ", 300))
    {
        String ggaString = readline();
        (void)waitForOK();

        hour = getValue(ggaString, ',', 1).substring(0, 2).toInt();
        minute = getValue(ggaString, ',', 1).substring(2, 4).toInt();
        second = getValue(ggaString, ',', 1).substring(4, 6).toInt();
        millisecond = getValue(ggaString, ',', 1).substring(7, 9).toInt();

        double decimalDegreeMinuteLat = getValue(ggaString, ',', 2).toDouble();
        latitude_direction = getValue(ggaString, ',', 3)[0];
        latitude_degrees = decimalDegreeMinuteLat / 100;
        latitude_degrees *= latitude_direction == 'N' ? 1 : -1;
        latitude_minutes = (uint32_t)decimalDegreeMinuteLat % 100;
        latitude_seconds = fmod(decimalDegreeMinuteLat, 1) * 60;

        double decimalDegreeMinuteLon = getValue(ggaString, ',', 4).toDouble();
        longitude_direction = getValue(ggaString, ',', 5)[0];
        longitude_degrees = decimalDegreeMinuteLon / 100;
        longitude_degrees *= longitude_direction == 'E' ? 1 : -1;
        longitude_minutes = (uint32_t)decimalDegreeMinuteLon % 100;
        longitude_seconds = fmod(decimalDegreeMinuteLon, 1) * 60;

        fix_type = getValue(ggaString, ',', 6).toInt();
        number_of_satellites = getValue(ggaString, ',', 7).toInt();
        horizontal_dilution = getValue(ggaString, ',', 8).toFloat();
        altitude = getValue(ggaString, ',', 9).toFloat();
        geoidal_separation = getValue(ggaString, ',', 11).toFloat();
        age_of_differential = getValue(ggaString, ',', 13).toInt();
        differential_reference_station_id = getValue(ggaString, ',', 14).toInt();

        return true;
    }
    return false;
}

/**************************************************************************/
/*!
    @brief Check if GPS has a fix
    @returns True if GPS has a fix, False if not
*/
/**************************************************************************/
bool MC60::gpsFix()
{
    return (bool)fix_type;
}

/**************************************************************************/
/*!
    @brief Get GGA sentence
    @returns GGA sentence
*/
/**************************************************************************/
String MC60::getGGASentence()
{
    if (!initializeGPS())
        return "";

    if (sendCommandWait("AT+QGNSSRD=\"NMEA/GGA\"\r", "+QGNSSRD: ", 300))
    {
        String ggaString = readline();
        (void)waitForOK();

        return ggaString;
    }
    return "";
}

/**************************************************************************/
/*!
    @brief Get value from string
    @param data String to get value from
    @param separator Separator character
    @param index Index of value to get
    @returns Value at index
*/
/**************************************************************************/
String MC60::getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++)
    {
        if (data.charAt(i) == separator || i == maxIndex)
        {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }

    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}