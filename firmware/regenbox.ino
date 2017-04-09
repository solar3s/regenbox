//Regenbox V1: mesure de tension et renvoi sur le serial selon les commandes serial

#define ref_voltage 2460      //tension de reference, tension sur la pin AREF
#define pin_charge 4     //
#define pin_decharge 3   //
#define sensorPin A0      //pin de mesure de tension pile
#define INTERVAL 3600000  //nombre de seconde dans une heure


byte etat = 0;                  // 0: pas de mesure/pas de remontée de donnée/attente ; 1: demarrage de cycle decharge/charge; 2: fonction voltmètre
byte last_etat = 0;
byte CMD_1 = 0;                 //commande transistor de charge
byte CMD_2 = 0;                 //commande transistor de decharge
byte cycle = 0;                 // 1: charge ; 2: decharge
byte nb_cycle = 0;              //compteur du nombre de cycle de charge/decharge
byte heure = 0;       
unsigned long voltage_mesure;   //tension mesure sur la pin SensorPin
boolean terminal_actif = false; // True: arduino en connexion avec le PC, FALSE: arduino non connecté avec le PC(exemple sur alim secteur)

unsigned long last_mesure;
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;

void setup() {
  Serial.begin(9600);
  //analogReference(EXTERNAL);          //reference de tension pour les mesures
  pinMode(pin_charge, OUTPUT);
  pinMode(pin_decharge, OUTPUT);

  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  
  digitalWrite(pin_charge, 0);      //activation de la charge
  digitalWrite(pin_decharge, 0);    //desactivation de la decharge
}

void loop() {
   if(terminal_actif == false){
    if(Serial.available()){
      Serial.read();
      terminal_actif = true;
      Serial.println("Demarrage");
      /****MENU****/
      Serial.println("1 : Lancement des cycles decharge/charge");
      Serial.println("2 : Mesure tension pile emplacement 1");
      Serial.println("3 : Lancement des cycles charge/decharge");
      Serial.println("4 : Charge des piles sur les quatre emplacement");
      Serial.println("");
      digitalWrite(pin_charge, 1);        //desactivation de la charge
      digitalWrite(pin_decharge, 0);        //desactivation de la decharge
    }
   }else{
      currentMillis = millis();
      switch(etat){
          case 0:
              //Serial.println(currentMillis);
              //Serial.println(last_mesure);
              if( (cycle == 2 ) || (cycle == 1)){
                if((currentMillis - last_mesure)  > 60000){ //Permet d'envoyer sur le serial tout les minutes la tension de la pile
                  etat = 1;
                }
              }
              break;
          case 1:

              voltage_mesure = analogRead(sensorPin);
              for(byte i=0; i< 9; i++){
                voltage_mesure = voltage_mesure + analogRead(sensorPin);
              }
              //Envoi sur le serial
              voltage_mesure = voltage_mesure/10;
              //Serial.println(voltage_mesure);
              voltage_mesure = (voltage_mesure * ref_voltage) / 1023;
              Serial.println(voltage_mesure);
              //Serial.println(";");

                
              if((voltage_mesure < 900) && (cycle == 2)){ // 374 correspond à 0,9V avec un VREF à 2,5V
                digitalWrite(pin_decharge, LOW);          //desactivation de la decharge
                digitalWrite(pin_charge, LOW);           //activation de la charge
                cycle = 1;
                heure = 0;
                Serial.println("Cycle de charge");
              }
              

    
              last_mesure = currentMillis;
              //Serial.println(last_mesure);
              
              if(cycle == 1){           
                if((currentMillis - previousMillis) >= INTERVAL){
                  heure = heure +1;
                  previousMillis = currentMillis;
                }
                if(heure > 23){ //Si charge de plus de 24h
                  if(voltage_mesure > 1400){ // 1,45V
					nb_cycle = nb_cycle + 1;
                    digitalWrite(pin_decharge, HIGH); //activation de la decharge
                    digitalWrite(pin_charge, HIGH); //desactivation de la charge
                    cycle = 2;
                  }else{
					nb_cycle = nb_cycle + 1;  
                    Serial.println("Arret des cycle decharge/charge");
                    Serial.println(nb_cycle);
                    //Serial.print("");
                    etat = 0;
                    cycle = 0;
                  }
                  heure = 0;
                }
                etat = 0;
              }else{
                etat = 0;
              }
              break;
          case 2:
              voltage_mesure = analogRead(sensorPin);
              for(byte i=0; i< 9; i++){
                voltage_mesure = voltage_mesure + analogRead(sensorPin);
              }
              
              voltage_mesure = voltage_mesure/10;
              //Serial.println(voltage_mesure);
              voltage_mesure = (voltage_mesure * ref_voltage) / 1023;
              Serial.print("Tension pile emplacement 1 : "); 
              Serial.print(voltage_mesure);
              Serial.println("mV;");
              etat = 0;
              cycle = 0;
              break;
           case 4:
              //charge des 4 piles sans remontée de donnée
              digitalWrite(pin_decharge, 0); //desactivation de la decharge
              digitalWrite(pin_charge, 0); //activation de la charge
              break;
      }
      
      if(Serial.available()){
          byte tamp = Serial.read();
          //Serial.print(etat);
          if(tamp == '1'){
              etat = 1;
              cycle = 2;
              Serial.println("Cycle de decharge");
              nb_cycle = 0;
              digitalWrite(pin_decharge, HIGH); //activation de la decharge
              digitalWrite(pin_charge, HIGH); //desactivation de la charge
          }else if(tamp == '2'){
              etat = 2;
              digitalWrite(pin_decharge, LOW); //desactivation de la decharge
              digitalWrite(pin_charge, HIGH); //desactivation de la charge
          }else if(tamp == '3'){
              etat = 1;
              cycle = 2;
              nb_cycle = 0;
              digitalWrite(pin_decharge, LOW); //desactivation de la decharge
              digitalWrite(pin_charge, LOW); //activation de la charge
              Serial.println("Cycle de charge");
          }else if(tamp == '4'){
              etat = 4;
              cycle = 0;
          }
          while(Serial.available()){
              tamp = Serial.read();
          }
      }
   }
}
