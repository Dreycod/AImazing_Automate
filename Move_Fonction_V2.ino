// === Définition des broches moteur ===
const int DIR_B = 2;  // Direction moteur gauche
const int PWM_B = 5;  // PWM moteur gauche
const int DIR_A = 4;  // Direction moteur droite
const int PWM_A = 6;  // PWM moteur droite


// === Capteurs ligne ===
int sensor_L = 11;
int sensor_M = 7;
int sensor_R = 8;
int L_val, M_val, R_val;


void toutDroit(int duree = 290) {
  unsigned long debut = millis();
  unsigned long dernierCorrection = 0;
  const unsigned long intervalCorrection = 20;

  while (millis() - debut < duree) {
    if (millis() - dernierCorrection >= intervalCorrection) {
      correctionSuiviLigne();
      dernierCorrection = millis();
    }
  }
  arreter();
}

void tournerDroite90(int vitesseG = 100, int vitesseD = 105, int duree = 450) {
  Serial.println("↩️ Rotation sur place à DROITE");

  // Moteur gauche en avant
  digitalWrite(DIR_B, HIGH);
  analogWrite(PWM_B, vitesseG);

  // Moteur droit en arrière
  digitalWrite(DIR_A, LOW);
  analogWrite(PWM_A, vitesseD);

  delay(duree);

  arreter();
  delay(1000);

  // 🔄 Recul correctif (1 pas arrière)
  Serial.println("↩️ Recul correctif pour recentrer");
  digitalWrite(DIR_B, LOW);
  analogWrite(PWM_B, 100);
  digitalWrite(DIR_A, LOW);
  analogWrite(PWM_A, 90); // Ajuste si besoin
  delay(200); // Ajuste si besoin
  arreter();
}



void tournerGauche90(int vitesseD = 100, int vitesseG = 105, int duree = 525) {
  Serial.println("↪️ Rotation sur place à GAUCHE");

  // Moteur gauche en arrière
  digitalWrite(DIR_B, LOW);
  analogWrite(PWM_B, vitesseG);

  // Moteur droit en avant
  digitalWrite(DIR_A, HIGH);
  analogWrite(PWM_A, vitesseD);

  delay(duree);

  arreter();
  delay(1000);

   // 🔄 Recul correctif (1 pas arrière)
  Serial.println("↩️ Recul correctif pour recentrer");
  digitalWrite(DIR_B, LOW);
  analogWrite(PWM_B, 100);
  digitalWrite(DIR_A, LOW);
  analogWrite(PWM_A, 90);
  delay(200); // Ajuste si besoin
  arreter();

}


void demiTour180() {
  Serial.println("↪️ Demi-tour à 180°");
  
  // Premier 90° à gauche (ou à droite, ça dépend de ton robot et de l’IA)
  tournerGauche90(100, 105, 535);
  
  delay(500); // Petite pause pour la stabilité

  // Deuxième 90° à gauche
  tournerGauche90(95, 105, 545);
  
  Serial.println("✅ Demi-tour terminé");
}

void arreter() {
  digitalWrite(DIR_B, LOW);
  analogWrite(PWM_B, 0);

  digitalWrite(DIR_A, LOW);
  analogWrite(PWM_A, 0);

  Serial.println("→ Arrêt forcé");
}


void AllerDroite()
{
  tournerDroite90();
  delay(500);
  toutDroit();
}


void AllerGauche()
{
  tournerGauche90();
  delay(500);
  toutDroit();
}


void FaireDemiTourViaGauche()
{
  demiTour180();
  delay(500);
  toutDroit();
}

void correctionSuiviLigne() {
  L_val = digitalRead(sensor_L);
  M_val = digitalRead(sensor_M);
  R_val = digitalRead(sensor_R);

  if (L_val == 1 && R_val == 0) {
    // ↩️ Correction à gauche
    digitalWrite(DIR_B, LOW);  // moteur gauche recule
    analogWrite(PWM_B, 100);
    digitalWrite(DIR_A, HIGH); // moteur droit avance
    analogWrite(PWM_A, 130);
  }
  else if (L_val == 0 && R_val == 1) {
    // ↪️ Correction à droite
    digitalWrite(DIR_B, HIGH); // moteur gauche avance
    analogWrite(PWM_B, 130);
    digitalWrite(DIR_A, LOW);  // moteur droit recule
    analogWrite(PWM_A, 100);
  }
  else {
    // ↔️ Tout droit
    digitalWrite(DIR_B, HIGH);
    analogWrite(PWM_B, 120);
    digitalWrite(DIR_A, HIGH);
    analogWrite(PWM_A, 120);
  }
}


void setup() {
  Serial.begin(9600);
  pinMode(DIR_B, OUTPUT);
  pinMode(PWM_B, OUTPUT);
  pinMode(DIR_A, OUTPUT);
  pinMode(PWM_A, OUTPUT);

  pinMode(sensor_L, INPUT);
  pinMode(sensor_M, INPUT);
  pinMode(sensor_R, INPUT);

  Serial.println("📡 Robot prêt. Tapez g180 ou d170 pour calibrer les vitesses en direct.");

  delay(3000); // Pause avant redémarrage

  
  AllerDroite();
  delay(500);
  AllerDroite();


  toutDroit();
  delay(500);
  toutDroit();

}




void loop() {

}