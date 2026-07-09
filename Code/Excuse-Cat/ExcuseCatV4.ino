#include "driver.h"
#include "TFT_eSPI.h"
#include <esp_system.h>
#include <pgmspace.h>

#include "sleep_cat_bitmap.h"
#include "play_ball_cat_bitmap.h"
#include "eat_cat_bitmap.h"
#include "hiss_cat_bitmap.h"
#include "cheems_bitmap.h"

#ifndef EPAPER_ENABLE
#error "EPAPER_ENABLE is not defined. Please check driver.h."
#endif

EPaper epaper;

// =====================================================
// Page state
// =====================================================
// true  = startup page
// false = generator page
bool showingStartPage = true;

// =====================================================
// Button + inactivity return
// =====================================================
const int BUTTON_GENERATE_PIN = 3;   // KEY2
bool generateButtonWasDown = false;

unsigned long lastInteractionTime = 0;
const unsigned long RETURN_HOME_DELAY = 60000;   // 60s
// =====================================================
// Impatient cat / easter egg logic
// =====================================================
// Presses 1-4: normal excuses.
// Press 5 and above within 10s: impatient cat UI.
// If no button press for more than 10s, the press streak resets.
// If no operation for 60s, return home and clear impatient state.

bool showingSpecialPage = false;

const int IMPATIENT_TRIGGER_COUNT = 5;
const int SPECIAL_TYPE_COUNT = 5;
const unsigned long PRESS_STREAK_TIMEOUT = 10000; // 10s

int impatientPressCount = 0;
unsigned long lastGeneratePressTime = 0;

// Bitmap type mapping:
// 0 = sleep
// 1 = play ball
// 2 = eat
// 3 = hiss
// 4 = cheems
int currentSpecialType = 0;

// From the 5th press onward, rotate in the order you listed:
// 5th=hiss, 6th=cheems, 7th=eat, 8th=play ball, 9th=sleep, 10th=hiss...
const int specialSequence[SPECIAL_TYPE_COUNT] = {3, 4, 2, 1, 0};


// =====================================================
// Excuses
// =====================================================
const int EXCUSE_COUNT = 60;

const char* excuses[EXCUSE_COUNT] = {
  // Weather
  "It's raining.",
  "The weather said no.",
  "Humidity changed my personality.",
  "The sky is in debug mode.",
  "The clouds are holding a meeting.",
  "The forecast failed QA.",
  "Air pressure lowered my ambition.",
  "The sun clocked out early.",
  "Outside is in maintenance mode.",
  "Room temperature changed the plot.",
  "The atmosphere felt confrontational.",
  "The forecast looked emotionally unsafe.",
  "The drizzle ruined my workflow.",
  "The sky deployed a soft shutdown.",
  "The breeze deleted my motivation.",
  "Today's climate lacked compatibility.",
  "The weather sent mixed signals.",
  "The sunlight forgot to render.",
  "The season was not on my side.",
  "Nature filed an objection.",

  // Mood
  "My brain is buffering.",
  "I am in low-power mode.",
  "The vibes are not passing QA.",
  "Motivation left without a ticket.",
  "My emotional firmware needs an update.",
  "Today is not compatible with progress.",
  "I opened the file and needed a snack.",
  "My focus is still in transit.",
  "The task looked at me with expectations.",
  "Productivity requires a restart.",
  "My willpower is in sleep mode.",
  "The moodboard said maybe tomorrow.",
  "My energy budget was exceeded.",
  "My brain launched background processes.",
  "The vibes entered safe mode.",
  "I needed a dramatic pause.",
  "My concentration missed the bus.",
  "Today's motivation failed to boot.",
  "My emotional battery is uncalibrated.",
  "The task triggered unnecessary feelings.",

  // Maker
  "The code only works when ignored.",
  "86F and humid. I melted.",
  "Need more power-washing videos.",
  "Body says go. Heart says no.",
  "Got distracted by another project.",
  "Noise would anger downstairs.",
  "Only 8-10am build window.",
  "Magic 8-ball said ask later.",
  "More analysis required.",
  "More tools required.",
  "No time, only vibes.",
  "Need supplies first.",
  "Needs more aimless pondering.",
  "My desk has room for one more WIP.",
  "The world is not ready yet.",
  "Projects move like the wind.",
  "Maybe I should socialize more.",
  "Need to clean my desk first.",
  "Squirrel!!!",
  "Finished yesterday."
};

int currentExcuse = 0;

// =====================================================
// Cat moods
// =====================================================
const int CAT_MOOD_COUNT = 10;
const int CAT_LINES = 6;
int currentCatMood = 0;

