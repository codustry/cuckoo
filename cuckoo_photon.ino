// This #include statement was automatically added by the Particle IDE.
#include "SparkTime/SparkTime.h"
#include <climits>
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))
#define bit(b) (1UL << (b))

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
typedef uint16_t word;
#define abs(x) ((x)>0?(x):-(x))
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))

#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))
UDP UDPClient;
SparkTime rtc;

unsigned long currentTime;
unsigned long lastTime = 0UL;
String timeStr;
const byte numPins = 8;
byte pins[] = {0,1,2,3,4,5,6,7};

bool alarmOnceSet = false;
short alarmHour = 0;
short alarmMinute = 0;
bool timeOnceSet = false;
short lastFPGAState = 0xf;
bool internetOnceConnected = false;

void setup() {
    Particle.publish("devStatus", "Device ON - Cuckoo MODE");

    //internet time sync
    rtc.begin(&UDPClient, "north-america.pool.ntp.org");
    rtc.setTimeZone(6); // gmt offset



    //set pin D0-D7 to be output
    for(int i=0;i<numPins;i++){
        pinMode(i, OUTPUT);
    }

    //set pin A0-A3 to be input
    for(int i=10;i<14;i++){
        pinMode(i, INPUT);
    }
    setIdleStateOnFPGA();

    //set debug LED
    pinMode(A4, OUTPUT); //FGPA Programmed?
    pinMode(A5, OUTPUT); //internet?

    delay(1000);
    reportState();
}

void loop() {
    if(lastFPGAState != 0xf){
        if(timeOnceSet == true){
            //check if an hour already
        }else{
            setTimeOnFPGA();
        }
    }
    delay(1000*5);
    reportState();
}

void checkInternet(){
    if(internetOnceConnected = WiFi.ready())digitalWrite(A5, HIGH);
    else digitalWrite(A5, LOW);
}

void reportState(){
    checkInternet();
    short state = 0;
    for(int i=0;i<4;i++){
        bitWrite(state, i, digitalRead(10+i));
    }
    digitalWrite(A4,LOW);
    switch (state) {
        case 0: Particle.publish("fpgaStatus", "normal"); break;
        case 1: Particle.publish("fpgaStatus", "never Set Clock"); break;
        case 2: Particle.publish("fpgaStatus", "never Set Alarm"); break;
        case 3: Particle.publish("fpgaStatus", "ringing"); break;
        case 0xf: Particle.publish("fpgaStatus", "fpga is on but unprogramed"); digitalWrite(A4,HIGH); break;
        default: break;
    }
    lastFPGAState = state ;
}

void setIdleStateOnFPGA(){
    setOpPins(0x0);
    setDataPins(0);
}

void setTimeOnFPGA(){
    Particle.publish("debug", "run setTimeOnFPGA");
    currentTime = rtc.now();

    short hour =  rtc.hour(currentTime);
    short hour_ = hour/10;
    short hour__ = hour-hour_*10;
    setOpPins(0xe);
    setDataPins(hour__);
    delay(50);
    setOpPins(0xf);
    setDataPins(hour_);
    delay(50);

    short min =  rtc.minute(currentTime);
    short min_ = min/10;
    short min__ = min-min_*10;
    setOpPins(0xc);
    setDataPins(min_);
    delay(50);
    setOpPins(0xd);
    setDataPins(min__);
    delay(50);

    short second =  rtc.second(currentTime);
    short second_ = second/10;
    short second__ = second-second_*10;
    setOpPins(0xa);
    setDataPins(second_);
    delay(50);
    setOpPins(0xb);
    setDataPins(second__);
    delay(50);
    Particle.publish("Debug", "Clock is seted at"+rtc.ISODateString(currentTime));
}

void setAlarmOnFPGA(short hour,short min){
    short hour_ = hour/10;
    short hour__ = hour-hour_*10;
    setOpPins(0x6);
    setDataPins(hour__);
    delay(5);
    setOpPins(0x7);
    setDataPins(hour_);
    delay(5);

    short min_ = min/10;
    short min__ = min-min_*10;
    setOpPins(0x4);
    setDataPins(min_);
    delay(5);
    setOpPins(0x5);
    setDataPins(min__);
    delay(5);

    timeStr = "";
    timeStr += rtc.hour12String(currentTime);
    timeStr += ":";
    timeStr += rtc.minuteString(currentTime);
    timeStr += ":";
    timeStr += rtc.secondString(currentTime);
    timeStr += " ";
    timeStr += rtc.AMPMString(currentTime);
    Particle.publish("info", timeStr);
    Particle.publish("Debug", "Clock is seted at"+rtc.ISODateString(currentTime)+"  "+timeStr);
}

/*function to set pin D0-D3 to desire byte*/
void setOpPins(byte data){
    for(int i=0;i<4;i++){
        byte state = bitRead(data, i);
        digitalWrite(pins[i], state);
    }
}
/*function to set pin D4-D7 to desire number*/
void setDataPins(byte data){
    for(int i=4;i<8;i++){
        byte state = bitRead(data, i);
        digitalWrite(pins[i], state);
    }
}


void publishTime(){
    currentTime = rtc.now();
    if (currentTime != lastTime) {
        byte sec = rtc.second(currentTime);
        if (sec == 10) {
        	// Build Date String
        	timeStr = "";
        	timeStr += rtc.dayOfWeekString(currentTime);
        	timeStr += ", ";
        	timeStr += rtc.monthNameString(currentTime);
        	timeStr += " ";
        	timeStr += rtc.dayString(currentTime);
        	timeStr += ", ";
        	timeStr += rtc.yearString(currentTime);
        	Particle.publish("info", timeStr);
        } else if (sec == 40) {
    	    // Including current timezone
    	    Particle.publish("info", rtc.ISODateString(currentTime));
    	    //Serial.println(rtc.ISODateString(currentTime));
        } else {
        	// Just the time in 12 hour format
        	timeStr = "";
        	timeStr += rtc.hour12String(currentTime);
        	timeStr += ":";
        	timeStr += rtc.minuteString(currentTime);
        	timeStr += ":";
        	timeStr += rtc.secondString(currentTime);
        	timeStr += " ";
        	timeStr += rtc.AMPMString(currentTime);
        	Particle.publish("info", timeStr);
        }
        lastTime = currentTime;
    }
}
