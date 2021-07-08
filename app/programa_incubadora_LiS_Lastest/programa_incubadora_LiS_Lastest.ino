#include <LiquidCrystal.h>
#include "Adafruit_SHT31.h"
#include <Wire.h>
#include <PID_v1.h>


//Definição de pinos - Aqui são definidos quais os pinos que vamos usar no nosso Arduino quer como inputs (leitura de sinais) quer outputs (alimentação de componentes)
Adafruit_SHT31 sht31 = Adafruit_SHT31();

LiquidCrystal lcd(9, 8, 7, 6, 5, 4);

#define PIN_OUTPUT 11 //Fan
int heatPin = 12; //Heater
int buttonPin = 13; //Pino botão encoder

//Variáveis do programa - Aqui definimos as variáveis para armazenamento de dados do programa
int encoder = 0;
int buttonState = 0;
int menuPage = 0;
volatile int menuPos = 0;
double Temperatura;
int Humidade;
double Setpoint = 37;
int cursorPos = 4;
int flag = 0;
int i = 0;
int old = LOW;
const int pin_BL = 10;
int debug = 1;

//Variáveis do Controlador(PID), Variáveis específicas para o controlador
double Input, Output;

//Parametros do Controlador(PID), são estes valores que são usados no calculo do nosso PID
double Kp = 150, Ki = 0.55, Kd = 15000;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

//Parametros Encoder - Pinos e variáveis específicas para o funcionamento e leitura do Encoder (botão) quer para a rotação quer para o botão de pressão
static int pinA = 2;
static int pinB = 3;
volatile byte aFlag = 0;
volatile byte bFlag = 0;
volatile byte encoderPos = 0;
volatile byte oldEncPos = 0;
volatile byte reading = 0;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 200;
int lastButtonState = LOW;
int cooldown;
int reading1;


//Período de Inicialização - Inicializa e declara ao Arduino quais os modos das portas que vamos usar
void setup() {

  Serial.begin(9600);
  lcd.begin(16, 2);

  analogWrite(PIN_OUTPUT, OUTPUT);
  analogWrite(buttonPin, Input);
  pinMode(heatPin, OUTPUT);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinA), PinA, RISING);
  attachInterrupt(digitalPinToInterrupt(pinB), PinB, RISING);

  while (!Serial)
    delay(10);

  lcd.print("Teste sensor");
  delay(500);
  lcd.clear();
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    lcd.setCursor(0, 0);
    lcd.write("Sensor Error");
    Serial.println("ler");
    //    lcd.clear();
    while (1) delay(1);
  }

  myPID.SetOutputLimits( 102 , 255 ); //102=2V(menor valor de tensao para funcionar a ventoinha) e 255=5V

  myPID.SetMode(AUTOMATIC); //turn the PID on


}

//Sub rotina do Controlador de temperatura - Definição dos valores e componenetes que são utilizados pelo PID para funcionamento do mesmo
void pid() {

  i++;
  if (i != 100) {
    return;
  }
  i = 0;
  Temperatura = sht31.readTemperature();
  Humidade = sht31.readHumidity();
  Input = sht31.readTemperature();

  double gap = abs(Setpoint - Input); //distance away from setpoint

}

  if (gap < 3 ) {  //we're close to setpoint, use conservative tuning parameters
    myPID.SetTunings(Kp, Ki, Kd);

    myPID.Compute();

    if  (Output < 127) {
      flag = 0;
      digitalWrite(heatPin, flag);
    }
    else if (Output > 179) {
      flag = 1;
      digitalWrite(heatPin, flag);
    }
    else if (flag == 1) {
      digitalWrite(heatPin, flag);
    }
    else if (flag == 0) {
      digitalWrite(heatPin, flag);

      //      Serial.println(Temperatura);
    }
  }

  else {
    //Longe do setpoint, usar parametros agressivos
    digitalWrite(11, HIGH);
    digitalWrite(12, HIGH);
  }
}

//Sub rotina do Cursor - Serve para o Arduino poder colocar o cursor no ecrã e saber onde está o mesmo para a selecção dos menus
void Cursor() {

  if (cursorPos % 2 == 0) {

    menuPos = 0;
    lcd.setCursor(1, 0);
    lcd.print(">");
  }

  else {

    menuPos = 1;
    lcd.setCursor(1, 1);
    lcd.print(">");
  }


}

