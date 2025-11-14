#include "cmsis_os.h"                                           // CMSIS RTOS header file
#include "LPC17xx.h"                  
#include "osObjects.h"
#include "GLCD.h"
#include "KBD.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "image0.c"
#include "image1.c"
#include "image2.c"
#include "type.h"
#include "usb.h"
#include "usbcfg.h"
#include "usbhw.h"
#include "usbcore.h"
#include "usbaudio.h"

#define __FI        1                 

//States
#define HOMEPAGE        0
#define SELECTIMAGE     1
#define SELECTMP3       2
#define SELECTGAME      3
#define IMAGEMENU       4
#define GAMEOVER        6
#define GENERATEIMAGE   7
#define GENERATEMP3     9
#define GENERATEGAME    11

//Joystick direction 
#define LEFT    0
#define RIGHT   1
#define UP      2
#define DOWN    3


//Default configuration of the snake game
#define W 20
#define H 10
#define SNAKE_MAX_LEN 100
 
//Threads of the application
void joystick (void const *argument); 
void controller (void const *argument); 
void homepageScreen (void const *argument); 
void selectImageScreen (void const *argument); 
void selectMp3Screen (void const *argument); 
void selectGameScreen (void const *argument); 
void imageMenuScreen (void const *argument); 
void gameOverScreen (void const *argument);
void generateImage (void const *argument); 
void generateMp3 (void const *argument); 
void generateSnakeGame (void const *argument); 

//Methods for processing the snake game
int tilt(int dir);
void initSnake();
bool inSnake(int x, int y, int add);
void generateFood();
void moveSnake();
bool gameOver();
int wrapAround(int val, int add);
void pixelDisplay(int x, int y, unsigned short color);

//Methods for processing the audio
void get_potval (void);
void TIMER0_IRQHandler(void);
void mp3Init();


osThreadId tid1_Thread;
osThreadDef (joystick, osPriorityNormal, 1, 0);                  

osThreadId tid2_Thread;
osThreadDef (controller, osPriorityNormal, 1, 0);                

osThreadId tid3_Thread; 
osThreadDef (homepageScreen, osPriorityNormal, 1, 0);               

osThreadId tid4_Thread; 
osThreadDef (selectImageScreen, osPriorityNormal, 1, 0);                 

osThreadId tid5_Thread;
osThreadDef (selectMp3Screen, osPriorityNormal, 1, 0);   

osThreadId tid6_Thread;
osThreadDef (selectGameScreen, osPriorityNormal, 1, 0);     

osThreadId tid7_Thread;
osThreadDef (imageMenuScreen, osPriorityNormal, 1, 0);    

osThreadId tid9_Thread;
osThreadDef (gameOverScreen, osPriorityNormal, 1, 0);    

osThreadId tid10_Thread;
osThreadDef (generateImage, osPriorityNormal, 1, 0);   

osThreadId tid12_Thread;
osThreadDef (generateMp3, osPriorityNormal, 1, 0); 

osThreadId tid14_Thread;
osThreadDef (generateSnakeGame, osPriorityNormal, 1, 0);   

//Variables for the overall application
char text[10];
int state = 0;
int val = 0;

//Variables for the joystick
int KBD_flag = 0;
int joystick_dir[2] = {0,0};


//Variables for the snake game
int food_x;
int food_y;
int snake_len;
int snake_dir;
int snake[SNAKE_MAX_LEN][2];
int head;
int tail;

//Variables for the gallery
int image_val;
int image_run;

uint8_t  Mute;                                 /* Mute State */
uint32_t Volume;                               /* Volume Level */

#if USB_DMA
uint32_t *InfoBuf = (uint32_t *)(DMA_BUF_ADR);
short *DataBuf = (short *)(DMA_BUF_ADR + 4*P_C);
#else
uint32_t InfoBuf[P_C];
short DataBuf[B_S];                         /* Data Buffer */
#endif

uint16_t  DataOut;                              /* Data Out Index */
uint16_t  DataIn;                               /* Data In Index */

uint8_t   DataRun;                              /* Data Stream Run State */
uint16_t  PotVal;                               /* Potenciometer Value */
uint32_t  VUM;                                  /* VU Meter */
uint32_t  Tick;                                 /* Time Tick */

