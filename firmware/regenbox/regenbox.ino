//Regenbox V1: mesure de tension et renvoi sur le serial selon les commandes serial

#define VOLTAGE_REF         2410      // tension de reference, tension sur la pin AREF exprimée en mV
#define CHARGE_PIN             4      // Identification de la pin de commande de la charge
#define DECHARGE_PIN           3      // Identification de la pin de commande de la décharge
#define SENSOR_PIN            A0      // Identification de la pin de mesure de tension pile
#define INTERVAL         3600000      // nombre de milli-seconde dans une heure
#define NB_ANALOG_RD         204      // nombre de lecture de la tension
#define CAN_RESOLUTION      1023      // resolution du convertisseur analogique numérique 10 bit

enum RBX_STATUS {
  RBX_STATUS_IDLE     = 0,
  RBX_STATUS_CHARGE   = 1,
  RBX_STATUS_DECHARGE = 2
};

enum RBX_MODE {
  RBX_MODE_IDLE            = 0,
  RBX_MODE_DECHARGE_CHARGE = 1,
  RBX_MODE_REPORT_VOLTAGE  = 2,
  RBX_MODE_CHARGE          = 4,
  RBX_MODE_DEEP_DECHARGE   = 5
};

RBX_STATUS status = RBX_STATUS_IDLE;
RBX_MODE mode = RBX_MODE_IDLE;

byte etat = 0;                  // 0: pas de mesure/pas de remontée de donnée/attente ; 1: demarrage de cycle decharge/charge; 2: fonction voltmètre
byte last_etat = 0;
byte CMD_1 = 0;                 //commande transistor de charge
byte CMD_2 = 0;                 //commande transistor de decharge
byte cycle = 0;                 // 1: charge ; 2: decharge
byte nb_cycle = 0;              //compteur du nombre de cycle de charge/decharge
byte heure = 0;
unsigned long voltage_mesure;   //tension mesure sur la pin SensorPin
boolean terminal_actif = false; // True: arduino en connexion avec le PC, FALSE: arduino non connecté avec le PC(exemple sur alim secteur)
byte etat_init = 0;             //Back up etat (pour Decharge pure)

unsigned long last_mesure;
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;

//-----------------------------------------------------------------------------
//--------- Mesure de la tension dans l'emplacement 1 
// Mesure de la tension de la pile dans l'emplacement 1
// Le temps de mesure de est de 0,1 ms on ajoute un delay de 1ms
// le cycle de mesure est de 1,1ms que l'on réalise 204 fois
// le cycle de mesure est de ~224 ms, c'est la mesure de cycle
// de charge, on peut donc espérer avoir une mesure de la tension
// à peu prêt constante
//-----------------------------------------------------------------------------
unsigned long getVoltage() {
    unsigned long voltage_mesure = analogRead(SENSOR_PIN);
    for (byte i = 0; i < NB_ANALOG_RD; i++) {
      voltage_mesure = voltage_mesure + analogRead(SENSOR_PIN);
      delay(1);
    }
    voltage_mesure = voltage_mesure / NB_ANALOG_RD;
    voltage_mesure = (voltage_mesure * VOLTAGE_REF) / CAN_RESOLUTION;
    
    return voltage_mesure;
}

//-----------------------------------------------------------------------------
// Envoie de la tension sur le port serie
//-----------------------------------------------------------------------------
void reportVoltage() {
  voltage_mesure = getVoltage();
  //Serial.print("Tension pile emplacement 1 : "); 
  Serial.println(voltage_mesure);
  //Serial.println("mV;");
}

//----------------------------------------------------------
//--- Sélection du mode de la Regenbox ---------------------
//----------------------------------------------------------
void setRegenBoxStatus(RBX_STATUS status) {
  if (status == RBX_STATUS_CHARGE) {
    digitalWrite(CHARGE_PIN,   LOW);    // activation de la charge
    digitalWrite(DECHARGE_PIN, LOW);    // desactivation de la decharge
  }
  else if (status == RBX_STATUS_DECHARGE) {
    digitalWrite(CHARGE_PIN,   HIGH);   // desactivation de la charge
    digitalWrite(DECHARGE_PIN, HIGH);   // activation de la charge
  }
  else { // Mode par défaut ni charge ni décharge IDLE
    digitalWrite(CHARGE_PIN,   HIGH);   // desactivation de la charge
    digitalWrite(DECHARGE_PIN, LOW);    // desactivation de la decharge
  }
}

void setup() {
  Serial.begin(9600);
  analogReference(EXTERNAL);          // reference de tension pour les mesures
  pinMode(CHARGE_PIN, OUTPUT);
  pinMode(CHARGE_PIN, OUTPUT);

  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  setRegenBoxStatus(RBX_STATUS_IDLE);
  //mode = RBX_MODE_REPORT_VOLTAGE;
  //terminal_actif = true;
}