// =====================================================
// ASCII cat art
// =====================================================
const char* catArt[CAT_MOOD_COUNT][CAT_LINES] = {
  // 0: dazed
  {
    "  /\\___/\\\\",
    " /  o o  \\\\",
    "( ==  ^  ==)",
    " )         (",
    "(   ._.     )",
    " (__(__)___)"
  },

  // 1: sleepy
  {
    "  /\\___/\\\\",
    " /  - -  \\\\",
    "( ==  ^  ==) z",
    " )         (",
    "(   u_u     )",
    " (__(__)___)"
  },

  // 2: angry
  {
    "  /\\___/\\\\",
    " /  > <  \\\\",
    "( ==  ^  ==)",
    " )   ---   (",
    "(   >:(     )",
    " (__(__)___)"
  },

  // 3: eating
  {
    "  /\\___/\\\\",
    " /  ^ ^  \\\\",
    "( ==  ^  ==) o",
    " )   \\_/   (",
    "(   nom     )",
    " (__(__)___)"
  },

  // 4: shocked
  {
    "  /\\___/\\\\",
    " /  O O  \\\\",
    "( ==  ^  ==)",
    " )   ___   (",
    "(   !!!     )",
    " (__(__)___)"
  },

  // 5: smug
  {
    "  /\\___/\\\\",
    " /  ^ -  \\\\",
    "( ==  ^  ==)",
    " )   ~~~   (",
    "(   hehe    )",
    " (__(__)___)"
  },

  // 6: crying
  {
    "  /\\___/\\\\",
    " /  ; ;  \\\\",
    "( ==  ^  ==)",
    " )   ___   (",
    "(   T_T     )",
    " (__(__)___)"
  },

  // 7: sitting / loaf
  {
    "  /\\___/\\\\",
    " /  - -  \\\\",
    "( ==  ^  ==)",
    " )  _____  (",
    "(  (_____)  )",
    " (_________)"
  },

  // 8: lying sleepy
  {
    "   /\\___/\\\\",
    "  /  - -  \\\\",
    " ( == ^ == ) z",
    " /         \\",
    "(   zzz     )",
    " (_________)"
  },

  // 9: flat sleeping
  {
    "  /\\___/\\\\",
    " /  - -  \\\\",
    "( ==  ^  ==)",
    " )  zzz    (",
    "(___________)",
    "  uu     uu "
  }
};

// =====================================================
// Function declarations
// =====================================================
void handleGenerateButton();
void resetImpatientState();
int getSpecialTypeForPressCount(int pressCount);
void showNormalExcuse();
void showSpecialPage();
void chooseNextExcuse();
void updateCatMoodForExcuse();

void renderScreen();

void drawStartPage(int w, int h);
void drawBrandHeader(int w);
void drawStartInstructionBox(int w, int h);
void drawSideBinaryColumns(int w, int h);
void drawStartupSparkles(int w, int h);
void drawStartupCatHead(int centerX, int topY, int s);

void drawGeneratorPage(int w, int h);
void drawOuterFrame(int w, int h);
void drawLeftTextArea(int w, int h);
void drawRightBackground(int w, int h);
void drawAsciiCatArea(int w, int h);
void drawAsciiCat(int x, int y, int mood);

void drawSpecialPage(int w, int h);
void drawSpecialHeader(int w);
void drawSpecialMessageBox(int w, int h);
void drawSpecialBitmapCentered(int w, int h, int imageTopY, int messageBoxY);

void drawBoldString(const char* text, int x, int y, int textSize);
void drawCenteredBoldText(const char* text, int centerX, int y, int textSize);
void drawCenteredText(const char* text, int centerX, int y, int textSize);
void drawWrappedText(const char* rawText, int x, int y, int maxCharsPerLine, int lineHeight, int maxLines);
void drawDottedBackground(int x, int y, int w, int h, uint16_t color);
void drawStar(int x, int y, uint16_t color);
void drawDashedRect(int x, int y, int w, int h, uint16_t color);
void drawPawPrint(int x, int y, int s, uint16_t color);
void drawPixelPawIcon(int x, int y, int s, uint16_t color);

const char* getSpecialTitle();
const char* getSpecialMessageLine1();
const char* getSpecialMessageLine2();

void drawMonochromeBitmap(int x, int y, const uint8_t* bitmap, int bmpW, int bmpH, uint16_t color);
void drawMonochromeBitmapFit(int centerX, int topY, int maxW, int maxH, const uint8_t* bitmap, int bmpW, int bmpH, uint16_t color);

