#include <Arduino.h>
#include <arduino.h>

// Include application, user and local libraries
#include "LocalLibrary.h"
#include </Users/sam/Documents/Arduino/libraries/Tlc5940/Tlc5940.h>
#include </Users/sam/Documents/Arduino/libraries/Tlc5940/Tlc5940.cpp>
#include </Users/sam/Documents/Arduino/libraries/Tlc5940/tlc_config.h>
#include <math.h>


/////////////////////////////

boolean dbg = false; // debugging variable, enables serial output.

/////////////////////////////


float max_dist;
float min_dist;
int max_brightness = 1000;
int idle_brightness = 500;
int idle_time = 1000;
int idlespeed;

boolean idle = false;

const int numLEDs = 11;
const int smoothing = 100;
int smarr1[smoothing], smarr2[smoothing];
int smix1 = 0, smix2 = 0;
int smsum1 = 0, smsum2 = 0;

unsigned int framecount;
int spinner = 0;

const uint8_t sonar = A0;
const uint8_t adjust = A1;

// timers:
unsigned long start;

int pinOffset = 1; // offset if you don't start at 0
int ledpins[numLEDs];
int currValues[numLEDs];
int dist8bit, pdist;



void tSET (int channel, int value) {
    for (int i = 0; i < numLEDs; i++) {
        ledpins[i] = i + pinOffset;
    }
    currValues[channel] = value;
    Tlc.set(ledpins[channel], value);
}

int tGET ( int channel ) {
    return currValues[channel];
}

void inc (int pin, int amt) {
    // increment an led's brightness
    
    if (tGET(pin) < (4095-amt)) {
        tSET(pin, tGET(pin) + amt);
    }
    else tSET(pin, 4095);
}

void dec (int pin, int amt) {
    // decrement an led's brightness
    if (tGET(pin) > (amt)) {
        tSET(pin, tGET(pin) - amt);
    }
    else tSET(pin, 0);
}

void spin( int slowness, int spread, int max, int min) {
    
    if (spread >= float(numLEDs/2)) {
        min = max/2;
        spread = int(floor(numLEDs/2));
    }
    
    if (framecount % slowness == 0) {
        spinner ++;
        int cch = spinner % numLEDs;
        tSET(cch, max);
        for (int i = 1; i <= spread; i++) {
            float fade = (float(max-min)*(spread-i)/spread) + min;
            int pch = (spinner + i) % numLEDs;
            int nch = (spinner - i) % numLEDs;
            tSET(pch, int(floor(fade)));
            tSET(nch, int(floor(fade)));
        }

        
//        if (dbg) {
//            Serial.print("leds: [ ");
//            for (int i = 0; i < numLEDs; i++) {
//                Serial.print(tGET(i));
//                Serial.print("\t");
//            }
//            Serial.println(" ]");
//        }
    }
}

void setup()
{
    /* Call Tlc.init() to setup the tlc.
     You can optionally pass an initial PWM value (0 - 4095) for all channels.*/
    Serial.println("=== setting up ===");
    start = millis();
    framecount = 0;
    
    min_dist = 0.02;
 
    pdist = 99999999;
    
    for (int i=0; i<smoothing; i++) {
        smarr1[i] = 0;
        smarr2[i] = 0;
    }
    
    if (dbg) Serial.print("using pins ");
    for (int i = 0; i < numLEDs; i++) {
        ledpins[i] = i + pinOffset;
        if (dbg) {
            Serial.print(ledpins[i]);
            if (i < numLEDs - 1) Serial.print(", ");
        }
    }
    if (dbg) Serial.println();
    
    Tlc.init();
    if (dbg) Serial.begin(9600);
    
}




void loop()
{
    framecount++;
            if (dbg && idle) Serial.println(" ::: idling :::");
    
    if (framecount >= pow(2, 32) - 100) setup();
    
    if (dist8bit == 0) {
        if (framecount % idle_time == 0) {
            if (dist8bit == pdist && dist8bit == 0) {
                idle = true;
                idlespeed = int(random(50, 100));
            }
            else {
                idle = false;
                pdist = dist8bit;
            }
        }
    }
    else idle = false;
    
    smsum2 -= smarr2[smix2];
    smarr2[smix2] = analogRead(adjust);
    smsum2 += smarr2[smix2];
    smix2 ++;
    if (smix2 >= smoothing) smix2 = 0;
    
    float adj = (smsum2 / smoothing) / 1023.0f * 0.2f;
    if (dbg) {
//        Serial.print("max distance: ");
//        Serial.print( adj, DEC );
    }
    max_dist = adj; // longest possible distance in current space. adjustable by pot on pin adjust
    
    smsum1 -= smarr1[smix1];
    smarr1[smix1] = analogRead(sonar);
    smsum1 += smarr1[smix1];
    smix1 ++;
    if (smix1 >= smoothing) smix1 = 0;
    
    float read = smsum1 / smoothing;
    
    float proximity =  read/1023;
    float frac = proximity / max_dist;
    frac = constrain(frac, min_dist, 1);
    dist8bit = int((1-frac)*255);
    analogWrite(5, dist8bit);
    

    

    
    if (dbg) {
//        if (dbg) Serial.print( "\tframe " );
//        if (dbg) Serial.println(framecount);
        delay(10);
    }
    
    int slow = (255-dist8bit);
    int bright = int(frac*max_brightness/2);
    if (!idle) spin(slow, 2, max_brightness, bright);
    else if (idle) spin(idlespeed, 4, 250, 100);
//    for (int i = 0; i < numLEDs; i++) {
//        tSET(i, 4095);
//        if (dbg) Serial.print( tGET(i) );
//        if (dbg) Serial.print("\t");
//    }
//    
//    if (dbg) Serial.println(  );

    
    Tlc.update();
}
