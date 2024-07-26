#include "helper.h"
#include "spiAVR.h"

#ifndef ST7735_H
#define ST7735_H
int CS = 2;
int RESET = 0;
int A0 = 7;
unsigned char XS = 50;
unsigned char XE = 78;

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
    SPI_SEND(0x2A); //CASET
    sendData();
    SPI_SEND(0x00); // Start column address high byte
    SPI_SEND(0x00); // Start column address low byte
    SPI_SEND(0x00); // End column address high byte
    SPI_SEND(130); // End column address low byte

    sendCommand();
    SPI_SEND(0x2B); //RASET
    sendData();
    SPI_SEND(0x00); // Start row address high byte
    SPI_SEND(0x00); // Start row address low byte
    SPI_SEND(0x00); // End row address high byte
    SPI_SEND(128); // End row address low byte

    sendCommand();
    SPI_SEND(0x2C); //RAMWR
    sendData();
    for(int i = 0; i < 128 * 168; i++){ //sets background
        SPI_SEND(0x6F);
        SPI_SEND(0x6F);
        SPI_SEND(0x6F);
    } 
}

void startDisplay(void){
    
    sendCommand();
    SPI_SEND(0x2A); //CASET
    sendData();
    SPI_SEND(0x00); // Start column address high byte
    SPI_SEND(XS); // Start column address low byte
    SPI_SEND(0x00); // End column address high byte
    SPI_SEND(XE); // End column address low byte

    sendCommand();
    SPI_SEND(0x2B); //RASET
    sendData();
    SPI_SEND(0x00); // Start row address high byte
    SPI_SEND(100); // Start row address low byte
    SPI_SEND(0x00); // End row address high byte
    SPI_SEND(128); // End row address low byte

    sendCommand();
    SPI_SEND(0x2C); //RAMWR
    sendData();

    for(int i = 0; i < 28 * 28; i++){ //sets background red
        SPI_SEND(0x00); // Blue
        SPI_SEND(0x00); // Green
        SPI_SEND(0xFF); // Red
    }
}

void moveCar(unsigned char XS, unsigned char XE){
    sendCommand();
    SPI_SEND(0x2A); //CASET
    sendData();
    SPI_SEND(0x00); // Start column address high byte
    SPI_SEND(0x00); // Start column address low byte
    SPI_SEND(0x00); // End column address high byte
    SPI_SEND(130); // End column address low byte

    sendCommand();
    SPI_SEND(0x2B); //RASET
    sendData();
    SPI_SEND(0x00); // End row address high byte
    SPI_SEND(100); // Start row address low byte
    SPI_SEND(0x00); // End row address high byte
    SPI_SEND(128); // End row address low byte

    sendCommand();
    SPI_SEND(0x2C); //RAMWR
    sendData();
    for(int i = 0; i < 128 * 168; i++){ //sets background
        SPI_SEND(0x6F);
        SPI_SEND(0x6F);
        SPI_SEND(0x6F);
    } 

    sendCommand();
    SPI_SEND(0x2A); //CASET
    sendData();
    SPI_SEND(0x00); // Start column address high byte
    SPI_SEND(XS); // Start column address low byte
    SPI_SEND(0x00); // End column address high byte
    SPI_SEND(XE); // End column address low byte

    sendCommand();
    SPI_SEND(0x2C); //RAMWR
    sendData();

    for(int i = 0; i < 28 * 128; i++){ //sets background red
        SPI_SEND(0x00); // Blue
        SPI_SEND(0x00); // Green
        SPI_SEND(0xFF); // Red
    }
}

#endif //ST7735_H