// =====================================================
// Setup
// =====================================================
void setup()
{
  Serial.begin(115200);

  pinMode(BUTTON_GENERATE_PIN, INPUT_PULLUP);

  epaper.begin();
  randomSeed((uint32_t)esp_random());

  currentExcuse = random(EXCUSE_COUNT);
  updateCatMoodForExcuse();

  showingStartPage = true;
  lastInteractionTime = millis();

  renderScreen();

  Serial.println("Excuse Cat ready.");
}

// =====================================================
// Loop
// =====================================================
void loop()
{
  handleGenerateButton();

  // 60s inactivity: return to startup page and clear impatient state.
  if (!showingStartPage) {
    if (millis() - lastInteractionTime > RETURN_HOME_DELAY) {
      showingStartPage = true;
      resetImpatientState();

      lastInteractionTime = millis();
      renderScreen();

      Serial.println("Returned to startup page. Impatient state cleared.");
    }
  }

  delay(20);
}

// =====================================================
// Button logic
// =====================================================
void handleGenerateButton()
{
  bool buttonDown = digitalRead(BUTTON_GENERATE_PIN) == LOW;

  if (buttonDown && !generateButtonWasDown) {
    delay(60); // debounce

    if (digitalRead(BUTTON_GENERATE_PIN) == LOW) {
      unsigned long now = millis();

      // More than 10s since last press = start a new streak.
      if (lastGeneratePressTime != 0 && now - lastGeneratePressTime > PRESS_STREAK_TIMEOUT) {
        resetImpatientState();
        Serial.println("Press streak timeout. Count reset.");
      }

      lastGeneratePressTime = now;
      lastInteractionTime = now;

      if (showingStartPage) {
        showingStartPage = false;
      }

      impatientPressCount++;

      Serial.print("Current press streak: ");
      Serial.println(impatientPressCount);

      if (impatientPressCount < IMPATIENT_TRIGGER_COUNT) {
        // Press 1-4: normal excuse generation.
        showNormalExcuse();
      } else {
        // Press 5 and above: impatient cat UI.
        showSpecialPage();
      }
    }
  }

  generateButtonWasDown = buttonDown;
}

void resetImpatientState()
{
  showingSpecialPage = false;
  impatientPressCount = 0;
  lastGeneratePressTime = 0;
  currentSpecialType = 0;
}

int getSpecialTypeForPressCount(int pressCount)
{
  int index = (pressCount - IMPATIENT_TRIGGER_COUNT) % SPECIAL_TYPE_COUNT;
  if (index < 0) index = 0;

  return specialSequence[index];
}

void showNormalExcuse()
{
  showingSpecialPage = false;

  chooseNextExcuse();
  updateCatMoodForExcuse();

  renderScreen();

  Serial.print("Generated excuse: ");
  Serial.println(excuses[currentExcuse]);

  Serial.print("Cat mood: ");
  Serial.println(currentCatMood);
}

void showSpecialPage()
{
  showingSpecialPage = true;
  currentSpecialType = getSpecialTypeForPressCount(impatientPressCount);

  renderScreen();

  Serial.print("Impatient cat page triggered. Type: ");
  Serial.println(currentSpecialType);
}

void chooseNextExcuse()
{
  int newExcuse = currentExcuse;

  while (newExcuse == currentExcuse) {
    newExcuse = random(EXCUSE_COUNT);
  }

  currentExcuse = newExcuse;
}

void updateCatMoodForExcuse()
{
  if (currentExcuse >= 0 && currentExcuse < 20) {
    int weatherMoods[] = {0, 1, 4};
    currentCatMood = weatherMoods[random(0, 3)];
  }
  else if (currentExcuse >= 20 && currentExcuse < 40) {
    int moodMoods[] = {1, 5, 6, 7, 8, 9};
    currentCatMood = moodMoods[random(0, 6)];
  }
  else {
    int makerMoods[] = {2, 3, 4, 6, 7};
    currentCatMood = makerMoods[random(0, 5)];
  }
}

// =====================================================
// Render
// =====================================================
void renderScreen()
{
  epaper.fillScreen(TFT_WHITE);

  int w = epaper.width();
  int h = epaper.height();

  if (showingStartPage) {
    drawStartPage(w, h);
  } else if (showingSpecialPage) {
    drawSpecialPage(w, h);
  } else {
    drawGeneratorPage(w, h);
  }

  epaper.update();
}

// =====================================================
// Start page
// =====================================================
void drawStartPage(int w, int h)
{
  drawBrandHeader(w);
  drawSideBinaryColumns(w, h);
  drawStartupSparkles(w, h);
  drawStartupCatHead(w / 2, 126, 6);
  drawStartInstructionBox(w, h);
}

