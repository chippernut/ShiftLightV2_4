/* 
   ________    _                                   __ 
  / ____/ /_  (_)___  ____  ___  _________  __  __/ /_
 / /   / __ \/ / __ \/ __ \/ _ \/ ___/ __ \/ / / / __/
/ /___/ / / / / /_/ / /_/ /  __/ /  / / / / /_/ / /_  
\____/_/ /_/_/ .___/ .___/\___/_/  /_/ /_/\__,_/\__/  
            /_/   /_/         


Visit us: www.chippernut.com 


ARDUINO RPM TACHOMETER MULTI-DISPLAY 
Written by Jonduino (Chippernut)
03-20-2016

********************* Version NOTES ******************** 
/* v1.0 BETA 11/17/2013 -- Initial Release 
** v1.1 03/09/2014 - 
** Fixed bug with flasher that didn't correspond to brightness value 
** Improved sleep function, now it shuts off after 5-seconds of engine OFF 
** Other minor improvements. 
** v2.1 08/10/2015 -- Too many to list here. 
** v2.2 01/17/2016 -- 
** New Display Support
** v2.4 03/20/2016 --
** Added dimmer support 
** Fixed array mathematics (bug)
** Improved rotary encoder response
** Finer PPr Control (0.1)
*/ 


// Include these libraries 
#include <Wire.h> 
#include <TM1637Display.h>                                                //#include <Adafruit_LEDBackpack.h> 
#include <Adafruit_GFX.h> 
#include <Adafruit_NeoPixel.h> 
#include <EEPROM.h> 
#include <EEPROMAnything.h> 
#include <FreqMeasure.h>


void(* resetFunc) (void) = 0;
int DEBUG;
int NUMPIXELS; 
#define PIN 6 
unsigned int Color(byte r, byte g, byte b) 
{ 
return( ((unsigned int)g & 0x1F )<<10 | ((unsigned int)b & 0x1F)<<5 | (unsigned int)r & 0x1F); 
} 

Adafruit_NeoPixel strip = Adafruit_NeoPixel(EEPROM.read(11), PIN, NEO_GRB + NEO_KHZ800); 
                                             
const int rpmPin = 2; 
const int ledPin = 13; 
const int dimPin = 9;
const int sensorInterrupt = 0; 
const int timeoutValue = 10; 
volatile unsigned long lastPulseTime; 
volatile unsigned long interval = 0; 
volatile int timeoutCounter; 
long rpm;
long rpm_last; 
int activation_rpm; 
int shift_rpm; 
int menu_enter = 0; 
int current_seg_number = 1;
int seg_mover = 0;
long previousMillis = 0;
long shiftinterval = 50;
boolean flashbool = true;      
int prev_animation;
int prev_color;
boolean testbright = false;
long prev_variable; 
int prev_menu;
boolean testdim = false; 

//array for rpm averaging, filtering comparison
const int numReadings = 5;
int rpmarray[numReadings];
int index = 0;                  // the index of the current reading
long total = 0;                  // the running total
long average = 0;                // the average


//These are stored memory variables for adjusting the (5) colors, activation rpm, shift rpm, brightness 
//Stored in EEPROM Memory 
int c1; 
int c2; 
int c3; 
int c4; 
int c5; 
int brightval;      //7-seg brightness 
int dimval;         //7-seg dim brightness
int sb;             //strip brightness
int dimsb;          // strip dim brightness
boolean dimmer = false;
boolean dimmerlogic = false;
int pixelanim=1;                                     
int senseoption;
int smoothing;
int rpmscaler;
long shift_rpm1;
long shift_rpm2;
long shift_rpm3;
long shift_rpm4;
int seg1_start = 1;
int seg1_end = 1;
int seg2_start = 2;
int seg2_end = 2; 
int seg3_start = 3;
int seg3_end = 3;
long activation_rpm1;
long activation_rpm2;
long activation_rpm3;
long activation_rpm4;
long cal1;
long cal2;
long cal3; 
long cal4;
int rst = 0; 
int cal; 
long calfunc;
int prev_cal;

// COLOR VARIABLES - for use w/ the strips and translated into 255 RGB colors 
uint32_t color1; 
uint32_t color2; 
uint32_t color3; 
uint32_t flclr1; 
uint32_t flclr2; 

//Creates a 32 wide table for our pixel animations
int rpmtable[32][3];

// ROTARY ENCODER VARIABLES 
int button_pin = 4; 
int menuvar; 
int val; 
int rotaryval = 0; 

// CONFIGURATION FOR 7-SEG DISPLAY
#define CLK A2
#define DIO A3
TM1637Display display(CLK, DIO);
 uint8_t SEG_WRITE[] = {   // [=] EXIT
  SEG_A | SEG_F| SEG_E | SEG_D,          
  SEG_A | SEG_D,   
  SEG_A | SEG_D,                          
  SEG_A | SEG_D | SEG_B | SEG_C           
  };
  
//
//      A
//     ---
//  F |   | B
//     -G-
//  E |   | C
//     ---
//      D



// CONFIGURATION FOR THE ROTARY ENCODER 
  // Arduino pins the encoder is attached to. Attach the center to ground. 
  #define ROTARY_PIN1 10 
  #define ROTARY_PIN2 11 
  
  // Use the full-step state table (emits a code at 00 only) 
  const char ttable[7][4] = { 
  {0x0, 0x2, 0x4, 0x0}, {0x3, 0x0, 0x1, 0x40}, 
  {0x3, 0x2, 0x0, 0x0}, {0x3, 0x2, 0x1, 0x0}, 
  {0x6, 0x0, 0x4, 0x0}, {0x6, 0x5, 0x0, 0x80}, 
  {0x6, 0x5, 0x4, 0x0}, 
  }; 
  
  volatile unsigned char state = 0; 
  
char rotary_process() {
  char pinstate = (digitalRead(ROTARY_PIN2) << 1) | digitalRead(ROTARY_PIN1);
  state = ttable[state & 0xf][pinstate];
  return (state & 0xc0);
}

int check_mem() {
  uint8_t * heapptr;
  uint8_t * stackptr;
  stackptr = (uint8_t *)malloc(4);          // use stackptr temporarily
  heapptr = stackptr;                     // save value of heap pointer
  free(stackptr);      // free up the memory again (sets stackptr to 0)
  stackptr =  (uint8_t *)(SP);           // save value of stack pointer
  return ((int)stackptr - (int)heapptr);
}


                              
                              /*************
                               * SETUP *
                              *************/

//SETUP TO CONFIGURE THE ARDUINO AND GET IT READY FOR FIRST RUN 
void setup() {
 Serial.begin(57600); 
 Serial.println("ChipperNut ShiftLight Project. V2."); 
 Serial.println("Prepare for awesome......"); 

//get stored variables 
getEEPROM();
check_first_run();
prev_animation = pixelanim;

  for (int thisReading = 0; thisReading < numReadings; thisReading++){
    rpmarray[thisReading] = 0;  
  }

timeoutCounter = timeoutValue;
buildarrays();

strip.begin(); 
strip.show(); // Initialize all pixels to 'off' 

//ROTARY ENCODER 
pinMode(rpmPin, INPUT);
pinMode(dimPin, INPUT_PULLUP);
pinMode(button_pin, INPUT_PULLUP); 
pinMode(ROTARY_PIN1, INPUT_PULLUP); 
pinMode(ROTARY_PIN2, INPUT_PULLUP); 

//senseoption   1 = Interrupt (pin 2), 2 = FreqMeasure (pin 8) 
switch (senseoption){
  case 1:
    attachInterrupt(0, sensorIsr, RISING); 
  break;
  case 2: 
    FreqMeasure.begin();
  break;  
}

//translate the stored 255 color variables into meaningful RGB colors 
color1 = load_color(c1); 
delay(10); 
color2 = load_color(c2); 
delay(10); 
color3 = load_color(c3); 
delay(10); 
flclr1 = load_color(c4); 
delay(10); 
flclr2 = load_color(c5); 
delay(10); 

        if (digitalRead(dimPin) == dimmerlogic){
          display.setBrightness(dimval); 
         }else{
          display.setBrightness(brightval);         
         }
display.setColon(false);         
SEG_WRITE[0] =  (SEG_A | SEG_F| SEG_E | SEG_D);           // [
      SEG_WRITE[1] =  (SEG_A | SEG_D);                    //=
      SEG_WRITE[2] =  (SEG_A | SEG_D);                    //=              
      SEG_WRITE[3] =  (SEG_A | SEG_D | SEG_B | SEG_C);    // ]
display.setSegments(SEG_WRITE);
delay(1000);
} 





                        /*************
                         * LOOP *
                        *************/
