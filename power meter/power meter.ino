/**************************************************************************
 This is an example for our Monochrome OLEDs based on SSD1306 drivers

 Pick one up today in the adafruit shop!
 ------> http://www.adafruit.com/category/63_98

 This example is for a 128x64 pixel display using I2C to communicate
 3 pins are required to interface (two I2C and one reset).

 Adafruit invests time and resources providing this open
 source code, please support Adafruit and open-source
 hardware by purchasing products from Adafruit!

 Written by Limor Fried/Ladyada for Adafruit Industries,
 with contributions from the open source community.
 BSD license, check license.txt for more information
 All text above, and the splash screen below must be
 included in any redistribution.
 **************************************************************************/

#include <HID-Project.h>
#include <RTClib.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_INA260.h>
#ifdef __AVR__
#include <avr/power.h>
#endif
#include <Adafruit_SSD1306.h>

const int pinLed = LED_BUILTIN;
const int pinButton = 4;

// Buffer to hold RawHID data.
// If host tries to send more data than this,
// it will respond with an error.
// If the data is not read until the host sends the next data
// it will also respond with an error and the data will be lost.
uint8_t rawhidData[255];

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

 // Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
 // The pins for I2C are defined by the Wire-library. 
 // On an arduino UNO:       A4(SDA), A5(SCL)
 // On an arduino MEGA 2560: 20(SDA), 21(SCL)
 // On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_INA260 ina260 = Adafruit_INA260();

unsigned long time = 0;

unsigned long displayLastTime = 0;
unsigned long displayInterval = 33;

unsigned long usbLastTime = 0;
unsigned long usbInterval = 33;

RTC_DS3231 rtc;

uint16_t totalWatts = 0;
uint16_t totalTime = 0;


uint16_t intervalMiliSeconds = 0, intervalSeconds = 0;
float intervalMiliAmps = 0;
int intervalAmps = 0;


uint16_t intervalMiliAmpMiliSeconds = 0, intervalAmpMiliSeconds = 0;
uint16_t intervalMiliAmpSeconds = 0, intervalAmpSeconds = 0;
uint16_t intervalMiliAmpMinutes = 0, intervalAmpMinutes = 0;
uint16_t intervalMiliAmpHours = 0, intervalAmpHours = 0;
uint16_t intervalMiliWattMiliSeconds = 0, intervalWattMiliSeconds = 0;
uint16_t intervalMiliWattSeconds = 0, intervalWattSeconds = 0;
uint16_t intervalMiliWattMinutes = 0, intervalWattMinutes = 0;
uint16_t intervalMiliWattHours = 0, intervalWattHours = 0;

void intervalAddMiliSeconds(int ms);
void intervalAddMiliAmps(float ma);
void intervalAddMiliAmpMiliSeconds(int m);
void intervalAddMiliAmpSeconds(int m);
void intervalAddMiliAmpMinutes(int m);
void intervalAddMiliWattMiliSeconds(float miliWatts, uint16_t miliseconds);
void intervalAddMiliWattSeconds(int m);
void intervalAddMiliWattMinutes(int m);

float totalEnergyUse = 0;

uint8_t megabuff1[64];

void setup() {
    Serial.begin(115200);

    pinMode(pinLed, OUTPUT);
    pinMode(pinButton, INPUT_PULLUP);

    // Set the RawHID OUT report array.
  // Feature reports are also (parallel) possible, see the other example for this.
    RawHID.begin(rawhidData, sizeof(rawhidData));

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;); // Don't proceed, loop forever
    }

    ina260.begin();
    ina260.setAveragingCount(INA260_COUNT_16);
    ina260.setVoltageConversionTime(INA260_TIME_558_us);
    ina260.setCurrentConversionTime(INA260_TIME_558_us);
    ina260.setMode(INA260_MODE_CONTINUOUS);

    rtc.begin();

    if (rtc.lostPower()) {
        Serial.println("RTC lost power, let's set the time!");
        // When time needs to be set on a new device, or after a power loss, the
        // following line sets the RTC to the date & time this sketch was compiled
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        // This line sets the RTC with an explicit date & time, for example to set
        // January 21, 2014 at 3am you would call:
        // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    }

    display.clearDisplay();
    time = millis();
    for (int i = 0; i < sizeof(megabuff1); i++) {
        megabuff1[i] = 0;
    }
}

