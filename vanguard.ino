// Protospace Vanguard
// Helps members host new members
//
// Board: Wemos D1 Mini
//        - select "LOLIN(WEMOS) D1 R2 & mini"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <ArduinoJson.h>        // v6.19.4
#include <LiquidCrystal_I2C.h>  // "LiquidCrystal I2C" by Marco Schwartz v1.1.2

#include "secrets.h"

//String portalAPI = "https://api.my.protospace.ca";
String portalAPI = "https://api.spaceport.dns.t0.vc";

#define DEBUG 1

#define BUTTON1LED D0
#define BUTTON1SW  D5
#define BUTTON2LED D6
#define BUTTON2SW  D3
#define BUTTON3LED D7
#define BUTTON3SW  D4
#define NUM_BUTTONS 3

#define LED_ON  LOW
#define LED_OFF HIGH

#define BUTTON_HOLD_TIME 1000
#define BUTTON_PRESS_TIME 20

#define CONTROLLER_IDLE_DELAY_MS 4500
#define CONTROLLER_UI_DELAY_MS 1000
#define CONNECT_TIMEOUT_MS 30000
#define ELLIPSIS_ANIMATION_DELAY_MS 1000
#define SCROLL_ANIMATION_DELAY_MS 500

enum buttonStates {
	BUTTON_OPEN,
	BUTTON_CLOSED,
	BUTTON_CHECK_PRESSED,
	BUTTON_PRESSED,
	BUTTON_HELD,
	NUM_BUTTONSTATES
};
enum buttonStates button1State = BUTTON_OPEN;
enum buttonStates button2State = BUTTON_OPEN;
enum buttonStates button3State = BUTTON_OPEN;

enum controllerStates {
	CONTROLLER_BEGIN,
	CONTROLLER_WIFI_CONNECT,
	CONTROLLER_GET_TIME,
	CONTROLLER_IDLE,
	CONTROLLER_IDLE_DELAY,
	CONTROLLER_IDLE_WAIT,
	CONTROLLER_UI_DELAY,
	CONTROLLER_UI_WAIT,
};
enum controllerStates controllerState = CONTROLLER_BEGIN;

enum LEDStates {
	LEDS_OFF,
	LEDS_SCROLL,
};
enum LEDStates LEDState = LEDS_OFF;

LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiClientSecure wc;

void rebootArduino() {
	lcd.clear();
	lcd.print("REBOOTING...");
	delay(1000);
	ESP.restart();
}


void processControllerState() {
	static unsigned long timer = millis();
	static enum controllerStates nextControllerState;
	static int statusCode;
	static StaticJsonDocument<1024> jsonDoc;
	static int retryCount;

	String response;
	String postData;
	bool failed;
	time_t now;
	struct tm timeinfo;
	int i;
	int result;
	HTTPClient https;

	switch (controllerState) {
		case CONTROLLER_BEGIN:
			Serial.println("[WIFI] Connecting...");
			retryCount = 0;

			timer = millis();
			controllerState = CONTROLLER_WIFI_CONNECT;
			break;

		case CONTROLLER_WIFI_CONNECT:
			lcd.setCursor(0,0);
			lcd.print("CONNECTING");
			for (i = 0; i < (millis() / ELLIPSIS_ANIMATION_DELAY_MS) % 4; i++) {
				lcd.print(".");
			}
			lcd.print("   ");

			if (WiFi.status() == WL_CONNECTED) {
				Serial.print("[WIFI] Connected. IP address: ");
				Serial.println(WiFi.localIP());

				lcd.setCursor(0,1);
				lcd.print(WiFi.localIP());

				Serial.println("[TIME] Setting time using NTP.");
				configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
				nextControllerState = CONTROLLER_GET_TIME;
				controllerState = CONTROLLER_UI_DELAY;
			}

			if (millis() - timer > CONNECT_TIMEOUT_MS) {  // overflow safe
				WiFi.disconnect();
				WiFi.mode(WIFI_OFF);

				lcd.clear();
				lcd.print("CONNECT FAILED");
				lcd.setCursor(0,1);
				lcd.print("WAIT 5 MINS...");
				delay(300 * 1000);

				rebootArduino();
			}

			break;

		case CONTROLLER_GET_TIME:
			// time is needed to compare closing time

			lcd.setCursor(0,0);
			lcd.print("GETTING TIME");
			for (i = 0; i < (millis() / ELLIPSIS_ANIMATION_DELAY_MS) % 4; i++) {
				lcd.print(".");
			}
			lcd.print("   ");

			time(&now);
			if (now > 8 * 3600 * 2) {
				gmtime_r(&now, &timeinfo);
				Serial.print("[TIME] Current time in UTC: ");
				Serial.print(asctime(&timeinfo));

				lcd.clear();
				lcd.print("CHECK PORTAL...");

				Serial.println("Moving to idle state...");
				controllerState = CONTROLLER_IDLE;
			}

			break;

		case CONTROLLER_IDLE:
			result = https.begin(wc, portalAPI + "/stats/");

			if (!result) {
				Serial.println("[WIFI] https.begin failed.");

				retryCount++;
				if (retryCount > 5) {
					lcd.clear();
					lcd.print("CONNECTION ERROR");
					nextControllerState = CONTROLLER_BEGIN;
					controllerState = CONTROLLER_UI_DELAY;
					break;
				}

				controllerState = CONTROLLER_IDLE_DELAY;
				break;
			}

			result = https.GET();

			Serial.printf("[WIFI] Http code: %d\n", result);

			if (result != HTTP_CODE_OK) {
				Serial.printf("[WIFI] Portal GET failed, error:\n%s\n", https.errorToString(result).c_str());
				lcd.clear();
				lcd.print("BAD REQUEST: ");
				lcd.print(result);

				retryCount++;
				if (retryCount > 5) {
					lcd.clear();
					lcd.print("CONNECTION ERROR");
					nextControllerState = CONTROLLER_BEGIN;
					controllerState = CONTROLLER_UI_DELAY;
					break;
				}

				controllerState = CONTROLLER_IDLE_DELAY;
				break;
			}
			retryCount = 0;

			Serial.println("[WIFI] Stats success.");
			lcd.setCursor(0,1);
			lcd.print("GOOD");

			controllerState = CONTROLLER_IDLE_DELAY;
			LEDState = LEDS_SCROLL;

			break;



		case CONTROLLER_IDLE_DELAY:
			timer = millis();
			controllerState = CONTROLLER_IDLE_WAIT;
			break;

		case CONTROLLER_IDLE_WAIT:
			if (millis() - timer > CONTROLLER_IDLE_DELAY_MS) {  // overflow safe
				controllerState = CONTROLLER_IDLE;
			}
			break;

		case CONTROLLER_UI_DELAY:
			timer = millis();
			controllerState = CONTROLLER_UI_WAIT;
			break;

		case CONTROLLER_UI_WAIT:
			if (millis() - timer > CONTROLLER_UI_DELAY_MS) {  // overflow safe
				controllerState = nextControllerState;
			}
			break;
	}

	return;
}