void loop() { 
switch (senseoption){
  case 1:
     rpm = long(60e7/cal)/(float)interval;
  break;

  case 2: 
    if (FreqMeasure.available()) {
    rpm = (600/cal)* FreqMeasure.countToFrequency(FreqMeasure.read());
    timeoutCounter = timeoutValue;
    }
  break;  
}

if (smoothing){
if (average != 0){
  if ((rpm-average > 2500) || (average-rpm > 2500)){                 
      if(DEBUG){
      Serial.print("FIXED!  ");
      Serial.println(rpm);  }      
        rpm = rpm_last;        
      }
}
  total = total - rpmarray[index]; 
  rpmarray[index] = rpm;
  total = total + rpmarray[index]; 
  index = index + 1;   
  if (index >= numReadings){              
      index = 0;    
   }                       
  average = total / numReadings;
 
  if(DEBUG){Serial.print("average: ");
  Serial.println(average);}

rpm = average;
       
}

if (timeoutCounter > 0){   
  if (rpm_last > 0 ){
      if(DEBUG){Serial.println(rpm); }
      if (rpm > 9999){ display.showNumberDec(rpm/10);
      }else{display.showNumberDec(rpm); }

    if (digitalRead(dimPin) != dimmer){
       dimmer = digitalRead(dimPin);
        color1 = load_color(c1); 
        color2 = load_color(c2); 
        color3 = load_color(c3); 
        flclr1 = load_color(c4); 
        flclr2 = load_color(c5);
        if (digitalRead(dimPin) == dimmerlogic){
          display.setBrightness(dimval); 
         }else{
          display.setBrightness(brightval);         
         }
    }
 }  else {
      display.setColon(false);         
      SEG_WRITE[0] =  (SEG_A | SEG_F| SEG_E | SEG_D);           // [
            SEG_WRITE[1] =  (SEG_A | SEG_D);                    //=
            SEG_WRITE[2] =  (SEG_A | SEG_D);                    //=              
            SEG_WRITE[3] =  (SEG_A | SEG_D | SEG_B | SEG_C);    // ]
      display.setSegments(SEG_WRITE);
    }
  rpm_last = rpm;             
} else {
  rpm = 0;
    display.setColon(false);         
    SEG_WRITE[0] =  (SEG_A | SEG_F| SEG_E | SEG_D);           // [
          SEG_WRITE[1] =  (SEG_A | SEG_D);                    //=
          SEG_WRITE[2] =  (SEG_A | SEG_D);                    //=              
          SEG_WRITE[3] =  (SEG_A | SEG_D | SEG_B | SEG_C);    // ]
    display.setSegments(SEG_WRITE);
  clearStrip();
  strip.show(); 
}
  
if (timeoutCounter > 0){ timeoutCounter--;}  

if (rpm < shift_rpm){
  int a; 
  for (a = 0; a<NUMPIXELS; a++){
    if (rpm>rpmtable[a][0]){
        switch (rpmtable[a][1]){
          case 1:
            strip.setPixelColor(a,color1);
          break;
  
          case 2:
            strip.setPixelColor(a,color2);  
          break;
  
          case 3:
             strip.setPixelColor(a,color3);  
          break;        
        }
    } else {
  strip.setPixelColor(a, strip.Color(0, 0, 0));    
    }
   strip.show();    
  }

} else {

  unsigned long currentMillis = millis();

    if(currentMillis - previousMillis > shiftinterval) {
        previousMillis = currentMillis;   
        flashbool = !flashbool; 

        if (flashbool == true)
         for(int i=0; i<NUMPIXELS; i++) { 
            strip.setPixelColor(i, flclr1); 
         }
        else
         for(int i=0; i<NUMPIXELS; i++) { 
             strip.setPixelColor(i, flclr2); 
          }
    strip.show();
      }
}
   

  //Poll the Button, if pushed, cue animation and enter menu subroutine 
  if (digitalRead(button_pin) == LOW){ 
    delay(250); 
    clearStrip(); 
  
  //Ascend strip 
  for (int i=0; i<(NUMPIXELS/2)+1; i++){ 
    strip.setPixelColor(i, strip.Color(0, 0, 25)); 
    strip.setPixelColor(NUMPIXELS-i, strip.Color(0, 0, 25)); 
    strip.show(); 
  delay(35); 
  } 
  // Descend Strip 
  for (int i=0; i<(NUMPIXELS/2)+1; i++){ 
    strip.setPixelColor(i, strip.Color(0, 0, 0)); 
    strip.setPixelColor(NUMPIXELS-i, strip.Color(0, 0, 0)); 
    strip.show(); 
    delay(35); 
  } 
  
  menuvar=1; 
  menu(); 
  } 
} 




void buildarrays(){
                   
int x;  //rpm increment
int y;  //starting point pixel address
int ya; // second starting point pixel address (for middle-out animation only)
int i;  //temporary for loop variable

if(DEBUG){
 Serial.println("PIXELANIM   ");
 Serial.println(pixelanim);
 Serial.println("  Start1 ");
 Serial.println(seg1_start);
 Serial.println("  End1 ");
 Serial.println(seg1_end);
 Serial.println("  Start2 ");
 Serial.println(seg2_start);
 Serial.println("  End2 ");
 Serial.println(seg2_end);
 Serial.println("Start3 ");
 Serial.println(seg3_start);
 Serial.println("  End3 ");
 Serial.println(seg3_end);
 Serial.println("  Activation RPM ");
 Serial.println(activation_rpm);
 Serial.println("  SHIFT RPM ");
 Serial.println(shift_rpm);
}

  switch(pixelanim){

    case 1:        
      y=0;
      x = ((shift_rpm - activation_rpm)/NUMPIXELS);
      for (i = 0; i<seg1_end+1; i++){
        rpmtable[i][0] = activation_rpm + (i*x);
        rpmtable[i][1] = 1;
      }
       for (i = seg1_end+1; i<seg2_end+1; i++){
        rpmtable[i][0] = activation_rpm + (i*x);
        rpmtable[i][1] = 2;
      }
      for (i = seg2_end+1; i<seg3_end+1; i++){
        rpmtable[i][0] = activation_rpm + (i*x);
        rpmtable[i][1] = 3;
      }
    break;


    case 2:
     if (((NUMPIXELS-1)%2)> 0){    
         x = ((shift_rpm - activation_rpm)/(NUMPIXELS/2));  //EVEN PIXELS
     }else{
       x = ((shift_rpm - activation_rpm)/((NUMPIXELS/2)+1));   //ODD PIXELS       
     }
     

    ya = 0;   // SEGMENT 1
     for (i = seg1_start; i<seg1_end+1; i++){      
        rpmtable[i][0] = activation_rpm + (ya*x);
        rpmtable[i][1] = 1;
        ya++;
      }

          
         if (((NUMPIXELS-1)%2)> 0){
          ya = 0;
           for (i = seg1_start-1; i>seg1_start-(seg1_end-seg1_start)-2; i--){
              rpmtable[i][0] = activation_rpm + (ya*x);
              rpmtable[i][1] = 1;
              ya++;
            } 
         } else {
          ya = 1;
           for (i = seg1_start-1; i>seg1_start-(seg1_end-seg1_start)-1; i--){
              rpmtable[i][0] = activation_rpm + (ya*x);
              rpmtable[i][1] = 1;
              ya++;     
            }
         }



      if (seg2_start == seg2_end){
        ya =  seg2_end - seg1_start;  //SEGMENT 2
      } else {
        ya =  seg2_start - seg1_start;  
      }
      
     for (i = seg2_start; i<seg2_end+1; i++){
        rpmtable[i][0] = activation_rpm + (ya*x);
        rpmtable[i][1] = 2;
        ya++;
      }
      
      if (seg2_start == seg2_end){
        ya =  seg2_end - seg1_start;  //SEGMENT 2
      } else {
        ya =  seg2_start - seg1_start;  
      }
      
         if (((NUMPIXELS-1)%2)> 0){
           for (i = seg1_start-(seg1_end-seg1_start)-2; i>seg1_start-(seg2_end-seg1_start)-2; i--){
              rpmtable[i][0] = activation_rpm + (ya*x);
              rpmtable[i][1] = 2;
              ya++;
            }
          } else {
            for (i = seg1_start-(seg1_end-seg1_start)-1; i>seg1_start-(seg2_end-seg1_start)-1; i--){
              rpmtable[i][0] = activation_rpm + (ya*x);
              rpmtable[i][1] = 2;
              ya++;
             }
           }
           
      if (seg3_start == seg3_end){
         ya =  seg3_end - seg1_start;    //SEGMENT 3
      } else {
         ya =  seg3_start - seg1_start;    //SEGMENT 3  
      }
      
      
     for (i = seg3_start; i<seg3_end+1; i++){
        rpmtable[i][0] = activation_rpm + (ya*x);
        rpmtable[i][1] = 3;
        ya++;
      }

      if (seg3_start == seg3_end){
         ya =  seg3_end - seg1_start;   
      } else {
         ya =  seg3_start - seg1_start;     
      }
      
      if (((NUMPIXELS-1)%2)> 0){
          for (i = seg1_start-(seg2_end-seg1_start)-2; i>seg1_start-(seg3_end-seg1_start)-2; i--){
             rpmtable[i][0] = activation_rpm + (ya*x);
             rpmtable[i][1] = 3;
             ya++;
           }
      } else {
          for (i = seg1_start-(seg2_end-seg1_start)-1; i>seg1_start-(seg3_end-seg1_start)-1; i--){
              rpmtable[i][0] = activation_rpm + (ya*x);
              rpmtable[i][1] = 3;
              ya++;
          }        
      }
    break;
    
  case 3:        
        y=0;
        x = ((shift_rpm - activation_rpm)/NUMPIXELS);
        for (i = NUMPIXELS-1; i>seg1_start-1; i--){
          rpmtable[i][0] = activation_rpm + (y*x);
          rpmtable[i][1] = 1;
          y++;
        }
         for (i = seg1_start-1; i>seg2_start-1; i--){
          rpmtable[i][0] = activation_rpm + (y*x);
          rpmtable[i][1] = 2;
          y++;
        }
        for (i = seg2_start-1; i>seg3_start-1; i--){
          rpmtable[i][0] = activation_rpm + (y*x);
          rpmtable[i][1] = 3;
          y++;
        }
      break;
  }

if(DEBUG){
  for (i = 0; i<NUMPIXELS; i++){
    Serial.print(rpmtable[i][0]);
    Serial.print("  ");
    Serial.print(rpmtable[i][1]);
    Serial.print("  ");
    Serial.println(rpmtable[i][2]);  
  }
}
}







