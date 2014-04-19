#include <Arduino.h>
#include <arduino.h>

// Include application, user and local libraries
#include "LocalLibrary.h"
#include </Users/sam/Documents/Arduino/libraries/Tlc5940/Tlc5940.h>
#include </Users/sam/Documents/Arduino/libraries/Tlc5940/Tlc5940.cpp>
#include </Users/sam/Documents/Arduino/libraries/Tlc5940/tlc_config.h>
#include <math.h>


/////////////////////////////

boolean dbg = true; // debugging variable, enables serial output.

/////////////////////////////


float max_dist;
float min_dist;
float read, proximity, frac;
int max_brightness = 1000;
int idle_brightness = 500;
int idle_time = 1000;
int idlespeed;

boolean idle = false;

const int numLEDs = 12;


const int smoothing = 5;
int smarr1[smoothing], smarr2[smoothing];
int smix1 = 0, smix2 = 0;
int smsum1 = 0, smsum2 = 0;

unsigned int framecount;
int spinner = 0;
int tspinner = 0;
unsigned long spinstart = 0;
int pulsedir = 1;

const uint8_t sonar = A0;
const uint8_t adjust = A1;

int indicator = 5;

// timers:
unsigned long waitstart;

int pinOffset = 1; // offset if you don't start at 0
int ledpins[numLEDs];
int currValues[numLEDs];
int dist8bit, pdist;

void smooth() {
    smsum1 -= smarr1[smix1];
    smarr1[smix1] = analogRead(sonar);
    smsum1 += smarr1[smix1];
    smix1 ++;
    if (smix1 >= smoothing) smix1 = 0;
    smsum2 -= smarr2[smix2];
    smarr2[smix2] = analogRead(adjust);
    smsum2 += smarr2[smix2];
    smix2 ++;
    if (smix2 >= smoothing) smix2 = 0;
}

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
    tSET(pin, tGET(pin) + amt);
}

void dec (int pin, int amt) {
    // decrement an led's brightness
    tSET(pin, tGET(pin) - amt);
}

void spin( int slowness, int spread, int max, int min) {
    
    if (spread >= float(numLEDs/2)) {
        // prettyifying hack in case the spread is too big
        min = max/2;
        spread = int(floor(numLEDs/2));
    }
    
    if (millis() - spinstart > slowness) {
        spinner ++;
        if (spinner == numLEDs) spinner = 0;

        tSET(spinner, max);
        
        for (int i = 1; i <= spread; i++) {
            float fade = (float(max-min)*(spread-i)/spread) + min;
            int pch = (spinner + i) % numLEDs;
            int nch;
            if (spinner - i < 0) nch = numLEDs - i;
            else nch = spinner - i;
            tSET(pch, int(floor(fade)));
            tSET(nch, int(floor(fade)));
        }

        spinstart = millis();
    }
}

void pulse( int speed, int wait) {
    // this should work for uniform + non-uniform
    // although speed is still constant (could be randomized per channel)
    // ...but it's broken.
    for (int i = 0; i < numLEDs; i++) {
        if (pulsedir == 1 && millis()-waitstart > wait) {
            if (tGET(i) < max_brightness - speed)
                inc(i, speed);
            else {
                tSET(i, max_brightness);
                pulsedir = -1;
            }
        }
            
        else if (pulsedir == -1 && millis()-waitstart > wait) {
            if (tGET(i) > max_brightness + speed)
                dec(i, speed);
            else {
                waitstart = millis();
                tSET(i, 0);
                pulsedir = 1;
            }
        }
    }
}

void setup()
{
    digitalWrite(A0, LOW);
    if (dbg) Serial.begin(9600);
    
    Serial.println("=== setting up ===");
    
    framecount = 0; // set / reset framecount
    
    min_dist = 0.02; // minimum distance cutoff (currently not a scale factor)
 
    pdist = 4; // random non-zero value, will be reset immediately
    
    for (int i=0; i<smoothing; i++) {
        // fill smoothing arrays with 0
        smarr1[i] = 0;
        smarr2[i] = 0;
    }

    // utility to start at a pin other than 0
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
    
}




void loop()
{
    framecount++;
//    if (dbg && idle) Serial.println(" ::: idling :::");
    
    if (framecount >= pow(2, 32) - 100) setup();
    // if frame count is nearing overflow
    
    // check for idle:
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
    

    //    for (int x = 0; x < 4; x++) {
    //        tSET(x, dist8bit);
    //    }

    

    //    smooth();
    // figure out smoothed values
    
    //    max_dist = (smsum2 / smoothing) / 1023.0f * 0.2f;
    max_dist = analogRead(A1) / 1023.0f * 0.2f;
    // longest possible distance in current space. adjustable by pot on pin adjust.
    // the max dist should be just smaller than the sonar's reading when there's nobody there
    
    //    float read = smsum1 / smoothing;
    read = analogRead(A0);
    // smoothed distance reading
    
    proximity =  read/1023;           // 0-1 absolute distance
    frac = proximity / max_dist;      // 0-1 scaled distance
    frac = constrain(frac, min_dist, 1);    // apply minimum distance cutoff
    dist8bit = int((1-frac)*255);           // 8 bit (0-255) distance value
    analogWrite(indicator, dist8bit);               // indicator LED
    
    if (dbg) {
        // uncomment this block to calibrate distance
        Serial.print( framecount );
        Serial.print( ":" );
        Serial.print( framecount % 100);
        Serial.print("\tmax dist: ");
        Serial.print(max_dist);
        Serial.print("\treal dist:");
        Serial.print(proximity);
        Serial.print("\tcalculated dist: ");
        Serial.println(dist8bit);
        delay(10);
    }
        
    int slow = (255-dist8bit);
    int bright = int(frac*max_brightness/2);
//    if (!idle) spin(slow, 2, max_brightness, bright);
//    else if (idle) spin(idlespeed, 4, 250, 100);
    
//    spin(50, 3, 4095, 2000);
//    tSET(0, dist8bit*4);

    
    
    Tlc.update();
}
