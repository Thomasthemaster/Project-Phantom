
#include <SPI.h>
#include <MFRC522.h>
#include <U8g2lib.h>
#include <avr/pgmspace.h>
#include <string.h>

/*=====================================================
PROJECT PHANTOM 2.0
BIRTHDAY PRESENT THEFT PREVENTION INSTITUTE

Made by Thomas
Oct 30 2026
=====================================================*/

// ---------- HARDWARE ----------
const byte SS_PIN = 8;
const byte RST_PIN = 4;

const byte BUTTON_PIN = A2;
const byte TILT_PIN = 2;

const byte BUZZER_PIN = 6;
const byte GREEN_LED = 7;
const byte RED_LED = 5;

const bool TILT_ACTIVE_LOW = false;

// ---------- DEVICES ----------
MFRC522 rfid(SS_PIN, RST_PIN);

U8G2_SSD1309_128X64_NONAME2_1_HW_I2C oled(
  U8G2_R0, U8X8_PIN_NONE
);

// ---------- RFID DATABASE ----------
const byte UID_LENGTH = 7;
const byte TAG_COUNT = 9;

const byte uid[TAG_COUNT][UID_LENGTH] PROGMEM = {
  {0x04,0x69,0x68,0xA0,0x8C,0x26,0x81}, // 1 Briefing
  {0x04,0x43,0x5B,0xA0,0x8C,0x26,0x81}, // 2 Clock
  {0x04,0x48,0x54,0xA0,0x8C,0x26,0x81}, // 3 Mirror
  {0x04,0x97,0x5B,0xA0,0x8C,0x26,0x81}, // 4 Book
  {0x04,0x91,0x5F,0xA0,0x8C,0x26,0x81}, // 5 Chair
  {0x04,0x2F,0x54,0xA0,0x8C,0x26,0x81}, // 6 Lamp
  {0x04,0x92,0x57,0xA0,0x8C,0x26,0x81}, // 7 Silver
  {0x04,0x34,0x64,0xA0,0x8C,0x26,0x81}, // 8 Final
  {0x04,0x91,0x5B,0xA0,0x8C,0x26,0x81}  // 9 Bonus
};

// RFID tag indexes
const byte BRIEFING_TAG = 0;
const byte CLOCK_TAG = 1;
const byte MIRROR_TAG = 2;
const byte BOOK_TAG = 3;
const byte CHAIR_TAG = 4;
const byte LAMP_TAG = 5;
const byte SILVER_TAG = 6;
const byte FINAL_TAG = 7;
const byte BONUS_TAG = 8;

// Evidence tags are indexes 0 through 6.
// Bit 8 is reserved for the optional bonus.
uint16_t found = 0;

// ---------- GAME STATE ----------
bool securityPassed = false;
bool securityChallengeAttempted = false;

bool accusationPrompted = false;
bool culpritSolved = false;

bool bonusRevealed = false;

byte accusationAttempts = 0;

// ---------- DISPLAY STATE ----------
char currentTitle[32];
char currentBody[180];

int currentPageStart = 0;
int currentPageNext = 0;

bool showingHomeScreen = true;

// =====================================================
// HARDWARE HELPERS
// =====================================================

bool isTilted() {
  int reading = digitalRead(TILT_PIN);

  if (TILT_ACTIVE_LOW) {
    return reading == LOW;
  }

  return reading == HIGH;
}

void successSound() {
  tone(BUZZER_PIN, 2200, 100);
}

void errorSound() {
  tone(BUZZER_PIN, 350, 350);
}

void startupHardwareTest() {
  digitalWrite(GREEN_LED, HIGH);
  delay(300);

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);
  delay(300);

  digitalWrite(RED_LED, LOW);
  delay(150);

  tone(BUZZER_PIN, 1500, 150);
  delay(200);
}

// =====================================================
// EVIDENCE HELPERS
// =====================================================

byte evidenceCount() {
  byte count = 0;

  for (byte i = 0; i < 7; i++) {
    if (found & (1U << i)) {
      count++;
    }
  }

  return count;
}

