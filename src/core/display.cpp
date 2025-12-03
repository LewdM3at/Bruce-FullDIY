#include "display.h"
#include "core/wifi/webInterface.h" // for server
#include "core/wifi/wg.h"           //for isConnectedWireguard to print wireguard lock
#include "mykeyboard.h"
#include "settings.h" //for timeStr
#include "utils.h"
#include <JPEGDecoder.h>
#include <interface.h> //for charging ischarging to print charging indicator

#define MAX_MENU_SIZE (int)(tftHeight / 25)

// Send the ST7789 into or out of sleep mode
void panelSleep(bool on) {
#if defined(ST7789_2_DRIVER) || defined(ST7789_DRIVER)
    if (on) {
        tft.writecommand(0x10); // SLPIN: panel off
        delay(5);
    } else {
        tft.writecommand(0x11); // SLPOUT: panel on
        delay(120);
    }
#endif
}

bool __attribute__((weak)) isCharging() { return false; }
/***************************************************************************************
** Function name: displayScrollingText
** Description:   Scroll large texts into screen
***************************************************************************************/
void displayScrollingText(const String &text, Opt_Coord &coord) {
    int len = text.length();
    String displayText = text + "        "; // Add spaces for smooth looping
    int scrollLen = len + 8;                // Full text plus space buffer
    static int i = 0;
    static long _lastmillis = 0;
    tft.setTextColor(coord.fgcolor, coord.bgcolor);
    if (len < coord.size) {
        // Text fits within limit, no scrolling needed
        return;
    } else if (millis() > _lastmillis + 200) {
        String scrollingPart =
            displayText.substring(i, i + (coord.size - 1)); // Display charLimit characters at a time
        tft.fillRect(
            coord.x, coord.y, (coord.size - 1) * LW * tft.textsize, LH * tft.textsize, bruceConfig.bgColor
        ); // Clear display area
        tft.setCursor(coord.x, coord.y);
        tft.setCursor(coord.x, coord.y);
        tft.print(scrollingPart);
        if (i >= scrollLen - coord.size) i = -1; // Loop back
        _lastmillis = millis();
        i++;
        if (i == 1) _lastmillis = millis() + 1000;
    }
}

/***************************************************************************************
** Function name: TouchFooter
** Description:   Draw touch screen footer
***************************************************************************************/
void TouchFooter(uint16_t color) {
    tft.drawRoundRect(5, tftHeight + 2, tftWidth - 10, 43, 5, color);
    tft.setTextColor(color);
    tft.setTextSize(FM);
    tft.drawCentreString("PREV", tftWidth / 6, tftHeight + 4, 1);
    tft.drawCentreString("SEL", tftWidth / 2, tftHeight + 4, 1);
    tft.drawCentreString("NEXT", 5 * tftWidth / 6, tftHeight + 4, 1);
}
/***************************************************************************************
** Function name: TouchFooter
** Description:   Draw touch screen footer
***************************************************************************************/
void MegaFooter(uint16_t color) {
    tft.drawRoundRect(5, tftHeight + 2, tftWidth - 10, 43, 5, color);
    tft.setTextColor(color);
    tft.setTextSize(FM);
    tft.drawCentreString("Exit", tftWidth / 6, tftHeight + 4, 1);
    tft.drawCentreString("UP", tftWidth / 2, tftHeight + 4, 1);
    tft.drawCentreString("DOWN", 5 * tftWidth / 6, tftHeight + 4, 1);
}

/***************************************************************************************
** Function name: resetTftDisplay
** Description:   set cursor to 0,0, screen and text to default color
***************************************************************************************/
void resetTftDisplay(int x, int y, uint16_t fc, int size, uint16_t bg, uint16_t screen) {
    tft.setCursor(x, y);
    tft.fillScreen(screen);
    tft.setTextSize(size);
    tft.setTextColor(fc, bg);
}

/***************************************************************************************
** Function name: setTftDisplay
** Description:   set cursor, font color, size and bg font color
***************************************************************************************/
void setTftDisplay(int x, int y, uint16_t fc, int size, uint16_t bg) {
    if (x >= 0 && y < 0) tft.setCursor(x, tft.getCursorY());      // if -1 on x, sets only y
    else if (x < 0 && y >= 0) tft.setCursor(tft.getCursorX(), y); // if -1 on y, sets only x
    else if (x >= 0 && y >= 0) tft.setCursor(x, y);               // if x and y > 0, sets both
    tft.setTextSize(size);
    tft.setTextColor(fc, bg);
}

void turnOffDisplay() { setBrightness(0, false); }

bool wakeUpScreen() {
    previousMillis = millis();
    if (isScreenOff) {
        isScreenOff = false;
        dimmer = false;
        getBrightness();
        vTaskDelay(pdMS_TO_TICKS(200));
        return true;
    } else if (dimmer) {
        dimmer = false;
        getBrightness();
        vTaskDelay(pdMS_TO_TICKS(200));
        return true;
    }
    return false;
}

/***************************************************************************************
** Function name: displayRedStripe
** Description:   Display Red Stripe with information
***************************************************************************************/
void displayRedStripe(String text, uint16_t fgcolor, uint16_t bgcolor) {
    // detect if not running in interactive mode -> show nothing onscreen and return immediately
    // if (server || isSleeping || isScreenOff) return; // webui is running

    int size;
    if (fgcolor == bgcolor && fgcolor == TFT_WHITE) fgcolor = TFT_BLACK;
    if (text.length() * LW * FM < (tftWidth - 2 * FM * LW)) size = FM;
    else size = FP;
    tft.drawPixel(0, 0, 0);
    tft.fillRoundRect(10, tftHeight / 2 - 13, tftWidth - 20, 26, 7, bgcolor);
    tft.setTextColor(fgcolor, bgcolor);
    if (size == FM) {
        tft.setTextSize(FM);
        tft.drawCentreString(text, tftWidth / 2, tftHeight / 2 - 8);
    } else {
        tft.setTextSize(FP);
        int text_size = text.length();
        if (text_size < (tftWidth - 20) / (LW * FP))
            tft.drawCentreString(text, tftWidth / 2, tftHeight / 2 - 8);
        else {
            tft.drawCentreString(text.substring(0, text_size / 2), tftWidth / 2, tftHeight / 2 - 9);
            tft.drawCentreString(text.substring(text_size / 2), tftWidth / 2, tftHeight / 2 + 1);
        }
    }
}

void drawButton(
    int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color, const char *text, bool inverted = false
) {
    if (inverted) {
        tft.fillRoundRect(x, y, w, h, 5, color);
    } else {
        tft.fillRoundRect(x, y, w, h, 5, TFT_BLACK);
        tft.drawRoundRect(x, y, w, h, 5, color);
    }
    tft.setTextColor(inverted ? TFT_BLACK : color);
    tft.drawString(text, x + w / 2, y + h);
}

int8_t displayMessage(
    const char *message, const char *leftButton, const char *centerButton, const char *rightButton,
    uint16_t color
) {
#ifdef HAS_SCREEN
    uint8_t oldTextDatum = tft.getTextDatum();
#endif

    tft.setTextColor(color);
    tft.setTextSize(FM);
    tft.setTextDatum(TC_DATUM);
    tft.drawString(message, tftWidth / 2, tftHeight / 2 - 20);

    tft.setTextDatum(BC_DATUM);
    int16_t buttonHeight = 20;
    int16_t buttonY = tftHeight - buttonHeight - 5;
    int16_t buttonWidth = tftWidth / 3 - 10;

    int8_t totalButtons = (leftButton ? 1 : 0) + (centerButton ? 1 : 0) + (rightButton ? 1 : 0);
    int8_t selected = 0; // Start at first available button
    bool redraw = true;

    while (true) {
        if (check(PrevPress) || check(EscPress)) {
            selected = (selected - 1 + totalButtons) % totalButtons; // Cycle backward
            redraw = true;
        }
        if (check(NextPress)) {
            selected = (selected + 1) % totalButtons; // Cycle forward
            redraw = true;
        }
        if (check(SelPress)) { break; }

        // Draw buttons with selection highlighting
        int8_t index = 0;

        if (redraw) {
            if (leftButton) {
                drawButton(5, buttonY, buttonWidth, buttonHeight, color, leftButton, selected == index);
                index++;
            }

            if (centerButton) {
                drawButton(
                    tftWidth / 3 + 5,
                    buttonY,
                    buttonWidth,
                    buttonHeight,
                    color,
                    centerButton,
                    selected == index
                );
                index++;
            }

            if (rightButton) {
                drawButton(
                    tftWidth * 2 / 3 + 5,
                    buttonY,
                    buttonWidth,
                    buttonHeight,
                    color,
                    rightButton,
                    selected == index
                );
            }
            redraw = false;
        }

        delay(10);
    }

#ifdef HAS_SCREEN
    tft.setTextDatum(oldTextDatum);
#endif

    return selected;
}

void displayError(String txt, bool waitKeyPress) {
    displayRedStripe(txt);
#ifndef HAS_SCREEN
    Serial.println("ERR: " + txt);
    return;
#endif
    delay(200);
    while (waitKeyPress && !check(AnyKeyPress)) vTaskDelay(10 / portTICK_PERIOD_MS);
}

void displayWarning(String txt, bool waitKeyPress) {
    displayRedStripe(txt, TFT_BLACK, TFT_YELLOW);
#ifndef HAS_SCREEN
    Serial.println("WARN: " + txt);
    return;
#endif
    delay(200);
    while (waitKeyPress && !check(AnyKeyPress)) vTaskDelay(10 / portTICK_PERIOD_MS);
}

void displayInfo(String txt, bool waitKeyPress) {
    // todo: add newlines to txt if too long
    displayRedStripe(txt, TFT_WHITE, TFT_BLUE);
#ifndef HAS_SCREEN
    Serial.println("INFO: " + txt);
    return;
#endif

    delay(200);
    while (waitKeyPress && !check(AnyKeyPress)) vTaskDelay(10 / portTICK_PERIOD_MS);
}

