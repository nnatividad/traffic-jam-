#include "timerISR.h"
#include "helper.h"
#include "spiAVR.h"
#include "periph.h"
#include "ST7735.h"
#include "serialATmega.h"



#define NUM_TASKS 3 //TODO: Change to the number of tasks being used


bool start = false; //start game flag
bool GameOver = false; //game over flag

//Task struct for concurrent synchSMs implmentations
typedef struct _task{
	signed 	 char state; 		//Task's current state
	unsigned long period; 		//Task period
	unsigned long elapsedTime; 	//Time elapsed since last task tick
	int (*TickFct)(int); 		//Task tick function
} task;


//TODO: Define Periods for each task
// e.g. const unsined long TASK1_PERIOD = <PERIOD>
const unsigned long JOYSTICK_PERIOD = 10;
const unsigned long BUTTON_PERIOD = 10;
const unsigned long GCD_PERIOD = 10;

task tasks[NUM_TASKS]; // declared task array with 5 tasks

//enum define states for tasks
enum ButtonStates {ButtonInit, off_release, on_press, startGame, off_press};
enum CountDownStates {BuzzerInit, Idle, Buzz, Delay, LastBuzzDelay};
enum JoystickStates {JoyStickInit, JoyStick_Idle, left_hold, right_hold, left_release, right_release};

//tick functions
int button_tick(int state); //button for start game
int countDown_tick(int state); //passive buzzer
int joyStick_tick(int state); //joystick for movement


void TimerISR() {
	for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {                   // Iterate through each task in the task array
		if ( tasks[i].elapsedTime == tasks[i].period ) {           // Check if the task is ready to tick
			tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
			tasks[i].elapsedTime = 0;                          // Reset the elapsed time for the next tick
		}
		tasks[i].elapsedTime += GCD_PERIOD;                        // Increment the elapsed time by GCD_PERIOD
	}
}


int main(void) {
    //TODO: initialize all your inputs and ouputs
    DDRC = 0xF8; PORTC = 0x07; //sets pins A0-A2, as inputs JOYSTICK A0, BUTTON A1
    DDRB = 0xFF; PORTB = 0x00;
    DDRD = 0xFF; PORTD = 0x00;
    //passive buzzer
    TCCR0A |= (1 << COM0A1);// use Channel A
    TCCR0A |= (1 << WGM01) | (1 << WGM00);// set fast PWM Mode
    TCCR0B = (TCCR0B & 0xF8) | 0x02;//set prescaler to 8
    // Set the PWM frequency
    OCR0A = 255;
    // Start the PWM
    TCCR0B |= (1 << CS01) | (1 << CS00); // Set the prescaler to 64 and start the PWM


    ADC_init();   // initializes ADC
    SPI_INIT();   // initializes SPI
    serial_init(9600); // initializes serial communication
    ST_init();    // initializes display
    setBackGroundColor(); // sets background color to white
    //startDisplay(); // displays start screen


    tasks[0].period = GCD_PERIOD;
    tasks[0].state = ButtonInit;
    tasks[0].elapsedTime = GCD_PERIOD;
    tasks[0].TickFct = &button_tick;
    tasks[1].period = GCD_PERIOD;
    tasks[1].state = BuzzerInit;
    tasks[1].elapsedTime = GCD_PERIOD;
    tasks[1].TickFct = &countDown_tick;
    tasks[2].period = JOYSTICK_PERIOD;
    tasks[2].state = JoyStickInit;
    tasks[2].elapsedTime = JOYSTICK_PERIOD;
    tasks[2].TickFct = &joyStick_tick;
    TimerSet(GCD_PERIOD);
    TimerOn();
    
    while (1) {}
    return 0;
}

int button_tick(int state) {
    static int countDown;
    switch(state){//transitions
        case ButtonInit:
            state = off_release;
            break;
        case off_release:
            if(GetBit(PINC, 1) == 1){
                state = on_press;
            }else{
                state = off_release;
            }
            start = false;
            break;
        case on_press:
            if(GetBit(PINC, 1) == 0){
                state = startGame;
                start = true;
            }else{
                state = on_press;
            }
            break;
        case startGame:
            if(GetBit(PINC, 1) == 1){
                state = off_press;
            }else if(countDown >= 600){
                state = off_release;
                countDown = 0;
                start = false;
            }else{
                state = startGame;
            }
            break;
        case off_press:
            if(GetBit(PINC, 1) == 0){
                state = off_release;
            }else{
                state = off_press;
            }
            break;
        default:
            break;
    }
    switch(state){//state actions
        case ButtonInit:
            break;
        case off_release:
            break;
        case on_press:
            break;
        case startGame:
            countDown++;
            break;
        case off_press:
            break;
        default:
            break;
    }
    return state;
}

int countDown_tick(int state){
    static int i;
    static int buzz;
    switch(state){//transitions
        case BuzzerInit:
            state = Idle;
            break;
        case Idle:
            if(start){
                    state = Buzz;
            }else{
                state = Idle;
                buzz = 4;
            }
            OCR0A = 255;
            break;
        case Buzz:
            if(i >= 100){
                state = Delay;
                OCR0A = 128;
                i = 0;
                if(buzz == 1){
                    serial_println("GO!");
                    startDisplay();
                }else{
                    serial_println(buzz - 1);
                }
                buzz--;
            }else{
                state = Buzz;
            }
            break;
        case Delay:
            if(buzz <= 0){
                state = Idle;
                buzz = 4;
            }else if(i >= 100 && buzz >= 0){
                state = Buzz;
                i = 0;
                OCR0A = 255;
            }else{
                state = Delay;
            }
            break;
        default:
            break;
    }
    switch(state){//state actions
        case BuzzerInit:
            i = 0;
            buzz = 4;
            break;
        case Idle:
            break;
        case Buzz:
            i++;
            break;
        case Delay:
            i++;
            break;
        default:
            break;
    }
    return state;
}

int joyStick_tick(int state){
    switch(state){//transitions
        case JoyStickInit:
            state = JoyStick_Idle;
            break;
        case JoyStick_Idle:
            if(ADC_read(0) > 700){
                state = right_hold;
            }else if(ADC_read(0) < 100){
                state = left_hold;
            }else{
                state = JoyStick_Idle;
            }
            break;
        case left_hold:
            if(ADC_read(0) < 100){
                state = left_hold;
            }else{
                state = left_release;
            }
            break;
        case right_hold:
            if(ADC_read(0) > 700){
                state = right_hold;
            }else{
                state = right_release;
            }
            break;
        case left_release:
            state = JoyStick_Idle;
            break;
        case right_release:
            state = JoyStick_Idle;
            break;
        default:
            break;
    }
    switch(state){//state actions
        case JoyStickInit:
            break;
        case JoyStick_Idle:
            break;
        case left_hold:
            break;
        case right_hold:
            break;
        case left_release:
                moveCar(XS - 20, XE - 20);
            break;
        case right_release:
                moveCar(XS + 20, XE + 20);
            break;
        default:
            break;
    }
    return state;
}
