

/*
 * TFG - ARDUINO
 * Grau Enginyeria Informatica
 * Cristian Gascón Pérez
 * 
 * Control automàtic de persianes elèctriques amb sensors de llum i pluja
 */
 
#include <DHT.h>
#include <Time.h>
#include <TimeLib.h>
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <LiquidCrystal.h>
#include <RTC.h>
#include <RCSwitch.h>


//Pantalla LCD
#define I2C_ADDR  0x27  //Definim la variable amb la direcció del Display LCD dins el bus I2C
LiquidCrystal_I2C lcd(I2C_ADDR,2, 1, 0, 4, 5, 6, 7);  //Generem una instancia del objecte LiquidCrystal_I2C amb els pins utilitzats

//Sensor de temperatura 
#define DHTPIN 8  //Definim la entrada digital on va connectat el sensor de temperatura DHT11
#define DHTTYPE DHT11 //Definim el tipus de sensor DHT11
DHT dht (DHTPIN,DHTTYPE); //Es defineix la variable dht que fará la comunicació amb el sensor de temperatura

//Rellotge de temps real (RTC)
RTC rtc(DST_ON); // Activem el canvi d'hora automátic en horari Estiu


//Sensor de llum LDR
int ldr;          //variable per el valor del sensor de llum LDR
int ldrUmbralMax = 120;   // Umbral Punt de llum màxim, aquest estat el dona el sensor quant tenim llum solar directe, sempre donarà l’ordre de baixada de la persiana.
int ldrUmbralOptim = 600; // Umbral Punt de llum òptim, aquest punt està entre l’alba i el capvespre, en aquest estat es dona l’ordre de pujada de la persiana.
int ldrUmbralNul = 1000;  // Umbral Punt de llum nul, el dona el sensor quan es de nit i donarà l’ordre de baixada de la persiana 
int pluja;
boolean llumMax = false;  //Variable que ens indica que este a punt de llum màxim
boolean llumOptim = true; //Variable que ens indica que este a punt de llum òptim
boolean llumNul = false;  //Variable que ens indica que este a punt de llum nul

//Estat Persiana
boolean persianaBaixada = true;    //variable que ens indica si la persiana esta baixada 
boolean persianaPujada = false;    //variable que ens indica si la persiana esta pujada 

//Sensor pluja
int plujaIn=10;     // Entrada digital del sensor de pluja

//Declarem variables i vector per la captura del codi del comandament
boolean capturant = false;  //Ens indica si estem capturant dades
boolean comprovacioRF = false;   //Ens indica si s'ha capturat alguna senyal RF
boolean repDada = false;    //Ens indica si estem reben dades
int i = 0;  //Iniciem el contador per la captura dels bits de codi RF
float periodeRF = 0;   //Variable on emmmagatzemar el periode de la senyal
int bitsRF[40]; //Vector per emmagatzemar el codi de 40 bits

//Declarem variables per l'envio del codi del comandament
RCSwitch mySwitch = RCSwitch();  //Generem mySwitch per l'envio del codi RF
char codiPujar1[] = "0Q0F010FFF0F0Q0F0F0F";   //Constant que emmagatzema el codi del comandament per pujar la persiana 1
char codiBaixar1[] = "0Q0F010FFF0F0Q0F0110";  //Constant que emmagatzema el codi del comandament per baixar la persiana 1
char codiStop1[] = "0Q0F010FFF0F0Q0FFFFF";    // Constant que emmagatzema el codi del comandament per parar la persiana 1

char codiPujar2[] = "0Q00010F11Q0Q10F0F1Q";   //Constant que emmagatzema el codi del comandament per pujar la persiana 2
char codiBaixar2[] = "0Q00010F11Q0Q10F0110";  //Constant que emmagatzema el codi del comandament per baixar la persiana 2
char codiStop2[] = "0Q00010F11Q0Q10FFFFF";    // Constant que emmagatzema el codi del comandament per parar la persiana 2


//Configuració inicial
void setup() {

  //rtc.setDateTime( 2016, 10, 31, 15, 05, 00 );  //Aquesta linia s'utilitza només per posar-ho en hora (Any,Mes,dia,hora,minuts,segons)
  
  missatgeInicialLcd ();  //Imprimim missatge inicial per pantalla LCD durant 5 segons
  delay (5000);

  mySwitch.enableTransmit(9);  //Transmitim per el pin 9 de la placa arduino
  mySwitch.setProtocol(4);    //Seleccionem protocol QUADSTATE

  dht.begin();    //Iniciem el sensor de temperatura


  //Inicialització comunicació serie per la captura del codi comandament
  Serial.begin(9600);
  Serial.println("Preparat...");
  Serial.println("Polsi un botó del comandament");
  pinMode(2, INPUT);    //Activo el pin 2 com a entrada en Arduino ONE aquesta es interrupció
  attachInterrupt(0, capturaInterrupcio, CHANGE);  //Aquesta interrupció s'activa quan hi ha un canvi de senyal al pin 2
  
}