void displaySuccess(String txt, bool waitKeyPress) {
    // todo: add newlines to txt if too long
    displayRedStripe(txt, TFT_WHITE, TFT_DARKGREEN);
#ifndef HAS_SCREEN
    Serial.println("SUCCESS: " + txt);
    return;
#endif
    delay(200);
    while (waitKeyPress && !check(AnyKeyPress)) vTaskDelay(10 / portTICK_PERIOD_MS);
}

void displayTextLine(String txt, bool waitKeyPress) {
    // todo: add newlines to txt if too long
    displayRedStripe(txt, getComplementaryColor2(bruceConfig.priColor), bruceConfig.priColor);
#ifndef HAS_SCREEN
    Serial.println("MESSAGE: " + txt);
    return;
#endif
    delay(200);
    while (waitKeyPress && !check(AnyKeyPress)) vTaskDelay(10 / portTICK_PERIOD_MS);
}

void setPadCursor(int16_t padx, int16_t pady) {
    for (int y = 0; y < pady; y++) tft.println();
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
}

void padprintf(int16_t padx, const char *format, ...) {
    char buffer[64];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.printf("%s", buffer);
}
void padprintf(const char *format, ...) {
    char buffer[64];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    tft.setCursor(BORDER_PAD_X, tft.getCursorY());
    tft.printf("%s", buffer);
}

void padprint(const String &s, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(s);
}
void padprint(const char str[], int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(str);
}
void padprint(char c, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(c);
}
void padprint(unsigned char b, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(b, base);
}
void padprint(int n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(n, base);
}
void padprint(unsigned int n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(n, base);
}
void padprint(long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(n, base);
}
void padprint(unsigned long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(n, base);
}
void padprint(long long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(n, base);
}
void padprint(unsigned long long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(n, base);
}
void padprint(double n, int digits, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.print(n, digits);
}

void padprintln(const String &s, int16_t padx) {
    if (s.isEmpty()) {
        tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
        tft.println(s);
        return;
    }

    String buff;
    size_t start = 0;
    int _maxCharsInLine = (tftWidth - (padx + 1) * BORDER_PAD_X) / (FP * LW);

    // automatically split into multiple lines
    while (!(buff = s.substring(start, start + _maxCharsInLine)).isEmpty()) {
        tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
        tft.println(buff);
        start += buff.length();
    }
}
void padprintln(const char str[], int16_t padx) {
    if (strcmp(str, "") == 0) {
        tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
        tft.println(str);
        return;
    }

    String buff;
    size_t start = 0;
    int _maxCharsInLine = (tftWidth - (padx + 1) * BORDER_PAD_X) / (FP * LW);

    // automatically split into multiple lines
    while (!(buff = String(str).substring(start, start + _maxCharsInLine)).isEmpty()) {
        tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
        tft.println(buff);
        start += buff.length();
    }
}
void padprintln(char c, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(c);
}
void padprintln(unsigned char b, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(b, base);
}
void padprintln(int n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(n, base);
}
void padprintln(unsigned int n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(n, base);
}
void padprintln(long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(n, base);
}
void padprintln(unsigned long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(n, base);
}
void padprintln(long long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(n, base);
}
void padprintln(unsigned long long n, int base, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(n, base);
}
void padprintln(double n, int digits, int16_t padx) {
    tft.setCursor(padx * BORDER_PAD_X, tft.getCursorY());
    tft.println(n, digits);
}

/*********************************************************************
**  Function: loopOptions
**  Where you choose among the options in menu
**********************************************************************/
int loopOptions(
    std::vector<Option> &options, uint8_t menuType, const char *subText, int index, bool interpreter
) {
    Opt_Coord coord;
    bool redraw = true;
    bool exit = false;
    int menuSize = options.size();
    int devModeCounter = 0;
    static unsigned long _clock_bat_timer = millis();
    if (options.size() > MAX_MENU_SIZE) { menuSize = MAX_MENU_SIZE; }
    if (index > 0)
        tft.fillRoundRect(
            tftWidth * 0.10,
            tftHeight / 2 - menuSize * (FM * 8 + 4) / 2 - 5,
            tftWidth * 0.8,
            (FM * 8 + 4) * menuSize + 10,
            5,
            bruceConfig.bgColor
        );
    if (index >= options.size()) index = 0;
    bool firstRender = true;
    drawMainBorder();
    while (1) {
        // Check for shutdown before drawing menu to avoid drawing a black bar on the screen
        if (exit) break;
        if (menuType == MENU_TYPE_MAIN) {
            checkReboot();
            if (devModeCounter >= 5 && !bruceConfig.devMode) {
                bruceConfig.setDevMode(true);
                displayInfo("Dev Mode Enabled", true);
            }
            if (millis() - _clock_bat_timer > 30000) {
                _clock_bat_timer = millis();
                drawStatusBar(); // update clock and battery status each 30s
            }
        }

        if (redraw) {
            menuOptionType = menuType; // updates menutype to the remote controller
            menuOptionLabel = subText;
            // update the hovered
            for (auto &opt : options) opt.hovered = false;
            options[index].hovered = true;

            bool renderedByLambda = false;
            if (options[index].hover)
                renderedByLambda = options[index].hover(options[index].hoverPointer, true);

            if (!renderedByLambda) {
                if (menuType == MENU_TYPE_SUBMENU) drawSubmenu(index, options, subText);
                else
                    coord = drawOptions(
                        index,
                        options,
                        bruceConfig.priColor,
                        bruceConfig.secColor,
                        bruceConfig.bgColor,
                        firstRender
                    );
            }
            firstRender = false;
            redraw = false;
        }

        // handleSerialCommands(); // always use serial task for it
#ifdef HAS_KEYBOARD
        checkShortcutPress(); // shortctus to quickly start apps without navigating the menus
#endif

        if (menuType == MENU_TYPE_REGULAR) {
            String txt = options[index].label;
            displayScrollingText(txt, coord);
        }

        if (PrevPress || check(UpPress)) {
            devModeCounter = 0;
#ifdef HAS_KEYBOARD
            check(PrevPress);
            if (index == 0) index = options.size() - 1;
            else if (index > 0) index--;
            redraw = true;
#else
            long _tmp = millis();
#ifndef HAS_ENCODER // T-Embed doesn't need it
            LongPress = true;
            while (PrevPress && menuType != MENU_TYPE_MAIN) {
                if (millis() - _tmp > 200)
                    tft.drawArc(
                        tftWidth / 2,
                        tftHeight / 2,
                        25,
                        15,
                        0,
                        360 * (millis() - (_tmp + 200)) / 500,
                        getColorVariation(bruceConfig.priColor),
                        bruceConfig.bgColor
                    );
                vTaskDelay(10 / portTICK_RATE_MS);
            }
            tft.drawArc(
                tftWidth / 2, tftHeight / 2, 25, 15, 0, 360, bruceConfig.bgColor, bruceConfig.bgColor
            );
            LongPress = false;
#endif
            if (millis() - _tmp > 700) { // longpress detected to exit
                index = -1;
                break;
            } else {
                check(PrevPress);
                if (index == 0) index = options.size() - 1;
                else if (index > 0) index--;
                redraw = true;
            }
#endif
        }
        /* DW Btn to next item */
        if (check(NextPress) || check(DownPress)) {
            index++;
            if ((index + 1) > options.size()) {
                if (!bruceConfig.devMode) devModeCounter++;
                index = 0;
            }
            redraw = true;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);

        /* Select and run function
        forceMenuOption is set by a SerialCommand to force a selection within the menu
        */
        if (check(SelPress) || forceMenuOption >= 0) {
            uint16_t chosen = index;
            if (forceMenuOption >= 0) {
                chosen = forceMenuOption;
                forceMenuOption = -1; // reset SerialCommand navigation option
                Serial.print("Forcely ");
            }
            Serial.println("Selected: " + String(options[chosen].label));
            options[chosen].operation();
            break;
        }
        // interpreter_start -> running the interpreter
        // interpreter -> loopOptions helper inside the Javascript
        if (interpreter_start && !interpreter) { break; }

#ifdef HAS_KEYBOARD
        if (check(EscPress)) {
            index = -1;
            break;
        }
        /* DISABLED: may conflict with custom shortcuts
        int pressed_number = checkNumberShortcutPress();
        if (pressed_number >= 0) {
            if (index == pressed_number) {
                // press 2 times the same number to confirm
                options[index].operation();
                break;
            }
            // else only highlight the option
            index = pressed_number;
            if ((index + 1) > options.size()) index = options.size() - 1;
            redraw = true;
        }*/

#elif defined(T_EMBED) || defined(HAS_TOUCH) || !defined(HAS_SCREEN)
        if (menuType != MENU_TYPE_MAIN && check(EscPress)) break;
#endif
    }
    return index;
}

/***************************************************************************************
** Function name: progressHandler
** Description:   Função para manipular o progresso da atualização
** Dependencia: prog_handler =>>    0 - Flash, 1 - LittleFS
***************************************************************************************/
void progressHandler(int progress, size_t total, String message) {
    int barWidth = map(progress, 0, total, 0, tftWidth - 40);
    if (barWidth < 3) {
        tft.fillRect(6, 27, tftWidth - 12, tftHeight - 33, bruceConfig.bgColor);
        tft.drawRect(18, tftHeight - 47, tftWidth - 36, 17, bruceConfig.priColor);
        displayRedStripe(message, TFT_WHITE, bruceConfig.priColor);
    }
    tft.fillRect(20, tftHeight - 45, barWidth, 13, bruceConfig.priColor);
}

