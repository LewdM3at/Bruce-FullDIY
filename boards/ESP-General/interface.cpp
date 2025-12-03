#include "core/powerSave.h"
#include <interface.h>
#include <Wire.h>

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio()
{
    pinMode(UP_BTN, INPUT_PULLUP);
    pinMode(SEL_BTN, INPUT_PULLUP);
    pinMode(DW_BTN, INPUT_PULLUP);

    Wire.setPins(GROVE_SDA, GROVE_SCL);
    Wire.begin(GROVE_SDA, GROVE_SCL);

    pinMode(NRF24_SS_PIN, OUTPUT);
    pinMode(CC1101_SS_PIN, OUTPUT);
    pinMode(SDCARD_CS, OUTPUT);
    pinMode(TFT_CS, OUTPUT);

    digitalWrite(NRF24_SS_PIN, HIGH);
    digitalWrite(CC1101_SS_PIN, HIGH);
    digitalWrite(SDCARD_CS, HIGH);
    digitalWrite(TFT_CS, HIGH);

    bruceConfig.rfModule = CC1101_SPI_MODULE;
    bruceConfig.rfidModule = PN532_I2C_MODULE;
    bruceConfig.irRx = RXLED;
    bruceConfig.startupApp = "WebUI";
}

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() { return 0; }

/***************************************************************************************
** Function name: isCharging()
** Description:   Default implementation that returns false
***************************************************************************************/
bool isCharging() { return false; }

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void)
{
    static unsigned long tm = 0;
    if (millis() - tm < 200 && !LongPress) return;

    bool upPressed = (digitalRead(UP_BTN) == LOW);
    bool selPressed = (digitalRead(SEL_BTN) == LOW);
    bool dwPressed = (digitalRead(DW_BTN) == LOW);

    bool anyPressed = upPressed || selPressed || dwPressed;
    if (anyPressed) tm = millis();
    if (anyPressed && wakeUpScreen()) return;

    AnyKeyPress = anyPressed;
    PrevPress = upPressed;
    EscPress = upPressed;
    NextPress = dwPressed;
    SelPress = selPressed;
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to turnoff the device (name is odd btw)
**********************************************************************/
void checkReboot() {}
