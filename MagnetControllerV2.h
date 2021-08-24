/*
  MagnetControllerV2.h - library for shifting out Animations to a series of 
  PCA9685 based PCBs that can drive electromagnets. Used in Fetch.
  Copyright (c) 2020 Simen E. Sørensen. 
*/

// ensure this library description is only included once
#ifndef MagnetControllerV2_h
#define MagnetControllerV2_h

#include <Adafruit_PWMServoDriver.h>
#include <Wire.h>
#include <Animation.h> //AprocAnimation library

class MagnetController
{
public:
    MagnetController(int num_pcbs, uint8_t first_address, int magnets_per_pcb = 21, int fps = 2);
    ~MagnetController();
    //void playAnimation(Animation anim);
    void shiftOutFrame(Frame *frame);
    void shiftOutPixel(int x, int y, uint16_t pixel_intensity);
    void playAnimation(Animation *anim, void (*anim_done_event_handler)(void) = nullptr);
    void fade_pixels();
    void animationManagement();

  private:
    int _num_pcbs;
    int _magnets_per_pcb;
    int _ics_per_pcb;
    int _num_ics;
    int _initialized_ics;
    bool fading = true; //This should be stored in the Animation object, but is here now for testing.
    Adafruit_PWMServoDriver **_ics;
    uint8_t _first_address;
    void (*_anim_done_event_handler)(void);
    Animation *_anim = nullptr;
    boolean _animation_playing = false;
    unsigned long _time_this_refresh = 0;
    unsigned long _time_for_next_frame = 0;
    unsigned long _frame_rate;          //fps. 8 FPS seems to be the maximum our ferrofluid can handle 02.Sept.2019
    unsigned long _frame_period;        //ms
};

#endif