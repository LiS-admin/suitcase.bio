#include <LiquidCrystal.h>
#include "Adafruit_SHT31.h"
#include <Wire.h>
#include <PID_v1.h>


//Definição de pinos, aqui é definido quais os pinos que vamos usar no nosso arduino quer como inputs(leitura de sinais) quer outputs(alimentação de componentes
Adafruit_SHT31 sht31 = Adafruit_SHT31();

LiquidCrystal lcd(9, 8, 7, 6, 5, 4);

int fanPin = 11; //fan
int heatPin = 12; //Heater
int buttonPin = 13; //pino botao encoder


int encoder = 0; //posicao da rotacao do botao

int buttonState = 0; //estado do botao do selacao

int menuPage = 0; // pagina do menu (0 - inicial, 1 - incubar, 2 - set-point)

volatile int menuPos = 0; //  linha do menu 1 e 2
double Temperature; // temperatura lida do sensor
int Humidity; // humidade lida do sensor
double Setpoint=37; // = 35; // temperatura a atingir
int cursorPos = 4; // posicao inicial do cursor
int flag = 0; // 1 quando esta em aquecer, 0 quando esta a arrefecer
int i = 0; // contador para restringir o numero de leitura de temperatura
int a = -1; // contador para defenir intervalo de tempo - So usado para testes
int old = LOW; // posicao do menu anterior
const int pin_BL = 10; //pin utilizado pelo ecra
double safety;
//Variaveis do Controlador(PID), Variaveis especificas para o controlador. input -> temperatura do sensor, output -> velocidade da ventoinha
double Input, Output;
int SafetyPin = A5; 

//Parametros do Controlador(PID), são estes valores que vao entrar para o calculo do nosso PID. Uma forma de calcular estes valores e' atravez de Ziegler-Nichols. Poderao tambem ser achados empiricamente
// Kp - fator proporcional e' o ganho do sistema
//Ki - aproxima o valor medido do valor imposto
//Kd - "aceleracao" do sistema
double Kp = 200, Ki = 10, Kd = 200;  // Kp = 200, Ki = 10, Kd = 200;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, REVERSE);

//Parametros Encoder, Pinos e variaveis especificas para o funcionamento e leitura do Encoder(botao)quer para a rotação quer para o botao de pressão

static int pinA = 2; //Rotacao direcao 1
static int pinB = 3; //Rotacao direcao 2
volatile byte aFlag = 0; // reset da posicao angular do botao para a direcao 1
volatile byte bFlag = 0; // reset da posicao angular do botao para a direcao 2
volatile byte encoderPos = 0; //Posicao angular do botao
volatile byte oldEncPos = 0; // Posicao angular do botao anterior
volatile byte reading = 0; // Valor da posicao do botao
unsigned long lastDebounceTime = 0; // tempo desde o ultimo destado do botao...a comparar com debouceDelay
unsigned long debounceDelay = 200;  // periodo de latencia entre mudanca de estados do botao
int lastButtonState = LOW; // ultimo estado do botao
//int cooldown; //





//Sub rotina do Controlador de Temperature, definição dos valores e componenetes que sao utilizados pelo PID para funcionamento do mesmo
void pid() {
  

  i++;
  if (i != 20) {
    return;
  }
  i = 0;
 
  Temperature = sht31.readTemperature();
  Humidity = sht31.readHumidity();
  Input = sht31.readTemperature();
  Serial.println(Input);

  double gap = abs(Setpoint - Input); //intervalo entre temperatura imposta e medida

  // bloco de seguranca para nao exceder o setpoint em demasiado...
  if (Temperature >= Setpoint + 1) {

    while (Temperature > Setpoint) {

      Temperature = sht31.readTemperature();
      digitalWrite(fanPin, LOW); //Ligar ventoinha
      digitalWrite(heatPin, LOW); //desligar resistencia
      Serial.println(Temperature);
      delay(2000);
      return;

    }
  }


  //Serial.println(Output);

  if (gap < 2 ) {  //quando estamos perto do ponto, usamos valores conservadores
    myPID.SetTunings(Kp, Ki, Kd);
\
    Serial.print("output");
        Serial.println(Output);

    myPID.Compute();
    analogWrite(fanPin, 260-Output);
    if  (Output < 127) {
      flag = 1;
      if (safety<3.2) digitalWrite(heatPin, flag);
      
    }
    else if (Output > 137) {
      flag = 0;
      digitalWrite(heatPin, flag);
    }
    else if (flag == 1) {
      digitalWrite(heatPin, flag);
    }
    else if (flag == 0) {
      if (safety<3.2) digitalWrite(heatPin, flag);

    }
  }

  else {
    //Longe do setpoint, usar parametros agressivos
    digitalWrite(fanPin, HIGH);
    digitalWrite(heatPin, HIGH);
  }
}

//Sub rotina do Cursor. Serve para colocar o cursor no ecra para a selecção dos menus
void Cursor() {

  if (cursorPos % 2 == 0) {

    menuPos = 0;
    lcd.setCursor(1, 0); //posicionamento na primeira linha, primeiro caracter
    lcd.print(">"); // caracter para mimetizar uma seta
  }
  else {
    menuPos = 1;
    lcd.setCursor(1, 1);
    lcd.print(">");
  }
}

