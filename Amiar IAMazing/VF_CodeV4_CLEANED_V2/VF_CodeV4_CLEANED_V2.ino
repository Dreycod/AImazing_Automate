/*#include <Servo.h>
Servo ServoBase;
Servo ServoArm;


int angleBase = 10;  // Angle actuel de ServoBase
int angleArm = 0;   // Angle actuel de ServoArm

// Déplacement fluide d’un servo (utilisé pour les 2 servos)
void moveServo(Servo &servo, int &angleActuel, int angleFinal, int dureeMs) {
  int nombreDePas = abs(angleFinal - angleActuel);
  if (nombreDePas == 0) return;


  int delaiParPas = dureeMs / nombreDePas;
  int direction = (angleFinal > angleActuel) ? 1 : -1;


  for (int i = 0; i < nombreDePas; i++) {
    angleActuel += direction;
    servo.write(angleActuel);
    delay(delaiParPas);
  }
}


// Pour le servo de base
void DeployBase() {
  moveServo(ServoBase, angleBase, 65, 2000);
}


void HideBase() {
  moveServo(ServoBase, angleBase, 0, 2000);
}


// Pour le servo du bras
void DeployArm() {
  moveServo(ServoArm, angleArm, 140, 2500);
}


void HideArm() {
  moveServo(ServoArm, angleArm, 0, 2500);
}


void setup() {
  ServoBase.attach(27); // Signal du servo base sur D27
  ServoArm.attach(29);  // Signal du servo bras sur D29


  ServoBase.write(angleBase);
  ServoArm.write(angleArm);

  delay(500);

  DeployBase();
  delay(300);
  DeployArm();
  delay(500); // Laisse le bras stabilisé avant de lancer la suite
}



void loop() {
}
*/

#include <Servo.h>

// === Définition des pins ===
#define SERVO_PIN A3
#define TRIG_AVANT 12
#define ECHO_AVANT 13
#define TRIG_ARRIERE 39
#define ECHO_ARRIERE 38

// === Définition des ServoMoteur===
Servo monServo;

Servo ServoBase;
Servo ServoArm;

int angleBase = 10;  // Angle actuel de ServoBase
int angleArm = 0;   // Angle actuel de ServoArm

// === Définition des capteurs et directions ===
int etat[4] = { 0, 0, 0, 0 };  // état global : [devant, droite, arrière, gauche]

int stateDroite = 0;
int stateGauche = 0;
int stateDevant = 0;
int stateArriere = 0;
long distDroite = 0;
long distDevant = 0;
long distGauche = 0;
long distArriere = 0;

// === Variable Chaine X ===
int accel_x = 0;
int accel_y = 0;
int Esc = 0;

// === Tag et Drapeau ===
float lastPredictionTag = 0.0;
bool predictionReceived = false;

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

// === TempoRecentrage ===
int TempoRecentrage = 150;

// === Définir le type Direction ===
enum Direction { UP,
                 RIGHT,
                 DOWN,
                 LEFT };
Direction priorites[4] = { UP, RIGHT, DOWN, LEFT };

// === Prototypes des fonctions ===
void toutDroit(int duree = 1500);
void tournerDroite90(int vitesseG = 90, int vitesseD = 95, int duree = 430);
void tournerGauche90(int vitesseD = 100, int vitesseG = 105, int duree = 525);
void demiTour180();
void arreter();
void correctionSuiviLigne();
void appliquerDeplacement(Direction dir);
void executerMouvement(Direction finale);

// Déplacement fluide d’un servo (utilisé pour les 2 servos)
void moveServo(Servo &servo, int &angleActuel, int angleFinal, int dureeMs) {
  int nombreDePas = abs(angleFinal - angleActuel);
  if (nombreDePas == 0) return;


  int delaiParPas = dureeMs / nombreDePas;
  int direction = (angleFinal > angleActuel) ? 1 : -1;


  for (int i = 0; i < nombreDePas; i++) {
    angleActuel += direction;
    servo.write(angleActuel);
    delay(delaiParPas);
  }
}


// Pour le servo de base
void DeployBase() {
  moveServo(ServoBase, angleBase, 65, 2000);
}


void HideBase() {
  moveServo(ServoBase, angleBase, 0, 2000);
}


// Pour le servo du bras
void DeployArm() {
  moveServo(ServoArm, angleArm, 140, 2500);
}


void HideArm() {
  moveServo(ServoArm, angleArm, 0, 2500);
}