/***************************************************************************************
** Function name: drawOptions
** Description:   Função para desenhar e mostrar as opçoes de contexto
***************************************************************************************/
Opt_Coord drawOptions(
    int index, std::vector<Option> &options, uint16_t fgcolor, uint16_t selcolor, uint16_t bgcolor,
    bool firstRender
) {
    Opt_Coord coord;
    int menuSize = options.size();
    if (options.size() > MAX_MENU_SIZE) { menuSize = MAX_MENU_SIZE; }

    // Uncomment to update the statusBar (causes flickering)
    // drawStatusBar();

    int32_t optionsTopY = tftHeight / 2 - menuSize * (FM * 8 + 4) / 2 - 5;
    tft.drawPixel(0, 0, bruceConfig.bgColor);
    if (firstRender) {
        tft.fillRoundRect(
            tftWidth * 0.10, optionsTopY, tftWidth * 0.8, (FM * 8 + 4) * menuSize + 10, 5, bgcolor
        );
        tft.drawRoundRect(
            tftWidth * 0.10,
            tftHeight / 2 - menuSize * (FM * 8 + 4) / 2 - 5,
            tftWidth * 0.8,
            (FM * 8 + 4) * menuSize + 10,
            5,
            fgcolor
        );
    }

    tft.setTextColor(fgcolor, bgcolor);
    tft.setTextSize(FM);
    tft.setCursor(tftWidth * 0.10 + 5, tftHeight / 2 - menuSize * (FM * 8 + 4) / 2);

    int i = 0;
    int init = 0;
    int cont = 1;
    menuSize = options.size();
    if (index >= MAX_MENU_SIZE) init = index - MAX_MENU_SIZE + 1;
    for (i = 0; i < menuSize; i++) {
        if (i >= init) {
            if (options[i].selected) tft.setTextColor(selcolor, bgcolor); // if selected, change Text color
            else tft.setTextColor(fgcolor, bgcolor);

            String text = "";
            if (i == index) {
                text += ">";
                coord.x = tftWidth * 0.10 + 5 + FM * LW;
                coord.y = tft.getCursorY() + 4;
                coord.size = (tftWidth * 0.8 - 10) / (LW * FM) - 1;
                coord.fgcolor = fgcolor;
                coord.bgcolor = bgcolor;
            } else text += " ";
            text += String(options[i].label) + "              ";
            tft.setCursor(tftWidth * 0.10 + 5, tft.getCursorY() + 4);
            tft.println(text.substring(0, (tftWidth * 0.8 - 10) / (LW * FM) - 1));
            cont++;
        }
        if (cont > MAX_MENU_SIZE) goto Exit;
    }
Exit:
    if (options.size() > MAX_MENU_SIZE) menuSize = MAX_MENU_SIZE;
#if defined(HAS_TOUCH)
    TouchFooter();
#endif
    return coord;
}

/***************************************************************************************
** Function name: drawSubmenu
** Description:   Função para desenhar e mostrar as opçoes de contexto
***************************************************************************************/
void drawSubmenu(int index, std::vector<Option> &options, const char *title) {
    drawStatusBar();
    int menuSize = options.size();
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.drawPixel(0, 0, 0);
    //tft.fillRect(6, 30, tftWidth - 12, 8 * FP, bruceConfig.bgColor);  // Remove draw override rect
    tft.drawString(title, 26, 52);  // Move the title string because of custom border

    // middle of the drawing area
    int middle = 25 /*status*/ + (tftHeight - 30 /*status + bottom margin*/) / 2;
    // drawCentreString uses TC_DATUM, so we need to adjust the Y position
    // 42 ensures that title isnt touched( 30 + 8 (LH) + 4(Margin))
    int middle_up = middle - (tftHeight - 42) / 3 - FM * LH / 2 + 4;
    int middle_down = middle + (tftHeight - 42) / 3 - FM * LH / 2;

    tft.setTextSize(FM);
#if defined(HAS_TOUCH)
    tft.drawCentreString("/\\", tftWidth / 2, middle_up - (FM * LH + 6), 1);
#endif
    // Previous item
    const char *firstOption =
        index - 1 >= 0 ? options[index - 1].label.c_str() : options[menuSize - 1].label.c_str();
    tft.setTextColor(bruceConfig.secColor);
    tft.fillRect(20, middle_up, tftWidth - 40, 8 * FM, bruceConfig.bgColor);    // make the override rect smaller to facilitate the custom border
    tft.drawCentreString(firstOption, tftWidth / 2, middle_up, SMOOTH_FONT);

    // Selected item
    int selectedTextSize = options[index].label.length() <= tftWidth / (LW * FG) - 1 ? FG : FM;
    tft.setTextSize(selectedTextSize);
    tft.setTextColor(bruceConfig.priColor);
    tft.fillRect(20, middle - FG * LH / 2 - 1, tftWidth - 40, FG * LH + 5, bruceConfig.bgColor); // make the override rect smaller to facilitate the custom border
    tft.drawCentreString(options[index].label, tftWidth / 2, middle - selectedTextSize * LH / 2, SMOOTH_FONT);
    tft.drawFastHLine(
        tftWidth / 2 - strlen(options[index].label.c_str()) * selectedTextSize * LW / 2,
        middle + selectedTextSize * LH / 2 + 1,
        strlen(options[index].label.c_str()) * selectedTextSize * LW,
        bruceConfig.priColor
    );
    // Next Item
    const char *thirdOption =
        index + 1 < menuSize ? options[index + 1].label.c_str() : options[0].label.c_str();
    tft.setTextSize(FM);
    tft.setTextColor(bruceConfig.secColor);
    tft.fillRect(20, middle_down, tftWidth - 40, 8 * FM, bruceConfig.bgColor);   // make the override rect smaller to facilitate the custom border
    tft.drawCentreString(thirdOption, tftWidth / 2, middle_down, SMOOTH_FONT);

    tft.fillRect(tftWidth - 5, 0, 5, tftHeight, bruceConfig.bgColor);
    tft.fillRect(tftWidth - 5, index * tftHeight / menuSize, 5, tftHeight / menuSize, bruceConfig.priColor);

#if defined(HAS_TOUCH)
    tft.drawCentreString("\\/", tftWidth / 2, middle_down + (FM * LH + 6), 1);
    tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
    tft.drawString("[ x ]", 7, 7, 1);
    TouchFooter();
#endif
}

