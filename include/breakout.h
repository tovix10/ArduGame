#ifndef BREAKOUT_H
#define BREAKOUT_H

#include <Arduino.h>

namespace BreakoutGame {

  #define BRK_COLS 10
  #define BRK_ROWS 8
  #define BRK_W 8
  #define BRK_H 4
  #define MAX_BALLS 5
  #define ARENA_RIGHT 90

  struct BreakoutState {
    uint16_t bricks[BRK_ROWS];
    byte paddleX;
    int8_t ballX[MAX_BALLS], ballY[MAX_BALLS];
    int8_t dx[MAX_BALLS], dy[MAX_BALLS];
    bool ballActive[MAX_BALLS];
    int8_t itemX, itemY;
    bool itemActive;
    int score;
    byte lives;
    byte level; // 0 s/d 9 (10 Level)
    byte bricksLeft;
    bool isGameOver;
    byte animX, animY;
    int8_t animDx, animDy;
  };

  BreakoutState* st = (BreakoutState*)WormGame::worm;

  const byte paddleY = 90;
  const byte paddleW = 16;

  void drawRightPanel() {
    tv.draw_line(2, 4, ARENA_RIGHT, 4, 1);
    tv.draw_line(2, 4, 2, 95, 1);
    tv.draw_line(ARENA_RIGHT, 4, ARENA_RIGHT, 95, 1);
    tv.draw_line(ARENA_RIGHT + 2, 4, ARENA_RIGHT + 2, 95, 1);

    tv.draw_rect(95, 4, 30, 18, 1);
    tv.draw_line(95, 8, 125, 8, 1);

    tv.draw_rect(95, 26, 30, 18, 1);
    tv.draw_line(95, 30, 125, 30, 1);

    tv.draw_rect(95, 48, 30, 14, 1);

    tv.draw_rect(95, 66, 30, 29, 1);
    tv.draw_rect(99, 70, 6, 2, 1, 1);
    tv.draw_rect(107, 70, 6, 2, 1, 1);
    tv.draw_rect(115, 70, 6, 2, 1, 1);
  }

  void updateStatsPanel() {
    tv.draw_rect(97, 10, 26, 10, 0, 0);
    drawDigit(99, 12, (st->score / 100) % 10);
    drawDigit(107, 12, (st->score / 10) % 10);
    drawDigit(115, 12, st->score % 10);

    tv.draw_rect(104, 32, 12, 10, 0, 0);
    drawDigit(108, 34, st->level);

    tv.draw_rect(97, 50, 26, 10, 0, 0);
    for (byte i = 0; i < st->lives && i < 4; i++) {
      tv.draw_rect(99 + (i * 6), 53, 3, 3, 1, 1);
    }
  }

  void updateMiniAnim() {
    tv.set_pixel(st->animX, st->animY, 0);
    tv.draw_line(100, 91, 120, 91, 0);

    st->animX += st->animDx;
    st->animY += st->animDy;

    if (st->animX <= 97 || st->animX >= 122) st->animDx = -st->animDx;
    if (st->animY <= 74 || st->animY >= 89) st->animDy = -st->animDy;

    tv.set_pixel(st->animX, st->animY, 1);
    byte miniPadX = constrain(st->animX - 3, 98, 116);
    tv.draw_line(miniPadX, 91, miniPadX + 6, 91, 1);
  }

  void drawAllBricks() {
    for (byte r = 0; r < BRK_ROWS; r++) {
      for (byte c = 0; c < BRK_COLS; c++) {
        byte bx = 6 + (c * BRK_W);
        byte by = 10 + (r * BRK_H);
        if (st->bricks[r] & (1 << c)) {
          tv.draw_rect(bx, by, BRK_W - 3, BRK_H - 2, 1, 1);
        } else {
          tv.draw_rect(bx, by, BRK_W - 3, BRK_H - 2, 0, 0);
        }
      }
    }
  }

