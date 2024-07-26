#include "timerISR.h"
#include "helper.h"
#include "spiAVR.h"
#include "periph.h"
#include "ST7735.h"
#include "serialATmega.h"
#include "LCD.h"
#include <stdio.h>

//defining notes
#define E4 6000
#define D4 6750
#define CS4 7300
#define A4 9000

int notes_array[6] = {E4, 0, E4, D4, CS4, A4};
int duration_array[6] = {6, 2, 6, 4, 6, 2};


#define NUM_TASKS 6

bool on; //start game flag
bool left; //used to spawn enemies
bool right; //used to spawn enemies
bool gameOver; //game over flag
bool reset; //reset flag
unsigned char score; //score

bool checkCollision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    if (x1 < x2 + w2 &&
        x1 + w1 > x2 &&
        y1 < y2 + h2 &&
        y1 + h1 > y2) {
        return true;
    }
    return false;
}

void printScore(int score) { //helper function to print score to serial monitor
    char buffer[50];
    sprintf(buffer, "SCORE: %d", score);
    serial_println(buffer);
}

//Task struct for concurrent synchSMs implmentations
typedef struct _task{
	signed 	 char state; 		//Task's current state
	unsigned long period; 		//Task period
	unsigned long elapsedTime; 	//Time elapsed since last task tick
	int (*TickFct)(int); 		//Task tick function
} task;



//TODO: Define Periods for each task
// e.g. const unsined long TASK1_PERIOD = <PERIOD>
const unsigned long JOYSTICK_PERIOD = 50;
const unsigned long BUTTON_PERIOD = 100;
const unsigned long BUZZER_PERIOD = 100;
const unsigned long DISPLAY_PERIOD = 50;
const unsigned long GCD_PERIOD = 50;

task tasks[NUM_TASKS]; // declared task array with 5 tasks

//enum define states for tasks
enum ButtonStates {ButtonInit, on_release, on_press, off_release, off_press};
enum JoystickStates {JoyStickInit, JoyStick_Idle, left_hold, right_hold, left_release, right_release, button_hold, button_release};
enum BuzzerStates { BuzzerInit, BuzzerIdle, SongPlay } BuzzerState;
enum DisplayStates {DisplayInit, DisplayOff, DisplayOn};
enum GameStates {GameInit, GameOff, SpawnLeft, SpawnRight, Delay} GameState;
enum EnemyStates {EnemyInit, EnemyIdle, EnemyMove};

//tick functions
int button_tick(int state); //button for start game
int joyStick_tick(int state); //controls user movement
int display_tick(int state); //displaying characters
int buzzer_tick(int state);  //game song
int spawn_tick(int state);   //controls spawning enemies 
int moveEnemy_tick(int state); //controls enemy movement


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
    DDRC = 0xF8; PORTC = 0x07; //sets pins A0-A2, as inputs JOYSTICK A0, JOYSTICK BUTTON A2, BUTTON A1
    DDRB = 0xFF; PORTB = 0x00; //sets pins B0-B7 as outputs (LCD)
    DDRD = 0xFF; PORTD = 0x00; //sets pins D0-D7 as outputs (button)

    //passive buzzer
    TCCR1A |= (1 << WGM11) | (1 << COM1A1); //COM1A1 sets it to channel A
    TCCR1B |= (1 << WGM12) | (1 << WGM13) | (1 << CS11); //CS11 sets the prescaler to be 8
    ICR1 = 0;
    

    ADC_init();   // initializes ADC
    SPI_INIT();   // initializes SPI
    lcd_init();   // initializes LCD
    lcd_clear();  // clears LCD
    serial_init(9600); // initializes serial communication
    ST_init();    // initializes display
    setBackGroundColor(); // sets background color to white


    tasks[0].period = GCD_PERIOD;
    tasks[0].state = ButtonInit;
    tasks[0].elapsedTime = GCD_PERIOD;
    tasks[0].TickFct = &button_tick;
    tasks[1].period = JOYSTICK_PERIOD;
    tasks[1].state = JoyStickInit;
    tasks[1].elapsedTime = JOYSTICK_PERIOD;
    tasks[1].TickFct = &joyStick_tick;
    tasks[2].period = BUZZER_PERIOD;
    tasks[2].state = BuzzerInit;
    tasks[2].elapsedTime = BUZZER_PERIOD;
    tasks[2].TickFct = &buzzer_tick;
    tasks[3].period = GCD_PERIOD;
    tasks[3].state = DisplayInit;
    tasks[3].elapsedTime = GCD_PERIOD;
    tasks[3].TickFct = &display_tick;
    tasks[4].period = GCD_PERIOD;
    tasks[4].state = GameInit;
    tasks[4].elapsedTime = GCD_PERIOD;
    tasks[4].TickFct = &spawn_tick;
    tasks[5].period = GCD_PERIOD;
    tasks[5].state = EnemyInit;
    tasks[5].elapsedTime = GCD_PERIOD;
    tasks[5].TickFct = &moveEnemy_tick;
    TimerSet(GCD_PERIOD);
    TimerOn();
    
    while (1) {}
    return 0;
}

