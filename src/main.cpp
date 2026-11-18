#include <Arduino.h>
#include <TVout.h>
#include <avr/pgmspace.h>

#undef LEFT
#undef RIGHT
#undef UP
#undef DOWN

#define LEFT 3
#define RIGHT 2
#define UP 4
#define DOWN 5

#define W 128
#define H 96

#define PS2_ATT 4  // CS
#define PS2_CMD 3  // DO
#define PS2_CLK 5  // CLK
#define PS2_DAT 2  // DI

#define PS2_BTN_SELECT (1 << 0)
#define PS2_BTN_START  (1 << 3)
#define PS2_BTN_UP     (1 << 4)
#define PS2_BTN_RIGHT  (1 << 5)
#define PS2_BTN_DOWN   (1 << 6)
#define PS2_BTN_LEFT   (1 << 7)
#define PS2_BTN_CROSS  (1 << 14)

TVout tv;
uint16_t ps2Buttons = 0xFFFF;
byte activeGame = 0; // 0=Menu, 1=Worm, 2=Pong, 3=Breakout, 4=Shooter, 5=Dodge, 6=Run, 7=Tetris
bool isPaused = false;

// --- DRIVER STIK PS2 ---
void setupPS2() {
  pinMode(PS2_ATT, OUTPUT);
  pinMode(PS2_CMD, OUTPUT);
  pinMode(PS2_CLK, OUTPUT);
  pinMode(PS2_DAT, INPUT_PULLUP);
  digitalWrite(PS2_ATT, HIGH);
  digitalWrite(PS2_CMD, HIGH);
  digitalWrite(PS2_CLK, HIGH);
}