//Sub rotina da selecção de menus - Aqui o Arduino procura saber em que posição está o cursor quando o botão é pressionado para que seja escolhido o menu correspondente à posição do mesmo
void selecionar() {

  buttonState = digitalRead(buttonPin);

  if (buttonState == HIGH && menuPos == 0 && menuPage == 0) {

    menuPage = 1;

  }

  if (buttonState == HIGH && menuPos == 1 && menuPage == 0) {

    menuPage = 2;

  }

}

//Sub Rotina do encoder - Nesta subrotina definimos os parametros do encoder. Devido ao funcionamento do mesmo, temos que procurar variações no sinal de saída do mesmo para detectar movimentos
// A partir daí vemos para que lado foi rodado e como proceder em cada caso
void PinA() {
  cli(); //stop interrupts happening before we read pin values
  reading = PIND & 0xC; // read all eight pin values then strip away all but pinA and pinB's values
  if (reading == B00001100 && aFlag) { //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
    encoderPos --; //decrement the encoder's position count
    cursorPos --;
    if (menuPage == 2) {
      Setpoint = Setpoint - 0.5;
    }
    bFlag = 0; //reset flags for the next turn
    aFlag = 0; //reset flags for the next turn
  }
  else if (reading == B00000100) bFlag = 1; //signal that we're expecting pinB to signal the transition to detent from free rotation
  sei(); //restart interrupts

}

void PinB() {
  cli(); //stop interrupts happening before we read pin values
  reading = PIND & 0xC; //read all eight pin values then strip away all but pinA and pinB's values
  if (reading == B00001100 && bFlag) { //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
    encoderPos ++; //increment the encoder's position count
    cursorPos ++;
    if (menuPage == 2) {
      Setpoint = Setpoint + 0.5;
    }
    bFlag = 0; //reset flags for the next turn
    aFlag = 0; //reset flags for the next turn
  }
  else if (reading == B00001000) aFlag = 1; //signal that we're expecting pinA to signal the transition to detent from free rotation
  sei(); //restart interrupts

}

//Loop do programa - Parte do programa que fica a correr continuamente para que o programa funcione. Para tal, chama as sub rotinas mencionadas previamente consoante o menu seleccionado
void loop() {

  switch (menuPage) {

    case 1:


      pid();


      Temperatura = sht31.readTemperature();
      Humidade = sht31.readHumidity();
      Serial.print("temp:");
      Serial.println(Temperatura);


      lcd.setCursor(0, 0);
      lcd.print("Tmp00.00C/Hum00%");
      lcd.setCursor(0, 1);
      lcd.print("Setpoint");
      lcd.setCursor(3, 0);
      lcd.print(Temperatura);
      lcd.setCursor(13, 0);
      lcd.print(Humidade);
      lcd.setCursor(8, 1);
      lcd.print(Setpoint);

      buttonState = digitalRead(buttonPin);

      if (buttonState == HIGH) {
        while (digitalRead(buttonPin) == HIGH) {
          delay(1);
        }

        if (old == 0) {
          menuPage = 1;
          old = 1;
        }
        else {
          menuPage = 0;
        }
      }
      break;

    case 2:

      lcd.setCursor(0, 0);
      lcd.print("Setpoint =");
      lcd.setCursor(12, 0);
      lcd.print(Setpoint);

      if (Setpoint > 40) {
        Setpoint = 40;
      }

      if (Setpoint < 25) {
        Setpoint = 25;
      }

      if (buttonState == HIGH && cooldown == 0 ) {

        menuPage == 0;

      }

      buttonState = digitalRead(buttonPin);

      if (buttonState == HIGH) {
        while (digitalRead(buttonPin) == HIGH) {
          delay(1);
        }

        if (old == 0) {
          menuPage = 2;
          old = 2;
        }
        else {
          menuPage = 0;
        }
      }
      break;

    case 0:


      lcd.setCursor(2, 0);
      lcd.print("Incubar!");
      lcd.setCursor(2, 1);
      lcd.print("Setpoint");

      cooldown = 0;

      Cursor();
      selecionar();
      old = 0;
      delay(50);
      lcd.clear();

      break;

  }

}
