#ifndef _BME_
#define _BME_

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
// #include "Adafruit_BMP3XX.h"
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)

void BMPsetup();
void BMPloop();

extern Adafruit_BME280 bmp;

#endif