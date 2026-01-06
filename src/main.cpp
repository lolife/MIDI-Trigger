#include "main.h"

static Button downButton1 = {10, 100, 60, 60, BLUE, BLUE, "-" };
static Button upButton1 = {75, 100, 60, 60, BLUE, BLUE, "+" }; // x and y are adjused in setup()
static Button triggerButton1 = {10, 100, 125, 60, DARKGREY, RED, "TRIGGER"};
static Button upButton2 = {50, 50, 60, 60, BLUE, BLUE, "+" }; // x and y are adjused in setup()
static Button downButton2 = {115, 100, 60, 60, BLUE, BLUE, "-" };
static Button triggerButton2 = {50, 100, 125, 60, DARKGREY, RED, "TRIGGER"};

static Trigger trigger1 = { TRIGGER_PIN1, 0, KICK, {}, 0, 0, 0, false, false, 0, 0, 0, 0, false, &triggerButton1 };
static Trigger trigger2 = { TRIGGER_PIN2, 0, SNARE, {}, 0, 0, 0, false, false, 0, 0, 0, 0, false, &triggerButton2 };

#define MAX_SAMPLES  44100
int tData[MAX_SAMPLES];
static int n=0;
static bool testDone = false;
static int tStart = 0;

int getFilteredADC( Trigger* newTrigger ) {
    // Read raw value
    //int raw = newTrigger->zeroPoint - analogRead(newTrigger->pin);
    //return raw;

    //delayMicroseconds(10);
    // Update circular buffer
    //newTrigger->filterBuffer[newTrigger->filterIndex] = raw;
    //newTrigger->filterIndex = (newTrigger->filterIndex + 1) % FILTER_SIZE;
    
    // // Calculate average
    int sum = 0;
    for (int i = 0; i < 3; i++) {
        //sum += newTrigger->filterBuffer[i];
//        sum += newTrigger->zeroPoint - analogRead(newTrigger->pin);
        sum += analogRead(newTrigger->pin);
        //sum += analogRead(newTrigger->pin);
        //delay(1);
    }
    return (sum/3) - newTrigger->zeroPoint;
}

void drawUI() {
    //Serial.println( "drawUI" );
    M5.Display.setFont(&FreeSansBold18pt7b);
    int top = 30;
    int left = 10;
    int pad = M5.Display.fontHeight();

    M5.Display.setFont(&Orbitron_Light_32);
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextColor(TFT_GREENYELLOW);
    M5.Display.setCursor( screenWidth/2 - M5.Display.textWidth("MIDI TRIGGER")/2, 5 );
    M5.Display.print( "MIDI TRIGGER" );
    
    M5.Display.setFont(&FreeSansBold12pt7b);
    pad = M5.Display.fontHeight() + 12;
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setCursor( left, top+pad );
    M5.Display.printf("Note: %d", trigger1.note );
    M5.Display.setCursor( screenWidth/2, top+pad );
    M5.Display.printf("Note: %d", trigger2.note );
}

void updateDisplay() {
    //Serial.println( "updateDisplay" );
    drawButton( &upButton1, false );
    drawButton ( &downButton1, false );
    drawButton( &triggerButton1, false );

    drawButton( &upButton2, false );
    drawButton ( &downButton2, false );
    drawButton( &triggerButton2, false );
}

void setup() {
    Serial.begin(115200);
    auto cfg = M5.config();
    M5.begin(cfg);

    screenWidth = M5.Display.width();
    screenHeight = M5.Display.height();

    int top = screenHeight/2 - 20;
    downButton1.y = top;
    upButton1.y = top;
    triggerButton1.y = top + downButton1.h + 5;

    downButton2.y = top;
    upButton2.y = top;
    triggerButton2.y = top + downButton2.h + 5;

    downButton2.x = screenWidth/2;
    upButton2.x = screenWidth/2 + downButton2.w + 5;
    triggerButton2.x = screenWidth/2;

    initializeTrigger( &trigger1 );
    initializeTrigger( &trigger2 );

    synth.begin( &Serial2, BAUD_RATE, TX_PIN, RX_PIN );

    Serial.printf("Trigger pin G%d, note %d, zeropoint %d\n", trigger1.pin, trigger1.note, trigger1.zeroPoint );
    Serial.printf("Trigger pin G%d, note %d, zeropoint %d\n", trigger2.pin, trigger2.note, trigger2.zeroPoint );
 
    drawUI();
}