void loop() {
   if(terminal_actif == false){
    if(Serial.available()){
      Serial.read();
      terminal_actif = true;
      //Serial.println("Demarrage");
      /****MENU****/
      Serial.println("AVIS AUX BETA-TESTEURS : Ne pas interrompre les tests pour préserver l'intégralité et la fiabilité des données");
      Serial.println("      Toutes les mesures sont données ici en mV et représentent la tension aux bornes de la pile +/- 3mV\n");
      Serial.println("                                    1 : Lancement des cycles decharge/charge;");
      Serial.println("                                      2 : Mesure tension pile emplacement 1;");
      Serial.println("                                    3 : Lancement des cycles charge/decharge;");
      Serial.println("                    4 : Charge des piles sur les quatre emplacements (sans collecte des données);");
      Serial.println("                          5 : Lancement Decharge profonde (jusqu'a la mort de la pile);");
      Serial.println("");
      setRegenBoxStatus(RBX_STATUS_IDLE);
    }
   }else{
      currentMillis = millis();
      switch(mode){
          //case 0:
              //Serial.println(currentMillis);
              //Serial.println(last_mesure);
            //  if( (cycle == 2 ) || (cycle == 1)){
            //    if((currentMillis - last_mesure)  > 60000){ //Permet d'envoyer sur le serial toutes les minutes la tension de la pile
             //     etat = 1;
                  
             //     setRegenBoxStatus(RBX_STATUS_DECHARGE);
             //     delay(5000);
              //    setRegenBoxStatus(RBX_STATUS_CHARGE);
               // }
             // }
             // break;
          case RBX_MODE_DECHARGE_CHARGE:
              voltage_mesure = getVoltage();
              Serial.println(voltage_mesure);
              //Serial.println(";");
              
              if (voltage_mesure < 688) { //Pile avec une tension inférieur à 700mV, on interdit les cycles de charge/decharge
                setRegenBoxStatus(RBX_STATUS_IDLE);
		            Serial.println("Mort de la pile en cours de cycle!");
              }
              else if ((voltage_mesure < 900) && (status == RBX_STATUS_DECHARGE)) { // 374 correspond à 0,9V avec un VREF à 2,5V
                  setRegenBoxStatus(RBX_STATUS_CHARGE);
                  //cycle = 1;
                  heure = 0;
                  Serial.println("Cycle de charge");
               }
               
               last_mesure = currentMillis;
                //if(cycle == 1){
               if (status = RBX_STATUS_CHARGE) {           
                  if ((currentMillis - previousMillis) >= INTERVAL) {
                    heure = heure +1;
                    previousMillis = currentMillis;
                }
                if (heure > 23) { //Si charge de plus de 24h
                    if (voltage_mesure > 1400) { // 1,45V
  				            nb_cycle++;
                      setRegenBoxStatus(RBX_STATUS_DECHARGE);
                      cycle = 2;
                    }
                    else {
  					          nb_cycle++;  
                      Serial.println("Arret des cycle decharge/charge");
                      Serial.println(nb_cycle);
                      //Serial.print("");
                      setRegenBoxStatus(RBX_STATUS_IDLE);
                      etat = 0;
                      cycle = 0;
                    }
                    heure = 0;
                  }
                }
                etat = 0;
              break;
          case RBX_MODE_REPORT_VOLTAGE:
              reportVoltage();
              delay(500);
              break;
           
           case RBX_MODE_CHARGE:
              //charge des 4 piles sans remontée de donnée
              setRegenBoxStatus(RBX_STATUS_CHARGE);
              break;
           case RBX_MODE_DEEP_DECHARGE:
              //Decharge profonde (jusqu'a la mort de la pile)
              setRegenBoxStatus(RBX_STATUS_DECHARGE);
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
              setRegenBoxStatus(RBX_STATUS_DECHARGE);
          }else if(tamp == '2'){
              etat = RBX_MODE_REPORT_VOLTAGE;
              mode = RBX_MODE_REPORT_VOLTAGE;
              setRegenBoxStatus(RBX_STATUS_IDLE);
          }else if(tamp == '3'){
              etat = 1;
              cycle = 2;
              nb_cycle = 0;
              setRegenBoxStatus(RBX_STATUS_CHARGE);
              Serial.println("Cycle de charge");
          }else if(tamp == '4'){
              etat = 4;
              cycle = 0;
          }else if(tamp == '5'){
              etat = 5;
              cycle = 0;
          }
          while(Serial.available()){
              tamp = Serial.read();
          }
      }
   }
}
