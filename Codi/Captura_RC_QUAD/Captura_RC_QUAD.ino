/*
 * TFG - ARDUINO
 * Grau Enginyeria Informatica
 * Cristian Gascón Pérez
 * 
 * Programa per capturar el codi del comandament a distància de les perianes
 * marca SUBLIMEX
 */


//Declarem variables i vector per la captura del codi del comandament

boolean capturant = false;  //Ens indica si estem capturant dades
boolean comprovacioRF = false;   //Ens indica si s'ha capturat alguna senyal RF
boolean repDada = false;    //Ens indica si estem reben dades
int i = 0;  //Iniciem el contador per la captura dels bits de codi RF
float periodeRF = 0;   //Variable on emmmagatzemar el periode de la senyal
int bitsRF[40]; //Vector per emmagatzemar el codi de 40 bits

//Configuració inicial i inici Interrupció

void setup() {

  Serial.begin(9600);
  Serial.println("Preparat...");
  Serial.println("Polsi un botó del comandament");
  pinMode(2, INPUT);    //Activo el pin 2 com a entrada en Arduino ONE aquesta es interrupció
  attachInterrupt(0, capturaInterrupcio, CHANGE);  //Aquesta interrupció s'activa quan hi ha un canvi de senyal al pin 2
}

void loop() {

  //El loop està buit perque utilitzo una interrupció per la captura de codi
  
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
      if ((micros() - periodeRF > 1000) && digitalRead(2) == HIGH) {  //that was the long LOW part before transmission of data
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


/*
 * FI DE PROGRAMA
 */
