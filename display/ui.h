#define STATUS_WIFI 0
#define STATUS_MQTT 1
#define STATUS_CONNECTING 0
#define STATUS_CONNECTED 1

void uiInit();
void uiDisplayStatus(int type, int status);
void uiClearValues();
void uiDisplayTemp(int value);
void uiDisplayPressure(int value);
void uiDisplayLux(int value);
void uiDisplayError(const char *error);