int Init_Thread (void) {
    
    //Initialze the LCD, joystick, and USB connection
	GLCD_Init(); 
	KBD_Init();
	mp3Init();
	
	tid1_Thread = osThreadCreate (osThread(joystick), NULL);
	tid2_Thread = osThreadCreate (osThread(controller), NULL);
	tid3_Thread = osThreadCreate (osThread(homepageScreen), NULL);
	tid4_Thread = osThreadCreate (osThread(selectImageScreen), NULL);
	tid5_Thread = osThreadCreate (osThread(selectMp3Screen), NULL);
	tid6_Thread = osThreadCreate (osThread(selectGameScreen), NULL);
	tid7_Thread = osThreadCreate (osThread(imageMenuScreen), NULL);
	tid9_Thread = osThreadCreate (osThread(gameOverScreen), NULL);
	tid10_Thread = osThreadCreate (osThread(generateImage), NULL);
	tid12_Thread = osThreadCreate (osThread(generateMp3), NULL);
	tid14_Thread = osThreadCreate (osThread(generateSnakeGame), NULL);
	
    if(!tid1_Thread) return(-1);
    if(!tid2_Thread) return(-1);
    if(!tid3_Thread) return(-1);
    if(!tid4_Thread) return(-1);
    if(!tid5_Thread) return(-1);
    if(!tid6_Thread) return(-1);
    if(!tid7_Thread) return(-1);
    if(!tid9_Thread) return(-1);
    if(!tid10_Thread) return(-1);
    if(!tid12_Thread) return(-1);
    if(!tid14_Thread) return(-1);
  
  return(0);
}

void get_potval (void) {
  uint32_t val;
  LPC_ADC->ADCR |= 0x01000000;              /* Start A/D Conversion */
  do {
    val = LPC_ADC->ADGDR;                   /* Read A/D Data Register */
  } while ((val & 0x80000000) == 0);      /* Wait for end of A/D Conversion */
  LPC_ADC->ADCR &= ~0x01000000;             /* Stop A/D Conversion */
  PotVal = ((val >> 8) & 0xF8) +          /* Extract Potenciometer Value */
           ((val >> 7) & 0x08);
}


/*
 * Timer Counter 0 Interrupt Service Routine
 *   executed each 31.25us (32kHz frequency)
 */

void TIMER0_IRQHandler(void) 
{
  long  val;
  uint32_t cnt;

  if (DataRun) {                            /* Data Stream is running */
    val = DataBuf[DataOut];                 /* Get Audio Sample */
    cnt = (DataIn - DataOut) & (B_S - 1);   /* Buffer Data Count */
    if (cnt == (B_S - P_C*P_S)) {           /* Too much Data in Buffer */
      DataOut++;                            /* Skip one Sample */
    }
    if (cnt > (P_C*P_S)) {                  /* Still enough Data in Buffer */
      DataOut++;                            /* Update Data Out Index */
    }
    DataOut &= B_S - 1;                     /* Adjust Buffer Out Index */
    if (val < 0) VUM -= val;                /* Accumulate Neg Value */
    else         VUM += val;                /* Accumulate Pos Value */
    val  *= Volume;                         /* Apply Volume Level */
    val >>= 16;                             /* Adjust Value */
    val  += 0x8000;                         /* Add Bias */
    val  &= 0xFFFF;                         /* Mask Value */
  } else {
    val = 0x8000;                           /* DAC Middle Point */
  }

  if (Mute) {
    val = 0x8000;                           /* DAC Middle Point */
  }
  LPC_DAC->DACR = val & 0xFFC0;             /* Set Speaker Output */

  if ((Tick++ & 0x03FF) == 0) {             /* On every 1024th Tick */
    get_potval();                           /* Get Potenciometer Value */
    if (VolCur == 0x8000) {                 /* Check for Minimum Level */
      Volume = 0;                           /* No Sound */
    } else {
      Volume = VolCur * PotVal;             /* Chained Volume Level */
    }
    val = VUM >> 20;                        /* Scale Accumulated Value */
    VUM = 0;                                /* Clear VUM */
    if (val > 7) val = 7;                   /* Limit Value */
  }

  LPC_TIM0->IR = 1;                         /* Clear Interrupt Flag */
  
  if(get_button() == KBD_LEFT && state == GENERATEMP3){
			NVIC_SystemReset();
			osSignalSet(tid12_Thread, 0x02);
  }
	
}