bool allEvidence() {
  return (found & 0x007F) == 0x007F;
}

// =====================================================
// TEXT STORAGE
// =====================================================

void appendFlashText(const __FlashStringHelper *source) {
  char temporary[64];

  strncpy_P(temporary, (PGM_P)source, 63);
  temporary[63] = '\0';

  if (strlen(currentBody) > 0) {
    strncat(
      currentBody,
      " ",
      sizeof(currentBody) - strlen(currentBody) - 1
    );
  }

  strncat(
    currentBody,
    temporary,
    sizeof(currentBody) - strlen(currentBody) - 1
  );
}

// =====================================================
// WORD WRAPPING
// =====================================================

int buildPage(int start, char lines[3][21]) {
  int position = start;
  int length = strlen(currentBody);

  for (byte row = 0; row < 3; row++) {
    lines[row][0] = '\0';

    while (
      position < length &&
      currentBody[position] == ' '
    ) {
      position++;
    }

    int lineLength = 0;

    while (position < length) {
      int wordEnd = position;

      while (
        wordEnd < length &&
        currentBody[wordEnd] != ' '
      ) {
        wordEnd++;
      }

      int wordLength = wordEnd - position;

      if (wordLength > 20 && lineLength == 0) {
        memcpy(
          lines[row],
          &currentBody[position],
          20
        );

        lines[row][20] = '\0';
        position += 20;
        break;
      }

      int needed = wordLength;

      if (lineLength > 0) {
        needed++;
      }

      if (lineLength + needed > 20) {
        break;
      }

      if (lineLength > 0) {
        lines[row][lineLength++] = ' ';
      }

      memcpy(
        &lines[row][lineLength],
        &currentBody[position],
        wordLength
      );

      lineLength += wordLength;
      lines[row][lineLength] = '\0';

      position = wordEnd;

      while (
        position < length &&
        currentBody[position] == ' '
      ) {
        position++;
      }
    }
  }

  return position;
}

// =====================================================
// OLED DISPLAY
// =====================================================

void renderCurrentPage() {
  char lines[3][21];

  currentPageNext = buildPage(
    currentPageStart,
    lines
  );

  oled.setFont(u8g2_font_6x10_tf);

  oled.firstPage();

  do {
    oled.drawStr(0, 11, currentTitle);
    oled.drawHLine(0, 14, 128);

    oled.drawStr(0, 28, lines[0]);
    oled.drawStr(0, 41, lines[1]);
    oled.drawStr(0, 55, lines[2]);

  } while (oled.nextPage());
}

void renderWarning() {
  oled.setFont(u8g2_font_6x10_tf);

  oled.firstPage();

  do {
    oled.drawStr(0, 11, "SECURITY WARNING");
    oled.drawHLine(0, 14, 128);

    oled.drawStr(0, 28, "DEVICE TILTED!");
    oled.drawStr(0, 41, "Return it to level.");
    oled.drawStr(0, 55, "Evidence at risk...");

  } while (oled.nextPage());
}

// =====================================================
// HOME SCREEN
// =====================================================

void homeScreen() {
  showingHomeScreen = true;

  char progress[22];

  snprintf(
    progress,
    sizeof(progress),
    "Evidence: %u/7",
    evidenceCount()
  );

  oled.setFont(u8g2_font_6x10_tf);

  oled.firstPage();

  do {
    oled.drawStr(0, 11, "CASE 013 ACTIVE");
    oled.drawHLine(0, 14, 128);

    oled.drawStr(0, 29, "PROJECT PHANTOM");
    oled.drawStr(0, 42, progress);

    if (evidenceCount() < 3) {
      oled.drawStr(0, 56, "Scan next evidence.");
    }

    else if (!securityPassed) {
      oled.drawStr(0, 56, "Hold: retry breach.");
    }

    else if (!allEvidence()) {
      oled.drawStr(0, 56, "Scan next evidence.");
    }

    else if (!culpritSolved) {
      oled.drawStr(0, 56, "Hold: accuse suspect.");
    }

    else {
      oled.drawStr(0, 56, "Scan final archive.");
    }

  } while (oled.nextPage());

  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);
}

