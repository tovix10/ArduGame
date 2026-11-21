#ifndef TETRIS_H
#define TETRIS_H

#include <Arduino.h>
#include <avr/pgmspace.h>

namespace TetrisGame {

  // Meminjam array worm[] milik WormGame saat Tetris aktif (0 byte RAM tambahan)
  uint16_t* board = (uint16_t*)WormGame::worm;

  int8_t curX, curY;
  byte curType, curRot;
  byte nextType = 0;
  int score = 0;
  boolean isGameOver = false;
  unsigned long dropTimer = 0;
  unsigned long inputTimer = 0;
  unsigned long rotateTimer = 0;
  unsigned long animTimer = 0;
  bool starState = false;

  const uint16_t tetrisShapes[7][4] PROGMEM = {
    {0x0F00, 0x2222, 0x00F0, 0x4444}, // I
    {0x6600, 0x6600, 0x6600, 0x6600}, // O
    {0x0E40, 0x4C40, 0x4E00, 0x4640}, // T
    {0x06C0, 0x8C40, 0x6C00, 0x4620}, // S
    {0x0C60, 0x4C80, 0xC600, 0x2640}, // Z
    {0x0E20, 0x44C0, 0x8E00, 0xC440}, // J
    {0x0E80, 0xC440, 0x2E00, 0x4460}  // L
  };

  byte getLevel() {
    byte lvl = score / 10;
    return (lvl > 9) ? 9 : lvl;
  }

  uint16_t getShape(byte type, byte rot) {
    return pgm_read_word(&(tetrisShapes[type][rot]));
  }

  void drawCell(int8_t c, int8_t r, byte color) {
    if (r < 0 || r >= 20 || c < 0 || c >= 10) return;
    byte px = 44 + (c * 4);
    byte py = 14 + (r * 4);
    tv.draw_rect(px, py, 2, 2, color, color);
  }

  void drawNextPiece() {
    tv.draw_rect(8, 46, 22, 18, 0, 0);
    uint16_t shape = getShape(nextType, 0);
    for (byte r = 0; r < 4; r++) {
      for (byte c = 0; c < 4; c++) {
        if (shape & (0x8000 >> (r * 4 + c))) {
          byte px = 12 + (c * 4);
          byte py = 49 + (r * 4);
          tv.draw_rect(px, py, 2, 2, 1, 1);
        }
      }
    }
  }

  void drawScoreAndLevel() {
    tv.draw_rect(7, 20, 24, 9, 0, 0);
    drawDigit(9, 21, (score / 100) % 10);
    drawDigit(16, 21, (score / 10) % 10);
    drawDigit(23, 21, score % 10);

    tv.draw_rect(101, 20, 12, 9, 0, 0);
    drawDigit(105, 21, getLevel());
  }