  void resetPrimaryBall() {
    st->ballX[0] = st->paddleX + (paddleW / 2);
    st->ballY[0] = paddleY - 4;
    st->dx[0] = (random(0, 2) == 0) ? 1 : -1;
    st->dy[0] = -1;
    st->ballActive[0] = true;
    for (byte b = 1; b < MAX_BALLS; b++) {
      st->ballActive[b] = false;
    }
    st->itemActive = false;
  }

  void initLevel() {
    st->bricksLeft = 0;
    // Level 0 = 4 baris (40 balok), naik perlahan tiap 2 level hingga 8 baris (80 balok)
    byte activeRows = 4 + (st->level / 2);
    if (activeRows > BRK_ROWS) activeRows = BRK_ROWS;

    for (byte r = 0; r < BRK_ROWS; r++) {
      if (r >= activeRows) {
        st->bricks[r] = 0;
      } else if (st->level % 3 == 0) {
        st->bricks[r] = 0x03FF; // Penuh
      } else if (st->level % 3 == 1) {
        st->bricks[r] = (r % 2 == 0) ? 0x03FF : 0x02AA; // Kisi selang-seling
      } else {
        st->bricks[r] = (r == 0 || r == activeRows - 1) ? 0x03FF : 0x039F; // Benteng
      }

      for (byte c = 0; c < BRK_COLS; c++) {
        if (st->bricks[r] & (1 << c)) st->bricksLeft++;
      }
    }
    drawAllBricks();
    resetPrimaryBall();
    updateStatsPanel();
  }

  void init() {
    tv.fill(0);
    st->score = 0;
    st->lives = 3;
    st->level = 0;
    st->paddleX = 38;
    st->isGameOver = false;
    st->animX = 105;
    st->animY = 80;
    st->animDx = 1;
    st->animDy = -1;

    drawRightPanel();
    initLevel();
  }