void drawBrandHeader(int w)
{
  epaper.fillRect(36, 36, w - 72, 84, TFT_BLACK);

  epaper.setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredBoldText("EXCUSE CAT", w / 2, 48, 4);
  drawCenteredText("WHY ISN'T IT DONE YET?", w / 2, 92, 2);

  epaper.setTextColor(TFT_BLACK, TFT_WHITE);
}

void drawStartInstructionBox(int w, int h)
{
  int boxW = 650;
  int boxH = 210;
  int boxX = (w - boxW) / 2;
  int boxY = 215;

  epaper.fillRect(boxX, boxY, boxW, boxH, TFT_WHITE);
  epaper.drawRect(boxX, boxY, boxW, boxH, TFT_BLACK);
  epaper.drawRect(boxX + 3, boxY + 3, boxW - 6, boxH - 6, TFT_BLACK);

  epaper.setTextColor(TFT_BLACK, TFT_WHITE);

  drawCenteredBoldText("Press the button to ask the cat", w / 2, boxY + 52, 3);
  drawCenteredBoldText("why your project is still not done.", w / 2, boxY + 102, 3);

  epaper.setTextSize(2);
  epaper.drawString("By Mason", boxX + boxW - 140, boxY + boxH - 42);
}

void drawSideBinaryColumns(int w, int h)
{
  epaper.setTextColor(TFT_BLACK, TFT_WHITE);
  epaper.setTextSize(2);

  int leftX = 42;
  int rightX = w - 42;
  int startY = 132;
  int stepY = 20;

  const char* leftBits[] = {
    "1", "0", "1", "1", "0", "0", "1", "0",
    "1", "1", "0", "1", "0", "1", "0"
  };

  const char* rightBits[] = {
    "0", "1", "0", "0", "1", "1", "0", "1",
    "0", "0", "1", "1", "0", "1", "1"
  };

  for (int i = 0; i < 15; i++) {
    epaper.drawString(leftBits[i], leftX, startY + i * stepY);
    epaper.drawString(rightBits[i], rightX, startY + i * stepY);
  }
}

void drawStartupSparkles(int w, int h)
{
  drawStar(120, 165, TFT_BLACK);
  drawStar(w - 110, 150, TFT_BLACK);
  drawStar(88, 355, TFT_BLACK);
  drawStar(w - 95, 330, TFT_BLACK);
  drawStar(155, 430, TFT_BLACK);
  drawStar(w - 120, 435, TFT_BLACK);

  epaper.drawPixel(105, 250, TFT_BLACK);
  epaper.drawPixel(108, 270, TFT_BLACK);
  epaper.drawPixel(w - 110, 235, TFT_BLACK);
  epaper.drawPixel(w - 95, 255, TFT_BLACK);
}

