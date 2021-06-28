#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>

#include "../../shared/shared.h"

#define RFM95_CS PA4
#define RFM95_RST PA3
#define RFM95_INT PA1

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
// RH_RF95 rf95(RFM95_CS, RFM95_INT);

uint8_t message[256];
uint8_t pt = 0;

void prtr(const char * ch, int size){
  memcpy(message + pt, ch, size);
  pt += size;
}

void prtr(std::string str){
  prtr(str.c_str(), str.length());
}

// Reverses a string 'str' of length 'len'
void reverse(char* str, int len)
{
    int i = 0, j = len - 1, temp;
    while (i < j) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++;
        j--;
    }
}

// Converts a given integer x to string str[]. 
// d is the number of digits required in the output. 
// If d is more than the number of digits in x, 
// then 0s are added at the beginning.
int intToStr(int x, char str[], int d)
{
    int i = 0;
    while (x) {
        str[i++] = (x % 10) + '0';
        x = x / 10;
    }
  
    // If number of digits required is more, then
    // add 0s at the beginning
    while (i < d)
        str[i++] = '0';
  
    reverse(str, i);
    str[i] = '\0';
    return i;
}
  
// Converts a floating-point/double number to a string.
int ftoa(float n, char* res, int afterpoint)
{
    // Extract integer part
    int ipart = (int)n;
  
    // Extract floating part
    float fpart = n - (float)ipart;
  
    // convert integer part to string
    int i = intToStr(ipart, res, 0);
  
    // check for display option after point
    if (afterpoint != 0) {
        res[i] = '.'; // add dot
  
        // Get the value of fraction part upto given no.
        // of points after dot. The third parameter 
        // is needed to handle cases like 233.007
        fpart = fpart * pow(10, afterpoint);
  
        i += intToStr((int)fpart, res + i + 1, afterpoint);
    }
    return i;
}

void prtr(double d){
  pt += ftoa(d, (char *)(message + pt), 2);
}

#include <Adafruit_GPS.h>
#include "gps.h"

#include <Adafruit_LSM9DS1.h>
#include <Adafruit_Sensor.h>  // not used in this demo but required!

#define GPSSerial Serial
Adafruit_GPS GPS(&GPSSerial);

void ser() 
{
  Serial.setRx(PA10); // using pin name PY_n
  Serial.setTx(PA9); // using pin number PYn
}

// include the library
#define LSM9DS1_XGCS PB4
#define LSM9DS1_MCS PB5

// SX1278 has the following connections:
// NSS pin:   10
// DIO0 pin:  2
// RESET pin: 9
// DIO1 pin:  3
RFM95 radio = new Module(RFM95_CS, RFM95_INT, RFM95_RST, PB8);

Adafruit_LSM9DS1 lsm = Adafruit_LSM9DS1(LSM9DS1_XGCS, LSM9DS1_MCS);

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

// or using RadioShield
// https://github.com/jgromes/RadioShield
//SX1278 radio = RadioShield.ModuleA;

packet p1;

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

void BMPsetup();
void BMPloop();

// save transmission state between loops
int transmissionState = ERR_NONE;

