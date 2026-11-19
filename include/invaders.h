#ifndef INVADERS_H
#define INVADERS_H

#include <Arduino.h>

namespace InvadersGame {

  struct InvadersState {
    uint8_t aliens[3];
    int8_t formX, formY;
    int8_t formDir;
    byte moveTick, moveSpeed;
    byte swayStep;
    byte shipX;
    int8_t bulX[3], bulY[3];
    byte bulWep[3];
    byte weaponLvl;
    int8_t diveX, diveY, diveDx;
    bool diveActive;
    int8_t eBulX, eBulY;
    int8_t powX, powY;
    bool powActive;
    int score;
    byte lives;
    byte level; // 0 s/d 9 (10 Level)
    bool isGameOver;
    bool animFrame;
  };

  InvadersState* st = (InvadersState*)WormGame::worm;
  const byte shipY = 85;

  void drawHeader() {
    tv.draw_rect(3, 1, 122, 9, 0, 0);
    drawDigit(5, 2, (st->score / 100) % 10);
    drawDigit(12, 2, (st->score / 10) % 10);
    drawDigit(19, 2, st->score % 10);

    // Level L0 s/d L9 (10 Level)
    tv.draw_line(52, 2, 52, 8, 1);
    tv.draw_line(52, 8, 56, 8, 1);
    drawDigit(59, 2, st->level);

    tv.draw_rect(78, 2, 13, 6, 1);
    for (byte w = 0; w < st->weaponLvl; w++) {
      tv.draw_line(81 + (w * 3), 4, 81 + (w * 3), 6, 1);
    }

    for (byte i = 0; i < st->lives; i++) {
      tv.draw_rect(118 - (i * 6), 3, 3, 3, 1, 1);
    }
  }

  void drawShip(byte x, byte y, byte color) {
    tv.set_pixel(x + 5, y, color);
    tv.draw_line(x + 4, y + 1, x + 6, y + 1, color);
    tv.draw_line(x + 4, y + 2, x + 6, y + 2, color);
    tv.set_pixel(x, y + 2, color);
    tv.set_pixel(x + 10, y + 2, color);
    tv.draw_line(x, y + 3, x + 10, y + 3, color);
    tv.draw_line(x, y + 4, x + 10, y + 4, color);
    tv.draw_line(x + 2, y + 5, x + 4, y + 5, color);
    tv.draw_line(x + 6, y + 5, x + 8, y + 5, color);
    if (color == 1 && st->animFrame) {
      tv.set_pixel(x + 3, y + 6, 1);
      tv.set_pixel(x + 7, y + 6, 1);
    } else {
      tv.set_pixel(x + 3, y + 6, 0);
      tv.set_pixel(x + 7, y + 6, 0);
    }
  }

  void drawAlienSprite(int8_t x, int8_t y, byte color, bool frame, byte row) {
    if (y < 12 || y > 88 || x < 4 || x > 116) return;
    if (color == 0) {
      tv.draw_rect(x, y, 6, 4, 0, 0);
      return;
    }
    if (row == 0) {
      tv.set_pixel(x + 1, y, 1); tv.set_pixel(x + 5, y, 1);
    } else {
      tv.draw_line(x + 1, y, x + 5, y, 1);
    }
    tv.draw_line(x, y + 1, x + 6, y + 1, 1);
    tv.set_pixel(x + 2, y + 1, 0);
    tv.set_pixel(x + 4, y + 1, 0);
    tv.draw_line(x + 1, y + 2, x + 5, y + 2, 1);
    if (frame) {
      tv.set_pixel(x, y + 3, 1);
      if (row != 2) tv.set_pixel(x + 3, y + 3, 1);
      tv.set_pixel(x + 6, y + 3, 1);
    } else {
      tv.set_pixel(x + 1, y + 3, 1);
      tv.set_pixel(x + 5, y + 3, 1);
    }
  }