void drawStartupCatHead(int centerX, int topY, int s)
{
  // wider cat head icon
  int x = centerX - 8 * s;
  int y = topY;

  // -------------------------
  // ears
  // -------------------------
  epaper.fillTriangle(
    x + 2 * s,  y + 4 * s,
    x + 4 * s,  y,
    x + 6 * s,  y + 4 * s,
    TFT_BLACK
  );

  epaper.fillTriangle(
    x + 10 * s, y + 4 * s,
    x + 12 * s, y,
    x + 14 * s, y + 4 * s,
    TFT_BLACK
  );

  // inner ears
  epaper.fillTriangle(
    x + 3 * s,  y + 4 * s,
    x + 4 * s,  y + 1 * s,
    x + 5 * s,  y + 4 * s,
    TFT_WHITE
  );

  epaper.fillTriangle(
    x + 11 * s, y + 4 * s,
    x + 12 * s, y + 1 * s,
    x + 13 * s, y + 4 * s,
    TFT_WHITE
  );

  // -------------------------
  // head (wider)
  // -------------------------
  epaper.fillRect(x + 1 * s, y + 3 * s, 14 * s, 10 * s, TFT_BLACK);
  epaper.fillRect(x + 2 * s, y + 2 * s, 12 * s, 1 * s, TFT_BLACK);
  epaper.fillRect(x + 2 * s, y + 13 * s, 12 * s, 1 * s, TFT_BLACK);

  // -------------------------
  // eyes (farther apart)
  // -------------------------
  epaper.fillRect(x + 4 * s, y + 6 * s, 2 * s, 4 * s, TFT_WHITE);
  epaper.fillRect(x + 9 * s, y + 6 * s, 2 * s, 4 * s, TFT_WHITE);

  // pupils
  epaper.drawPixel(x + 5 * s,  y + 8 * s, TFT_BLACK);
  epaper.drawPixel(x + 10 * s, y + 8 * s, TFT_BLACK);

  // -------------------------
  // nose only
  // -------------------------
  epaper.drawPixel(x + 7 * s, y + 11 * s, TFT_WHITE);
  epaper.drawPixel(x + 8 * s, y + 11 * s, TFT_WHITE);

  // -------------------------
  // whiskers: 3 left, 3 right
  // -------------------------
  // left top
  epaper.drawLine(x + 1 * s, y + 8 * s,  x - 2 * s, y + 7 * s, TFT_BLACK);
  epaper.drawLine(x + 1 * s, y + 8 * s + 1,  x - 2 * s, y + 7 * s + 1, TFT_BLACK);

  // left middle
  epaper.drawLine(x + 1 * s, y + 10 * s, x - 2 * s, y + 10 * s, TFT_BLACK);
  epaper.drawLine(x + 1 * s, y + 10 * s + 1, x - 2 * s, y + 10 * s + 1, TFT_BLACK);

  // left bottom
  epaper.drawLine(x + 1 * s, y + 12 * s, x - 2 * s, y + 13 * s, TFT_BLACK);
  epaper.drawLine(x + 1 * s, y + 12 * s + 1, x - 2 * s, y + 13 * s + 1, TFT_BLACK);

  // right top
  epaper.drawLine(x + 15 * s, y + 8 * s,  x + 18 * s, y + 7 * s, TFT_BLACK);
  epaper.drawLine(x + 15 * s, y + 8 * s + 1,  x + 18 * s, y + 7 * s + 1, TFT_BLACK);

  // right middle
  epaper.drawLine(x + 15 * s, y + 10 * s, x + 18 * s, y + 10 * s, TFT_BLACK);
  epaper.drawLine(x + 15 * s, y + 10 * s + 1, x + 18 * s, y + 10 * s + 1, TFT_BLACK);

  // right bottom
  epaper.drawLine(x + 15 * s, y + 12 * s, x + 18 * s, y + 13 * s, TFT_BLACK);
  epaper.drawLine(x + 15 * s, y + 12 * s + 1, x + 18 * s, y + 13 * s + 1, TFT_BLACK);
}

// =====================================================
// Generator page
// =====================================================
void drawGeneratorPage(int w, int h)
{
  drawOuterFrame(w, h);
  drawBrandHeader(w);
  drawLeftTextArea(w, h);
  drawRightBackground(w, h);
  drawAsciiCatArea(w, h);
}

// =====================================================
// Layout drawing
// =====================================================
void drawOuterFrame(int w, int h)
{
  epaper.drawRect(14, 14, w - 28, h - 28, TFT_BLACK);
  epaper.drawRect(20, 20, w - 40, h - 40, TFT_BLACK);

  int len = 32;

  epaper.drawLine(32, 32, 32 + len, 32, TFT_BLACK);
  epaper.drawLine(32, 32, 32, 32 + len, TFT_BLACK);

  epaper.drawLine(w - 32, 32, w - 32 - len, 32, TFT_BLACK);
  epaper.drawLine(w - 32, 32, w - 32, 32 + len, TFT_BLACK);

  epaper.drawLine(32, h - 32, 32 + len, h - 32, TFT_BLACK);
  epaper.drawLine(32, h - 32, 32, h - 32 - len, TFT_BLACK);

  epaper.drawLine(w - 32, h - 32, w - 32 - len, h - 32, TFT_BLACK);
  epaper.drawLine(w - 32, h - 32, w - 32, h - 32 - len, TFT_BLACK);
}

void drawLeftTextArea(int w, int h)
{
  int leftX = 52;
  epaper.setTextColor(TFT_BLACK, TFT_WHITE);

  drawBoldString("The Cat Says:", leftX, 145, 4);

  int boxX = 44;
  int boxY = 205;
  int boxW = 470;
  int boxH = 225;

  drawDashedRect(boxX, boxY, boxW, boxH, TFT_BLACK);

  epaper.setTextSize(4);
  drawWrappedText(
    excuses[currentExcuse],
    leftX + 12,
    250,
    17,
    50,
    3
  );

  // New pixel paw icon at bottom-right of the excuse box.
  drawPixelPawIcon(boxX + boxW - 50, boxY + boxH - 50, 1, TFT_BLACK);
}