byte ps2Transfer(byte cmd) {
  byte data = 0;
  for (byte i = 0; i < 8; i++) {
    if (cmd & (1 << i)) digitalWrite(PS2_CMD, HIGH);
    else digitalWrite(PS2_CMD, LOW);
    digitalWrite(PS2_CLK, LOW);
    __asm__ __volatile__ ("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
    if (digitalRead(PS2_DAT)) data |= (1 << i);
    digitalWrite(PS2_CLK, HIGH);
    __asm__ __volatile__ ("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
  }
  return data;
}

void readPS2() {
  digitalWrite(PS2_ATT, LOW);
  __asm__ __volatile__ ("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
  ps2Transfer(0x01);
  byte id = ps2Transfer(0x42);
  byte z  = ps2Transfer(0x00);
  byte b1 = ps2Transfer(0x00);
  byte b2 = ps2Transfer(0x00);
  digitalWrite(PS2_ATT, HIGH);
  if (z == 0x5A || id == 0x41 || id == 0x73) ps2Buttons = (b2 << 8) | b1;
  else ps2Buttons = 0xFFFF;
}

bool isPS2Pressed(uint16_t btnMask) {
  return !(ps2Buttons & btnMask);
}

// Output suara ke Pin D11 Arduino Nano
void playTone(unsigned int frequency, unsigned long duration_ms) {
  tv.tone(frequency, duration_ms);
}

// --- SISTEM MUSIK JINGLE TEMA 7 GAME (PROGMEM - 0 BYTE RAM!) ---
const uint16_t gameJingles[7][4] PROGMEM = {
  {523, 659, 784, 1046}, // 1. Worm: Arpeggio Ceria
  {440, 440, 659, 880},  // 2. Pong: Peluit Stadion Arcade
  {587, 740, 880, 1175}, // 3. Breakout: Pantulan Enerjik
  {330, 392, 494, 659},  // 4. Shooter: Sci-Fi Galaksi
  {392, 330, 523, 784},  // 5. Dodge: Alarm Meteor Cepat
  {523, 784, 659, 1046}, // 6. Run: Ritme Petualangan Gurun
  {659, 494, 523, 587}   // 7. Tetris: Motif Korobeiniki Klasik
};

void playGameJingle(byte gameIdx) {
  for (byte i = 0; i < 4; i++) {
    uint16_t f = pgm_read_word(&gameJingles[gameIdx][i]);
    byte d = (i == 3) ? 110 : 55;
    playTone(f, d);
    tv.delay(d + 15);
  }
}

// Melodi Kenaikan Level Global untuk Semua Game
void playLevelUpSound() {
  playTone(784, 35); tv.delay(45);
  playTone(988, 35); tv.delay(45);
  playTone(1319, 70);
}

void drawDigit(byte x, byte y, byte d) {
  switch (d) {
    case 0: tv.draw_rect(x, y, 4, 6, 1); break;
    case 1: tv.draw_line(x + 2, y, x + 2, y + 6, 1); break;
    case 2:
      tv.draw_line(x, y, x + 4, y, 1); tv.draw_line(x + 4, y, x + 4, y + 3, 1);
      tv.draw_line(x, y + 3, x + 4, y + 3, 1); tv.draw_line(x, y + 3, x, y + 6, 1);
      tv.draw_line(x, y + 6, x + 4, y + 6, 1); break;
    case 3:
      tv.draw_line(x, y, x + 4, y, 1); tv.draw_line(x + 4, y, x + 4, y + 6, 1);
      tv.draw_line(x + 1, y + 3, x + 4, y + 3, 1); tv.draw_line(x, y + 6, x + 4, y + 6, 1); break;
    case 4:
      tv.draw_line(x, y, x, y + 3, 1); tv.draw_line(x, y + 3, x + 4, y + 3, 1);
      tv.draw_line(x + 4, y, x + 4, y + 6, 1); break;
    case 5:
      tv.draw_line(x, y, x + 4, y, 1); tv.draw_line(x, y, x, y + 3, 1);
      tv.draw_line(x, y + 3, x + 4, y + 3, 1); tv.draw_line(x + 4, y + 3, x + 4, y + 6, 1);
      tv.draw_line(x, y + 6, x + 4, y + 6, 1); break;
    case 6:
      tv.draw_line(x, y, x + 4, y, 1); tv.draw_line(x, y, x, y + 6, 1);
      tv.draw_line(x, y + 3, x + 4, y + 3, 1); tv.draw_line(x + 4, y + 3, x + 4, y + 6, 1);
      tv.draw_line(x, y + 6, x + 4, y + 6, 1); break;
    case 7:
      tv.draw_line(x, y, x + 4, y, 1); tv.draw_line(x + 4, y, x + 4, y + 6, 1); break;
    case 8:
      tv.draw_rect(x, y, 4, 6, 1); tv.draw_line(x, y + 3, x + 4, y + 3, 1); break;
    case 9:
      tv.draw_rect(x, y, 4, 3, 1); tv.draw_line(x + 4, y, x + 4, y + 6, 1);
      tv.draw_line(x, y + 6, x + 4, y + 6, 1); break;
  }
}

// --- INDIKATOR SENSOR BATERAI 1S LI-ION (PIN A1) ---
void drawBatteryIcon(bool blinkState) {
  int adc = analogRead(A1);
  byte bars = 0;
  // Kalibrasi 1S Li-ion (Referensi Step-Up 5.0V):
  // > 4.0V (~818) = 3 Bar, > 3.6V (~735) = 2 Bar, > 3.25V (~665) = 1 Bar
  if (adc > 815) bars = 3;
  else if (adc > 735) bars = 2;
  else if (adc > 665) bars = 1;

  // Bersihkan area baterai di kanan atas header menu
  tv.draw_rect(100, 8, 15, 7, 0, 0);

  // Jika baterai kritis (0 bar), kedipkan kerangka baterai sebagai peringatan!
  if (bars == 0 && !blinkState) return;

  tv.draw_rect(101, 8, 11, 6, 1, 0);
  tv.draw_line(113, 10, 113, 12, 1); // Kutub positif baterai
  for (byte i = 0; i < bars; i++) {
    tv.draw_rect(103 + (i * 3), 10, 1, 2, 1, 1);
  }
}

// --- FUNGSI GLOBAL KOTAK & NADA "GAME OVER" ---
void drawGameOverBox() {
  playTone(440, 110); tv.delay(120);
  playTone(349, 110); tv.delay(120);
  playTone(294, 110); tv.delay(120);
  playTone(220, 280);

  tv.draw_rect(26, 32, 76, 32, 1, 0);
  tv.draw_rect(28, 34, 72, 28, 1);

  byte x = 47, y = 38;
  // G A M E
  tv.draw_line(x, y, x + 5, y, 1); tv.draw_line(x, y, x, y + 8, 1);
  tv.draw_line(x, y + 8, x + 5, y + 8, 1); tv.draw_line(x + 5, y + 4, x + 5, y + 8, 1);
  tv.draw_line(x + 3, y + 4, x + 5, y + 4, 1);
  tv.draw_line(x + 9, y + 8, x + 9, y, 1); tv.draw_line(x + 9, y, x + 14, y, 1);
  tv.draw_line(x + 14, y, x + 14, y + 8, 1); tv.draw_line(x + 9, y + 4, x + 14, y + 4, 1);
  tv.draw_line(x + 18, y + 8, x + 18, y, 1); tv.draw_line(x + 18, y, x + 21, y + 4, 1);
  tv.draw_line(x + 21, y + 4, x + 24, y, 1); tv.draw_line(x + 24, y, x + 24, y + 8, 1);
  tv.draw_line(x + 28, y, x + 28, y + 8, 1); tv.draw_line(x + 28, y, x + 33, y, 1);
  tv.draw_line(x + 28, y + 4, x + 31, y + 4, 1); tv.draw_line(x + 28, y + 8, x + 33, y + 8, 1);

  y = 50;
  // O V E R
  tv.draw_rect(x, y, 5, 8, 1);
  tv.draw_line(x + 9, y, x + 9, y + 5, 1); tv.draw_line(x + 9, y + 5, x + 11, y + 8, 1);
  tv.draw_line(x + 11, y + 8, x + 14, y + 5, 1); tv.draw_line(x + 14, y + 5, x + 14, y, 1);
  tv.draw_line(x + 18, y, x + 18, y + 8, 1); tv.draw_line(x + 18, y, x + 23, y, 1);
  tv.draw_line(x + 18, y + 4, x + 21, y + 4, 1); tv.draw_line(x + 18, y + 8, x + 23, y + 8, 1);
  tv.draw_line(x + 27, y, x + 27, y + 8, 1); tv.draw_rect(x + 27, y, 5, 4, 1);
  tv.draw_line(x + 30, y + 4, x + 33, y + 8, 1);
}

// --- FUNGSI GLOBAL PAUSE (SIMPAN & PULIHKAN LAYAR OTOMATIS) ---
bool checkGlobalPause() {
  if (isPS2Pressed(PS2_BTN_START)) {
    isPaused = true;
    playTone(900, 30); tv.delay(40); playTone(1200, 40);

    byte backup[96];
    for (byte r = 0; r < 16; r++) {
      for (byte c = 0; c < 6; c++) {
        backup[r * 6 + c] = tv.screen[(40 + r) * 16 + (5 + c)];
      }
    }

    tv.draw_rect(40, 40, 47, 15, 1, 0);
    byte x = 46, y = 44;
    tv.draw_line(x, y, x, y + 7, 1); tv.draw_rect(x, y, 4, 3, 1);
    tv.draw_line(x + 7, y + 7, x + 7, y, 1); tv.draw_line(x + 7, y, x + 11, y, 1);
    tv.draw_line(x + 11, y, x + 11, y + 7, 1); tv.draw_line(x + 7, y + 3, x + 11, y + 3, 1);
    tv.draw_line(x + 14, y, x + 14, y + 7, 1); tv.draw_line(x + 18, y, x + 18, y + 7, 1);
    tv.draw_line(x + 14, y + 7, x + 18, y + 7, 1);
    tv.draw_line(x + 21, y, x + 25, y, 1); tv.draw_line(x + 21, y, x + 21, y + 3, 1);
    tv.draw_line(x + 21, y + 3, x + 25, y + 3, 1); tv.draw_line(x + 25, y + 3, x + 25, y + 7, 1);
    tv.draw_line(x + 21, y + 7, x + 25, y + 7, 1);
    tv.draw_line(x + 28, y, x + 28, y + 7, 1); tv.draw_line(x + 28, y, x + 32, y, 1);
    tv.draw_line(x + 28, y + 3, x + 31, y + 3, 1); tv.draw_line(x + 28, y + 7, x + 32, y + 7, 1);

    while (isPS2Pressed(PS2_BTN_START)) { tv.delay(16); readPS2(); }

    while (isPaused) {
      tv.delay(20);
      readPS2();
      if (isPS2Pressed(PS2_BTN_SELECT)) {
        isPaused = false;
        activeGame = 0;
        return true;
      }
      if (isPS2Pressed(PS2_BTN_START)) {
        playTone(1200, 30); tv.delay(40); playTone(900, 40);
        while (isPS2Pressed(PS2_BTN_START)) { tv.delay(16); readPS2(); }
        isPaused = false;
      }
    }

    for (byte r = 0; r < 16; r++) {
      for (byte c = 0; c < 6; c++) {
        tv.screen[(40 + r) * 16 + (5 + c)] = backup[r * 6 + c];
      }
    }
  }
  return false;
}

// --- GENERATOR HURUF "HAFIZ" & TEKS MENU ---
void drawWordHAFIZ(byte x, byte y, bool large) {
  if (!large) {
    tv.draw_line(x, y, x, y + 6, 1); tv.draw_line(x + 4, y, x + 4, y + 6, 1);
    tv.draw_line(x, y + 3, x + 4, y + 3, 1);
    tv.draw_line(x + 7, y + 6, x + 7, y, 1); tv.draw_line(x + 7, y, x + 11, y, 1);
    tv.draw_line(x + 11, y, x + 11, y + 6, 1); tv.draw_line(x + 7, y + 3, x + 11, y + 3, 1);
    tv.draw_line(x + 14, y, x + 14, y + 6, 1); tv.draw_line(x + 14, y, x + 18, y, 1);
    tv.draw_line(x + 14, y + 3, x + 17, y + 3, 1);
    tv.draw_line(x + 20, y, x + 22, y, 1); tv.draw_line(x + 21, y, x + 21, y + 6, 1);
    tv.draw_line(x + 20, y + 6, x + 22, y + 6, 1);
    tv.draw_line(x + 25, y, x + 29, y, 1); tv.draw_line(x + 29, y, x + 25, y + 6, 1);
    tv.draw_line(x + 25, y + 6, x + 29, y + 6, 1);
  } else {
    tv.draw_rect(x, y, 2, 16, 1, 1); tv.draw_rect(x + 8, y, 2, 16, 1, 1);
    tv.draw_rect(x + 2, y + 7, 6, 2, 1, 1);
    tv.draw_rect(x + 14, y + 2, 2, 14, 1, 1); tv.draw_rect(x + 22, y + 2, 2, 14, 1, 1);
    tv.draw_rect(x + 16, y, 6, 2, 1, 1); tv.draw_rect(x + 16, y + 8, 6, 2, 1, 1);
    tv.draw_rect(x + 28, y, 2, 16, 1, 1); tv.draw_rect(x + 30, y, 8, 2, 1, 1);
    tv.draw_rect(x + 30, y + 7, 6, 2, 1, 1);
    tv.draw_rect(x + 41, y, 6, 2, 1, 1); tv.draw_rect(x + 43, y + 2, 2, 12, 1, 1);
    tv.draw_rect(x + 41, y + 14, 6, 2, 1, 1);
    tv.draw_rect(x + 51, y, 10, 2, 1, 1);
    tv.draw_line(x + 60, y + 2, x + 51, y + 14, 1);
    tv.draw_line(x + 61, y + 2, x + 52, y + 14, 1);
    tv.draw_rect(x + 51, y + 14, 10, 2, 1, 1);
  }
}

// --- ANIMASI SPLASH SCREEN & MELODI CHIPTUNE PEMBUKA ---
void showSplashScreen() {
  tv.fill(0);

  for (byte i = 0; i <= 36; i += 3) {
    tv.draw_line(14, 48 - i, 114, 48 - i, 1);
    tv.draw_line(14, 48 + i, 114, 48 + i, 1);
    playTone(300 + (i * 20), 12);
    tv.delay(18);
    if (i < 36) {
      tv.draw_line(14, 48 - i, 114, 48 - i, 0);
      tv.draw_line(14, 48 + i, 114, 48 + i, 0);
    }
  }

  tv.draw_rect(10, 10, 108, 76, 1);
  tv.draw_rect(13, 13, 102, 70, 1);

  drawWordHAFIZ(33, 30, true);

  tv.draw_line(26, 54, 102, 54, 1);
  drawDigit(52, 62, 7);
  tv.draw_rect(61, 64, 4, 2, 1, 1);
  drawDigit(70, 62, 1);

  const unsigned int notes[6] = {523, 659, 784, 1046, 784, 1046};
  const byte durs[6]          = {70,  70,  70,  110,  70,  220};
  for (byte i = 0; i < 6; i++) {
    byte c = (i % 2 == 0) ? 1 : 0;
    tv.draw_circle(22, 22, 2, c);
    tv.draw_circle(106, 22, 2, !c);
    tv.draw_circle(22, 74, 2, !c);
    tv.draw_circle(106, 74, 2, c);
    playTone(notes[i], durs[i]);
    tv.delay(durs[i] + 30);
  }

  for (byte w = 0; w < 35; w++) {
    readPS2();
    if (isPS2Pressed(PS2_BTN_START) || isPS2Pressed(PS2_BTN_CROSS)) break;
    tv.delay(20);
  }
}

void drawWordWORM(byte x, byte y) {
  tv.draw_line(x, y, x, y + 10, 1); tv.draw_line(x, y + 10, x + 4, y + 6, 1);
  tv.draw_line(x + 4, y + 6, x + 8, y + 10, 1); tv.draw_line(x + 8, y + 10, x + 8, y, 1);
  tv.draw_rect(x + 12, y, 7, 10, 1);
  tv.draw_line(x + 23, y, x + 23, y + 10, 1); tv.draw_rect(x + 23, y, 6, 5, 1);
  tv.draw_line(x + 26, y + 5, x + 29, y + 10, 1);
  tv.draw_line(x + 33, y + 10, x + 33, y, 1); tv.draw_line(x + 33, y, x + 37, y + 4, 1);
  tv.draw_line(x + 37, y + 4, x + 41, y, 1); tv.draw_line(x + 41, y, x + 41, y + 10, 1);
}

void drawWordPONG(byte x, byte y) {
  tv.draw_line(x, y, x, y + 10, 1); tv.draw_rect(x, y, 6, 5, 1);
  tv.draw_rect(x + 10, y, 7, 10, 1);
  tv.draw_line(x + 21, y + 10, x + 21, y, 1); tv.draw_line(x + 21, y, x + 27, y + 10, 1);
  tv.draw_line(x + 27, y + 10, x + 27, y, 1);
  tv.draw_line(x + 31, y, x + 38, y, 1); tv.draw_line(x + 31, y, x + 31, y + 10, 1);
  tv.draw_line(x + 31, y + 10, x + 38, y + 10, 1); tv.draw_line(x + 38, y + 5, x + 38, y + 10, 1);
  tv.draw_line(x + 35, y + 5, x + 38, y + 5, 1);
}

void drawWordBRKOUT(byte x, byte y) {
  tv.draw_line(x, y, x, y + 10, 1); tv.draw_rect(x, y, 5, 5, 1); tv.draw_rect(x, y + 5, 6, 5, 1);
  tv.draw_line(x + 9, y, x + 9, y + 10, 1); tv.draw_rect(x + 9, y, 5, 5, 1); tv.draw_line(x + 12, y + 5, x + 14, y + 10, 1);
  tv.draw_line(x + 18, y, x + 18, y + 10, 1); tv.draw_line(x + 18, y + 5, x + 23, y, 1); tv.draw_line(x + 18, y + 5, x + 23, y + 10, 1);
  tv.draw_rect(x + 27, y, 6, 10, 1);
  tv.draw_line(x + 36, y, x + 36, y + 10, 1); tv.draw_line(x + 42, y, x + 42, y + 10, 1); tv.draw_line(x + 36, y + 10, x + 42, y + 10, 1);
  tv.draw_line(x + 45, y, x + 51, y, 1); tv.draw_line(x + 48, y, x + 48, y + 10, 1);
}

void drawWordSHOOTER(byte x, byte y) {
  tv.draw_line(x, y, x + 6, y, 1); tv.draw_line(x, y, x, y + 5, 1);
  tv.draw_line(x, y + 5, x + 6, y + 5, 1); tv.draw_line(x + 6, y + 5, x + 6, y + 10, 1);
  tv.draw_line(x, y + 10, x + 6, y + 10, 1);
  tv.draw_line(x + 10, y, x + 10, y + 10, 1); tv.draw_line(x + 16, y, x + 16, y + 10, 1);
  tv.draw_line(x + 10, y + 5, x + 16, y + 5, 1);
  tv.draw_rect(x + 20, y, 6, 10, 1);
  tv.draw_rect(x + 28, y, 6, 10, 1);
  tv.draw_line(x + 36, y, x + 42, y, 1); tv.draw_line(x + 39, y, x + 39, y + 10, 1);
  tv.draw_line(x + 46, y, x + 46, y + 10, 1); tv.draw_line(x + 46, y, x + 52, y, 1);
  tv.draw_line(x + 46, y + 5, x + 50, y + 5, 1); tv.draw_line(x + 46, y + 10, x + 52, y + 10, 1);
  tv.draw_line(x + 56, y, x + 56, y + 10, 1); tv.draw_rect(x + 56, y, 5, 5, 1);
  tv.draw_line(x + 59, y + 5, x + 62, y + 10, 1);
}

void drawWordDODGE(byte x, byte y) {
  tv.draw_line(x, y, x, y + 10, 1); tv.draw_line(x, y, x + 4, y, 1);
  tv.draw_line(x + 4, y, x + 6, y + 2, 1); tv.draw_line(x + 6, y + 2, x + 6, y + 8, 1);
  tv.draw_line(x + 6, y + 8, x + 4, y + 10, 1); tv.draw_line(x + 4, y + 10, x, y + 10, 1);
  tv.draw_rect(x + 10, y, 6, 10, 1);
  tv.draw_line(x + 18, y, x + 18, y + 10, 1); tv.draw_line(x + 18, y, x + 22, y, 1);
  tv.draw_line(x + 22, y, x + 24, y + 2, 1); tv.draw_line(x + 24, y + 2, x + 24, y + 8, 1);
  tv.draw_line(x + 24, y + 8, x + 22, y + 10, 1); tv.draw_line(x + 22, y + 10, x + 18, y + 10, 1);
  tv.draw_line(x + 28, y, x + 34, y, 1); tv.draw_line(x + 28, y, x + 28, y + 10, 1);
  tv.draw_line(x + 28, y + 10, x + 34, y + 10, 1); tv.draw_line(x + 34, y + 5, x + 34, y + 10, 1);
  tv.draw_line(x + 31, y + 5, x + 34, y + 5, 1);
  tv.draw_line(x + 38, y, x + 38, y + 10, 1); tv.draw_line(x + 38, y, x + 44, y, 1);
  tv.draw_line(x + 38, y + 5, x + 42, y + 5, 1); tv.draw_line(x + 38, y + 10, x + 44, y + 10, 1);
}

void drawWordRUN(byte x, byte y) {
  tv.draw_line(x, y, x, y + 10, 1); tv.draw_rect(x, y, 5, 5, 1);
  tv.draw_line(x + 3, y + 5, x + 6, y + 10, 1);
  tv.draw_line(x + 10, y, x + 10, y + 10, 1); tv.draw_line(x + 16, y, x + 16, y + 10, 1);
  tv.draw_line(x + 10, y + 10, x + 16, y + 10, 1);
  tv.draw_line(x + 20, y + 10, x + 20, y, 1); tv.draw_line(x + 20, y, x + 26, y + 10, 1);
  tv.draw_line(x + 26, y + 10, x + 26, y, 1);
}

void drawWordTETRIS(byte x, byte y) {
  tv.draw_line(x, y, x + 6, y, 1); tv.draw_line(x + 3, y, x + 3, y + 10, 1);
  tv.draw_line(x + 10, y, x + 10, y + 10, 1); tv.draw_line(x + 10, y, x + 15, y, 1);
  tv.draw_line(x + 10, y + 5, x + 14, y + 5, 1); tv.draw_line(x + 10, y + 10, x + 15, y + 10, 1);
  tv.draw_line(x + 19, y, x + 25, y, 1); tv.draw_line(x + 22, y, x + 22, y + 10, 1);
  tv.draw_line(x + 29, y, x + 29, y + 10, 1); tv.draw_rect(x + 29, y, 5, 5, 1);
  tv.draw_line(x + 32, y + 5, x + 35, y + 10, 1);
  tv.draw_line(x + 39, y, x + 39, y + 10, 1);
  tv.draw_line(x + 43, y, x + 49, y, 1); tv.draw_line(x + 43, y, x + 43, y + 5, 1);
  tv.draw_line(x + 43, y + 5, x + 49, y + 5, 1); tv.draw_line(x + 49, y + 5, x + 49, y + 10, 1);
  tv.draw_line(x + 43, y + 10, x + 49, y + 10, 1);
}

void drawGameTitle(byte idx, byte x, byte y) {
  if (idx == 0) drawWordWORM(x, y);
  else if (idx == 1) drawWordPONG(x, y);
  else if (idx == 2) drawWordBRKOUT(x, y);
  else if (idx == 3) drawWordSHOOTER(x, y);
  else if (idx == 4) drawWordDODGE(x, y);
  else if (idx == 5) drawWordRUN(x, y);
  else if (idx == 6) drawWordTETRIS(x, y);
}

// Menggambar Ikon Mini (Preview Game) di Pojok Kanan Bawah Menu
void drawGamePreviewIcon(byte idx) {
  tv.draw_rect(97, 67, 16, 16, 0, 0);
  tv.draw_rect(96, 66, 18, 18, 1);
  byte cx = 100, cy = 70;
  if (idx == 0) {
    tv.draw_rect(cx, cy + 6, 6, 1, 1, 1);
    tv.draw_rect(cx + 5, cy + 2, 1, 5, 1, 1);
    tv.draw_rect(cx + 5, cy + 2, 3, 2, 1, 1);
    tv.draw_rect(cx + 9, cy + 2, 1, 1, 1, 1);
  } else if (idx == 1) {
    tv.draw_line(cx, cy + 1, cx, cy + 8, 1);
    tv.draw_line(cx + 10, cy + 3, cx + 10, cy + 10, 1);
    tv.draw_rect(cx + 5, cy + 5, 1, 1, 1, 1);
  } else if (idx == 2) {
    tv.draw_rect(cx, cy + 1, 4, 2, 1, 1);
    tv.draw_rect(cx + 6, cy + 1, 4, 2, 1, 1);
    tv.draw_line(cx + 2, cy + 10, cx + 8, cy + 10, 1);
    tv.set_pixel(cx + 5, cy + 6, 1);
  } else if (idx == 3) {
    tv.set_pixel(cx + 5, cy + 2, 1);
    tv.draw_line(cx + 4, cy + 4, cx + 6, cy + 4, 1);
    tv.draw_line(cx + 1, cy + 6, cx + 9, cy + 6, 1);
    tv.set_pixel(cx + 1, cy + 4, 1); tv.set_pixel(cx + 9, cy + 4, 1);
  } else if (idx == 4) {
    tv.draw_circle(cx + 5, cy + 7, 3, 1);
    tv.draw_line(cx + 3, cy + 1, cx + 3, cy + 3, 1);
    tv.draw_line(cx + 7, cy + 1, cx + 7, cy + 3, 1);
  } else if (idx == 5) {
    tv.draw_rect(cx + 4, cy + 1, 4, 3, 1, 1);
    tv.draw_rect(cx + 2, cy + 4, 4, 3, 1, 1);
    tv.draw_line(cx + 2, cy + 7, cx + 2, cy + 9, 1);
    tv.draw_line(cx + 5, cy + 7, cx + 5, cy + 9, 1);
  } else if (idx == 6) {
    tv.draw_rect(cx + 1, cy + 2, 8, 2, 1, 1);
    tv.draw_rect(cx + 4, cy + 5, 2, 3, 1, 1);
  }
}

// Sertakan seluruh 7 modul game
#include "worm.h"
#include "pong.h"
#include "breakout.h"
#include "invaders.h"
#include "dodge.h"
#include "run.h"
#include "tetris.h"

void showMainMenu() {
  byte selected = 0;
  byte scrollOffset = 0;
  const byte totalGames = 7;
  const byte visibleRows = 3;
  byte animTick = 0;

  tv.fill(0);
  // Bingkai Ganda Konsol Arcade
  tv.draw_rect(4, 3, 120, 90, 1);
  tv.draw_rect(6, 5, 116, 86, 1);
  tv.draw_line(6, 17, 122, 17, 1);

  // Header "HAFIZ 7" di Atas Menu
  drawWordHAFIZ(44, 8, false);
  drawDigit(80, 8, 7);

  // Tampilkan Ikon Baterai Pertama Kali
  drawBatteryIcon(true);

  // Jalur Scrollbar di Sisi Kanan
  tv.draw_line(116, 20, 116, 62, 1);

  bool redrawMenu = true;

  while (true) {
    if (redrawMenu) {
      tv.draw_rect(10, 20, 84, 66, 0, 0);

      for (byte i = 0; i < visibleRows; i++) {
        byte gameIdx = scrollOffset + i;
        if (gameIdx >= totalGames) break;

        byte rowY = 26 + (i * 20);
        drawDigit(24, rowY, gameIdx + 1);
        drawGameTitle(gameIdx, 34, rowY - 2);
      }
      redrawMenu = false;
    }

    // Update Scrollbar & Ikon Preview sesuai pilihan kursor
    tv.draw_rect(115, 20, 3, 43, 0, 0);
    tv.draw_line(116, 20, 116, 62, 1);
    byte barY = 20 + (selected * 6);
    tv.draw_rect(115, barY, 3, 6, 1, 1);
    drawGamePreviewIcon(selected);

    // Animasi Kursor Panah Segitiga
    animTick++;
    byte arrowShift = (animTick / 6) % 2;

    tv.draw_rect(12, 22, 9, 60, 0, 0);
    byte cursorRow = selected - scrollOffset;
    byte cy = 26 + (cursorRow * 20);
    byte cx = 14 + arrowShift;
    tv.draw_line(cx, cy, cx, cy + 6, 1);
    tv.draw_line(cx + 1, cy + 1, cx + 1, cy + 5, 1);
    tv.draw_line(cx + 2, cy + 2, cx + 2, cy + 4, 1);
    tv.set_pixel(cx + 3, cy + 3, 1);

    // Ornamen lampu berkedip di kiri nama HAFIZ & Sensor Baterai di kanan
    byte starColor = (animTick / 8) % 2;
    tv.draw_rect(22, 10, 3, 3, starColor, starColor);
    if (animTick % 24 == 0) {
      drawBatteryIcon(starColor);
    }

    tv.delay(20);
    readPS2();

    if (isPS2Pressed(PS2_BTN_UP)) {
      if (selected > 0) {
        selected--;
        if (selected < scrollOffset) {
          scrollOffset = selected;
          redrawMenu = true;
        }
        playTone(800, 15);
      }
      while (isPS2Pressed(PS2_BTN_UP)) { tv.delay(16); readPS2(); }
    } else if (isPS2Pressed(PS2_BTN_DOWN)) {
      if (selected < totalGames - 1) {
        selected++;
        if (selected >= scrollOffset + visibleRows) {
          scrollOffset = selected - visibleRows + 1;
          redrawMenu = true;
        }
        playTone(800, 15);
      }
      while (isPS2Pressed(PS2_BTN_DOWN)) { tv.delay(16); readPS2(); }
    } else if (isPS2Pressed(PS2_BTN_START) || isPS2Pressed(PS2_BTN_CROSS)) {
      // Putar Musik Jingle Spesial sesuai Tema Game yang dipilih!
      playGameJingle(selected);
      while (isPS2Pressed(PS2_BTN_START) || isPS2Pressed(PS2_BTN_CROSS)) {
        tv.delay(16);
        readPS2();
      }
      isPaused = false;
      activeGame = selected + 1;
      if (activeGame == 1) WormGame::init();
      else if (activeGame == 2) PongGame::init();
      else if (activeGame == 3) BreakoutGame::init();
      else if (activeGame == 4) InvadersGame::init();
      else if (activeGame == 5) DodgeGame::init();
      else if (activeGame == 6) RunGame::init();
      else if (activeGame == 7) TetrisGame::init();
      return;
    }
  }
}

void setup() {
  setupPS2();
  tv.begin(NTSC, W, H);
  randomSeed(analogRead(0));
  showSplashScreen();
}

void loop() {
  if (activeGame == 0) showMainMenu();
  else if (activeGame == 1) WormGame::update();
  else if (activeGame == 2) PongGame::update();
  else if (activeGame == 3) BreakoutGame::update();
  else if (activeGame == 4) InvadersGame::update();
  else if (activeGame == 5) DodgeGame::update();
  else if (activeGame == 6) RunGame::update();
  else if (activeGame == 7) TetrisGame::update();
}