#ifndef DODGE_H
#define DODGE_H

#include <Arduino.h>

namespace DodgeGame {

  #define MAX_METEORS 5

  struct DodgeState {
    byte playerX;
    byte runFrame;
    int8_t obsX[MAX_METEORS];
    int8_t obsY[MAX_METEORS];
    int8_t shieldX, shieldY;
    bool shieldItemActive;
    byte shieldTimer;
    int score;
    byte lives;
    byte level;
    bool isGameOver;
    bool flameToggle;
  };

  // Pinjam memori WormGame::worm (0 byte RAM tambahan!)
  DodgeState* st = (DodgeState*)WormGame::worm;
  const byte playerY = 82;

  void updateScoreAndLevel() {
    tv.draw_rect(4, 2, 20, 7, 0, 0);
    drawDigit(5, 2, (st->score / 100) % 10);
    drawDigit(12, 2, (st->score / 10) % 10);
    drawDigit(19, 2, st->score % 10);

    tv.draw_rect(58, 2, 6, 7, 0, 0);
    drawDigit(59, 2, st->level);
  }

  void updateShieldAndLives() {
    tv.draw_rect(80, 1, 44, 8, 0, 0);
    if (st->shieldTimer > 0) {
      tv.draw_circle(84, 5, 3, 1);
      tv.set_pixel(84, 5, 1);
    }
    for (byte i = 0; i < st->lives; i++) {
      tv.draw_rect(118 - (i * 6), 3, 3, 3, 1, 1);
    }
  }

  void drawHeader() {
    tv.draw_rect(3, 1, 122, 9, 0, 0);
    tv.draw_line(52, 2, 52, 8, 1);
    tv.draw_line(52, 8, 56, 8, 1);
    updateScoreAndLevel();
    updateShieldAndLives();
  }

  // Ornamen Planet Bercincin (Saturnus) di Langit Atas
  void drawSpaceOrnaments() {
    tv.draw_circle(108, 20, 3, 1);
    tv.draw_line(102, 20, 114, 20, 1);
    tv.set_pixel(20, 18, 1);
    tv.set_pixel(64, 16, 1);
  }

  void drawGround() {
    tv.draw_line(2, 91, 125, 91, 1);
    for (byte x = 8; x < 120; x += 14) {
      tv.draw_line(x, 93, x + 4, 93, 1);
      tv.set_pixel(x + 8, 94, 1);
    }
  }

  void drawRunner(byte x, byte y, byte color) {
    if (color == 0) {
      // Hapus presisi tanpa pernah menyentuh garis tanah y = 91!
      tv.draw_rect(x - 3, y - 3, 12, 10, 0, 0);
      return;
    }

    // Kepala & Badan
    tv.draw_rect(x + 2, y, 2, 2, 1, 1);
    tv.draw_line(x + 3, y + 2, x + 3, y + 5, 1);

    // Animasi Tangan & Kaki Berlari
    if (st->runFrame == 0) {
      tv.draw_line(x + 1, y + 3, x + 5, y + 3, 1);
      tv.draw_line(x + 2, y + 6, x + 1, y + 7, 1);
      tv.draw_line(x + 4, y + 6, x + 5, y + 7, 1);
    } else {
      tv.draw_line(x + 2, y + 3, x + 4, y + 4, 1);
      tv.draw_line(x + 2, y + 6, x + 3, y + 7, 1);
      tv.draw_line(x + 4, y + 6, x + 3, y + 7, 1);
    }

    // Kubah Perisai Kebal (Bagian atas dan samping saja agar tidak menabrak tanah)
    if (st->shieldTimer > 0 && (st->flameToggle || st->shieldTimer > 30)) {
      tv.draw_circle(x + 3, y + 2, 5, 1);
    }
  }

  void drawMeteor(int8_t x, int8_t y, byte color) {
    if (y < 14 || y > 85) return;
    if (color == 0) {
      tv.draw_rect(x - 1, y - 4, 6, 9, 0, 0);
      return;
    }
    tv.draw_rect(x, y, 4, 3, 1, 1);
    tv.draw_line(x + 1, y - 1, x + 3, y - 1, 1);
    tv.draw_line(x + 1, y + 4, x + 3, y + 4, 1);
    if (st->flameToggle) {
      tv.draw_line(x + 1, y - 3, x + 1, y - 2, 1);
      tv.set_pixel(x + 3, y - 4, 1);
    } else {
      tv.draw_line(x + 3, y - 3, x + 3, y - 2, 1);
      tv.set_pixel(x + 2, y - 4, 1);
    }
  }