void mp3Init(){
    volatile uint32_t pclkdiv, pclk;

    /* SystemClockUpdate() updates the SystemFrequency variable */
    SystemCoreClockUpdate();
	
    LPC_PINCON->PINSEL1 &=~((0x03<<18)|(0x03<<20));  
    /* P0.25, A0.0, function 01, P0.26 AOUT, function 10 */
    LPC_PINCON->PINSEL1 |= ((0x01<<18)|(0x02<<20));

    /* Enable CLOCK into ADC controller */
    LPC_SC->PCONP |= (1 << 12);

    LPC_ADC->ADCR = 0x00200E04;		/* ADC: 10-bit AIN2 @ 4MHz */
    LPC_DAC->DACR = 0x00008000;		/* DAC Output set to Middle Point */

    /* By default, the PCLKSELx value is zero, thus, the PCLK for
    all the peripherals is 1/4 of the SystemFrequency. */
    /* Bit 2~3 is for TIMER0 */
    pclkdiv = (LPC_SC->PCLKSEL0 >> 2) & 0x03;
    switch ( pclkdiv )
    {
	    case 0x00:
	    default:
    	  pclk = SystemCoreClock/4;
    	break;
    	case 0x01:
    	  pclk = SystemCoreClock;
    	break; 
    	case 0x02:
    	  pclk = SystemCoreClock/2;
    	break; 
    	case 0x03:
    	  pclk = SystemCoreClock/8;
    	break;
      }

      LPC_TIM0->MR0 = pclk/DATA_FREQ - 1;	/* TC0 Match Value 0 */
      LPC_TIM0->MCR = 3;					/* TCO Interrupt and Reset on MR0 */
	  LPC_TIM0->TCR = 1;

}

//Get the event from the joystick
int tilt(int dir){

    
    if( (dir == LEFT && KBD_flag == 1 && KBD_val == KBD_LEFT) ||
        (dir == RIGHT && KBD_flag == 1 && KBD_val == KBD_RIGHT) ||
        (dir == UP && KBD_flag == 1 && KBD_val == KBD_UP) ||
        (dir == DOWN && KBD_flag == 1 && KBD_val == KBD_DOWN)){
        KBD_flag = 0;
        return 1;
    }
    return 0;
    
}

//Generate the event from the joystick
void joystick (void const *argument) {
    while(1){
        joystick_dir[0] = joystick_dir[1];
        joystick_dir[1] = get_button();
        if(joystick_dir[0] == 0 && joystick_dir[0] != joystick_dir[1]){
            KBD_val = joystick_dir[1];
            KBD_flag = 1;

        }
        osDelay(10);
    }
}


//Controller thread
void controller (void const *argument) {
    while(1){
        switch(state){
            case HOMEPAGE:
                osSignalSet(tid3_Thread, 0x01);
				osSignalWait(0x01, osWaitForever);
                state = SELECTIMAGE;
                break;
            case SELECTIMAGE:
                osSignalSet(tid4_Thread, 0x01);
                if(tilt(DOWN))state = SELECTMP3;
                else if(tilt(RIGHT)) state = IMAGEMENU;
                break;
            case SELECTMP3:
                osSignalSet(tid5_Thread, 0x01);
                if(tilt(DOWN)) state = SELECTGAME;
                else if(tilt(UP)) state = SELECTIMAGE;
                else if(tilt(RIGHT)) state = GENERATEMP3;
                break;
            case SELECTGAME:
                osSignalSet(tid6_Thread, 0x01);
                if(tilt(UP)) state = SELECTMP3;
                else if(tilt(RIGHT)) state = GENERATEGAME;
                break;
            case IMAGEMENU:
                osSignalSet(tid7_Thread, 0x01);
                state = GENERATEIMAGE;
                break;
            case GAMEOVER:
                osSignalSet(tid9_Thread, 0x01);
                if(tilt(LEFT)) state = HOMEPAGE;
                else if(tilt(RIGHT)) state = GENERATEGAME;
                break;
            case GENERATEIMAGE:
                osSignalSet(tid10_Thread, 0x01);
                osSignalWait(0x01, osWaitForever);
                state = HOMEPAGE;
                break;
            case GENERATEMP3:
                osSignalSet(tid12_Thread, 0x01);
				osSignalWait(0x01, osWaitForever);
                state = HOMEPAGE;
                break;
            case GENERATEGAME:
                osSignalSet(tid14_Thread, 0x01);
                osSignalWait(0x01, osWaitForever);
                state = GAMEOVER;
                break;
        }
        
    osDelay(100);
    }
    
}

