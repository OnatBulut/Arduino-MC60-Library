#include "Serial_Command_Handler.h"

/**************************************************************************/
/*!
    @brief Start the HW or SW serial port
    @param baud Baud rate for serial communication
*/
/**************************************************************************/
void Serial_Command_Handler::begin(uint32_t baud)
{
#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
    if (SwSerial)
        SwSerial->begin(baud);
#endif
    if (HwSerial)
        HwSerial->begin(baud);
}

/**************************************************************************/
/*!
    @brief Constructor when using SoftwareSerial
    @param ser Pointer to SoftwareSerial device
*/
/**************************************************************************/
#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
Serial_Command_Handler::Serial_Command_Handler(SoftwareSerial *ser)
{
    common_init();
    SwSerial = ser;
}
#endif

/**************************************************************************/
/*!
    @brief Constructor when using HardwareSerial
    @param ser Pointer to a HardwareSerial object
*/
/**************************************************************************/
Serial_Command_Handler::Serial_Command_Handler(HardwareSerial *ser)
{
    common_init();
    HwSerial = ser;
}

/**************************************************************************/
/*!
    @brief Constructor when there are no communications attached
*/
/**************************************************************************/
Serial_Command_Handler::Serial_Command_Handler()
{
    common_init();
    noComms = true;
}

Serial_Command_Handler::~Serial_Command_Handler() = default;

/**************************************************************************/
/*!
    @brief Initialization code used by all constructor types
*/
/**************************************************************************/
void Serial_Command_Handler::common_init(void)
{
#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
    SwSerial = NULL;
#endif
    HwSerial = NULL;
    lineidx = 0;
    paused = false;
}

/**************************************************************************/
/*!
    @brief How many bytes are available to read - part of 'Print'-class
   functionality
    @return Bytes available, 0 if none
*/
/**************************************************************************/
size_t Serial_Command_Handler::available(void)
{
    if (paused)
        return 0;

#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
    if (SwSerial)
        return SwSerial->available();
#endif
    if (HwSerial)
        return HwSerial->available();
    return 0;
}

/**************************************************************************/
/*!
    @brief Write a byte to the underlying transport - part of 'Print'-class
   functionality
    @param byte Byte to send
    @return Bytes written -  1 on success, 0 on failure
*/
/**************************************************************************/
size_t Serial_Command_Handler::write(uint8_t byte)
{
#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
    if (SwSerial)
        return SwSerial->write(byte);
#endif
    if (HwSerial)
        return HwSerial->write(byte);
    return 0;
}

/**************************************************************************/
/*!
    @brief Write a char array to the underlying transport - part of 'Print'-class
   functionality
    @param cmd Char array to send
    @return Bytes written - amount of characters written on success, 0 on failure
*/
/**************************************************************************/
size_t Serial_Command_Handler::write(const char *cmd)
{
#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
    if (SwSerial)
        return SwSerial->write(cmd);
#endif
    if (HwSerial)
        return HwSerial->write(cmd);
    return 0;
}

/**************************************************************************/
/*!
    @brief Write an String to the underlying transport - part of 'Print'-class
   functionality
    @param cmd String to send
    @return Bytes written - amount of characters written on success, 0 on failure
*/
/**************************************************************************/
size_t Serial_Command_Handler::write(String cmd)
{
    return write(cmd.c_str());
}

/**************************************************************************/
/*!
    @brief Flush the input buffer - part of 'Print'-class functionality
*/
/**************************************************************************/
void Serial_Command_Handler::flush(void)
{
#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
    while (SwSerial && available())
        SwSerial->read();
#endif
    while (HwSerial && available())
        HwSerial->read();
}

/**************************************************************************/
/*!
    @brief Read one character.
    @return The character that we received, or 0 if nothing was available
*/
/**************************************************************************/
char Serial_Command_Handler::read(void)
{
    char c = 0;

    if (paused || noComms)
        return 0;

#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
    if (SwSerial)
    {
        if (!SwSerial->available())
            return 0;
        c = SwSerial->read();
    }
#endif
    if (HwSerial)
    {
        if (!HwSerial->available())
            return 0;
        c = HwSerial->read();
    }

    return c;
}

