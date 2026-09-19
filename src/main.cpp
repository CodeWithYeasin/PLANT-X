#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <Keypad.h>
#include <math.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define DHTPIN   PB9
#define DHTTYPE  DHT11
DHT dht(DHTPIN, DHTTYPE);

#define BUZZER  PB12
#define LED     PC13
#define SOIL    PB0
#define LDR     PB1

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'D','#','0','*'},
  {'C','9','8','7'},
  {'B','6','5','4'},
  {'A','3','2','1'}
};
byte rowPins[ROWS] = {PA3, PA2, PA1, PA0};
byte colPins[COLS] = {PA7, PA6, PA5, PA4};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

bool buzzerEnabled = true;

float temp     = 0;
float hum      = 0;
int   moisture = 0;
int   light    = 0;

enum Screen {
  MAIN_MENU, MONITOR_MENU, GAME_MENU, SETTINGS_MENU, ABOUT_SCREEN,
  TEMP_SCREEN, HUMIDITY_SCREEN, SOIL_SCREEN, ALL_SENSOR_SCREEN, LIGHT_SCREEN,
  SNAKE_GAME, TETRIS_GAME, DINO_GAME, SPACE_INVADERS, BREAKOUT_GAME,
  HOME_SCREEN, SYSTEM_SCREEN
};

Screen screen = HOME_SCREEN;
int mainSelection = 0, monitorSelection = 0, gameSelection = 0, settingsSelection = 0;

unsigned long lastActivity = 0;
#define INACTIVITY_MS 60000  // 1 minute

void initSnake(); void initTetris(); void initDino();
void initSpaceInvaders(); void initBreakout();

void clearScreen() { display.clearDisplay(); display.setTextColor(WHITE); }

void printCentered(const char* text, int y, int textSize = 1) {
  display.setTextSize(textSize);
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, y);
  display.print(text);
}

void alert() {
  if (!buzzerEnabled) return;
  static unsigned long lastAlert = 0;
  if (millis() - lastAlert < 3000) return;
  lastAlert = millis();
  for (int i = 0; i < 5; i++) {
    digitalWrite(BUZZER, HIGH); digitalWrite(LED, LOW);  delay(60);
    digitalWrite(BUZZER, LOW);  digitalWrite(LED, HIGH); delay(60);
  }
}

void eatSound() {
  if (!buzzerEnabled) return;
  digitalWrite(BUZZER, HIGH); digitalWrite(LED, LOW);  delay(30);
  digitalWrite(BUZZER, LOW);  digitalWrite(LED, HIGH);
}

void gameOverSound() {
  if (!buzzerEnabled) return;
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER, HIGH); digitalWrite(LED, LOW);  delay(400);
    digitalWrite(BUZZER, LOW);  digitalWrite(LED, HIGH); delay(150);
  }
}

// ============================================================
// BOOT SCREEN — matrix rain
// ============================================================

void bootScreen() {
  // matrix rain
  int cols = 16; // 128/8 chars wide
  int drops[16];
  for (int i = 0; i < cols; i++) drops[i] = random(-20, 0);

  unsigned long start = millis();
  while (millis() - start < 2500) {
    clearScreen();

    for (int c = 0; c < cols; c++) {
      int x = c * 8;
      int y = drops[c] * 8;

      // draw trail
      for (int t = 3; t >= 1; t--) {
        int ty = y - t * 8;
        if (ty >= 0 && ty < 64) {
          display.setCursor(x, ty);
          display.setTextSize(1);
          display.print((char)(random(33, 126)));
        }
      }

      // draw head (bright)
      if (y >= 0 && y < 64) {
        display.setCursor(x, y);
        display.print((char)(random(33, 126)));
      }

      drops[c]++;
      if (drops[c] * 8 > 80) drops[c] = random(-10, 0);
    }

    display.display();
    delay(60);
  }

  // PLANT-X reveal — typewriter over fading matrix
  const char* title = "PLANT-X";
  int titleLen = 7;
  for (int frame = 0; frame < 30; frame++) {
    clearScreen();

    // fading matrix behind — gets sparser each frame
    int density = max(0, 12 - frame);
    for (int i = 0; i < density; i++) {
      int x = random(0, 16) * 8;
      int y = random(0, 8) * 8;
      display.setCursor(x, y);
      display.setTextSize(1);
      display.print((char)(random(33, 126)));
    }

    // reveal title chars one by one
    int charsToShow = map(frame, 0, 20, 0, titleLen);
    charsToShow = constrain(charsToShow, 0, titleLen);

    display.setTextSize(2);
    // center manually
    int startX = (128 - titleLen * 12) / 2;
    for (int c = 0; c < charsToShow; c++) {
      display.setCursor(startX + c * 12, 18);
      display.print(title[c]);
    }
    // last char flickers until next one arrives
    if (charsToShow < titleLen && charsToShow > 0 && frame % 2 == 0) {
      display.setCursor(startX + charsToShow * 12, 18);
      display.print((char)(random(33, 126)));
    }

    // subtitle fades in after title done
    if (frame > 22) {
      display.setTextSize(1);
      printCentered("SMART PLANT SYSTEM", 40);
    }

    display.display();
    delay(70);
  }

  delay(400);

  // real hardware checks
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  bool dhtOk  = !isnan(t) && !isnan(h);
  bool soilOk = analogRead(SOIL) >= 0;
  bool ldrOk  = analogRead(LDR)  >= 0;

  // system check with typewriter effect
  clearScreen();
  display.setTextSize(1);
  display.setCursor(0, 0); display.println("SYSTEM CHECK");
  display.drawLine(0, 9, 128, 9, WHITE);
  display.display(); delay(100);

  const char* labels[] = {"OLED  ","KEYPAD","DHT11 ","SOIL  ","LDR   "};
  bool results[]       = {true,   true,    dhtOk,  soilOk, ldrOk };

  for (int i = 0; i < 5; i++) {
    display.setCursor(0, 16 + i * 10);
    display.print(labels[i]);
    display.print("  ");
    display.display();
    delay(150);
    display.print(results[i] ? "OK" : "FAIL");
    display.display();
    delay(100);
  }

  delay(800);
}

// ============================================================
// SENSOR READ
// ============================================================

void readSensors() {
  int rawSoil = analogRead(SOIL);
  moisture = constrain(map(rawSoil, 1000, 600, 0, 100), 0, 100);
  light = analogRead(LDR);
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) temp = t;
  if (!isnan(h)) hum  = h;
}

// ============================================================
// HOME SCREEN — animated growing plant
// ============================================================

int plantGrowth = 0;       // 0-100, grows once
int leafFrame   = 0;       // sways forever
bool plantFullyGrown = false;
unsigned long lastPlantFrame = 0;
#define PLANT_FRAME_MS 60

