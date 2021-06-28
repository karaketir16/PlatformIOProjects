#include <Arduino.h>
#include "gps.h"
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_LSM9DS1.h>
#include <Adafruit_Sensor.h>  // not used in this demo but required!
#include <RadioLib.h>

#include "bme.h"

#include "../../shared/shared.h"


// #define SIMULATION

// i2c
Adafruit_LSM9DS1 lsm = Adafruit_LSM9DS1();

HardwareSerial GPSSerial(USART2);

Adafruit_BMP3XX bmp;

// Adafruit_GPS GPS(&GPSSerial);


// #define LSM9DS1_SCK A5
// #define LSM9DS1_MISO 12
// #define LSM9DS1_MOSI A4
// #define LSM9DS1_XGCS PB0
// #define LSM9DS1_MCS PB1
// You can also use software SPI
//Adafruit_LSM9DS1 lsm = Adafruit_LSM9DS1(LSM9DS1_SCK, LSM9DS1_MISO, LSM9DS1_MOSI, LSM9DS1_XGCS, LSM9DS1_MCS);
// Or hardware SPI! In this case, only CS pins are passed in
// Adafruit_LSM9DS1 lsm = Adafruit_LSM9DS1(LSM9DS1_XGCS, LSM9DS1_MCS);

void setupSensor()
{
  // 1.) Set the accelerometer range
  lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_2G);
  //lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_4G);
  //lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_8G);
  //lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_16G);
  
  // 2.) Set the magnetometer sensitivity
  lsm.setupMag(lsm.LSM9DS1_MAGGAIN_4GAUSS);
  //lsm.setupMag(lsm.LSM9DS1_MAGGAIN_8GAUSS);
  //lsm.setupMag(lsm.LSM9DS1_MAGGAIN_12GAUSS);
  //lsm.setupMag(lsm.LSM9DS1_MAGGAIN_16GAUSS);

  // 3.) Setup the gyroscope
  lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_245DPS);
  //lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_500DPS);
  //lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_2000DPS);
}


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


void setup() {
  
  // ser();
  Serial.begin(38400);
  Serial.println("Booting");

  delay(1000);

  gps_setup();
  
  BMPsetup();

  int x = 10;
  while(x --> 0){
    BMPloop();
    initial_altitude = bmp.readAltitude(SEALEVELPRESSURE_HPA);
  }

  state = STATE::WAIT;

  // return;

  // SPI.setClockDivider(SPI_CLOCK_DIV2);

  if (!lsm.begin())
  {
    Serial.println("Oops ... unable to initialize the LSM9DS1. Check your wiring!");
    while (1);
  }
  Serial.println("Found LSM9DS1 9DOF");
  setupSensor();

  p1.package_number = 0;
  p1.acc_x = 0;
  p1.acc_y = 0;
  p1.acc_z = 0;

  p1.altitude = 0;
  p1.gps_string[0] = '0';
  p1.gps_string[1] = '\0';

  p1.pressure = 0;
  p1.speed = 0;
  p1.temperature = 0;
  
  // return;

  // initialize SX1278 with default settings
  Serial.print(F("[SX1278] Initializing ... "));
  int state = radio.begin();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
  // BMPsetup();
  // return;

  // set the function that will be called
  // when packet transmission is finished
  radio.setDio0Action(setFlag);

  state = radio.startTransmit((byte*) &p1, sizeof p1);
  

  last_speed_time = millis();
}

int pins[3] = {PC14, PC15, PA0};

int stages_times[3] = {0,0,0};


void check_stages(){
  for(int i =0;i < 3;i++){
    if( millis() - stages_times[i]> 750){
      digitalWrite(pins[i], LOW);
    }
  }
}
void fire_STAGE(int i){
  pinMode(pins[i], OUTPUT);
  digitalWrite(pins[i], HIGH);   
  stages_times[i] = millis();
}

void loop() {

  delay(100);

  // timer = millis();
  BMPloop();
  // Serial.print("Barometer read time: ");
  // Serial.println(millis() - timer);

  lsm.read();  /* ask it to read in the data */ 

  /* Get a new sensor event */ 
  sensors_event_t a, m, g, temp;

  lsm.getEvent(&a, &m, &g, &temp); 

  p1.package_number++;

  p1.acc_x = a.acceleration.x;
  p1.acc_y = a.acceleration.y;
  p1.acc_z = a.acceleration.z;

  float old_alt = p1.altitude;
  p1.altitude = bmp.readAltitude(SEALEVELPRESSURE_HPA) - initial_altitude;

  #ifdef SIMULATION
  p1.altitude = simulation_altitude();
  #endif

  p1.speed = (p1.altitude - old_alt) / ( (millis() - last_speed_time) / 1000 );
  last_speed_time = millis();

  p1.pressure = bmp.pressure / 100.0;
  p1.temperature = bmp.temperature;

  switch (state)
  {
  case STATE::INIT:
    break;
  case STATE::WAIT:
    if(p1.altitude > 200){
      state = STATE::RISING_STAGE;
    }
    break;
  case STATE::RISING_STAGE:
    if( p1.speed <= 0){
      state = STATE::FALLING_1;
      fire_STAGE(0);
    }
    break;
  case STATE::FALLING_1:
    if( p1.altitude < 1500){
      state = STATE::FALLING_2;
      fire_STAGE(1);
    }
    check_stages();
    break;
  case STATE::FALLING_2:
    if( p1.altitude < 600){
      state = STATE::FALLING_3;
      fire_STAGE(2);
    }
  check_stages();
    break;
  case STATE::FALLING_3:
    if( p1.altitude < 20){
      state = STATE::END_STAGE;
      pinMode(PC13, OUTPUT);
      digitalWrite(PC13, HIGH);
    }
  check_stages();
    break;
  case STATE::END_STAGE:
  check_stages();
    break;
  default:
    break;
  }



  String gps_data = gps_read();

  // Serial.print("GPS: ");
  // Serial.println(gps_data);

  strcpy(p1.gps_string, gps_data.c_str());
  p1.gps_string[gps_data.length()] = '\0';

  Serial.print("Pressure: ");
  Serial.println(p1.pressure);

  Serial.print("Altitude: ");
  Serial.println(p1.altitude);

  Serial.print("Temp: ");
  Serial.println(p1.temperature);

  Serial.print("GPS: ");
  Serial.println(p1.gps_string);

  Serial.print("Acceleration x: ");
  Serial.println(p1.acc_x);

  Serial.print("Acceleration y: ");
  Serial.println(p1.acc_y);

  Serial.print("Acceleration z: ");
  Serial.println(p1.acc_z);

  // check if the previous transmission finished
  if(transmittedFlag) {
    // disable the interrupt service routine while
    // processing the data
    enableInterrupt = false;

    // reset flag
    transmittedFlag = false;

    if (transmissionState == ERR_NONE) {
      // packet was successfully sent


      Serial.println(F("transmission finished!**********************************"));

      
      
      // NOTE: when using interrupt-driven transmit method,
      //       it is not possible to automatically measure
      //       transmission data rate using getDataRate()

    } else {
      Serial.print(F("failed, code "));
      Serial.println(transmissionState);

    }

    p_send = p1;
    // Serial.printf("press: %f\n accx: %f\n", p_send.pressure, p_send.acc_x);
   int state = radio.startTransmit((byte*) &p_send, sizeof p_send);

    // we're ready to send more packets,
    // enable interrupt service routine
    enableInterrupt = true;
  }
}
