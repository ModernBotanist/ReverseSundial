#include <DS1302.h>
#include <Adafruit_NeoPixel.h>
#include "math.h"

const int kCePin   = 8;  // Chip Enable
const int kIoPin   = 7;  // Input/Output
const int kSclkPin = 9;  // Serial Clock

DS1302 rtc(kCePin, kIoPin, kSclkPin);

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel ledWheels = Adafruit_NeoPixel(48, 4, NEO_GRB + NEO_KHZ800);



double az, ele;
boolean rising;

//lat and lon for Madison, WI
const float lat = 43.06;
const float lon = -89.40;

float radLat = (lat*M_PI)/180.0;

//This is used to calc the day of the year
//zero is a place holder to avoid a bunch of '+1's
const int daysPerMonth[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30};

//make 'em global for debug stuff
/*int second = -1;
int minute = 32;
byte hour = 16;
byte month = 1;
byte day = 11;*/


void setup() {
  
  
  //Serial.begin(115200);
  ledWheels.begin();
  ledWheels.show();
  
}

void loop() {
  
  //for debug to run specific time frames
  /*if (second < 59) {
    second ++;
  } else if (second == 59 && minute == 32) {
    minute = 53;
    second = 0;
  } else {
    return;
  }*/
  
 /* while (Serial.available() > 0) {

    // look for the next valid integer in the incoming serial stream:
    hour = Serial.parseInt();
    // do it again:
    minute = Serial.parseInt();*/
  
  
  solveLocation();
  
  
/*
  Serial.print("solar elevation = ");    
  Serial.print(ele);
  //printf("%02d", ele);
  Serial.print("\t");
  Serial.print("Azimuth = ");
  Serial.println(az);*/
      
  writePixels();

  
  /*
  Time t = rtc.time();
  Serial.print(t.yr);
  Serial.print("\t");
  Serial.print(t.mon);
  Serial.print("\t");
  Serial.print(t.date);
  Serial.print("\t");
  Serial.print(t.min);
  Serial.print("\t");
  Serial.println(t.sec); */
 
  delay(10000);
  
  

}