// =====================================================
// TILT RESPONSE
// =====================================================

void handleTilt() {
  if (!isTilted()) {
    return;
  }

  unsigned long startTime = millis();

  // 300 ms debounce
  while (millis() - startTime < 300) {
    if (!isTilted()) {
      return;
    }

    delay(10);
  }

  renderWarning();

  digitalWrite(GREEN_LED, LOW);
  tone(BUZZER_PIN, 1800);

  unsigned long levelStart = 0;

  while (true) {
    if (isTilted()) {
      levelStart = 0;

      digitalWrite(
        RED_LED,
        (millis() / 150) % 2
      );
    }

    else {
      if (levelStart == 0) {
        levelStart = millis();
      }

      if (millis() - levelStart >= 300) {
        break;
      }
    }

    delay(10);
  }

  noTone(BUZZER_PIN);

  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, HIGH);

  if (showingHomeScreen) {
    homeScreen();
  }

  else {
    renderCurrentPage();
  }

  delay(250);
}

// =====================================================
// BUTTON INPUT
// =====================================================

// Waits for a complete button press and release.
// Returns how long the button was held in milliseconds.

unsigned long getButtonPressDuration() {
  while (digitalRead(BUTTON_PIN) == HIGH) {
    handleTilt();
    delay(5);
  }

  delay(30);

  if (digitalRead(BUTTON_PIN) != LOW) {
    return 0;
  }

  unsigned long pressStart = millis();

  while (digitalRead(BUTTON_PIN) == LOW) {
    handleTilt();
    delay(5);
  }

  unsigned long duration = millis() - pressStart;

  delay(30);

  return duration;
}

void waitForButton() {
  while (getButtonPressDuration() == 0) {
    handleTilt();
  }
}

// =====================================================
// SCREEN FUNCTION
// =====================================================

void screen(
  const __FlashStringHelper *title,
  const __FlashStringHelper *a,
  const __FlashStringHelper *b,
  const __FlashStringHelper *c
) {
  showingHomeScreen = false;

  strncpy_P(
    currentTitle,
    (PGM_P)title,
    sizeof(currentTitle) - 1
  );

  currentTitle[sizeof(currentTitle) - 1] = '\0';

  currentBody[0] = '\0';

  appendFlashText(a);
  appendFlashText(b);
  appendFlashText(c);

  currentPageStart = 0;

  do {
    renderCurrentPage();

    waitForButton();

    currentPageStart = currentPageNext;

  } while (
    currentPageStart < strlen(currentBody)
  );
}

// =====================================================
// SECURITY BREACH CHALLENGE
// =====================================================

void drawSecurityChallenge(
  byte taps,
  byte secondsLeft
) {
  char tapText[20];
  char timeText[20];

  snprintf(
    tapText,
    sizeof(tapText),
    "TAPS: %u / 5",
    taps
  );

  snprintf(
    timeText,
    sizeof(timeText),
    "TIME LEFT: %u",
    secondsLeft
  );

  oled.setFont(u8g2_font_6x10_tf);

  oled.firstPage();

  do {
    oled.drawStr(0, 11, "SECURITY BREACH!");
    oled.drawHLine(0, 14, 128);

    oled.drawStr(0, 29, "TAP BUTTON 5 TIMES");

    oled.drawStr(0, 43, tapText);
    oled.drawStr(0, 57, timeText);

  } while (oled.nextPage());
}

