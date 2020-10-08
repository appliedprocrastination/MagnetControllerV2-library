#include "MagnetControllerV2.h"

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
    _num_ics = _num_pcbs *_ics_per_pcb;
    _ics = new Adafruit_PWMServoDriver *[_num_ics];

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
    for (int i = 0; i < _num_ics; i++)
    {
        delete _ics[i];
    }
    delete _ics;
}


void MagnetController::shiftOutFrame(Frame *frame){
    Adafruit_PWMServoDriver *current_ic;
    int ic_idx = 0;
    uint16_t upscaled; // TODO: delete this when the duty cycle of the Frame class has been increased to uint16_t (max 4096).
    for (int y = 0; y < _num_pcbs; y++)
    {
        current_ic = _ics[ic_idx];
        for (int x = 0; x < _magnets_per_pcb; x++)
        {
            if(x == 16){
                //Jump to next IC
                ic_idx++;
                current_ic = _ics[ic_idx];
            }
            //Do stuff with the current IC. Set ON/OFF state and PWM value.
            //TODO: Duty cycle can be increased to uint16_t (max 4096)
            upscaled = map(frame->get_duty_cycle_at(x,y),0,255,0,4096); 
            current_ic->setPWM(x,0,upscaled);
        }
        ic_idx++;
    }
   
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

void MagnetController::animationManagement(){
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
            Serial.print(anim.get_current_frame_num());
            Serial.println();
#endif
            shiftOutFrame(_anim->get_current_frame());

            _time_for_next_frame += _frame_period;
        }
    }
    
}
