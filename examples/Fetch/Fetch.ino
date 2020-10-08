#include "MagnetControllerV2.h"
#include <Animation.h>
#include <SdFat.h>

MagnetController fetch = MagnetController(1, 0b1000000, 21, 2);
Animation anim;
SdFatSdioEX sd;
boolean sd_present = true;

void eventHandler(void);

void setup()
{
    Serial.begin(9600);
    while(millis()<5000){
        if(Serial){
            break;
        }
    }
    Serial.println("Serial initiated");

    if (!sd.begin())
    {
        Serial.println("SD initialitization failed. Read unsucessful.");
        sd_present = false;
    }
    //Prepare a pointer for storing an Animation 
    //anim = Animation(nullptr,1,true);
    
    //Read an animation from SD
    if (sd_present)
    {
        anim.read_from_SD_card(sd, 99);
    }

    fetch.playAnimation(&anim,eventHandler);
}

void loop()
{
    fetch.animationManagement();
}

void eventHandler(void){
    //Handle events such as "AnimationDoneEvent"

    //Example: Switch to new/next animation
    //Read an animation from SD
    if (sd_present)
    {
        anim.read_from_SD_card(sd, 100);
    }
    fetch.playAnimation(&anim,eventHandler);
}