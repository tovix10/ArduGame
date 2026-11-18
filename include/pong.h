#ifndef PONG_H
#define PONG_H

#include <Arduino.h>

namespace PongGame {

  #define MAX_SCORE 11
  #define PADDLE_WIDTH 2
  #define PADDLE_HEIGHT 14
  #define BALL_SIZE 3 // Bola 4x4 piksel

  // Batas Lapangan Stadion Simetris
  #define COURT_LEFT   4
  #define COURT_RIGHT  123
  #define COURT_TOP    16
  #define COURT_BOTTOM 93

  int ballx, bally;
  int8_t dx, dy;
  byte paddleAy = 48; // AI (Kiri)
  byte paddleBy = 48; // Pemain PS2 (Kanan)
  const byte paddleAx = 8;
  const byte paddleBx = 118;
  byte score = 0;     // Skor AI
  byte score2 = 0;    // Skor Pemain
  byte rallyCount = 0;
  byte speedLevel = 1;
  boolean isGameOver = false;

  void updateSpeedBadge() {
    tv.draw_rect(104, 3, 14, 8, 0, 0);
    // Tampilkan indikator "L1" - "L3" di kanan atas saat reli meningkat
    tv.draw_line(105, 4, 105, 10, 1);
    tv.draw_line(105, 10, 109, 10, 1);
    drawDigit(112, 4, speedLevel);
  }

  void drawScore() {
    tv.draw_rect(20, 1, 102, 13, 0, 0);

    // Bingkai Papan Skor Tengah Atas
    tv.draw_rect(44, 1, 40, 12, 1);
    tv.draw_line(64, 1, 64, 13, 1);

    // Skor AI (Kiri)
    drawDigit(48, 4, (score / 10) % 10);
    drawDigit(55, 4, score % 10);

    // Skor Pemain (Kanan)
    drawDigit(68, 4, (score2 / 10) % 10);
    drawDigit(75, 4, score2 % 10);

    // Ornamen lampu stadion di kiri & kanan papan skor
    tv.draw_rect(34, 4, 6, 6, 1); tv.set_pixel(37, 7, 1);
    tv.draw_rect(88, 4, 6, 6, 1); tv.set_pixel(91, 7, 1);

    updateSpeedBadge();
  }

  // Hanya menggambar garis tengah & lingkaran tengah (Tanpa menimpa dinding atas/bawah!)
  void refreshCenterCourt() {
    for (byte y = COURT_TOP + 3; y < COURT_BOTTOM - 3; y += 6) {
      tv.draw_line(64, y, 64, y + 3, 1);
    }
    tv.draw_circle(64, 54, 10, 1);
    tv.set_pixel(64, 54, 1); // Titik kick-off tengah stadion
  }

  void drawCourt() {
    tv.draw_rect(COURT_LEFT, COURT_TOP, COURT_RIGHT - COURT_LEFT, COURT_BOTTOM - COURT_TOP, 1);
    // Ornamen Sudut Stadion (Corner Arcs)
    tv.set_pixel(COURT_LEFT + 2, COURT_TOP + 1, 1);
    tv.set_pixel(COURT_LEFT + 1, COURT_TOP + 2, 1);
    tv.set_pixel(COURT_RIGHT - 2, COURT_TOP + 1, 1);
    tv.set_pixel(COURT_RIGHT - 1, COURT_TOP + 2, 1);
    tv.set_pixel(COURT_LEFT + 2, COURT_BOTTOM - 1, 1);
    tv.set_pixel(COURT_LEFT + 1, COURT_BOTTOM - 2, 1);
    tv.set_pixel(COURT_RIGHT - 2, COURT_BOTTOM - 1, 1);
    tv.set_pixel(COURT_RIGHT - 1, COURT_BOTTOM - 2, 1);
    refreshCenterCourt();
  }

  void drawPaddle(byte x, byte y) {
    tv.draw_line(x, COURT_TOP + 2, x, COURT_BOTTOM - 2, 0);
    tv.draw_line(x + 1, COURT_TOP + 2, x + 1, COURT_BOTTOM - 2, 0);
    tv.draw_line(x, y, x, y + PADDLE_HEIGHT, 1);
    tv.draw_line(x + 1, y, x + 1, y + PADDLE_HEIGHT, 1);
  }

  void setBallPixels(byte color) {
    tv.draw_rect(ballx, bally, BALL_SIZE, BALL_SIZE, color, color);
  }

  void resetBall(int8_t dirX) {
    ballx = 63;
    bally = random(COURT_TOP + 10, COURT_BOTTOM - 12);
    dx = dirX;
    dy = (random(0, 2) == 0) ? 1 : -1;
    rallyCount = 0;
    speedLevel = 1;
    updateSpeedBadge();
    drawCourt();
  }

