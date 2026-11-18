#ifndef RUN_H
#define RUN_H

#include <Arduino.h>

namespace RunGame {

  struct RunState {
    int8_t dinoY;
    int8_t vy;
    bool isJumping;
    bool isDucking;
    int16_t obsX;
    byte obsType; // 0 = Kaktus Kecil, 1 = Kaktus Ganda, 2 = Burung Terbang
    byte cloudX[2];
    byte groundOffset;
    int score;
    byte lives;
    byte level;
    bool isGameOver;
    bool animFrame;
  };

  // Pinjam memori WormGame::worm (0 byte RAM tambahan!)
  RunState* st = (RunState*)WormGame::worm;
  const byte groundY = 86;

  // Pembaruan parsial Skor & Level (Tidak menghapus seluruh bar atas!)
  void updateScoreAndLevel() {
    tv.draw_rect(4, 2, 20, 7, 0, 0);
    drawDigit(5, 2, (st->score / 100) % 10);
    drawDigit(12, 2, (st->score / 10) % 10);
    drawDigit(19, 2, st->score % 10);

    tv.draw_rect(62, 2, 6, 7, 0, 0);
    drawDigit(63, 2, st->level);
  }

  void updateLives() {
    tv.draw_rect(98, 2, 25, 6, 0, 0);
    for (byte i = 0; i < st->lives; i++) {
      tv.draw_rect(118 - (i * 6), 3, 3, 3, 1, 1);
    }
  }

  void drawHeader() {
    tv.draw_rect(3, 1, 122, 9, 0, 0);
    tv.draw_line(56, 2, 56, 8, 1);
    tv.draw_line(56, 8, 60, 8, 1);
    updateScoreAndLevel();
    updateLives();
  }

  // Hanya menggambar Awan (Bulan & Bintang dipisah agar tidak berkedip!)
  void drawClouds(byte color) {
    for (byte i = 0; i < 2; i++) {
      byte cx = st->cloudX[i];
      byte cy = (i == 0) ? 20 : 28;
      tv.draw_line(cx, cy, cx + 8, cy, color);
      tv.draw_line(cx + 2, cy - 2, cx + 6, cy - 2, color);
      tv.draw_line(cx + 1, cy - 1, cx + 7, cy - 1, color);
    }
  }

  // Ornamen Statis Bulan Sabit & Bintang Gurun Malam
  void drawStaticSky() {
    tv.draw_circle(108, 22, 4, 1);
    tv.set_pixel(106, 21, 1);
    tv.set_pixel(45, 16, 1);
    tv.set_pixel(88, 18, 1);
  }

  void drawGround() {
    tv.draw_line(3, groundY, 124, groundY, 1);
    for (byte x = 6; x < 120; x += 12) {
      byte gx = x + st->groundOffset;
      tv.draw_line(gx, groundY + 2, gx + 4, groundY + 2, 0);
      tv.set_pixel(gx + 1, groundY + 2, 1);
      tv.set_pixel(gx + 6, groundY + 3, 1);
    }
  }

  void drawTrex(int8_t y, bool duck, byte color) {
    byte x = 16;
    if (color == 0) {
      // Tinggi hapus dikunci 9px (y-9 s/d y) agar TIDAK PERNAH menghapus garis tanah (groundY = 86)!
      tv.draw_rect(x - 1, y - 9, 12, 9, 0, 0);
      return;
    }

    if (!duck) {
      // Pose Berdiri / Lompat
      tv.draw_rect(x + 4, y - 9, 4, 3, 1, 1);
      tv.set_pixel(x + 5, y - 8, 0);
      tv.draw_rect(x + 2, y - 5, 4, 3, 1, 1);
      tv.draw_line(x, y - 6, x + 1, y - 4, 1);
      tv.set_pixel(x + 7, y - 4, 1);
      if (st->isJumping || st->animFrame) {
        tv.draw_line(x + 2, y - 2, x + 2, y, 1);
        tv.set_pixel(x + 5, y - 2, 1);
      } else {
        tv.set_pixel(x + 2, y - 2, 1);
        tv.draw_line(x + 5, y - 2, x + 5, y, 1);
      }
    } else {
      // Pose Menunduk (Ducking)
      tv.draw_rect(x, y - 4, 7, 2, 1, 1);
      tv.draw_rect(x + 7, y - 5, 4, 3, 1, 1);
      tv.set_pixel(x + 8, y - 4, 0);
      if (st->animFrame) {
        tv.draw_line(x + 3, y - 1, x + 3, y, 1);
      } else {
        tv.draw_line(x + 6, y - 1, x + 6, y, 1);
      }
    }
  }