/*************************
 * MENU SYSTEM
 *************************/

// MENU SYSTEM 
void menu(){ 
prev_menu = 2;
//this keeps us in the menu 
while (menuvar == 1){   
  
  // This little bit calls the rotary encoder   
  int result = rotary_process(); 
  if(DEBUG){if(result!=0){Serial.println(result);}}
  if (result == -128){rotaryval--;}   
  else if (result == 64){rotaryval++;} 
 
  rotaryval = constrain(rotaryval, 0, 17); 

//Poll the rotary encoder button to enter menu items
     if (digitalRead(button_pin) == LOW){ 
        delay(250); 
        menu_enter = 1; 
      } 
      
      if (prev_menu != rotaryval || menu_enter == 1 || menu_enter == 2){      
          prev_menu = rotaryval;
          if (menu_enter == 2){menu_enter = 0;}

         if (digitalRead(dimPin)== dimmerlogic){
            display.setBrightness(dimval);  
          } else {
            display.setBrightness(brightval);   
          }
      
        switch (rotaryval){ 
        
          case 0: //Menu Screen. Exiting saves variables to EEPROM 
            SEG_WRITE[0] =  (SEG_A | SEG_F| SEG_E | SEG_D);     // [
            SEG_WRITE[1] =  (SEG_A | SEG_D);                    //=
            SEG_WRITE[2] =  (SEG_A | SEG_D);                    //=              
            SEG_WRITE[3] =  (SEG_A | SEG_D | SEG_B | SEG_C);    // ]
            display.setSegments(SEG_WRITE);
            
            //Poll the Button to exit 
            if (menu_enter == 1){ 
              delay(250); 
              rotaryval = 0; 
              menuvar=0;
              menu_enter = 0;              
              writeEEPROM(); 
              getEEPROM(); 
              buildarrays();
              display.setColon(false);
              //Ascend strip 
              for (int i=0; i<(NUMPIXELS/2)+1; i++){ 
              strip.setPixelColor(i, strip.Color(0, 0, 25)); 
              strip.setPixelColor(NUMPIXELS-i, strip.Color(0, 0, 25)); 
              strip.show(); 
              delay(35); 
              } 
              // Descend Strip 
              for (int i=0; i<(NUMPIXELS/2)+1; i++){ 
              strip.setPixelColor(i, strip.Color(0, 0, 0)); 
              strip.setPixelColor(NUMPIXELS-i, strip.Color(0, 0, 0)); 
              strip.show(); 
              delay(35); 
              }   
            }     
          break; 
          
          
          case 1: //Adjust the global brightness 
            SEG_WRITE[0] =  (SEG_G);                                 // -
            SEG_WRITE[1] =  (SEG_F | SEG_E | SEG_D | SEG_C | SEG_G); //b
            SEG_WRITE[2] =  (SEG_E | SEG_G);                         //r              
            SEG_WRITE[3] =  (SEG_F | SEG_E | SEG_G | SEG_D);         //t      
            display.setSegments(SEG_WRITE);      
            while (menu_enter == 1){ 
            
            int bright = rotary_process(); 
            
            if (bright == -128){ 
              sb++;
              testbright = false;
            } 
            if (bright == 64){ 
              sb--;
              testbright = false;
            } 

            brightval  = map(sb,1,15,15,8);          
            brightval = constrain (brightval, 8, 15); 
            sb = constrain(sb, 1, 15); 
            Serial.println(brightval);
      
            if (prev_variable != sb){
                  prev_variable = sb;
                  color1 = load_color(c1); 
                  color2 = load_color(c2); 
                  color3 = load_color(c3); 
                  flclr1 = load_color(c4); 
                  flclr2 = load_color(c5); 
                  
                  display.setBrightness(brightval); 
                  display.showNumberDec(16-sb); 
                  if (testbright == false){
                  testlights(4);
                  testbright = true;
                  }
              }
      
      
           if (digitalRead(button_pin) == LOW){ 
              delay(250); 
              menu_enter = 2;
              prev_variable = 0;
              clearStrip(); 
              strip.show(); 
              for(int i=0; i<NUMPIXELS+1; i++) { 
                strip.setPixelColor(i, color1); 
                strip.show(); 
                delay(15); 
                strip.setPixelColor(i, strip.Color(0, 0, 0)); 
                strip.show(); 
              }         
            }      
            }     
          break; 
      
         //dimPin
          case 2: //Adjust the global brightness DIMMER (when Pin 9 == HIGH)
            SEG_WRITE[0] =  (SEG_G);                                 // -
            SEG_WRITE[1] =  (SEG_G | SEG_E | SEG_D | SEG_C | SEG_B);         //d   
            SEG_WRITE[2] =  (SEG_E | SEG_F);                                 //i              
            SEG_WRITE[3] =  (SEG_E | SEG_F | SEG_A | SEG_B | SEG_C);         //m    
            display.setSegments(SEG_WRITE);      
            while (menu_enter == 1){ 
            
            int bright = rotary_process(); 
            
            if (bright == -128){ 
              dimsb++;
              testbright = false;
            } 
            if (bright == 64){ 
              dimsb--;
              testbright = false;
            } 

            dimval  = map(dimsb,1,15,15,8);          
            dimval = constrain (dimval, 8, 15); 
            dimsb = constrain(dimsb, 1, 15); 
            Serial.println(dimval);
      
            if (prev_variable != dimsb){
                  prev_variable = dimsb;
                  testdim = true;
                  color1 = load_color(c1); 
                  color2 = load_color(c2); 
                  color3 = load_color(c3); 
                  flclr1 = load_color(c4); 
                  flclr2 = load_color(c5); 
                  
                  display.setBrightness(dimval); 
                  display.showNumberDec(16-dimsb); 
                  if (testbright == false){
                  testlights(4);
                  testbright = true;
                  }
              }
      
      
           if (digitalRead(button_pin) == LOW){ 
              delay(250); 
              menu_enter = 4;     
            }      
            }
 while (menu_enter == 4){ 
            
            int bright = rotary_process(); 
            
            if (bright == -128){ 
              dimmerlogic = false;
              testbright = false;
            } 
            if (bright == 64){ 
              dimmerlogic = true;
              testbright = false;
            } 
            Serial.println(dimmerlogic);
      
            if (prev_variable != dimmerlogic){
                  prev_variable = dimmerlogic;
                  
                if (dimmerlogic == false){
                    SEG_WRITE[0] =  (SEG_G);                                         // -    
                    SEG_WRITE[1] =  (SEG_G);                                         // -   
                    SEG_WRITE[2] =  (SEG_F | SEG_E | SEG_C | SEG_G | SEG_B);         //H 
                    SEG_WRITE[3] =  (SEG_E | SEG_F);                                 //i
                    display.setSegments(SEG_WRITE);  
               } else if (dimmerlogic == true){                   
                    SEG_WRITE[0] =  (SEG_G);                                         // -    
                    SEG_WRITE[1] =  (SEG_G);                                         // -
                    SEG_WRITE[2] =  (SEG_F | SEG_E | SEG_D);                          //L
                    SEG_WRITE[3] =  (SEG_G | SEG_E | SEG_D | SEG_C );                 //o
                    display.setSegments(SEG_WRITE);  
                }
          
                  if (testbright == false){
                  testlights(4);
                  testbright = true;
                  }
              }
      
      
           if (digitalRead(button_pin) == LOW){ 
              delay(250); 
              menu_enter = 2;
              prev_variable = 0;
              testdim = false;
              clearStrip(); 
              strip.show(); 
              for(int i=0; i<NUMPIXELS+1; i++) { 
                strip.setPixelColor(i, color1); 
                strip.show(); 
                delay(15); 
                strip.setPixelColor(i, strip.Color(0, 0, 0)); 
                strip.show(); 
              }         
            }      
            }
          break; 

          
          case 3: // ACTIVATION RPM 
            SEG_WRITE[0] =  (SEG_G);                                          // -
            SEG_WRITE[1] =  (SEG_F | SEG_E | SEG_A | SEG_C | SEG_G | SEG_B);  //A
            SEG_WRITE[2] =  (SEG_E | SEG_G | SEG_D);                          //C             
            SEG_WRITE[3] =  ( SEG_F | SEG_E | SEG_G | SEG_D);                 //t      
            display.setSegments(SEG_WRITE);  
      
            while (menu_enter == 1){      
              int coloradjust1 = rotary_process();           
              if (coloradjust1 == -128){activation_rpm=activation_rpm-10;} 
              if (coloradjust1 == 64){activation_rpm=activation_rpm+10;}         
              activation_rpm = constrain(activation_rpm, 0, 20000); 
              
              if (prev_variable != activation_rpm) {
                    prev_variable = activation_rpm;
                    if (activation_rpm<9999){        
                      display.showNumberDec(activation_rpm); 
                      strip.setPixelColor(0, strip.Color(0, 0, 0));
                      } else {
                      display.showNumberDec(activation_rpm-10000);
                      strip.setPixelColor(0, strip.Color(50, 50, 100)); 
                      }    
                      strip.show();
              }   
              if (digitalRead(button_pin) == LOW){ 
                delay(250);
                prev_variable = 0;
                menu_enter = 2; 
                clearStrip(); 
                strip.show(); 
                for(int i=0; i<NUMPIXELS+1; i++) { 
                  strip.setPixelColor(i, strip.Color(50, 50, 50)); 
                  strip.show(); 
                  delay(15); 
                  strip.setPixelColor(i, strip.Color(0, 0, 0)); 
                  strip.show(); 
                }        
              } 
            }   
          break; 
          
          
          case 4: // SHIFT RPM 
            SEG_WRITE[0] =  (SEG_A | SEG_F | SEG_G | SEG_C | SEG_D);    //S                                
            SEG_WRITE[1] =  (SEG_F | SEG_E | SEG_C | SEG_G | SEG_B);    //H
            SEG_WRITE[2] =  (SEG_E | SEG_G | SEG_F | SEG_A);             //F              
            SEG_WRITE[3] =  ( SEG_F | SEG_E | SEG_G | SEG_D);           //t
            display.setSegments(SEG_WRITE);  
            
            while (menu_enter == 1){ 
                    
              int coloradjust1 = rotary_process(); 
              
              if (coloradjust1 == -128){shift_rpm = shift_rpm-10;}           
              if (coloradjust1 == 64){shift_rpm = shift_rpm+10;}                   
              shift_rpm = constrain(shift_rpm, 0, 20000); 
      
              if (prev_variable != shift_rpm) {
                   prev_variable = shift_rpm;
                  if (shift_rpm<9999){        
                    display.showNumberDec(shift_rpm); 
                    strip.setPixelColor(0, strip.Color(0, 0, 0));
                    } else {
                    display.showNumberDec(shift_rpm-10000);
                    strip.setPixelColor(0, strip.Color(50, 50, 100)); 
                  }       
                  strip.show(); 
              }
              if (digitalRead(button_pin) == LOW){ 
                delay(250); 
                menu_enter = 2; 
                prev_variable = 0;
                clearStrip(); 
                strip.show(); 
                for(int i=0; i<NUMPIXELS+1; i++) { 
                  strip.setPixelColor(i, strip.Color(50, 50, 50)); 
                  strip.show(); 
                  delay(15); 
                  strip.setPixelColor(i, strip.Color(0, 0, 0)); 
                  strip.show(); 
                }        
              } 
            }    
          break;
      
      
           case 5:  //RPM SENSE MODE
            SEG_WRITE[0] =  (SEG_A | SEG_F | SEG_G | SEG_C | SEG_D);            //S       
            SEG_WRITE[1] =  (SEG_A | SEG_F | SEG_E | SEG_G | SEG_D);            //E   
            SEG_WRITE[2] =  (SEG_E | SEG_G | SEG_C);                            //n              
            SEG_WRITE[3] =  (SEG_A | SEG_F | SEG_G | SEG_C | SEG_D);            //S
            display.setSegments(SEG_WRITE);  
      
            
            while (menu_enter == 1){         
              int coloradjust1 = rotary_process();         
              if (coloradjust1 == -128){senseoption--;}           
              if (coloradjust1 == 64){senseoption++;}                   
              senseoption = constrain(senseoption, 1, 2); 
      
      
              if (prev_variable != senseoption) {
                   prev_variable = senseoption;
              switch (senseoption){
                case 1:
                    SEG_WRITE[0] =  (SEG_E | SEG_F);                                 //i      
                    SEG_WRITE[1] =  (SEG_E | SEG_G | SEG_C);                         //n 
                    SEG_WRITE[2] =  ( SEG_F | SEG_E | SEG_G | SEG_D);                //t   
                    SEG_WRITE[3] =  (SEG_E | SEG_G);                                 //r
                    display.setSegments(SEG_WRITE);  
                break;
      
                case 2:
      
                    SEG_WRITE[0] =  (SEG_G);                                         // -    
                    SEG_WRITE[1] =  (SEG_G);                                         // -
                    SEG_WRITE[2] =  (SEG_E | SEG_G | SEG_F | SEG_A);                  //F 
                    SEG_WRITE[3] =  (SEG_E | SEG_G);                                 //r
                    display.setSegments(SEG_WRITE);  
                break;   
              }
      
              }     
                 
              if (digitalRead(button_pin) == LOW){ 
                delay(250); 
                menu_enter = 2;
                prev_variable = 0;
                clearStrip(); 
                strip.show(); 
                for(int i=0; i<NUMPIXELS+1; i++) { 
                  strip.setPixelColor(i, strip.Color(50, 50, 50)); 
                  strip.show(); 
                  delay(15); 
                  strip.setPixelColor(i, strip.Color(0, 0, 0)); 
                  strip.show(); 
                }
      
                    
                    SEG_WRITE[0] =  (SEG_F | SEG_E | SEG_D | SEG_C | SEG_G);     //b
                    SEG_WRITE[1] =  (SEG_G | SEG_E | SEG_D | SEG_C );            //o
                    SEG_WRITE[2] =  (SEG_G | SEG_E | SEG_D | SEG_C );            //o
                    SEG_WRITE[3] =  (SEG_F | SEG_E | SEG_G | SEG_D);             //t
                    display.setSegments(SEG_WRITE);  
             delay(1000);
             writeEEPROM(); 
             resetFunc();
              } 
            }    
           break;
      
      
           case 6:  //SMOOTHING (conditioning)
            SEG_WRITE[0] =  (SEG_A | SEG_F | SEG_E | SEG_D);     //C 
            SEG_WRITE[1] =  (SEG_G | SEG_E | SEG_D | SEG_C );   //O
            SEG_WRITE[2] =  (SEG_E | SEG_G | SEG_C);         //n                
            SEG_WRITE[3] =  (SEG_G | SEG_E | SEG_D | SEG_C | SEG_B);  //d
            display.setSegments(SEG_WRITE);   
            while (menu_enter == 1){ 
              
              int coloradjust1 = rotary_process();         
              if (coloradjust1 == -128){smoothing--;}           
              if (coloradjust1 == 64){smoothing++;}                   
              smoothing = constrain(smoothing, 0, 1); 
      
          if (prev_variable != smoothing) {
                   prev_variable = smoothing;
            if (smoothing){
            SEG_WRITE[0] =  (SEG_G);                                      // -
            SEG_WRITE[1] =  (SEG_G);                                      // -
            SEG_WRITE[2] =  (SEG_G | SEG_E | SEG_D | SEG_C );             //o
            SEG_WRITE[3] =  (SEG_E | SEG_G | SEG_C);                      //n  
            display.setSegments(SEG_WRITE);  
              
      
              
            }else{
      
      
            SEG_WRITE[0] =  (SEG_G);                                      // -
            SEG_WRITE[1] =  (SEG_G | SEG_E | SEG_D | SEG_C );             //o
            SEG_WRITE[2] =  (SEG_E | SEG_G | SEG_F | SEG_A);              //F  
            SEG_WRITE[3] =  (SEG_E | SEG_G | SEG_F | SEG_A);              //F    
            display.setSegments(SEG_WRITE);  
             }
             
          }      
              if (digitalRead(button_pin) == LOW){ 
                delay(250); 
                menu_enter = 2; 
                prev_variable = 0;
                clearStrip(); 
                strip.show(); 
                for(int i=0; i<NUMPIXELS+1; i++) { 
                  strip.setPixelColor(i, strip.Color(50, 50, 50)); 
                  strip.show(); 
                  delay(15); 
                  strip.setPixelColor(i, strip.Color(0, 0, 0)); 
                  strip.show(); 
                }             
              } 
            }    
           break;
      
      
       case 7:  // PULSES PER REVOLUTION
            SEG_WRITE[0] =  (SEG_G);     // -
            SEG_WRITE[1] =  (SEG_A | SEG_F | SEG_E | SEG_G | SEG_B); //P
            SEG_WRITE[2] =  (SEG_A | SEG_F | SEG_E | SEG_G | SEG_B); //P               
            SEG_WRITE[3] =  (SEG_E | SEG_G);                         //r
            display.setSegments(SEG_WRITE);
            
            while (menu_enter == 1){               
              int coloradjust1 = rotary_process();         
              if (coloradjust1 == -128){cal--;}           
              if (coloradjust1 == 64){cal++;}                   
              cal = constrain(cal, 1, 255); 
      
              if (prev_cal != cal){
                display.setColon(true);
                display.showNumberDec(cal*10);                
                display.setBrightness(brightval/3);  
                prev_cal = cal;
           
                
                delay(500);     
              }
       
              switch (senseoption){
                case 1:
                   rpm = long(60e7/cal)/(float)interval;
                break;
              
                case 2: 
                  if (FreqMeasure.available()) {
                  rpm = (600/cal)* FreqMeasure.countToFrequency(FreqMeasure.read());
                  timeoutCounter = timeoutValue;
                  }
                break;  
              }

              Serial.println(rpm);
              display.setColon(false);
              if (rpm > 9999){ display.showNumberDec(rpm/10);
              }else{display.showNumberDec(rpm); }
              display.setBrightness(brightval);      
              
                 
             if (digitalRead(button_pin) == LOW){ 
                delay(250); 
                menu_enter = 2;
                display.setColon(false);
                clearStrip(); 
                strip.show(); 
                for(int i=0; i<NUMPIXELS+1; i++) { 
                  strip.setPixelColor(i, strip.Color(50, 50, 50)); 
                  strip.show(); 
                  delay(15); 
                  strip.setPixelColor(i, strip.Color(0, 0, 0)); 
                  strip.show(); 
                }
              } 
            }    
           break;
      
     
           case 8:  // NUMBER OF LEDS
            SEG_WRITE[0] =  (SEG_E | SEG_G | SEG_C);                // n
            SEG_WRITE[1] =  (SEG_F | SEG_E | SEG_D);                 //L
            SEG_WRITE[2] =  (SEG_A | SEG_F | SEG_E | SEG_G | SEG_D); //E               
            SEG_WRITE[3] =  (SEG_G | SEG_E | SEG_D | SEG_C | SEG_B); //d
            display.setSegments(SEG_WRITE);
            
            while (menu_enter == 1){ 
              
              int coloradjust1 = rotary_process();         
              if (coloradjust1 == -128){NUMPIXELS--;}           
              if (coloradjust1 == 64){NUMPIXELS++;}                   
              NUMPIXELS = constrain(NUMPIXELS, 0, 32); 
      
              if (prev_variable != NUMPIXELS) {
                   prev_variable = NUMPIXELS;
              display.showNumberDec(NUMPIXELS);       
      
              }
             if (digitalRead(button_pin) == LOW){ 
                delay(250); 
                menu_enter = 2;
                prev_variable = 0;
                clearStrip(); 
                strip.show(); 
                for(int i=0; i<NUMPIXELS+1; i++) { 
                  strip.setPixelColor(i, strip.Color(50, 50, 50)); 
                  strip.show(); 
                  delay(15); 
                  strip.setPixelColor(i, strip.Color(0, 0, 0)); 
                  strip.show(); 
                }
      
            SEG_WRITE[0] =  (SEG_F | SEG_E | SEG_D | SEG_C | SEG_G);      //b
            SEG_WRITE[1] =  (SEG_G | SEG_E | SEG_D | SEG_C );             //o
            SEG_WRITE[2] =  (SEG_G | SEG_E | SEG_D | SEG_C );             //o              
            SEG_WRITE[3] =  (SEG_F | SEG_E | SEG_G | SEG_D);              //t
            display.setSegments(SEG_WRITE);
             delay(1000);
             writeEEPROM(); 
             resetFunc();
              } 
            }    
           break;    
          
       
           case 9:  // Color Segmentation   
      
            SEG_WRITE[0] =  (SEG_G);// -
            SEG_WRITE[1] =  (SEG_A | SEG_F | SEG_G | SEG_C | SEG_D); //S
            SEG_WRITE[2] =  (SEG_A | SEG_F | SEG_E | SEG_G | SEG_D); //E               
            SEG_WRITE[3] =  (SEG_A | SEG_F | SEG_E | SEG_D | SEG_C); //G
            display.setSegments(SEG_WRITE);
                           
            
            if (menu_enter == 1){
                  color1 = load_color(c1); 
                  color2 = load_color(c2); 
                  color3 = load_color(c3); 
                  flclr1 = load_color(c4); 
                  flclr2 = load_color(c5); 
                build_segments();
                menu_enter = 2;
                current_seg_number = 1;
                seg_mover = 0;
                clearStrip(); 
                strip.show();
                clearStrip(); 
                buildarrays();
                for(int i=0; i<NUMPIXELS+1; i++) { 
                  strip.setPixelColor(i, color1); 
                  strip.show(); 
                  delay(15); 
                  strip.setPixelColor(i, strip.Color(0, 0, 0)); 
                  strip.show(); 
                }
             }
      break;    
      
      
           case 10:  // PIXEL ANIMATION MODE
            SEG_WRITE[0] =  (SEG_A | SEG_F | SEG_B | SEG_E | SEG_G | SEG_C); //A
            SEG_WRITE[1] =  (SEG_E | SEG_G | SEG_C);                         //n
            SEG_WRITE[2] =  (SEG_E | SEG_F);                                 //i               
            SEG_WRITE[3] =  (SEG_E | SEG_F | SEG_A | SEG_B | SEG_C);         //m
            display.setSegments(SEG_WRITE);
          
            while (menu_enter == 1){        
              int coloradjust1 = rotary_process();         
              if (coloradjust1 == -128){pixelanim--;} 
              if (coloradjust1 == 64){pixelanim++;}         
              pixelanim = constrain(pixelanim, 1, 3);        
              display.showNumberDec(pixelanim); 
      
                      
                if (prev_animation != pixelanim){
                 if(DEBUG){ Serial.println("Animation Change");}
                prev_animation = pixelanim;
                  color1 = load_color(c1); 
                  color2 = load_color(c2); 
                  color3 = load_color(c3); 
                  flclr1 = load_color(c4); 
                  flclr2 = load_color(c5); 
                
                if (pixelanim == 1){
                  for (int a = 0; a<NUMPIXELS; a++){
                    strip.setPixelColor(a,color1);
                    strip.show();
                    delay(50);
                  }
                }else if(pixelanim == 2){
                  for (int a = NUMPIXELS/2; a<NUMPIXELS; a++){
                    strip.setPixelColor(a,color1);
                    strip.setPixelColor(NUMPIXELS-a,color1);
                    strip.show();
                    delay(75);
                  } 
                } else if(pixelanim == 3) {
                  for (int a = NUMPIXELS; a>-1; a--){
                    strip.setPixelColor(a,color1);
                    strip.show();
                    delay(50);
                  }
                }
                clearStrip();
                }
      
              
              if (digitalRead(button_pin) == LOW){           
                delay(250);
                menu_enter = 2;
                clearStrip(); 
                strip.show(); 
      
              SEG_WRITE[0] =  (SEG_E | SEG_G);                                      //r
              SEG_WRITE[1] =  (SEG_A | SEG_F | SEG_E | SEG_G | SEG_D);              //E  
              SEG_WRITE[2] =  (SEG_G | SEG_E | SEG_D | SEG_C | SEG_B);              //d              
              SEG_WRITE[3] =  (SEG_G | SEG_E | SEG_D | SEG_C );                     //o
              display.setSegments(SEG_WRITE);
             delay(1000);
              SEG_WRITE[0] =  (SEG_G);                                              // -
              SEG_WRITE[1] =  (SEG_A | SEG_F | SEG_G | SEG_C | SEG_D);              //S 
              SEG_WRITE[2] =  (SEG_A | SEG_F | SEG_E | SEG_G | SEG_D);              //E            
              SEG_WRITE[3] =  (SEG_A | SEG_F | SEG_E | SEG_D | SEG_C);              //G
              display.setSegments(SEG_WRITE);
             delay(1000);
              SEG_WRITE[0] =  (SEG_G);                                              // -
              SEG_WRITE[1] =  (SEG_G);                                              // -
              SEG_WRITE[2] =  (SEG_G);                                              // -            
              SEG_WRITE[3] =  (SEG_G);                                              // -
              display.setSegments(SEG_WRITE);
             delay(500);
             build_segments();
                for(int i=0; i<NUMPIXELS+1; i++) { 
                  strip.setPixelColor(i, color1); 
                  strip.show(); 
                  delay(15); 
                  strip.setPixelColor(i, strip.Color(0, 0, 0)); 
                  strip.show(); 
                }
           }        
          } 
         break;  
      
              
          
          case 11: //Adjust Color #1 
            SEG_WRITE[0] =  (SEG_A | SEG_F | SEG_D | SEG_E);    //C
            SEG_WRITE[1] =  (SEG_E | SEG_F | SEG_D);            //L
            SEG_WRITE[2] =  (SEG_E | SEG_G);                    //r               
            SEG_WRITE[3] =  (SEG_B | SEG_C);                    //1
            display.setSegments(SEG_WRITE);
            
           while (menu_enter == 1){ 
              
              int coloradjust1 = rotary_process();         
              if (coloradjust1 == -128){c1--;} 
              if (coloradjust1 == 64){c1++;}         
              c1 = constrain(c1, 0, 255); 
      
              if (prev_color != c1){
                prev_color = c1;
                color1 = load_color(c1); 
                testlights(1);
              }
            
              if (digitalRead(button_pin) == LOW){ 
                delay(250); 
                prev_color = 0;
                menu_enter = 2; 
                clearStrip(); 
                strip.show(); 
                for(int i=0; i<NUMPIXELS+1; i++) { 
                  strip.setPixelColor(i, color1); 
                  strip.show(); 
                  delay(15); 
                  strip.setPixelColor(i, strip.Color(0, 0, 0)); 
                  strip.show(); 
                }        
              } 
            } 
          break; 
             
          
          
          case 12: //Adjust Color #2 
      
          SEG_WRITE[0] =  (SEG_A | SEG_F | SEG_D | SEG_E);              //C
            SEG_WRITE[1] =  (SEG_E | SEG_F | SEG_D);                    //L
            SEG_WRITE[2] =  (SEG_E | SEG_G);                            //r               
            SEG_WRITE[3] =  (SEG_A | SEG_B | SEG_G | SEG_E | SEG_D);    //2
            display.setSegments(SEG_WRITE);
                 
            while (menu_enter == 1){ 
            
              int coloradjust1 = rotary_process(); 
              
              if (coloradjust1 == -128){c2--;} 
              if (coloradjust1 == 64){c2++;}         
              c2 = constrain(c2, 0, 255); 
              
              if (prev_color != c2){
                prev_color = c2;
                color2 = load_color(c2); 
                testlights(2);
              }
              
              
              if (digitalRead(button_pin) == LOW){ 
                delay(250); 
                prev_color = 0;
                menu_enter = 2; 
                clearStrip(); 
                strip.show(); 
                for(int i=0; i<NUMPIXELS+1; i++) { 
                  strip.setPixelColor(i, color2); 
                  strip.show(); 
                  delay(15); 
                  strip.setPixelColor(i, strip.Color(0, 0, 0)); 
                  strip.show(); 
                }          
              } 
            }    
          break; 
          
          case 13: //Adjust Color #3 
            SEG_WRITE[0] =  (SEG_A | SEG_F | SEG_D | SEG_E);            //C
            SEG_WRITE[1] =  (SEG_E | SEG_F | SEG_D);                    //L
            SEG_WRITE[2] =  (SEG_E | SEG_G);                            //r               
            SEG_WRITE[3] =  (SEG_A | SEG_B | SEG_G | SEG_C | SEG_D);    //3
            display.setSegments(SEG_WRITE);
                  
            while (menu_enter == 1){       
              int coloradjust1 = rotary_process();         
              if (coloradjust1 == -128){c3--;} 
              if (coloradjust1 == 64){c3++;}        
              c3 = constrain(c3, 0, 255); 
              
               if (prev_color != c3){
                prev_color = c3;
                color3 = load_color(c3); 
                testlights(3);
              }
              
              
              if (digitalRead(button_pin) == LOW){ 
                delay(250); 
                prev_color = 0;
                menu_enter = 2; 
                clearStrip(); 
                strip.show(); 
                for(int i=0; i<NUMPIXELS+1; i++) { 
                  strip.setPixelColor(i, color3); 
                  strip.show(); 
                  delay(15); 
                  strip.setPixelColor(i, strip.Color(0, 0, 0)); 
                  strip.show(); 
                }       
              } 
            }    
          break; 
          
          case 14: //Adjust Color #4 
            SEG_WRITE[0] =  (SEG_A | SEG_F | SEG_G | SEG_C | SEG_D);            //S
            SEG_WRITE[1] =  (SEG_A | SEG_F | SEG_D | SEG_E);                    //C
            SEG_WRITE[2] =  (SEG_G);                                            //-               
            SEG_WRITE[3] =  (SEG_C | SEG_B);            //1
            display.setSegments(SEG_WRITE);
                  
            while (menu_enter == 1){       
              int coloradjust1 = rotary_process();         
              if (coloradjust1 == -128){c4--;} 
              if (coloradjust1 == 64){c4++;} 
              
              c4 = constrain(c4, 0, 255); 
              
              flclr1 = load_color(c4);        
              
              for(int i=0; i<NUMPIXELS+1; i++) { 
                strip.setPixelColor(i, flclr1); 
              } 
              
              strip.show(); 
                     
              if (digitalRead(button_pin) == LOW){ 
                delay(250); 
                menu_enter = 2; 
                clearStrip(); 
                strip.show(); 
                for(int i=0; i<NUMPIXELS+1; i++) { 
                  strip.setPixelColor(i, flclr1); 
                  strip.show(); 
                  delay(15); 
                  strip.setPixelColor(i, strip.Color(0, 0, 0)); 
                  strip.show(); 
                }        
              } 
            }    
          break; 
          
          case 15: //Adjust Color #5 
      
            SEG_WRITE[0] =  (SEG_A | SEG_F | SEG_G | SEG_C | SEG_D);            //S
            SEG_WRITE[1] =  (SEG_A | SEG_F | SEG_D | SEG_E);                    //C
            SEG_WRITE[2] =  (SEG_G);                                            //-               
            SEG_WRITE[3] =  (SEG_A | SEG_B | SEG_G | SEG_E | SEG_D);            //2
            display.setSegments(SEG_WRITE);
            
            while (menu_enter == 1){       
              int coloradjust1 = rotary_process();        
              if (coloradjust1 == -128){c5--;} 
              if (coloradjust1 == 64){c5++;}         
              c5 = constrain(c5, 0, 255); 
              
              flclr2 = load_color(c5);        
              
              for(int i=0; i<NUMPIXELS+1; i++) { 
                strip.setPixelColor(i, flclr2); 
              } 
              
              strip.show(); 
                      
              if (digitalRead(button_pin) == LOW){ 
                delay(250); 
                menu_enter = 2; 
                clearStrip(); 
                strip.show(); 
                for(int i=0; i<NUMPIXELS+1; i++) { 
                  strip.setPixelColor(i, flclr2); 
                  strip.show(); 
                  delay(15); 
                  strip.setPixelColor(i, strip.Color(0, 0, 0)); 
                  strip.show(); 
                }        
              } 
            }     
          break; 
      
          
          case 16:   //DEBUG MODE
          
            SEG_WRITE[0] =  (SEG_G | SEG_E | SEG_D | SEG_C | SEG_B);          //d
            SEG_WRITE[1] =  (SEG_A | SEG_F | SEG_E | SEG_G | SEG_D);          //E  
            SEG_WRITE[2] =  (SEG_F | SEG_E | SEG_D | SEG_C | SEG_G);          //b             
            SEG_WRITE[3] =  (SEG_A | SEG_F | SEG_B | SEG_G | SEG_C | SEG_D);  //g
            display.setSegments(SEG_WRITE);
              
       while (menu_enter == 1){       
              int coloradjust1 = rotary_process();        
              if (coloradjust1 == -128){DEBUG--;} 
              if (coloradjust1 == 64){DEBUG++;}         
              DEBUG = constrain(DEBUG, 0, 1); 
               
            if (DEBUG== 1){
            SEG_WRITE[0] =  (SEG_G);                                      // -
            SEG_WRITE[1] =  (SEG_G);                                      // -
            SEG_WRITE[2] =  (SEG_G | SEG_E | SEG_D | SEG_C );             //o
            SEG_WRITE[3] =  (SEG_E | SEG_G | SEG_C);                      //n  
            display.setSegments(SEG_WRITE);  
              
              
            }else{
            SEG_WRITE[0] =  (SEG_G);                                      // -
            SEG_WRITE[1] =  (SEG_G | SEG_E | SEG_D | SEG_C );             //o
            SEG_WRITE[2] =  (SEG_E | SEG_G | SEG_F | SEG_A);              //F  
            SEG_WRITE[3] =  (SEG_E | SEG_G | SEG_F | SEG_A);              //F    
            display.setSegments(SEG_WRITE); 
              
            }
      
      
              if (digitalRead(button_pin) == LOW){ 
                delay(250); 
                menu_enter = 2; 
                clearStrip(); 
                strip.show(); 
                for(int i=0; i<NUMPIXELS+1; i++) { 
                  strip.setPixelColor(i, flclr2); 
                  strip.show(); 
                  delay(15); 
                  strip.setPixelColor(i, strip.Color(0, 0, 0)); 
                  strip.show(); 
                }        
              } 
            }  
          break;
      
      
          case 17:   //RESET
      
            SEG_WRITE[0] =  (SEG_G);                                  //-
            SEG_WRITE[1] =  (SEG_E | SEG_G);                          //r 
            SEG_WRITE[2] =  (SEG_A | SEG_F | SEG_G | SEG_C | SEG_D);  //S              
            SEG_WRITE[3] =  (SEG_F | SEG_G | SEG_E | SEG_D);          //t
            display.setSegments(SEG_WRITE);
      
            
       while (menu_enter == 1){       
              int coloradjust1 = rotary_process();        
              if (coloradjust1 == -128){rst--;} 
              if (coloradjust1 == 64){rst++;}         
              rst = constrain(rst, 0, 1); 
              
            if (rst == 1){
      
            SEG_WRITE[0] =  (SEG_G);                                               //-
            SEG_WRITE[1] =  (SEG_F | SEG_B | SEG_G | SEG_C | SEG_D);              //Y
            SEG_WRITE[2] =  (SEG_A | SEG_F | SEG_E | SEG_G | SEG_D);              //E            
            SEG_WRITE[3] =  (SEG_A | SEG_F | SEG_G | SEG_C | SEG_D);              //S  
            display.setSegments(SEG_WRITE);
              
            }else{
            SEG_WRITE[0] =  (SEG_G);                                              //-
            SEG_WRITE[1] =  (SEG_G);                                              // -
            SEG_WRITE[2] =  (SEG_E | SEG_G | SEG_C);                              //n              
            SEG_WRITE[3] =  (SEG_G | SEG_E | SEG_D | SEG_C );                     //o
            display.setSegments(SEG_WRITE);
              
            }
      
                      
              if (digitalRead(button_pin) == LOW){ 
                delay(250); 
                menu_enter = 2; 
                clearStrip(); 
                strip.show(); 
                for(int i=0; i<NUMPIXELS+1; i++) { 
                  strip.setPixelColor(i, flclr2); 
                  strip.show(); 
                  delay(15); 
                  strip.setPixelColor(i, strip.Color(0, 0, 0)); 
                  strip.show();
                }
                  if (rst ==1){
                  SEG_WRITE[0] =  (SEG_G);                                              //-
                  SEG_WRITE[1] =  (SEG_F | SEG_E | SEG_D | SEG_C | SEG_G);              //b
                  SEG_WRITE[2] =  (SEG_F | SEG_B | SEG_G | SEG_C | SEG_D);              //Y              
                  SEG_WRITE[3] =  (SEG_A | SEG_F | SEG_E | SEG_G | SEG_D);              //E  
                  display.setSegments(SEG_WRITE);
                     delay(500);
      
                  SEG_WRITE[0] =  (SEG_G | SEG_E | SEG_D | SEG_C | SEG_B);              //d
                  SEG_WRITE[1] =  (SEG_F | SEG_E | SEG_A | SEG_C | SEG_G | SEG_B);      //A  
                  SEG_WRITE[2] =  (SEG_F | SEG_E | SEG_G | SEG_D);                      //t            
                  SEG_WRITE[3] =  (SEG_F | SEG_E | SEG_A | SEG_C | SEG_G | SEG_B);      //A  
                  display.setSegments(SEG_WRITE);
                     delay(500);
      
                     for (int i = 0; i < 512; i++){EEPROM.write(i, 0);}
                     resetFunc();
                  
                  }       
              } 
            }  
          break;
                
          }
      } 
  } 
} 






