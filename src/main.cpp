//#include "EasyOta.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <NeoPixelBus.h>
//#include <NeoPixelAnimator.h>
#include <SPI.h>
#include "RF24.h"

#define BOIA_FRAME 4
#define BOIA_SHOW 5

bool active = true;

long frame_count[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long frame_errors[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long frame_show_errors = 0;
long frame_show_count =0;

int mode = 0;


char message[32] = "";
long last_frame_time = millis();
uint8_t last_frame_show = 0;
uint8_t last_frame_data[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long last_log = millis();
long last_reset = millis();

int ultimo=0;

// LEDs configuration

    const int mapping[105] = {91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,
        90 , 89  ,88 , 87 , 86,  85 , 84 , 83  ,82 , 81 , 80 , 79, 78 , 77,  76,
        61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,
        60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,
        31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,
        30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,
        1 ,2,3 ,4,5,6, 7 ,8, 9 ,10,11,12,13,14,15};

    const uint8_t gamma8[256] = {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
        2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
        5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
    10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
    17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
    25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
    37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
    51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
    69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
    90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
    115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
    144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
    177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
    215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

    const uint16_t PixelCount = 105;
    NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, 2);

    void colorAll(RgbColor color){
        for (int p=0;p<PixelCount;p++){
            strip.SetPixelColor(p,color);
        }
        strip.Show();
        last_frame_time = millis();
    }



// Radio Configuratio
    RF24 radio(D2,D8);
    byte addresses[][6] = {"boias"};


void ping(){
    digitalWrite(LED_BUILTIN, 0);
}
void pong(){
    digitalWrite(LED_BUILTIN, 1);
}


void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    pong();

    Serial.begin(115200);
    Serial.println();
    Serial.println("Booting... ");

    strip.Begin();
    colorAll(RgbColor(0,0,10));

    radio.begin();
    //radio.setPALevel(RF24_PA_LOW);
    //radio.setDataRate(RF24_250KBPS);  
    radio.setDataRate(RF24_2MBPS);  
    radio.setChannel(60);
    radio.setAutoAck(false);
    radio.openReadingPipe(1,addresses[0]);  
    radio.startListening();

    delay(2000);

    Serial.println("Booted!");
}

void log(){
Serial.println("----------------");
        for (int i=0; i<12;i++){
            Serial.print(frame_errors[i]);
            Serial.print("\t\t");
            Serial.println(frame_count[i]);
        }
        Serial.println("----------------");
        Serial.print(frame_show_errors);
        Serial.print("\t\t");
        Serial.println(frame_show_count);
         Serial.print("--->");
        Serial.println(frame_show_count-ultimo);
        last_log = millis();
        ultimo = frame_show_count;
}


void loop() {
    if( radio.available()){
      ping();

      while (radio.available()) {
        radio.read( &message, sizeof(message) );
      }
      
      uint8_t command = (uint8_t) message[0];

      if (command == BOIA_SHOW){
        uint8_t frame = (uint8_t) message[1];
        last_frame_time = millis();
        strip.Show();

        if ((frame-last_frame_show)>1){
            frame_show_errors=frame_show_errors+(frame-last_frame_show)-1;
        }

        long diff1 = (frame - last_frame_show);
        if (diff1==-255){
            diff1=1;
        }

        frame_show_count = frame_show_count + diff1;
        last_frame_show=frame;
       

      }

      if (command == 4){
        uint8_t frame = (uint8_t) message[1];
        uint8_t part = (uint8_t) message[2];
        uint8_t conf3 = (uint8_t) message[3];
        uint8_t conf4 = (uint8_t) message[4];

        if ((frame-last_frame_data[part])>1){
            frame_errors[part] = frame_errors[part] + (frame-last_frame_data[part])-1;
        }

        long diff = (frame - last_frame_data[part]);
        if (diff==-255){
            diff=1;
        }
        frame_count[part] = frame_count[part] + diff;



        last_frame_data[part]=frame;

        uint8_t pixels_per_part = 9;

        for (int i=0; i<pixels_per_part; i++){
          int pixel = pixels_per_part*part+i;
          int pos = 5+3*i;

          if (pixel<=PixelCount){
            strip.SetPixelColor(mapping[pixel]-1,RgbColor((gamma8[(int)message[pos]]),gamma8[(int)message[pos+1]],gamma8[(int)message[pos+2]]));
          }           
        }

      }
         
      pong();
   }

    if (millis() - last_frame_time > 5000){
        colorAll(RgbColor(0,0,0));
    }

    if (millis()-last_log>1000){
        log();
    }

        if (millis()-last_reset>30000){
            memset(frame_count, 0, sizeof frame_count);
            memset(frame_errors, 0, sizeof frame_errors);
 
            frame_show_errors = 0;
            frame_show_count =0;

            last_reset=millis();
        
    }

    //delay(10);
}