void drawRightBackground(int w, int h)
{
  int bgX = 530;
  int bgY = 130;
  int bgW = w - bgX - 45;
  int bgH = h - bgY - 70;

  drawDottedBackground(bgX, bgY, bgW, bgH, TFT_BLACK);
}

void drawAsciiCatArea(int w, int h)
{
  int x = 525;
  int y = 155;

  epaper.setTextColor(TFT_BLACK, TFT_WHITE);
  epaper.setTextSize(3);

  drawAsciiCat(x, y, currentCatMood);
}

// =====================================================
// ASCII cat rendering
// =====================================================
void drawAsciiCat(int x, int y, int mood)
{
  int lineHeight = 40;

  for (int i = 0; i < CAT_LINES; i++) {
    int lineY = y + i * lineHeight;
    const char* line = catArt[mood][i];

    // base bold
    epaper.drawString(line, x,     lineY);
    epaper.drawString(line, x + 1, lineY);
    epaper.drawString(line, x,     lineY + 1);
    epaper.drawString(line, x + 1, lineY + 1);

    // eyes row extra bold
    if (i == 1) {
      epaper.drawString(line, x + 2, lineY);
      epaper.drawString(line, x + 2, lineY + 1);
      epaper.drawString(line, x,     lineY + 2);
      epaper.drawString(line, x + 1, lineY + 2);
      epaper.drawString(line, x + 2, lineY + 2);

      epaper.drawString(line, x + 3, lineY);
      epaper.drawString(line, x + 3, lineY + 1);
      epaper.drawString(line, x + 3, lineY + 2);

      epaper.drawString(line, x + 4, lineY);
      epaper.drawString(line, x + 4, lineY + 1);
      epaper.drawString(line, x + 4, lineY + 2);
    }

    // middle row slight compensation
    if (i == 2) {
      epaper.drawString(line, x + 2, lineY);
      epaper.drawString(line, x + 2, lineY + 1);
    }
  }
}
void drawSpecialPage(int w, int h)
{
  drawOuterFrame(w, h);

  // Top black warning banner
  drawSpecialHeader(w);

  // Large centered bitmap
  int imageTopY = 118;
  int messageBoxY = h - 116;
  drawSpecialBitmapCentered(w, h, imageTopY, messageBoxY);

  // Bottom dashed message box
  drawSpecialMessageBox(w, h);
}

void drawSpecialHeader(int w)
{
  int bannerX = 36;
  int bannerY = 36;
  int bannerW = w - 72;
  int bannerH = 64;

  epaper.fillRect(bannerX, bannerY, bannerW, bannerH, TFT_BLACK);

  epaper.setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredBoldText(getSpecialTitle(), w / 2, bannerY + 18, 3);

  epaper.setTextColor(TFT_BLACK, TFT_WHITE);
}

void drawSpecialMessageBox(int w, int h)
{
  int boxW = w - 140;
  int boxH = 82;
  int boxX = (w - boxW) / 2;
  int boxY = h - 116;

  drawDashedRect(boxX, boxY, boxW, boxH, TFT_BLACK);

  epaper.setTextColor(TFT_BLACK, TFT_WHITE);
  drawCenteredBoldText(getSpecialMessageLine1(), w / 2, boxY + 18, 2);
  drawCenteredBoldText(getSpecialMessageLine2(), w / 2, boxY + 46, 2);
}

// =====================================================
// Helpers
// =====================================================
void drawBoldString(const char* text, int x, int y, int textSize)
{
  epaper.setTextSize(textSize);

  epaper.drawString(text, x, y);
  epaper.drawString(text, x + 1, y);
  epaper.drawString(text, x, y + 1);
  epaper.drawString(text, x + 1, y + 1);
}

void drawCenteredBoldText(const char* text, int centerX, int y, int textSize)
{
  int textWidth = String(text).length() * 6 * textSize;
  int x = centerX - textWidth / 2;
  drawBoldString(text, x, y, textSize);
}

void drawCenteredText(const char* text, int centerX, int y, int textSize)
{
  int textWidth = String(text).length() * 6 * textSize;
  int x = centerX - textWidth / 2;

  epaper.setTextSize(textSize);
  epaper.drawString(text, x, y);
}