/*************************
 * SUBROUTINES
 *************************/


//This subroutine reads the stored variables from memory 
void getEEPROM(){ 
brightval = EEPROM.read(0); 
sb = EEPROM.read(1); 
c1 = EEPROM.read(2); 
c2 = EEPROM.read(3); 
c3 = EEPROM.read(4); 
c4 = EEPROM.read(5); 
c5 = EEPROM.read(6); 
activation_rpm = EEPROM.read(7); 
pixelanim  = EEPROM.read(8); 
senseoption  = EEPROM.read(9); 
smoothing = EEPROM.read(10); 
NUMPIXELS = EEPROM.read(11); 
rpmscaler = EEPROM.read(12); 
shift_rpm1 = EEPROM.read(13); 
shift_rpm2 = EEPROM.read(14); 
shift_rpm3 = EEPROM.read(15); 
shift_rpm4 = EEPROM.read(16); 
DEBUG = EEPROM.read(17); 
seg1_start = EEPROM.read(18); 
seg1_end = EEPROM.read(19); 
seg2_start = EEPROM.read(20); 
seg2_end = EEPROM.read(21); 
seg3_start = EEPROM.read(22); 
seg3_end = EEPROM.read(23); 
activation_rpm1 = EEPROM.read(24); 
activation_rpm2 = EEPROM.read(25); 
activation_rpm3 = EEPROM.read(26); 
activation_rpm4 = EEPROM.read(27); 
cal = EEPROM.read(28);
dimval = EEPROM.read(29);
dimsb = EEPROM.read(30);
dimmerlogic = EEPROM.read(31);

activation_rpm = ((activation_rpm1 << 0) & 0xFF) + ((activation_rpm2 << 8) & 0xFFFF) + ((activation_rpm3 << 16) & 0xFFFFFF) + ((activation_rpm4 << 24) & 0xFFFFFFFF);
shift_rpm = ((shift_rpm1 << 0) & 0xFF) + ((shift_rpm2 << 8) & 0xFFFF) + ((shift_rpm3 << 16) & 0xFFFFFF) + ((shift_rpm4 << 24) & 0xFFFFFFFF);

buildarrays();

//seg1_start = 3;
//seg2_start = 5; 
//seg3_start = 9;

} 