void runSecurityChallenge() {
  securityChallengeAttempted = true;

  const byte REQUIRED_TAPS = 5;
  const unsigned long TIME_LIMIT = 10000UL;

  byte taps = 0;

  bool buttonArmed = true;

  unsigned long startTime = millis();
  unsigned long lastDisplay = 0;

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);

  tone(BUZZER_PIN, 1100, 180);

  while (
    taps < REQUIRED_TAPS &&
    millis() - startTime < TIME_LIMIT
  ) {
    handleTilt();

    // Count one tap per complete press.
    if (digitalRead(BUTTON_PIN) == HIGH) {
      buttonArmed = true;
    }

    else if (buttonArmed) {
      delay(25);

      if (digitalRead(BUTTON_PIN) == LOW) {
        buttonArmed = false;

        taps++;

        tone(
          BUZZER_PIN,
          1100 + taps * 150,
          80
        );
      }
    }

    unsigned long elapsed = millis() - startTime;

    byte secondsLeft = 0;

    if (elapsed < TIME_LIMIT) {
      secondsLeft =
        (TIME_LIMIT - elapsed + 999) / 1000;
    }

    // Refresh display and security light.
    if (millis() - lastDisplay >= 100) {
      lastDisplay = millis();

      drawSecurityChallenge(
        taps,
        secondsLeft
      );
    }

    digitalWrite(
      RED_LED,
      (millis() / 140) % 2
    );

    delay(5);
  }

  noTone(BUZZER_PIN);
  digitalWrite(RED_LED, LOW);

  if (taps >= REQUIRED_TAPS) {
    securityPassed = true;

    digitalWrite(GREEN_LED, HIGH);

    tone(BUZZER_PIN, 2200, 120);
    delay(150);
    tone(BUZZER_PIN, 2600, 160);

    screen(
      F("BREACH STOPPED"),
      F("Connection restored."),
      F("Evidence protected."),
      F("Continue mission.")
    );
  }

  else {
    securityPassed = false;

    errorSound();

    screen(
      F("SECURITY LOCKED"),
      F("Too slow, agent."),
      F("You can retry."),
      F("Hold button at home.")
    );
  }
}

// =====================================================
// SUSPECT ACCUSATION MENU
// =====================================================

void drawAccusationMenu(byte selected) {
  oled.setFont(u8g2_font_6x10_tf);

  oled.firstPage();

  do {
    oled.drawStr(0, 11, "FINAL DEDUCTION");
    oled.drawHLine(0, 14, 128);

    oled.drawStr(0, 28, "WHO MOVED THE GIFT?");

    oled.drawStr(
      0,
      40,
      selected == 0 ? "> RAVEN" : "  RAVEN"
    );

    oled.drawStr(
      0,
      51,
      selected == 1 ? "> MAGGIE" : "  MAGGIE"
    );

    oled.drawStr(
      0,
      62,
      selected == 2 ? "> MOTH" : "  MOTH"
    );

  } while (oled.nextPage());
}

void revealCaseTwist() {
  screen(
    F("MYSTERY SOLVED"),
    F("MOTH moved the gift."),
    F("The silver trace and"),
    F("camera frame prove it.")
  );

  screen(
    F("THE SECOND SECRET"),
    F("Moth was following"),
    F("BPTPI instructions."),
    F("This was no real theft.")
  );

  screen(
    F("OPERATION PHANTOM"),
    F("Raven guarded the door."),
    F("Maggie staged the glass"),
    F("crash. Moth hid the gift.")
  );

  screen(
    F("RECRUITMENT TEST"),
    F("BPTPI planned the mystery"),
    F("to test your detective"),
    F("skills. You passed, Agent.")
  );
}

