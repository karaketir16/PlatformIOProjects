#ifndef _SHARED_
#define _SHARED_


#define RFM95_CS PB12
#define RFM95_RST PB13
#define RFM95_INT PB14
#define RFM95_DUMMY PB1

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0



typedef struct __attribute__((packed)) packet_t
{
    int package_number;
    float altitude;
    float pressure;
    float temperature;
    char  gps_string[100];
    float speed;
    float acc_x;
    float acc_y;
    float acc_z;
} packet;


enum STATE {
  INIT,
  WAIT,
  RISING_STAGE,
  FALLING_1,
  FALLING_2,
  FALLING_3,
  END_STAGE
};

#endif //_SHARED_
