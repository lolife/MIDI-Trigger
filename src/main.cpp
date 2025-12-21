#include <M5Unified.h>
#include <M5UnitSynth.h>

#define RX_PIN 18
#define TX_PIN 17
#define BAUD_RATE 31250

#define TRIGGER_PIN G9

#define KICK 36
#define SNARE 40
#define RIDE 51
#define CONGA_HI 51
#define CONGA_LOW 59

int currentNote = 37;
char currentNoteName[32] = "Rim";

M5UnitSynth synth;

const int THRESHOLD = 175;
const int HYSTERESIS = 5;        // Signal must drop this far below threshold to reset
const int MIN_TRIGGER_DURATION = 3; // Signal must stay above threshold for this many samples
const int MIN_HIT_INTERVAL = 60;
const int NOTE_DURATION = 60;

unsigned long lastTrigger = 0;
unsigned long noteOffTime = 0;
bool noteIsOn = false;
bool wasAboveThreshold = false;
int lastVelocity = 0;
int currentADC = 0;
int zeroPoint = 0;
int peakValue = 0;
int samplesAboveThreshold = 0;

// Moving average filter
const int FILTER_SIZE = 5;
int filterBuffer[FILTER_SIZE] = {0};
int filterIndex = 0;

// Display update throttling
unsigned long lastDisplayUpdate = 0;
const int DISPLAY_UPDATE_INTERVAL = 100;
bool lastDisplayTriggered = false;
int screenWidth = 0;
int screenHeight = 0;

int getFilteredADC() {
    // Read raw value
    int raw = analogRead(TRIGGER_PIN) - zeroPoint;
    
    // Update circular buffer
    filterBuffer[filterIndex] = raw;
    filterIndex = (filterIndex + 1) % FILTER_SIZE;
    
    // Calculate average
    int sum = 0;
    for (int i = 0; i < FILTER_SIZE; i++) {
        sum += filterBuffer[i];
    }
    return sum / FILTER_SIZE;
}

void drawUI() {
    M5.Display.setFont(&FreeSansBold18pt7b);
    int top = 30;
    int left = 20;
    int pad = M5.Display.fontHeight();

    M5.Display.setFont(&Orbitron_Light_32);
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextColor(TFT_GREENYELLOW);
    M5.Display.setCursor( screenWidth/2 - M5.Display.textWidth("MIDI TRIGGER")/2, 5 );
    M5.Display.print( "MIDI TRIGGER" );
    
    M5.Display.setFont(&FreeSansBold12pt7b);
    pad = M5.Display.fontHeight() + 12;
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.drawString("Note:", left, top + pad*1);
    M5.Display.drawString("Velocity:", left, top + pad*2);
    M5.Display.drawString("ADC Value:", left, top + pad*3);
}

void updateDisplay(int adc, int velocity, int note, bool triggered) {
    M5.Display.setFont(&FreeSansBold12pt7b);
    int top = 30;
    int left = screenWidth/2;
    int pad = M5.Display.fontHeight() + 12;

    M5.Display.fillRect( left, top+pad, screenWidth/2, pad*23-5, TFT_BLACK );
    M5.Display.setTextColor(TFT_SKYBLUE);
    M5.Display.setCursor(left, top + pad*1);
    M5.Display.printf( "%3d - %s", note, currentNoteName);

    M5.Display.setTextColor(TFT_GREEN);
    M5.Display.setCursor(left, top + pad*2);
    M5.Display.printf( "%3d", velocity);

    M5.Display.setTextColor(TFT_YELLOW);
    M5.Display.setCursor(left, top + pad*3);
    M5.Display.printf( "%4d [%d]", abs(adc), peakValue );
    
    M5.Display.fillRect( screenWidth/4, top + pad*4, screenWidth/2, 50, TFT_BLACK );
    if (triggered) {
        M5.Display.setTextColor(TFT_RED);
        M5.Display.setCursor( screenWidth/2 - M5.Display.textWidth("TRIGGERED!")/2, top + pad*4 );
        M5.Display.print( "TRIGGERED!" );
        M5.Display.fillCircle(290, top + pad*4, 8, TFT_RED);
    } else {
        M5.Display.setTextColor(TFT_DARKGREY);
        M5.Display.setCursor( screenWidth/2 - M5.Display.textWidth("Waiting...")/2, top + pad*4 );
        M5.Display.print( "Waiting..." );
        M5.Display.fillCircle(290, top + pad*4, 8, TFT_DARKGREY );
    }

    M5.Display.fillRect(10, 220, screenWidth, 15, TFT_DARKGREY);
    int barWidth = map( adc, 0, 4095-zeroPoint, 0, screenWidth);
    int thresh = map(THRESHOLD, 0, 4095-zeroPoint, 0, screenWidth);
    //Serial.printf("ADC: %d, Bar Width: %d, Threshold: %d\n", adc, barWidth, thresh);
    if( barWidth > thresh ) {
        M5.Display.fillRect(10, 220, barWidth, 15, TFT_RED);
    } else {
        M5.Display.fillRect(10, 220, barWidth, 15, TFT_ORANGE);
    }
}