void initialiserBras() {
  ServoBase.attach(27); // Signal du servo base sur D27
  ServoArm.attach(29);  // Signal du servo bras sur D29

  ServoBase.write(angleBase);
  ServoArm.write(angleArm);
  delay(500);

  DeployBase();
  delay(300);
  DeployArm();
  delay(500); // Laisse le bras stabilisé avant de lancer la suite
}

// === Fonctions utilitaires ===
String directionToString(Direction dir) {
  switch (dir) {
    case UP: return "UP";
    case RIGHT: return "RIGHT";
    case DOWN: return "DOWN";
    case LEFT: return "LEFT";
    default: return "UNKNOWN";
  }
}

Direction convertirSortieIA(float val) {
  if (val == 0.0) return UP;
  else if (val == 0.25) return RIGHT;
  else if (val == 0.5) return DOWN;
  else if (val == 0.75) return LEFT;
  else return UP;  // fallback
}

Direction filtrerPrediction(Direction prediction, int etat[]) {
  if (etat[prediction] == 0) {
    return prediction;
  }
  Serial.print("❌ Direction IA bloquée : ");
  Serial.println(directionToString(prediction));

  for (int i = 0; i < 4; i++) {
    Direction alt = priorites[i];
    if (etat[alt] == 0) {
      Serial.print("⚠️ Direction alternative : ");
      Serial.println(directionToString(alt));
      return alt;
    }
  }
  Serial.println("❌ Aucune direction libre !");
  return prediction;
}

void executerMouvement(Direction finale) {
  Serial.println("🚀 Exécution du déplacement...");

  switch (finale) {
    case UP:
      Serial.println("🟢 Avancer tout droit !");
      toutDroit();
      break;

    case RIGHT:
      Serial.println("🟡 Tourner à DROITE + Avancer !");
      tournerDroite90();
      delay(500);
      toutDroit();
      break;

    case DOWN:
      Serial.println("🔴 Demi-tour (180°) + Avancer !");
      demiTour180();
      delay(500);
      toutDroit();
      break;

    case LEFT:
      Serial.println("🔵 Tourner à GAUCHE + Avancer !");
      tournerGauche90();
      delay(500);
      toutDroit();
      break;
  }
}


void appliquerDeplacement(Direction dir) {
  switch (dir) {
    case UP:
      accel_y = -1;
      accel_x = 0;
      break;
    case DOWN:
      accel_y = 1;
      accel_x = 0;
      break;
    case RIGHT:
      accel_x = 1;
      accel_y = 0;
      break;
    case LEFT:
      accel_x = -1;
      accel_y = 0;
      break;
  }
}

void toutDroit(int duree = 1500) {
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


void tournerDroite90(int vitesseG = 90, int vitesseD = 95, int duree = 430) {
  delay(100);
  digitalWrite(DIR_B, HIGH);
  analogWrite(PWM_B, vitesseG);
  digitalWrite(DIR_A, LOW);
  analogWrite(PWM_A, vitesseD);
  delay(duree);
  arreter();
  delay(700);
  digitalWrite(DIR_B, LOW);
  analogWrite(PWM_B, 90);
  digitalWrite(DIR_A, LOW);
  analogWrite(PWM_A, 85);
  delay(TempoRecentrage);
  arreter();
}

void tournerGauche90(int vitesseD = 100, int vitesseG = 105, int duree = 525) {
  digitalWrite(DIR_B, LOW);
  analogWrite(PWM_B, vitesseG);
  digitalWrite(DIR_A, HIGH);
  analogWrite(PWM_A, vitesseD);
  delay(duree);
  arreter();
  delay(1000);
  digitalWrite(DIR_B, LOW);
  analogWrite(PWM_B, 100);
  digitalWrite(DIR_A, LOW);
  analogWrite(PWM_A, 90);
  delay(TempoRecentrage);
  arreter();
}

void demiTour180() {
  tournerGauche90();
  delay(500);
  tournerGauche90();
}

void arreter() {
  digitalWrite(DIR_B, LOW);
  analogWrite(PWM_B, 0);
  digitalWrite(DIR_A, LOW);
  analogWrite(PWM_A, 0);
}

void correctionSuiviLigne() {
  L_val = digitalRead(sensor_L);
  M_val = digitalRead(sensor_M);
  R_val = digitalRead(sensor_R);

  if (L_val == 1 && R_val == 0) {
    digitalWrite(DIR_B, LOW);
    analogWrite(PWM_B, 100);
    digitalWrite(DIR_A, HIGH);
    analogWrite(PWM_A, 130);
  } else if (L_val == 0 && R_val == 1) {
    digitalWrite(DIR_B, HIGH);
    analogWrite(PWM_B, 130);
    digitalWrite(DIR_A, LOW);
    analogWrite(PWM_A, 100);
  } else {
    digitalWrite(DIR_B, HIGH);
    analogWrite(PWM_B, 120);
    digitalWrite(DIR_A, HIGH);
    analogWrite(PWM_A, 120);
  }
}

void attendreTagIA() {
  unsigned long timeout = millis() + 5000;
  while (millis() < timeout) {
    if (Serial1.available()) {
      String messageESP = Serial1.readStringUntil('\n');
      if (messageESP.startsWith("TAG:")) {
        lastPredictionTag = messageESP.substring(4).toFloat();
        return;
      }
    }
  }
}

void envoyerMatriceVersESP() {
  String State_Brut = "{" + String(accel_x) + "," + String(accel_y) + "," + String(Esc) + "," + String(stateDevant) + "," + String(stateArriere) + "," + String(stateDroite) + "," + String(stateGauche) + "}";
  Serial1.println(State_Brut);
}
long lireDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duree = pulseIn(echoPin, HIGH, 20000);  // timeout 20 ms
  long distance = duree * 0.034 / 2;

  return distance;
}