void writePixels() {
  
  //first, find the color coefficients depending on alt of sun
  //establish variables for color value mapping
  //full LED brightness is very bright, so adjust color/brightness
  //depending on time of day
  byte RED = 0;
  byte GREEN = 0;
  byte BLUE = 0;
  
  if (ele > 7) { //7 degrees above horizon, highest colors
    RED = 70;
    GREEN = 80;
    BLUE = 80;
  } else if (ele < 0) { //below horizon
    RED = 6;
    GREEN = 0;
    BLUE = 0;
  } else { //dusk or dawn
    RED = 40;
    GREEN = 25;
    BLUE = 20;
  }
  
  //Now let's deal with the Azimuth display
  
  // 360/24 = 15 degrees per LED
  // 256/15 = 17.06 power levels per degree
  
  
  byte azprimLED = int(az)/15; //the "primary" azimuth LED
  byte azsecLED;
  
  if (azprimLED == 23) {
    azsecLED = 0;
  } else {
    azsecLED = azprimLED + 1;
  }
  
  //the proportional the first LED is off the true mark
  byte azsecLEDval = az - (15*(azprimLED));  //from 0-14
  byte azprimLEDval = 15 - azsecLEDval; //from 1-15
  
  //Serial.println(azprimLEDval);
  
  //adjust to 0-255
  azprimLEDval = byte(azprimLEDval * 17.06);
  azsecLEDval = byte(azsecLEDval * 17.06);
  
  //Serial.print(azprimLEDval);
  //Serial.println(azsecLEDval);
  
  //and then map each value to it's respective color
  byte azprimRED   = map(azprimLEDval, 0, 255, 0, RED);
  byte azprimGREEN = map(azprimLEDval, 0, 255, 0, GREEN);
  byte azprimBLUE  = map(azprimLEDval, 0, 255, 0, BLUE);
  byte azsecRED    = map(azsecLEDval, 0, 255, 0, RED);
  byte azsecGREEN  = map(azsecLEDval, 0, 255, 0, GREEN);
  byte azsecBLUE   = map(azsecLEDval, 0, 255, 0, BLUE);
  
  //remove prior settings
  setZeroLEDs();
  
  //finally, set the az LEDs, but do not write
  ledWheels.setPixelColor(azprimLED, ledWheels.Color(azprimRED, azprimGREEN, azprimBLUE));
  ledWheels.setPixelColor(azsecLED, ledWheels.Color(azsecRED, azsecGREEN, azsecBLUE));
  
  
  //and now we're onto the elevation LEDs. This one is tricker.
  //The az uses LED 0 as azimuth of 0
  //Here, everything will be offset 90 degrees to represent the horizon
  //LED 30 = Due "horizon east"
  //LED 42 = due "horizon west" (both zero indexed, after first wheel) 
  
  //the sun gets a max of 66.5 deg above/below horizon
  //twice a year here. If you live at a
  //higher latitude, you might want to scale the real elevation
  //to make better use of the whole LED wheel
  
  byte elprimLED;
  byte elsecLED;
  byte elprimLEDval;
  byte elsecLEDval;
  
  //Both LEDs will flip sides at the same time. 
  if (rising) { // the sun is climbing, use the right (eastern) side
    
    elprimLED = 31 - (ele/15); //negative for counterclockwise   
    elprimLEDval = abs(abs(ele) - abs(15*(elprimLED-31)));  //from 0-14

  } else {
    
    elprimLED = (ele/15) + 43; //west side
    elprimLEDval = abs(abs(ele) - abs(15*(elprimLED-43)));  //from 0-14
  } 
  
  elsecLED = elprimLED - 1;
  elsecLEDval = 15 - elprimLEDval; //from 1-15

 // Serial.println(rising);

  
  
  //adjust to 0-255
  elprimLEDval = byte(elprimLEDval * 17.06);
  elsecLEDval = byte(elsecLEDval * 17.06);
  //and then map each value to it's respective color
  byte elprimRED   = map(elprimLEDval, 0, 255, 0, RED);
  byte elprimGREEN = map(elprimLEDval, 0, 255, 0, GREEN);
  byte elprimBLUE  = map(elprimLEDval, 0, 255, 0, BLUE);
  byte elsecRED    = map(elsecLEDval, 0, 255, 0, RED);
  byte elsecGREEN  = map(elsecLEDval, 0, 255, 0, GREEN);
  byte elsecBLUE   = map(elsecLEDval, 0, 255, 0, BLUE);
  
  
  //finally, set the  ele LEDs, but do not write
  ledWheels.setPixelColor(elprimLED, ledWheels.Color(elprimRED, elprimGREEN, elprimBLUE));
  ledWheels.setPixelColor(elsecLED, ledWheels.Color(elsecRED, elsecGREEN, elsecBLUE));
  
  //write LED data to wheels
  ledWheels.show();
  
}
  

void setZeroLEDs() {
  //this sets all LEDs to off, but does not send data to wheels
  //the LEDs that need to be light are rewitten before sending
  //to wheels
  for(byte i=0; i<ledWheels.numPixels(); i++) {
      ledWheels.setPixelColor(i, ledWheels.Color(0,0,0));
      //ledWheels.show();
  }
}
 
