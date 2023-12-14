#include <Arduino.h>

#include <ESP8266WiFi.h>  // Built-in for ESP8266
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>   // use these for Arduino
#include <Wire.h>
//#include <RTClib.h>
//#include <time.h>  // use these for Arduino  /* time_t, struct tm, time, localtime, strftime */
#include "myconfig.h"
#include "Font_Data.h"
// needed for Parola
#define SPEED_TIME 75
#define PAUSE_TIME 0
#define MAX_MESG 101  // has to be 1 longer than your longest message

// RTC_DS1307 rtc;

// char need to store info for the matrix
char hm_Char[10];
char yearChar[4], dayChar[30], fullChar[50], weekdayChar[10];
char hour_Char[3], min_Char[3], sec_Char[3];
char szTime[9];  // mm:ss
char szMesg[MAX_MESG + 1] = "";

byte hour_Int, min_Int, sec_Int;
// bytes can store 0 to 255, an int can store -32,768 to 32,767 so bytes take up less memory

String TimeFormat_str, Date_str;  // to select M or I for 24hrs or 12hr clock

// parola effects
textEffect_t scrollEffect = PA_SCROLL_RIGHT;
textPosition_t scrollAlign = PA_RIGHT;


//********* USER SETTINGS - CHANGE YOUR SETTING BELOW **********

String Time_display = "I";  // M for metric 24hrs clock,  I for imperial 12hrs clock

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW  // See video number 45 you have a choice of 4 settings
#define MAX_DEVICES 4                      // As we have a 4x1 matrix
#define MAX_NUMBER_OF_ZONES 1

//Connections to a Wemos D1 Mini
const uint8_t CLK_PIN = D5;   // or SCK D5
const uint8_t DATA_PIN = D7;  // or MOSI  D7
const uint8_t CS_PIN = D8;    // or SS D8

// MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);  // I have called my instance "P"
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

byte Time_dim = 20;    // what time to auto dim the display 24 hrs clock
byte Time_bright = 7;  // what time to auto dim the display 24 hr clock
byte Max_bright = 5;   // max brightness of the maxtrix
byte Min_bright = 1;   // minimum brightness of the matrix 0-15

// ********** Change to your WiFi credentials in the myconfig.h file, select your time zone and adjust your NTS **********

const char *ssid = WIFI_SSID;                         // SSID of local network
const char *password = WIFI_PW;                       // Password on network
const char *Timezone = "IST-5:30";  // Rome/italy see link below for more short codes -1
const char *NTP_Server_1 = "in.pool.ntp.org";
const char *NTP_Server_2 = "time.nist.gov";

/*  Useful links

    Zones               https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

    NTP world servers   https://www.ntppool.org/en/

    World times         https://www.timeanddate.com/time/zones/
*/


//********* END OF USER SETTINGS**********/

void StartWiFi();
void UpdateLocalTime(String);

void getTime(char *psz, bool f = true)  // get the time and pull out the parts we need hours, minutes, seconds
{
  min_Int = atoi(min_Char);  // convert char to int
  hour_Int = atoi(hour_Char);
  sec_Int = atoi(sec_Char);

  //combine text and variables into a string for output to the Serial Monitor/matrix
  sprintf(psz, "%c%02d%c%02d%c",(f ? '.' : ' '), hour_Int, (f ? ':' : ' '), min_Int,(f ? '.' : ' '),sec_Int );  // add the seperating two dots between the Hours and minutes
  //  d or i for signed decimal integer, u – unsigned decimal integer, c for Character   	s a string of characters
}

// https://www.tutorialspoint.com/c_standard_library/c_function_sprintf.htm

void setup() {
  Serial.begin(115200);
  StartWiFi();

//   Wire.begin();
//   rtc.begin();
  
  configTime(0, 0, NTP_Server_1, NTP_Server_2);
  setenv("TZ", Timezone, 1);

  TimeFormat_str = Time_display;  // using the string above to select the type of clock 24 hrs or 12hr

  UpdateLocalTime(TimeFormat_str);
  
  P.begin(0);  // start Parola
  P.displayClear();
  P.setInvert(false);

  P.setZone(0, 0, 3);  //  for HH:MM  zone 0 0,1,3 when not using PA_FLIP_UD ect below
  //P.setZone(1, 0, 3);
  P.setFont(0, nullptr);
  P.setIntensity(Max_bright);  // brightness of matrix

  P.displayZoneText(0, szMesg, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);
  //P.displayZoneText(1, szTime, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);

  // we dont need this now but here how to turn the matrix 180 degrees
  // you also would have to change the text direction in the effects below
    P.setZone(0, 0, 3);
     P.setZoneEffect(0, true, PA_FLIP_UD);  // will make it upside down but reversed text
     P.setZoneEffect(0, true, PA_FLIP_LR);  // make the digits the not mirrored
     P.setZoneEffect(1, true, PA_FLIP_UD);
     P.setZoneEffect(1, true, PA_FLIP_LR);

}


