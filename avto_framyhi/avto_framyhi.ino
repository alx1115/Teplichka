
/* 
 *  
 * A0 - 
 * A1 - 
 * A2 - 
 * A3 - Кнопка - GND
 * A4 - SDA (Екранчик)
 * A5 - SCL (Екранчик)
 * A6 - Кнопка - GND
 * A7 - Кнопка - GND
 * 
 * 
 * D2 - Детектор Zero
 * D3 - 1 Диммирующий
 * D4 - 2 Диммирующий
 * D5 - 
 * D6 - Trig - 2.2kOm - Eho  (Ехолокатор)
 * D7 - 
 * D8 - Реле фрамуги
 * D9 - Реле фрамуги
 * D10 - Реле полива
 * D11 - Реле набора
 * D12 - Центральная нога DS18B20 - 4.7kOm - 5V
 * 
 * D13 -
 * 
 */

#include <EEPROM.h>
#include <GyverTimers.h>
#include <Wire.h>                    //
#include <LCD_1602_RUS.h>            // Екранчик Lcd 16x2
LCD_1602_RUS lcd(0x27, 16, 2);       //

#include <OneWire.h>                 //
#include <DallasTemperature.h>       //
#define ow 12                        // Датчик DS18B20
OneWire oneWire(ow);                 // 
DallasTemperature sensors(&oneWire); //

#define keyDown    A7                //
#define keySelect  A6                // Кнопки
#define keyUp      A3                //

#define rele1 8                      // Фрамуги
#define rele2 9                      // Фрамуги

#define relePump 10                  // Насос
#define releEho 11                   // Реле набора

#define PIN_ECHO 6                   // Пин рывня води

#define zeroPin 2                    // пин детектора нуля
const byte dimPins[2]={3, 4};// пины диммеров

int dimmer[2];               // переменная диммера