void drawPlantHome() {
  unsigned long now = millis();
  if (now - lastPlantFrame >= PLANT_FRAME_MS) {
    lastPlantFrame = now;
    if (!plantFullyGrown) {
      plantGrowth += 3;
      if (plantGrowth >= 100) { plantGrowth = 100; plantFullyGrown = true; }
    }
    leafFrame++;
  }

  clearScreen();

  // title
  display.setTextSize(1);
  printCentered("PLANT-X", 0);
  display.drawLine(0, 9, 128, 9, WHITE);

  // === PLANT ===
  int cx = 64;

  // pot
  int potTop = 57;
  // trapezoid pot
  display.drawLine(cx-12, potTop,   cx+12, potTop,   WHITE); // rim
  display.drawLine(cx-12, potTop,   cx-9,  63,        WHITE); // left
  display.drawLine(cx+12, potTop,   cx+9,  63,        WHITE); // right
  display.drawLine(cx-9,  63,       cx+9,  63,        WHITE); // bottom
  // soil line
  display.drawLine(cx-11, potTop+2, cx+11, potTop+2,  WHITE);

  // stem — grows from pot up
  int stemBase = potTop - 1;
  int stemTop  = stemBase - map(plantGrowth, 0, 100, 0, 38);

  // main stem with slight curve
  for (int y = stemTop; y <= stemBase; y++) {
    int offset = (y - stemBase) / 8; // slight lean
    display.drawPixel(cx + offset, y, WHITE);
    display.drawPixel(cx + offset + 1, y, WHITE); // 2px wide
  }

  if (plantGrowth > 25) {
    // left branch — lower
    int branchY = stemBase - map(plantGrowth, 25, 100, 0, 14);
    bool sway = (leafFrame / 5) % 2 == 0;
    int tip = sway ? -10 : -9;
    display.drawLine(cx, branchY, cx+tip, branchY-5, WHITE);
    // leaf shape on branch
    display.drawLine(cx+tip, branchY-5, cx+tip-4, branchY-3, WHITE);
    display.drawLine(cx+tip-4, branchY-3, cx, branchY, WHITE);
  }

  if (plantGrowth > 50) {
    // right branch — higher
    int branchY = stemBase - map(plantGrowth, 50, 100, 0, 20);
    bool sway = (leafFrame / 5) % 2 == 0;
    int tip = sway ? 11 : 10;
    display.drawLine(cx+1, branchY, cx+tip, branchY-6, WHITE);
    display.drawLine(cx+tip, branchY-6, cx+tip+4, branchY-3, WHITE);
    display.drawLine(cx+tip+4, branchY-3, cx+1, branchY, WHITE);
  }

  if (plantGrowth > 70) {
    // second left leaf — near top
    int branchY = stemBase - map(plantGrowth, 70, 100, 0, 28);
    bool sway = (leafFrame / 4) % 2 == 0;
    int tip = sway ? -9 : -11;
    display.drawLine(cx, branchY, cx+tip, branchY-4, WHITE);
    display.drawLine(cx+tip, branchY-4, cx+tip-3, branchY-2, WHITE);
    display.drawLine(cx+tip-3, branchY-2, cx, branchY, WHITE);
  }

  // rose at top when fully grown
  if (plantGrowth >= 90) {
    int fy = stemTop - 1;
    int bloom = map(plantGrowth, 90, 100, 0, 4);
    bool pb = (leafFrame / 8) % 2 == 0;

    if (bloom >= 1) {
      display.fillCircle(cx+1, fy, 2, WHITE);
    }
    if (bloom >= 2) {
      int pd = pb ? 0 : 1;
      display.drawCircle(cx+1, fy-3-pd, 2, WHITE);
      display.drawCircle(cx-3, fy+1,    2, WHITE);
      display.drawCircle(cx+5, fy+1,    2, WHITE);
    }
    if (bloom >= 3) {
      display.fillCircle(cx+1, fy-3, 2, WHITE);
      display.fillCircle(cx-3, fy+1, 2, WHITE);
      display.fillCircle(cx+5, fy+1, 2, WHITE);
      display.fillCircle(cx+1, fy+3, 2, WHITE);
      display.drawPixel(cx+1, fy, BLACK);
      display.drawPixel(cx,   fy, BLACK);
    }
  }

  // wind particles — natural behavior with vertical drift
  if (plantFullyGrown) {
    int wf = leafFrame;
    for (int i = 0; i < 10; i++) {
      // each particle has unique horizontal and vertical movement
      int baseY  = (i * 13 + 11) % 50 + 12;
      int spd    = (i % 3) + 2;
      int drift  = (i % 5) - 2; // -2 to +2 vertical drift per cycle
      int px2    = (wf * spd + i * 31) % 135 - 5;
      int py     = baseY + ((wf / 8 + i) % 5) * drift / 3;
      py = constrain(py, 11, 62);
      if (px2 < 0 || px2 > 127) continue;

      int type = i % 3;
      if (type == 0) {
        // dot — dust/dirt
        display.drawPixel(px2, py, WHITE);
        if (i%2==0) display.drawPixel(px2+1, py, WHITE); // slightly bigger
      } else if (type == 1) {
        // dash — twig
        display.drawLine(px2, py, min(px2+3,127), py+1, WHITE);
      } else {
        // leaf — curved shape
        display.drawPixel(px2,   py,   WHITE);
        display.drawPixel(min(px2+1,127), py-1, WHITE);
        display.drawPixel(min(px2+2,127), py,   WHITE);
        display.drawPixel(min(px2+1,127), py+1, WHITE);
      }
    }
  }

  // press hint
  display.setTextSize(1);
  display.setCursor(0, 57);
  display.print("Menu(5)");

  display.display();
}

void drawSystem() {
  clearScreen();
  display.setTextSize(1);
  printCentered("SYSTEM INFO", 0);
  display.drawLine(0, 9, 128, 9, WHITE);

  display.setCursor(0, 13); display.print("MCU:   STM32F103C8T6");
  display.setCursor(0, 23); display.print("CPU:   Cortex-M3");
  display.setCursor(0, 33); display.print("CLK:   72 MHz");
  display.setCursor(0, 43); display.print("FLASH: 64 KB");
  display.setCursor(0, 53); display.print("SRAM:  20 KB");

  unsigned long sec = millis() / 1000;
  display.setCursor(70, 53);
  display.print("UP:");
  display.print(sec / 60); display.print("m");
  display.print(sec % 60); display.print("s");

  display.display();
}

// ============================================================
// FLIPPER SCROLL MENU
// ============================================================

void drawScrollMenu(const char* title, const char** items, int count, int selected) {
  clearScreen();
  int prev = (selected - 1 + count) % count;
  int next = (selected + 1) % count;
  display.setTextSize(1);
  printCentered(title, 0);
  display.drawLine(0, 9, 120, 9, WHITE);
  display.setCursor(4, 13); display.print(items[prev]);
  display.fillRoundRect(0, 22, 120, 14, 3, WHITE);
  display.setTextColor(BLACK);
  display.setCursor(4, 26); display.print(items[selected]);
  display.setTextColor(WHITE);
  display.setCursor(4, 40); display.print(items[next]);
  display.setCursor(0, 56); display.print("2/8:move 5:ok *:back");
  display.drawRect(122, 0, 6, 64, WHITE);
  int barH = max(4, 64 / count);
  int barY = (64 - barH) * selected / max(1, count - 1);
  display.fillRect(123, barY, 4, barH, WHITE);
  display.display();
}