//This subroutine writes the stored variables to memory 
void writeEEPROM(){ 

byte four = (shift_rpm & 0xFF);
byte three = ((shift_rpm >> 8) & 0xFF);
byte two = ((shift_rpm >> 16) & 0xFF);
byte one = ((shift_rpm >> 24) & 0xFF);

byte activation_four = (activation_rpm & 0xFF);
byte activation_three = ((activation_rpm >> 8) & 0xFF);
byte activation_two = ((activation_rpm >> 16) & 0xFF);
byte activation_one = ((activation_rpm >> 24) & 0xFF);

EEPROM.write(0, brightval); 
EEPROM.write(1, sb); 
EEPROM.write(2, c1); 
EEPROM.write(3, c2); 
EEPROM.write(4, c3); 
EEPROM.write(5, c4); 
EEPROM.write(6, c5); 
EEPROM.write(7, activation_rpm); 
EEPROM.write(8, pixelanim); 
EEPROM.write(9, senseoption); 
EEPROM.write(10, smoothing); 
EEPROM.write(11, NUMPIXELS); 
EEPROM.write(12, rpmscaler); 
EEPROM.write(13, four); 
EEPROM.write(14, three); 
EEPROM.write(15, two); 
EEPROM.write(16, one); 
EEPROM.write(17, DEBUG); 
EEPROM.write(18, seg1_start); 
EEPROM.write(19, seg1_end); 
EEPROM.write(20, seg2_start); 
EEPROM.write(21, seg2_end); 
EEPROM.write(22, seg3_start); 
EEPROM.write(23, seg3_end); 
EEPROM.write(24, activation_four); 
EEPROM.write(25, activation_three); 
EEPROM.write(26, activation_two); 
EEPROM.write(27, activation_one); 
EEPROM.write(28, cal);
EEPROM.write(29, dimval);
EEPROM.write(30, dimsb);
EEPROM.write(31, dimmerlogic);
} 