  void resetObs(byte i) {
    st->obsX[i] = random(8, 116);
    st->obsY[i] = random(-45, -5);
  }

  void init() {
    tv.fill(0);
    st->score = 0;
    st->lives = 3;
    st->level = 1;
    st->playerX = 60;
    st->runFrame = 0;
    st->shieldItemActive = false;
    st->shieldTimer = 0;
    st->isGameOver = false;
    st->flameToggle = false;

    for (byte i = 0; i < MAX_METEORS; i++) {
      resetObs(i);
    }

    tv.draw_line(2, 11, 125, 11, 1);
    tv.draw_line(2, 11, 2, 95, 1);
    tv.draw_line(125, 11, 125, 95, 1);
    drawGround();
    drawHeader();
    drawSpaceOrnaments();
    drawRunner(st->playerX, playerY, 1);
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

    st->flameToggle = !st->flameToggle;

    // 1. IMMEDIATE ERASE-MOVE-DRAW KARAKTER (Bebas Kedip!)
    drawRunner(st->playerX, playerY, 0);

    bool moved = false;
    // Tombol X untuk Dash/Sprint menghindar meteor!
    byte step = isPS2Pressed(PS2_BTN_CROSS) ? 4 : 3;
    if (isPS2Pressed(PS2_BTN_LEFT) && st->playerX > 8) {
      st->playerX -= step;
      moved = true;
    }
    if (isPS2Pressed(PS2_BTN_RIGHT) && st->playerX < 114) {
      st->playerX += step;
      moved = true;
    }
    if (moved && st->flameToggle) {
      st->runFrame = !st->runFrame;
    } else if (!moved) {
      st->runFrame = 0;
    }

    drawRunner(st->playerX, playerY, 1);

    // 2. Durasi Perisai Kebal
    if (st->shieldTimer > 0) {
      st->shieldTimer--;
      if (st->shieldTimer == 0) updateShieldAndLives();
    }

    // 3. Kapsul Bonus Perisai (Shield Orb)
    if (st->shieldItemActive) {
      tv.draw_circle(st->shieldX, st->shieldY, 2, 0);
      st->shieldY += 2;
      if (st->shieldY >= playerY - 2 && st->shieldY <= playerY + 8 &&
          st->shieldX >= st->playerX - 2 && st->shieldX <= st->playerX + 8) {
        st->shieldItemActive = false;
        st->shieldTimer = 150;
        playTone(1200, 50);
        updateShieldAndLives();
      } else if (st->shieldY > 85) {
        st->shieldItemActive = false;
      } else {
        tv.draw_circle(st->shieldX, st->shieldY, 2, 1);
      }
    } else if (st->shieldTimer == 0 && random(0, 120) == 0) {
      st->shieldItemActive = true;
      st->shieldX = random(12, 114);
      st->shieldY = 16;
    }

    // 4. IMMEDIATE ERASE-MOVE-DRAW TIAP METEOR SATU PER SATU
    byte activeMeteors = (st->level >= 4) ? 5 : (st->level + 2);
    if (activeMeteors > MAX_METEORS) activeMeteors = MAX_METEORS;
    byte fallSpeed = (st->level >= 4) ? 3 : 2;

    for (byte i = 0; i < activeMeteors; i++) {
      drawMeteor(st->obsX[i], st->obsY[i], 0); // Hapus meteor ke-i
      st->obsY[i] += fallSpeed;               // Geser posisi

      if (st->obsY[i] > 85) {
        resetObs(i);
        st->score += 2;
        byte newLvl = (st->score / 50) + 1;
        if (newLvl > 5) newLvl = 5;
        if (newLvl != st->level) {
          st->level = newLvl;
          playLevelUpSound();
        }
        updateScoreAndLevel();
      } else if (st->obsY[i] + 4 >= playerY && st->obsY[i] <= playerY + 7 &&
                 st->obsX[i] + 4 >= st->playerX && st->obsX[i] <= st->playerX + 6) {
        resetObs(i);
        if (st->shieldTimer > 0) {
          st->score += 10;
          playTone(880, 25);
          updateScoreAndLevel();
        } else {
          playTone(150, 180);
          st->lives--;
          updateShieldAndLives();
          if (st->lives == 0) {
            st->isGameOver = true;
            drawGameOverBox();
            return;
          }
        }
      } else {
        drawMeteor(st->obsX[i], st->obsY[i], 1); // Langsung gambar kembali meteor ke-i!
      }
    }

    // Jaga ornamen planet langit tetap utuh jika sempat terlewati meteor
    if (st->flameToggle) drawSpaceOrnaments();

    tv.delay(22);
  }

} // namespace DodgeGame

#endif