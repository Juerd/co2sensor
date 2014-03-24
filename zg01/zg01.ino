#include <RF24.h>
#include <nRF24L01.h>
#include <RF24_config.h>

#include <SPI.h>

#include <stdint.h>

#include "zg01_fsm.h"

#define PIN_CLOCK   2
#define PIN_DATA    3

static long int address = 0x66996699L;  // So that's 0x0066996699
const int tries = 1;

static uint8_t buffer[5];
static RF24 rf(/*ce*/ 8, /*cs*/ 10);

void setup(void)
{
    // initialize ZG01 pins
    pinMode(PIN_CLOCK, INPUT);
    pinMode(PIN_DATA, INPUT);

    // initialize serial port
    Serial.begin(9600);
    Serial.println("Hello world!\n");
    
    // initialize ZG01 finite state machine
    zg01_init(buffer);
    
    // init RF24
    rf.begin();
    rf.setRetries(15, 15);
    rf.enableDynamicPayloads();
    rf.openWritingPipe(address);
}

void loop(void)
{
    // wait until clock is low
    while (digitalRead(PIN_CLOCK) != LOW);

    // sample data and process in the ZG01 state machine
    uint8_t data = (digitalRead(PIN_DATA) == HIGH) ? 1 : 0;
    unsigned long ms = millis();
    bool ready = zg01_process(ms, data);
    
    // process data if ready
    if (ready) {
        // dump buffer to serial
        for (int i = 0; i < 5; i++) {
            Serial.print(buffer[i], HEX);
            Serial.print(" ");
        }
        
        // get binary value
        uint16_t val = buffer[1] << 8 | buffer[2];
        Serial.print(val, DEC);
        Serial.println();

        // calculate checksum
        uint8_t chk = buffer[0] + buffer[1] + buffer[2];        
        
        // send CO2 measurement over RF24
        if (buffer[0] == 'P') {
            // construct buffer: [length]"CO_2"[ppm high byte][ppm low byte]
            uint8_t buf[8];
            buf[0] = 6;
            memcpy(&buf[1], "CO_2", 4);
            memcpy(&buf[5], &buffer[1], 2);
            // dump send buffer
            Serial.print("Send:");
            for (int i = 0; i < 6; i++) {
                Serial.print(buf[i], HEX);
                Serial.print(" ");
            }
            Serial.println();
            // send it
            rf.write(&buf, 7);
        }
    }

    // wait until clock is high again
    while (digitalRead(PIN_CLOCK) == LOW);    
}