int button_tick(int state) {
    static bool buttonPressed = false;
    switch(state) {
        case ButtonInit:
            state = off_release;
            on = false;
            buttonPressed = false;
            break;
        case off_release:
            if(GetBit(PINC, 1) == 1) {
                state = on_press;
            } else {
                state = off_release;
            }
            break;
        case on_press:
            if(GetBit(PINC, 1) == 1) {
                state = on_press;
            } else {
                state = on_release;
            }
            break;
        case on_release:
            if(GetBit(PINC, 1) == 1) {
                state = off_press;
                buttonPressed = true;
            } else {
                if (gameOver) {
                    state = off_release;
                }
            }
            break;
        case off_press:
            if(GetBit(PINC, 1) == 1) {
                state = off_press;
            } else {
                state = off_release;
                on = false;
            }
            break;
        default:
            break;
    }

    switch(state) {
        case ButtonInit:
            break;
        case off_release:
            on = false;
            buttonPressed = false;
            break;
        case on_press:
            break;
        case on_release:
            if(!buttonPressed) {
                if(!on) {
                    on = true;
                }
                buttonPressed = true;
            }
            break;
        case off_press:
            break;
        default:
            break;
    }
    return state;
}

int joyStick_tick(int state){
    static int prevXS;
    static int prevXE;
    static int line_width = 4;
    static int start_column = (128 - line_width) / 2;
    static int end_column = start_column + line_width;

    switch(state){//transitions
        case JoyStickInit:
            state = JoyStick_Idle;
            prevXS = XS;
            prevXE = XE;
            break;
        case JoyStick_Idle:
            if(ADC_read(0) > 700){
                state = right_hold;
            }else if(ADC_read(0) < 100){
                state = left_hold;
            }else if(GetBit(PINC, 2) == 0){
                state = button_hold;
            }else{
                state = JoyStick_Idle;
            }
            break;
        case button_hold:
            if(GetBit(PINC, 2) == 0){
                state = button_hold;
            }else{
                state = button_release;
            }
            break;
        case button_release:
            state = JoyStick_Idle;
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
            XS = 20;
            XE = 40;
            break;
        case JoyStick_Idle:
            reset = false;
            if(!on){
                XS = 20;
                XE = 40;
            }
            break;
        case button_hold:
            break;
        case button_release:
            reset = true;
            XS = 20;
            XE = 40;
            break;
        case left_hold:
            if(on){
                prevXS = XS;
                prevXE = XE;
                if(XS > 10){
                    XS--;
                    XE--;
                }
                drawRectangle(start_column, end_column, 100, 140, BOUNDARY_COLOR, BOUNDARY_COLOR, BOUNDARY_COLOR);
                drawRectangle(prevXS, prevXE, YS, YE, 0, 0, 0);
                drawRectangle(XS, XE, YS, YE, 0, 0, BOUNDARY_COLOR);
            }
            break;
        case right_hold:
            if(on){
                prevXS = XS;
                prevXE = XE;
                if(XE < 120){
                    XS++;
                    XE++;
                }
                drawRectangle(start_column, end_column, 100, 140, BOUNDARY_COLOR, BOUNDARY_COLOR, BOUNDARY_COLOR);
                drawRectangle(prevXS, prevXE, YS, YE, 0, 0, 0);
                drawRectangle(XS, XE, YS, YE, 0, 0, BOUNDARY_COLOR);
            }
            break;
        case left_release:
            break;
        case right_release:
            break;
        default:
            break;
    }
    return state;
}

int display_tick(int state) {
    switch(state) { //transitions
        case DisplayInit:
            serial_println("TRAFFIC JAM! press button to start");
            state = DisplayOff;
            break;
        case DisplayOff:
            if(on) {
                state = DisplayOn;
                startDisplay();
            } else if (reset) {
                state = DisplayOff;
            } else {
                state = DisplayOff;
            }
            break;
        case DisplayOn:
            if(!on) {
                state = DisplayOff;
                setBackGroundColor(); //turn game off
            } else if (reset) {
                state = DisplayOff;
                setBackGroundColor(); // reset

            } else {
                state = DisplayOn;
            }
            break;
        default:
            break;
    }
    switch(state) { //state actions
        case DisplayInit:
            break;
        case DisplayOff:
            if(reset){
                serial_println("GAME RESET!"); // Print to serial
            }
            if(on) {
                startDisplay();
            }
            break;
        case DisplayOn:
            break;
        default:
            break;
    }
    return state;
}

int buzzer_tick(int state) {
    static int noteIndex;
    static int durationCount;

    switch(state) {
        case BuzzerInit:
            state = BuzzerIdle;
            noteIndex = 0;
            durationCount = 0;
            break;
        case BuzzerIdle:
            if(on) {
                state = SongPlay;
            } else if (reset) {
                state = BuzzerInit;
            } else {
                state = BuzzerIdle;
            }
            break;
        case SongPlay:
            if (!on || reset) {
                state = BuzzerIdle;
                noteIndex = 0;
                durationCount = 0;
            } else if (durationCount < duration_array[noteIndex]) {
                state = SongPlay;
                durationCount++;
            }else {
                noteIndex = (noteIndex + 1) % (sizeof(notes_array) / sizeof(notes_array[0]));
                durationCount = 0;
                state = SongPlay;
            }
            break;
        default:
            break;
    }

    switch(state) {
        case BuzzerInit:
            break;
        case BuzzerIdle:
            ICR1 = 0;
            break;
        case SongPlay:
            ICR1 = notes_array[noteIndex];
            OCR1A = ICR1 / 2;
            if (noteIndex == sizeof(notes_array) / sizeof(notes_array[0]) - 1) {
                noteIndex = 0;
            }
            break;
        default:
            break;
    }
    return state;
}

