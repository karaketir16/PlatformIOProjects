#include <Arduino.h>
#include "gps.h"
#include <Wire.h>
#include <SPI.h>

#include <Adafruit_Sensor.h>  // not used in this demo but required!
#include <RadioLib.h>

#include "bme.h"
#include "lsm.h"
#include "../../shared/shared.h"

#include "minmea/minmea.h"

// #define SIMULATION
#define PRINT

// i2c

HardwareSerial GPSSerial(USART2);


packet p1;
packet p_send;

// flag to indicate that a packet was sent
volatile bool transmittedFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;

// this function is called when a complete packet
// is transmitted by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void) {
  // check if the interrupt is enabled
  if(!enableInterrupt) {
    return;
  }

  // we sent a packet, set the flag
  transmittedFlag = true;
}

// void BMPsetup();
// void BMPloop();

static float sim_alt = 0;
static float sim_mul = 100;

float simulation_altitude(){
  sim_alt += sim_mul;
  if(sim_alt > 3000){
    sim_mul = -100;
  }
  if (sim_alt < 0)
    sim_alt = 0;
  return sim_alt;
}

// save transmission state between loops
int transmissionState = ERR_NONE;

RFM95 radio = new Module(RFM95_CS, RFM95_INT, RFM95_RST, RFM95_DUMMY) ;//, SPI, SPISettings(8000000, MSBFIRST, SPI_MODE0));

float initial_altitude = 0;
uint32_t timer = millis();
uint32_t last_speed_time = millis();



STATE state = INIT;

#define PAYLOAD PB5 //M3
#define YAY     PB4 //M1
#define ANA     PB3 //M2

void setup() {
  pinMode(YAY, OUTPUT);
  digitalWrite(YAY, HIGH);
}

void loop() {

}