void initializeTrigger( Trigger* newTrigger ) {
    pinMode( newTrigger->pin, INPUT_PULLDOWN );
    analogSetAttenuation(ADC_11db);  // Set ADC range to 0-3.3V
    analogReadResolution(12);         // Explicit 12-bit resolution
    newTrigger->zeroPoint = calibrateTrigger( newTrigger->pin );
    initializeFilterBuffer( newTrigger );
}

void initializeFilterBuffer( Trigger* newTrigger ) {
    for (int i = 0; i < FILTER_SIZE; i++) {
        newTrigger->filterBuffer[i] = 0;
    }
}

int calibrateTrigger( int inputPin ) {
    // Calibrate zero points
    int total = 0;
    for (int i = 0; i < 50; i++) {
        int raw = analogRead( inputPin );
        delay(1);
        total += raw;
        Serial.printf( "%d, %d\n", raw, total);
    }
    return(total / 50);
}

void initTData() {
    for( int i = 0; i < 10240; i++ ) {
        tData[i]=0;
    }
}

void loop() {
    static int loopCounter = 0;
    M5.update();

    handleTouchInput();

    unsigned long now = millis();
    
    //run_test();


    handleTrigger( &trigger1 );
    handleTrigger( &trigger2 );

    // Throttled display updates
   if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
       updateDisplay();
        trigger1.lastDisplayTriggered = false;
        trigger2.lastDisplayTriggered = false;

       lastDisplayUpdate = now;
    }
}

void run_test() {
    unsigned long now = millis();
    initTData();
    for( int t = 0; t < 5; t++ ) {
        Serial.printf( "%d\n", 5-t );
        delay(500);
    }

    while( ! testDone ) {
        get_trigger_data( &trigger1 );
        n++;
        if( n > MAX_SAMPLES )
            n=0;
    }
    plot_trigger_data( &trigger1 );
    delay(5000);
    n = 0;
    trigger1.peakValue = 0;
    trigger1.lastTrigger = now;
    trigger1.wasAboveThreshold = false;

    testDone = false;
}

int cstrain( int val, int lo, int hi ) {
    if( val < lo )
        val = lo;
    else if ( val > hi )
        val = hi;
    return( val );
}
void plot_trigger_data( Trigger* newTrigger ) {
    int samplesAboveThreshold = 0;
    int samplesBelowThreshold = 0;

    if( n < tStart )
        return;
    Serial.printf( "#samples=%d, peak=%d\n", n-tStart, newTrigger->peakValue );

    M5.Display.clear( TFT_BLUE );
//    M5.Display.setColor( BLUE );
//    M5.Display.fillRect( 0, 0, screenWidth, screenHeight, BLUE );
    M5.Display.setColor( SILVER );
    for( int i = 0; i<n-tStart; i++ ) {
        if( tStart+i > MAX_SAMPLES )
            tStart -+ MAX_SAMPLES;
        if( tData[tStart+i] >= THRESHOLD )
            samplesAboveThreshold++;
        else
            samplesBelowThreshold++;
        int lineHeight = map( tData[tStart+i], THRESHOLD, newTrigger->peakValue, 0, screenHeight-20 );
        lineHeight = cstrain( lineHeight, 0, screenHeight-20 );
        if( tData[tStart+i] > 0 )
                Serial.printf( "%d -> %d\n", tData[tStart+i], lineHeight );
        M5.Display.drawLine( i, screenHeight-lineHeight, i, screenHeight );
    }
    Serial.printf("%d:%d\n", samplesAboveThreshold, samplesBelowThreshold );
}

