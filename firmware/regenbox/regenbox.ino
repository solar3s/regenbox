//Regenbox V1: mesure de tension et renvoi sur le serial selon les commandes serial

#define VOLTAGE_REF            2410      // tension de reference, tension sur la pin AREF exprimée en mV
#define CHARGE_PIN                4      // Identification de la pin de commande de la charge
#define DECHARGE_PIN              3      // Identification de la pin de commande de la décharge
#define SENSOR_PIN_1             A0      // Identification de la pin de mesure de tension pile 1
#define SENSOR_PIN_2             A1      // Identification de la pin de mesure de tension pile 2
#define SENSOR_PIN_3             A2      // Identification de la pin de mesure de tension pile 3
#define SENSOR_PIN_4             A3      // Identification de la pin de mesure de tension pile 4
#define ONE_HOUR            3600000      // nombre de milli-secondes dans une heure
#define ONE_MINUTE            60000      // nombre de milli-secondes dans une minute
#define TWENTY_FOUR_HOURS  86400000      // nombre de milli-secondes dans 24 heures
#define NB_ANALOG_RD            204      // nombre de lecture de la tension
#define CAN_RESOLUTION         1023      // resolution du convertisseur analogique numérique 10 bit
#define OFFICIAL_TEST
//#define MULTIPLE_SENSORS

enum RBX_STATUS {
    RBX_STATUS_IDLE     = 0,
    RBX_STATUS_CHARGE   = 1,
    RBX_STATUS_DECHARGE = 2
};

enum RBX_MODE {
    RBX_MODE_IDLE            = 0,
    RBX_MODE_DECHARGE_CHARGE = 1,
    RBX_MODE_REPORT_VOLTAGE  = 2,
    RBX_MODE_CHARGE_DECHARGE = 3,
    RBX_MODE_CHARGE          = 4,
    RBX_MODE_DEEP_DECHARGE   = 5
};

RBX_STATUS gStatus = RBX_STATUS_CHARGE;
RBX_MODE gMode     = RBX_MODE_IDLE;

byte gNb_cycle                = 0;     //compteur du nombre de cycle de charge/decharge
byte gHeure                   = 0;
boolean gTerminal_actif       = false; // True: arduino en connexion avec le PC, FALSE: arduino non connecté avec le PC(exemple sur alim secteur)
unsigned long gPreviousMillis = 0;

