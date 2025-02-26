/*
  ESP-01 pinout from top:
  
  GND    GP2 GP0 RX/GP3
  TX/GP1 CH  RST VCC

  MAX7219
  ESP-1 from rear
  Re Br Or Ye
  Gr -- -- --

  USB to Serial programming
  ESP-1 from rear, FF to GND, RR to GND before upload
  Gr FF -- Bl
  Wh -- RR Vi

  GPIO 2 - DataIn
  GPIO 1 - LOAD/CS
  GPIO 0 - CLK

  ------------------------
  NodeMCU 1.0 pinout:

  D8 - DataIn
  D7 - LOAD/CS
  D6 - CLK
  
*/


#include "Arduino.h"
#include <ESP8266WiFi.h>
#include "DHT.h"

WiFiClient client;

String date;

#define NUM_MAX 4
#define DTYPE DHT11
const int DPIN = 4; //D2

DHT dht (DPIN, DTYPE);
uint8_t degC[] = {6, 3, 3, 56, 68, 68, 68};
uint8_t display = 0;
char mesg [20]= "";
// for ESP-01 module
//#define DIN_PIN 2 // D4
//#define CS_PIN  3 // D9/RX
//#define CLK_PIN 0 // D3

// for NodeMCU 1.0
#define DIN_PIN 15  // D8
#define CS_PIN  13  // D7
#define CLK_PIN 12  // D6

#include "max7219.h"
#include "fonts.h"


// =======================================================================
// CHANGE YOUR CONFIG HERE:
// =======================================================================
const char* ssid     = "Redmi gue";     // SSID of local network
const char* password = "12345678";   // Password on network

// =======================================================================
#define MAX_DIGITS 16
byte dig[MAX_DIGITS]={0};
byte digold[MAX_DIGITS]={0};
byte digtrans[MAX_DIGITS]={0};
int updCnt = 0;
int dots = 0;
long dotTime = 0;
long clkTime = 0;
int dx=0;
int dy=0;
byte del=0;
int h,m,s;

void setup() 
{
  Serial.begin(115200);
  initMAX7219();
  dht.begin();
  sendCmdAll(CMD_SHUTDOWN,1);
  sendCmdAll(CMD_INTENSITY,0);
  Serial.print("Connecting WiFi ");
  WiFi.begin(ssid, password);
  printStringWithShift("Connecting ",16);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected: "); Serial.println(WiFi.localIP());
  Serial.println("Getting data ...");
    printStringWithShift("  WELCOME RAFI :v ",10);
   
    getTime();
    Serial.println("Data loaded");
    clkTime = millis();
    delay(200);
    WiFi.disconnect();
}
// =======================================================================

// =======================================================================
void loop()
{

  if(millis()-clkTime > 60000 && !del && dots) { // clock for 15s, then scrolls for about 30s
        printStringWithShift(date.c_str(),40);
        
        printStringWithShift(" ",40);

        float temp = dht.readTemperature();
        dtostrf (temp, 3, 1, mesg);
        strcat (mesg, " °C");
        printStringWithShift("  Temperatur: ",40);
        printStringWithShift(mesg,40);
        scrollLeft();
    
        float hum = dht.readHumidity();
        printStringWithShift("  Kelembaban:",40);
        dtostrf (hum, 3, 0, mesg);
        strcat (mesg, " %");
        printStringWithShift(mesg,40);
        scrollLeft();

        display++;
    
   delay(2000);
    updCnt--;
    clkTime = millis();
  }
  
  if(millis()-dotTime > 500) {
    dotTime = millis();
    dots = !dots;
  }
  updateTime();
  showAnimClock();
  
}

// =======================================================================

void showSimpleClock()
{
  dx=dy=0;
  clr();
  showDigit(h/10,  1, dig6x8);
  showDigit(h%10,  9, dig6x8);
  showDigit(m/10, 18, dig6x8);
  showDigit(m%10, 26, dig6x8);
 // showDigit(s/10, 38, dig6x8);
  //showDigit(s%10, 46, dig6x8);
  setCol(16,dots ? B00100100 : 0);
  setCol(33,dots ? B00100100 : 0);
  refreshAll();
}

// =======================================================================