//*****Codi Principal****************************

void loop() {

  
  ldr = analogRead (0);   //llegeix el valor analogic de la entrada A0 que indica la LDR
  pluja = digitalRead (plujaIn);  //llegeix el valor digital de la entrada amb 0 indica pluja, 1 no indica pluja
  
  int h = dht.readHumidity(); //Es fa lectura del valor de humitat i es posa en la variable "h"
  int t = dht.readTemperature(); //Es fa lectura del valor de humitat i es posa en la variable "h"

  
  lcd.clear ();   // Fem un esborrat de la pantalla LCD i imprimim l'estat de la persiana, el rellotge, la temperatura i si plou
  lcdEstatPersiana();
  visualitzacioHoraRTC();
  visualitzacioMeteoLcd(t,h); //Es pasen per parametres el valor de temperatura i humitat
  visualitzacioPlujaLcd();
  visualitzacioLlumLcd();

  //Comprovació estat de llum per determinar l'estat Maxim, Optim i nul
  
  if (ldr > ldrUmbralNul) {   
      llumMax = false;
      llumOptim = false;
      llumNul = true;
    } 
    
    if (ldr < ldrUmbralOptim) {   
      llumMax = false;
      llumOptim = true;
      llumNul = false;
    } 
    
    if (ldr < ldrUmbralMax){
      llumMax = true;
      llumOptim = false;
      llumNul = false;
    }
  
  //Es compara l'estat de la LDR i del sensor de pluja per pujar o baixar la persiana segons convingui
  
  if ((llumNul) || (llumMax) || (pluja == 0)) {   
      emisorBaixar();
    } else if (llumOptim) {   
      emisorPujar();
    } 
}

//***********************************************


//******Funcions programa***********************

//Codi del emisor per pujar persiana
void emisorPujar(){   
  
  if (persianaBaixada) {         //Si la persiana està baixada activem la pujada

    lcd.setCursor( 10, 2 );        // ir a la quarta linia
    lcd.print("Pujant...");
   
    mySwitch.sendQuadState (codiPujar1); //Enviem el codi RF per pujar la persiana 2
    delay (200);
    mySwitch.sendQuadState (codiPujar2); //Enviem el codi RF per pujar la persiana 2
    
    delay(5000);  //Donem un temps per baixar la persiana
    
    persianaPujada = true;
    persianaBaixada = false;
  }
}


//Codigo del emisor para baixar la persiana
void emisorBaixar(){

  if (persianaPujada) {        //Si la persiana està pujada activem la baixada
    lcd.setCursor( 10, 2 );        // ir a la quarta linia
    lcd.print("Baixant...");

    mySwitch.sendQuadState (codiBaixar1); //Enviem el codi RF per baixar la persiana 1
    delay(200);
    mySwitch.sendQuadState (codiBaixar2); //Enviem el codi RF per baixar la persiana 2
    
    delay(5000); //Donem un temps per baixar la persiana

    persianaPujada = false;
    persianaBaixada = true;
  }
}


//Codi per la visualització estat persiana per LCD
void lcdEstatPersiana (){

  lcd.setCursor ( 0, 2 );        // ir a la tercera linia
  lcd.print("Persiana:");                
  
  switch (persianaPujada){
  
    case true:               
    lcd.setCursor( 10, 2 );        // ir a la quarta linia
    lcd.print("Pujada");
    break;

    case false:    
    lcd.setCursor ( 10, 2 );        // ir a la quarta linia
    lcd.print("Baixada");
    break;
  }
}


//Codi per la inicialització LCD
void missatgeInicialLcd () {

  lcd.begin (20,4);    // Inicialtizar el display amb 20 caracters 4 linias
  lcd.setBacklightPin(3,POSITIVE);
  lcd.setBacklight(HIGH);

  lcd.home ();                  
  lcd.print("TFG - ARDUINO - UOC");
  lcd.setCursor ( 2, 1 );        // ir a la segona linia
  lcd.print("Control persianes");
  lcd.setCursor ( 5, 2 );        // ir a la tercera linia
  lcd.print("amb sensors");
  lcd.setCursor ( 0, 3 );        // ir a la cuarta linia
  lcd.print("- Cristian Gascon -");
}

//Codi per visualitzar la hora i la data per la LCD
void visualitzacioHoraRTC(){

  Data d = rtc.getData();

  lcd.setCursor (0,0);
  lcd.print (d.toString((char*)"H:i:s"));
  lcd.setCursor(10, 0); 
  lcd.print( d.toString((char*)"d/m/Y") );  
}



