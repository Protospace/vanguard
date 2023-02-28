// Protospace Vanguard
// Helps members host new members
//
// Board: Wemos D1 Mini
//        - select "LOLIN(WEMOS) D1 R2 & mini"

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>  // "LiquidCrystal I2C" by Marco Schwartz v1.1.2

//String portalAPI = "https://api.my.protospace.ca";
String portalAPI = "https://api.spaceport.dns.t0.vc";

#define DEBUG 1

#define BUTTON1LED D0
#define BUTTON1SW  D5
#define BUTTON2LED D6
#define BUTTON2SW  D3
#define BUTTON3LED D7
#define BUTTON3SW  D4

#define BUTTON_HOLD_TIME 1000
#define BUTTON_PRESS_TIME 20

enum buttonStates {
	OPEN,
	CLOSED,
	CHECK_PRESSED,
	PRESSED,
	HELD,
	NUM_BUTTONSTATES
};

enum buttonStates button1State = OPEN;
enum buttonStates button2State = OPEN;
enum buttonStates button3State = OPEN;

LiquidCrystal_I2C lcd(0x27, 16, 2);

void rebootArduino() {
	lcd.clear();
	lcd.print("REBOOTING...");
	delay(1000);
	ESP.restart();
}

void setup()
{
	Serial.begin(115200);

	Serial.println("");
	Serial.println("======= BOOT UP =======");

	pinMode(BUTTON1LED, OUTPUT);
	pinMode(BUTTON2LED, OUTPUT);
	pinMode(BUTTON3LED, OUTPUT);

	pinMode(BUTTON1SW, INPUT_PULLUP);
	pinMode(BUTTON2SW, INPUT_PULLUP);
	pinMode(BUTTON3SW, INPUT_PULLUP);

	delay(1000);

	lcd.init();
	lcd.backlight();

	lcd.clear();
	lcd.print("BOOT UP");

	Serial.println("Setup complete.");
	delay(500);
}

void loop() {
	pollButtons();
}

void pollButtons() {
	static unsigned long button1Time = 0;
	static unsigned long button2Time = 0;
	static unsigned long button3Time = 0;

	processButtonState(BUTTON1SW, button1State, button1Time);
	processButtonState(BUTTON2SW, button2State, button2Time);
	processButtonState(BUTTON3SW, button3State, button3Time);

	if (DEBUG) {
		if (button1State == PRESSED) {
			Serial.println("Button 1 pressed");
		} else if (button1State == HELD) {
			Serial.println("Button 1 held");
		}

		if (button2State == PRESSED) {
			Serial.println("Button 2 pressed");
		} else if (button2State == HELD) {
			Serial.println("Button 2 held");
		}

		if (button3State == PRESSED) {
			Serial.println("Button 3 pressed");
		} else if (button3State == HELD) {
			Serial.println("Button 3 held");
		}
	}
}

void processButtonState(int buttonPin, buttonStates &buttonState, unsigned long &buttonTime) {
	bool pinState = !digitalRead(buttonPin);

	switch(buttonState) {
		case OPEN:
			if (pinState) {
				buttonState = CLOSED;
				buttonTime = millis();
			}
			break;
		case CLOSED:
			if (millis() >= buttonTime + BUTTON_HOLD_TIME) {
				buttonState = HELD;
			}
			if (pinState) {
				;
			} else {
				buttonState = CHECK_PRESSED;
			}
			break;
		case CHECK_PRESSED:
			if (millis() >= buttonTime + BUTTON_PRESS_TIME) {
				buttonState = PRESSED;
			} else {
				buttonState = OPEN;
			}
			break;
		case PRESSED:
			buttonState = OPEN;
			break;
		case HELD:
			if (!pinState) {
				buttonState = OPEN;
			}
			break;
		default:
			break;
	}
}