//Display the homepage
void homepageScreen (void const *argument) {
    while(1){
        osSignalWait(0x01, osWaitForever);
        
        GLCD_Clear(Black);  
        GLCD_SetBackColor(Black);
        GLCD_SetTextColor(White);
	    GLCD_DisplayString(1, 1, __FI, "1. Gallery");
        GLCD_DisplayString(2, 1, __FI, "2. MP3");
        GLCD_DisplayString(3, 1, __FI, "3. Snake Game");
        GLCD_DisplayString(8, 0, __FI, "Navigate: up/down");
        GLCD_DisplayString(9, 0, __FI, "Select: right");
				osSignalSet(tid2_Thread, 0x01);
        
    }

}

//Highlight the gallery option
void selectImageScreen (void const *argument) {
    while(1){
        osSignalWait(0x01, osWaitForever);
        
        GLCD_SetTextColor(Red);
        GLCD_DisplayString(1, 1, __FI, "1. Gallery");
        GLCD_SetTextColor(White);
        GLCD_DisplayString(2, 1, __FI, "2. MP3");
        GLCD_DisplayString(3, 1, __FI, "3. Snake Game");
    }
}

//Highlight the mp3 option
void selectMp3Screen (void const *argument) {
    while(1){
        osSignalWait(0x01, osWaitForever);
        
        GLCD_SetTextColor(Red);
        GLCD_DisplayString(2, 1, __FI, "2. MP3");
        GLCD_SetTextColor(White);
        GLCD_DisplayString(1, 1, __FI, "1. Gallery");
        GLCD_DisplayString(3, 1, __FI, "3. Snake Game");
    }
}

//Highlight the game option
void selectGameScreen (void const *argument) {
   while(1){
        osSignalWait(0x01, osWaitForever);
        
        GLCD_SetTextColor(Red);
        GLCD_DisplayString(3, 1, __FI, "3. Snake Game");
        GLCD_SetTextColor(White);
        GLCD_DisplayString(1, 1, __FI, "1. Gallery");
        GLCD_DisplayString(2, 1, __FI, "2. MP3");
    }
}

//Display the menu of the gallery
void imageMenuScreen (void const *argument) {
    while(1){
        osSignalWait(0x01, osWaitForever);
        
        GLCD_Clear(Black);  
        GLCD_SetTextColor(White);
	    GLCD_DisplayString(8, 0, __FI, "Next: right");
        GLCD_DisplayString(9, 0, __FI, "Back: left");
    }

}

//Display a "game over" text in the LCD 
void gameOverScreen (void const *argument) {
    while(1){
        osSignalWait(0x01, osWaitForever);
        
        GLCD_SetTextColor(White);
        GLCD_DisplayString(4, 3, __FI, "Game Over!");
		GLCD_DisplayString(9, 0, __FI, "Back: left");
        GLCD_DisplayString(8, 0, __FI, "Restart: right");
    }

}

//Generate the gallery feature
void generateImage (void const *argument) {
    while(1){
        osSignalWait(0x01, osWaitForever);
        
        image_val = 0;
        image_run = true;
        
        while(image_run){
            //Get the next image
		    image_val++;
            image_val = image_val%3;
            GLCD_SetTextColor(Black);
	        GLCD_Bargraph (0, 0, 320, 192, 320);
					
					//Display that selected image
					switch(image_val){
						case 0:
							GLCD_Bitmap (  0,   0, IMAGE0_WIDTH, IMAGE0_HEIGHT, IMAGE0_PIXEL_DATA);
							break;
						case 1:
							GLCD_Bitmap (  0,   0, IMAGE1_WIDTH, IMAGE1_HEIGHT, IMAGE1_PIXEL_DATA);
							break;
						case 2:
							GLCD_Bitmap (  0,   0, IMAGE2_WIDTH, IMAGE2_HEIGHT, IMAGE2_PIXEL_DATA);
							break;
							
					}
	        
	        //Wait an event to occur in the joystick
	        while(true){
	            if(tilt(RIGHT)) break;
	            else if(tilt(LEFT)){
	                image_run = false;
	                break;
	            }
	            osDelay(100);
	        }
        }
        osSignalSet(tid2_Thread, 0x01);
    }

}