void drawStatusBar() {
    int i = 0;
    uint8_t bat = getBattery();
    uint8_t bat_margin = 85;
    if (bat > 0) {
        drawBatteryStatus(bat);
    } else bat_margin = 34; // increase battery margin for custom border
    if (sdcardMounted) {
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.setTextSize(FP);
        tft.drawString("SD", tftWidth - (bat_margin + 20 * i), 35);     // shift positions of status icons around for custom border
        i++;
    } // Indication for SD card on screen
    if (gpsConnected) {
        drawGpsSmall(tftWidth - (bat_margin + 23 * i), 30);
        i++;
    }
    if (WiFi.getMode()) {
        drawWifiSmall(tftWidth - (bat_margin + 23 * i), 30);
        i++;
    } // Draw Wifi Symbol beside battery
    if (isWebUIActive) {
        drawWebUISmall(tftWidth - (bat_margin + 23 * i), 30);
        i++;
    } // Draw Wifi Symbol beside battery
    if (BLEConnected) {
        drawBLESmall(tftWidth - (bat_margin + 23 * i), 30);
        i++;
    } // Draw BLE beside Wifi
    if (isConnectedWireguard) {
        drawWireguardStatus(tftWidth - (bat_margin + 24 * i), 30);
        i++;
    } // Draw Wg bedide BLE, if the others exist, if not, beside battery

    if (bruceConfig.theme.border) {
        // Cyberpunk themed border
        tft.drawLine(305, 18, 351, 18, 0xF602);
        tft.drawLine(352, 17, 360, 9, 0xF602);
        tft.drawLine(361, 9, 445, 9, 0xF602);
        tft.drawLine(446, 10, 469, 33, 0xF602);
        tft.drawLine(360, 10, 445, 10, 0xF602);
        tft.drawLine(469, 34, 469, 53, 0xF602);
        tft.drawLine(446, 11, 468, 33, 0xF602);
        tft.drawLine(470, 54, 473, 57, 0xF602);
        tft.drawLine(470, 53, 473, 56, 0xF602);
        tft.drawLine(470, 52, 474, 56, 0xF602);
        tft.drawLine(390, 14, 395, 14, 0xF602);
        tft.drawLine(400, 14, 405, 14, 0xF602);
        tft.drawLine(411, 14, 416, 14, 0xF602);
        tft.drawLine(422, 14, 427, 14, 0xF602);
        tft.drawLine(391, 15, 396, 15, 0xF602);
        tft.drawLine(401, 15, 406, 15, 0xF602);
        tft.drawLine(392, 16, 397, 16, 0xF602);
        tft.drawLine(393, 17, 398, 17, 0xF602);
        tft.drawLine(394, 18, 399, 18, 0xF602);
        tft.drawLine(395, 19, 400, 19, 0xF602);
        tft.drawLine(396, 20, 401, 20, 0xF602);
        tft.drawLine(402, 16, 407, 16, 0xF602);
        tft.drawLine(403, 17, 408, 17, 0xF602);
        tft.drawLine(404, 18, 409, 18, 0xF602);
        tft.drawLine(405, 19, 410, 19, 0xF602);
        tft.drawLine(406, 20, 411, 20, 0xF602);
        tft.drawLine(412, 15, 417, 15, 0xF602);
        tft.drawLine(413, 16, 418, 16, 0xF602);
        tft.drawLine(414, 17, 419, 17, 0xF602);
        tft.drawLine(415, 18, 420, 18, 0xF602);
        tft.drawLine(416, 19, 421, 19, 0xF602);
        tft.drawLine(417, 20, 422, 20, 0xF602);
        tft.drawLine(423, 15, 428, 15, 0xF602);
        tft.drawLine(424, 16, 429, 16, 0xF602);
        tft.drawLine(425, 17, 430, 17, 0xF602);
        tft.drawLine(426, 18, 431, 18, 0xF602);
        tft.drawLine(427, 19, 432, 19, 0xF602);
        tft.drawLine(428, 20, 433, 20, 0xF602);
        tft.drawLine(307, 16, 300, 9, 0xF602);
        tft.drawLine(299, 9, 230, 9, 0xF602);
        tft.drawLine(325, 45, 346, 24, 0xF602);
        tft.drawLine(346, 23, 305, 23, 0xF602);
        tft.drawLine(325, 46, 312, 46, 0xF602);
        tft.drawLine(289, 22, 296, 22, 0xF602);
        tft.drawLine(292, 24, 298, 24, 0xF602);
        tft.drawLine(304, 24, 307, 24, 0xF602);
        tft.drawLine(303, 25, 306, 25, 0xF602);
        tft.drawLine(252, 22, 255, 22, 0xF602);
        tft.drawLine(251, 23, 254, 23, 0xF602);
        tft.drawLine(250, 24, 253, 24, 0xF602);
        tft.drawLine(249, 25, 252, 25, 0xF602);
        tft.drawLine(248, 26, 251, 26, 0xF602);
        tft.drawLine(258, 22, 261, 22, 0xF602);
        tft.drawLine(257, 23, 260, 23, 0xF602);
        tft.drawLine(256, 24, 259, 24, 0xF602);
        tft.drawLine(255, 25, 258, 25, 0xF602);
        tft.drawLine(254, 26, 257, 26, 0xF602);
        tft.drawLine(264, 22, 267, 22, 0xF602);
        tft.drawLine(263, 23, 266, 23, 0xF602);
        tft.drawLine(262, 24, 265, 24, 0xF602);
        tft.drawLine(261, 25, 264, 25, 0xF602);
        tft.drawLine(260, 26, 263, 26, 0xF602);
        tft.drawLine(270, 22, 273, 22, 0xF602);
        tft.drawLine(269, 23, 272, 23, 0xF602);
        tft.drawLine(268, 24, 271, 24, 0xF602);
        tft.drawLine(267, 25, 270, 25, 0xF602);
        tft.drawLine(266, 26, 269, 26, 0xF602);
        tft.drawLine(276, 22, 279, 22, 0xF602);
        tft.drawLine(275, 23, 278, 23, 0xF602);
        tft.drawLine(274, 24, 277, 24, 0xF602);
        tft.drawLine(273, 25, 276, 25, 0xF602);
        tft.drawLine(272, 26, 275, 26, 0xF602);
        tft.drawLine(282, 22, 285, 22, 0xF602);
        tft.drawLine(281, 23, 284, 23, 0xF602);
        tft.drawLine(280, 24, 283, 24, 0xF602);
        tft.drawLine(279, 25, 282, 25, 0xF602);
        tft.drawLine(278, 26, 281, 26, 0xF602);
        tft.drawLine(298, 12, 195, 12, 0xF602);
        tft.drawLine(194, 13, 185, 22, 0xF602);
        tft.drawLine(190, 16, 173, 16, 0xF602);
        tft.drawLine(191, 11, 187, 15, 0xF602);
        tft.drawLine(174, 17, 146, 17, 0xF602);
        tft.drawLine(184, 22, 137, 22, 0xF602);
        tft.drawLine(136, 23, 141, 23, 0xF602);
        tft.drawLine(135, 24, 138, 24, 0xF602);
        tft.drawLine(137, 25, 78, 25, 0xF602);
        tft.drawLine(136, 26, 77, 26, 0xF602);
        tft.drawLine(299, 13, 300, 13, 0xF602);
        tft.drawLine(305, 19, 305, 20, 0xF602);
        tft.drawLine(312, 47, 324, 47, 0xF602);
        tft.drawLine(326, 45, 346, 25, 0xF602);
        tft.drawLine(193, 10, 186, 10, 0xF602);
        tft.drawLine(229, 8, 21, 8, 0xF602);
        tft.drawLine(20, 9, 2, 27, 0xF602);
        tft.drawLine(21, 9, 3, 27, 0xF602);
        tft.drawLine(22, 9, 220, 9, 0xF602);
        tft.drawLine(165, 10, 165, 13, 0xF602);
        tft.drawLine(166, 13, 183, 13, 0xF602);
        tft.drawLine(184, 12, 185, 11, 0xF602);
        tft.drawLine(160, 10, 156, 14, 0xF602);
        tft.drawLine(155, 14, 145, 14, 0xF602);
        tft.drawLine(112, 10, 104, 18, 0xF602);
        tft.drawLine(111, 10, 103, 18, 0xF602);
        tft.drawLine(119, 10, 116, 13, 0xF602);
        tft.drawLine(130, 10, 120, 20, 0xF602);
        tft.drawLine(121, 20, 127, 20, 0xF602);
        tft.drawLine(128, 20, 134, 14, 0xF602);
        tft.drawLine(125, 10, 115, 20, 0xF602);
        tft.drawLine(114, 20, 24, 20, 0xF602);
        tft.drawLine(23, 19, 115, 19, 0xF602);
        tft.drawLine(116, 18, 124, 10, 0xF602);
        tft.drawLine(25, 13, 102, 13, 0xF602);
        tft.drawLine(101, 14, 24, 14, 0xF602);
        tft.drawLine(100, 15, 23, 15, 0xF602);
        tft.drawLine(22, 20, 9, 33, 0xF602);
        tft.drawLine(23, 20, 10, 33, 0xF602);
        tft.drawLine(21, 10, 4, 27, 0xF602);
        tft.drawLine(23, 21, 11, 33, 0xF602);
        tft.drawLine(25, 34, 69, 34, 0xF602);
        tft.drawLine(70, 33, 76, 27, 0xF602);
        tft.drawLine(70, 34, 77, 27, 0xF602);
        tft.drawLine(71, 34, 78, 27, 0xF602);
        tft.drawLine(70, 35, 25, 35, 0xF602);
        tft.drawLine(24, 36, 14, 46, 0xF602);
        tft.drawLine(24, 35, 19, 40, 0xF602);
        tft.drawLine(14, 47, 14, 90, 0xF602);
        tft.drawLine(9, 34, 9, 62, 0xF602);
        tft.drawLine(10, 34, 10, 65, 0xF602);
        tft.drawLine(9, 63, 5, 67, 0xF602);
        tft.drawLine(9, 64, 5, 68, 0xF602);
        tft.drawLine(9, 65, 5, 69, 0xF602);
        tft.drawLine(9, 66, 5, 70, 0xF602);
        tft.drawLine(4, 28, 4, 148, 0xF602);
        tft.drawLine(3, 28, 3, 147, 0xF602);
        tft.drawLine(5, 137, 5, 149, 0xF602);
        tft.fillRect(6, 140, 3, 114, 0xF602);
        tft.drawRect(6, 138, 2, 2, 0xF602);
        tft.drawLine(2, 262, 2, 28, 0xF602);
        tft.drawLine(10, 73, 10, 76, 0xF602);
        tft.drawLine(9, 74, 9, 77, 0xF602);
        tft.drawLine(8, 75, 8, 78, 0xF602);
        tft.drawLine(7, 76, 7, 79, 0xF602);
        tft.drawLine(10, 83, 10, 86, 0xF602);
        tft.drawLine(9, 84, 9, 87, 0xF602);
        tft.drawLine(8, 85, 8, 88, 0xF602);
        tft.drawLine(7, 86, 7, 89, 0xF602);
        tft.drawLine(13, 91, 8, 96, 0xF602);
        tft.drawLine(14, 91, 9, 96, 0xF602);
        tft.drawRect(7, 97, 2, 37, 0xF602);
        tft.drawLine(8, 134, 8, 135, 0xF602);
        tft.drawRect(9, 268, 2, 21, 0xF602);
        tft.drawLine(9, 267, 3, 261, 0xF602);
        tft.drawLine(8, 267, 3, 262, 0xF602);
        tft.drawLine(8, 268, 3, 263, 0xF602);
        tft.drawRect(29, 306, 76, 2, 0xF602);
        tft.drawLine(11, 288, 28, 305, 0xF602);
        tft.drawLine(11, 289, 28, 306, 0xF602);
        tft.drawLine(10, 289, 28, 307, 0xF602);
        tft.drawLine(101, 297, 109, 297, 0xF602);
        tft.drawLine(102, 298, 110, 298, 0xF602);
        tft.drawLine(103, 299, 111, 299, 0xF602);
        tft.drawLine(104, 300, 112, 300, 0xF602);
        tft.drawLine(105, 301, 113, 301, 0xF602);
        tft.drawLine(106, 302, 114, 302, 0xF602);
        tft.drawLine(107, 303, 115, 303, 0xF602);
        tft.drawLine(108, 304, 116, 304, 0xF602);
        tft.drawLine(109, 305, 117, 305, 0xF602);
        tft.drawLine(110, 306, 118, 306, 0xF602);
        tft.drawLine(111, 307, 119, 307, 0xF602);
        tft.drawLine(112, 308, 120, 308, 0xF602);
        tft.drawLine(113, 309, 121, 309, 0xF602);
        tft.drawLine(114, 310, 122, 310, 0xF602);
        tft.drawLine(115, 297, 123, 297, 0xF602);
        tft.drawLine(116, 298, 124, 298, 0xF602);
        tft.drawLine(117, 299, 125, 299, 0xF602);
        tft.drawLine(118, 300, 126, 300, 0xF602);
        tft.drawLine(119, 301, 127, 301, 0xF602);
        tft.drawLine(120, 302, 128, 302, 0xF602);
        tft.drawLine(121, 303, 129, 303, 0xF602);
        tft.drawLine(122, 304, 130, 304, 0xF602);
        tft.drawLine(123, 305, 131, 305, 0xF602);
        tft.drawLine(124, 306, 132, 306, 0xF602);
        tft.drawLine(125, 307, 133, 307, 0xF602);
        tft.drawLine(126, 308, 134, 308, 0xF602);
        tft.drawLine(127, 309, 135, 309, 0xF602);
        tft.drawLine(128, 310, 136, 310, 0xF602);
        tft.drawLine(129, 297, 137, 297, 0xF602);
        tft.drawLine(130, 298, 138, 298, 0xF602);
        tft.drawLine(131, 299, 139, 299, 0xF602);
        tft.drawLine(132, 300, 140, 300, 0xF602);
        tft.drawLine(133, 301, 141, 301, 0xF602);
        tft.drawLine(134, 302, 142, 302, 0xF602);
        tft.drawLine(135, 303, 143, 303, 0xF602);
        tft.drawLine(136, 304, 144, 304, 0xF602);
        tft.drawLine(137, 305, 145, 305, 0xF602);
        tft.drawLine(138, 306, 146, 306, 0xF602);
        tft.drawLine(139, 307, 147, 307, 0xF602);
        tft.drawLine(140, 308, 148, 308, 0xF602);
        tft.drawLine(141, 309, 149, 309, 0xF602);
        tft.drawLine(142, 310, 150, 310, 0xF602);
        tft.drawLine(143, 297, 151, 297, 0xF602);
        tft.drawLine(144, 298, 152, 298, 0xF602);
        tft.drawLine(145, 299, 153, 299, 0xF602);
        tft.drawLine(146, 300, 154, 300, 0xF602);
        tft.drawLine(147, 301, 155, 301, 0xF602);
        tft.drawLine(148, 302, 156, 302, 0xF602);
        tft.drawLine(149, 303, 157, 303, 0xF602);
        tft.drawLine(150, 304, 158, 304, 0xF602);
        tft.drawLine(151, 305, 159, 305, 0xF602);
        tft.drawLine(152, 306, 160, 306, 0xF602);
        tft.drawLine(153, 307, 161, 307, 0xF602);
        tft.drawLine(154, 308, 162, 308, 0xF602);
        tft.drawLine(155, 309, 163, 309, 0xF602);
        tft.drawLine(156, 310, 164, 310, 0xF602);
        tft.drawLine(227, 309, 271, 309, 0xF602);
        tft.drawLine(272, 308, 285, 295, 0xF602);
        tft.drawLine(208, 297, 220, 309, 0xF602);
        tft.drawLine(157, 297, 170, 310, 0xF602);
        tft.drawLine(158, 297, 171, 310, 0xF602);
        tft.drawLine(159, 297, 172, 310, 0xF602);
        tft.drawLine(160, 297, 173, 310, 0xF602);
        tft.drawLine(161, 297, 174, 310, 0xF602);
        tft.drawLine(162, 297, 175, 310, 0xF602);
        tft.drawLine(163, 297, 176, 310, 0xF602);
        tft.drawLine(164, 297, 177, 310, 0xF602);
        tft.drawLine(180, 298, 191, 309, 0xF602);
        tft.drawLine(181, 298, 192, 309, 0xF602);
        tft.drawLine(182, 298, 193, 309, 0xF602);
        tft.drawLine(183, 298, 194, 309, 0xF602);
        tft.drawLine(184, 298, 195, 309, 0xF602);
        tft.drawLine(189, 298, 200, 309, 0xF602);
        tft.drawLine(190, 298, 201, 309, 0xF602);
        tft.drawLine(191, 298, 202, 309, 0xF602);
        tft.drawLine(192, 298, 203, 309, 0xF602);
        tft.drawLine(193, 298, 204, 309, 0xF602);
        tft.drawLine(198, 298, 209, 309, 0xF602);
        tft.drawLine(199, 298, 210, 309, 0xF602);
        tft.drawLine(200, 298, 211, 309, 0xF602);
        tft.drawLine(201, 298, 212, 309, 0xF602);
        tft.drawLine(202, 298, 213, 309, 0xF602);
        tft.drawLine(278, 309, 289, 298, 0xF602);
        tft.drawLine(279, 309, 290, 298, 0xF602);
        tft.drawLine(280, 309, 291, 298, 0xF602);
        tft.drawLine(281, 309, 292, 298, 0xF602);
        tft.drawLine(282, 309, 293, 298, 0xF602);
        tft.drawLine(283, 309, 294, 298, 0xF602);
        tft.drawLine(288, 309, 299, 298, 0xF602);
        tft.drawLine(289, 309, 300, 298, 0xF602);
        tft.drawLine(290, 309, 301, 298, 0xF602);
        tft.drawLine(291, 309, 302, 298, 0xF602);
        tft.drawLine(297, 309, 308, 298, 0xF602);
        tft.drawLine(298, 309, 309, 298, 0xF602);
        tft.drawLine(299, 309, 310, 298, 0xF602);
        tft.drawLine(306, 309, 317, 298, 0xF602);
        tft.drawLine(307, 309, 318, 298, 0xF602);
        tft.drawLine(324, 302, 317, 309, 0xF602);
        tft.drawLine(325, 302, 318, 309, 0xF602);
        tft.fillRect(319, 307, 129, 3, 0xF602);
        tft.drawRect(324, 302, 123, 2, 0xF602);
        tft.drawRect(373, 297, 71, 2, 0xF602);
        tft.drawLine(372, 298, 369, 301, 0xF602);
        tft.drawLine(370, 301, 372, 299, 0xF602);
        tft.drawLine(371, 301, 373, 299, 0xF602);
        tft.fillRect(474, 57, 3, 223, 0xF602);
        tft.drawLine(474, 279, 445, 308, 0xF602);
        tft.drawLine(448, 306, 474, 280, 0xF602);
        tft.drawLine(448, 307, 475, 280, 0xF602);
        tft.drawLine(448, 308, 476, 280, 0xF602);
        tft.drawLine(444, 298, 467, 275, 0xF602);
        tft.drawLine(444, 297, 466, 275, 0xF602);
        tft.drawLine(444, 296, 465, 275, 0xF602);
        tft.drawLine(443, 296, 464, 275, 0xF602);
        tft.fillRect(465, 73, 3, 202, 0xF602);
        tft.drawLine(465, 73, 470, 68, 0xF602);
        tft.drawLine(470, 69, 467, 72, 0xF602);
        tft.drawLine(470, 70, 468, 72, 0xF602);
        tft.drawLine(470, 71, 468, 73, 0xF602);
        tft.drawLine(470, 72, 468, 74, 0xF602);
        tft.drawLine(470, 73, 468, 75, 0xF602);
        tft.drawLine(470, 74, 468, 76, 0xF602);
    }

    if (clock_set) {
        setTftDisplay(26, 43, bruceConfig.priColor, 1, bruceConfig.bgColor);
#if defined(HAS_RTC)
        _rtc.GetTime(&_time);
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", _time.Hours, _time.Minutes);
        tft.print(timeStr);
#else
        updateTimeStr(rtc.getTimeStruct());
        tft.print(timeStr);
#endif
    } else {
        setTftDisplay(26, 43, bruceConfig.priColor, 1, bruceConfig.bgColor);
        tft.print("DAEMON " + String(BRUCE_VERSION));
    }
}