void runAccusation() {
  byte selected = 0;

  screen(
    F("FINAL DEDUCTION"),
    F("Three suspects remain."),
    F("Tap to change suspect."),
    F("Hold to confirm.")
  );

  while (true) {
    drawAccusationMenu(selected);

    unsigned long duration =
      getButtonPressDuration();

    if (duration == 0) {
      continue;
    }

    // Short press: cycle suspect.
    if (duration < 650) {
      selected++;

      if (selected > 2) {
        selected = 0;
      }

      successSound();
    }

    // Long press: confirm suspect.
    else {
      accusationAttempts++;

      if (selected == 2) {
        culpritSolved = true;

        successSound();

        revealCaseTwist();

        return;
      }

      errorSound();

      if (accusationAttempts == 1) {
        screen(
          F("ACCUSATION REJECTED"),
          F("Something is missing."),
          F("Recheck the camera"),
          F("and the lamp evidence.")
        );
      }

      else if (accusationAttempts == 2) {
        screen(
          F("ACCESS DENIED"),
          F("The timeline conflicts."),
          F("Compare the reflection"),
          F("with the silver trace.")
        );
      }

      else {
        screen(
          F("NOT SO FAST"),
          F("One suspect's alibi"),
          F("does not match the"),
          F("physical evidence.")
        );
      }

      return;
    }
  }
}

// =====================================================
// GAME PROGRESSION
// =====================================================

void advanceGameStages() {
  // Trigger security challenge after three clues.
  if (
    evidenceCount() >= 3 &&
    !securityPassed &&
    !securityChallengeAttempted
  ) {
    runSecurityChallenge();
  }

  // Once all seven clues and security are complete,
  // automatically open the accusation menu once.
  if (
    allEvidence() &&
    securityPassed &&
    !culpritSolved &&
    !accusationPrompted
  ) {
    accusationPrompted = true;
    runAccusation();
  }
}

// Called from the home screen if a retry is available.
void checkHomeButton() {
  bool canRetrySecurity =
    evidenceCount() >= 3 &&
    !securityPassed;

  bool canRetryAccusation =
    allEvidence() &&
    securityPassed &&
    !culpritSolved;

  if (!canRetrySecurity && !canRetryAccusation) {
    return;
  }

  if (digitalRead(BUTTON_PIN) == LOW) {
    unsigned long duration =
      getButtonPressDuration();

    if (duration >= 700) {
      if (canRetrySecurity) {
        runSecurityChallenge();
      }

      else if (canRetryAccusation) {
        runAccusation();
      }

      homeScreen();
    }
  }
}

// =====================================================
// RFID IDENTIFICATION
// =====================================================

int scanTag() {
  if (rfid.uid.size != UID_LENGTH) {
    return -1;
  }

  for (byte t = 0; t < TAG_COUNT; t++) {
    bool matches = true;

    for (byte j = 0; j < UID_LENGTH; j++) {
      byte expected = pgm_read_byte(&uid[t][j]);

      if (rfid.uid.uidByte[j] != expected) {
        matches = false;
        break;
      }
    }

    if (matches) {
      return t;
    }
  }

  return -1;
}

// =====================================================
// EVIDENCE FILES
// =====================================================