  void drawFormation(byte color) {
    for (byte r = 0; r < 3; r++) {
      for (byte c = 0; c < 6; c++) {
        if (st->aliens[r] & (1 << c)) {
          int8_t waveOffset = ((st->level >= 2) && (st->level % 2 == 0) && (c & 1)) ? 2 : 0;
          int8_t ax = st->formX + (c * 14);
          int8_t ay = st->formY + (r * 8) + waveOffset;
          drawAlienSprite(ax, ay, color, st->animFrame, r);
        }
      }
    }
  }

  void initWave() {
    for (byte r = 0; r < 3; r++) {
      if (st->level == 1 && r == 1) st->aliens[r] = 0x33;
      else st->aliens[r] = 0x3F;
    }
    st->formX = 16;
    st->formY = 14 + (st->level / 2);
    st->formDir = 1; // Geser halus 1px per langkah agar tidak terlalu liar
    st->moveTick = 0;
    st->swayStep = 0;
    // Level 0 (pertama): sangat pelan (moveSpeed = 9), makin cepat perlahan di level tinggi
    st->moveSpeed = (st->level >= 7) ? 2 : (9 - st->level);
    st->diveActive = false;
    st->powActive = false;
    st->eBulY = -1;
    for (byte b = 0; b < 3; b++) st->bulY[b] = -1;
    drawFormation(1);
    drawHeader();
  }

  void init() {
    tv.fill(0);
    st->score = 0;
    st->lives = 3;
    st->level = 0;
    st->shipX = 58;
    st->weaponLvl = 1;
    st->isGameOver = false;
    st->animFrame = false;

    tv.draw_line(2, 11, 125, 11, 1);
    tv.draw_line(2, 11, 2, 94, 1);
    tv.draw_line(125, 11, 125, 94, 1);
    tv.draw_line(2, 94, 125, 94, 1);

    initWave();
    drawShip(st->shipX, shipY, 1);
  }

  void drawBullet(byte b, byte color) {
    byte w = st->bulWep[b];
    int8_t x = st->bulX[b];
    int8_t y = st->bulY[b];
    if (w == 1 || w == 3) {
      tv.draw_line(x + 5, y, x + 5, y + 2, color);
    }
    if (w >= 2) {
      tv.draw_line(x, y, x, y + 2, color);
      tv.draw_line(x + 10, y, x + 10, y + 2, color);
    }
  }

  bool checkHitAt(int8_t bx, int8_t by) {
    // 1. Cek Alien Kamikaze (+3 Poin)
    if (st->diveActive && bx >= st->diveX - 1 && bx <= st->diveX + 7 &&
        by >= st->diveY && by <= st->diveY + 5) {
      drawAlienSprite(st->diveX, st->diveY, 0, false, 0);
      st->diveActive = false;
      st->score += 3;
      if (st->score > 999) st->score = 999;
      playTone(650, 25);
      if (!st->powActive && st->weaponLvl < 3) {
        st->powActive = true;
        st->powX = st->diveX + 2;
        st->powY = st->diveY;
      }
      drawHeader();
      return true;
    }

    // 2. Cek Barisan Alien (+1 Poin per alien!)
    for (byte r = 0; r < 3; r++) {
      for (byte c = 0; c < 6; c++) {
        if (st->aliens[r] & (1 << c)) {
          int8_t waveOffset = ((st->level >= 2) && (st->level % 2 == 0) && (c & 1)) ? 2 : 0;
          int8_t ax = st->formX + (c * 14);
          int8_t ay = st->formY + (r * 8) + waveOffset;
          if (bx >= ax && bx <= ax + 6 && by >= ay && by <= ay + 4) {
            st->aliens[r] &= ~(1 << c);
            drawAlienSprite(ax, ay, 0, false, r);
            if (st->score < 999) st->score += 1;
            playTone(450, 15);
            if (!st->powActive && st->weaponLvl < 3 && random(0, 5) == 0) {
              st->powActive = true;
              st->powX = ax + 2;
              st->powY = ay;
            }
            drawHeader();
            return true;
          }
        }
      }
    }
    return false;
  }

