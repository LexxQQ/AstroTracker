//#include "timer-api.h"
#include <splash.h>
#include <gfxfont.h>
#include <Adafruit_SPITFT_Macros.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_GrayOLED.h>
#include "GyverEncoder.h"
#include <GyverStepper.h>
#include <GyverTimers.h>
#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define encoderCLK	2
#define encoderDT	3
#define encoderSW	4

#define motorPinSTEP	6
#define motorPinDIR		7
#define motorPinENABLE	8

// Управление микрошаговым режимом.
// По умолчанию входы притянуты к питанию и выставлен шаг 1 / 8.
// Для установки полного шага, на оба входа нужно подать низкий уровень, для полушага только на MS2,
// для 1 / 4 шага, только на MS1. (полный шаг(0, 0), полушаг(1, 0), шаг 1 / 4 (0, 1) и шаг 1 / 8 (1, 1).
#define motorPinMS1		9	// 9
#define motorPinMS2		A3	// 22

#define MAX_SPEED		255.0

#define BEEPER_MINUS	A0
#define BEEPER_PLUS		A1

GStepper<STEPPER2WIRE> stepper(2040, motorPinSTEP, motorPinDIR, motorPinENABLE); // драйвер step-dir + пин enable
Encoder encoder(encoderCLK, encoderDT, encoderSW, TYPE2);  // для работы c кнопкой и сразу выбираем тип

float speed = MAX_SPEED;		// скорость вращения при "передвижении" влево/вправо
float trackingSpeed = 68.0;	// скорость при "трекинге"
float axeleration = 500.0;	// ускорение при старте и стопе

//enum Modes
//{
//	IDDLE,
//	LEFT,
//	RIGHT,
//	/*TOP,
//	DOWN,*/
//	TRACKING,
//};

//Modes mode = Modes::IDDLE;
//byte isBlink = false;	// флаг моргания светодиодом

void setup()
{
	initPorts();
	initVars();
	initDisplay();

	initTimers();
	initStepper();

	/*
	  // • −−•− ••−−−
	  const double dot = 100;
	  const double dash = dot * 3;
	  const double pause_char = dot; //	пауза между элементами одного знака — одна точка;
	  const double pause_word = dot * 3; //	пауза между знаками в слове — три точки;
	  const double pause_sentence = dot * 7; //	пауза между словами — семь точек.

	  tone(BEEPER_PLUS, 2000, dot);
	  _delay_ms(dot + pause_word);

	  tone(BEEPER_PLUS, 2000, dash);
	  _delay_ms(dash + pause_char);
	  tone(BEEPER_PLUS, 2000, dash);
	  _delay_ms(dash + pause_char);
	  tone(BEEPER_PLUS, 2000, dot);
	  _delay_ms(dot + pause_char);
	  tone(BEEPER_PLUS, 2000, dash);
	  _delay_ms(dash + pause_word);

	  tone(BEEPER_PLUS, 2000, dot);
	  _delay_ms(dot + pause_char);
	  tone(BEEPER_PLUS, 2000, dot);
	  _delay_ms(dot + pause_char);
	  tone(BEEPER_PLUS, 2000, dash);
	  _delay_ms(dash + pause_char);
	  tone(BEEPER_PLUS, 2000, dash);
	  _delay_ms(dash + pause_char);
	  tone(BEEPER_PLUS, 2000, dash);
	  _delay_ms(dash + pause_word);*/

	tone(BEEPER_PLUS, 3000, 150);
	_delay_ms(200);
	tone(BEEPER_PLUS, 3000, 150);
}

void initDisplay() {
	// SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
	if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {	// 0x3D or 0x3C
	  //Serial.println(F("SSD1306 allocation failed"));
		for (;;); // Don't proceed, loop forever
	}

	display.clearDisplay();	// Clear the buffer

	//for (int i = 0; i < display.width(); i++)
	//{
	//	for (int j = 0; j < display.height(); j++)
	//	{
	//		display.drawPixel(i, j, SSD1306_WHITE); // Draw a single pixel in white
	//	}
	//}

	display.ssd1306_command(SSD1306_SETCONTRAST);
	display.ssd1306_command(1);


	display.setTextColor(SSD1306_WHITE); // Draw white text
	//display.cp437(true);         // Use full 256 char 'Code Page 437' font

	/*for (int16_t i = 0; i < 169; i++) {
	  if (i == '\n') display.write(' ');
	  else          display.write(i);
	  }*/

	  /*for (int16_t i = 169; i < 256; i++) {
		if (i == '\n') display.write(' ');
		else          display.write(i);
		}*/

	setTitleText(" EQ2 Tr v2");
	setSpeedText(speed);
	setTrackingText(trackingSpeed);
	setStatusText("  Iddle");
	display.display();
}