//This sub clears the strip to all OFF 
void clearStrip() { 
  for( int i = 0; i<strip.numPixels(); i++){ 
    strip.setPixelColor(i, strip.Color(0, 0, 0)); 
    strip.show(); 
  } 
} 



//Helper Color Manager - This translates our 255 value into a meaningful color
uint32_t load_color(int cx){ 
unsigned int r,g,b; 
if (cx == 0){ 
    r = 0; 
    g = 0; 
    b = 0; 
} 

if (cx>0 && cx<=85){ 
  r = 255-(cx*3); 
  g = cx*3; 
  b=0; 
} 

if (cx>85 && cx < 170){ 
  r = 0; 
  g = 255 - ((cx-85)*3); 
  b = (cx-85)*3; 
} 

if (cx >= 170 && cx<255){ 
  r = (cx-170)*3; 
  g = 0; 
  b = 255 - ((cx-170)*3); 
} 

if (cx == 255){ 
  r=255; 
  g=255; 
  b=255; 
} 



if (digitalRead(dimPin)== dimmerlogic || testdim == true){
  r = (r/dimsb); 
  g = (g/dimsb); 
  b = (b/dimsb);   
} else {
  r = (r/sb); 
  g = (g/sb); 
  b = (b/sb); 
}
     


return strip.Color(r,g,b); 
}