void showAnimClock()
{
  byte digPos[6]={1,9,18,26,36,44};
  int digHt = 12;
  int num = 6; 
  int i;
  if(del==0) {
    del = digHt;
    for(i=0; i<num; i++) digold[i] = dig[i];
    dig[0] = h/10 ? h/10 : 10;
    dig[1] = h%10;
    dig[2] = m/10;
    dig[3] = m%10;
    dig[4] = s/10;
    dig[5] = s%10;
    for(i=0; i<num; i++)  digtrans[i] = (dig[i]==digold[i]) ? 0 : digHt;
  } else
    del--;
  
  clr();
  for(i=0; i<num; i++) {
    if(digtrans[i]==0) {
      dy=0;
      showDigit(dig[i], digPos[i], dig6x8);
    } else {
      dy = digHt-digtrans[i];
      showDigit(digold[i], digPos[i], dig6x8);
      dy = -digtrans[i];
      showDigit(dig[i], digPos[i], dig6x8);
      digtrans[i]--;
    }
  }
  dy=0;
  setCol(16,dots ? B00100100 : 0);
  setCol(33,dots ? B00100100 : 0);
  refreshAll();
  delay(20);
}

// =======================================================================

void showDigit(char ch, int col, const uint8_t *data)
{
  if(dy<-8 | dy>8) return;
  int len = pgm_read_byte(data);
  int w = pgm_read_byte(data + 1 + ch * len);
  col += dx;
  for (int i = 0; i < w; i++)
    if(col+i>=0 && col+i<8*NUM_MAX) {
      byte v = pgm_read_byte(data + 1 + ch * len + 1 + i);
      if(!dy) scr[col + i] = v; else scr[col + i] |= dy>0 ? v>>dy : v<<-dy;
    }
}

// =======================================================================

void setCol(int col, byte v)
{
  if(dy<-8 | dy>8) return;
  col += dx;
  if(col>=0 && col<8*NUM_MAX)
    if(!dy) scr[col] = v; else scr[col] |= dy>0 ? v>>dy : v<<-dy;
}

// =======================================================================

int showChar(char ch, const uint8_t *data)
{
  int len = pgm_read_byte(data);
  int i,w = pgm_read_byte(data + 1 + ch * len);
  for (i = 0; i < w; i++)
    scr[NUM_MAX*8 + i] = pgm_read_byte(data + 1 + ch * len + 1 + i);
  scr[NUM_MAX*8 + i] = 0;
  return w;
}

// =======================================================================

void printCharWithShift(unsigned char c, int shiftDelay) {
  
  if (c < ' ' || c > '~'+25) return;
  c -= 32;
  int w = showChar(c, font);
  for (int i=0; i<w+1; i++) {
    delay(shiftDelay);
    scrollLeft();
    refreshAll();
  }
}

// =======================================================================

void printStringWithShift(const char* s, int shiftDelay){
  while (*s) {
    printCharWithShift(*s, shiftDelay);
    s++;
  }
}

// =======================================================================

float utcOffset = 7;//OFSET WAKTU
long localEpoc = 0;
long localMillisAtUpdate = 0;

void getTime()
{
  WiFiClient client;
  if (!client.connect("www.google.com", 80)) {
    Serial.println("connection to google failed");
    return;
  }

  client.print(String("GET / HTTP/1.1\r\n") +
               String("Host: www.google.com\r\n") +
               String("Connection: close\r\n\r\n"));
  int repeatCounter = 0;
  while (!client.available() && repeatCounter < 10) {
    delay(500);
    //Serial.println(".");
    repeatCounter++;
    
  }

  String line;
  client.setNoDelay(false);
  while(client.connected() && client.available()) {
    line = client.readStringUntil('\n');
    line.toUpperCase();
    if (line.startsWith("DATE: ")) {
      date = "     "+line.substring(6, 22);
      h = line.substring(23, 25).toInt();
      m = line.substring(26, 28).toInt();
      s = line.substring(29, 31).toInt();
      localMillisAtUpdate = millis();
      localEpoc = (h * 60 * 60 + m * 60 + s);
      
    }
  }
  client.stop();
}

// =======================================================================

void updateTime()
{
  long curEpoch = localEpoc + ((millis() - localMillisAtUpdate) / 1000);
  long epoch = round(curEpoch + 3600 * utcOffset + 86400L);
  h = ((epoch  % 86400L) / 3600) % 24;
  m = (epoch % 3600) / 60;
  s = epoch % 60;
}

// =======================================================================