void drawWrappedText(const char* rawText, int x, int y, int maxCharsPerLine, int lineHeight, int maxLines)
{
  String text = String(rawText);
  String line = "";

  int currentY = y;
  int lineCount = 0;
  int start = 0;

  while (start < text.length() && lineCount < maxLines) {
    int spaceIndex = text.indexOf(' ', start);
    if (spaceIndex == -1) {
      spaceIndex = text.length();
    }

    String word = text.substring(start, spaceIndex);

    String testLine = line;
    if (testLine.length() > 0) {
      testLine += " ";
    }
    testLine += word;

    if (testLine.length() > maxCharsPerLine && line.length() > 0) {
      epaper.drawString(line, x, currentY);
      currentY += lineHeight;
      lineCount++;
      line = word;
    } else {
      line = testLine;
    }

    start = spaceIndex + 1;
  }

  if (line.length() > 0 && lineCount < maxLines) {
    epaper.drawString(line, x, currentY);
  }
}

void drawDottedBackground(int x, int y, int w, int h, uint16_t color)
{
  for (int yy = y; yy < y + h; yy += 8) {
    int offset = ((yy / 8) % 2) * 4;
    for (int xx = x + offset; xx < x + w; xx += 8) {
      epaper.drawPixel(xx, yy, color);
    }
  }

  drawStar(x + 24, y + 22, color);
  drawStar(x + 168, y + 44, color);
  drawStar(x + 84, y + 182, color);
  drawStar(x + 182, y + 210, color);
}

void drawStar(int x, int y, uint16_t color)
{
  epaper.drawPixel(x, y, color);
  epaper.drawPixel(x - 1, y, color);
  epaper.drawPixel(x + 1, y, color);
  epaper.drawPixel(x, y - 1, color);
  epaper.drawPixel(x, y + 1, color);
}

void drawDashedRect(int x, int y, int w, int h, uint16_t color)
{
  int dash = 8;
  int gap = 6;

  // top and bottom
  for (int xx = x; xx < x + w; xx += dash + gap) {
    int x2 = xx + dash;
    if (x2 > x + w) x2 = x + w;

    epaper.drawLine(xx, y, x2, y, color);
    epaper.drawLine(xx, y + h, x2, y + h, color);
  }

  // left and right
  for (int yy = y; yy < y + h; yy += dash + gap) {
    int y2 = yy + dash;
    if (y2 > y + h) y2 = y + h;

    epaper.drawLine(x, yy, x, y2, color);
    epaper.drawLine(x + w, yy, x + w, y2, color);
  }
}

void drawPixelPawIcon(int x, int y, int s, uint16_t color)
{
  // Smaller and tighter cat paw print
  // x, y = top-left anchor
  // s = scale factor

  // -------------------------
  // Toe pads
  // Closer to the main pad than previous version
  // -------------------------
  epaper.fillCircle(x +  5 * s, y +  9 * s, 3 * s, color);  // left toe
  epaper.fillCircle(x + 11 * s, y +  5 * s, 3 * s, color);  // upper-left toe
  epaper.fillCircle(x + 18 * s, y +  5 * s, 3 * s, color);  // upper-right toe
  epaper.fillCircle(x + 24 * s, y +  9 * s, 3 * s, color);  // right toe

  // Make toe pads slightly oval, but not too chunky
  epaper.fillRect(x +  4 * s, y +  9 * s, 2 * s, 2 * s, color);
  epaper.fillRect(x + 10 * s, y +  5 * s, 2 * s, 2 * s, color);
  epaper.fillRect(x + 17 * s, y +  5 * s, 2 * s, 2 * s, color);
  epaper.fillRect(x + 23 * s, y +  9 * s, 2 * s, 2 * s, color);

  // -------------------------
  // Main paw pad
  // Smaller palm, closer to toe pads
  // -------------------------
  epaper.fillCircle(x + 14 * s, y + 21 * s, 6 * s, color);  // center upper pad
  epaper.fillCircle(x +  9 * s, y + 24 * s, 5 * s, color);  // left lower lobe
  epaper.fillCircle(x + 20 * s, y + 24 * s, 5 * s, color);  // right lower lobe

  // Connect the pad into one smooth shape
  epaper.fillRect(x + 9 * s, y + 20 * s, 11 * s, 8 * s, color);

  // Slight bottom fill, keeps the pad rounded but not oversized
  epaper.fillRect(x + 11 * s, y + 27 * s, 7 * s, 2 * s, color);
}
void drawPawPrint(int x, int y, int s, uint16_t color)
{
  // big pad
  epaper.fillCircle(x, y, s + 2, color);

  // 3 toe pads in a fan shape
  epaper.fillCircle(x - (s + 4), y - (s + 7), s - 1, color);   // left toe
  epaper.fillCircle(x,           y - (s + 10), s - 1, color);  // top toe
  epaper.fillCircle(x + (s + 4), y - (s + 7), s - 1, color);   // right toe
}
const char* getSpecialTitle()
{
  switch (currentSpecialType) {
    case 0: return "DO NOT DISTURB: NAP MODE";
    case 1: return "BALL MODE ENGAGED";
    case 2: return "SNACK BREAK ACTIVATED";
    case 3: return "WARNING: CAT IS HISSING!";
    default: return "CHEEMS HAS ENTERED THE CHAT";
  }
}