/**************************************************************************/
/*!
    @brief Read one line.
    @param timeout How long to wait in milliseconds (optional)
    @param length Max characters to read (optional)
    @return The string that we received, or empty string if nothing was available
*/
/**************************************************************************/
String Serial_Command_Handler::readline(unsigned long timeout, uint8_t length)
{
    unsigned long startTime = millis();
    sbuffer = "";
    char c = 0;
    lineidx = 0;

    while (millis() - startTime < timeout)
    {
        if (available())
        {
            c = read();

            if (c == '\r')
                continue;

            if (c == '\n')
                return sbuffer;

            sbuffer += c;

            if (++lineidx >= length - 1)
                return sbuffer;
        }
    }
    return "";
}

/**************************************************************************/
/*!
    @brief Read one line.
    @param first The first character to match
    @param last The last character to match
    @param timeout How long to wait in milliseconds (optional)
    @param length Max characters to read (optional)
    @return The character that we received, or 0 if nothing was available
*/
/**************************************************************************/
char *Serial_Command_Handler::readbetween(const char first, const char last, unsigned long timeout, uint8_t length)
{
    unsigned long startTime = millis();
    cleanBuffer(buffer);
    char c = 0;
    lineidx = 0;

    bool firstEncounter = false;

    while (millis() - startTime < timeout)
    {
        if (available())
        {
            c = read();

            if (!firstEncounter && c == first)
                firstEncounter = true;
            else
                continue;

            if (c == last)
                return buffer;

            buffer[lineidx++] = c;

            if (lineidx >= length - 1)
                return buffer;
        }
    }
    return NULL;
}

/**************************************************************************/
/*!
    @brief Pause/unpause receiving new data
    @param p True = pause, false = unpause
*/
/**************************************************************************/
void Serial_Command_Handler::pause(bool p) { paused = p; }

/**************************************************************************/
/*!
    @brief Send "AT" to the device and wait for the response "OK"
    @param timeout How long to wait for a response in milliseconds (optional)
    @return True if we got an "OK", false otherwise
*/
/**************************************************************************/
bool Serial_Command_Handler::sendAT(unsigned long timeout)
{
    return sendCommandWaitOK("AT\r", timeout);
}

/**************************************************************************/
/*!
    @brief Send CTRL+Z to the device
*/
/**************************************************************************/
void Serial_Command_Handler::sendEndMarker(void) { write(26); }

/**************************************************************************/
/*!
    @brief Wait for the OK sentence from the device
    @param timeout How long to wait for a response in milliseconds (optional)
    @param length How many characters to wait (optional)
    @return True if we got what we wanted, false otherwise
*/
/**************************************************************************/
bool Serial_Command_Handler::waitForOK(unsigned long timeout, uint8_t length)
{
    unsigned long startTime = millis();
    lineidx = 0;

    while (lineidx < length)
    {
        if (available())
        {
            buffer[lineidx] = read();
            buffer[++lineidx] = '\0';

            if (strstr(buffer, "OK") != NULL)
            {
                cleanBuffer(buffer, lineidx);
                flush();
                return true;
            }
        }

        if (millis() - startTime >= timeout)
            break;
    }

    cleanBuffer(buffer, lineidx);
    flush();
    return false;
}

/**************************************************************************/
/*!
    @brief Wait for a specified sentence from the device
    @param wait4me Pointer to a string holding the desired response
    @param timeout How long to wait for a response in milliseconds (optional)
    @param length How many characters to wait (optional)
    @return True if we got what we wanted, false otherwise
*/
/**************************************************************************/
bool Serial_Command_Handler::waitForResponse(const char *wait4me, unsigned long timeout, uint8_t length)
{
    unsigned long startTime = millis();
    lineidx = 0;

    while (lineidx < length)
    {
        if (available())
        {
            buffer[lineidx] = read();
            buffer[++lineidx] = '\0';

            if (strstr(buffer, wait4me) != NULL)
            {
                cleanBuffer(buffer, lineidx);
                return true;
            }
        }

        if (millis() - startTime >= timeout)
            break;
    }

    cleanBuffer(buffer, lineidx);
    return false;
}