// ============================================================
// MENUS
// ============================================================

void drawMainMenu() {
  const char* items[] = {"Monitor", "Games", "Settings", "System", "About"};
  drawScrollMenu("PLANT-X", items, 5, mainSelection);
}
void handleMainMenu(char key) {
  if      (key=='2') { mainSelection--; if (mainSelection<0) mainSelection=4; }
  else if (key=='8') { mainSelection++; if (mainSelection>4) mainSelection=0; }
  else if (key=='5') {
    if      (mainSelection==0) { screen=MONITOR_MENU; monitorSelection=0; }
    else if (mainSelection==1) { screen=GAME_MENU;    gameSelection=0;    }
    else if (mainSelection==2) { screen=SETTINGS_MENU; }
    else if (mainSelection==3) { screen=SYSTEM_SCREEN; }
    else if (mainSelection==4) { screen=ABOUT_SCREEN; }
  }
  else if (key=='*') screen=HOME_SCREEN;
}

void drawMonitorMenu() {
  const char* items[] = {"Temperature","Humidity","Soil Moisture","Light (LDR)","All Sensors"};
  drawScrollMenu("MONITOR", items, 5, monitorSelection);
}
void handleMonitorMenu(char key) {
  if      (key=='2') { monitorSelection--; if (monitorSelection<0) monitorSelection=4; }
  else if (key=='8') { monitorSelection++; if (monitorSelection>4) monitorSelection=0; }
  else if (key=='5') {
    switch(monitorSelection) {
      case 0: screen=TEMP_SCREEN;       break;
      case 1: screen=HUMIDITY_SCREEN;   break;
      case 2: screen=SOIL_SCREEN;       break;
      case 3: screen=LIGHT_SCREEN;      break;
      case 4: screen=ALL_SENSOR_SCREEN; break;
    }
  }
  else if (key=='*') screen=MAIN_MENU;
}

void drawGameMenu() {
  const char* games[] = {"Snake","Tetris","Dino Run","Space Inv.","Breakout"};
  drawScrollMenu("GAME CENTER", games, 5, gameSelection);
}
void handleGameMenu(char key) {
  if      (key=='2') { gameSelection--; if (gameSelection<0) gameSelection=4; }
  else if (key=='8') { gameSelection++; if (gameSelection>4) gameSelection=0; }
  else if (key=='5') {
    if      (gameSelection==0) { initSnake();         screen=SNAKE_GAME;     }
    else if (gameSelection==1) { initTetris();        screen=TETRIS_GAME;    }
    else if (gameSelection==2) { initDino();          screen=DINO_GAME;      }
    else if (gameSelection==3) { initSpaceInvaders(); screen=SPACE_INVADERS; }
    else if (gameSelection==4) { initBreakout();      screen=BREAKOUT_GAME;  }
  }
  else if (key=='*') screen=MAIN_MENU;
}

void drawSettingsMenu() {
  clearScreen();
  display.setTextSize(1); printCentered("SETTINGS", 0);
  display.drawLine(0, 9, 120, 9, WHITE);
  display.fillRoundRect(0, 22, 120, 14, 3, WHITE);
  display.setTextColor(BLACK);
  display.setCursor(4, 26); display.print("Buzzer: "); display.print(buzzerEnabled?"ON ":"OFF");
  display.setTextColor(WHITE);
  display.setCursor(4, 44); display.print("5:toggle  *:back");
  display.drawRect(122, 0, 6, 64, WHITE);
  display.fillRect(123, 0, 4, 64, WHITE);
  display.display();
}
void handleSettingsMenu(char key) {
  if (key=='5') buzzerEnabled=!buzzerEnabled;
  else if (key=='*') screen=MAIN_MENU;
}

// ============================================================
// ABOUT — DVD bouncing names
// ============================================================

struct DVDName {
  float x, y, vx, vy;
  const char* name;
};

DVDName dvdNames[] = {
  {10, 10, 1.2, 0.8, "Pavel"},
  {60, 30, -1.0, 1.1, "Yeasin"},
  {20, 45, 1.5, -0.9, "Emu"},
  {90, 15, -1.3, 1.0, "Mashrur"}
};

void updateDVD() {
  for (int i = 0; i < 4; i++) {
    dvdNames[i].x += dvdNames[i].vx;
    dvdNames[i].y += dvdNames[i].vy;
    int w = strlen(dvdNames[i].name) * 6;
    if (dvdNames[i].x <= 0)         { dvdNames[i].x = 0;       dvdNames[i].vx = -dvdNames[i].vx; }
    if (dvdNames[i].x >= 128 - w)   { dvdNames[i].x = 128 - w; dvdNames[i].vx = -dvdNames[i].vx; }
    if (dvdNames[i].y <= 10)        { dvdNames[i].y = 10;      dvdNames[i].vy = -dvdNames[i].vy; }
    if (dvdNames[i].y >= 56)        { dvdNames[i].y = 56;      dvdNames[i].vy = -dvdNames[i].vy; }
  }
}

void drawAbout() {
  updateDVD();
  clearScreen();
  display.setTextSize(1);
  printCentered("PLANT-X  *=back", 0);
  display.drawLine(0, 9, 128, 9, WHITE);
  for (int i = 0; i < 4; i++) {
    display.setCursor((int)dvdNames[i].x, (int)dvdNames[i].y);
    display.print(dvdNames[i].name);
  }
  display.display();
}

// ============================================================
// SENSOR SCREENS
// ============================================================

int fireFrame = 0;

void drawFire() {
  // layered fire — wider base, narrower top
  for (int x=0; x<22; x++) {
    int maxH = (x < 4 || x > 17) ? random(3,7) : random(6,14);
    int baseY = 63;
    for (int y=baseY; y>baseY-maxH; y--)
      display.drawPixel(x+92, y, WHITE);
  }
  // bright core flicker
  for (int x=4; x<18; x++) {
    if (random(0,2)==0) {
      int h=random(3,7);
      for (int y=63; y>63-h; y--)
        display.drawPixel(x+92, y, WHITE);
    }
  }
}
void drawIce() {
  int cx=108, cy=37;
  // main cross
  display.drawLine(cx-10, cy,    cx+10, cy,    WHITE);
  display.drawLine(cx,    cy-10, cx,    cy+10, WHITE);
  // diagonals
  display.drawLine(cx-7, cy-7, cx+7, cy+7, WHITE);
  display.drawLine(cx+7, cy-7, cx-7, cy+7, WHITE);
  // center dot
  display.fillCircle(cx, cy, 2, WHITE);
  // branch tips
  if (fireFrame % 2 == 0) {
    display.drawLine(cx-10, cy,   cx-12, cy-2, WHITE);
    display.drawLine(cx-10, cy,   cx-12, cy+2, WHITE);
    display.drawLine(cx+10, cy,   cx+12, cy-2, WHITE);
    display.drawLine(cx+10, cy,   cx+12, cy+2, WHITE);
    display.drawLine(cx, cy-10,   cx-2, cy-12, WHITE);
    display.drawLine(cx, cy-10,   cx+2, cy-12, WHITE);
    display.drawLine(cx, cy+10,   cx-2, cy+12, WHITE);
    display.drawLine(cx, cy+10,   cx+2, cy+12, WHITE);
  } else {
    display.drawPixel(cx-5, cy-8, WHITE);
    display.drawPixel(cx+5, cy+8, WHITE);
    display.drawPixel(cx+5, cy-8, WHITE);
    display.drawPixel(cx-5, cy+8, WHITE);
  }
}