//Sub rotina da selecção de menus para saber em que posição esta o cursos e  quando o botao é pressionado
void selecionar() {

  buttonState = digitalRead(buttonPin); // leitura do botao em cada ciclo (milhares de vezes por segundo)

  if (buttonState == HIGH && menuPos == 0 && menuPage == 0) {

    menuPage = 1; // posicionamento da pagina do menu consonte dodos anteriores e estado do botao

  }

  if (buttonState == HIGH && menuPos == 1 && menuPage == 0) {

    menuPage = 2; // posicionamento da pagina do menu consonte dodos anteriores e estado do

  }

}

//Sub Rotina do encoder, nesta subrotina definimos os parametros do encoder, devido ao funcionamento do mesmo temos que procurar variações no sinal de saida do mesmo para detectar movimentos
// apartir dai vemos para que lado foi rodado e como proceder em cada caso

void PinA() {
  cli(); //Parar os interrupts enquanto obtemos os valores dos pinos
  reading = PIND & 0xC; // Ler os valores e apagar todos os valores que nao sejam do pinA e do pinB
  if (reading == B00001100 && aFlag) {
    encoderPos --; //decrementar a variavel do contador
    cursorPos --;
    if (menuPage == 2) {
      Setpoint = Setpoint - 0.5;
    }
    bFlag = 0; //Fazer reset às flags
    aFlag = 0;
  }
  else if (reading == B00000100) bFlag = 1;
  sei(); //recomeçar os interrupts - funcao especifica do arduino

}

// comentario na funcao de cima
void PinB() {
  cli(); //
  reading = PIND & 0xC;
  if (reading == B00001100 && bFlag) {
    encoderPos ++; //incrementar a variavel do contador
    cursorPos ++;
    if (menuPage == 2) {
      Setpoint = Setpoint + 0.5;
    }
    bFlag = 0;
    aFlag = 0;
  }
  else if (reading == B00001000) aFlag = 1;
  sei();

}

//Periodo de Inicialização, inicializa e declara ao arduino quais os modos das portas que vamos usar
void setup() {

  Serial.begin(9600); //velocidade de ligacao porta serie. So para debug
  lcd.begin(16, 2); //initialização do ecra, definimos o numero de linhas e caracteres do ecra aqui

  analogWrite(fanPin, OUTPUT); //definir a porta da ventoinha como output analogico
  analogWrite(buttonPin, INPUT); // definir a porta do botao como input
  pinMode(heatPin, OUTPUT); //definir a porta da resistencia como um output digital
  pinMode(2, INPUT_PULLUP); // Codigo especifico do arduino para definir o pull up da porta digital
  pinMode(3, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinA), PinA, RISING); //Definir o interrupt da porta do encoder
  attachInterrupt(digitalPinToInterrupt(pinB), PinB, RISING);

  while (!Serial)
    delay(10);

  //Initialização do Sensor, com mensagem para o caso de dar erro
  lcd.print("Teste sensor");
  delay(500);
  lcd.clear();
  if (! sht31.begin(0x44)) {
    lcd.setCursor(0, 0);
    lcd.write("Sensor Error");
    while (1) delay(1);
  }

  //definir os valores do output do PID para nao exder os valores operacionais da ventoinha
  myPID.SetOutputLimits( 102 , 255 ); //102=2V(menor valor de tensao para funcionar a ventoinha) e 255=5V

  myPID.SetMode(AUTOMATIC); //ligar o PID


}
//Loop do programa, é esta a parte do programa que fica a correr continuamente para que o programa funcione, para tal fica a chamar as sub rotinas mencionadas previamente consoante o menu onde esta
void loop() {

  switch (menuPage) {

    case 1: //Menu incubar

      pid();//chamar a rotina do PID
      Temperature = sht31.readTemperature();
      Humidity = sht31.readHumidity();


      //Linhas para uso com a porta serie do arduino, servem apenas para debug
      //      if (a < 0 ) {
      //        Serial.println(Temperature);
      //      }
      //      a++;
      //
      //      if (a == 180) {
      //        Serial.println(Temperature);
      //        a = 0;
      //      }

      //Programação do menu de incubação
      lcd.setCursor(0, 0);
      lcd.print("Tmp00.0oC Hum00%");
      lcd.setCursor(0, 1);
      lcd.print("Setpoint");
      lcd.setCursor(3, 0);
      lcd.print(Temperature,1);
      lcd.setCursor(13, 0);
      lcd.print(Humidity);
      lcd.setCursor(8, 1);
      lcd.print(Setpoint);

 

      //Leitura do botao para saida do menu
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

    case 2: //Menu setpoint

      //Programação do menu de definição de temperatura
      lcd.setCursor(0, 0);
      lcd.print("Setpoint =");
      lcd.setCursor(12, 0);
      lcd.print(Setpoint);

      digitalWrite(fanPin, LOW);
      digitalWrite(heatPin, LOW);

      if (Setpoint > 55) {
        Setpoint = 55;
      }

      if (Setpoint < 15) {
        Setpoint = 15;
      }

      //if (buttonState == HIGH && cooldown == 0 ) {
      if (buttonState == HIGH ) {
        menuPage == 0;

      }

      //Leitura do botao para saida do menu
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

    case 0: //Menu inicial

      //Programação do menu de Entrada
      lcd.setCursor(2, 0);
      lcd.print("Incubar!");
      lcd.setCursor(2, 1);
      lcd.print("Setpoint");

      digitalWrite(fanPin, LOW);
      digitalWrite(heatPin, LOW);
      

      Cursor();
      selecionar();
      old = 0;
      delay(5);
      lcd.clear();

      break;

  }

}