void processLEDState() {
	int onLed = 0;

	switch(LEDState) {
		case LEDS_OFF:
			digitalWrite(BUTTON1LED, LED_OFF);
			digitalWrite(BUTTON2LED, LED_OFF);
			digitalWrite(BUTTON3LED, LED_OFF);

			break;
		case LEDS_SCROLL:
			onLed = (millis() / SCROLL_ANIMATION_DELAY_MS) % NUM_BUTTONS;

			if (onLed == 0) {
				digitalWrite(BUTTON1LED, LED_ON);
				digitalWrite(BUTTON2LED, LED_OFF);
				digitalWrite(BUTTON3LED, LED_OFF);
			} else if (onLed == 1) {
				digitalWrite(BUTTON1LED, LED_OFF);
				digitalWrite(BUTTON2LED, LED_ON);
				digitalWrite(BUTTON3LED, LED_OFF);
			} else if (onLed == 2) {
				digitalWrite(BUTTON1LED, LED_OFF);
				digitalWrite(BUTTON2LED, LED_OFF);
				digitalWrite(BUTTON3LED, LED_ON);
			}

			break;
	}

	return;
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

	WiFi.mode(WIFI_STA);
	WiFi.begin(WIFI_SSID, WIFI_PASS);

	//X509List cert(lets_encrypt_ca);
	//wc.setTrustAnchors(&cert);
	wc.setInsecure();  // disables all SSL checks. don't use in production

	Serial.println("Setup complete.");
	delay(500);
}

void loop() {
	pollButtons();
	processControllerState();
	processLEDState();
}

void pollButtons() {
	static unsigned long button1Time = 0;
	static unsigned long button2Time = 0;
	static unsigned long button3Time = 0;

	processButtonState(BUTTON1SW, button1State, button1Time);
	processButtonState(BUTTON2SW, button2State, button2Time);
	processButtonState(BUTTON3SW, button3State, button3Time);

	if (DEBUG) {
		if (button1State == BUTTON_PRESSED) {
			Serial.println("Button 1 pressed");
		} else if (button1State == BUTTON_HELD) {
			Serial.println("Button 1 held");
		}

		if (button2State == BUTTON_PRESSED) {
			Serial.println("Button 2 pressed");
		} else if (button2State == BUTTON_HELD) {
			Serial.println("Button 2 held");
		}

		if (button3State == BUTTON_PRESSED) {
			Serial.println("Button 3 pressed");
		} else if (button3State == BUTTON_HELD) {
			Serial.println("Button 3 held");
		}
	}
}

void processButtonState(int buttonPin, buttonStates &buttonState, unsigned long &buttonTime) {
	bool pinState = !digitalRead(buttonPin);

	switch(buttonState) {
		case BUTTON_OPEN:
			if (pinState) {
				buttonState = BUTTON_CLOSED;
				buttonTime = millis();
			}
			break;
		case BUTTON_CLOSED:
			if (millis() >= buttonTime + BUTTON_HOLD_TIME) {
				buttonState = BUTTON_HELD;
			}
			if (pinState) {
				;
			} else {
				buttonState = BUTTON_CHECK_PRESSED;
			}
			break;
		case BUTTON_CHECK_PRESSED:
			if (millis() >= buttonTime + BUTTON_PRESS_TIME) {
				buttonState = BUTTON_PRESSED;
			} else {
				buttonState = BUTTON_OPEN;
			}
			break;
		case BUTTON_PRESSED:
			buttonState = BUTTON_OPEN;
			break;
		case BUTTON_HELD:
			if (!pinState) {
				buttonState = BUTTON_OPEN;
			}
			break;
		default:
			break;
	}
}