void drawTemp(float temperature) {
  clearScreen();
  display.setTextSize(1); display.setCursor(0,0); display.println("TEMPERATURE");
  display.drawLine(0,10,128,10,WHITE);
  display.setTextSize(2); display.setCursor(5,20); display.print(temperature,1);
  display.setTextSize(1); display.setCursor(75,20); display.println("o");
  display.setTextSize(2); display.setCursor(82,24); display.println("C");
  display.setTextSize(1); display.setCursor(0,55);
  if      (temperature<27)  { drawIce();  display.println("!! COLD !!"); }
  else if (temperature<=30) {             display.println("Comfortable"); }
  else                      { drawFire(); display.println("!! HOT !!");  }
  display.display(); fireFrame++;
  if (temperature>30) alert();
}

void drawHumidity(float humidity) {
  static bool filled = false;
  static int lastHum = -1;
  int cur = (int)humidity;
  if (cur != lastHum) { filled = false; lastHum = cur; }

  clearScreen();
  display.setTextSize(1);
  display.setCursor(0, 0); display.println("HUMIDITY");
  display.drawLine(0, 10, 128, 10, WHITE);

  // big number centered
  display.setTextSize(3);
  display.setCursor(38, 16); display.print(cur);
  display.setTextSize(2);
  display.setCursor(98, 22); display.println("%");

  // droplet icon top left
  display.drawPixel(8, 14, WHITE);
  display.drawLine(6,16,10,16,WHITE);
  display.drawLine(5,17,11,17,WHITE);
  display.drawLine(5,18,11,18,WHITE);
  display.drawLine(5,19,11,19,WHITE);
  display.drawLine(6,20,10,20,WHITE);
  display.drawPixel(8, 21, WHITE);
  display.drawPixel(6, 17, BLACK);
  display.drawPixel(6, 18, BLACK);

  // status text
  display.setTextSize(1);
  display.setCursor(0, 44);
  if      (humidity > 70) display.println("Humid - stay cool!");
  else if (humidity > 40) display.println("Normal - comfy!");
  else                    display.println("Dry - drink water!");

  // animated fill bar
  display.drawRect(0, 54, 128, 10, WHITE);
  int fill = 126 * cur / 100;
  if (!filled) {
    for (int i = 0; i <= fill; i++) {
      display.drawFastVLine(1+i, 55, 8, WHITE);
      if (i % 3 == 0) { display.display(); delay(10); }
    }
    filled = true;
  } else {
    for (int i = 0; i <= fill; i++) display.drawFastVLine(1+i, 55, 8, WHITE);
  }
  display.display();
}

void drawSoil(int sm) {
  clearScreen();
  display.setTextSize(1); display.setCursor(0,0); display.println("SOIL MOISTURE");
  display.drawLine(0,10,128,10,WHITE);
  display.setTextSize(3); display.setCursor(35,14); display.print(sm);
  display.setTextSize(2); display.setCursor(95,20); display.println("%");
  // better pot
  display.drawLine(7, 44, 21, 44, WHITE);   // rim
  display.drawLine(9, 45, 19, 45, WHITE);
  display.drawLine(9, 45,  7, 53, WHITE);   // left side
  display.drawLine(19, 45, 21, 53, WHITE);  // right side
  display.drawLine(7, 53, 21, 53, WHITE);   // bottom
  // soil top
  display.drawLine(9, 44, 19, 44, WHITE);
  // stem
  display.drawLine(14, 43, 14, 34, WHITE);
  // left leaf
  display.drawLine(14, 38, 10, 34, WHITE);
  display.drawLine(10, 34, 9,  36, WHITE);
  display.drawLine(9,  36, 13, 38, WHITE);
  // right leaf
  display.drawLine(14, 36, 19, 32, WHITE);
  display.drawLine(19, 32, 20, 34, WHITE);
  display.drawLine(20, 34, 15, 37, WHITE);
  display.setTextSize(1); display.setCursor(30,44);
  if      (sm<30) display.println("DRY-Water me!");
  else if (sm<60) display.println("OK-Happy!");
  else            display.println("WET-Good!");
  display.drawRect(0,54,128,10,WHITE);
  int fill=126*sm/100;
  for (int i=0; i<=fill; i++) display.drawFastVLine(1+i,55,8,WHITE);
  display.display();
  if (sm<30) alert();
}

void drawLight(int lv) {
  clearScreen();
  display.setTextSize(1); display.setCursor(0,0); display.println("LIGHT (LDR)");
  display.drawLine(0,10,128,10,WHITE);
  display.setTextSize(3); display.setCursor(10,16); display.print(lv);
  // better sun icon
  int cx=108, cy=36;
  display.fillCircle(cx, cy, 6, WHITE);
  // inner dark for detail
  display.drawCircle(cx, cy, 4, BLACK);
  // rays
  display.drawLine(cx, cy-9,  cx,   cy-11, WHITE);
  display.drawLine(cx, cy+9,  cx,   cy+11, WHITE);
  display.drawLine(cx-9, cy,  cx-11, cy,   WHITE);
  display.drawLine(cx+9, cy,  cx+11, cy,   WHITE);
  display.drawLine(cx-7, cy-7, cx-9, cy-9, WHITE);
  display.drawLine(cx+7, cy-7, cx+9, cy-9, WHITE);
  display.drawLine(cx-7, cy+7, cx-9, cy+9, WHITE);
  display.drawLine(cx+7, cy+7, cx+9, cy+9, WHITE);
  display.setTextSize(1); display.setCursor(0,44);
  if      (lv<100) { display.println("Need light!"); alert(); }
  else if (lv<500) display.println("NORMAL LIGHT");
  else             display.println("BRIGHT");
  display.drawRect(0,56,128,7,WHITE);
  int fill=constrain(map(lv,0,1000,0,126),0,126);
  for (int i=0; i<=fill; i++) display.drawFastVLine(1+i,57,5,WHITE);
  display.display();
}

void drawAll(float t, float h, int sm, int lv) {
  clearScreen();
  display.setTextSize(1); printCentered("ALL SENSORS",0);
  display.drawLine(0,10,128,10,WHITE);
  display.setCursor(0,15); display.print("Temp:  "); display.print(t,1); display.print((char)247); display.println("C");
  display.setCursor(0,26); display.print("Hum:   "); display.print((int)h); display.println(" %");
  display.setCursor(0,37); display.print("Soil:  "); display.print(sm); display.println(" %");
  display.setCursor(0,48); display.print("Light: "); display.println(lv);
  display.setCursor(0,58); display.println("* Back");
  display.display();
  if (t>30 || sm<30) alert();
}

