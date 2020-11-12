//#include "timer-api.h"
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
#define motorPin1 6 // 28BYJ48 pin 1
#define motorPin2 7	// 28BYJ48 pin 2
#define motorPin3 8	// 28BYJ48 pin 3
#define motorPin4 9	// 28BYJ48 pin 4
#define MAX_SPEED   1000.0

int counter = 0;
int currentStateCLK;
int lastStateCLK;
String currentDir = "";

GStepper<STEPPER4WIRE> stepper(2048, motorPin4, motorPin2, motorPin3, motorPin1); // мотор с драйвером ULN2003 подключается по порядку пинов, но крайние нужно поменять местами
Encoder encoder(encoderCLK, encoderDT, encoderSW, TYPE2);  // для работы c кнопкой и сразу выбираем тип

float speed = 200.0;	// скорость вращения при "передвижении" влево/вправо
float trackingSpeed = 50.0;	// скорость при "трекинге"
float axeleration = 100.0;	// ускорение при старте и стопе

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
byte isBlink = false;	// флаг моргания светодиодом

void setup()
{
	initVars();
	initDisplay();
	initButtons();
	initTimers();
	initStepper();
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

	display.setTextColor(SSD1306_WHITE); // Draw white text
	display.cp437(true);         // Use full 256 char 'Code Page 437' font

	/*for (int16_t i = 0; i < 169; i++) {
		if (i == '\n') display.write(' ');
		else          display.write(i);
	}*/

	/*for (int16_t i = 169; i < 256; i++) {
		if (i == '\n') display.write(' ');
		else          display.write(i);
	}*/

	setTitleText(" EQ2 Tr v0");
	setSpeedText(speed);
	setTrackingText(trackingSpeed);
	setStatusText("  Iddle");
	display.display();
}

void initVars() {
	int eeAddress = 0; //EEPROM address to start reading from
	speed = EEPROM.read(eeAddress);
	eeAddress += sizeof(float);
	trackingSpeed = EEPROM.read(eeAddress);
	eeAddress += sizeof(float);
	axeleration = EEPROM.read(eeAddress);
	eeAddress += sizeof(float);
}

void saveVars() {
	int eeAddress = 0; //EEPROM address to start reading from
	EEPROM.write(eeAddress, speed);
	eeAddress += sizeof(float);
	EEPROM.write(eeAddress, trackingSpeed);
	eeAddress += sizeof(float);
	EEPROM.write(eeAddress, axeleration);
	eeAddress += sizeof(float);

	
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
}

void isrCLK() {
	encoder.tick();
}

void isrDT() {
	encoder.tick();
}

void initButtons() {
	pinMode(LED_BUILTIN, OUTPUT);
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

ISR(TIMER1_A) {
	//display.setCursor(0, 25);
	//display.print(F("Speed"));
	///*display.print(speed);*/
	//display.display();
	digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void initStepper() {
	stepper.setMaxSpeed(MAX_SPEED);
	stepper.autoPower(true);
	stepper.setAcceleration(axeleration);

	stepper.setMaxSpeed(MAX_SPEED);
	stepper.setSpeed(speed);
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

void loop()
{
	encoder.tick(); // обязательная функция отработки. Должна постоянно опрашиваться

	if (encoder.isTurn()) {     // если был совершён поворот (индикатор поворота в любую сторону)
		isBlink = true;
	}

	if (encoder.isRight()) {
		stepper.setRunMode(FOLLOW_POS);
		stepper.setMaxSpeed(speed);
		stepper.setTarget(30, RELATIVE);

		setStatusText(" Move ");
		display.write(2);
		display.write((uint8_t)0);
		display.write(175);
		display.display();
	}

	if (encoder.isLeft()) {
		stepper.setRunMode(FOLLOW_POS);
		stepper.setMaxSpeed(speed);
		stepper.setTarget(-30, RELATIVE);

		setStatusText(" Move ");
		display.write(2);
		display.write((uint8_t)0);
		display.write(174);
		display.display();
	}

	if (encoder.isRightH())
	{
		trackingSpeed++;
		stepper.setSpeed(trackingSpeed);

		setTrackingText(trackingSpeed);
		display.display();
	}

	if (encoder.isLeftH())
	{
		trackingSpeed--;
		stepper.setSpeed(trackingSpeed);

		setTrackingText(trackingSpeed);
	}

	//if (enc1.isPress()) Serial.println("Press");         // нажатие на кнопку (+ дебаунс)
	//if (enc1.isRelease()) Serial.println("Release");     // то же самое, что isClick

	if (encoder.isClick())
	{
		/*if (!isMoving) {
			stepper.setRunMode(FOLLOW_POS);
			stepper.setMaxSpeed(speed);
			stepper.setTarget(30, RELATIVE);
		}*/
	}

	if (encoder.isSingle())
	{
		if (isMoving) {
			stepper.stop();
		}
		else {
			stepper.setRunMode(KEEP_SPEED);
			stepper.setSpeed(trackingSpeed);

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