  void drawDecorations() {
    tv.draw_line(42, 13, 42, 94, 1);
    tv.draw_line(85, 13, 85, 94, 1);
    tv.draw_line(42, 94, 85, 94, 1);
    tv.draw_line(40, 13, 40, 94, 1);
    tv.draw_line(87, 13, 87, 94, 1);

    tv.draw_rect(4, 13, 31, 19, 1);
    tv.draw_line(4, 17, 35, 17, 1);

    tv.draw_rect(4, 38, 31, 28, 1);
    tv.draw_line(4, 43, 35, 43, 1);
    tv.draw_line(11, 39, 11, 41, 1); tv.set_pixel(12, 40, 1); tv.draw_line(13, 39, 13, 41, 1);
    tv.set_pixel(17, 39, 1); tv.set_pixel(19, 39, 1); tv.set_pixel(18, 40, 1); tv.set_pixel(17, 41, 1); tv.set_pixel(19, 41, 1);
    tv.draw_line(23, 39, 25, 39, 1); tv.draw_line(24, 39, 24, 41, 1);

    tv.draw_rect(4, 72, 31, 22, 1);
    tv.draw_rect(10, 78, 11, 3, 1, 1); tv.draw_rect(14, 82, 3, 3, 1, 1);
    tv.draw_rect(24, 77, 3, 11, 1, 1); tv.draw_rect(20, 85, 3, 3, 1, 1);

    tv.draw_rect(92, 13, 31, 19, 1);
    tv.draw_line(92, 17, 123, 17, 1);

    tv.draw_rect(92, 38, 31, 56, 1);
    tv.draw_rect(95, 85, 25, 7, 1, 1);
    tv.draw_rect(97, 72, 5, 13, 1);
    tv.draw_rect(105, 60, 6, 25, 1);
    tv.draw_rect(114, 74, 5, 11, 1);
    tv.set_pixel(107, 65, 1); tv.set_pixel(108, 65, 1);
    tv.set_pixel(107, 72, 1); tv.set_pixel(108, 72, 1);
    tv.draw_rect(96, 68, 7, 3, 1, 1);  tv.draw_line(99, 63, 99, 68, 1);
    tv.draw_rect(104, 54, 8, 5, 1, 1); tv.draw_line(108, 46, 108, 54, 1);
    tv.draw_line(106, 48, 110, 48, 1);
    tv.draw_rect(113, 70, 7, 3, 1, 1); tv.draw_line(116, 65, 116, 70, 1);
  }

  void updateRightPanelAnim() {
    starState = !starState;
    byte c1 = starState ? 1 : 0;
    byte c2 = starState ? 0 : 1;

    tv.set_pixel(97, 44, c1);
    tv.set_pixel(118, 48, c2);
    tv.set_pixel(99, 55, c2);
    tv.set_pixel(117, 58, c1);
    tv.set_pixel(108, 44, c1);
  }

  void drawActivePiece(byte color) {
    uint16_t shape = getShape(curType, curRot);
    for (int8_t r = 0; r < 4; r++) {
      for (int8_t c = 0; c < 4; c++) {
        if (shape & (0x8000 >> (r * 4 + c))) {
          drawCell(curX + c, curY + r, color);
        }
      }
    }
  }

  void redrawStaticBoard() {
    for (int8_t r = 0; r < 20; r++) {
      for (int8_t c = 0; c < 10; c++) {
        bool filled = (board[r] & (1 << (9 - c))) != 0;
        drawCell(c, r, filled ? 1 : 0);
      }
    }
  }

  bool checkCollision(int8_t px, int8_t py, byte pType, byte pRot) {
    uint16_t shape = getShape(pType, pRot);
    for (int8_t r = 0; r < 4; r++) {
      for (int8_t c = 0; c < 4; c++) {
        if (shape & (0x8000 >> (r * 4 + c))) {
          int8_t boardX = px + c;
          int8_t boardY = py + r;
          if (boardX < 0 || boardX >= 10 || boardY >= 20) return true;
          if (boardY >= 0 && (board[boardY] & (1 << (9 - boardX)))) return true;
        }
      }
    }
    return false;
  }

  void spawnPiece() {
    curX = 3;
    curY = 0;
    curType = nextType;
    nextType = random(0, 7);
    curRot = 0;
    drawNextPiece();

    if (checkCollision(curX, curY, curType, curRot)) {
      isGameOver = true;
    } else {
      drawActivePiece(1);
    }
  }