void setup() {
  ser();
  Serial.begin(9600);
  gps_setup();
  if (!lsm.begin())
  {
    Serial.println("Oops ... unable to initialize the LSM9DS1. Check your wiring!");
    while (1);
  }
  setupSensor();

  p1 = packet{1,2,3,4,5,6};
  
  // return;

  // initialize SX1278 with default settings
  // Serial.print(F("[SX1278] Initializing ... "));
  int state = radio.begin();
  if (state == ERR_NONE) {
    // Serial.println(F("success!"));
  } else {
    // Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
  BMPsetup();
  // return;

  // set the function that will be called
  // when packet transmission is finished
  radio.setDio0Action(setFlag);

  // start transmitting the first packet
  // Serial.print(F("[SX1278] Sending first packet ... "));

  // you can transmit C-string or Arduino string up to
  // 256 characters long
  // transmissionState = radio.startTransmit("Hello World!");
  prtr("\nHello World\n");
  prtr("\nHello World\n");
  // message[pt] = 0;
  // transmissionState = radio.startTransmit(message, pt + 1);
  // Serial.print((char *)message);

  // you can also transmit byte array up to 256 bytes long
  
    
    state = radio.startTransmit((byte*) &p1, sizeof p1);
  
}


void loop() {
  // BMPloop();
  // return;
  
  // check if the previous transmission finished
  if(transmittedFlag) {
    // disable the interrupt service routine while
    // processing the data
    enableInterrupt = false;

    // reset flag
    transmittedFlag = false;

    if (transmissionState == ERR_NONE) {
      // packet was successfully sent
      // Serial.println(F("transmission finished!"));
      BMPloop();
      gps_loop();
      if (GPS.fix){
        p1.gps_lati = GPS.lat;
        p1.gps_longi = GPS.lon;
      }
      lsm.read();  /* ask it to read in the data */ 

      /* Get a new sensor event */ 
      sensors_event_t a, m, g, temp;

      lsm.getEvent(&a, &m, &g, &temp); 

      p1.acc_x = a.acceleration.x;
      p1.acc_y = a.acceleration.y;
      p1.acc_z = a.acceleration.z;
      
      // NOTE: when using interrupt-driven transmit method,
      //       it is not possible to automatically measure
      //       transmission data rate using getDataRate()

    } else {
      // Serial.print(F("failed, code "));
      // Serial.println(transmissionState);

    }

    // NOTE: in FSK mode, SX127x will not automatically
    //       turn transmitter off after sending a packet
    //       set mode to standby to ensure we don't jam others
    //radio.standby()

    // wait a second before transmitting again
    delay(1000);

    // send another one
    // Serial.print(F("[SX1278] Sending another packet ... "));

    // you can transmit C-string or Arduino string up to
    // 256 characters long
    // transmissionState = radio.startTransmit("Hello World!");
    /*
    message[pt] = 0;
    transmissionState = radio.startTransmit(message, pt + 1);
    Serial.println();
    Serial.print((char *)message);
*/
    // you can also transmit byte array up to 256 bytes long
    /*
      byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                        0x89, 0xAB, 0xCD, 0xEF};
      int state = radio.startTransmit(byteArr, 8);
    */
   int state = radio.startTransmit((byte*) &p1, sizeof p1);

    // we're ready to send more packets,
    // enable interrupt service routine
    enableInterrupt = true;
  }
}



/***************************************************************************
  This is a library for the BMP3XX temperature & pressure sensor

  Designed specifically to work with the Adafruit BMP388 Breakout
  ----> http://www.adafruit.com/products/3966

  These sensors use I2C or SPI to communicate, 2 or 4 pins are required
  to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing products
  from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"


// #define BMP_SCK 13
// #define BMP_MISO 12
// #define BMP_MOSI 11
#define BMP_CS PA12

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BMP3XX bmp;



void BMPsetup() {
  // Serial.begin(115200);
  // while (!Serial);
  // Serial.println("Adafruit BMP388 / BMP390 test");

  // if (!bmp.begin_I2C()) {   // hardware I2C mode, can pass in address & alt Wire
  if (! bmp.begin_SPI(BMP_CS)) {  // hardware SPI mode  
  //if (! bmp.begin_SPI(BMP_CS, BMP_SCK, BMP_MISO, BMP_MOSI)) {  // software SPI mode
    // Serial.println("Could not find a valid BMP3 sensor, check wiring!");
    while (1);
  }

  // Set up oversampling and filter initialization
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);
}



void BMPloop() {
  // return;
  // memset(message,' ', 256);
  // Serial.print("0\n");
  pt = 0;
  if (! bmp.performReading()) {
    prtr("Failed to perform reading :(\n");
    // Serial.print("Failed to perform reading :(\n");
    return;
  }
  prtr("\n");
  prtr("Temperature = ");
  prtr(bmp.temperature);
  prtr(" *C\n");

  // Serial.print("3\n");

  prtr("Pressure = ");
  prtr(bmp.pressure / 100.0);
  prtr(" hPa\n");

  prtr("Approx. Altitude = ");
  prtr(bmp.readAltitude(SEALEVELPRESSURE_HPA));
  prtr(" m\n");

  prtr("\n");
  // delay(2000);
}