void initVars() {
	int eeAddress = 0; //EEPROM address to start reading from
	/*speed = EEPROM.read(eeAddress);
	eeAddress += sizeof(float);*/
	EEPROM.get(eeAddress, trackingSpeed);
	eeAddress += sizeof(float);
	/*axeleration = EEPROM.read(eeAddress);
	eeAddress += sizeof(float);*/
}

void saveVars() {
	int eeAddress = 0; //EEPROM address to start reading from
	/*EEPROM.write(eeAddress, speed);
	eeAddress += sizeof(float);*/
	EEPROM.update(eeAddress, trackingSpeed);
	eeAddress += sizeof(float);
	/*EEPROM.write(eeAddress, axeleration);
	eeAddress += sizeof(float);*/


	display.fillRect(0, 50, display.width(), 20, SSD1306_BLACK);

	display.setTextSize(2);
	display.setCursor(0, 50);

	for (size_t i = 0; i < 10; i++)
	{
		display.write(176);
		display.display();
		_delay_ms(40);
	}

	display.fillRect(0, 50, display.width(), 20, SSD1306_BLACK);

	setStatusText("  SAVED ");
	display.write(2);
	display.display();

	tone(BEEPER_PLUS, 2000, 500);
}

void isrCLK() {
	encoder.tick();
}

void isrDT() {
	encoder.tick();
}

void initPorts() {
	pinMode(LED_BUILTIN, OUTPUT);

	pinMode(BEEPER_MINUS, OUTPUT);
	digitalWrite(BEEPER_MINUS, 0);
	pinMode(BEEPER_PLUS, OUTPUT);

	attachInterrupt(0, isrCLK, CHANGE);
	attachInterrupt(1, isrDT, CHANGE);
}

void initTimers() {
	//timer_init_ISR_500KHz(TIMER_DEFAULT);
	//timer_init_ISR_200KHz(TIMER_DEFAULT);
	//timer_init_ISR_100KHz(TIMER_DEFAULT);
	//timer_init_ISR_50KHz(TIMER_DEFAULT);
	//timer_init_ISR_20KHz(TIMER_DEFAULT);
	//timer_init_ISR_10KHz(TIMER_DEFAULT);
	//timer_init_ISR_5KHz(TIMER_DEFAULT);
	//timer_init_ISR_2KHz(TIMER_DEFAULT);
	//timer_init_ISR_1KHz(TIMER_DEFAULT);
	//timer_init_ISR_500Hz(TIMER_DEFAULT);
	//timer_init_ISR_200Hz(TIMER_DEFAULT);
	//timer_init_ISR_100Hz(TIMER_DEFAULT);
	//timer_init_ISR_50Hz(TIMER_DEFAULT);
	//timer_init_ISR_20Hz(TIMER_DEFAULT);
	//timer_init_ISR_10Hz(TIMER_DEFAULT);
	//timer_init_ISR_5Hz(TIMER_DEFAULT);
	//timer_init_ISR_2Hz(TIMER_DEFAULT);
	//timer_init_ISR_1Hz(TIMER_DEFAULT);

	Timer1.setFrequency(1);
	Timer1.enableISR();
}

void initStepper() {
	stepper.setMaxSpeed(MAX_SPEED);
	stepper.autoPower(true);
	stepper.setAcceleration(axeleration);

	stepper.setMaxSpeed(MAX_SPEED);
	stepper.setSpeed(speed);

	pinMode(motorPinMS1, OUTPUT);
	digitalWrite(motorPinMS1, HIGH);

	pinMode(motorPinMS2, OUTPUT);
	digitalWrite(motorPinMS2, HIGH);
}

