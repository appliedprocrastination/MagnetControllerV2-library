#include "MagnetControllerV2.h"

//#define DEBUG 1
//Constructor
MagnetController::MagnetController(int num_pcbs, uint8_t first_address, int magnets_per_pcb = 21, int fps = 2){
    _num_pcbs = num_pcbs;
    _magnets_per_pcb = magnets_per_pcb;
    _first_address = first_address;
    _frame_rate = fps;
    _frame_period = 1000 / _frame_rate; //ms

    if (_magnets_per_pcb > 16)
    {
        //The default case is that each PCB has 2 PCA9685 ICs.
        _ics_per_pcb = 2;
    }else
    {
        //Only necessary to initiate one PCA9685 per PCB
        _ics_per_pcb = 1;
    }
    _num_ics = _num_pcbs * 2; //There will still be 2 ICs per pcb, even if we only initialize one of them.
    _initialized_ics = _num_pcbs * _ics_per_pcb;
    _ics = new Adafruit_PWMServoDriver *[_initialized_ics];

    for (int i = 0; i < _num_ics; i++)
    {
        _ics[i] = new Adafruit_PWMServoDriver(_first_address + i);
        _ics[i]->begin();
        _ics[i]->setOutputMode(true); //Totempole is the default setting of the IC, but we're still setting this - just in case it has been changed unintentionally.
        _ics[i]->setPWMFreq(1600);
        i++; //Set address to the second IC on the same PCB. If _ics_per_pcb == 1 we still want to skip the address of the second IC on the PCB.
        if (_ics_per_pcb == 2)
        {
            _ics[i] = new Adafruit_PWMServoDriver(_first_address + i);
            _ics[i]->begin();
            _ics[i]->setOutputMode(true); //Totempole is the default setting of the IC, but we're still setting this - just in case it has been changed unintentionally.
            _ics[i]->setPWMFreq(1600);
        }
    }
}

//Deconstructor
MagnetController::~MagnetController(){
    for (int i = 0; i < _initialized_ics; i++)
    {
        delete _ics[i];
    }
    delete _ics;
}


void MagnetController::shiftOutFrame(Frame *frame){
    Adafruit_PWMServoDriver *current_ic1;
    Adafruit_PWMServoDriver *current_ic2; //only used when both ICs on the same board is initialized
    
    int ic_idx = 0;
    for (int y = 0; y < _num_pcbs; y++)
    {
        current_ic1 = _ics[ic_idx];
        if (_ics_per_pcb == 2)
        {
            ic_idx++;
            current_ic2 = _ics[ic_idx];
        }
        
        for (int x = 0; x < _magnets_per_pcb; x++)
        {
            //Do stuff with the current IC. Set ON/OFF state and PWM value.
            
            if (x < 16)
            {
                current_ic1->setPin(x, frame->get_pixel_intensity_at(x, y));
            }else{
                current_ic2->setPin(x - 16, frame->get_pixel_intensity_at(x, y));
            }

            # ifdef DEBUG_VERBOSE:
            Serial.print("Shifting out ");
            Serial.print(frame->get_pixel_intensity_at(x, y));
            Serial.print(" to pixel (x,y): (");
            Serial.print(x);
            Serial.print(",");
            Serial.print(y);
            Serial.print("). Readback value: ");
            if (x < 16)
            {
                Serial.println(current_ic1->getPWM(x));   
            }else{
                Serial.println(current_ic2->getPWM(x-16));
            }
            #endif;
        }
        ic_idx++;
    }
    #ifdef DEBUG:
    frame->print_to_terminal(true);
    #endif
    #ifdef DEBUG_VERBOSE:
    frame->print_to_terminal(false);
    #endif
   
}

