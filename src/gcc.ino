#include "Nintendo.h"

#define USE_SERIAL 0

#define SW_NOTCH_X 56
#define SW_NOTCH_Y 60

#define SE_NOTCH_X 206
#define SE_NOTCH_Y 54

#if USE_SERIAL
#define PIN_CONTROLLER 2
#define PIN_CONSOLE 3
#else
#define PIN_CONTROLLER 0
#define PIN_CONSOLE 1
#endif

class CGamecubeControllerModded : public CGamecubeController {
public:
    inline CGamecubeControllerModded(const uint8_t p) : CGamecubeController(p) {}
    inline void mod(void);
};

static CGamecubeControllerModded s_controller(PIN_CONTROLLER);
static CGamecubeConsole s_console(PIN_CONSOLE);

void CGamecubeControllerModded::mod(void)
{
    if (report.l || report.r || report.left > 74 || report.right > 74)
    {
        if (report.xAxis > (SW_NOTCH_X - 2) && report.xAxis < (SW_NOTCH_X + 2) && report.yAxis > (SW_NOTCH_Y - 2) && report.yAxis < (SW_NOTCH_Y + 2))
        {
            report.xAxis = origin.inititalData.xAxis - 55;
            report.yAxis = origin.inititalData.yAxis - 55;
        }
        else if (report.xAxis > (SE_NOTCH_X - 2) && report.xAxis < (SE_NOTCH_X + 2) && report.yAxis > (SE_NOTCH_Y - 2) && report.yAxis < (SE_NOTCH_Y + 2))
        {
            report.xAxis = origin.inititalData.xAxis + 55;
            report.yAxis = origin.inititalData.yAxis - 55;
        }
    }
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
#if USE_SERIAL
    Serial.begin(115200);
#endif
}

void loop()
{
    if (s_controller.read())
    {
        s_controller.mod();
        if (!s_console.write(s_controller))
        {
#if USE_SERIAL
            Serial.println(F("Error writing to console"));
#endif
            digitalWrite(LED_BUILTIN, HIGH);
            delay(1000);
        }
    }
    else
    {
#if USE_SERIAL
        Serial.println(F("Error reading controller"));
#endif
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
    }
}