// ============================================================
// SNAKE
// ============================================================

#define GRID_W 21
#define GRID_H 10
#define CELL    6
#define MAX_LEN 50
#define OFFSET_X 3
#define OFFSET_Y 4

int snakeX[MAX_LEN], snakeY[MAX_LEN];
int snakeLen, dirX, dirY, foodX, foodY, snakeScore;
bool snakeGameOver;
unsigned long lastSnakeMove=0;
#define SNAKE_SPEED 200

void spawnFood() {
  bool on=true;
  while(on) {
    foodX=random(0,GRID_W); foodY=random(0,GRID_H); on=false;
    for(int i=0;i<snakeLen;i++) if(snakeX[i]==foodX&&snakeY[i]==foodY){on=true;break;}
  }
}
void initSnake() {
  snakeLen=3; dirX=1; dirY=0;
  snakeX[0]=5;snakeY[0]=4; snakeX[1]=4;snakeY[1]=4; snakeX[2]=3;snakeY[2]=4;
  snakeScore=0; snakeGameOver=false; spawnFood(); lastSnakeMove=millis();
}
void drawSnake() {
  clearScreen();
  if (snakeGameOver) {
    display.setTextSize(2); display.setCursor(10,15); display.println("GAME OVER");
    display.setTextSize(1);
    display.setCursor(25,35); display.print("Score: "); display.println(snakeScore);
    display.setCursor(10,48); display.println("#=restart");
    display.setCursor(10,57); display.println("*=menu");
    display.display(); return;
  }
  display.fillRect(OFFSET_X+foodX*CELL, OFFSET_Y+foodY*CELL, CELL-1, CELL-1, WHITE);
  for(int i=0;i<snakeLen;i++) {
    if(i==0) display.fillRect(OFFSET_X+snakeX[i]*CELL,OFFSET_Y+snakeY[i]*CELL,CELL-1,CELL-1,WHITE);
    else     display.drawRect(OFFSET_X+snakeX[i]*CELL,OFFSET_Y+snakeY[i]*CELL,CELL-1,CELL-1,WHITE);
  }
  display.display();
}
void updateSnake() {
  if(snakeGameOver) return;
  int nx=snakeX[0]+dirX, ny=snakeY[0]+dirY;
  if(nx<0) nx=GRID_W-1; if(nx>=GRID_W) nx=0;
  if(ny<0) ny=GRID_H-1; if(ny>=GRID_H) ny=0;
  for(int i=0;i<snakeLen;i++) if(snakeX[i]==nx&&snakeY[i]==ny){snakeGameOver=true;gameOverSound();return;}
  for(int i=snakeLen-1;i>0;i--){snakeX[i]=snakeX[i-1];snakeY[i]=snakeY[i-1];}
  snakeX[0]=nx; snakeY[0]=ny;
  if(nx==foodX&&ny==foodY){if(snakeLen<MAX_LEN)snakeLen++;snakeScore++;eatSound();spawnFood();}
}

// ============================================================
// TETRIS
// ============================================================

#define T_W 42
#define T_H 21
#define T_CELL 3
#define T_OFF_X 0
#define T_OFF_Y 1