void MagnetController::shiftOutPixel(int x, int y, uint16_t pixel_intensity)
{
    Adafruit_PWMServoDriver *current_ic1;
    Adafruit_PWMServoDriver *current_ic2; //only used when both ICs on the same board is initialized

    int ic_idx = 0;
    for (int row = 0; row < _num_pcbs; row++)
    {
        current_ic1 = _ics[ic_idx];
        if (_ics_per_pcb == 2)
        {
            ic_idx++;
            current_ic2 = _ics[ic_idx];
        }
        ic_idx++;
        if(row == y){
            break;
        }
    }

    if (x < 16)
    {
        current_ic1->setPin(x, pixel_intensity);
    }
    else
    {
        current_ic2->setPin(x - 16, pixel_intensity);
    }
    
#ifdef DEBUG_VERBOSE:
            Serial.print("Shifting out ");
            Serial.print(pixel_intensity);
            Serial.print(" to pixel (x,y): (");
            Serial.print(x);
            Serial.print(",");
            Serial.print(y);
            Serial.print("). Readback value: ");
            if (x < 16)
            {
                Serial.println(current_ic1->getPWM(x));
            }
            else
            {
                Serial.println(current_ic2->getPWM(x - 16));
            }
#endif;
}

void MagnetController::playAnimation(Animation *anim, void (*anim_done_event_handler)(void) = nullptr)
{
    _animation_playing = true;
    _anim_done_event_handler = anim_done_event_handler;
    _anim = anim;
    //Start the loaded animation according to its playback direction
    _anim->start_animation();
    //Send the first frame to the MagnetController(s). In "Fetch" this is equivalent to displaying the image on the screen.
    shiftOutFrame(_anim->get_current_frame());
    //Initialize timekeeping
    _time_this_refresh = millis();
    _time_for_next_frame = millis() + _frame_period;
}

void MagnetController::fade_pixels()
{
    //Fade out dying pixels (reducing the PWM value of pixels that are currently active but will be inactive in the next frame)
    //Fade in waking pixels (increasing the PWM value of pixels that are currently inactive but will become active in the next frame)
    unsigned long time_remaining = _time_for_next_frame - _time_this_refresh;
    float percent_of_frame_period_remaining = (float)time_remaining / (float)_frame_period;
    float min_intensity = 0;     //The lowest intensity that should be used during the fade. Intensities lower than this value will be converted to 0.
    float fade_start_percentage = 0.5;   //At which percentage of completion (of the frames duration) the fade should start
    if (percent_of_frame_period_remaining < fade_start_percentage) //&& time_remaining % 2 == 0
    {
        
        Frame* curr_frame = _anim->get_current_frame();
        Frame* next_frame = _anim->get_next_frame();
        int cols = curr_frame->get_width();
        int rows = curr_frame->get_height();
        for (int y = 0; y < rows; y++)
        {
            for (int x = 0; x < cols; x++)
            {
                uint16_t curr_intensity =  curr_frame->get_pixel_intensity_at(x,y);
                if (curr_intensity > 0 && next_frame->get_pixel_intensity_at(x,y) == 0)
                {
                    uint16_t intensity = curr_intensity * (percent_of_frame_period_remaining / fade_start_percentage);
                    if (intensity < min_intensity)
                    {
                        intensity = 0;
                    }
                    
                    shiftOutPixel(x, y, intensity);
                }
                else if (curr_intensity == 0 && next_frame->get_pixel_intensity_at(x, y) > 0)
                {

                    uint16_t intensity = DUTY_CYCLE_RESOLUTION * (1-(percent_of_frame_period_remaining / fade_start_percentage));
                    shiftOutPixel(x, y, intensity);
                }
            }
            
        }
    }
}

void MagnetController::animationManagement()
{
    //This function must be called in the main loop of the parent program in order for the animation
    //to adhere to its given framerate. 
    _time_this_refresh = millis();
    if (_animation_playing)
    {
        if (_time_this_refresh >= _time_for_next_frame)
        {

            _anim->goto_next_frame();

            if (_anim->anim_done())
            {
                _anim_done_event_handler();
            }
#ifdef DEBUG:
            Serial.print("Preparing to shift out frame number: ");
            Serial.print(_anim->get_current_frame_num());
            Serial.println();
#endif
            shiftOutFrame(_anim->get_current_frame());

            _time_for_next_frame += _frame_period;
        }
        else if(fading=true)
        {
            fade_pixels();
        }
        
    }
    
}