int spawn_tick(int state) {
    static int i; //used for delay
    switch(state) { //transitions
        case GameInit:
            state = GameOff;
            break;
        case GameOff:
            if(on) {
                state = Delay;
                i = 0; // Reset the counter when transitioning to Delay state
            } else if (gameOver || reset) {
                state = GameInit;
            }
            break;
        case SpawnLeft:
            if(on) {
                state = Delay;
            } else if (reset) {
                state = GameInit;
            } else {
                state = GameOff;
            }
            break;
        case SpawnRight:
            if(on) {
                state = Delay;
            } else if (reset) {
                state = GameInit;
            } else {
                state = GameOff;
            }
            break;
        case Delay:
            if(i >= 140 && on) {
                if(!left && !right) {
                    state = SpawnLeft;
                } else if (right && !left) {
                    state = SpawnRight;
                } else if(left && !right) {
                    state = SpawnLeft;
                }
            } else if (reset || !on) {
                state = GameInit;
            } else {
                state = Delay;
            }
            break;
        default:
            break;
    }
    switch(state) { //state actions
        case GameInit:
            left = false;
            right = false;
            i = 0;
            break;
        case GameOff:
            break;
        case SpawnLeft:
            LeftEnemy();
            right = true;
            left = false;
            i = 0;
            break;
        case SpawnRight:
            RightEnemy();
            left = true;
            right = false;
            i = 0;
            break;
        case Delay:
            i++;
            break;
        default:
            break;
    }
    return state;
}

int moveEnemy_tick(int state) {
    static int prevLeftYS;
    static int prevLeftYE;
    static int prevRightYS;
    static int prevRightYE;

    switch(state) {
        case EnemyInit:
            score = 0;
            state = EnemyIdle;
            break;
        case EnemyIdle:
            if(on && !gameOver) {
                state = EnemyMove;
                // Initialize variables
                prevLeftYS = LYS;
                prevLeftYE = LYE;
                prevRightYS = RYS;
                prevRightYE = RYE;
                score = 0;
                LYS = 0;
                LYE = 20;
                RYS = 0;
                RYE = 20;
            } else if (reset) {
                state = EnemyInit;
            } else {
                state = EnemyIdle;
            }
            break;
        case EnemyMove:
            if(gameOver || !on || reset) {
                state = EnemyIdle;
                gameOver = false;
            } else {
                state = EnemyMove;
            }
            break;
        default:
            break;
    }

    switch(state) {
        case EnemyInit:
            break;
        case EnemyIdle:
            prevLeftYS = LYS;
            prevLeftYE = LYE;
            prevRightYS = RYS;
            prevRightYE = RYE;
            score = 0;
            LYS = 0;
            LYE = 20;
            RYS = 0;
            RYE = 20;
            break;
        case EnemyMove:
            if(on && !gameOver) {
                // Move left enemy
                if(left) {
                    drawRectangle(LXS, LXE, prevLeftYS, prevLeftYE, 0, 0, 0);
                    prevLeftYS = LYS;
                    prevLeftYE = LYE;
                    if(LYS < 140) {
                        LYS++;
                        LYE++;
                    } else {
                        LYS = 0;
                        LYE = 20;
                        score++;
                    }
                    drawRectangle(LXS, LXE, LYS - 1, LYE - 1, 0, 0, 0);
                    drawRectangle(LXS, LXE, LYS, LYE, BOUNDARY_COLOR, 0, 0);
                } else if(right) { //move right enemy
                    drawRectangle(RXS, RXE, prevRightYS, prevRightYE, 0, 0, 0);
                    prevRightYS = RYS;
                    prevRightYE = RYE;
                    if(RYS < 140) {
                        RYS++;
                        RYE++;
                    } else {
                        RYS = 0;
                        RYE = 20;
                        score++;
                    }
                    drawRectangle(RXS, RXE, RYS - 1, RYE - 1, 0, 0, 0);
                    drawRectangle(RXS, RXE, RYS, RYE, BOUNDARY_COLOR, 0, 0);
                }
                // Check for collisions
                if(checkCollision(XS, YS, XE - XS, YE - YS, LXS, LYS, LXE - LXS, LYE - LYS) ||
                   checkCollision(XS, YS, XE - XS, YE - YS, RXS, RYS, RXE - RXS, RYE - RYS)) {
                    gameOver = true;
                    serial_println("GAME OVER! (press button to start new game)"); // Print to serial
                    printScore(score); // Print the score to serial
                }
            }
            break;
        default:
            break;
    }
    return state;
}