  void drawObstacle(int16_t ox, byte type, byte color) {
    if (ox < 4 || ox > 116) return;
    byte x = (byte)ox;

    if (type == 0) {
      // Kaktus Kecil (Hapus hanya sampai groundY - 1 agar garis tanah tidak terputus)
      if (color == 0) { tv.draw_rect(x, groundY - 8, 5, 7, 0, 0); return; }
      tv.draw_line(x + 2, groundY - 7, x + 2, groundY - 1, 1);
      tv.draw_line(x, groundY - 5, x, groundY - 3, 1);
      tv.draw_line(x + 4, groundY - 6, x + 4, groundY - 4, 1);
      tv.draw_line(x, groundY - 3, x + 4, groundY - 4, 1);
    } else if (type == 1) {
      // Kaktus Ganda
      if (color == 0) { tv.draw_rect(x, groundY - 9, 8, 8, 0, 0); return; }
      tv.draw_rect(x + 1, groundY - 8, 2, 7, 1, 1);
      tv.draw_rect(x + 5, groundY - 6, 2, 5, 1, 1);
      tv.draw_line(x, groundY - 5, x + 7, groundY - 4, 1);
    } else {
      // Burung Pterodactyl Terbang Rendah
      byte by = groundY - 9;
      if (color == 0) { tv.draw_rect(x, by - 3, 9, 6, 0, 0); return; }
      tv.draw_line(x, by, x + 3, by, 1);
      tv.draw_rect(x + 3, by - 1, 5, 2, 1, 1);
      if (st->animFrame) {
        tv.draw_line(x + 4, by - 3, x + 6, by - 1, 1);
      } else {
        tv.draw_line(x + 4, by + 3, x + 6, by + 1, 1);
      }
    }
  }

  void spawnNextObstacle() {
    st->obsX = 115;
    if (st->level == 1 && st->score < 15) {
      st->obsType = random(0, 2);
    } else {
      st->obsType = random(0, 3);
    }
  }

  void init() {
    tv.fill(0);
    st->score = 0;
    st->lives = 3;
    st->level = 1;
    st->dinoY = groundY - 1;
    st->vy = 0;
    st->isJumping = false;
    st->isDucking = false;
    st->cloudX[0] = 25;
    st->cloudX[1] = 75;
    st->groundOffset = 0;
    st->isGameOver = false;
    st->animFrame = false;

    tv.draw_line(2, 11, 125, 11, 1);
    tv.draw_line(2, 11, 2, 94, 1);
    tv.draw_line(125, 11, 125, 94, 1);
    tv.draw_line(2, 94, 125, 94, 1);

    spawnNextObstacle();
    drawHeader();
    drawStaticSky();
    drawClouds(1);
    drawGround();
    drawTrex(st->dinoY, st->isDucking, 1);
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

    st->animFrame = !st->animFrame;

    // 1. IMMEDIATE ERASE-MOVE-DRAW T-REX (Bebas Kedip!)
    drawTrex(st->dinoY, st->isDucking, 0);

    bool downPressed = isPS2Pressed(PS2_BTN_DOWN);
    bool jumpPressed = isPS2Pressed(PS2_BTN_UP) || isPS2Pressed(PS2_BTN_CROSS);

    if (st->isJumping) {
      st->isDucking = false;
      if (downPressed) st->vy += 2;
    } else {
      st->isDucking = downPressed;
      if (jumpPressed && !st->isDucking) {
        st->vy = -6;
        st->isJumping = true;
        playTone(650, 15);
      }
    }

    if (st->isJumping) {
      st->dinoY += st->vy;
      st->vy += 1;
      if (st->dinoY >= groundY - 1) {
        st->dinoY = groundY - 1;
        st->isJumping = false;
        st->vy = 0;
      }
    }

    drawTrex(st->dinoY, st->isDucking, 1);

    // 2. IMMEDIATE ERASE-MOVE-DRAW RINTANGAN & KALIBRASI LEVEL KECEPATAN
    drawObstacle(st->obsX, st->obsType, 0);

    // Kecepatan santai di awal (Level 1-2 = 2px, Level 3-5 = 3px, Level 6 = 4px)
    byte speed = 2 + (st->level / 3);
    st->obsX -= speed;

    if (st->obsX < 5) {
      st->score += 5;
      byte newLvl = (st->score / 30) + 1;
      if (newLvl > 6) newLvl = 6;
      if (newLvl != st->level) {
        st->level = newLvl;
        playLevelUpSound();
      }
      updateScoreAndLevel();
      spawnNextObstacle();
    }

    // 3. Deteksi Tabrakan Presisi
    bool hit = false;
    if (st->obsX >= 11 && st->obsX <= 25) {
      if (st->obsType == 2) {
        if (!st->isDucking && st->dinoY >= groundY - 4) hit = true;
      } else {
        if (st->dinoY >= groundY - 6) hit = true;
      }
    }

    if (hit) {
      playTone(150, 180);
      st->lives--;
      updateLives();
      spawnNextObstacle();
      if (st->lives == 0) {
        drawGround();
        st->isGameOver = true;
        drawGameOverBox();
        return;
      }
    } else {
      drawObstacle(st->obsX, st->obsType, 1);
    }

    // 4. Gerakkan Awan & Kerikil Tanah Tanpa Mengganggu Bulan
    if (st->animFrame) {
      drawClouds(0);
      for (byte i = 0; i < 2; i++) {
        st->cloudX[i]--;
        if (st->cloudX[i] < 6) st->cloudX[i] = 92;
      }
      drawClouds(1);
    }
    st->groundOffset = (st->groundOffset + 2) % 6;
    drawGround();

    tv.delay(32);
  }

} // namespace RunGame

#endif