void drawMainBorder(bool clear) {
    if (clear) {
        tft.drawPixel(0, 0, 0);
        tft.fillScreen(bruceConfig.bgColor);
    }
    setTftDisplay(12, 12, bruceConfig.priColor, 1, bruceConfig.bgColor);
    tft.setTextDatum(0);

    // if(wifiConnected) {tft.print(timeStr);} else {tft.print("BRUCE 1.0b");}

    drawStatusBar();

#if defined(HAS_TOUCH)
    TouchFooter();
#endif
}

void drawMainBorderWithTitle(String title, bool clear) {
    drawMainBorder(clear);
    printTitle(title);
}

void printTitle(String title) {
    tft.setCursor((tftWidth - (title.length() * FM * LW)) / 2, BORDER_PAD_Y);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FM);

    title.toUpperCase();
    tft.println(title);

    tft.setTextSize(FP);
}

void printSubtitle(String subtitle, bool withLine) {
    int16_t cursorX = (tftWidth - (subtitle.length() * FP * LW)) / 2;
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FP);

    tft.setCursor(cursorX, BORDER_PAD_Y + FM * LH);
    tft.println(subtitle);

    if (withLine) {
        String line = "";
        for (byte i = 0; i < subtitle.length(); i++) line += "-";

        tft.setCursor(cursorX, tft.getCursorY());
        tft.println(line);
    }
}

void printFootnote(String text) {
    tft.setTextSize(FP);
    tft.drawRightString(text, tftWidth - BORDER_PAD_X, tftHeight - BORDER_PAD_X - FP * LH, SMOOTH_FONT);
}

void printCenterFootnote(String text) {
    tft.fillRect(10, tftHeight - BORDER_PAD_X - FP * LH, tftWidth - 20, FP * LH, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.drawCentreString(text, tftWidth / 2, tftHeight - BORDER_PAD_X - FP * LH, SMOOTH_FONT);
}

/***************************************************************************************
** Function name: getBattery()
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() {
    int percent = 0;

    return (percent < 0) ? 0 : (percent >= 100) ? 100 : percent;
}

/***************************************************************************************
** Function name: drawBatteryStatus()
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
void drawBatteryStatus(uint8_t bat) {
    if (bat == 0) return;

    bool charging = isCharging();

    uint16_t color = bruceConfig.priColor;
    uint16_t barcolor = bruceConfig.priColor;
    if (bat < 16) barcolor = color = TFT_RED;
    else if (bat < 34) barcolor = color = TFT_YELLOW;
    if (charging) color = TFT_GREEN;

    tft.drawRoundRect(tftWidth - 43, 6, 36, 19, 2, charging ? color : bruceConfig.bgColor); // (bolder border)
    tft.drawRoundRect(tftWidth - 42, 7, 34, 17, 2, color);
    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawRightString((bat == 100 ? "" : " ") + String(bat) + "%", tftWidth - 45, 12, 1);
    tft.fillRoundRect(tftWidth - 40, 9, 30 * bat / 100, 13, 2, barcolor);
    tft.drawLine(tftWidth - 30, 9, tftWidth - 30, 9 + 13, bruceConfig.bgColor);
    tft.drawLine(tftWidth - 20, 9, tftWidth - 20, 9 + 13, bruceConfig.bgColor);
}
/***************************************************************************************
** Function name: drawWireguardStatus()
** Description:   Draws a padlock when connected
***************************************************************************************/
void drawWireguardStatus(int x, int y) {
    tft.fillRect(x, y, 20, 17, bruceConfig.bgColor);
    if (isConnectedWireguard) {
        tft.drawRoundRect(11 + x, 0 + y, 8, 12, 5, TFT_GREEN);
        tft.fillRoundRect(10 + x, 8 + y, 10, 8, 0, TFT_GREEN);
    } else {
        tft.drawRoundRect(1 + x, 0 + y, 8, 12, 5, bruceConfig.priColor);
        tft.fillRoundRect(0 + x, 8 + y, 10, 8, 0, bruceConfig.bgColor);
        tft.fillRoundRect(6 + x, 8 + y, 10, 10, 0, bruceConfig.priColor);
    }
}

/***************************************************************************************
** Function name: listFiles
** Description:   Função para desenhar e mostrar o menu principal
***************************************************************************************/
#define MAX_ITEMS (int)(tftHeight - 20) / (LH * FM)
Opt_Coord listFiles(int index, std::vector<FileList> fileList) {
    Opt_Coord coord;
    tft.drawPixel(0, 0, bruceConfig.bgColor);
    if (index == 0) {
        tft.fillScreen(bruceConfig.bgColor);
        tft.drawRoundRect(5, 5, tftWidth - 10, tftHeight - 10, 5, bruceConfig.priColor);
    }
    tft.setCursor(10, 10);
    tft.setTextSize(FM);
    int i = 0;
    int arraySize = fileList.size();
    int start = 0;
    if (index >= MAX_ITEMS) {
        start = index - MAX_ITEMS + 1;
        if (start < 0) start = 0;
    }
    int nchars = (tftWidth - 20) / (6 * tft.textsize);
    String txt = ">";
    while (i < arraySize) {
        if (i >= start) {
            tft.setCursor(10, tft.getCursorY());
            if (fileList[i].folder == true)
                tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
            else if (fileList[i].operation == true) tft.setTextColor(ALCOLOR, bruceConfig.bgColor);
            else { tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor); }

            if (index == i) {
                txt = ">";
                coord.x = 10 + FM * LW;
                coord.y = tft.getCursorY();
                coord.size = nchars;
                coord.fgcolor =
                    fileList[i].folder ? getColorVariation(bruceConfig.priColor) : bruceConfig.priColor;
                coord.bgcolor = bruceConfig.bgColor;
            } else txt = " ";
            txt += fileList[i].filename + "                 ";
            tft.println(txt.substring(0, nchars));
        }
        i++;
        if (i == (start + MAX_ITEMS) || i == arraySize) break;
    }
    return coord;
}