  void drawItem(int8_t x, int8_t y, byte color) {
    tv.draw_line(x - 1, y, x + 1, y, color);
    tv.draw_line(x, y - 1, x, y + 1, color);
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

    updateMiniAnim();

    tv.draw_line(st->paddleX, paddleY, st->paddleX + paddleW, paddleY, 0);
    tv.draw_line(st->paddleX, paddleY + 1, st->paddleX + paddleW, paddleY + 1, 0);

    if (isPS2Pressed(PS2_BTN_LEFT) && st->paddleX > 5) st->paddleX -= 3;
    if (isPS2Pressed(PS2_BTN_RIGHT) && st->paddleX < (ARENA_RIGHT - paddleW - 2)) st->paddleX += 3;

    tv.draw_line(st->paddleX, paddleY, st->paddleX + paddleW, paddleY, 1);
    tv.draw_line(st->paddleX, paddleY + 1, st->paddleX + paddleW, paddleY + 1, 1);

    if (st->itemActive) {
      drawItem(st->itemX, st->itemY, 0);
      st->itemY += 1;

      if (st->itemY >= paddleY - 2 && st->itemY <= paddleY + 2 &&
          st->itemX >= st->paddleX - 1 && st->itemX <= st->paddleX + paddleW + 1) {
        st->itemActive = false;
        playTone(1200, 40);

        bool spawned = false;
        for (byte b = 0; b < MAX_BALLS; b++) {
          if (!st->ballActive[b]) {
            st->ballActive[b] = true;
            st->ballX[b] = st->paddleX + (paddleW / 2);
            st->ballY[b] = paddleY - 3;
            st->dx[b] = (b & 1) ? 1 : -1;
            st->dy[b] = -1;
            spawned = true;
            break;
          }
        }
        if (!spawned) {
          if (st->lives < 4) st->lives++;
          st->score += 3;
          if (st->score > 999) st->score = 999;
          updateStatsPanel();
        }
      } else if (st->itemY > 93) {
        st->itemActive = false;
      } else {
        drawItem(st->itemX, st->itemY, 1);
      }
    }

    byte activeBallsCount = 0;
    for (byte b = 0; b < MAX_BALLS; b++) {
      if (!st->ballActive[b]) continue;
      activeBallsCount++;

      tv.set_pixel(st->ballX[b], st->ballY[b], 0);
      tv.set_pixel(st->ballX[b] + 1, st->ballY[b], 0);

      st->ballX[b] += st->dx[b];
      st->ballY[b] += st->dy[b];

      if (st->ballX[b] <= 4) { st->ballX[b] = 4; st->dx[b] = -st->dx[b]; playTone(300, 8); }
      if (st->ballX[b] >= ARENA_RIGHT - 3) { st->ballX[b] = ARENA_RIGHT - 3; st->dx[b] = -st->dx[b]; playTone(300, 8); }
      if (st->ballY[b] <= 6) { st->ballY[b] = 6; st->dy[b] = -st->dy[b]; playTone(300, 8); }

      if (st->dy[b] > 0 && st->ballY[b] >= paddleY - 2 && st->ballY[b] <= paddleY + 1 &&
          st->ballX[b] + 1 >= st->paddleX && st->ballX[b] <= st->paddleX + paddleW) {
        st->dy[b] = -1;
        if (st->ballX[b] < st->paddleX + (paddleW / 2) - 2) st->dx[b] = -1;
        else if (st->ballX[b] > st->paddleX + (paddleW / 2) + 2) st->dx[b] = 1;
        playTone(500, 12);
      }

      if (st->ballY[b] >= 10 && st->ballY[b] < 10 + (BRK_ROWS * BRK_H)) {
        byte r = (st->ballY[b] - 10) / BRK_H;
        byte c = (st->ballX[b] - 6) / BRK_W;
        if (r < BRK_ROWS && c < BRK_COLS && (st->bricks[r] & (1 << c))) {
          st->bricks[r] &= ~(1 << c);
          byte bx = 6 + (c * BRK_W);
          byte by = 10 + (r * BRK_H);
          tv.draw_rect(bx, by, BRK_W - 3, BRK_H - 2, 0, 0);
          st->dy[b] = -st->dy[b];
          if (st->score < 999) st->score += 1; // +1 Poin per balok!
          st->bricksLeft--;
          updateStatsPanel();
          playTone(850 + (r * 60), 12);

          if (!st->itemActive && activeBallsCount < MAX_BALLS && random(0, 3) == 0) {
            st->itemActive = true;
            st->itemX = bx + 2;
            st->itemY = by + 2;
          }

          if (st->bricksLeft == 0) {
            for (byte n = 0; n < MAX_BALLS; n++) {
              if (st->ballActive[n]) {
                tv.set_pixel(st->ballX[n], st->ballY[n], 0);
                tv.set_pixel(st->ballX[n] + 1, st->ballY[n], 0);
              }
            }
            if (st->itemActive) drawItem(st->itemX, st->itemY, 0);
            playLevelUpSound();
            tv.delay(400);
            st->level++;
            if (st->level > 9) st->level = 9;
            initLevel();
            return;
          }
        }
      }

      if (st->ballY[b] > 93) {
        st->ballActive[b] = false;
        activeBallsCount--;
      } else {
        tv.set_pixel(st->ballX[b], st->ballY[b], 1);
        tv.set_pixel(st->ballX[b] + 1, st->ballY[b], 1);
      }
    }

    if (activeBallsCount == 0) {
      if (st->itemActive) { drawItem(st->itemX, st->itemY, 0); st->itemActive = false; }
      st->lives--;
      updateStatsPanel();
      playTone(150, 200);
      if (st->lives == 0) {
        st->isGameOver = true;
        drawGameOverBox();
        return;
      }
      tv.delay(450);
      resetPrimaryBall();
    }

    tv.delay(17);
  }

} // namespace BreakoutGame

#endif