ISR(TIMER1_A) {
	//display.setCursor(0, 25);
	//display.print(F("Speed"));
	///*display.print(speed);*/
	//display.display();
	digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

	/*digitalWrite(motorPinMS1, HIGH);
	digitalWrite(motorPinMS2, HIGH);*/
}

void setTitleText(char* text) {
	display.fillRect(0, 0, display.width(), 20, SSD1306_BLACK);
	display.setTextSize(2);
	display.setCursor(0, 0);
	display.println(text);
	display.display();
}

void setSpeedText(float _speed) {
	display.fillRect(0, 20, display.width(), 10, SSD1306_BLACK);
	display.setTextSize(1);
	display.setCursor(0, 20);
	display.print(F("Move speed:  "));
	display.print(_speed, 1);
	display.display();
}

void setTrackingText(float _trackingSpeed) {
	display.fillRect(0, 30, display.width(), 10, SSD1306_BLACK);
	display.setTextSize(1);
	display.setCursor(0, 30);
	display.print(F("Track speed: "));
	display.print(_trackingSpeed, 1);
	display.display();
}

void setStatusText(char* text) {
	display.fillRect(0, 50, display.width(), 20, SSD1306_BLACK);
	display.setTextSize(2);
	display.setCursor(0, 50);
	display.print(text);
	/*display.display();*/
}

byte isMoving, isShowedStop = false;

void SetStepperSpeed(bool isMax) {
	if (isMax) {
		digitalWrite(motorPinMS1, LOW);
		digitalWrite(motorPinMS2, LOW);		
	}
	else {
		digitalWrite(motorPinMS1, HIGH);
		digitalWrite(motorPinMS2, HIGH);
	}
}

void loop()
{
	encoder.tick(); // обязательная функция отработки. Должна постоянно опрашиваться

	if (encoder.isRight()) {		
		stepper.setSpeed(speed);
		SetStepperSpeed(true);
		stepper.setRunMode(KEEP_SPEED);

		setStatusText(" Move ");
		display.write(2);
		display.write((uint8_t)0);
		display.write(175);
		display.display();

		 tone(BEEPER_PLUS, 2000, 100);
	}

	if (encoder.isLeft()) {		
		stepper.setSpeed(-speed);		
		SetStepperSpeed(true);
		stepper.setRunMode(KEEP_SPEED);

		setStatusText(" Move ");
		display.write(2);
		display.write((uint8_t)0);
		display.write(174);
		display.display();

		 tone(BEEPER_PLUS, 2000, 100);
	}

	if (encoder.isRightH())
	{
		if (trackingSpeed < speed) {
			trackingSpeed += 0.5;
		}
		stepper.setSpeed(trackingSpeed);

		setTrackingText(trackingSpeed);
		//display.display();
		tone(BEEPER_PLUS, 2000, 100);
	}

	if (encoder.isLeftH())
	{
		if (trackingSpeed > speed) {
			trackingSpeed -= 0.5;
		}
		stepper.setSpeed(trackingSpeed);

		setTrackingText(trackingSpeed);
		tone(BEEPER_PLUS, 2000, 100);
	}

	//if (enc1.isPress()) Serial.println("Press");         // нажатие на кнопку (+ дебаунс)
	//if (enc1.isRelease()) Serial.println("Release");     // то же самое, что isClick

	if (encoder.isClick())
	{
		tone(BEEPER_PLUS, 2000, 100);
	}

	if (encoder.isSingle())
	{
		if (isMoving) {			
			stepper.stop();			
		}
		else {
			SetStepperSpeed(false);
			stepper.setSpeed(trackingSpeed);
			stepper.setRunMode(KEEP_SPEED);			

			setStatusText("TRACKING ");
			display.write(175);
			display.display();
		}
	}

	if (encoder.isDouble())
	{
		stepper.setRunMode(KEEP_SPEED);
		stepper.setSpeed(-trackingSpeed);

		setStatusText("TRACKING ");
		display.write(174);
		display.display();
	}

	if (encoder.isHolded()) {
		stepper.stop();
		saveVars();
	}

	//if (enc1.isHold()) Serial.println("Hold");         // возвращает состояние кнопки

	isMoving = stepper.tick();

	if (isMoving) {
		isShowedStop = false;
	}
	if (!isMoving && !isShowedStop) {
		isShowedStop = true;
		setStatusText("  STOP ");
		display.write(1);
		display.display();
	}
}