void get_trigger_data( Trigger* newTrigger ) {
    //Serial.println( "get_trigger_data" );

    unsigned long now = millis();
    newTrigger->currentADC =  getFilteredADC( newTrigger );
    tData[n]= newTrigger->currentADC;

    // State machine with hysteresis
    if (!newTrigger->wasAboveThreshold) {
        // Waiting for trigger
        if (newTrigger->currentADC > THRESHOLD) {
            //Serial.println( "Above THRESHOLD");
            newTrigger->samplesAboveThreshold++;
            if (newTrigger->samplesAboveThreshold >= MIN_TRIGGER_DURATION) {
                // Confirmed trigger start
                tStart = n-newTrigger->samplesAboveThreshold;
                Serial.printf( "Trigger start with %d samples\n", newTrigger->samplesAboveThreshold );
                newTrigger->wasAboveThreshold = true;
                newTrigger->peakValue = newTrigger->currentADC;
                newTrigger->samplesAboveThreshold = 0;
            }
            //else
            //    Serial.println( "Failed" );
        }
        else {
             newTrigger->samplesAboveThreshold = 0;
         }
    } else {
        // In triggered state - track peak
        if (newTrigger->currentADC > newTrigger->peakValue) {
            newTrigger->peakValue = newTrigger->currentADC;
        }
       
        // Wait for signal to drop below threshold minus hysteresis
        if (newTrigger->currentADC < THRESHOLD - HYSTERESIS ) {
            // Trigger complete - send MIDI
            if (now - newTrigger->lastTrigger > MIN_HIT_INTERVAL) {
                testDone = true;
                return;
                newTrigger->lastVelocity = map(newTrigger->peakValue, THRESHOLD, newTrigger->zeroPoint, 0, 127);
                newTrigger->lastVelocity = constrain(newTrigger->lastVelocity, 24, 127);
                //Serial.printf("Note %d ON Peak: %d Velocity: %d\n", newTrigger->note, newTrigger->peakValue, newTrigger->lastVelocity );

                synth.setNoteOn(0, newTrigger->note, newTrigger->lastVelocity);
                newTrigger->noteIsOn = true;
                newTrigger->noteOffTime = now + NOTE_DURATION;
                //newTrigger->triggerButton->label = "TRIGGER";
                drawButton( newTrigger->triggerButton, true );
                //newTrigger->triggerButton->label = "WAITING";
                newTrigger->lastTrigger = now;
                newTrigger->lastDisplayTriggered = true;
            }
            
            newTrigger->wasAboveThreshold = false;
            newTrigger->peakValue = 0;
        }
        //else
        //    Serial.println( "trigger continues..." );
    }
}

void handleTrigger( Trigger* newTrigger ) {
    unsigned long now = millis();
    newTrigger->currentADC =  getFilteredADC( newTrigger );

    // State machine with hysteresis
    if (!newTrigger->wasAboveThreshold) {
        // Waiting for trigger
        if (newTrigger->currentADC > THRESHOLD) {
            //Serial.println( "Above THRESHOLD");
            newTrigger->samplesAboveThreshold++;
            if (newTrigger->samplesAboveThreshold >= MIN_TRIGGER_DURATION) {
                // Confirmed trigger start
                //Serial.printf( "%d samples : ", newTrigger->samplesAboveThreshold );
                newTrigger->wasAboveThreshold = true;
                newTrigger->peakValue = newTrigger->currentADC;
                newTrigger->samplesAboveThreshold = 0;
            }
            //else
            //    Serial.println( "Failed" );
        }
        else {
             newTrigger->samplesAboveThreshold = 0;
         }
    } else {
        // In triggered state - track peak
        if (newTrigger->currentADC > newTrigger->peakValue) {
            newTrigger->peakValue = newTrigger->currentADC;
        }
       
        // Wait for signal to drop below threshold minus hysteresis
        if (newTrigger->currentADC < THRESHOLD + HYSTERESIS ) {
            // Trigger complete - send MIDI
            if (now - newTrigger->lastTrigger > MIN_HIT_INTERVAL) {
                newTrigger->lastVelocity = map(newTrigger->peakValue, THRESHOLD, 4095-newTrigger->zeroPoint, 32, 127);
                newTrigger->lastVelocity = constrain(newTrigger->lastVelocity, 24, 127);
                Serial.printf("Note %d ON Peak: %d Velocity: %d\n", newTrigger->note, newTrigger->peakValue, newTrigger->lastVelocity );

                synth.setNoteOn(0, newTrigger->note, newTrigger->lastVelocity);
                newTrigger->noteIsOn = true;
                newTrigger->noteOffTime = now + NOTE_DURATION;
                //newTrigger->triggerButton->label = "TRIGGER";
                drawButton( newTrigger->triggerButton, true );
                //newTrigger->triggerButton->label = "WAITING";
                newTrigger->lastTrigger = now;
                newTrigger->lastDisplayTriggered = true;
            }
            
            newTrigger->wasAboveThreshold = false;
            newTrigger->peakValue = 0;
        }
        //else
        //    Serial.println( "trigger continues..." );
    }
    
    // Handle note off
    if (newTrigger->noteIsOn && now >= newTrigger->noteOffTime) {
        //Serial.printf("Note %d OFF Peak: %d Velocity: %d\n\n", newTrigger->note, newTrigger->peakValue, newTrigger->lastVelocity );
        synth.setNoteOff(0, newTrigger->note, 0);
        newTrigger->noteIsOn = false;
        newTrigger->peakValue = 0;
    }
}
 