void GenerateMatriceEntry() {
  monServo.write(5);
  delay(600);
  distDroite = lireDistance(TRIG_AVANT, ECHO_AVANT);
  stateDroite = (distDroite < 16) ? 1 : 0;
  delay(400);
  monServo.write(105);
  delay(600);
  distDevant = lireDistance(TRIG_AVANT, ECHO_AVANT);
  stateDevant = (distDevant < 12) ? 1 : 0;
  delay(400);
  monServo.write(180);
  delay(600);
  distGauche = lireDistance(TRIG_AVANT, ECHO_AVANT);
  stateGauche = (distGauche < 15) ? 1 : 0;
  delay(400);
  distArriere = lireDistance(TRIG_ARRIERE, ECHO_ARRIERE);
  stateArriere = (distArriere < 15) ? 1 : 0;
  delay(200);

  etat[0] = stateDevant;
  etat[1] = stateDroite;
  etat[2] = stateArriere;
  etat[3] = stateGauche;

  //envoyerMatriceVersESP();
}

/*void executerCycleIA() {
  for (int i = 0; i < 4; i++) {
    Serial.print("\n🔁 [Itération ");
    Serial.print(i + 1);
    Serial.println(" / 4]");

    GenerateMatriceEntry();
    attendreTagIA();
    Direction prediction = convertirSortieIA(lastPredictionTag);
    Direction finale = filtrerPrediction(prediction, etat);
    appliquerDeplacement(finale);
    executerMouvement(finale);
    delay(500);
  }
}*/

void executerCycleIA() {
  for (int i = 0; i < 4; i++) {
    Serial.print("\n🔁 [Itération ");
    Serial.print(i + 1);
    Serial.println(" / 4]");

    // Étape 1 : capter l’état (avant mouvement)
    GenerateMatriceEntry();               // Capteurs => état X
    envoyerMatriceVersESP();              // Envoi vers ESP

    // Étape 2 : attendre la prédiction
    attendreTagIA();                      // Attente du tag

    // Étape 3 : interpréter & appliquer la prédiction
    Direction prediction = convertirSortieIA(lastPredictionTag);
    Direction finale = filtrerPrediction(prediction, etat);
    appliquerDeplacement(finale);
    executerMouvement(finale);

    // Attendre avant prochaine boucle
    delay(300);
  }
}
void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  pinMode(DIR_B, OUTPUT);
  pinMode(PWM_B, OUTPUT);
  pinMode(DIR_A, OUTPUT);
  pinMode(PWM_A, OUTPUT);
  pinMode(sensor_L, INPUT);
  pinMode(sensor_M, INPUT);
  pinMode(sensor_R, INPUT);
  pinMode(TRIG_AVANT, OUTPUT);
  pinMode(ECHO_AVANT, INPUT);
  pinMode(TRIG_ARRIERE, OUTPUT);
  pinMode(ECHO_ARRIERE, INPUT);
  monServo.attach(SERVO_PIN);
  monServo.write(105);
  delay(500);

  initialiserBras();
  delay(1000);

  executerCycleIA();
  while (true); // 🛑 boucle infinie à la fin de setup(), bien placée

}




void loop() {}
