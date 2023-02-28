#define BUTTON1LED D0
#define BUTTON1SW  D5
#define BUTTON2LED D6
#define BUTTON2SW  D3
#define BUTTON3LED D7
#define BUTTON3SW  D4

void setup() {
	pinMode(BUTTON1LED, OUTPUT);
	pinMode(BUTTON2LED, OUTPUT);
	pinMode(BUTTON3LED, OUTPUT);

	pinMode(BUTTON1SW, INPUT_PULLUP);
	pinMode(BUTTON2SW, INPUT_PULLUP);
	pinMode(BUTTON3SW, INPUT_PULLUP);
}

void loop() {
	if (!digitalRead(BUTTON1SW)) {
		digitalWrite(BUTTON1LED, LOW);
	} else {
		digitalWrite(BUTTON1LED, HIGH);
	}

	if (!digitalRead(BUTTON2SW)) {
		digitalWrite(BUTTON2LED, LOW);
	} else {
		digitalWrite(BUTTON2LED, HIGH);
	}

	if (!digitalRead(BUTTON3SW)) {
		digitalWrite(BUTTON3LED, LOW);
	} else {
		digitalWrite(BUTTON3LED, HIGH);
	}

	delay(50);
}