void loop() {

  UpdateLocalTime(TimeFormat_str);  // update time

  if (hour_Int >= Time_dim || hour_Int <= Time_bright) P.setIntensity(Min_bright);  // based using 24hr clock to save code dim at 20:00 bright at 07:00
  else {
    P.setIntensity(Max_bright);  // matrix brightness
  }

  static uint32_t lastTime = 0;  // Memory (ms)
  static uint8_t display = 0;    // Current display mode
  static bool flasher = false;   // Seconds passing flasher

  P.displayAnimate();

  if (P.getZoneStatus(0)) {
    switch (display) {

      case 0:  // HH:MM format like 13:55 in a new font

        P.setFont(0, F4x7straight);
        P.setFont(1, F3x5std);
        P.setTextEffect(0, PA_PRINT, PA_NO_EFFECT);
        P.setTextEffect(1,PA_SCROLL_DOWN, PA_SCROLL_DOWN);
        P.displayZoneText(0, szMesg, PA_LEFT, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);
        P.displayZoneText(1, sec_Char, PA_RIGHT, SPEED_TIME, PAUSE_TIME, PA_SCROLL_DOWN, PA_SCROLL_DOWN);
        if (millis() - lastTime >= 1000) {
          lastTime = millis();
          getTime(szMesg, flasher);
          flasher = !flasher;
        }
        // time delay to show the time for a while
        if (sec_Int == 15 && sec_Int <= 45) {  // wait for both seconds to have equal 15 and 45 so this "time"
                                               // delay can vary a bit the first time around. dont use 00 and 30
                                               // otherwise you dont see the hour all change at say 19:59 to 20:00
          display++;

          P.setTextEffect(0, PA_PRINT, PA_SCROLL_RIGHT);
        }
        break;

      case 1:  // scroll full date Wednesday, 15, March, 2023, 11:11 the one we created

        P.setFont(0, nullptr);  // use the built-in font
        P.displayZoneText(0, fullChar, PA_RIGHT, SPEED_TIME, PAUSE_TIME, PA_SCROLL_RIGHT, PA_SCROLL_RIGHT);
        display=0;
        break;
    }
    P.displayReset(0);  // zone 0 update the display
  }
}
//#########################################################################################
void UpdateLocalTime(String Format) {
//   if (WiFi.status() != WL_CONNECTED )
//   {
//     if (rtc.isrunning()) {
//     DateTime now = rtc.now();

//     // Get the time components from the DS1307 module
//     hour_Int = now.hour();
//     min_Int = now.minute();
//     sec_Int = now.second();
//     // ...

//     // Format the time strings
//     sprintf(hm_Char, "%02d:%02d", hour_Int, min_Int);
//     sprintf(hour_Char, "%02d", hour_Int);
//     sprintf(min_Char, "%02d", min_Int);
//     sprintf(sec_Char, "%02d", sec_Int);
//   }
//   }
//   else
//   {
  time_t now;  // = time(nullptr);
  time(&now);

  //Serial.println(time(&now));
  //Unix time or Epoch or Posix time, time since 00:00:00 UTC 1st jan 1970 minus leap seconds
  // an example as of 28 feb 2023 at 16:19 1677597570

  //See http://www.cplusplus.com/reference/ctime/strftime/

  if (TimeFormat_str == "M") {

    strftime(hm_Char, 10, "%H:%M", localtime(&now));  // hours and minutes NO flashing dots 14:06
  } else {
    strftime(hm_Char, 10, "%I:%M", localtime(&now));  // Formats hour as: 02:06 ,  12hrs clock
  }

  strftime(hour_Char, 3, "%I", localtime(&now));                     // hours xx
  strftime(min_Char, 3, "%M", localtime(&now));                      // minutes xx
  strftime(sec_Char, 3, "%S", localtime(&now));                      // seconds xx
  strftime(yearChar, 5, "%G", localtime(&now));                      // year xxxx
  strftime(weekdayChar, 12, "%A", localtime(&now));                  // day of the week like Thursday
  strftime(dayChar, 30, "%c", localtime(&now));                      // Formats date as: Thu Aug 23 14:55:02 2001
  strftime(fullChar, 50, "%A, %d, %B, %G, %I:%M", localtime(&now));  // as the above includes the seconds lets make our own

  // convert char to int  NOTE  based on 24hr time so we can select when to dim the matrix

  min_Int = atoi(min_Char);
  hour_Int = atoi(hour_Char);
  sec_Int = atoi(sec_Char);
  String dayChar(dayChar);
  // Serial.println(hour_Int);
  // Serial.println(hour_Char);
  // Serial.println(minInt);
  // Serial.println(min_Char);
  // Serial.println(sec_Int);
  // Serial.println(sec_Char);
  // Serial.println("");
  //wdayInt = atoi(weekdayChar);
  //Date_str = dayChar;
}
  
//#########################################################################################
void StartWiFi() {
  /* Set the ESP to be a WiFi-client, otherwise by default, it acts as ss both a client and an access-point
      and can cause network-issues with other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  Serial.print(F("\r\nConnecting to SSID: "));
  Serial.println(String(ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println(" ");
  Serial.print("WiFi connected to address: ");
  Serial.print(WiFi.localIP());
  Serial.println(" ");
}