bool operatingMode = 1, checkMenu = 1, traffic = 1, flagManual = 1, flagWater = 1;
byte menuMod = 1, jobMod = 0;
int klikTime = 300, loopTime = 3000;                   
int timeCheckTemp = 1000*30;         // Время между измирениями температури в MS
int temp, lastTemp = 0, minTemp, maxTemp, setTemp, runTime, stopTime, nowWater, needWater;
unsigned long int millisCheckTemp = millis(), millisKey=millis(), millisTurn = millis(), currentMillis, millisWater=millis();

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  EEPROM.get(10,operatingMode);
  EEPROM.get(20, setTemp);
  EEPROM.get(30, stopTime);
  EEPROM.get(40, runTime);
  EEPROM.get(50, maxTemp);
  EEPROM.get(60, minTemp);
  EEPROM.get(70, dimmer[0]);
  EEPROM.get(80, dimmer[1]);
  EEPROM.get(90, needWater);

  pinMode(zeroPin, INPUT_PULLUP);
  for (byte i = 0; i < 2; i++) {pinMode(dimPins[i], OUTPUT);}
  
  Timer1.enableISR();
  Timer2.enableISR();
  
  pinMode(ow,INPUT); 
  sensors.setWaitForConversion(false);
  sensors.setResolution(0, 12);
  temp = (int)sensors.getTempCByIndex(0);

  pinMode( keyUp, INPUT_PULLUP);                     
  digitalWrite(keyUp, HIGH);                  
  pinMode(keySelect, INPUT_PULLUP);                  
  digitalWrite(keySelect, HIGH);              
  pinMode(keyDown, INPUT_PULLUP);                    
  digitalWrite(keyDown, HIGH);  
  
  pinMode(rele1,OUTPUT);
  digitalWrite(rele1,1);
  pinMode(rele2,OUTPUT);
  digitalWrite(rele2,1);
  pinMode(releEho,OUTPUT);
  digitalWrite(releEho,flagWater);
  pinMode(relePump,OUTPUT);
  digitalWrite(relePump, 1);
  
  lcd.init();                                 
  lcd.backlight();                            
  lcd.clear(); 
}
// прерывание таймера
ISR(TIMER1_A) {
  digitalWrite(dimPins[1], 1);  // включаем симистор
  Timer1.stop();                // останавливаем таймер
  digitalWrite(dimPins[1], 0);  // виключаем симистор
}
ISR(TIMER2_A) {
  digitalWrite(dimPins[0], 1);  // включаем симистор
  Timer2.stop();                // останавливаем таймер
  digitalWrite(dimPins[0], 0);  // виключаем симистор
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void isr() {
  static int lastDim[2];
  if (lastDim[0] != dimmer[0] && dimmer[0]!=0) Timer2.setPeriod(map(lastDim[0] = dimmer[0],0 , 100, 9500, 500));
  else if(dimmer[0]!=0) Timer2.restart();
  if (lastDim[1] != dimmer[1] && dimmer[1]!=0) Timer1.setPeriod(map(lastDim[1] = dimmer[1],0 , 100, 9500, 500));
  else if (dimmer[1]!=0) Timer1.restart();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void menu1(){                               //Меню температури
  //0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5//
  ///////////////////////////////////
  //s e t { 8 8 } M A X   8 8   A B//
  //n o w <-8 8-> M I N   8 8      //
  ///////////////////////////////////
  lcd.setCursor(0,0);lcd.print("set{");
  lcd.setCursor(6,0);lcd.print("}");
  lcd.setCursor(0,1);lcd.print("now ");
  lcd.setCursor(6,1);lcd.print(" ");
  lcd.setCursor(8,0);lcd.print("MAX");
  lcd.setCursor(8,1);lcd.print("MIN");
  lcd.setCursor(4,1);lcd.print(temp);
  lcd.setCursor(4,0);lcd.print(setTemp);
  lcd.setCursor(11,0);lcd.print(maxTemp);
  lcd.setCursor(11,1);lcd.print(minTemp);
  if(operatingMode == 1){
    lcd.setCursor(14,0);lcd.print(L"АВ");
  }else{
    lcd.setCursor(14,0);lcd.print(L"РЧ");
  }
}
void menu2(){                               //Меню води
  //0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5//
  ///////////////////////////////////
  //Т р е б а   н а б р   1 0 0   |//
  //З а р а з   в о д и   1 0 0   |//
  ///////////////////////////////////
  lcd.setCursor(0,  0);lcd.print(L"Треба набр");
  lcd.setCursor(0,  1);lcd.print(L"Зараз води");
  lcd.setCursor(11,  0);lcd.print(needWater);
  lcd.setCursor(11,  1);lcd.print(nowWater);
}
void menu3(){                               //Меню вентиляторів
  //0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5//
  ///////////////////////////////////
  //В е н т и л я т о р 1   1 0 0 %//
  //В е н т и л я т о р 2   1 0 0 %//
  ///////////////////////////////////
  lcd.clear();
  lcd.setCursor(0,  0);lcd.print(L"Вент. 1    % об.");
  lcd.setCursor(8,  0);lcd.print(dimmer[0]);
  lcd.setCursor(0,  1);lcd.print(L"Вент. 2    % об.");
  lcd.setCursor(8,  1);lcd.print(dimmer[1]);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void checkTemp (){
  if (millis()-millisCheckTemp>=timeCheckTemp){
    checkMenu = 1;
    millisCheckTemp = millis();
    sensors.setWaitForConversion(false); 
    sensors.requestTemperatures();
    temp = (int)sensors.getTempCByIndex(0);
    if (lastTemp != temp && menuMod == 1){
      lcd.setCursor(4 ,1);lcd.print(temp);
      if(temp<=9){   lcd.setCursor(5,1);lcd.print(L" ");}
      if (temp > maxTemp){
        maxTemp = temp;
        EEPROM.put(50,maxTemp);
        if (menuMod == 1){
          lcd.setCursor(11 ,0);lcd.print(maxTemp);
          if (maxTemp < 10) {
            lcd.setCursor(12 ,0);lcd.print(" ");
          }
        }
      }
      if (temp<minTemp && temp!=-127){
        minTemp = temp;
        EEPROM.put(60,minTemp);
        if (menuMod == 1){
          lcd.setCursor(11 ,1);lcd.print(minTemp);
          if (minTemp < 10) {
            lcd.setCursor(12 ,1);lcd.print(" ");
          }
        }
      }
    }
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void menu(){
  if(analogRead(keyUp)   < 100 && millis()-millisKey>=klikTime) {menuMod++; millisKey = millis();checkMenu = 1;}
  if(analogRead(keyDown) < 100 && millis()-millisKey>=klikTime) {menuMod--; millisKey = millis();checkMenu = 1;}
  if(menuMod>3)menuMod=1;else if (menuMod<1)menuMod=3;
  if (checkMenu){
    lcd.clear();
    if(menuMod == 1){                       //Меню температури
      menu1();
      checkMenu = 0;
    }else if(menuMod == 2){                 //Меню води
      menu2();
      checkMenu = 0;
    }else if(menuMod == 3){                 //Меню вентиляторів
      menu3();
      checkMenu = 0;
    }
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void settings(){
  if (analogRead(keySelect)< 100) { 
    delay(300);
    checkMenu = 1;
    jobMod = 0;workRele();
    flagManual = 0;
    if (menuMod == 1){                      //Налаштування температури
      lcd.clear();lcd.setCursor(0, 0);lcd.print("SET ");lcd.print(setTemp);
      currentMillis = millisKey =  millis();
      while(millis()-currentMillis<=loopTime){
        if(millis()-millisKey>=klikTime){
          if(analogRead(keyUp)    < 100){lcd.setCursor(4, 0);lcd.print(setTemp);setTemp++;currentMillis = millis(); millisKey =  millis();}
          else if(analogRead(keyDown)  < 100){lcd.setCursor(4, 0);lcd.print(setTemp);setTemp--;currentMillis = millis(); millisKey =  millis();}
          else if(analogRead(keySelect) < 1 ){break;}
        }
      }
      EEPROM.put(20,setTemp);
      delay(300);
      currentMillis = millis();
      millisKey =  millis();
      lcd.setCursor(6, 0); lcd.print(L"СБРОС");
      while(millis()-currentMillis<=loopTime)
      {
        if(millis()-millisKey>=klikTime){
          if((analogRead(keyUp)< 100 || analogRead(keyDown) < 100)){maxTemp=temp;minTemp=temp;lcd.setCursor(6, 0);lcd.print(L" ОК  ");break;}
          else if(analogRead(keySelect)< 100){break;}
        }
      }
      EEPROM.put(50,temp);
      EEPROM.put(60,temp);
      delay(300);
      currentMillis = millis();
      while(millis()-currentMillis<=loopTime)
      {
        if(millis()-millisKey>=klikTime){
          if((analogRead(keyUp)< 100 || analogRead(keyDown) < 100) ){
            operatingMode=!operatingMode;
            currentMillis = millis(); millisKey =  millis();
            if(!operatingMode) flagManual = 1; 
          }
          else if(analogRead(keySelect) < 1){millisKey =  millis();break;}
        }
        if(operatingMode){ lcd.setCursor(12, 0); lcd.print(L"ABTO"); }else{ lcd.setCursor(12, 0); lcd.print(L"РУЧН"); }
        if (flagManual){
          lcd.clear();
          lcd.setCursor(0,1);lcd.print("now ");
          lcd.setCursor(8, 0);lcd.print(L"Ручний");lcd.setCursor(8, 1);lcd.print(L"режим");millisKey =  millis();lcd.setCursor(4,1);lcd.print(temp);
        }
        while(!operatingMode && flagManual){
          checkZero();
          checkTemp();
          if(millis()-millisKey>=klikTime){
            if(analogRead(keyUp)       < 100){millisKey =  millis();if(jobMod == 0){jobMod=1;}else{jobMod=0;}workRele();lcd.setCursor(4,1);lcd.print(temp);}
            else if(analogRead(keyDown)< 100){millisKey =  millis();if(jobMod == 0){jobMod=2;}else{jobMod=0;}workRele();lcd.setCursor(4,1);lcd.print(temp);}
            else if(analogRead(keySelect) < 1){millisKey =  millis();currentMillis = millis();


lcd.clear();lcd.setCursor(0, 0);lcd.print("SET ");lcd.print(setTemp);lcd.setCursor(6, 0); lcd.print(L"СБРОС");lcd.setCursor(12, 0); lcd.print(L"РУЧН");
              delay(500);
              flagManual = 0;
              jobMod = 0;workRele();
              break;
            } 
          }       
        }
      }
      EEPROM.put(10,operatingMode);
      lcd.setCursor(0, 1);lcd.print(L"stop");
      lcd.setCursor(4, 1); lcd.print(stopTime);
      currentMillis = millis();
      while(millis()-currentMillis<=loopTime)
      {
        if(millis()-millisKey>=klikTime){
          if(analogRead(keyUp)    < 100){stopTime++;currentMillis = millis();millisKey=millis();if (stopTime>=32){stopTime=32;}lcd.setCursor(4, 1); lcd.print(stopTime);}
          else if(analogRead(keyDown)    < 100){
            stopTime--;
            currentMillis = millis();millisKey=millis();millisKey=millis();
            if (stopTime<=9){lcd.setCursor(5, 1); lcd.print(L" ");}
            if (stopTime<=0)stopTime=0;lcd.setCursor(4, 1); lcd.print(stopTime);
          }
          else if(analogRead(keySelect) < 1 ){break;}
        }
      }
      EEPROM.put(30,stopTime);
      delay(300);
      lcd.setCursor(11, 1);lcd.print("run");lcd.print(runTime);
      currentMillis = millis();
      while(millis()-currentMillis<=loopTime)
      {
         if(millis()-millisKey>=klikTime){
          if(analogRead(keyUp)    < 100){runTime++;currentMillis = millis();millisKey=millis();if (runTime>=32){runTime=32;}lcd.setCursor(14, 1); lcd.print(runTime);}
          else if(analogRead(keyDown)    < 100){
            runTime--;
            currentMillis = millis();millisKey=millis();millisKey=millis();
            if (runTime<=9){lcd.setCursor(15, 1); lcd.print(L" ");}
            if (runTime<=0)runTime=0;lcd.setCursor(14, 1); lcd.print(runTime);
          }
          else if(analogRead(keySelect) < 1 ){break;}
        }
      }
      EEPROM.put(40,runTime);
    }else 
    if(menuMod == 2){                       //Налаштування води
   /////0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5//
   //////////////////////////////////////
   //0//Т р е б а   н а б р   1 0 0    //
   //1//        Н а б о р   O N        //
   //////////////////////////////////////
      currentMillis = millis();
      lcd.clear();
      lcd.setCursor(0,  0);lcd.print(L"Треба набр");
      lcd.setCursor(11,  0);lcd.print(needWater);
      while(millis()-currentMillis<=loopTime)
       {
        checkZero();
        if(millis()-millisKey>=klikTime){
          if (analogRead(keyDown)   < 100 ) { needWater--;currentMillis = millis();millisKey=millis();if (needWater<0){needWater=0;} lcd.setCursor(11,0);lcd.print(needWater);
            if(dimmer[0]==99){lcd.setCursor(13,  0);lcd.print(" ");}if(dimmer[0]==9){lcd.setCursor(12,  0);lcd.print("  ");}} 
          else if (analogRead(keyUp)< 100 ) { needWater++;currentMillis = millis();millisKey=millis();if (needWater>100){needWater=100;} lcd.setCursor(11,0);lcd.print(needWater);}
          else if (analogRead(keySelect)< 100) { millisKey=millis();break; }
        } 
       }
       EEPROM.put(90,needWater);
       lcd.setCursor(4,  1);lcd.print(L"Набор ");
       if (!flagWater){lcd.print(L"ON ");}else{lcd.print(L"OFF");}
       while(millis()-currentMillis<=loopTime)
       {
        checkZero();
        if(millis()-millisKey>=klikTime){
          if (analogRead(keyDown)   < 100 || analogRead(keyUp)< 100) {flagWater=!flagWater;
            lcd.setCursor(10,  1);
            if (!flagWater){lcd.print(L"ON ");}else{lcd.print(L"OFF");}
            }
          else if (analogRead(keySelect)< 100) { millisKey=millis();break; }
        } 
       }
    }else 
    if(menuMod == 3){                       //Налаштування вентиляляторів
      currentMillis = millis();
      lcd.clear();
      lcd.setCursor(0, 0);lcd.print(L"Вент. 1    % об.");
      lcd.setCursor(8, 0);lcd.print(dimmer[0]);
        while(millis()-currentMillis<=loopTime)
       {
        checkZero();
        if(millis()-millisKey>=klikTime){
          if (analogRead(keyDown) < 100 ) { dimmer[0]--;currentMillis = millis();millisKey=millis();if (dimmer[0]<0){dimmer[0]=0;} lcd.setCursor(8, 0);lcd.print(dimmer[0]);
          if(dimmer[0]<100){lcd.setCursor(10,  0);lcd.print(" ");}if(dimmer[0]<10){lcd.setCursor(9,  0);lcd.print("  ");}} 
          else if (analogRead(keyUp)   < 100 ) { dimmer[0]++;currentMillis = millis();millisKey=millis();if (dimmer[0]>100){dimmer[0]=100;} lcd.setCursor(8, 0);lcd.print(dimmer[0]);
          if(dimmer[0]<100){lcd.setCursor(10,  0);lcd.print(" ");}if(dimmer[0]<10){lcd.setCursor(9,  0);lcd.print("  ");}}
          else if (analogRead(keySelect)< 100) { millisKey=millis();break; }
        } 
        
       }
       EEPROM.put(70,dimmer[0]);
       delay(300);
       lcd.clear();
       currentMillis = millis();
       lcd.setCursor(0, 1);lcd.print(L"Вент. 2    % об.");
       lcd.setCursor(8, 1);lcd.print(dimmer[1]);
       while(millis()-currentMillis<=loopTime)
       {
        checkZero();
        if(millis()-millisKey>=klikTime){
          if (analogRead(keyDown) < 100 ) { dimmer[1]--;currentMillis = millis();millisKey=millis();if (dimmer[1]<0){dimmer[1]=0;} lcd.setCursor(8, 1);lcd.print(dimmer[1]);
          if(dimmer[1]<100){lcd.setCursor(10,  1);lcd.print(" ");}if(dimmer[1]<10){lcd.setCursor(9,  1);lcd.print("  ");}} 
          else if (analogRead(keyUp)   < 100 ) { dimmer[1]++;currentMillis = millis();millisKey=millis();if (dimmer[1]>100){dimmer[1]=100;} lcd.setCursor(8, 1);lcd.print(dimmer[1]);
          if(dimmer[1]<100){lcd.setCursor(10,  1);lcd.print(" ");}if(dimmer[1]<10){lcd.setCursor(9,  1);lcd.print("  ");}}
          else if (analogRead(keySelect)< 100) { millisKey=millis();break; }
        } 
       }
       EEPROM.put(80,dimmer[1]);
    }
    delay(300);


}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void avto(){
  if (operatingMode == 1){
    if(millis()-millisTurn >= stopTime*1000 && traffic == 0 && stopTime!=0){
      millisTurn = millis();
      if (setTemp>temp){
        traffic = 1;
        jobMod = 1;
        workRele();
      }else if (setTemp<temp){
        traffic = 1;
        jobMod = 2;
        workRele();
      }
    }else if(millis()-millisTurn >= runTime*100 && traffic == 1 && runTime!=0){
      millisTurn = millis();
      traffic = 0;
      jobMod = 0;
      workRele();
    }
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void workRele(){
if(jobMod == 0){digitalWrite(rele2, 1);digitalWrite(rele1, 1);if(menuMod == 1){lcd.setCursor(3,1);lcd.print(L" ");   lcd.setCursor(6,1);lcd.print(L" ");}}
if(jobMod == 2){digitalWrite(rele2, 0);digitalWrite(rele1, 1);if(menuMod == 1){lcd.setCursor(3,1);lcd.print(L"\x7F");lcd.setCursor(6,1);lcd.print(L" ");}}
if(jobMod == 1){digitalWrite(rele2, 1);digitalWrite(rele1, 0);if(menuMod == 1){lcd.setCursor(3,1);lcd.print(L" ");   lcd.setCursor(6,1);lcd.print(L"\x7E");}}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void checkZero(){
  if(analogRead(zeroPin)>=990){
    isr();
    if(millis()-millisWater>=15000){
    pinMode(PIN_ECHO, OUTPUT);
    
    digitalWrite(PIN_ECHO, LOW);
    delayMicroseconds(5);
    digitalWrite(PIN_ECHO, HIGH);

    // Выставив высокий уровень сигнала, ждем около 10 микросекунд. В этот момент датчик будет посылать сигналы с частотой 40 КГц.
    delayMicroseconds(10);
    digitalWrite(PIN_ECHO, LOW);
    
    pinMode(PIN_ECHO, INPUT);
    
    //  Время задержки акустического сигнала на эхолокаторе.
    nowWater = pulseIn(PIN_ECHO, HIGH)/2;       //3223 - 0
    millisWater = millis();
    }
  }
  
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void checkWater(){
  if(nowWater == 0){
    digitalWrite(relePump, 0);
  }else{
    digitalWrite(relePump, 1);
  }
//  if(waterNow>=waterNeed){
//    flagWater = 1;
//  }
  digitalWrite(releEho, flagWater);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {  
  checkTemp();
  checkWater();
  menu();
  checkZero();
  avto();
  settings();
}