  void playerHit() {
    drawShip(st->shipX, shipY, 0);
    if (st->eBulY >= 0) { tv.draw_line(st->eBulX, st->eBulY, st->eBulX, st->eBulY + 2, 0); st->eBulY = -1; }
    if (st->weaponLvl > 1) st->weaponLvl--;
    st->lives--;
    drawHeader();
    playTone(150, 250);
    if (st->lives == 0) {
      st->isGameOver = true;
      drawGameOverBox();
    } else {
      tv.delay(400);
      st->shipX = 58;
    }
  }

  void update() {
    readPS2();
    if (isPS2Pressed(PS2_BTN_SELECT)) {
      activeGame = 0;
      return;
    }

    if (st->isGameOver) {
      tv.delay(16);
      if (isPS2Pressed(PS2_BTN_START)) {
        playTone(1046, 20);
        while (isPS2Pressed(PS2_BTN_START)) { tv.delay(16); readPS2(); }
        init();
      }
      return;
    }

    if (checkGlobalPause()) return;

    drawShip(st->shipX, shipY, 0);
    if (isPS2Pressed(PS2_BTN_LEFT) && st->shipX > 5) st->shipX -= 3;
    if (isPS2Pressed(PS2_BTN_RIGHT) && st->shipX < 111) st->shipX += 3;

    static byte fireCooldown = 0;
    if (fireCooldown > 0) fireCooldown--;
    if (isPS2Pressed(PS2_BTN_CROSS) && fireCooldown == 0) {
      for (byte b = 0; b < 3; b++) {
        if (st->bulY[b] < 0) {
          st->bulWep[b] = st->weaponLvl;
          st->bulX[b] = st->shipX;
          st->bulY[b] = shipY - 3;
          fireCooldown = 6;
          playTone(950 + (st->weaponLvl * 100), 10);
          break;
        }
      }
    }

    for (byte b = 0; b < 3; b++) {
      if (st->bulY[b] >= 0) {
        drawBullet(b, 0);
        st->bulY[b] -= 4;
        if (st->bulY[b] < 13) {
          st->bulY[b] = -1;
        } else {
          byte w = st->bulWep[b];
          bool hit = false;
          if (w == 1 || w == 3) hit = checkHitAt(st->bulX[b] + 5, st->bulY[b]);
          if (!hit && w >= 2)   hit = checkHitAt(st->bulX[b], st->bulY[b]);
          if (!hit && w >= 2)   hit = checkHitAt(st->bulX[b] + 10, st->bulY[b]);
          if (hit) st->bulY[b] = -1;
          else drawBullet(b, 1);
        }
      }
    }

    if (st->eBulY >= 0) {
      tv.draw_line(st->eBulX, st->eBulY, st->eBulX, st->eBulY + 2, 0);
      st->eBulY += (st->level >= 5) ? 3 : 2;
      if (st->eBulY >= shipY && st->eBulY <= shipY + 5 &&
          st->eBulX >= st->shipX && st->eBulX <= st->shipX + 10) {
        st->eBulY = -1;
        playerHit();
        if (st->isGameOver) return;
      } else if (st->eBulY > 90) {
        st->eBulY = -1;
      } else {
        tv.draw_line(st->eBulX, st->eBulY, st->eBulX, st->eBulY + 2, 1);
      }
    }

    if (st->powActive) {
      tv.draw_rect(st->powX, st->powY, 4, 4, 0, 0);
      st->powY += 1;
      if (st->powY >= shipY - 2 && st->powY <= shipY + 5 &&
          st->powX + 4 >= st->shipX && st->powX <= st->shipX + 10) {
        st->powActive = false;
        if (st->weaponLvl < 3) st->weaponLvl++;
        st->score += 2;
        if (st->score > 999) st->score = 999;
        playTone(1350, 50);
        drawHeader();
      } else if (st->powY > 88) {
        st->powActive = false;
      } else {
        tv.draw_rect(st->powX, st->powY, 4, 4, 1);
        tv.set_pixel(st->powX + 2, st->powY + 2, 1);
      }
    }

    st->moveTick++;
    if (st->moveTick >= st->moveSpeed) {
      st->moveTick = 0;
      drawFormation(0);

      st->animFrame = !st->animFrame;
      st->formX += st->formDir;
      st->swayStep++;

      // Gerakan mengayun naik-turun baru muncul mulai Level 3 ke atas
      if (st->level >= 3 && (st->swayStep % 6 == 0)) {
        st->formY += (st->swayStep & 6) ? 1 : -1;
      }

      if (st->formX <= 6 || st->formX >= 42) {
        st->formDir = -st->formDir;
        st->formY += 2; // Turun pelan 2px saat mentok pinggir
      }

      drawFormation(1);

      // Kurva kesulitan bertahap:
      // Level 0 (Pertama): Tidak ada tembakan musuh & tidak ada kamikaze
      // Level 1 - 2: Alien mulai menembak pelan, kamikaze belum ada
      // Level 3 ke atas: Kamikaze mulai muncul bertahap
      if (st->level >= 1) {
        for (int8_t r = 2; r >= 0; r--) {
          byte c = random(0, 6);
          if (st->aliens[r] & (1 << c)) {
            int8_t ax = st->formX + (c * 14);
            int8_t ay = st->formY + (r * 8);
            if (st->eBulY < 0 && random(0, 12) < st->level) {
              st->eBulX = ax + 3;
              st->eBulY = ay + 5;
              playTone(220, 10);
            }
            if (st->level >= 3 && !st->diveActive && random(0, 14) < (st->level - 2)) {
              st->aliens[r] &= ~(1 << c);
              drawAlienSprite(ax, ay, 0, false, r);
              st->diveActive = true;
              st->diveX = ax;
              st->diveY = ay;
              st->diveDx = (st->shipX > ax) ? 1 : -1;
              playTone(320, 15);
            }
            break;
          }
        }
      }

      for (byte r = 0; r < 3; r++) {
        if (st->aliens[r] && (st->formY + (r * 8) >= shipY - 4)) {
          playerHit();
          if (!st->isGameOver) initWave();
          return;
        }
      }
    }

    if (st->diveActive) {
      drawAlienSprite(st->diveX, st->diveY, 0, false, 0);
      st->diveY += (st->level >= 6) ? 3 : 2;
      st->diveX += st->diveDx;
      if (st->diveX <= 6 || st->diveX >= 114 || random(0, 8) == 0) {
        st->diveDx = (st->shipX > st->diveX) ? 1 : -1;
      }
      if (st->level >= 5 && st->eBulY < 0 && st->diveY > 30 && st->diveY < 55) {
        st->eBulX = st->diveX + 3;
        st->eBulY = st->diveY + 5;
      }

      if (st->diveY + 4 >= shipY && st->diveY <= shipY + 5 &&
          st->diveX + 6 >= st->shipX && st->diveX <= st->shipX + 10) {
        st->diveActive = false;
        playerHit();
        if (st->isGameOver) return;
      } else if (st->diveY > 89) {
        st->diveActive = false;
      } else {
        drawAlienSprite(st->diveX, st->diveY, 1, st->animFrame, 0);
      }
    }

    if (st->aliens[0] == 0 && st->aliens[1] == 0 && st->aliens[2] == 0 && !st->diveActive) {
      if (st->powActive) { tv.draw_rect(st->powX, st->powY, 4, 4, 0, 0); st->powActive = false; }
      if (st->eBulY >= 0) { tv.draw_line(st->eBulX, st->eBulY, st->eBulX, st->eBulY + 2, 0); st->eBulY = -1; }
      playLevelUpSound();
      tv.delay(400);
      st->level++;
      if (st->level > 9) st->level = 9;
      initWave();
    }

    drawShip(st->shipX, shipY, 1);
    tv.delay(18);
  }

} // namespace InvadersGame

#endif