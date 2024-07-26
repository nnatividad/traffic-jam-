#include "helper.h"
#include "spiAVR.h"

#ifndef ST7735_H
#define ST7735_H
int CS = 2;
int RESET = 0;
int A0 = 7;

void sendCommand(void){
    PORTD = SetBit(PORTD, A0, 0); //set A0 to low
}

void sendData(void){
    PORTD = SetBit(PORTD, A0, 1); //set A0 to high
}


void HW_Reset(void){
    PORTB = SetBit(PORTB, RESET, 0); //set reset pin to low
    _delay_ms(200);
    PORTB = SetBit(PORTB, RESET, 1); //set reset pin to high
    _delay_ms(200);
}

void ST_init(void){
    HW_Reset();
    PORTB = SetBit(PORTB, CS, 0); //set CS pin to low
    sendCommand();
    SPI_SEND(0x01); //SWRESET
    _delay_ms(150); 
    SPI_SEND(0x11); //SLPOUT
    _delay_ms(200);
    SPI_SEND(0x3A); //COLORMODE
    sendData();
    SPI_SEND(0x06); //18 bit color
    sendCommand();
    SPI_SEND(0x29); //DISPON
    _delay_ms(200);
}

void setBackGroundColor(){
    sendCommand();
    SPI_SEND(0x2A); 
    sendData();
    SPI_SEND(0x00);
    SPI_SEND(0x00);
    SPI_SEND(0x00);
    SPI_SEND(130); 

    sendCommand();
    SPI_SEND(0x2B); //RASET
    sendData();
    SPI_SEND(0x00); 
    SPI_SEND(0x00); 
    SPI_SEND(0x00); 
    SPI_SEND(128);

    sendCommand();
    SPI_SEND(0x2C); //RAMWR
    sendData();
    for(int i = 0; i < 128 * 168; i++){ //sets background
        SPI_SEND(0x00);
        SPI_SEND(0x00);
        SPI_SEND(0x00);
    } 
}

unsigned char BOUNDARY_WIDTH = 5;
unsigned char BOUNDARY_COLOR = 0xFF; //white

void drawRectangle(int start_column, int end_column, int start_row, int end_row, int blue, int green, int red){
    sendCommand();
    SPI_SEND(0x2A); //CASET
    sendData();
    SPI_SEND(0x00);
    SPI_SEND(start_column);
    SPI_SEND(0x00); 
    SPI_SEND(end_column);

    sendCommand();
    SPI_SEND(0x2B);
    sendData();
    SPI_SEND(0x00); 
    SPI_SEND(start_row); 
    SPI_SEND(0x00); 
    SPI_SEND(end_row); 

    sendCommand();
    SPI_SEND(0x2C); //RAMWR
    sendData();
    for(int i = 0; i < (end_column - start_column) * (end_row - start_row); i++){
        SPI_SEND(blue);
        SPI_SEND(green);
        SPI_SEND(red); 
    }
}


int XS = 20;
int XE = 40;
int YS = 100;
int YE = 120;

void startDisplay(void){ 
    //left boundary
    drawRectangle(0, BOUNDARY_WIDTH, 0, 160, BOUNDARY_COLOR, BOUNDARY_COLOR, BOUNDARY_COLOR);

    //right boundary
    int startColumn = ((130 - BOUNDARY_WIDTH) + BOUNDARY_WIDTH - 1) / BOUNDARY_WIDTH * BOUNDARY_WIDTH;
    drawRectangle(startColumn, 130, 0, 160, BOUNDARY_COLOR, BOUNDARY_COLOR, BOUNDARY_COLOR);

    //lanes
    int line_width = 4;
    int start_column = (128 - line_width) / 2;
    int end_column = start_column + line_width;
    drawRectangle(start_column, end_column, 0, 40, BOUNDARY_COLOR, BOUNDARY_COLOR, BOUNDARY_COLOR);
    drawRectangle(start_column, end_column, 50, 90, BOUNDARY_COLOR, BOUNDARY_COLOR, BOUNDARY_COLOR);
    drawRectangle(start_column, end_column, 100, 140, BOUNDARY_COLOR, BOUNDARY_COLOR, BOUNDARY_COLOR);

    //starting position of character
    drawRectangle(XS, XE, YS, YE, 0, 0, BOUNDARY_COLOR);
}

int LXS = 20;
int LXE = 40;
int LYS = 0;
int LYE = 20;
int RXS = 80;
int RXE = 100;
int RYS = 0;
int RYE = 20;
void LeftEnemy(){ //spawning enemy in left lane 
    int LXS = 20;
    int LXE = 40;
    drawRectangle(LXS, LXE, RYS, RYE, BOUNDARY_COLOR, 0, 0);
}

void RightEnemy(){  //spawning enemy in right lane
    drawRectangle(RXS, RXE, RYS, RYE, BOUNDARY_COLOR, 0, 0);
}
#endif //ST7735_H