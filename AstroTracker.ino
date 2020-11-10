#include "timer-api.h"
#include "GyverEncoder.h"
#include <GyverStepper.h>
#include <EEPROM.h>

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

float axeleration = 100.0;	// ускорение при старте и стопе
float speed = 200.0;	// скорость вращения при "передвижении" влево/вправо
float trackingSpeed = 50.0;	// скорость при "трекинге"

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
	Serial.println("Init...");

	Serial.begin(115200);

	initVars();
	initButtons();
	initTimers();
	initStepper();
}

void initVars() {
	int eeAddress = 0; //EEPROM address to start reading from
	axeleration = EEPROM.read(eeAddress);
	eeAddress += sizeof(float);
	speed = EEPROM.read(eeAddress);
	eeAddress += sizeof(float);
	trackingSpeed = EEPROM.read(eeAddress);
	eeAddress += sizeof(float);

	Serial.print("axeleration = ");
	Serial.println(axeleration);
	Serial.print("speed = ");
	Serial.println(speed);
	Serial.print("trackingSpeed = ");
	Serial.println(trackingSpeed);
}

void saveVars() {
	int eeAddress = 0; //EEPROM address to start reading from
	EEPROM.write(eeAddress, axeleration);
	eeAddress += sizeof(float);
	EEPROM.write(eeAddress, speed);
	eeAddress += sizeof(float);
	EEPROM.write(eeAddress, trackingSpeed);
	eeAddress += sizeof(float);

	Serial.print("axeleration = ");
	Serial.println(axeleration);
	Serial.print("speed = ");
	Serial.println(speed);
	Serial.print("trackingSpeed = ");
	Serial.println(trackingSpeed);
	Serial.println("Saved!");
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
	timer_init_ISR_10Hz(TIMER_DEFAULT);
	//timer_init_ISR_5Hz(TIMER_DEFAULT);
	//timer_init_ISR_2Hz(TIMER_DEFAULT);
	//timer_init_ISR_1Hz(TIMER_DEFAULT);	
}

void initStepper() {
	stepper.setMaxSpeed(MAX_SPEED);
	stepper.setAcceleration(axeleration);
	stepper.setSpeed(speed);
}

void timer_handle_interrupts(int timer) {
	static int blinkDivider = 5;	// дополнильный множитель периода

	static unsigned long prev_time = micros();

	if (isBlink) {
		digitalWrite(LED_BUILTIN, HIGH);

		if (--blinkDivider < 1) {
			blinkDivider = 5;

			digitalWrite(LED_BUILTIN, LOW);
			isBlink = false;

			Serial.println(trackingSpeed);
		}
	}
}

void loop()
{
	encoder.tick(); // обязательная функция отработки. Должна постоянно опрашиваться
	if (encoder.isTurn()) {     // если был совершён поворот (индикатор поворота в любую сторону)
		isBlink = true;
	}

	if (encoder.isRight()) {
		Serial.println("Right");         // если был поворот
		
		/*stepper.stop();
		stepper.setRunMode(FOLLOW_POS);*/
		stepper.setSpeed(speed);
		stepper.setTarget(25, RELATIVE);
	}
	if (encoder.isLeft()) {
		Serial.println("Left");
		
		/*stepper.stop();
		stepper.setRunMode(FOLLOW_POS);*/
		stepper.setSpeed(speed);
		stepper.setTarget(-25, RELATIVE);
	}

	if (encoder.isRightH())
	{
		Serial.println("Right holded"); // если было удержание + поворот

		trackingSpeed++;
	}
	if (encoder.isLeftH())
	{
		Serial.println("Left holded");

		trackingSpeed--;
	}

	//if (enc1.isPress()) Serial.println("Press");         // нажатие на кнопку (+ дебаунс)
	//if (enc1.isRelease()) Serial.println("Release");     // то же самое, что isClick

	if (encoder.isClick())
	{
		Serial.println("Click");         // одиночный клик		
	}
	if (encoder.isSingle())
	{
		Serial.println("Single");       // одиночный клик (с таймаутом для двойного)

		stepper.setRunMode(KEEP_SPEED);
		stepper.setSpeed(trackingSpeed);
	}
	if (encoder.isDouble())
	{
		Serial.println("Double");       // двойной клик

		stepper.setRunMode(KEEP_SPEED);
		stepper.setSpeed(-trackingSpeed);
	}


	if (encoder.isHolded()) {
		Serial.println("Holded");       // если была удержана и энк не поворачивался
		saveVars();
	}
	//if (enc1.isHold()) Serial.println("Hold");         // возвращает состояние кнопки

	stepper.tick();
}