const char* getSpecialMessageLine1()
{
  switch (currentSpecialType) {
    case 0: return "The excuse generator is asleep.";
    case 1: return "The cat found a better priority:";
    case 2: return "No more excuses until";
    case 3: return "Press slower, human.";
    default: return "maybe rest first...";
  }
}

const char* getSpecialMessageLine2()
{
  switch (currentSpecialType) {
    case 0: return "Please try again after the nap.";
    case 1: return "chaos, speed, and ball.";
    case 2: return "the cat has been properly fed.";
    case 3: return "You're making too many excuses!";
    default: return "the project can wait a little.";
  }
}

void drawMonochromeBitmap(int x, int y, const uint8_t* bitmap, int bmpW, int bmpH, uint16_t color)
{
  int bytesPerRow = (bmpW + 7) / 8;

  for (int yy = 0; yy < bmpH; yy++) {
    for (int xx = 0; xx < bmpW; xx++) {
      int byteIndex = yy * bytesPerRow + (xx / 8);

      uint8_t b = pgm_read_byte(&bitmap[byteIndex]);
      uint8_t mask = 0x80 >> (xx % 8);

      if (b & mask) {
        epaper.drawPixel(x + xx, y + yy, color);
      }
    }
  }
}
void drawSpecialBitmapCentered(int w, int h, int imageTopY, int messageBoxY)
{
  const uint8_t* bitmap = sleep_cat_bitmap;
  int bmpW = sleep_cat_bitmap_width;
  int bmpH = sleep_cat_bitmap_height;

  switch (currentSpecialType) {
    case 0:
      bitmap = sleep_cat_bitmap;
      bmpW = sleep_cat_bitmap_width;
      bmpH = sleep_cat_bitmap_height;
      break;

    case 1:
      bitmap = play_ball_cat_bitmap;
      bmpW = play_ball_cat_bitmap_width;
      bmpH = play_ball_cat_bitmap_height;
      break;

    case 2:
      bitmap = eat_cat_bitmap;
      bmpW = eat_cat_bitmap_width;
      bmpH = eat_cat_bitmap_height;
      break;

    case 3:
      bitmap = hiss_cat_bitmap;
      bmpW = hiss_cat_bitmap_width;
      bmpH = hiss_cat_bitmap_height;
      break;

    default:
      bitmap = cheems_bitmap;
      bmpW = cheems_bitmap_width;
      bmpH = cheems_bitmap_height;
      break;
  }

  int maxImgW = w - 170;
  int maxImgH = messageBoxY - imageTopY - 12;

  drawMonochromeBitmapFit(
    w / 2,
    imageTopY,
    maxImgW,
    maxImgH,
    bitmap,
    bmpW,
    bmpH,
    TFT_BLACK
  );
}
void drawMonochromeBitmapFit(
  int centerX,
  int topY,
  int maxW,
  int maxH,
  const uint8_t* bitmap,
  int bmpW,
  int bmpH,
  uint16_t color
)
{
  if (maxW <= 0 || maxH <= 0 || bmpW <= 0 || bmpH <= 0) {
    return;
  }

  float scaleW = (float)maxW / (float)bmpW;
  float scaleH = (float)maxH / (float)bmpH;
  float scale = scaleW < scaleH ? scaleW : scaleH;

  // Avoid becoming too huge and too slow to draw
  if (scale > 1.45) {
    scale = 1.45;
  }

  int outW = (int)(bmpW * scale);
  int outH = (int)(bmpH * scale);

  if (outW < 1) outW = 1;
  if (outH < 1) outH = 1;

  int startX = centerX - outW / 2;
  int startY = topY;

  int bytesPerRow = (bmpW + 7) / 8;

  for (int yy = 0; yy < outH; yy++) {
    int srcY = (yy * bmpH) / outH;

    for (int xx = 0; xx < outW; xx++) {
      int srcX = (xx * bmpW) / outW;

      int byteIndex = srcY * bytesPerRow + (srcX / 8);
      uint8_t b = pgm_read_byte(&bitmap[byteIndex]);
      uint8_t mask = 0x80 >> (srcX % 8);

      if (b & mask) {
        epaper.drawPixel(startX + xx, startY + yy, color);
      }
    }
  }
}