void clue(byte t) {
  switch (t) {

    // -------------------------------------------------
    // FILE 01: BRIEFING
    // -------------------------------------------------

    case BRIEFING_TAG:

      screen(
        F("FILE 01: BRIEFING"),
        F("Present stolen at"),
        F("11:47 PM on Halloween."),
        F("BPTPI requests your help.")
      );

      screen(
        F("SUSPECT: RAVEN"),
        F("Claims to have guarded"),
        F("the front entrance."),
        F("Saw nobody enter or leave.")
      );

      screen(
        F("SUSPECT: MAGGIE"),
        F("Claims she was asleep."),
        F("Woke to breaking glass."),
        F("Saw gift near wall clock.")
      );

      screen(
        F("SUSPECT: MOTH"),
        F("Claims he filmed in attic."),
        F("Heard Maggie downstairs."),
        F("Says he never left attic.")
      );

      screen(
        F("YOUR MISSION"),
        F("Find seven evidence files."),
        F("Identify who moved the gift."),
        F("Discover the hidden truth.")
      );

      break;

    // -------------------------------------------------
    // FILE 02: CLOCK
    // -------------------------------------------------

    case CLOCK_TAG:

      screen(
        F("FILE 02: CLOCK"),
        F("The wall clock stopped"),
        F("at exactly 11:47 PM."),
        F("The reported theft time.")
      );

      screen(
        F("CLOCK ANALYSIS"),
        F("Dust on the wall shows"),
        F("the clock was moved"),
        F("after its hands stopped.")
      );

      screen(
        F("TIME MANIPULATION"),
        F("Was 11:47 the real time?"),
        F("Or did someone want you"),
        F("to believe it was?")
      );

      break;

    // -------------------------------------------------
    // FILE 03: MIRROR
    // -------------------------------------------------

    case MIRROR_TAG:

      screen(
        F("FILE 03: MIRROR"),
        F("KOOB EHT DNI F"),
        F("Read the message"),
        F("backwards.")
      );

      screen(
        F("MESSAGE DECODED"),
        F("FIND THE BOOK."),
        F("A new clue location"),
        F("has been revealed.")
      );

      break;

    // -------------------------------------------------
    // FILE 04: BOOK
    // -------------------------------------------------

    case BOOK_TAG:

      screen(
        F("FILE 04: THE BOOK"),
        F("Uifsf jt b tfdsfu"),
        F("voefs uif dibjs."),
        F("A cipher has been found.")
      );

      screen(
        F("CIPHER INSTRUCTIONS"),
        F("Move each letter"),
        F("back by one position."),
        F("Decode the message.")
      );

      screen(
        F("MESSAGE DECODED"),
        F("THERE IS A SECRET"),
        F("UNDER THE CHAIR."),
        F("Search beneath it.")
      );

      break;

    // -------------------------------------------------
    // FILE 05: CHAIR
    // -------------------------------------------------

    case CHAIR_TAG:

      screen(
        F("FILE 05: THE CHAIR"),
        F("A hidden camera frame"),
        F("was found underneath."),
        F("Timestamp: 11:47:13.")
      );

      screen(
        F("CAMERA FRAME"),
        F("A shadow carries the gift."),
        F("Its direction leads from"),
        F("the attic stairs.")
      );

      screen(
        F("NEW QUESTION"),
        F("Who could leave the attic"),
        F("without being seen"),
        F("by the front entrance?")
      );

      break;

    // -------------------------------------------------
    // FILE 06: LAMP
    // -------------------------------------------------

    case LAMP_TAG:

      screen(
        F("FILE 06: THE LAMP"),
        F("Its polished base"),
        F("contains a reflection."),
        F("Something was captured.")
      );

      screen(
        F("REFLECTION ANALYSIS"),
        F("A hand is carrying"),
        F("the stolen present."),
        F("A silver ring is visible.")
      );

      screen(
        F("COMPARE THE SUSPECTS"),
        F("Who wears a silver ring?"),
        F("Who claimed to stay"),
        F("in the attic?")
      );

      break;

    // -------------------------------------------------
    // FILE 07: SILVER
    // -------------------------------------------------

    case SILVER_TAG:

      screen(
        F("FILE 07: SILVER"),
        F("Silver thread was caught"),
        F("on a camera strap."),
        F("The strap belongs to Moth.")
      );

      screen(
        F("FINAL EVIDENCE"),
        F("The ring matches Moth."),
        F("The strap matches him."),
        F("His attic alibi is false.")
      );

      screen(
        F("CASE ANALYSIS"),
        F("Raven guarded the entrance."),
        F("Maggie heard the glass."),
        F("Moth moved the present.")
      );

      break;
  }
}

// =====================================================
// FINAL ARCHIVE
// =====================================================

void openFinalArchive() {
  if (!allEvidence()) {
    screen(
      F("ARCHIVE LOCKED"),
      F("Seven evidence files"),
      F("are required."),
      F("Continue investigating.")
    );

    return;
  }

  if (!securityPassed) {
    screen(
      F("ACCESS DENIED"),
      F("Security challenge"),
      F("must be completed."),
      F("Restore the connection.")
    );

    return;
  }

  if (!culpritSolved) {
    screen(
      F("ARCHIVE LOCKED"),
      F("Make your accusation"),
      F("before opening the"),
      F("final archive.")
    );

    return;
  }

  successSound();

  screen(
    F("ARCHIVE UNLOCKED"),
    F("CASE 013 CLOSED."),
    F("BOX CODE: 147"),
    F("The gift is waiting.")
  );

  screen(
    F("FINAL LOCATION"),
    F("Your present is near"),
    F("some furniture."),
    F("Find it, agent.")
  );
}