void setup() {
    Serial.begin(115200);
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(1);
    //M5.Display.setTextDatum(middle_center);
    screenWidth = M5.Display.width();
    screenHeight = M5.Display.height();
    
    synth.begin( &Serial2, BAUD_RATE, TX_PIN, RX_PIN );
    drawUI();
    pinMode( TRIGGER_PIN, INPUT_PULLDOWN );
    
    // Calibrate zero point with more samples
    int total = 0;
    for (int i = 0; i < 50; i++) {
        total += analogRead(TRIGGER_PIN);
        delay(10);
    }
    zeroPoint = total / 50;
    
    // Initialize filter buffer
    for (int i = 0; i < FILTER_SIZE; i++) {
        filterBuffer[i] = 0;
    }
    
    Serial.println("MIDI Trigger Ready");
    Serial.printf("Zero point: %d\n", zeroPoint);
}

void loop() {
    static int loopCounter = 0;
    M5.update();
    
    unsigned long now = millis();
    currentADC = getFilteredADC();  // Use filtered value
    
    // State machine with hysteresis
    if (!wasAboveThreshold) {
        // Waiting for trigger
        if (currentADC > THRESHOLD) {
            samplesAboveThreshold++;
            if (samplesAboveThreshold >= MIN_TRIGGER_DURATION) {
                // Confirmed trigger start
                wasAboveThreshold = true;
                peakValue = currentADC;
                samplesAboveThreshold = 0;
            }
        } else {
            samplesAboveThreshold = 0;
        }
    } else {
        // In triggered state - track peak
        if (currentADC > peakValue) {
            peakValue = currentADC;
        }
        
        // Wait for signal to drop below threshold minus hysteresis
        if (currentADC < (THRESHOLD - HYSTERESIS)) {
            // Trigger complete - send MIDI
            if (now - lastTrigger > MIN_HIT_INTERVAL) {
                lastVelocity = map(peakValue, THRESHOLD, 4095-zeroPoint, 24, 127);
                lastVelocity = constrain(lastVelocity, 24, 127);
        
                synth.setNoteOn(0, currentNote, lastVelocity);
                noteIsOn = true;
                noteOffTime = now + NOTE_DURATION;
                
                //Serial.printf("Trigger! Peak: %d, Velocity: %d\n", peakValue, lastVelocity);
                
                lastTrigger = now;
                lastDisplayTriggered = true;
            }
            
            wasAboveThreshold = false;
            //peakValue = 0;
        }
    }
    
    // Handle note off
    if (noteIsOn && now >= noteOffTime) {
        synth.setNoteOff(0, currentNote, 0);
        noteIsOn = false;
    }
    
    if (now - lastTrigger > 3333) {
      peakValue = 0;
      lastVelocity = 0;
    }

    // Throttled display updates
    if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
        updateDisplay(currentADC, lastVelocity, currentNote, lastDisplayTriggered);
        lastDisplayUpdate = now;
        lastDisplayTriggered = false;
    }
}