#include <Arduino.h>

#define PASSTHROUGH 0 //Set to 1 just to test camera stream displayed on LCD, and check the UI

#ifdef __cplusplus
extern "C"
{
#endif
#if !PASSTHROUGH
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#define INCBIN_PREFIX
#include "incbin.h"
#include "names.h"
#include "region_layer.h"
#endif
#ifdef __cplusplus
}
#endif

#if !PASSTHROUGH
//Include model in memory
// INCBIN(model, "logoclassifier.kmodel");
// INCBIN(model, "deviceclassifier_5_0.kmodel");
INCBIN(model, "deviceclassifier.kmodel");
// INCBIN(model, "yolo.kmodel");
#endif

// Should be the same as image size during training
#define IMAGE_WIDTH               224
#define IMAGE_HEIGHT              224
#define VALID_CLASSIFY_THRESHOLD  0.1f //Change this depends on how confidence you are :)

#include <Sipeed_OV2640.h>
#include <Sipeed_ST7789.h>
#include "MobileNet.h"
#include "Maix_KPU.h"

SPIClass spi_(SPI0); // MUST be SPI0 for Maix series on board LCD
Sipeed_ST7789 lcd(320, 240, spi_);

Sipeed_OV2640 camera(IMAGE_WIDTH, IMAGE_HEIGHT, PIXFORMAT_RGB565);
KPUClass KPU;
MobileNet mbnet(KPU, lcd, camera);

//Specific for this demo
uint8_t detectedCount = 0;
unsigned long bootStart;
bool timePrinted = false;

void setup()
{
    bootStart = millis();
    Serial.begin(115200);
    while (!Serial) {};

    Serial.print("Serial up in ");
    Serial.println(millis() - bootStart);

    mbnet.setScreenRotation(2);

    int ret = mbnet.beginWithModelData(model_data, VALID_CLASSIFY_THRESHOLD);

    if(ret != 0)
    {
        Serial.print("Mobile net init is failed with err: ");
        Serial.println(ret);
        while(1);
    }

    Serial.print("Booted in ");
    Serial.println(millis() - bootStart);

    if (timePrinted == false) {
      timePrinted = true;
      Serial.print("First classification in ");
      Serial.println(millis() - bootStart);
    }
}

void loop()
{
    int status = mbnet.detect();

    if(status != 0)
    {
      Serial.print("Object classification failed: ");
      Serial.println(status);
      return;
    }

    mbnet.show();

    // Just for the demo on my video
#if !PASSTHROUGH
    //Specific for this demo
    // Serial.print("Prediction: ");
    Serial.println(mbnet.lastPredictionLabelIndex);
    if (mbnet.lastPredictionLabelIndex == 0) {
      detectedCount++;
    }
    else if (mbnet.lastPredictionLabelIndex == -1){
      detectedCount = 0;
    }
    else {
      detectedCount = 0;
    }

    if (detectedCount > 10) {
      if (lcd.getRotation() == 3 || lcd.getRotation() == 1) {
          uint16_t textX = (uint16_t)((lcd.width() - (strlen(mbnet_label_name[0])*1.0f*LABEL_TEXT_SIZE*6))/2);
          lcd.setCursor(textX, lcd.height() - 1*(8*LABEL_TEXT_SIZE + 8));
          lcd.print(mbnet_label_name[0]);
      }
    }
#endif
}