// desenhos do menu principal, sprite "draw" com 80x80 pixels

void drawWifiSmall(int x, int y) {
    tft.fillRect(x, y, 16, 16, bruceConfig.bgColor);
    tft.fillCircle(9 + x, 14 + y, 1, bruceConfig.priColor);
    tft.drawArc(9 + x, 14 + y, 4, 6, 130, 230, bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawArc(9 + x, 14 + y, 10, 12, 130, 230, bruceConfig.priColor, bruceConfig.bgColor);
}

void drawWebUISmall(int x, int y) {
    tft.fillRect(x, y, 16, 16, bruceConfig.bgColor);

    tft.drawCircle(8 + x, 8 + y, 7, bruceConfig.priColor);

    tft.drawLine(3 + x, 4 + y, 14 + x, 4 + y, bruceConfig.priColor);
    tft.drawLine(2 + x, 8 + y, 15 + x, 8 + y, bruceConfig.priColor);
    tft.drawLine(3 + x, 12 + y, 14 + x, 12 + y, bruceConfig.priColor);
}

void drawBLESmall(int x, int y) {
    tft.fillRect(x, 2 + y, 17, 13, bruceConfig.bgColor);
    tft.drawWideLine(8 + x, 8 + y, 4 + x, 5 + y, 2, bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawWideLine(8 + x, 8 + y, 4 + x, 13 + y, 2, bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawTriangle(8 + x, 8 + y, 8 + x, 2 + y, 13 + x, 5 + y, bruceConfig.priColor);
    tft.drawTriangle(8 + x, 8 + y, 8 + x, 14 + y, 13 + x, 11 + y, bruceConfig.priColor);
}

void drawBLE_beacon(int x, int y, uint16_t color) {
    tft.fillRect(x, y, 40, 80, bruceConfig.bgColor);
    tft.drawWideLine(40 + x, 53 + y, 2 + x, 26 + y, 5, color, bruceConfig.bgColor);
    tft.drawWideLine(40 + x, 26 + y, 2 + x, 53 + y, 5, color, bruceConfig.bgColor);
    tft.drawWideLine(40 + x, 53 + y, 20 + x, 68 + y, 5, color, bruceConfig.bgColor);
    tft.drawWideLine(40 + x, 26 + y, 20 + x, 12 + y, 5, color, bruceConfig.bgColor);
    tft.drawWideLine(20 + x, 12 + y, 20 + x, 68 + y, 5, color, bruceConfig.bgColor);
    tft.fillTriangle(40 + x, 26 + y, 20 + x, 40 + y, 20 + x, 12 + y, color);
    tft.fillTriangle(40 + x, 53 + y, 20 + x, 40 + y, 20 + x, 68 + y, color);
}

void drawGPS(int x, int y) {
    tft.fillRect(x, y, 80, 80, bruceConfig.bgColor);
    tft.drawEllipse(40 + x, 70 + y, 15, 8, bruceConfig.priColor);
    tft.drawArc(40 + x, 25 + y, 23, 7, 0, 340, bruceConfig.priColor, bruceConfig.bgColor);
    tft.fillTriangle(40 + x, 70 + y, 20 + x, 64 + y, 60 + x, 64 + y, bruceConfig.priColor);
}

void drawGpsSmall(int x, int y) {
    tft.fillRect(x, y, 17, 17, bruceConfig.bgColor);
    tft.drawEllipse(9 + x, 14 + y, 4, 3, bruceConfig.priColor);
    tft.drawArc(9 + x, 6 + y, 5, 2, 0, 340, bruceConfig.priColor, bruceConfig.bgColor);
    tft.fillTriangle(9 + x, 15 + y, 5 + x, 9 + y, 13 + x, 9 + y, bruceConfig.priColor);
}

void drawCreditCard(int x, int y) {
    tft.fillRect(x, y, 70, 50, bruceConfig.bgColor);
    tft.fillRoundRect(x + 5, y + 5, 60, 40, 5, bruceConfig.priColor);
    tft.fillRect(x + 5, y + 15, 60, 10, getColorVariation(bruceConfig.priColor, 3, -1));
    tft.fillRect(x + 10, y + 30, 12, 10, getColorVariation(bruceConfig.priColor, 3, 1));
    tft.drawRect(x + 10, y + 30, 12, 10, getColorVariation(bruceConfig.priColor, 5, -1));
    tft.drawRect(x + 10 + 4, y + 30, 4, 10, getColorVariation(bruceConfig.priColor, 5, -1));
    tft.drawRect(x + 10, y + 33, 5, 4, getColorVariation(bruceConfig.priColor, 5, -1));
    tft.drawRect(x + 17, y + 33, 5, 4, getColorVariation(bruceConfig.priColor, 5, -1));
    tft.fillRect(x + 30, y + 35, 30, 5, getColorVariation(bruceConfig.priColor, 5, 1));
}

void drawMfkey32Icon(int x, int y) {
    tft.drawRect(x + 2, y + 15, 24, 40, bruceConfig.priColor);
    tft.drawRect(x + 5, y + 18, 18, 12, bruceConfig.priColor);
    tft.drawRect(x + 5, y + 34, 18, 18, bruceConfig.priColor);
    tft.drawLine(x + 5, y + 40, x + 22, y + 40, bruceConfig.priColor);
    tft.drawLine(x + 5, y + 46, x + 22, y + 46, bruceConfig.priColor);
    tft.drawLine(x + 11, y + 34, x + 11, y + 51, bruceConfig.priColor);
    tft.drawLine(x + 17, y + 34, x + 17, y + 51, bruceConfig.priColor);
    tft.drawRect(x + 30, y + 10, 25, 35, bruceConfig.priColor);
    int startX = x + 32;
    int startY = y + 12;
    int endX = x + 52;
    int endY = y + 32;
    int step = 2;
    int turns = 0;

    while (startX <= endX && startY <= endY && turns < 3) {
        for (int i = startX; i <= endX; i++) { tft.drawPixel(i, startY, bruceConfig.priColor); }
        startY += step;
        for (int i = startY; i <= endY; i++) { tft.drawPixel(endX, i, bruceConfig.priColor); }
        endX -= step;
        for (int i = endX; i >= startX; i--) { tft.drawPixel(i, endY, bruceConfig.priColor); }
        endY -= step;
        for (int i = endY; i >= startY; i--) { tft.drawPixel(startX, i, bruceConfig.priColor); }
        startX += step;
        turns++;
    }
    tft.fillRect(x + 40, y + 36, 6, 6, getColorVariation(bruceConfig.priColor, 3, 1));
}

void drawMfkey64Icon(int x, int y) {
    drawMfkey32Icon(x, y);
    tft.fillRoundRect(x + 40, y + 6, 24, 14, 4, bruceConfig.bgColor);
    tft.drawRoundRect(x + 40, y + 6, 24, 14, 4, getColorVariation(bruceConfig.priColor, 3, -1));
    tft.drawCircle(x + 48, y + 12, 4, getColorVariation(bruceConfig.priColor, 3, -1));
}

// ####################################################################################################
//  Draw a JPEG on the TFT, images will be cropped on the right/bottom sides if they do not fit
// ####################################################################################################
//  from:
//  https://github.com/Bodmer/TFT_eSPI/blob/master/examples/Generic/ESP32_SDcard_jpeg/ESP32_SDcard_jpeg.ino
//  This function assumes xpos,ypos is a valid screen coordinate. For convenience images that do not
//  fit totally on the screen are cropped to the nearest MCU size and may leave right/bottom borders.
void jpegRender(int xpos, int ypos) {

    // jpegInfo(); // Print information from the JPEG file (could comment this line out)

    uint16_t *pImg;
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;
    uint32_t max_x = JpegDec.width;
    uint32_t max_y = JpegDec.height;

    bool swapBytes = tft.getSwapBytes();
    tft.setSwapBytes(true);

    // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
    // Typically these MCUs are 16x16 pixel blocks
    // Determine the width and height of the right and bottom edge image blocks
    uint32_t min_w = jpg_min(mcu_w, max_x % mcu_w);
    uint32_t min_h = jpg_min(mcu_h, max_y % mcu_h);

    // save the current image block size
    uint32_t win_w = mcu_w;
    uint32_t win_h = mcu_h;

    // save the coordinate of the right and bottom edges to assist image cropping
    // to the screen size
    max_x += xpos;
    max_y += ypos;

    // Fetch data from the file, decode and display
    tft.fillRect(xpos, ypos, JpegDec.width, JpegDec.height, TFT_BLACK);
    while (JpegDec.read()) {   // While there is more data in the file
        pImg = JpegDec.pImage; // Decode a MCU (Minimum Coding Unit, typically a 8x8 or 16x16 pixel block)

        // Calculate coordinates of top left corner of current MCU
        int mcu_x = JpegDec.MCUx * mcu_w + xpos;
        int mcu_y = JpegDec.MCUy * mcu_h + ypos;

        // check if the image block size needs to be changed for the right edge
        if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
        else win_w = min_w;

        // check if the image block size needs to be changed for the bottom edge
        if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
        else win_h = min_h;

        // copy pixels into a contiguous block
        if (win_w != mcu_w) {
            uint16_t *cImg;
            int p = 0;
            cImg = pImg + win_w;
            for (int h = 1; h < win_h; h++) {
                p += mcu_w;
                for (int w = 0; w < win_w; w++) {
                    *cImg = *(pImg + w + p);
                    cImg++;
                }
            }
        }

        // calculate how many pixels must be drawn
        uint32_t mcu_pixels = win_w * win_h;

        // draw image MCU block only if it will fit on the screen
        if ((mcu_x + win_w) <= tft.width() && (mcu_y + win_h) <= tft.height())
            tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
        else if ((mcu_y + win_h) > tft.height())
            JpegDec.abort(); // Image has run off bottom of screen so abort decoding
    }

    tft.setSwapBytes(swapBytes);
}

bool showJpeg(FS &fs, String filename, int x, int y, bool center) {
    // record the current time so we can measure how long it takes to draw an image
    uint32_t drawTime = millis();
    File picture;
    if (fs.exists(filename)) picture = fs.open(filename, FILE_READ);
    else return false;

    const size_t data_size = picture.size();

    // Alloc memory into heap
    uint8_t *data_array = new uint8_t[data_size];
    if (data_array == nullptr) {
        // Fail allocating memory
        picture.close();
        delete[] data_array;
        return false;
    }

    uint8_t data;
    int i = 0;
    byte line_len = 0;

    while (picture.available()) {
        data = picture.read();
        data_array[i] = data;
        i++;

        // print array on Serial
        /*
        Serial.print("0x");
        if (abs(data) < 16) {
          Serial.print("0");
        }

        Serial.print(data, HEX);
        Serial.print(","); // Add value and comma
        line_len++;
        if (line_len >= 32) {
          line_len = 0;
          Serial.println();
        }
        */
    }

    picture.close();

    bool decoded = false;
    if (data_array) {
        decoded = JpegDec.decodeArray(data_array, data_size);
    } else {
        displayError(filename + " Fail");
        delay(2500);
        delete[] data_array; // free heap before leaving
        return false;
    }

    if (decoded) {
        if (center) {
            x = x + (tftWidth - JpegDec.width) / 2;
            y = y + (tftHeight - JpegDec.height) / 2;
        }
        jpegRender(x, y);
    }
    // calculate how long it took to draw the image
    drawTime = millis() - drawTime; // Calculate the time it took

    // print the results to the serial port
    Serial.print("Total render time was    : ");
    Serial.print(drawTime);
    Serial.println(" ms");
    Serial.println("=====================================");

    delete[] data_array; // free heap before leaving
    return true;
}

#if !defined(LITE_VERSION)
// ####################################################################################################
//  Draw a GIF on the TFT
//  derived from
//  https://github.com/bitbank2/AnimatedGIF/blob/master/examples/TFT_eSPI_memory/TFT_eSPI_memory.ino and
//  https://github.com/bitbank2/AnimatedGIF/blob/master/examples/best_practices_example/best_practices_example.ino
// ####################################################################################################

Gif::Gif() : gifPosition(0, 0) {}

Gif::~Gif() {
    gif->close();
    delete gif;
}

FS *Gif::GifFs = NULL;

void *Gif::openFile(const char *fname, int32_t *pSize) {
    File GifFile;

    if (GifFs != NULL) {
        GifFile = GifFs->open(fname);
    } else {
        if (SD.exists(fname)) GifFile = SD.open(fname);
        else if (LittleFS.exists(fname)) GifFile = LittleFS.open(fname);
    }

    File *FSGifFile = new File(GifFile);

    if (FSGifFile) {
        *pSize = FSGifFile->size();
        return (void *)FSGifFile;
    }
    return NULL;
}

void Gif::closeFile(void *pHandle) {
    File *f = static_cast<File *>(pHandle);
    if (f != NULL) { f->close(); }
    delete f;
}

int32_t Gif::readFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
    int32_t iBytesRead;
    iBytesRead = iLen;
    File *f = static_cast<File *>(pFile->fHandle);
    // Note: If you read a file all the way to the last byte, seek() stops working
    if ((pFile->iSize - pFile->iPos) < iLen)
        iBytesRead = pFile->iSize - pFile->iPos - 1; // <-- ugly work-around
    if (iBytesRead <= 0) return 0;
    iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
    pFile->iPos = f->position();
    return iBytesRead;
}

int32_t Gif::seekFile(GIFFILE *pFile, int32_t iPosition) {
    int i = micros();
    File *f = static_cast<File *>(pFile->fHandle);
    f->seek(iPosition);
    pFile->iPos = (int32_t)f->position();
    i = micros() - i;
    return pFile->iPos;
}

void Gif::GIFDraw(GIFDRAW *pDraw) {
    uint8_t *s;
    uint16_t *d, *usPalette, usTemp[tftWidth];
    int x, y, iWidth;

    GifPosition *position = (GifPosition *)(pDraw->pUser);

    iWidth = pDraw->iWidth;
    if (iWidth > tftWidth) iWidth = tftWidth;
    usPalette = pDraw->pPalette;
    y = pDraw->iY + pDraw->y; // current line

    s = pDraw->pPixels;
    if (pDraw->ucDisposalMethod == 2) { // restore to background color
        for (x = 0; x < iWidth; x++) {
            if (s[x] == pDraw->ucTransparent) s[x] = pDraw->ucBackground;
        }
        pDraw->ucHasTransparency = 0;
    }
    // Apply the new pixels to the main image
    if (pDraw->ucHasTransparency) { // if transparency used
        uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
        int x, iCount;
        pEnd = s + iWidth;
        x = 0;
        iCount = 0; // count non-transparent pixels
        while (x < iWidth) {
            c = ucTransparent - 1;
            d = usTemp;
            while (c != ucTransparent && s < pEnd) {
                c = *s++;
                if (c == ucTransparent) { // done, stop
                    s--;                  // back up to treat it like transparent
                } else {                  // opaque
                    *d++ = usPalette[c];
                    iCount++;
                }
            } // while looking for opaque pixels
            if (iCount) { // any opaque pixels?
                tft.drawPixel(0, 0, 0);
                tft.pushImage(pDraw->iX + x + position->x, y + position->y, iCount, 1, (uint16_t *)usTemp);
                x += iCount;
                iCount = 0;
            }
            // no, look for a run of transparent pixels
            c = ucTransparent;
            while (c == ucTransparent && s < pEnd) {
                c = *s++;
                if (c == ucTransparent) iCount++;
                else s--;
            }
            if (iCount) {
                x += iCount; // skip these
                iCount = 0;
            }
        }
    } else {
        s = pDraw->pPixels;
        // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
        for (x = 0; x < iWidth; x++) usTemp[x] = usPalette[*s++];
        tft.drawPixel(0, 0, 0);
        tft.pushImage(pDraw->iX + position->x, y + position->y, iWidth, 1, (uint16_t *)usTemp);
    }
} /* GIFDraw() */

bool Gif::openGIF(FS *fs, const char *filename) {
    if (fs != NULL) {
        GifFs = fs;
        if (!fs->exists(filename)) return false;
    } else {
        GifFs = NULL;
    }

    gif = new AnimatedGIF();
    gif->begin(BIG_ENDIAN_PIXELS);
    if (gif->open(filename, openFile, closeFile, readFile, seekFile, GIFDraw)) { return true; }

    log_e("GIF opening error: %d\n", gif->getLastError());
    return false;
}

// Play a single frame
// returns:
// 2 = skipped waiting for another frame
// 1 = good result and more frames exist
// 0 = no more frames exist, a frame may or may not have been played: use getLastError() and look for
// GIF_SUCCESS to know if a frame was played -1 = error
int Gif::playFrame(int x, int y, bool bSync) {
    if (bSync && ((millis() - lTime) >= *delayMilliseconds)) {
        lTime = millis();
        gifPosition.x = x;
        gifPosition.y = y;
        return gif->playFrame(false, delayMilliseconds, &gifPosition);
    }

    return 2;
}

int Gif::getLastError() { return gif->getLastError(); }

/*
 * playDurationMs:
 *  -1 : Play the GIF in an infinite loop
 *  0  : Play the GIF once
 * >0  : Play the GIF for the specified duration in milliseconds
 *       (e.g., 1000 = play for 1 second)
 */
bool showGif(FS *fs, const char *filename, int x, int y, bool center, int playDurationMs) {
    if (!fs->exists(filename)) return false;

    Gif gif;
    bool success = gif.openGIF(fs, filename);
    if (!success) { return false; }

    if (center) {
        x = x + (tftWidth - gif.getCanvasWidth()) / 2;
        y = y + (tftHeight - gif.getCanvasHeight()) / 2;
    }

    int result = 0;
    long timeStart = millis();
    do {
        result = gif.playFrame(x, y);
        if (result == -1) log_e("GIF playFrame error: %d\n", gif.getLastError());

        if (check(AnyKeyPress)) break;

        if (playDurationMs > 0 && (millis() - timeStart) > playDurationMs) break;
        if (playDurationMs == 0 && result == 0) break;
    } while (result >= 0);

    return true;
}
#endif
/***************************************************************************************
** Function name: getComplementaryColor2
** Description:   Get simple complementary color in RGB565 format
***************************************************************************************/
uint16_t getComplementaryColor2(uint16_t color) {
    int r = 31 - ((color >> 11) & 0x1F);
    int g = 63 - ((color >> 5) & 0x3F);
    int b = 31 - (color & 0x1F);
    return (r << 11) | (g << 5) | b;
}
/***************************************************************************************
** Function name: getComplementaryColor
** Description:   Get complementary color in RGB565 format
***************************************************************************************/
uint16_t getComplementaryColor(uint16_t color) {
    double r = ((color >> 11) & 0x1F) / 31.0;
    double g = ((color >> 5) & 0x3F) / 63.0;
    double b = (color & 0x1F) / 31.0;

    double cmax = fmax(r, fmax(g, b));
    double cmin = fmin(r, fmin(g, b));
    double delta = cmax - cmin;

    double hue = 0.0;
    if (delta == 0) hue = 0.0;
    else if (cmax == r) hue = 60 * fmod((g - b) / delta, 6);
    else if (cmax == g) hue = 60 * ((b - r) / delta + 2);
    else hue = 60 * ((r - g) / delta + 4);

    if (hue < 0) hue += 360;

    double lightness = (cmax + cmin) / 2;
    double saturation = (delta == 0) ? 0 : delta / (1 - std::abs(2 * lightness - 1));

    double compHue = fmod(hue + 180, 360);

    double c = (1 - std::abs(2 * lightness - 1)) * saturation;
    double x = c * (1 - std::abs(fmod(compHue / 60, 2) - 1));
    double m = lightness - c / 2;

    double compR = 0, compG = 0, compB = 0;
    if (compHue >= 0 && compHue < 60) {
        compR = c;
        compG = x;
    } else if (compHue >= 60 && compHue < 120) {
        compR = x;
        compG = c;
    } else if (compHue >= 120 && compHue < 180) {
        compG = c;
        compB = x;
    } else if (compHue >= 180 && compHue < 240) {
        compG = x;
        compB = c;
    } else if (compHue >= 240 && compHue < 300) {
        compB = c;
        compR = x;
    } else {
        compB = x;
        compR = c;
    }

    uint16_t compl_color = uint8_t(compR * 31) << 11 | uint8_t(compG * 63) << 5 | uint8_t(compB * 31);

    // change black color
    if (compl_color == 0) compl_color = color - 0x1111;

    return compl_color;
}

/***************************************************************************************
** Function name: getColorVariation
** Description:   Get a variation of color in RGB565 format
***************************************************************************************/
uint16_t getColorVariation(uint16_t color, int delta, int direction) {
    uint8_t r = ((color >> 11) & 0x1F);
    uint8_t g = ((color >> 5) & 0x3F);
    uint8_t b = (color & 0x1F);

    float brightness = 0.299 * r / 31 + 0.587 * g / 63 + 0.114 * b / 31;

    if (direction < 0 || (direction == 0 && brightness >= 0.5)) {
        r = max(0, r - delta);
        g = max(0, g - 2 * delta);
        b = max(0, b - delta);
    } else {
        r = min(31, r + delta);
        g = min(63, g + 2 * delta);
        b = min(31, b + delta);
    }

    uint16_t compl_color = r << 11 | g << 5 | b;

    return compl_color;
}

// Draw BITMAP files
// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(fs::File &f) {
    uint16_t result;
    ((uint8_t *)&result)[0] = f.read(); // LSB
    ((uint8_t *)&result)[1] = f.read(); // MSB
    return result;
}

uint32_t read32(fs::File &f) {
    uint32_t result;
    ((uint8_t *)&result)[0] = f.read(); // LSB
    ((uint8_t *)&result)[1] = f.read();
    ((uint8_t *)&result)[2] = f.read();
    ((uint8_t *)&result)[3] = f.read(); // MSB
    return result;
}
bool drawBmp(FS &fs, String filename, int x, int y, bool center) {
    if ((x >= tft.width()) || (y >= tft.height())) return false;
    uint32_t startTime = millis();

    File bmpFS;

    // Open requested file on SD card
    bmpFS = fs.open(filename, "r");

    if (!bmpFS) {
        Serial.print("File not found");
        goto ERROR;
    }

    uint32_t seekOffset;
    uint16_t w, h, row, col;
    uint8_t r, g, b;

    if (read16(bmpFS) == 0x4D42) {
        read32(bmpFS);
        read32(bmpFS);
        seekOffset = read32(bmpFS);
        read32(bmpFS);
        w = read32(bmpFS);
        h = read32(bmpFS);
        if (center) {
            x = x + (tftWidth - w) / 2;
            y = y + (tftHeight - h) / 2;
        }

        if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0)) {
            y += h - 1;

            bool oldSwapBytes = tft.getSwapBytes();
            tft.setSwapBytes(true);
            bmpFS.seek(seekOffset);

            uint16_t padding = (4 - ((w * 3) & 3)) & 3;
            uint8_t lineBuffer[w * 3 + padding];

            for (row = 0; row < h; row++) {

                bmpFS.read(lineBuffer, sizeof(lineBuffer));
                uint8_t *bptr = lineBuffer;
                uint16_t *tptr = (uint16_t *)lineBuffer;
                // Convert 24 to 16-bit colours
                for (uint16_t col = 0; col < w; col++) {
                    b = *bptr++;
                    g = *bptr++;
                    r = *bptr++;
                    *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                }

                // Push the pixel row to screen, pushImage will crop the line if needed
                // y is decremented as the BMP image is drawn bottom up
                tft.drawPixel(
                    0, 0, 0
                ); // shared TFT_Spi devices struggle to work, need call a line first sometimes
                tft.pushImage(x, y--, w, 1, (uint16_t *)lineBuffer);
            }
            tft.setSwapBytes(oldSwapBytes);
            Serial.print("BMP Loaded in ");
            Serial.print(millis() - startTime);
            Serial.println(" ms");
        } else {
            goto ERROR;
        }
    } else {
    ERROR:
        Serial.println("BMP format not recognized.");
        bmpFS.close();
        return false;
    }
    bmpFS.close();
    return true;
}

