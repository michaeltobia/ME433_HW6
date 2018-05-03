#include <xc.h>           // processor SFR definitions
#include <sys/attribs.h>  // __ISR macro
#include <stdio.h>        // standard i/o functions
#include "ST7735.h"       // TFTLCD Library

// DEVCFG0
#pragma config DEBUG = 0b00 // no debugging
#pragma config JTAGEN = 0b0 // no jtag
#pragma config ICESEL = 0b11 // use PGED1 and PGEC1
#pragma config PWP = 0b11111111 // no write protect
#pragma config BWP = 0b1 // no boot write protect
#pragma config CP = 0b1 // no code protect

// DEVCFG1
#pragma config FNOSC = 0b011 // use primary oscillator with pll
#pragma config FSOSCEN = 0b0 // turn off secondary oscillator
#pragma config IESO = 0b0 // no switching clocks
#pragma config POSCMOD = 0b10 // high speed crystal mode
#pragma config OSCIOFNC = 0b1 // disable secondary osc
#pragma config FPBDIV = 0b00 // divide sysclk freq by 1 for peripheral bus clock
#pragma config FCKSM = 0b10 // do not enable clock switch
#pragma config WDTPS = 0b00000 // use slowest wdt
#pragma config WINDIS = 0b1 // wdt no window mode
#pragma config FWDTEN = 0b0 // wdt disabled
#pragma config FWDTWINSZ = 0b11 // wdt window at 25%

// DEVCFG2 - get the sysclk clock to 48MHz from the 8MHz crystal
#pragma config FPLLIDIV = 0b001 //(2x: 8->4) divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = 0b111 // (24x: 4->96) multiply clock after FPLLIDIV
#pragma config FPLLODIV = 0b001 // (2: 96->48) divide clock after FPLLMUL to get 48MHz
#pragma config UPLLIDIV = 0b001 // (2: 8->4, 4*12->48) divider for the 8MHz input clock, then multiplied by 12 to get 48MHz for USB
#pragma config UPLLEN = 0b0 // USB clock on

// DEVCFG3
#pragma config USERID = 0b1010101010101010 // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = 0b0 // allow multiple reconfigurations
#pragma config IOL1WAY = 0b0 // allow multiple reconfigurations
#pragma config FUSBIDIO = 0b1 // USB pins controlled by USB module
#pragma config FVBUSONIO = 0b1 // USB BUSON controlled by USB module

void clearLCD(void);
void drawChar(unsigned short,unsigned short,char,unsigned short,unsigned short);
void drawString(unsigned short,unsigned short, char *,unsigned short,unsigned short);
void progBar(unsigned short, unsigned short, unsigned short, unsigned short,unsigned short,unsigned short,unsigned short);
void drawFPS(unsigned short, unsigned short, unsigned short, unsigned short);


int main(){
    
    __builtin_disable_interrupts();

    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;

    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;

    // disable JTAG to get pins back
    DDPCONbits.JTAGEN = 0;

    // do your TRIS and LAT commands here
    TRISAbits.TRISA4 = 0;
    TRISBbits.TRISB4 = 1;
    
    // SPI1 and LCD init
    SPI1_init();
    LCD_init();
    
    __builtin_enable_interrupts();
    
    clearLCD();
    
    // debug: draw a single character (H)
//    drawChar(10,10,'H',0xFFFF,0x0000);
    char message[30];
    sprintf(message, "Hello world!");
    drawString(28,32,message,WHITE,BLACK);
    int fill = 0;
    
    // debug: draw progress bar at 50% fill
//    progBar(10,30,10,50,CYAN,108,BLACK);
    for(fill = 0; fill <= 100; fill++){
        _CP0_SET_COUNT(0);
        progBar(28,42,10,fill,CYAN,78,BLACK);
        drawFPS(28,100,GREEN,BLACK);
        LATAINV = 0x10;
        fill++;
        while(_CP0_GET_COUNT() < 24000000/10){;}
    }
    while(1){
        drawFPS(28,100,GREEN,BLACK);
    }
   
    
}

// clearLCD: colors entire screen black
void clearLCD(void){
    int i = 0, j = 0;
    for(i = 0; i<128;i++){
        for (j = 0; j<160;j++){
            LCD_drawPixel(i,j,BLACK);
        }   
    }
}

// drawChar: draws a single character at (x,y) w/ foreground color c1 and
//           background color c2
void drawChar(unsigned short x, unsigned short y, char mes, unsigned short c1, unsigned short c2){
    char row = mes - 0x20;         // subtract for missing ascii command characters
    int col = 0;
    for (col = 0; col < 5; col++){
        char pixels = ASCII[row][col];
        int j = 0;
        for(j = 0; j < 8; j++){
            if((pixels>>j)&1==1){
                LCD_drawPixel(x+col,y+j,c1);
            }else{
                LCD_drawPixel(x+col,y+j,c2);
            }
        }
    }
    
}

// drawString: writes a sprintf character array at (x,y) w/ foreground color c1
//             and background color c2
void drawString(unsigned short x, unsigned short y, char *message, unsigned short c1, unsigned short c2){
   int i = 0;
   while (message[i]){
       drawChar(x+6*i,y,message[i],c1,c2);
       i++;
   }
}

// progBar: draws a progress bar at (x,y) w/ height h,
//          fill length len1 (0 to 100), total length len2, 
//          foreground color c1, and background color c2
void progBar(unsigned short x, unsigned short y, unsigned short h, unsigned short len1, unsigned short c1, unsigned short len2, unsigned short c2){
    int i = 0, j = 0, k = 0;
    if(len1 > 100){
        len1 = 100;
    }else if(len1 < 0){
        len1 = 0;
    }
    int fill = (int) (((((float) len1)/100)*len2)-1);
    for(j = 0; j < h; j++){
        LCD_drawPixel(x,y+j,c1);
        LCD_drawPixel(x+len2,y+j,c1);
    }
    for(i = 0; i <= len2; i++){
        LCD_drawPixel(x+i,y,c1);
        LCD_drawPixel(x+i,y+h,c1);
    }
    for(k = 0; k < fill; k++){
        for(j = 0; j < h; j++){
            LCD_drawPixel(x+1+k, y+1+j, c1);
        }
    }
}

void drawFPS(unsigned short x, unsigned short y, unsigned short c1, unsigned short c2){
    long int count1, count2;
    char mes_FPS[30];
    char count_FPS[30];
    
    
    count1 = _CP0_GET_COUNT();
    sprintf(mes_FPS, "FPS: ");
    count2 = _CP0_GET_COUNT();
    drawString(x,y,mes_FPS,c1,c2);
    float elapsed = ((24000000)/(count2 - count1))/100000;
    sprintf(count_FPS, "%.*f",2,elapsed);
    drawString(x+25,y,count_FPS,c1,c2); 
}

