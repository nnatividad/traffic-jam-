#include "timerISR.h"
#include "helper.h"
#include "spiAVR.h"
#include "periph.h"
#include "ST7735.h"
#include "serialATmega.h"

//defining notes
#define C4  262
#define D4  294
#define E4  330
#define F4  349
#define G4  392
#define A4  440
#define B4  494

int notes_array[16] = {C4, D4, E4, F4, G4, A4, B4, C4, G4, F4, E4, D4, C4, 0, 0};
int duration_array[16] = {80, 80, 80, 80, 80, 80, 80, 160, 80, 80, 80, 80, 160, 80, 80};


#define NUM_TASKS 4 //TODO: Change to the number of tasks being used

bool countDown = false; //start game flag
bool GameOver = false; //game over flag
bool song = false; //song flag


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
enum SongStates { SongInit, SongIdle, SongPlay } songState;

//tick functions
int button_tick(int state); //button for start game
int countDown_tick(int state); //passive buzzer
int joyStick_tick(int state); //joystick for movement
int game_tick(int state); //game logic
int song_tick(int state);  //game song


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
    TCCR1A |= (1 << WGM11) | (1 << COM1A1); //COM1A1 sets it to channel A
    TCCR1B |= (1 << WGM12) | (1 << WGM13) | (1 << CS11); //CS11 sets the prescaler to be 8
    ICR1 = 196;
    

    ADC_init();   // initializes ADC
    SPI_INIT();   // initializes SPI
    serial_init(9600); // initializes serial communication
    ST_init();    // initializes display
    setBackGroundColor(); // sets background color to white


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
    tasks[3].period = GCD_PERIOD;
    tasks[3].state = SongInit;
    tasks[3].elapsedTime = GCD_PERIOD;
    tasks[3].TickFct = &song_tick;
    TimerSet(GCD_PERIOD);
    TimerOn();
    
    while (1) {}
    return 0;
}

int button_tick(int state) {
    static int x;
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
            countDown = false;
            break;
        case on_press:
            if(GetBit(PINC, 1) == 0 && !countDown){
                state = startGame;
                countDown = true;
            }else if (GetBit(PINC, 1) == 0 && countDown){
                state = off_press;
                }else{
                state = on_press;
            }
            break;
        case startGame:
            if(GetBit(PINC, 1) == 1){
                state = off_press;
            }else if(x >= 600){
                state = off_release;
                x = 0;
                countDown = false;
                song = true;
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
            x++;
            break;
        case off_press:
            song = false;
            break;
        default:
            break;
    }
    return state;
}

int countDown_tick(int state){//edit it to use timer1 (pin 9)
    static int i;
    static int buzz;
    switch(state){//transitions
        case BuzzerInit:
            state = Idle;
            break;
        case Idle:
            if(countDown){
                    state = Buzz;
            }else{
                state = Idle;
                buzz = 4;
            }
            OCR1A = 255;
            break;
        case Buzz:
            if(i >= 100){
                state = Delay;
                OCR1A = 0;
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
                OCR1A = ICR1 / 2;
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

int song_tick(int state) {
    static int noteIndex;
    static int durationCount;
    static int count;

    switch(state) {
        case SongInit:
            state = SongIdle;
            noteIndex = 0;
            durationCount = 0;
            break;
        case SongIdle:
            if (song) {
                state = SongPlay;
            } else {
                state = SongIdle;
            }
            break;
        case SongPlay:
            if (!song) {
                state = SongIdle;
                noteIndex = 0;
                durationCount = 0;
            } else if (durationCount < duration_array[noteIndex]) {
                state = SongPlay;
                durationCount++;
            }else if(count >= 3){
                state = SongIdle;
                noteIndex = 0;
                durationCount = 0;
                count = 0;
            } else {
                noteIndex = (noteIndex + 1) % (sizeof(notes_array) / sizeof(notes_array[0]));
                durationCount = 0;
                state = SongPlay;
            }
            break;
        default:
            break;
    }

    switch(state) {
        case SongInit:
            break;
        case SongIdle:
            break;
        case SongPlay:
            ICR1 = notes_array[noteIndex];
            OCR1A = ICR1 / 2;
            if (noteIndex == sizeof(notes_array) / sizeof(notes_array[0]) - 1) {
                noteIndex = 0;
                count++;
            }
            break;
        default:
            break;
    }
    return state;
}