bool drawImg(FS &fs, String filename, int x, int y, bool center, int playDurationMs) {
    String ext = filename.substring(filename.lastIndexOf('.'));
    ext.toLowerCase();
    uint8_t fls = 2;         // 2 for Little FS
    if (&fs == &SD) fls = 0; // 0 for SD
    tft.imageToBin(fls, filename, x, y, center, playDurationMs);
    if (ext.endsWith("jpg")) return showJpeg(fs, filename, x, y, center);
    else if (ext.endsWith("bmp")) return drawBmp(fs, filename, x, y, center);
    else if (ext.endsWith("png")) return drawPNG(fs, filename, x, y, center);

#if !defined(LITE_VERSION)

    else if (ext.endsWith("gif")) return showGif(&fs, filename.c_str(), x, y, center, playDurationMs);
#endif
    else log_e("Image not supported");

    return false;
}

#if !defined(LITE_VERSION)
/// Draw PNG files

#include <PNGdec.h>
#if TFT_WIDTH > TFT_HEIGHT
#define MAX_IMAGE_WIDTH TFT_WIDTH
#else
#define MAX_IMAGE_WIDTH TFT_HEIGHT
#endif
PNG *png;
// Functions to access a file on the SD card
File myfile;
FS *_fs;

void *myOpen(const char *filename, int32_t *size) {
    // Serial.printf("Attempting to open %s\n", filename);
    myfile = _fs->open(filename);
    *size = myfile.size();
    return &myfile;
}
void myClose(void *handle) {
    if (myfile) myfile.close();
}
int32_t myRead(PNGFILE *handle, uint8_t *buffer, int32_t length) {
    if (!myfile) return 0;
    return myfile.read(buffer, length);
}
int32_t mySeek(PNGFILE *handle, int32_t position) {
    if (!myfile) return 0;
    return myfile.seek(position);
}
// Function to draw pixels to the display
int16_t xpos = 0;
int16_t ypos = 0;
int PNGDraw(PNGDRAW *pDraw) {
    uint16_t usPixels[MAX_IMAGE_WIDTH];
    // static uint16_t dmaBuffer[MAX_IMAGE_WIDTH]; // static so buffer persists after fn exit
    uint8_t r = ((uint16_t)bruceConfig.bgColor & 0xF800) >> 8;
    uint8_t g = ((uint16_t)bruceConfig.bgColor & 0x07E0) >> 3;
    uint8_t b = ((uint16_t)bruceConfig.bgColor & 0x001F) << 3;
    png->getLineAsRGB565(pDraw, usPixels, PNG_RGB565_BIG_ENDIAN, b << 16 | g << 8 | r);
    tft.drawPixel(0, 0, 0);
    tft.drawPixel(0, 0, 0);
    tft.pushImage(xpos, ypos + pDraw->y, pDraw->iWidth, 1, usPixels);
    return 1;
}