byte tBoard[T_H][T_W];
const byte tPieces[7][4][4]={
  {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
  {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
  {{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
  {{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
  {{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
  {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
  {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}}
};
int tPiece,tX,tY,tRot,tScore,tLines,tLevel;
bool tGameOver;
unsigned long tLastDrop=0;

void tGetCell(int piece,int rot,int px,int py,int&bx,int&by){
  int rx=px,ry=py;
  for(int r=0;r<rot;r++){int tmp=rx;rx=3-ry;ry=tmp;}
  bx=rx;by=ry;
}
bool tCollide(int px,int py,int piece,int rot){
  for(int y=0;y<4;y++)for(int x=0;x<4;x++){
    if(!tPieces[piece][y][x])continue;
    int bx,by;tGetCell(piece,rot,x,y,bx,by);
    int fx=px+bx,fy=py+by;
    if(fx<0||fx>=T_W||fy>=T_H)return true;
    if(fy>=0&&tBoard[fy][fx])return true;
  }
  return false;
}
void tPlace(){
  for(int y=0;y<4;y++)for(int x=0;x<4;x++){
    if(!tPieces[tPiece][y][x])continue;
    int bx,by;tGetCell(tPiece,tRot,x,y,bx,by);
    int fx=tX+bx,fy=tY+by;
    if(fx>=0&&fx<T_W&&fy>=0&&fy<T_H)tBoard[fy][fx]=1;
  }
}
void tClearLines(){
  int cleared=0;
  for(int y=T_H-1;y>=0;y--){
    bool full=true;
    for(int x=0;x<T_W;x++)if(!tBoard[y][x]){full=false;break;}
    if(full){cleared++;for(int yy=y;yy>0;yy--)for(int x=0;x<T_W;x++)tBoard[yy][x]=tBoard[yy-1][x];for(int x=0;x<T_W;x++)tBoard[0][x]=0;y++;}
  }
  if(cleared>0){
    tLines+=cleared;
    if(cleared==1)tScore+=100;else if(cleared==2)tScore+=300;else if(cleared==3)tScore+=500;else tScore+=800;
    tLevel=1+(tLines/10);eatSound();
  }
}
void tNewPiece(){tPiece=random(0,7);tRot=0;tX=T_W/2-2;tY=0;if(tCollide(tX,tY,tPiece,tRot))tGameOver=true;}
void tRotate(){
  if(tGameOver)return;int nr=(tRot+1)%4;
  if(!tCollide(tX,tY,tPiece,nr))tRot=nr;
  else if(!tCollide(tX-1,tY,tPiece,nr)){tX--;tRot=nr;}
  else if(!tCollide(tX+1,tY,tPiece,nr)){tX++;tRot=nr;}
}
void tDrop(){
  if(tGameOver)return;
  if(!tCollide(tX,tY+1,tPiece,tRot))tY++;
  else{tPlace();tClearLines();tNewPiece();}
}
void initTetris(){
  for(int y=0;y<T_H;y++)for(int x=0;x<T_W;x++)tBoard[y][x]=0;
  tScore=0;tLines=0;tLevel=1;tGameOver=false;tLastDrop=millis();tNewPiece();
}
void drawTetris(){
  clearScreen();
  for(int y=0;y<T_H;y++)for(int x=0;x<T_W;x++)
    if(tBoard[y][x])display.fillRect(T_OFF_X+x*T_CELL,T_OFF_Y+y*T_CELL,T_CELL-1,T_CELL-1,WHITE);
  if(!tGameOver){
    for(int y=0;y<4;y++)for(int x=0;x<4;x++){
      if(!tPieces[tPiece][y][x])continue;
      int bx,by;tGetCell(tPiece,tRot,x,y,bx,by);
      int fx=tX+bx,fy=tY+by;
      if(fx>=0&&fx<T_W&&fy>=0&&fy<T_H)display.fillRect(T_OFF_X+fx*T_CELL,T_OFF_Y+fy*T_CELL,T_CELL-1,T_CELL-1,WHITE);
    }
  }
  display.setTextSize(1);
  display.setCursor(104,0);display.print("S:"); display.print(tScore);
  display.setCursor(104,8);display.print("L:"); display.print(tLevel);
  if(tGameOver){
    clearScreen();
    display.setTextSize(2);display.setCursor(10,15);display.println("GAME OVER");
    display.setTextSize(1);
    display.setCursor(25,35);display.print("Score: ");display.println(tScore);
    display.setCursor(25,45);display.print("Lines: ");display.println(tLines);
    display.setCursor(10,55);display.println("#=restart  *=menu");
    display.display();return;
  }
  display.display();
}

// ============================================================
// DINO RUN
// ============================================================

#define DINO_X 10
#define GROUND_Y 54
#define DINO_H 8
#define DINO_W 8
#define CACTUS_W 5
#define CACTUS_H 12
#define JUMP_FORCE -10

int dinoY,dinoVY,cactusX,dinoScore;
bool dinoOnGround,dinoGameOver;
unsigned long dinoLastFrame=0;
#define DINO_FRAME_MS 40

void initDino(){
  dinoY=GROUND_Y-DINO_H;dinoVY=0;dinoOnGround=true;
  cactusX=140;dinoScore=0;dinoGameOver=false;dinoLastFrame=millis();
}
void drawDinoChar(int x,int y){
  display.fillRect(x,y,DINO_W,DINO_H,WHITE);
  display.drawPixel(x+DINO_W-2,y+1,BLACK);
  if((dinoScore/5)%2==0){display.drawLine(x+2,y+DINO_H,x+2,y+DINO_H+2,WHITE);display.drawLine(x+5,y+DINO_H,x+5,y+DINO_H+1,WHITE);}
  else{display.drawLine(x+2,y+DINO_H,x+2,y+DINO_H+1,WHITE);display.drawLine(x+5,y+DINO_H,x+5,y+DINO_H+2,WHITE);}
}
void updateDino(){
  if(dinoGameOver)return;
  dinoVY+=1;dinoY+=dinoVY;
  if(dinoY>=GROUND_Y-DINO_H){dinoY=GROUND_Y-DINO_H;dinoVY=0;dinoOnGround=true;}
  int speed=2+dinoScore/20;
  cactusX-=speed;
  if(cactusX<-CACTUS_W){cactusX=140;dinoScore++;}
  if(cactusX<DINO_X+DINO_W-2&&cactusX+CACTUS_W>DINO_X&&dinoY+DINO_H>GROUND_Y-CACTUS_H){dinoGameOver=true;gameOverSound();}
}
void drawDino(){
  clearScreen();
  if(dinoGameOver){
    display.setTextSize(2);display.setCursor(10,15);display.println("GAME OVER");
    display.setTextSize(1);display.setCursor(25,35);display.print("Score: ");display.println(dinoScore);
    display.setCursor(10,48);display.println("#=restart");display.setCursor(10,57);display.println("*=menu");
    display.display();return;
  }
  display.drawLine(0,GROUND_Y,128,GROUND_Y,WHITE);
  drawDinoChar(DINO_X,dinoY);
  display.fillRect(cactusX,GROUND_Y-CACTUS_H,CACTUS_W,CACTUS_H,WHITE);
  display.fillRect(cactusX-3,GROUND_Y-CACTUS_H+3,3,4,WHITE);
  display.fillRect(cactusX+CACTUS_W,GROUND_Y-CACTUS_H+5,3,4,WHITE);
  display.display();
}

// ============================================================
// SPACE IMPACT
// ============================================================

#define SI_MAX_ENEMIES 6
#define SI_MAX_BULLETS 4
#define SI_PLAYER_Y    56
#define SI_PLAYER_W    8
#define SI_AUTO_SHOOT  400

struct SIEnemy{int x,y;bool active;int type;};
struct SIBullet{int x,y;bool active;};

SIEnemy  siEnemies[SI_MAX_ENEMIES];
SIBullet siBullets[SI_MAX_BULLETS];
int siPlayerX,siScore,siFrame;
bool siGameOver;
unsigned long siLastAutoShot=0,siLastSpawn=0,siLastUpdate=0;
#define SI_UPDATE_MS 25
#define SI_SPAWN_MS  700

void initSpaceInvaders(){
  siPlayerX=60;siScore=0;siGameOver=false;siFrame=0;
  for(int i=0;i<SI_MAX_ENEMIES;i++)siEnemies[i].active=false;
  for(int i=0;i<SI_MAX_BULLETS;i++)siBullets[i].active=false;
  siLastAutoShot=siLastSpawn=siLastUpdate=millis();
}
void siSpawnEnemy(){
  for(int i=0;i<SI_MAX_ENEMIES;i++)if(!siEnemies[i].active){siEnemies[i]={random(4,120),-10,true,random(0,3)};return;}
}
void siFireBullet(){
  for(int i=0;i<SI_MAX_BULLETS;i++)if(!siBullets[i].active){siBullets[i]={siPlayerX+SI_PLAYER_W/2,SI_PLAYER_Y-2,true};return;}
}
void updateSpaceInvaders(){
  if(siGameOver)return;
  unsigned long now=millis();
  if(now-siLastUpdate<SI_UPDATE_MS)return;
  siLastUpdate=now;siFrame++;
  if(now-siLastAutoShot>=SI_AUTO_SHOOT){siLastAutoShot=now;siFireBullet();eatSound();}
  unsigned long spawnMs=(unsigned long)max(200,(int)(SI_SPAWN_MS-siScore*4));
  if(now-siLastSpawn>=spawnMs){siLastSpawn=now;siSpawnEnemy();}
  for(int i=0;i<SI_MAX_BULLETS;i++){
    if(!siBullets[i].active)continue;
    siBullets[i].y-=4;
    if(siBullets[i].y<0){siBullets[i].active=false;continue;}
    for(int e=0;e<SI_MAX_ENEMIES;e++){
      if(!siEnemies[e].active)continue;
      int ew=(siEnemies[e].type==0)?5:(siEnemies[e].type==1)?7:9;
      int eh=(siEnemies[e].type==0)?4:(siEnemies[e].type==1)?6:8;
      if(siBullets[i].x>=siEnemies[e].x&&siBullets[i].x<=siEnemies[e].x+ew&&siBullets[i].y>=siEnemies[e].y&&siBullets[i].y<=siEnemies[e].y+eh){
        siEnemies[e].active=false;siBullets[i].active=false;siScore+=(siEnemies[e].type+1)*10;eatSound();break;
      }
    }
  }
  int espeed=1+siScore/50;
  for(int e=0;e<SI_MAX_ENEMIES;e++){
    if(!siEnemies[e].active)continue;
    siEnemies[e].y+=espeed;
    if(siEnemies[e].y>64){siEnemies[e].active=false;continue;}
    int ew=(siEnemies[e].type==0)?5:(siEnemies[e].type==1)?7:9;
    int eh=(siEnemies[e].type==0)?4:(siEnemies[e].type==1)?6:8;
    if(siEnemies[e].y+eh>=SI_PLAYER_Y&&siEnemies[e].x+ew>=siPlayerX&&siEnemies[e].x<=siPlayerX+SI_PLAYER_W){siGameOver=true;gameOverSound();}
  }
}
void drawSpaceInvaders(){
  clearScreen();
  if(siGameOver){
    display.setTextSize(2);display.setCursor(10,12);display.println("GAME OVER");
    display.setTextSize(1);display.setCursor(25,35);display.print("Score: ");display.println(siScore);
    display.setCursor(10,48);display.println("#=restart");display.setCursor(10,57);display.println("*=menu");
    display.display();return;
  }
  for(int i=0;i<10;i++){display.drawPixel((i*23+siFrame)%128,(i*13+siFrame*2)%52,WHITE);}
  display.fillRect(siPlayerX,SI_PLAYER_Y,SI_PLAYER_W,4,WHITE);
  display.drawPixel(siPlayerX+SI_PLAYER_W/2,SI_PLAYER_Y-2,WHITE);
  display.drawPixel(siPlayerX+SI_PLAYER_W/2,SI_PLAYER_Y-1,WHITE);
  for(int i=0;i<SI_MAX_BULLETS;i++)if(siBullets[i].active)display.drawFastVLine(siBullets[i].x,siBullets[i].y,3,WHITE);
  for(int e=0;e<SI_MAX_ENEMIES;e++){
    if(!siEnemies[e].active)continue;
    int ex=siEnemies[e].x,ey=siEnemies[e].y;
    if(siEnemies[e].type==0){display.drawPixel(ex+2,ey,WHITE);display.drawLine(ex,ey+2,ex+4,ey+2,WHITE);display.drawPixel(ex+2,ey+4,WHITE);}
    else if(siEnemies[e].type==1){display.fillRect(ex,ey,7,6,WHITE);display.drawPixel(ex+3,ey,BLACK);display.drawPixel(ex+3,ey+5,BLACK);}
    else{display.fillRect(ex,ey+2,9,4,WHITE);display.fillRect(ex+2,ey,5,8,WHITE);display.drawPixel(ex+4,ey+1,BLACK);display.drawPixel(ex+4,ey+6,BLACK);}
  }
  display.setTextSize(1);display.setCursor(90,0);display.print("S:");display.println(siScore);
  display.display();
}

// ============================================================
// BREAKOUT
// ============================================================

#define BO_PADDLE_W   20
#define BO_PADDLE_Y   58
#define BO_BALL_SIZE   2
#define BO_BRICK_COLS  8
#define BO_BRICK_ROWS  4
#define BO_BRICK_W    14
#define BO_BRICK_H     4
#define BO_BRICK_GAP   2
#define BO_BRICK_OFF_X 4
#define BO_BRICK_OFF_Y 5

bool boBricks[BO_BRICK_ROWS][BO_BRICK_COLS];
int boPaddleX,boScore,boLives;
float boBallX,boBallY,boBallVX,boBallVY;
bool boGameOver,boWin;
unsigned long boLastUpdate=0;
#define BO_UPDATE_MS 20

void initBreakout(){
  boPaddleX=54;boBallX=64;boBallY=45;boBallVX=1.0;boBallVY=-1.5;
  boScore=0;boLives=3;boGameOver=false;boWin=false;boLastUpdate=millis();
  for(int r=0;r<BO_BRICK_ROWS;r++)for(int c=0;c<BO_BRICK_COLS;c++)boBricks[r][c]=true;
}
void updateBreakout(){
  if(boGameOver||boWin)return;
  unsigned long now=millis();
  if(now-boLastUpdate<BO_UPDATE_MS)return;
  boLastUpdate=now;
  boBallX+=boBallVX;boBallY+=boBallVY;
  if(boBallX<=1){boBallX=1;boBallVX=-boBallVX;}
  if(boBallX>=127-BO_BALL_SIZE){boBallX=127-BO_BALL_SIZE;boBallVX=-boBallVX;}
  if(boBallY<=1){boBallY=1;boBallVY=-boBallVY;}
  if(boBallY>=BO_PADDLE_Y-BO_BALL_SIZE&&boBallX>=boPaddleX-BO_BALL_SIZE&&boBallX<=boPaddleX+BO_PADDLE_W){
    boBallVY=-abs(boBallVY);
    boBallVX=((boBallX-(boPaddleX+BO_PADDLE_W/2))/(BO_PADDLE_W/2))*2.5;
    eatSound();
  }
  if(boBallY>64){
    boLives--;
    if(boLives<=0){boGameOver=true;gameOverSound();return;}
    boBallX=boPaddleX+BO_PADDLE_W/2;boBallY=45;boBallVX=1.0;boBallVY=-1.5;
  }
  int bx=(int)boBallX,by=(int)boBallY;
  bool hit=false;
  for(int r=0;r<BO_BRICK_ROWS;r++){
    for(int c=0;c<BO_BRICK_COLS;c++){
      if(!boBricks[r][c])continue;
      int rx=BO_BRICK_OFF_X+c*(BO_BRICK_W+BO_BRICK_GAP);
      int ry=BO_BRICK_OFF_Y+r*(BO_BRICK_H+BO_BRICK_GAP);
      if(!hit&&bx+BO_BALL_SIZE>=rx&&bx<=rx+BO_BRICK_W&&by+BO_BALL_SIZE>=ry&&by<=ry+BO_BRICK_H){
        boBricks[r][c]=false;hit=true;boScore+=(BO_BRICK_ROWS-r)*10;boBallVY=-boBallVY;eatSound();
      }
    }
  }
  // count remaining bricks separately
  int left=0;
  for(int r=0;r<BO_BRICK_ROWS;r++)
    for(int c=0;c<BO_BRICK_COLS;c++)
      if(boBricks[r][c])left++;
  if(left==0)boWin=true;
}
void drawBreakout(){
  clearScreen();
  if(boGameOver||boWin){
    display.setTextSize(2);display.setCursor(boWin?20:10,12);display.println(boWin?"YOU WIN!":"GAME OVER");
    display.setTextSize(1);display.setCursor(25,35);display.print("Score: ");display.println(boScore);
    display.setCursor(10,48);display.println("#=restart");display.setCursor(10,57);display.println("*=menu");
    display.display();return;
  }
  for(int r=0;r<BO_BRICK_ROWS;r++)for(int c=0;c<BO_BRICK_COLS;c++){
    if(!boBricks[r][c])continue;
    int rx=BO_BRICK_OFF_X+c*(BO_BRICK_W+BO_BRICK_GAP);
    int ry=BO_BRICK_OFF_Y+r*(BO_BRICK_H+BO_BRICK_GAP);
    display.fillRect(rx,ry,BO_BRICK_W,BO_BRICK_H,WHITE);
    display.drawRect(rx,ry,BO_BRICK_W,BO_BRICK_H,BLACK);
  }
  display.fillRect(boPaddleX,BO_PADDLE_Y,BO_PADDLE_W,3,WHITE);
  display.fillRect((int)boBallX,(int)boBallY,BO_BALL_SIZE,BO_BALL_SIZE,WHITE);
  display.setTextSize(1);
  display.setCursor(0,57);display.print("L:");display.print(boLives);
  display.display();
}

// ============================================================
// SETUP
// ============================================================

void setup() {
  pinMode(BUZZER,OUTPUT); pinMode(LED,OUTPUT);
  pinMode(SOIL,INPUT_ANALOG); pinMode(LDR,INPUT_ANALOG);
  digitalWrite(BUZZER,LOW); digitalWrite(LED,HIGH);
  dht.begin();
  keypad.setDebounceTime(50); keypad.setHoldTime(200);
  display.begin(SSD1306_SWITCHCAPVCC,0x3C);
  display.clearDisplay(); display.setTextSize(1); display.setTextColor(WHITE);
  randomSeed(analogRead(SOIL));
  bootScreen();
  screen = HOME_SCREEN;
  lastActivity = millis();
}

// ============================================================
// LOOP
// ============================================================

void loop() {
  char key=keypad.getKey();
  readSensors();

  // track activity
  if (key) lastActivity = millis();

  // inactivity timeout — return to home from menu screens only
  if (screen == MAIN_MENU || screen == MONITOR_MENU || screen == GAME_MENU || screen == SETTINGS_MENU) {
    if (millis() - lastActivity > INACTIVITY_MS) {
      screen = HOME_SCREEN;
    }
  }

  // home screen
  if (screen == HOME_SCREEN) {
    if (key == '5') { screen = MAIN_MENU; lastActivity = millis(); }
    drawPlantHome();
    delay(16);
    return;
  }

  if(screen==SNAKE_GAME){
    if(key){
      if(key=='2'&&dirY!=1){dirX=0;dirY=-1;}
      else if(key=='8'&&dirY!=-1){dirX=0;dirY=1;}
      else if(key=='4'&&dirX!=1){dirX=-1;dirY=0;}
      else if(key=='6'&&dirX!=-1){dirX=1;dirY=0;}
      else if(key=='#'&&snakeGameOver)initSnake();
      else if(key=='*')screen=GAME_MENU;
    }
    if(!snakeGameOver&&millis()-lastSnakeMove>SNAKE_SPEED){updateSnake();lastSnakeMove=millis();}
    drawSnake();delay(30);return;
  }

  if(screen==TETRIS_GAME){
    if(key){
      if(key=='2')tRotate();
      else if(key=='8'){while(!tCollide(tX,tY+1,tPiece,tRot))tY++;tPlace();tClearLines();tNewPiece();}
      else if(key=='#'&&tGameOver)initTetris();
      else if(key=='*')screen=GAME_MENU;
    }
    if(keypad.getState()==HOLD||keypad.getState()==PRESSED){
      char k=keypad.key[0].kchar;
      if(k=='4'){if(!tCollide(tX-1,tY,tPiece,tRot))tX--;}
      else if(k=='6'){if(!tCollide(tX+1,tY,tPiece,tRot))tX++;}
    }
    if(!tGameOver){int dm=max(100,700-(tLevel-1)*60);if(millis()-tLastDrop>=(unsigned long)dm){tDrop();tLastDrop=millis();}}
    drawTetris();delay(80);return;
  }

  if(screen==DINO_GAME){
    if(key){
      if(key=='2'&&dinoOnGround){dinoVY=JUMP_FORCE;dinoOnGround=false;}
      else if(key=='#'&&dinoGameOver)initDino();
      else if(key=='*')screen=GAME_MENU;
    }
    if(millis()-dinoLastFrame>=DINO_FRAME_MS){updateDino();dinoLastFrame=millis();}
    drawDino();delay(20);return;
  }

  if(screen==SPACE_INVADERS){
    if(key){
      if(key=='#'&&siGameOver)initSpaceInvaders();
      else if(key=='*')screen=GAME_MENU;
    }
    if(keypad.getState()==HOLD||keypad.getState()==PRESSED){
      char k=keypad.key[0].kchar;
      if(k=='4'){siPlayerX-=5;if(siPlayerX<0)siPlayerX=0;}
      else if(k=='6'){siPlayerX+=5;if(siPlayerX>120)siPlayerX=120;}
    }
    updateSpaceInvaders();drawSpaceInvaders();delay(20);return;
  }

  if(screen==BREAKOUT_GAME){
    if(key=='#'&&(boGameOver||boWin))initBreakout();
    else if(key=='*')screen=GAME_MENU;
    if(keypad.getState()==HOLD||keypad.getState()==PRESSED){
      char k=keypad.key[0].kchar;
      if(k=='4'){boPaddleX-=4;if(boPaddleX<0)boPaddleX=0;}
      else if(k=='6'){boPaddleX+=4;if(boPaddleX>108)boPaddleX=108;}
    }
    updateBreakout();drawBreakout();delay(10);return;
  }

  if (screen == ABOUT_SCREEN) {
    if (key == '*') screen = MAIN_MENU;
    drawAbout();
    delay(16);
    return;
  }

  if(key){
    switch(screen){
      case MAIN_MENU:       handleMainMenu(key);     break;
      case MONITOR_MENU:    handleMonitorMenu(key);  break;
      case GAME_MENU:       handleGameMenu(key);     break;
      case SETTINGS_MENU:   handleSettingsMenu(key); break;
      case TEMP_SCREEN:       if(key=='*')screen=MONITOR_MENU; break;
      case HUMIDITY_SCREEN:   if(key=='*')screen=MONITOR_MENU; break;
      case SOIL_SCREEN:       if(key=='*')screen=MONITOR_MENU; break;
      case LIGHT_SCREEN:      if(key=='*')screen=MONITOR_MENU; break;
      case ALL_SENSOR_SCREEN: if(key=='*')screen=MONITOR_MENU; break;
      case SYSTEM_SCREEN:     if(key=='*')screen=MAIN_MENU;    break;
      default: break;
    }
  }

  switch(screen){
    case MAIN_MENU:         drawMainMenu();                   break;
    case MONITOR_MENU:      drawMonitorMenu();                break;
    case GAME_MENU:         drawGameMenu();                   break;
    case SETTINGS_MENU:     drawSettingsMenu();               break;
    case TEMP_SCREEN:       drawTemp(temp);                   break;
    case HUMIDITY_SCREEN:   drawHumidity(hum);                break;
    case SOIL_SCREEN:       drawSoil(moisture);               break;
    case LIGHT_SCREEN:      drawLight(light);                 break;
    case ALL_SENSOR_SCREEN: drawAll(temp,hum,moisture,light); break;
    case SYSTEM_SCREEN:     drawSystem();                     break;
    default: break;
  }
  delay(120);
}