// =====================================================
// BONUS FILE
// =====================================================

void openBonusFile() {
  // Bonus is optional and does not count toward
  // the seven required evidence files.

  if (!culpritSolved) {
    screen(
      F("BONUS FILE SEALED"),
      F("This file is encrypted."),
      F("Solve the main case"),
      F("before opening it.")
    );

    return;
  }

  if (bonusRevealed) {
    screen(
      F("BONUS ALREADY OPEN"),
      F("Operation Black Cat"),
      F("has been documented."),
      F("Good work, agent.")
    );

    return;
  }

  bonusRevealed = true;

  successSound();

  screen(
    F("OPERATION BLACK CAT"),
    F("A final recording"),
    F("was hidden by BPTPI."),
    F("You found the ninth file.")
  );

  screen(
    F("CLASSIFIED MESSAGE"),
    F("Your agent file existed"),
    F("before Case 013 began."),
    F("You were the real target.")
  );

  screen(
    F("WELCOME, AGENT"),
    F("The mission was designed"),
    F("for your birthday."),
    F("You have been recruited.")
  );
}

// =====================================================
// STARTUP
// =====================================================

void setup() {
  Serial.begin(9600);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(TILT_PIN, INPUT_PULLUP);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);

  // D10 MUST remain OUTPUT for Uno SPI master mode.
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  oled.setI2CAddress(0x3C * 2);
  oled.begin();

  SPI.begin();
  rfid.PCD_Init();

  startupHardwareTest();

  Serial.println(F("PROJECT PHANTOM 2.0 READY"));

  screen(
    F("PROJECT PHANTOM"),
    F("CASE 013"),
    F("Scanner online."),
    F("Press to begin.")
  );

  screen(
    F("WELCOME, AGENT"),
    F("Your investigation"),
    F("starts now."),
    F("Find the evidence.")
  );

  homeScreen();
}

// =====================================================
// MAIN GAME LOOP
// =====================================================

void loop() {
  handleTilt();

  // Run game stages when their conditions are met.
  advanceGameStages();

  // Long press can retry a failed challenge
  // or reopen the suspect accusation menu.
  checkHomeButton();

  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  int t = scanTag();

  // End RFID communication before showing screens.
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  // ---------------------------------------------------
  // UNKNOWN TAG
  // ---------------------------------------------------

  if (t < 0) {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);

    errorSound();

    screen(
      F("UNKNOWN TAG"),
      F("No matching case file."),
      F("Try another tag."),
      F("Case 013 active.")
    );

    digitalWrite(RED_LED, LOW);
  }

  // ---------------------------------------------------
  // FINAL ARCHIVE TAG
  // ---------------------------------------------------

  else if (t == FINAL_TAG) {
    openFinalArchive();
  }

  // ---------------------------------------------------
  // BONUS TAG
  // ---------------------------------------------------

  else if (t == BONUS_TAG) {
    found |= (1U << 8);

    openBonusFile();
  }

  // ---------------------------------------------------
  // DUPLICATE EVIDENCE
  // ---------------------------------------------------

  else if (found & (1U << t)) {
    screen(
      F("FILE ALREADY FOUND"),
      F("Evidence preserved."),
      F("No duplicate needed."),
      F("Keep investigating.")
    );
  }

  // ---------------------------------------------------
  // NEW EVIDENCE
  // ---------------------------------------------------

  else {
    found |= (1U << t);

    successSound();

    Serial.print(F("Evidence file found: "));
    Serial.println(t + 1);

    clue(t);

    // Trigger security and accusation stages
    // when their requirements have been met.
    advanceGameStages();
  }

  homeScreen();
}