  void init() {
    tv.fill(0);
    score = 0;
    score2 = 0;
    paddleAy = 48;
    paddleBy = 48;
    isGameOver = false;
    drawScore();
    resetBall(1);
    drawPaddle(paddleAx, paddleAy);
    drawPaddle(paddleBx, paddleBy);
  }

  void updateAI() {
    if (dx < 0 && ballx < 85) {
      byte center = paddleAy + (PADDLE_HEIGHT / 2);
      byte aiStep = (speedLevel >= 3) ? 3 : 2;
      if (bally < center - 2 && paddleAy > COURT_TOP + 3) paddleAy -= aiStep;
      else if (bally > center + 2 && paddleAy < (COURT_BOTTOM - PADDLE_HEIGHT - 3)) paddleAy += aiStep;
    }
  }

  void updatePlayer() {
    // Tahan tombol X untuk menggerakkan paddle sedikit lebih cepat (Sprint Paddle)
    byte pStep = isPS2Pressed(PS2_BTN_CROSS) ? 3 : 2;
    if (isPS2Pressed(PS2_BTN_UP) && paddleBy > COURT_TOP + 3) {
      paddleBy -= pStep;
    } else if (isPS2Pressed(PS2_BTN_DOWN) && paddleBy < (COURT_BOTTOM - PADDLE_HEIGHT - 3)) {
      paddleBy += pStep;
    }
  }

  void missSound() {
    for (byte i = 0; i < 6; i++) {
      tv.delay(15);
      playTone(246, 15);
    }
  }

  void triggerGameOver() {
    isGameOver = true;
    drawGameOverBox();
  }

  void checkRallyLevelUp() {
    rallyCount++;
    byte newSpd = 1;
    if (rallyCount >= 8) newSpd = 3;
    else if (rallyCount >= 4) newSpd = 2;

    if (newSpd != speedLevel) {
      speedLevel = newSpd;
      updateSpeedBadge();
      playTone(784, 25);
    } else {
      playTone(493, 20);
    }
  }

  void moveBall() {
    // Bola lewat batas kiri (Poin untuk Pemain)
    if (ballx <= COURT_LEFT + 2) {
      score2++;
      drawScore();
      missSound();
      if (score2 >= MAX_SCORE) { triggerGameOver(); return; }
      tv.delay(500);
      resetBall(-1);
      return;
    }
    // Bola lewat batas kanan (Poin untuk AI)
    if (ballx >= COURT_RIGHT - BALL_SIZE - 2) {
      score++;
      drawScore();
      missSound();
      if (score >= MAX_SCORE) { triggerGameOver(); return; }
      tv.delay(500);
      resetBall(1);
      return;
    }

    int8_t moveX = (speedLevel >= 2) ? dx * 2 : dx;
    ballx += moveX;
    bally += dy;

    // Pantulan dinding atas & bawah bebas kedip (Tidak menyentuh garis COURT_TOP = 16)
    if (bally <= COURT_TOP + 2) {
      bally = COURT_TOP + 2;
      dy = (speedLevel >= 3) ? 2 : 1;
      playTone(294, 15);
    } else if (bally >= COURT_BOTTOM - BALL_SIZE - 2) {
      bally = COURT_BOTTOM - BALL_SIZE - 2;
      dy = (speedLevel >= 3) ? -2 : -1;
      playTone(294, 15);
    }

    // Deteksi tumbukan Paddle Kiri (AI)
    if (ballx <= paddleAx + PADDLE_WIDTH && ballx >= paddleAx - 1 &&
        bally + BALL_SIZE >= paddleAy && bally <= paddleAy + PADDLE_HEIGHT) {
      dx = 1;
      ballx = paddleAx + PADDLE_WIDTH + 1;
      dy = (bally < paddleAy + (PADDLE_HEIGHT / 2)) ? -1 : 1;
      checkRallyLevelUp();
    }

    // Deteksi tumbukan Paddle Kanan (Pemain)
    if (ballx + BALL_SIZE >= paddleBx && ballx <= paddleBx + PADDLE_WIDTH &&
        bally + BALL_SIZE >= paddleBy && bally <= paddleBy + PADDLE_HEIGHT) {
      dx = -1;
      ballx = paddleBx - BALL_SIZE - 1;
      dy = (bally < paddleBy + (PADDLE_HEIGHT / 2)) ? -1 : 1;
      checkRallyLevelUp();
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

    // Immediate Erase-Move-Draw pada Bola & Paddle
    byte oldBallX = ballx;
    setBallPixels(0);

    // Hanya pulihkan garis/lingkaran tengah jika bola baru saja melintasinya
    if (oldBallX >= 48 && oldBallX <= 78) {
      refreshCenterCourt();
    }

    updateAI();
    updatePlayer();
    drawPaddle(paddleAx, paddleAy);
    drawPaddle(paddleBx, paddleBy);

    moveBall();
    if (!isGameOver) {
      setBallPixels(1);
    }

    tv.delay(16);
  }

} // namespace PongGame

#endif