void loop() {
    time = millis();
    if ((time - displayLastTime >= displayInterval) || (time - usbLastTime >= usbInterval)) {
        float current = ina260.readCurrent();
        int miliAmps = current;
        if (miliAmps < 0) {
            miliAmps = 0;
        }

        float voltage = ina260.readBusVoltage();
        int miliVolts = voltage;
        if (miliVolts < 0) {
            miliVolts = 0;
        }

        float power = ina260.readPower();
        float miliWatts = power;
        if ((miliAmps) <= 10 && miliWatts >= 10) {
            miliWatts = 0;
        }

        if (time - displayLastTime >= displayInterval) {
            display.clearDisplay();
            char ampsString[20];
            char voltsString[20];
            char wattsString[20];
            char dateString[20];
            uint16_t thisTime = (time - displayLastTime);
            totalTime += thisTime;

            float miliWattMiliSeconds = miliWatts * thisTime;

            Serial.print(miliWatts);
            Serial.print("mW * ");
            Serial.print(thisTime);
            Serial.print("ms = ");
            Serial.print(miliWattMiliSeconds);
            Serial.println("mWms");

            //intervalAddMiliWattMiliSeconds(miliWatts, thisTime);
            intervalAddMiliAmps(miliAmps);
            intervalAddMiliSeconds(thisTime);

            sprintf(ampsString, "%05dmA", miliAmps);
            sprintf(voltsString, "%05dmV", miliVolts);
            //sprintf(wattsString, "%06dmW", miliWatts);
            uint16_t avgEnergy = (uint16_t)calcAvgEnergyUseMiliWattMinutes();
            sprintf(wattsString, "%dmWm", avgEnergy);

            DateTime now = rtc.now();

            sprintf(dateString, "%d/%d/%d", now.month(), now.day(), now.year());

            display.setTextSize(2); // Draw 2X-scale text
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0, 0);
            display.println(ampsString);
            display.println(voltsString);
            display.println(wattsString);
            display.println(dateString);
            display.display();
            displayLastTime = time;
        }

        if (time - usbLastTime >= usbInterval) {
            byte* b = (byte*)&current;

            megabuff1[0] = b[0];
            megabuff1[1] = b[1];
            megabuff1[2] = b[2];
            megabuff1[3] = b[3];

            byte* c = (byte*)&voltage;

            megabuff1[4] = c[0];
            megabuff1[5] = c[1];
            megabuff1[6] = c[2];
            megabuff1[7] = c[3];

            byte* d = (byte*)&power;

            megabuff1[8] = d[0];
            megabuff1[9] = d[1];
            megabuff1[10] = d[2];
            megabuff1[11] = d[3];

            byte* e = (byte*)&time;

            megabuff1[12] = e[0];
            megabuff1[13] = e[1];
            megabuff1[14] = e[2];
            megabuff1[15] = e[3];

            RawHID.write(megabuff1, sizeof(megabuff1));
            usbLastTime = time;
        }
        
    }

    // Send data to the host
    if (!digitalRead(pinButton)) {
        digitalWrite(pinLed, HIGH);

        // Create buffer with numbers and send it
        uint8_t megabuff[100];
        for (uint8_t i = 0; i < sizeof(megabuff); i++) {
            megabuff[i] = i;
        }
        RawHID.write(megabuff, sizeof(megabuff));
        //RawHID.println("Test Debug Message");

        // Simple debounce
        delay(300);
        digitalWrite(pinLed, LOW);
    }


    // Check if there is new data from the RawHID device
    auto bytesAvailable = RawHID.available();
    if (bytesAvailable)
    {
        digitalWrite(pinLed, HIGH);

        // Mirror data via Serial
        while (bytesAvailable--) {
            Serial.println(RawHID.read());
        }

        digitalWrite(pinLed, LOW);
    }
}

void intervalAddMiliSeconds(int ms) {
    intervalMiliSeconds += ms;
    int s = intervalMiliSeconds / 1000;
    intervalMiliSeconds = intervalMiliSeconds % 1000;
    intervalSeconds += s;
}

void intervalAddMiliAmps(float ma) {
    intervalMiliAmps += ma;
    //int a = intervalMiliAmps / 1000;
    //intervalMiliAmps = intervalMiliAmps - (1000 * a);
    //intervalAmps += a;
}

float calcAvgEnergyUseMiliWattMinutes(){
    return intervalMiliAmps / intervalSeconds;
}

void intervalAddMiliAmpMiliSeconds(int m) {
    intervalMiliAmpMiliSeconds += m;
    int a = intervalMiliAmpMiliSeconds / 1000;
    intervalMiliAmpMiliSeconds = intervalMiliAmpMiliSeconds % 1000;
    intervalAddMiliAmpSeconds(a);
}

void intervalAddMiliAmpSeconds(int m) {
    intervalMiliAmpSeconds += m;
    int a = intervalMiliAmpSeconds / 60;
    intervalMiliAmpSeconds = intervalMiliAmpSeconds % 1000;
    intervalAddMiliAmpMinutes(a);
}

void intervalAddMiliAmpMinutes(int m) {
    intervalMiliAmpMinutes += m;
    int a = intervalMiliAmpMinutes / 60;
    intervalMiliAmpMinutes = intervalMiliAmpMinutes % 1000;
    intervalMiliAmpHours += a;
}

void intervalAddAmpMiliSeconds(int m) {
    intervalAmpMiliSeconds += m;
    int a = intervalAmpMiliSeconds / 1000;
    intervalAmpMiliSeconds = intervalAmpMiliSeconds % 1000;
    intervalAddAmpSeconds(a);
}

void intervalAddAmpSeconds(int m) {
    intervalAmpSeconds += m;
    int a = intervalAmpSeconds / 60;
    intervalAmpSeconds = intervalAmpSeconds % 1000;
    intervalAddAmpMinutes(a);
}

void intervalAddAmpMinutes(int m) {
    intervalAmpMinutes += m;
    int a = intervalAmpMinutes / 60;
    intervalAmpMinutes = intervalAmpMinutes % 60;
    intervalAmpHours += a;
}

void intervalAddMiliWattMiliSeconds(float miliWatts, uint16_t miliseconds) {
    //intervalMiliWattMiliSeconds += m;
    int a = intervalMiliWattMiliSeconds / 1000;
    intervalMiliWattMiliSeconds = intervalMiliWattMiliSeconds % 1000;
    intervalAddMiliWattSeconds(a);
}

void intervalAddMiliWattSeconds(int m) {
    Serial.print("Adding ");
    Serial.print(m);
    Serial.println("mWs");
    intervalMiliWattSeconds += m;
    int a = intervalMiliWattSeconds / 60;
    intervalMiliWattSeconds = intervalMiliWattSeconds % 60;
    intervalAddMiliWattMinutes(a);
}

void intervalAddMiliWattMinutes(int m) {
    intervalMiliWattMinutes += m;
    int a = intervalMiliWattMinutes / 60;
    intervalMiliWattMinutes = intervalMiliWattMinutes % 60;
    intervalMiliWattHours += a;
}