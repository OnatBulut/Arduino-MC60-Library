#ifndef __SERIAL_COMMAND_HANDLER_H__
#define __SERIAL_COMMAND_HANDLER_H__

#define DEFAULT_TIMEOUT 1000UL
#define SHORT_TIMEOUT 100UL

#define MAXLINELENGTH 120 ///< how long are max lines to parse

#if (defined(__AVR__) || defined(ESP8266)) && !defined(NO_SW_SERIAL)
#define USE_SW_SERIAL
#endif

#include <Arduino.h>
#ifdef USE_SW_SERIAL
#include <SoftwareSerial.h>
#endif

/**************************************************************************/
/*!
    @brief  The Serial_Command_Handler class
*/
/**************************************************************************/
class Serial_Command_Handler : public Print
{
public:
    void begin(uint32_t baud);

#ifdef USE_SW_SERIAL
    Serial_Command_Handler(SoftwareSerial *ser);
#endif
    Serial_Command_Handler(HardwareSerial *ser);
    Serial_Command_Handler();
    virtual ~Serial_Command_Handler();

    size_t available(void);
    size_t write(uint8_t);
    size_t write(const char *cmd);
    size_t write(String cmd);
    void flush(void);
    char read(void);
    String readline(unsigned long timeout = SHORT_TIMEOUT, uint8_t length = MAXLINELENGTH);
    char *readbetween(const char first, const char last, unsigned long timeout = SHORT_TIMEOUT, uint8_t length = MAXLINELENGTH);
    void pause(bool b);

    bool sendAT(unsigned long timeout = DEFAULT_TIMEOUT);
    void sendEndMarker(void);

    bool waitForOK(unsigned long timeout = DEFAULT_TIMEOUT, uint8_t length = MAXLINELENGTH);
    bool waitForResponse(const char *wait, unsigned long timeout = DEFAULT_TIMEOUT, uint8_t length = MAXLINELENGTH);
    bool waitForResponse(String wait, unsigned long timeout = DEFAULT_TIMEOUT, uint8_t length = MAXLINELENGTH);
    bool sendCommandWait(const char *cmd, const char *response, unsigned long timeout = DEFAULT_TIMEOUT, uint8_t length = MAXLINELENGTH);
    bool sendCommandWait(String cmd, const char *response, unsigned long timeout = DEFAULT_TIMEOUT, uint8_t length = MAXLINELENGTH);
    bool sendCommandWait(const char *cmd, String response, unsigned long timeout = DEFAULT_TIMEOUT, uint8_t length = MAXLINELENGTH);
    bool sendCommandWait(String cmd, String response, unsigned long timeout = DEFAULT_TIMEOUT, uint8_t length = MAXLINELENGTH);
    bool sendCommandWaitOK(const char *cmd, unsigned long timeout = DEFAULT_TIMEOUT);
    bool sendCommandWaitOK(String cmd, unsigned long timeout = DEFAULT_TIMEOUT);

    void ATBypass(void);

protected:
    void common_init(void);
    void cleanBuffer(char *buffer, int count = MAXLINELENGTH);

    bool paused;
    bool noComms = false;

#ifdef USE_SW_SERIAL
    SoftwareSerial *SwSerial;
#endif
    HardwareSerial *HwSerial;

private:
    uint8_t lineidx = 0;        ///< our index into filling the current line
    char buffer[MAXLINELENGTH]; ///< Current line buffer
    String sbuffer = "";        ///< Current line buffer
};

#endif