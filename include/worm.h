#ifndef WORM_H
#define WORM_H

#include <Arduino.h>

namespace WormGame {

  #define FRAME_X0 4
  #define FRAME_Y0 12
  #define FRAME_X1 123
  #define FRAME_Y1 92
  #define MAX_SEGMENTS 100
  #define FOOD_COUNT 3

  struct Segment {
    byte x;
    byte y;
  };

  Segment worm[MAX_SEGMENTS];
  byte currentSegments = 6;
  byte currentDirection = RIGHT;
  byte foodX[FOOD_COUNT];
  byte foodY[FOOD_COUNT];
  byte bonusX = 0, bonusY = 0;
  byte bonusSpawnTimer = 0;
  byte ghostTimer = 0;
  int currentScore = 0;
  boolean isGameOver = false;
  bool blinkToggle = false;

  // 10 Tingkatan Level (L0 s/d L9), naik sangat sabar setiap 25 poin (25 makanan)
  byte getLevel() {
    byte lvl = currentScore / 25;
    return (lvl > 9) ? 9 : lvl;
  }

  void growWorm(byte addCount) {
    byte tx = worm[currentSegments - 1].x;
    byte ty = worm[currentSegments - 1].y;
    while (addCount > 0 && currentSegments < MAX_SEGMENTS) {
      worm[currentSegments].x = tx;
      worm[currentSegments].y = ty;
      currentSegments++;
      addCount--;
    }
  }

  void drawHeader() {
    tv.draw_rect(4, 1, 120, 10, 0, 0);
    drawDigit(6, 3, (currentScore / 100) % 10);
    drawDigit(13, 3, (currentScore / 10) % 10);
    drawDigit(20, 3, currentScore % 10);

    // Indikator Level L0 s/d L9 (10 Level)
    tv.draw_line(44, 3, 44, 9, 1);
    tv.draw_line(44, 9, 48, 9, 1);
    drawDigit(51, 3, getLevel());

    if (ghostTimer > 0) {
      tv.draw_rect(70, 3, 50, 6, 1);
      byte barW = (ghostTimer > 48) ? 48 : ghostTimer;
      tv.draw_rect(71, 4, barW, 4, 1, 1);
    }
  }

  void drawFrame() {
    tv.draw_rect(FRAME_X0, FRAME_Y0, FRAME_X1 - FRAME_X0, FRAME_Y1 - FRAME_Y0, 1);
  }

  void spawnFood(byte idx) {
    foodX[idx] = (random((FRAME_X0 + 4) / 2, (FRAME_X1 - 6) / 2)) * 2;
    foodY[idx] = (random((FRAME_Y0 + 4) / 2, (FRAME_Y1 - 6) / 2)) * 2;
  }

  void drawFoods() {
    for (byte i = 0; i < FOOD_COUNT; i++) {
      tv.draw_rect(foodX[i], foodY[i], 1, 1, 1, 1);
      tv.set_pixel(foodX[i] + 1, foodY[i] - 1, 1);
    }
  }

  void drawBonusFood(byte color) {
    if (bonusSpawnTimer == 0) return;
    if (color == 0) {
      tv.draw_rect(bonusX - 1, bonusY - 1, 4, 4, 0, 0);
      return;
    }
    tv.draw_rect(bonusX, bonusY, 2, 2, 1);
    if (blinkToggle) {
      tv.set_pixel(bonusX - 1, bonusY + 1, 1);
      tv.set_pixel(bonusX + 3, bonusY + 1, 1);
      tv.set_pixel(bonusX + 1, bonusY - 1, 1);
      tv.set_pixel(bonusX + 1, bonusY + 3, 1);
    }
  }

  void drawHead(byte x, byte y, byte color) {
    if (color == 0) {
      tv.draw_rect(x - 1, y - 1, 3, 3, 0, 0);
      return;
    }
    tv.draw_rect(x, y, 1, 1, 1, 1);
    if (currentDirection == RIGHT || currentDirection == LEFT) {
      tv.set_pixel(x, y - 1, 1);
      tv.set_pixel(x, y + 2, 1);
    } else {
      tv.set_pixel(x - 1, y, 1);
      tv.set_pixel(x + 2, y, 1);
    }
  }

  void drawWorm() {
    for (byte i = 1; i < currentSegments; i++) {
      tv.draw_rect(worm[i].x, worm[i].y, 1, 1, 1, 1);
    }
    drawHead(worm[0].x, worm[0].y, 1);
  }

  void eraseTail() {
    byte tx = worm[currentSegments - 1].x;
    byte ty = worm[currentSegments - 1].y;
    tv.draw_rect(tx - 1, ty - 1, 3, 3, 0, 0);
  }

  void init() {
    tv.fill(0);
    isGameOver = false;
    currentScore = 0;
    currentSegments = 6;
    currentDirection = RIGHT;
    bonusSpawnTimer = 0;
    ghostTimer = 0;

    for (byte i = 0; i < currentSegments; i++) {
      worm[i].x = 40 - (i * 2);
      worm[i].y = 50;
    }

    for (byte i = 0; i < FOOD_COUNT; i++) {
      spawnFood(i);
    }

    drawFrame();
    drawHeader();
    drawFoods();
    drawWorm();
  }

  boolean checkForTailBite() {
    if (ghostTimer > 0) return false;
    for (byte i = 1; i < currentSegments; i++) {
      if (abs((int)worm[0].x - (int)worm[i].x) < 2 &&
          abs((int)worm[0].y - (int)worm[i].y) < 2) {
        return true;
      }
    }
    return false;
  }

