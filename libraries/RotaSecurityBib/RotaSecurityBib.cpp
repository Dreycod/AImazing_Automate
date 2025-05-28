#include "RotaSecurityBib.h"

Direction RotaSecurityBib::priorites[4] = {UP, RIGHT, DOWN, LEFT};

String RotaSecurityBib::directionToString(Direction dir) {
  switch (dir) {
    case UP: return "UP";
    case DOWN: return "DOWN";
    case RIGHT: return "RIGHT";
    case LEFT: return "LEFT";
    default: return "UNKNOWN";
  }
}

void RotaSecurityBib::ajusterCapteursPourRotation(int capteurs[], int rotations90Clockwise) {
  int temp[4];
  for (int i = 0; i < 4; i++) {
    temp[(i + rotations90Clockwise) % 4] = capteurs[i];
  }
  for (int i = 0; i < 4; i++) {
    capteurs[i] = temp[i];
  }
}

void RotaSecurityBib::reordonnerPourIA(int capteurs[], int entreeIA[]) {
  entreeIA[0] = capteurs[0];
  entreeIA[1] = capteurs[2];
  entreeIA[2] = capteurs[1];
  entreeIA[3] = capteurs[3];
}

Direction RotaSecurityBib::convertirSortieIA(float val) {
  if (val == 0.0) return UP;
  else if (val == 0.25) return RIGHT;
  else if (val == 0.5) return DOWN;
  else if (val == 0.75) return LEFT;
  else return UP; // fallback
}

Direction RotaSecurityBib::filtrerPrediction(Direction prediction, int etat[]) {
  if (etat[prediction] == 0) {
    return prediction;
  }

  Serial.print("❌ Direction prédite : ");
  Serial.print(directionToString(prediction));
  Serial.println(" (bloquée)");

  for (int i = 0; i < 4; i++) {
    Direction alt = priorites[i];
    if (etat[alt] == 0) {
      Serial.print("⚠️ Alternative utilisée : ");
      Serial.println(directionToString(alt));
      return alt;
    }
  }

  Serial.println("❌ Aucune direction libre !");
  return prediction;
}

void RotaSecurityBib::appliquerDeplacement(Direction dir, int &x, int &y) {
  switch (dir) {
    case UP:    y = -1; x = 0; break;
    case DOWN:  y = 1;  x = 0; break;
    case RIGHT: x = 1;  y = 0; break;
    case LEFT:  x = -1; y = 0; break;
  }
}