void testlights(int color){
  for (int a = 0; a<NUMPIXELS; a++){    
    if (color <4){
        if (rpmtable[a][1]==color){
           switch (color){
            case 1:
              strip.setPixelColor(a,color1);
            break;
    
            case 2:
              strip.setPixelColor(a,color2);  
            break;
    
            case 3:
               strip.setPixelColor(a,color3);  
            break;        
          }
        } else {
           strip.setPixelColor(a, strip.Color(0, 0, 0));    
        }
    } else {
      switch (rpmtable[a][1]){
          case 1:
            strip.setPixelColor(a,color1);
          break;
  
          case 2:
            strip.setPixelColor(a,color2);  
          break;
  
          case 3:
             strip.setPixelColor(a,color3);  
          break; 
    }
      
  }
  strip.show(); 
}
}


void check_first_run(){
  if (shift_rpm == 0){
     Serial.println("FIRST RUN! LOADING DEFAULTS");  
      brightval = 15; 
      dimval = 8;
      dimsb = 15;
      dimmerlogic = false;
      sb = 3; 
      c1 = 79; 
      c2 = 48; 
      c3 = 1; 
      c4 = 255; 
      c5 = 0; 
      
      activation_rpm = 1000; 
      shift_rpm = 6000;
      pixelanim  = 1; 
      senseoption  = 2;
      smoothing = 1; 
      NUMPIXELS = 8;
      //rpmscaler = EEPROM.read(12);  
      DEBUG = 0; 
      seg1_start = 0; 
      seg1_end = 3; 
      seg2_start = 0; 
      seg2_end = 5; 
      seg3_start = 0; 
      seg3_end = 7;
      cal = 10;
      writeEEPROM();
      resetFunc();
  }  
}