  void shiftSegments() {
    for (byte i = currentSegments - 1; i > 0; i--) {
      worm[i].x = worm[i - 1].x;
      worm[i].y = worm[i - 1].y;
    }
  }

  void triggerGameOver() {
    isGameOver = true;
    drawGameOverBox();
  }

  void moveWorm() {
    drawHead(worm[0].x, worm[0].y, 0);
    tv.draw_rect(worm[0].x, worm[0].y, 1, 1, 1, 1);

    int nextX = worm[0].x;
    int nextY = worm[0].y;

    if (currentDirection == UP) nextY -= 2;
    else if (currentDirection == DOWN) nextY += 2;
    else if (currentDirection == LEFT) nextX -= 2;
    else if (currentDirection == RIGHT) nextX += 2;

    if (nextX >= FRAME_X1 - 2 || nextX <= FRAME_X0 + 1 ||
        nextY >= FRAME_Y1 - 2 || nextY <= FRAME_Y0 + 1) {
      if (ghostTimer > 0) {
        if (nextX >= FRAME_X1 - 2) nextX = FRAME_X0 + 2;
        else if (nextX <= FRAME_X0 + 1) nextX = FRAME_X1 - 4;
        if (nextY >= FRAME_Y1 - 2) nextY = FRAME_Y0 + 2;
        else if (nextY <= FRAME_Y0 + 1) nextY = FRAME_Y1 - 4;
        drawFrame();
        playTone(600, 10);
      } else {
        triggerGameOver();
        return;
      }
    }

    // 1. Makan Makanan Biasa: +1 Poin & +2 Segmen
    for (byte i = 0; i < FOOD_COUNT; i++) {
      if (abs(nextX - (int)foodX[i]) <= 2 && abs(nextY - (int)foodY[i]) <= 2) {
        tv.draw_rect(foodX[i] - 1, foodY[i] - 1, 3, 3, 0, 0);
        byte oldLvl = getLevel();
        shiftSegments();
        worm[0].x = nextX;
        worm[0].y = nextY;
        growWorm(2);
        if (currentScore < 999) currentScore += 1;
        spawnFood(i);

        if (getLevel() > oldLvl) {
          playLevelUpSound();
        } else {
          playTone(1046, 15);
        }

        drawHeader();

        if (bonusSpawnTimer == 0 && (currentScore % 10 == 0)) {
          bonusX = (random((FRAME_X0 + 6) / 2, (FRAME_X1 - 8) / 2)) * 2;
          bonusY = (random((FRAME_Y0 + 6) / 2, (FRAME_Y1 - 8) / 2)) * 2;
          bonusSpawnTimer = 65;
        }
        return;
      }
    }

    // 2. Makan Bonus Bintang: +3 Poin & +3 Segmen
    if (bonusSpawnTimer > 0 && abs(nextX - (int)bonusX) <= 3 && abs(nextY - (int)bonusY) <= 3) {
      drawBonusFood(0);
      bonusSpawnTimer = 0;
      ghostTimer = 85;
      growWorm(3);
      currentScore += 3;
      if (currentScore > 999) currentScore = 999;
      playTone(1400, 40);
      drawHeader();
    }

    shiftSegments();
    worm[0].x = nextX;
    worm[0].y = nextY;

    if (checkForTailBite()) {
      triggerGameOver();
    }
  }

  void update() {
    readPS2();
    if (isPS2Pressed(PS2_BTN_SELECT)) {
      activeGame = 0;
      return;
    }

    if (isGameOver) {
      tv.delay(16);
      if (isPS2Pressed(PS2_BTN_START)) {
        playTone(1046, 20);
        while (isPS2Pressed(PS2_BTN_START)) { tv.delay(16); readPS2(); }
        init();
      }
      return;
    }

    if (checkGlobalPause()) return;

    // Kurva kecepatan halus untuk 10 tingkatan (L0 s/d L9)
    byte lvl = getLevel();
    byte pollLoops = (lvl <= 4) ? (8 - lvl) : ((lvl <= 7) ? 3 : 2);
    byte stepDelay = (lvl <= 7) ? 16 : (22 - lvl);
    byte nextDir = currentDirection;

    for (byte i = 0; i < pollLoops; i++) {
      readPS2();
      if (isPS2Pressed(PS2_BTN_SELECT)) { activeGame = 0; return; }
      if (checkGlobalPause()) return;

      if (isPS2Pressed(PS2_BTN_UP) && currentDirection != DOWN) nextDir = UP;
      else if (isPS2Pressed(PS2_BTN_DOWN) && currentDirection != UP) nextDir = DOWN;
      else if (isPS2Pressed(PS2_BTN_LEFT) && currentDirection != RIGHT) nextDir = LEFT;
      else if (isPS2Pressed(PS2_BTN_RIGHT) && currentDirection != LEFT) nextDir = RIGHT;

      tv.delay(stepDelay);

      if (isPS2Pressed(PS2_BTN_CROSS)) {
        break;
      }
    }

    currentDirection = nextDir;
    blinkToggle = !blinkToggle;

    if (bonusSpawnTimer > 0) {
      drawBonusFood(0);
      bonusSpawnTimer--;
      if (bonusSpawnTimer > 0) drawBonusFood(1);
    }
    if (ghostTimer > 0) {
      ghostTimer--;
      if (ghostTimer % 2 == 0 || ghostTimer == 0) drawHeader();
    }

    eraseTail();
    moveWorm();
    if (!isGameOver) {
      drawFoods();
      drawWorm();
      if (ghostTimer > 0) drawFrame();
    }
  }

} // namespace WormGame

#endif