//-----------------------------------------------------------------------------
//--------- Mesure de la tension dans l'emplacement 1 
// Mesure de la tension de la pile dans l'emplacement 1
// Le temps de mesure de est de 0,1 ms on ajoute un delay de 1ms
// le cycle de mesure est de 1,1ms que l'on réalise 204 fois
// le cycle de mesure est de ~224 ms, c'est la mesure de cycle
// de charge, on peut donc espérer avoir une mesure de la tension
// à peu prêt constante
//-----------------------------------------------------------------------------
unsigned long getVoltage(byte sensor_pin) {
    unsigned long voltage_mesure = analogRead(sensor_pin);
    for (byte i = 0; i < NB_ANALOG_RD; i++) {
        voltage_mesure = voltage_mesure + analogRead(sensor_pin);
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
    unsigned long voltage_mesure = getVoltage(SENSOR_PIN_1);
#ifndef OFFICIAL_TEST
    Serial.print("Tension pile emplacement 1 : ");
#endif 
    Serial.println(voltage_mesure);
#ifdef MULTIPLE_SENSORS
    voltage_mesure = getVoltage(SENSOR_PIN_2);
    Serial.print("Tension pile emplacement 2 : "); 
    Serial.println(voltage_mesure);
    voltage_mesure = getVoltage(SENSOR_PIN_3);
    Serial.print("Tension pile emplacement 3 : "); 
    Serial.println(voltage_mesure);
    voltage_mesure = getVoltage(SENSOR_PIN_4);
    Serial.print("Tension pile emplacement 4 : "); 
    Serial.println(voltage_mesure);
    //Serial.println("mV;");
#endif // MULTIPLE_SENSOR
}

//----------------------------------------------------------
//--- Sélection du mode de la Regenbox ---------------------
//----------------------------------------------------------
void setRegenBoxStatus(RBX_STATUS status) {
    if (status == gStatus) {
        return;
    }
    
    gStatus = status;
    if (status == RBX_STATUS_CHARGE) {
        digitalWrite(CHARGE_PIN,   LOW);    // activation de la chargeat
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
//-------------------------------------------------------------
//--- Sélection du mode de fonctionnement de la RegenBox
//-------------------------------------------------------------
void setRegenBoxMode(RBX_MODE mode) {
    gMode = mode;
    gPreviousMillis = millis();
    switch(gMode) {
        case RBX_MODE_DECHARGE_CHARGE:
            gNb_cycle = 0;
            setRegenBoxStatus(RBX_STATUS_DECHARGE); // initialisation
            Serial.println("Debut du cycle de decharge/charge");
            reportVoltage();
            break;
        case RBX_MODE_CHARGE_DECHARGE:
            gNb_cycle = 0;
            setRegenBoxStatus(RBX_STATUS_CHARGE); // initial state
            Serial.println("Debut du cycle de charge/decharge");
            reportVoltage();
            break;
        case RBX_MODE_REPORT_VOLTAGE:
            setRegenBoxStatus(RBX_STATUS_IDLE);
            Serial.println("Cycle de lecture de la tension");
            reportVoltage();
            break;    
        case RBX_MODE_CHARGE:
            setRegenBoxStatus(RBX_STATUS_CHARGE);
            Serial.println("Debut du cycle de charge");
            reportVoltage();
            break;
        case RBX_MODE_IDLE:
            setRegenBoxStatus(RBX_STATUS_IDLE);
            break;
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
}

void loop() {
    if (gTerminal_actif == false) {
        if(Serial.available()) {
            Serial.read();
            gTerminal_actif = true;
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
    }
    else {
        unsigned long currentMillis = millis();
        unsigned long voltage_mesure = 0;
        switch(gMode) {
            case RBX_MODE_DECHARGE_CHARGE:
            case RBX_MODE_CHARGE_DECHARGE:
                if ((currentMillis - gPreviousMillis) > ONE_MINUTE) {
                    reportVoltage();
                    gPreviousMillis = currentMillis;
                }

                voltage_mesure = getVoltage(SENSOR_PIN_1);
                if (voltage_mesure < 688) { //Pile avec une tension inférieur à 700mV, on interdit les cycles de charge/decharge
                    setRegenBoxMode(RBX_MODE_IDLE);
		                Serial.println("Mort de la pile en cours de cycle!");
                }
                else if ((voltage_mesure < 900) && (gStatus == RBX_STATUS_DECHARGE)) { // 374 correspond à 0,9V avec un VREF à 2,5V
                    setRegenBoxStatus(RBX_STATUS_CHARGE);
                    gHeure = 0;
                    Serial.println("Cycle de charge");
                }
               
                if (gStatus == RBX_STATUS_CHARGE) {           
                    if ((currentMillis - gPreviousMillis) >= ONE_HOUR) {
                        gHeure++;
                        gPreviousMillis = currentMillis;
                    }
                    if (voltage_mesure > 1500) {
                        Serial.println("Cycle de decharge");
                        setRegenBoxStatus(RBX_STATUS_DECHARGE);
                    }
                    if (gHeure > 23) { //Si charge de plus de 24h
                        if (voltage_mesure > 1400) { // 1,45V
  				                  gNb_cycle++;
                            setRegenBoxStatus(RBX_STATUS_DECHARGE);
                        }
                        else {
  					                gNb_cycle++;  
                            Serial.println("Arret des cycle decharge/charge");
                            Serial.println(gNb_cycle);
                            setRegenBoxMode(RBX_MODE_IDLE);
                        }
                        gHeure = 0;
                    }
                }
                break;
            
            case RBX_MODE_REPORT_VOLTAGE:
                if ((currentMillis - gPreviousMillis) > ONE_MINUTE) {
                    reportVoltage();
                    gPreviousMillis = currentMillis;
                }
                break;
           
           case RBX_MODE_CHARGE:
               if ((currentMillis - gPreviousMillis) > ONE_MINUTE) {
                   //setRegenBoxStatus(RBX_STATUS_CHARGE);
                   Serial.print("Time: ");
                   Serial.println(currentMillis);
                   reportVoltage();
                   gPreviousMillis = currentMillis;
                   // TODO : Define the strategy to stop charge !
               }
               break;
              
           case RBX_MODE_DEEP_DECHARGE:
                if ((currentMillis - gPreviousMillis) > ONE_MINUTE) {
                    //Decharge profonde (jusqu'a la mort de la pile)
                    setRegenBoxStatus(RBX_STATUS_DECHARGE);
                    reportVoltage();
                    gPreviousMillis = currentMillis;
                    // TODO : Define the strategy to stop the decharge
                }
                break;
              
            default:
               break;
        }
      
        if (Serial.available()){
            byte tamp = Serial.read();
            if (tamp == '1') {
                Serial.println("Cycle de decharge");
                setRegenBoxMode(RBX_MODE_DECHARGE_CHARGE);
            }
            else if (tamp == '2') {
                setRegenBoxMode(RBX_MODE_REPORT_VOLTAGE);
            }
            else if (tamp == '3') {
                setRegenBoxMode(RBX_MODE_CHARGE_DECHARGE);
                Serial.println("Cycle de charge");
            }
            else if (tamp == '4') {
                setRegenBoxMode(RBX_MODE_CHARGE);
            }
            else if (tamp == '5') {
                setRegenBoxMode(RBX_MODE_DEEP_DECHARGE);
            }
        }
    }
}
