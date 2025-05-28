#ifndef ROTASECURITYBIB_H
#define ROTASECURITYBIB_H

#include <Arduino.h>

enum Direction { UP = 0, DOWN = 1, RIGHT = 2, LEFT = 3 };

class RotaSecurityBib {
  public:
    static String directionToString(Direction dir);
    static void ajusterCapteursPourRotation(int capteurs[], int rotations90Clockwise);
    static void reordonnerPourIA(int capteurs[], int entreeIA[]);
    static Direction convertirSortieIA(float val);
    static Direction filtrerPrediction(Direction prediction, int etat[]);
    static void appliquerDeplacement(Direction dir, int &x, int &y);

  private:
    static Direction priorites[4];
};

#endif