//Generate the MP# feature
void generateMp3 (void const *argument) {
    while(1){
        osSignalWait(0x01, osWaitForever);
        
        //Display the menu of the mp3
        GLCD_Clear(Black);  
        GLCD_SetTextColor(White);
        GLCD_DisplayString(9, 0, __FI, "Back: left");
        GLCD_DisplayString(4, 1, __FI, "Audio Streaming...");
				
		//Connect to the USB port
		NVIC_EnableIRQ(TIMER0_IRQn); 
		USB_Init();
		USB_Connect(TRUE);
        
        osSignalWait(0x02, osWaitForever);
        osSignalSet(tid2_Thread, 0x01);
    }
}

//Generate the game feature
void generateSnakeGame (void const *argument) {
    while(1){
        osSignalWait(0x01, osWaitForever);
        
        initSnake();
        generateFood();
    	GLCD_Clear(Black); 
	    pixelDisplay(snake[tail][0], snake[tail][1], White);
		pixelDisplay(snake[head][0], snake[head][1], White);
		pixelDisplay(food_x, food_y, Red);
		osDelay(3000);
    
    	while(true){
    		pixelDisplay(snake[tail][0], snake[tail][1], Black);
    		moveSnake();
    
    		if(gameOver()) break;
    
    		pixelDisplay(snake[head][0], snake[head][1], White);
    
    		if(inSnake(food_x, food_y, 0)){
    			snake_len++;
    			tail = wrapAround(tail, -1);
    			generateFood();
    			pixelDisplay(snake[tail][0], snake[tail][1], White);
    			pixelDisplay(food_x, food_y, Red);
    		}
    		
    		osDelay(3000);
    	}
    		 
        osSignalSet(tid2_Thread, 0x01);
        
    }
}

//Set the default length of the snake
void initSnake(){
	food_x = 0;
	food_y = 0;
	snake_len = 2;
	snake_dir = RIGHT;
	tail = 0;
	head = 1;

	snake[tail][0] = W/2;
	snake[tail][1] = H/2;

	snake[head][0] = W/2 + 1;
	snake[head][1] = H/2;
}

//Determine if the given coordinates is within the snake's body
bool inSnake(int x, int y, int add){
	int i;
	for(i = 0; i < snake_len+add; i++){
		if(x == snake[wrapAround(tail, i)][0] && y == snake[wrapAround(tail, i)][1]) return true;
	}
	return false;
}

//Radomly generate a food
void generateFood(){
	food_x = rand()%W;
	food_y = rand()%H;
	while(inSnake(food_x, food_y, 0)){
		food_x = rand()%W;
		food_y = rand()%H;
	}
}

//Move the snake based on the event from the joystick
void moveSnake(){
	if((tilt(RIGHT) || tilt(LEFT)) && (snake_dir == UP || snake_dir == DOWN)){
		if(KBD_val == KBD_LEFT) snake_dir = LEFT;
		else snake_dir = RIGHT;
	}

	if((tilt(UP) || tilt(DOWN)) && (snake_dir == LEFT || snake_dir == RIGHT)){
		if(KBD_val == KBD_UP) snake_dir = UP;
		else snake_dir = DOWN;
	}
	
	tail = wrapAround(tail, 1);
	head = wrapAround(head, 1);
	switch(snake_dir){
		case RIGHT:
			snake[head][0] = snake[wrapAround(head, -1)][0] + 1;
			snake[head][1] = snake[wrapAround(head, -1)][1];
			break;
		case LEFT:
			snake[head][0] = snake[wrapAround(head, -1)][0] - 1;
			snake[head][1] = snake[wrapAround(head, -1)][1];
			break;
		case UP:
			snake[head][0] = snake[wrapAround(head, -1)][0];
			snake[head][1] = snake[wrapAround(head, -1)][1] - 1;
			break;
		case DOWN:
			snake[head][0] = snake[wrapAround(head, -1)][0];
			snake[head][1] = snake[wrapAround(head, -1)][1] + 1;
			break;
			
	}
}

//Check if the snake goes beyond the boundary or the snake has bitten its body
bool gameOver(){
	return snake[head][0] < 0 || snake[head][0] >= W || snake[head][1] < 0 || snake[head][1] >= H || inSnake(snake[head][0],snake[head][1],-1);
}

//For extracting the index of the snake's body
int wrapAround(int val, int add){
	val += add;
	if(val < 0) val += SNAKE_MAX_LEN;
	return val%SNAKE_MAX_LEN;
}

//Display a pixel in a 20x10 grid;
void pixelDisplay(int x, int y, unsigned short color){
	GLCD_SetBackColor(color);
	GLCD_DisplayString(y, x, __FI, " ");
	GLCD_SetBackColor(Black);
}