// Codi per visualitzar temperatura i humitat per la LCD, rep com a parametres dos enters (temperatura i humitat)
void visualitzacioMeteoLcd(int temp, int hum) {  
  lcd.setCursor(0, 1); 
  lcd.print("Temp:");
  lcd.print (temp);
  lcd.print("C");
  lcd.setCursor(9, 1); 
  lcd.print("Humitat:");
  lcd.print (hum); 
  lcd.print("%"); 
}

// Codi per visualitzar estat llum.
void visualitzacioLlumLcd() {  
  
  lcd.setCursor(10, 3);
  lcd.print("Llum:");
  lcd.setCursor(15, 3);
  
  if (llumMax){
    lcd.print("MAX");
  }

  if (llumOptim){
    lcd.print("OPT");
  }

  if (llumNul){
    lcd.print("NUL");
  }
    
}



// Codi per visualitzar si hi ha pluja o no.
void visualitzacioPlujaLcd() {  
  
  lcd.setCursor(0, 3);
  lcd.print("Plou?:");
  lcd.setCursor(6, 3);
  
  switch (pluja) {
    
    case 0:
    lcd.print("SI");
    break;

    case 1:
    lcd.print("NO");
    break; 
  }
}


// Interrupció per la captura de codi

void capturaInterrupcio() {

  if (!capturant) {  //Si no estem capturant codi
    if (!comprovacioRF) {  // Si no hi ha comprovació de senyal RF
      if (digitalRead(2) == HIGH) {  //Si hi ha canvi de nivell baix (LOW) a nivell alt (HIGH) es calcula el periode
        periodeRF = micros();   
        comprovacioRF = true;
      }
    }

    else {    //Si estem capturan codi 
      
      // Si la senyal a nivel alt (HIGH) té un periode més gran que 4000us i estem a nivell baix (LOW)
      if ((micros() - periodeRF > 4000) && (digitalRead(2) == LOW)) {    
        comprovacioRF = false;
        capturant = true;
        periodeRF = micros();
      }

      else {
        // Si no hi ha senyal el posem a false
        comprovacioRF = false;
      }
    }
  }

  else {  // Comencem a capturar el codi
    if (!repDada) {  //Si no hem rebut dades
      if ((micros() - periodeRF > 1000) && digitalRead(2) == HIGH) {  
        repDada = true; //hem rebut dades
        periodeRF = micros();
      }
    }

    else {  //rebem les dades del codi
      //En el flanc de pujada (HIGH)
      if (digitalRead(2) == HIGH) {
        //Comprovem el periode de la senyal
        periodeRF = micros();
      }  

      //En el flanc de baixada (OW) 
      else if (digitalRead(2) == LOW) {
        // Comprovem el periode de la senyal
        if (micros() - periodeRF > 500) {
          //posem la dada 1 en el vector
          bitsRF[i] = 1;
        }

        else {
          //posem la dada 0 en el vector
          bitsRF[i] = 0;
        }

        if (i < 39) {
          // Comprovem que tenim tots els bits rebuts si no el tenim sumem contador
          i++;
        }

        else {
          //Finalitzada la captura del codi
          noInterrupts();  //Posem la interrupció a OFF


          //Imprimint el codi amb el protocol "quad-bit" el llegim del vector
          
          Serial.println("Codi comandament capturat:");

          //Comencem recorregut per el vector per llegir els bits
          for (i = 0; i <= 38; i = i + 2) {
            // Si hi ha dos bits a 0, imprimim 0
            if ((bitsRF[i] == 0) && (bitsRF[i+1] == 0)){
              Serial.print("0");
              
            } 
            // Si hi ha un bit a 0 i el bit següent a 1 , imprimim F
            else if ((bitsRF[i] == 0) && (bitsRF[i+1] == 1)){
              Serial.print("F");
             
            } 
            // Si hi ha un bit a 1 i el bit següent a 0 , imprimim Q
            else if ((bitsRF[i] == 1) && (bitsRF[i+1] == 0)){
              Serial.print("Q");
             
            }
            // Si hi ha dos bits a 1, imprimim 1
            else if ((bitsRF[i] == 1) && (bitsRF[i+1] == 1)){
              Serial.print("1");
             
            }
            
          }  
          Serial.println(); //Fem un salt de línia       
          i = 0;  //iniciem el contador per la impresió del codi
          repDada = false;    //Posem les variables de recepció dades i captura a false per pode començar una altra captura de codi
          capturant = false;
          interrupts();  //Posem la interrupció a ON
          return;  //Fem un return per començar de nou
        }
      }

    }
  }
}



//********************FI DE PROGRAMA***************************************