  void mergeAndClearLines() {
    uint16_t shape = getShape(curType, curRot);
    for (int8_t r = 0; r < 4; r++) {
      for (int8_t c = 0; c < 4; c++) {
        if (shape & (0x8000 >> (r * 4 + c))) {
          int8_t boardX = curX + c;
          int8_t boardY = curY + r;
          if (boardY >= 0 && boardY < 20 && boardX >= 0 && boardX < 10) {
            board[boardY] |= (1 << (9 - boardX));
          }
        }
      }
    }

    uint32_t fullMask = 0;
    byte linesCleared = 0;
    for (int8_t r = 0; r < 20; r++) {
      if (board[r] == 0x03FF) {
        fullMask |= (1UL << r);
        linesCleared++;
      }
    }

    if (linesCleared > 0) {
      byte prevLevel = getLevel();
      for (byte blink = 0; blink < 4; blink++) {
        byte color = (blink % 2 == 0) ? 0 : 1;
        for (int8_t r = 0; r < 20; r++) {
          if (fullMask & (1UL << r)) {
            for (int8_t c = 0; c < 10; c++) drawCell(c, r, color);
          }
        }
        playTone((blink % 2 == 0) ? 900 : 1200, 30);
        tv.delay(70);
      }

      for (int8_t r = 0; r < 20; r++) {
        if (fullMask & (1UL << r)) {
          for (int8_t k = r; k > 0; k--) {
            board[k] = board[k - 1];
          }
          board[0] = 0;
        }
      }

      score += linesCleared;
      if (score > 999) score = 999;

      if (getLevel() > prevLevel) {
        playLevelUpSound();
      }

      drawScoreAndLevel();
      redrawStaticBoard();
    }
  }

  void init() {
    tv.fill(0);
    score = 0;
    isGameOver = false;
    for (byte i = 0; i < 20; i++) board[i] = 0;

    nextType = random(0, 7);
    drawDecorations();
    drawScoreAndLevel();
    spawnPiece();

    unsigned long now = millis();
    dropTimer = now;
    inputTimer = now;
    rotateTimer = now;
    animTimer = now;
  }

  void triggerGameOver() {
    isGameOver = true;
    drawGameOverBox();
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

    bool wasPaused = isPS2Pressed(PS2_BTN_START);
    if (checkGlobalPause()) return;
    if (wasPaused) {
      // Pulihkan garis batas bawah & papan setelah keluar dari mode pause
      tv.draw_line(42, 94, 85, 94, 1);
      redrawStaticBoard();
      drawActivePiece(1);
    }

    unsigned long now = millis();

    if (now - animTimer > 500) {
      animTimer = now;
      updateRightPanelAnim();
    }

    if (now - inputTimer > 90) {
      if (isPS2Pressed(PS2_BTN_LEFT)) {
        if (!checkCollision(curX - 1, curY, curType, curRot)) {
          drawActivePiece(0);
          curX--;
          drawActivePiece(1);
          playTone(400, 8);
        }
        inputTimer = now;
      } else if (isPS2Pressed(PS2_BTN_RIGHT)) {
        if (!checkCollision(curX + 1, curY, curType, curRot)) {
          drawActivePiece(0);
          curX++;
          drawActivePiece(1);
          playTone(400, 8);
        }
        inputTimer = now;
      }

      if (isPS2Pressed(PS2_BTN_DOWN)) {
        if (!checkCollision(curX, curY + 1, curType, curRot)) {
          drawActivePiece(0);
          curY++;
          drawActivePiece(1);
          dropTimer = now;
        }
        inputTimer = now - 40;
      }
    }

    if (now - rotateTimer > 160) {
      if (isPS2Pressed(PS2_BTN_CROSS) || isPS2Pressed(PS2_BTN_UP)) {
        byte nextRot = (curRot + 1) % 4;
        if (!checkCollision(curX, curY, curType, nextRot)) {
          drawActivePiece(0);
          curRot = nextRot;
          drawActivePiece(1);
          playTone(600, 10);
        }
        rotateTimer = now;
      }
    }

    byte level = getLevel();
    unsigned int currentDropInterval = 520 - (level * 30);

    if (now - dropTimer > currentDropInterval) {
      dropTimer = now;
      if (!checkCollision(curX, curY + 1, curType, curRot)) {
        drawActivePiece(0);
        curY++;
        drawActivePiece(1);
      } else {
        playTone(250, 15);
        mergeAndClearLines();
        spawnPiece();
        if (isGameOver) {
          triggerGameOver();
          return;
        }
      }
    }

    tv.delay(16);
  }

} // namespace TetrisGame

#endif