void drawButton( Button* btn, bool isOn ) {
    // Draw filled rectangle
    M5.Display.fillRoundRect(btn->x, btn->y, btn->w, btn->h, 10, isOn ? btn->colorOn : btn->colorOff );
    // Draw border
    M5.Display.drawRoundRect(btn->x, btn->y, btn->w, btn->h, 10, SKYBLUE );

    // Draw label
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setFont(&fonts::FreeSansBold12pt7b);
    M5.Display.setTextSize(1);

    // Center text in button
    int textWidth = M5.Display.textWidth(btn->label);
    int textX = btn->x + (btn->w - textWidth) / 2;
    int textY = btn->y + (btn->h / 2) - (M5.Display.fontHeight() / 2);

    M5.Display.setCursor(textX, textY);
    M5.Display.print(btn->label);
}

void handleTouchInput() {
    auto touch = M5.Touch.getDetail();

    if ( touch.wasClicked() ) {
        int touchX = touch.x;
        int touchY = touch.y;

         if (touchX >= downButton1.x && touchX <= (downButton1.x + downButton1.w) &&
            touchY >= downButton1.y && touchY <= (downButton1.y + downButton1.h)) {
                trigger1.note -= 1;
                drawUI();
        }
        if (touchX >= upButton1.x && touchX <= (upButton1.x + upButton1.w) &&
            touchY >= upButton1.y && touchY <= (upButton1.y + upButton1.h)) {
                trigger1.note += 1;
                 drawUI();
       }

        if (touchX >= triggerButton1.x && touchX <= (triggerButton1.x + triggerButton1.w) &&
            touchY >= triggerButton1.y && touchY <= (triggerButton1.y + triggerButton1.h)) {
                //Serial.println("Manual trigger!");
                synth.setNoteOn(0, trigger1.note, 127);
                triggerButton1.label = "TRIGGER";
                drawButton( &triggerButton1, true );
                delay(100);
                synth.setNoteOff(0, trigger1.note, 0);
                triggerButton1.label = "WAITING";
        }

        if (touchX >= downButton2.x && touchX <= (downButton2.x + downButton2.w) &&
            touchY >= downButton2.y && touchY <= (downButton2.y + downButton2.h)) {
                trigger2.note -= 1;
                drawUI();
        }
        if (touchX >= upButton2.x && touchX <= (upButton2.x + upButton2.w) &&
            touchY >= upButton2.y && touchY <= (upButton2.y + upButton2.h)) {
                trigger2.note += 1;
                drawUI();
        }

        if (touchX >= triggerButton2.x && touchX <= (triggerButton2.x + triggerButton2.w) &&
            touchY >= triggerButton2.y && touchY <= (triggerButton2.y + triggerButton2.h)) {
                //Serial.println("Manual trigger!");
                synth.setNoteOn(0, trigger2.note, 127);
                triggerButton2.label = "TRIGGER";
                drawButton( &triggerButton2, true );
                delay(100);
                synth.setNoteOff(0, trigger2.note, 0);
                triggerButton2.label = "WAITING";
        }
    }
}

void displayMessage(const char* msg) {
    M5.Display.clear( NORMAL_COLOR );
    M5.Display.setFont(&fonts::FreeSansBold18pt7b);
    M5.Display.setTextSize(1);
    centerCursor(&fonts::FreeSansBold18pt7b, 1, msg);
    M5.Display.print(msg);
    //Serial.println(msg);
}

void centerCursor(const lgfx::GFXfont* font, int size, const char* text) {
    M5.Display.setFont(font);
    M5.Display.setTextSize(size);
    int textWidth = M5.Display.textWidth(text);
    int textHeight = M5.Display.fontHeight();
    M5.Display.setCursor((M5.Display.width() - textWidth) / 2, 
                        (M5.Display.height() - textHeight) / 2);
}