/**************************************************************************/
/*!
    @brief Wait for a specified sentence from the device
    @param wait4me String holding the desired response
    @param timeout How long to wait for a response in milliseconds (optional)
    @param length How many characters to wait (optional)
    @return True if we got what we wanted, false otherwise
*/
/**************************************************************************/
bool Serial_Command_Handler::waitForResponse(String wait4me, unsigned long timeout, uint8_t length)
{
    return waitForResponse(wait4me.c_str(), timeout, length);
}

/**************************************************************************/
/*!
    @brief Send an user defined command to the device and wait for the response
    @param cmd Pointer to a string holding the command to send
    @param response Pointer to a string holding the desired response
    @param timeout How long to wait for a response in milliseconds (optional)
    @param length How many characters to wait (optional)
    @return True if we got the response, false otherwise
*/
/**************************************************************************/
bool Serial_Command_Handler::sendCommandWait(const char *cmd, const char *response, unsigned long timeout, uint8_t length)
{
    write(cmd);
    return waitForResponse(response, timeout, length);
}

/**************************************************************************/
/*!
    @brief Send an user defined command to the device and wait for the response
    @param cmd String holding the command to send
    @param response Pointer to a string holding the desired response
    @param timeout How long to wait for a response in milliseconds (optional)
    @param length How many characters to wait (optional)
    @return True if we got the response, false otherwise
*/
/**************************************************************************/
bool Serial_Command_Handler::sendCommandWait(String cmd, const char *response, unsigned long timeout, uint8_t length)
{
    return sendCommandWait(cmd.c_str(), response, timeout, length);
}

/**************************************************************************/
/*!
    @brief Send an user defined command to the device and wait for the response
    @param cmd Pointer to a string holding the command to send
    @param response String holding the desired response
    @param timeout How long to wait for a response in milliseconds (optional)
    @param length How many characters to wait (optional)
    @return True if we got the response, false otherwise
*/
/**************************************************************************/
bool Serial_Command_Handler::sendCommandWait(const char *cmd, String response, unsigned long timeout, uint8_t length)
{
    return sendCommandWait(cmd, response.c_str(), timeout, length);
}

/**************************************************************************/
/*!
    @brief Send an user defined command to the device and wait for the response
    @param cmd String holding the command to send
    @param response String holding the desired response
    @param timeout How long to wait for a response in milliseconds (optional)
    @param length How many characters to wait (optional)
    @return True if we got the response, false otherwise
*/
/**************************************************************************/
bool Serial_Command_Handler::sendCommandWait(String cmd, String response, unsigned long timeout, uint8_t length)
{
    return sendCommandWait(cmd.c_str(), response.c_str(), timeout, length);
}

/**************************************************************************/
/*!
    @brief Send an user defined command to the device and wait for the OK response
    @param cmd Pointer to a string holding the command to send
    @param timeout How long to wait for a response in milliseconds (optional)
    @return True if we got the response, false otherwise
*/
/**************************************************************************/
bool Serial_Command_Handler::sendCommandWaitOK(const char *cmd, unsigned long timeout)
{
    write(cmd);
    return waitForOK(timeout);
}

/**************************************************************************/
/*!
    @brief Send an user defined command to the device and wait for the OK response
    @param cmd String holding the command to send
    @param timeout How long to wait for a response in milliseconds (optional)
    @return True if we got the response, false otherwise
*/
/**************************************************************************/
bool Serial_Command_Handler::sendCommandWaitOK(String cmd, unsigned long timeout)
{
    write(cmd.c_str());
    return waitForOK(timeout);
}

void Serial_Command_Handler::ATBypass(void)
{
#if (defined(__AVR__) || defined(ESP8266)) && defined(USE_SW_SERIAL)
    if (SwSerial)
    {
        if (SwSerial->available())
            Serial.write(SwSerial->read());
        if (Serial.available())
            SwSerial->write(Serial.read());
    }
#endif
    if (HwSerial)
    {
        if (HwSerial->available())
            Serial.write(HwSerial->read());
        if (Serial.available())
            HwSerial->write(Serial.read());
    }
}

/**************************************************************************/
/*!
    @brief Clean a buffer
    @param buffer Pointer to the buffer to clean
    @param count How many bytes to clean (optional)
*/
/**************************************************************************/
void Serial_Command_Handler::cleanBuffer(char *buffer, int count)
{
    for (int i = 0; i < count; i++)
        buffer[i] = '\0';
}