void solveLocation() {
  
  
  
  Time t = rtc.time();
  byte month = t.mon;
  byte day = t.date;
  byte hour = t.hr;
  byte minute = t.min;
  byte second = t.sec;
  
  byte pastHour = hour;
  byte pastMin = minute;
  byte pastSec = second-1; //this is the Past part
  byte pastDay = day;
  byte pastMonth = month;
 
 //this below is all to deal with subtracting 1 second from the current time.

  if (second == 0) {
    pastSec = 59;
    pastMin = minute - 1;
    if (minute == 0) {
      pastMin = 59;
      pastHour = hour - 1;
      if (hour == 0) {
        pastHour = 23;
        pastDay = day - 1;
        if (day == 1) {
          pastDay = daysPerMonth[month-1];
          pastMonth = month - 1;
          if (month == 1) { //this case comes up for one second per year.
            pastMonth = 12;
            pastDay = 31;
          }}}}}

  //find time in fractional years (in radians)

  //First, fractional hours
  float fracHour = hour + (minute/60.0) + (second/3600.0);
  float pastFracHour = pastHour + (pastMin/60.0) + (pastSec/3600.0);

  //Then frac year
  float radYear = FractionalYear(dayOfYear(month, day), fracHour);
  float pastRadYear = FractionalYear(dayOfYear(pastMonth, pastDay), pastFracHour);

  //Now we start doing stuff:
  //First, how far north/south of the local east/west line
  //or the celesital equator
  
  float radDec = SolarDeclination(radYear);
  float pastRadDec = SolarDeclination(pastRadYear);
  
  //and another thing that's important
  //but little underlying understanding
  
  float angleCorRad = angleCorrection(radYear);
  float pastAngleCorRad = angleCorrection(pastRadYear);
  
  //not sure why, but this needs to be negative
  ele = SolarElevation(fracHour, angleCorRad, radDec);
  float pastEle = SolarElevation(pastFracHour, pastAngleCorRad, pastRadDec);
  
  SolveAzimuth(radDec);
 // Serial.println(az);
  
  
  //this here is to adjust for the afternoon
  //acos is used to solve for the az, which asumtotes at 180*
  //so, to get any western values, this is needed,
  //the bummer is that the sun does not always reach zenith due south
  //so for most days, this correction will create a several
  //degree error (~5) at noon and at midnight.
  //this is not due to rounding errors,
  //but essentially the wacky pattern between a non-circular
  //orbit and the axial tilt. See wikipeida "Analemma"
  //we also want to keep track of rising/setting direction
  //to know how to display
  rising = true;
  
  if (pastEle > ele) { //sun is dropping
    rising = false;
    az = (180-az)+180; //adjust so it's in the west 
  }

}


int dayOfYear(int M, int D) {
 //find the day of the year given a month and day
 
 int days = 0;
 
 for(int i = 0; i < M; i++) {
   days += daysPerMonth[i];
 }
 
 days += D;
 
 return days;
 
 }


float FractionalYear(int D, float H) {
  //find the fractional year in radians
  //from day of year and fractional hour
  float g = (360.0/365.25) * (D + (H/24));
  float radg = (g*M_PI)/180.0;
  
  return radg;
}
  
  
float SolarDeclination(float Ry) {
  //how far north/south of the local east/west line
  //or the celesital equator
  //I don't totally (or at all) understand
  
  float d = (0.396372-(22.91327*cos(Ry))+
        (4.02543*sin(Ry))-(0.387205*cos(2.0*Ry))+
        (0.051967*sin(2.0*Ry))-(0.154527*cos(3.0*Ry))+
        (0.084798*sin(3.0*Ry)));
    
  return d;
  
}

float angleCorrection(float Ry) {
  //don't know what's going on here. Something spacey.
  
    float tc = (0.004297+(0.107029*cos(Ry))-
    (1.837877*sin(Ry))-(0.837378*cos(2.0*Ry))-
    (2.340475*sin(2.0*Ry)));

    return tc;
   
}

float SolarElevation(float hr, float tc, float D) {
  
    float SHA = (hr-12.0)*15.0 + lon + tc;
    
    //to radians
    SHA = (SHA*M_PI)/180.0;
    
    //Serial.print(SHA);
    
    if (SHA > M_PI) {
        SHA -= (2*M_PI);
    }
    
    if (SHA < -M_PI){
        SHA += (2*M_PI);
    }
    
    //Serial.print(SHA);
    
    //to radians
    D = (D*M_PI)/180.0;
    
    float SZA = (sin(radLat) * sin(D)) + (cos(radLat)*
                cos(D) * cos(SHA));
    
    if (SZA > 1.0) {
        SZA = 1.0;
    }
    
    if (SZA < -1.0) {
        SZA = -1.0;
    }
        
    SZA = acos(SZA);
    //Serial.print(SZA);
    
    //to degrees - 90
    float SZAdeg = ((SZA*180.0)/M_PI)-90.0;
    
    return -SZAdeg;
 
}


void SolveAzimuth(float D) {
    
    //to radians
    D = (D*M_PI)/180.0;
    //Serial.print(D);
    
    //to radians
    float SZA = ((90.0-ele)*M_PI)/180.0;
    //Serial.println(SZA);
   
    float AzCos = (sin(D)-(sin(radLat)*cos(SZA)))/(cos(radLat)*sin(SZA));
    //Serial.print(AzCos);
    //to degrees
    
    AzCos = constrain(AzCos, -1, 1);
    
    az = (acos(AzCos)*180.0)/M_PI;
    //Serial.print(az);
    
}
