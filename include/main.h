#include <Arduino.h>
#include <M5Unified.h>
#include "M5UnitSynth.h"
#include <ArduinoOTA.h>

#define FILTER_SIZE 5

// Button definitions
struct Button {
     int x;
     int y;
     int w; 
     int h;
     int colorOff;
     int colorOn;
    const char* label;
};

struct Trigger {
    int pin;
    int zeroPoint = 0;
    int note = 36;;
    int filterBuffer[FILTER_SIZE] = {0};
    int filterIndex = 0;
    unsigned long lastTrigger = 0;
    unsigned long noteOffTime = 0;
    bool noteIsOn = false;
    bool wasAboveThreshold = false;
    int lastVelocity = 0;
    int currentADC = 0;
    int peakValue = 0;
    int samplesAboveThreshold = 0;
    bool lastDisplayTriggered = false;
    Button* triggerButton;
};

//#define ARDUINO_M5STACK_CORES2

#define BAUD_RATE 31250

#if defined(ARDUINO_M5STACK_CORES2)
#define TRIGGER_PIN1 G36
#define TRIGGER_PIN2 G26
#define RX_PIN 13
#define TX_PIN 14
#elif defined(ARDUINO_M5STACK_CORES3)
#define TRIGGER_PIN1 G8
#define TRIGGER_PIN2 G9
#define RX_PIN 18
#define TX_PIN 17
#endif

#define KICK 36
#define SNARE 40
#define RIDE 51
#define CONGA_HI 51
#define CONGA_LOW 59

int currentNote = KICK;
char currentNoteName[32] = "Kick";

M5UnitSynth synth;

const int THRESHOLD = 175;
const int HYSTERESIS = 5;        // Signal must drop this far below threshold to reset
const int MIN_TRIGGER_DURATION = 3; // Signal must stay above threshold for this many samples
const int MIN_HIT_INTERVAL = 60;
const int NOTE_DURATION = 60;

// Display update throttling
#define NORMAL_COLOR TFT_OLIVE
unsigned long lastDisplayUpdate = 0;
const int DISPLAY_UPDATE_INTERVAL = 250;

int screenWidth = 0;
int screenHeight = 0;

bool initializeWiFi();
int calibrateTrigger( int inputPin );
int getFilteredADC( Trigger* newTrigger  );
void handleTrigger( Trigger* newTrigger );
void initializeTrigger( Trigger* newTrigger, int pin, int note, Button* tButton );
void initializeFilterBuffer( Trigger* newTrigger );
void drawUI();
void updateDisplay();
void handleTouchInput();
void drawButton( Button* btn, bool isOn );
void displayMessage(const char* msg);
void centerCursor(const lgfx::GFXfont* font, int size, const char* text);