void build_segments(){

if (pixelanim == 3){seg_mover = NUMPIXELS-1;}
  
while (current_seg_number<4){
      display.showNumberDec(current_seg_number);

      int coloradjust1 = rotary_process();         
        if (coloradjust1 == -128){seg_mover--;} 
        if (coloradjust1 == 64){seg_mover++;}    
  
      if (digitalRead(button_pin) == LOW){ 
          delay(250);
          current_seg_number++;     
        } 
              
  switch(current_seg_number){
      case 1:        
         if (pixelanim == 1){  
             seg_mover = constrain(seg_mover, 0, (NUMPIXELS-1));
             seg1_end = seg_mover;     
             strip.setPixelColor(seg1_end, color1); 
             for (int x = seg1_end+1; x<NUMPIXELS; x++){
               strip.setPixelColor(x, strip.Color(0, 0, 0));
             } 
         } else if (pixelanim == 3){  
             seg_mover = constrain(seg_mover, 0, (NUMPIXELS-1));
             seg1_start = seg_mover;     
             strip.setPixelColor(seg1_start, color1); 
             for (int x = seg1_start-1; x>-1; x--){
               strip.setPixelColor(x, strip.Color(0, 0, 0));
             }
         } else if (pixelanim == 2) {
             seg1_start = ((NUMPIXELS-1)/2);
              if (((NUMPIXELS-1)%2)> 0){seg1_start=seg1_start+1;}
             seg_mover = constrain(seg_mover, seg1_start, (NUMPIXELS-1));
             seg1_end = seg_mover; 

              for (int x = seg1_start; x<seg1_end+1; x++){
                  strip.setPixelColor(x, color1);
                }
              for (int x = seg1_end+1; x<(NUMPIXELS); x++){
                  strip.setPixelColor(x, strip.Color(0, 0, 0));
                }

              if (((NUMPIXELS-1)%2)> 0){
                  for (int x = seg1_start-1; x>seg1_start-(seg1_end-seg1_start)-2; x--){
                     strip.setPixelColor(x, color1);
                  }
                  for (int x = seg1_start-(seg1_end-seg1_start)-2; x>-1; x--){
                     strip.setPixelColor(x, strip.Color(0, 0, 0));
                  }
                if(DEBUG){
                Serial.println("MoDULo");
                }
              } else {
                  for (int x = seg1_start; x>seg1_start-(seg1_end-seg1_start)-1; x--){
                     strip.setPixelColor(x, color1);
                  }
                  for (int x = seg1_start-(seg1_end-seg1_start)-1; x>-1; x--){
                     strip.setPixelColor(x, strip.Color(0, 0, 0));
                  }         
              }
         }
       if(DEBUG){
           Serial.print("S1end: ");
           Serial.println(seg1_end);
           Serial.print("S1start: ");
           Serial.println(seg1_start);
       }
         
         strip.show();
 
     
      break;



      case 2:           
         if (pixelanim == 1){       
             seg_mover = constrain(seg_mover, seg1_end+1, (NUMPIXELS-1));
             seg2_end = seg_mover;
             strip.setPixelColor(seg2_end, color2); 
             for (int x = seg2_end+1; x<strip.numPixels(); x++){
               strip.setPixelColor(x, strip.Color(0, 0, 0));
             }
         } else if (pixelanim == 3){  
             seg_mover = constrain(seg_mover, 0, (seg1_start-1));
             seg2_start = seg_mover;     
             strip.setPixelColor(seg2_start, color2); 
             for (int x = seg2_start-1; x>-1; x--){
               strip.setPixelColor(x, strip.Color(0, 0, 0));
             }
         } else if (pixelanim == 2) {
             //seg1_start = ((NUMPIXELS-1)/2);
             seg2_start = seg1_end + 1;
            //  if (((NUMPIXELS-1)%2)> 0){seg1_start=seg1_start+1;}
             seg_mover = constrain(seg_mover, seg2_start, (NUMPIXELS-1));
             seg2_end = seg_mover; 

              for (int x = seg2_start; x<seg2_end+1; x++){
                  strip.setPixelColor(x, color2);
                }
              for (int x = seg2_end+1; x<(NUMPIXELS); x++){
                  strip.setPixelColor(x, strip.Color(0, 0, 0));
                }

              if (((NUMPIXELS-1)%2)> 0){
                  for (int x = seg1_start-(seg1_end-seg1_start)-2; x>seg1_start-(seg2_end-seg1_start)-2; x--){
                     strip.setPixelColor(x, color2);
                  }
                  for (int x = seg1_start-(seg2_end-seg1_start)-2; x>-1; x--){
                     strip.setPixelColor(x, strip.Color(0, 0, 0));
                  }
                  if(DEBUG){Serial.println("MoDULo");
                  }
              } else {
                  for (int x = seg1_start-(seg1_end-seg1_start)-1; x>seg1_start-(seg2_end-seg1_start)-1; x--){
                     strip.setPixelColor(x, color2);
                  }
                  for (int x = seg1_start-(seg2_end-seg1_start)-1; x>-1; x--){
                     strip.setPixelColor(x, strip.Color(0, 0, 0));
                  }         
              }
              
         }

          if(DEBUG){
           Serial.print("S2end: ");
           Serial.println(seg2_end);
           Serial.print("S2start: ");
           Serial.println(seg2_start);
          }

      strip.show();   
      break;

      case 3:
      
         if (pixelanim == 1){        
             seg_mover = constrain(seg_mover, seg2_end+1, (strip.numPixels()-1));
             seg3_end = seg_mover;
            // seg3_start = seg2_end +1;
             strip.setPixelColor(seg3_end, color3); 
             for (int x = seg3_end+1; x<strip.numPixels(); x++){
               strip.setPixelColor(x, strip.Color(0, 0, 0));
             }
         } else if (pixelanim == 3){  
             seg_mover = constrain(seg_mover, 0, (seg2_start-1));
             seg3_start = seg_mover;     
             strip.setPixelColor(seg3_start, color3); 
             for (int x = seg3_start-1; x>-1; x--){
               strip.setPixelColor(x, strip.Color(0, 0, 0));
             }
         } else if (pixelanim == 2) {             
             seg3_start = seg2_end + 1;            
             seg_mover = constrain(seg_mover, seg3_start, (NUMPIXELS-1));
             seg3_end = seg_mover; 

              for (int x = seg3_start; x<seg3_end+1; x++){
                  strip.setPixelColor(x, color3);
                }
              for (int x = seg3_end+1; x<(NUMPIXELS); x++){
                  strip.setPixelColor(x, strip.Color(0, 0, 0));
                }

              if (((NUMPIXELS-1)%2)> 0){
                  for (int x = seg1_start-(seg2_end-seg1_start)-2; x>seg1_start-(seg3_end-seg1_start)-2; x--){
                     strip.setPixelColor(x, color3);
                  }
                  for (int x = seg1_start-(seg3_end-seg1_start)-2; x>-1; x--){
                     strip.setPixelColor(x, strip.Color(0, 0, 0));
                  }
                if(DEBUG){
                Serial.println("MoDULo");
                }
              } else {
                  for (int x = seg1_start-(seg2_end-seg1_start)-1; x>seg1_start-(seg3_end-seg1_start)-1; x--){
                     strip.setPixelColor(x, color3);
                  }
                  for (int x = seg1_start-(seg3_end-seg1_start)-1; x>-1; x--){
                     strip.setPixelColor(x, strip.Color(0, 0, 0));
                  }         
              }          
         }

          if(DEBUG){
           Serial.print("S3end: ");
           Serial.println(seg3_end);
           Serial.print("S3start: ");
           Serial.println(seg3_start);
          }
      strip.show();
      break;




   }
  }
}



void sensorIsr() 
{ 
  unsigned long now = micros(); 
  interval = now - lastPulseTime; 
  lastPulseTime = now; 
  timeoutCounter = timeoutValue; 
} 