bool drawPNG(FS &fs, String filename, int x, int y, bool center) {
    if ((x >= tft.width()) || (y >= tft.height())) return false;
    _fs = &fs;
    uint32_t dt = millis();

    // After starting WebUI, it is not possible to draw PNGs anymore, because there are no RAM memoty
    // available Need to fin out a way to make it work
    void *mem = psramFound() ? ps_malloc(sizeof(PNG)) : malloc(sizeof(PNG));
    if (!mem) {
        Serial.println("Fail alloc PNG!");
        bruceConfig.theme.label = true;
        return false;
    }

    png = new (mem) PNG();
    int16_t rc = png->open(filename.c_str(), myOpen, myClose, myRead, mySeek, PNGDraw);
    if (rc == PNG_SUCCESS) {
        // Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png->getWidth(),
        // png->getHeight(), png->getBpp(), png->getPixelType());

        if (center) {
            xpos = x + (tftWidth - png->getWidth()) / 2;
            ypos = y + (tftHeight - png->getHeight()) / 2;
        }

        if (png->getWidth() > MAX_IMAGE_WIDTH) {
            Serial.println("Image too wide for allocated line buffer size!");
        } else {
            rc = png->decode(NULL, 0);
            png->close();
        }
        // How long did rendering take...
        Serial.print("PNG Loaded in ");
        Serial.print(millis() - dt);
        Serial.println("ms");
    } else {
    ERROR:
        delete png;
        return false;
    }
    delete png;
    return true;
}
#else
bool drawPNG(FS &fs, String filename, int x, int y, bool center) {
    log_w("PNG: Not supported in this version");
    return false;
}
#endif
