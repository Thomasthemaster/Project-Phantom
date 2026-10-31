# 🕵️ PROJECT PHANTOM

**A DIY Arduino-powered Halloween escape room.**

PROJECT PHANTOM is an interactive spy-themed mystery game built around an Arduino Nano, an RFID scanner, and an OLED display. Players investigate the disappearance of a birthday present by discovering hidden RFID clues, solving puzzles, examining suspects, and uncovering a secret.

The project combines electronics, programming, physical props, storytelling, and puzzle design to create a personalized escape-room experience at home.

> **Mission Status:** Successfully completed. The birthday agent played the game!
>
> You can build this at home yourself and play with it too!

---

## 📸 Preview

<!-- Add photos of your finished device, physical documents, and gameplay here. -->

![PROJECT PHANTOM device](assets/images/Project_Phantom_Image.png)

*The PROJECT PHANTOM handheld investigation device.*

### Physical Documents

| Document | Description |
|---|---|
| Classified Briefing Letter | The original handwritten mission briefing from BPTPI. |
| Field Notes | A special investigator worksheet for recording evidence and deductions. |

<!-- Add your actual document images below. -->
![Original briefing letter FRONT](assets/images/Briefing_Letter_FRONT.png)

![Original briefing letter BACK](assets/images/Briefing_Letter_BACK.png)

![Investigator field notes FRONT](assets/images/Field_Notes_FRONT.png)

![Investigator field notes BACK](assets/images/Field_Notes_BACK.png)

---

## 🎯 Mission Objective

A birthday present has mysteriously disappeared on Halloween night.

The Birthday Present Theft Prevention Institute (BPTPI) has narrowed the investigation down to three suspects:

- **Raven** — Claims to have guarded the front entrance.
- **Maggie** — Claims she was resting inside the house.
- **Moth** — Claims he was filming in the attic.

The player must investigate the crime by finding hidden RFID tags, collecting evidence, solving puzzles, and determining who moved the present.

But the mystery has another layer...

**Not everything is as it seems.**

---

## ⚙️ Hardware

The project uses the following components:

| Component | Purpose |
|---|---|
| Arduino Nano | Main microcontroller |
| RFID-RC522 Reader | Detects the hidden RFID tags |
| SSD1309 128×64 OLED | Displays clues, menus, and game information |
| Pushbutton | Navigation and game controls |
| Tilt Switch | Detects when the device is tilted |
| Passive Buzzer | Sound effects and security alerts |
| Green LED | Normal operating status |
| Red LED | Warnings and security alerts |
| Power MB V2 | Allows device to be portable |

### Pin Connections

| Component | Arduino Pin |
|---|---|
| MFRC522 SS/SDA | D8 |
| MFRC522 RST | D4 |
| MFRC522 MOSI | D11 |
| MFRC522 MISO | D12 |
| MFRC522 SCK | D13 |
| OLED SDA | A4 |
| OLED SCL | A5 |
| Pushbutton | A2 |
| Tilt Switch | D2 |
| Passive Buzzer | D6 |
| Green LED | D7 |
| Red LED | D5 |

**Important:** The MFRC522 reader is powered using 3.3V. Pin D10 is also configured as an OUTPUT to maintain SPI master mode on the Nano.

---

## 📡 RFID Investigation System

Nine RFID tags are used throughout the game.

Seven tags contain the main evidence, one opens the final archive, and one reveals an optional bonus mission.

| Tag | File | Purpose |
|---|---|---|
| 01 | Briefing | Introduces the case and suspects |
| 02 | Clock | Reveals the suspicious 11:47 timestamp |
| 03 | Mirror | Contains a reverse-text puzzle |
| 04 | Book | Contains a Caesar cipher |
| 05 | Chair | Reveals hidden camera evidence |
| 06 | Lamp | Reveals evidence through a reflection |
| 07 | Silver | Connects the evidence to a suspect |
| 08 | Final Archive | Unlocks the conclusion and present information |
| 09 | Bonus File | Opens Operation Black Cat |

Each RFID tag is identified using its unique identifier (UID), which is stored in the Arduino sketch.

The seven main evidence tags are tracked using a bitmask so the program can identify which files have already been discovered.

---

## 🎮 Gameplay Features

### 1. RFID Investigation

Players scan hidden RFID tags to discover new evidence.

Each tag reveals information through the OLED display. Some files contain multiple pages, requiring the player to press the button to continue.

### 2. Puzzle Solving

The investigation includes multiple puzzle types:

- Reverse-text decoding.
- Caesar cipher decoding.
- Evidence comparison.
- Suspect analysis.
- Timeline reconstruction.

### 3. Security Breach Challenge

The device introduces a timed challenge in which the player must press the button five times within ten seconds.

The buzzer and red LED provide security-alert feedback.

### 4. Suspect Accusation

After collecting all seven evidence files, the player can make a final accusation.

The button controls the suspect selection:

- Short press: Cycle through suspects.
- Long press: Confirm the accusation.

Incorrect accusations provide additional hints.

### 5. Plot Twist

The final investigation reveals that Moth moved the present, but the incident was actually part of a secret BPTPI recruitment test.

### 6. Operation Black Cat

An optional ninth RFID tag unlocks a bonus mission with additional classified information.

### 7. Tilt Protection

The device monitors its orientation using a tilt switch.

When tilted, it displays a security warning and activates an alert until the device is returned to a level position.

---

## 💻 Software

The project is programmed in C++ using the Arduino IDE.

### Libraries

| Library | Purpose |
|---|---|
| SPI | Communication with the RFID reader |
| MFRC522 | RFID identification |
| U8g2 | OLED graphics and text |
| avr/pgmspace.h | Stores data in program memory |
| string.h | Text manipulation |

### Source Code

The complete Arduino sketch is located at:

`src/PROJECT_PHANTOM/PROJECT_PHANTOM.ino`

The sketch contains the RFID database, game logic, display functions, puzzle files, sound effects, security challenge, and game progression system.

---

## 📁 Repository Structure

```text
PROJECT-PHANTOM/
│
├── README.md
│
├── src/
│   └── PROJECT_PHANTOM/
│       └── Project_Phantom.ino
│
├── assets/
│   ├── images/
│   │   ├── Project_Phantom_Image.png
│   │   ├── Briefing_Letter_FRONT.png
│   │   ├── Briefing_Letter_BACK.png
|   |   ├── Field_Notes_FRONT.png
│   │   └── Field_Notes_BACK.png
│   │
│   └── rfid/
│       └── tag-reference.md
│
└── LICENSE
```

---

## 🔧 How to Upload

1. Install the Arduino IDE.
2. Install the MFRC522 library.
3. Install the U8g2 library.
4. Connect the hardware according to the pin connection table.
5. Open `PROJECT_PHANTOM.ino`.
6. Select **Arduino Nano** as the board.
7. Select the correct COM port.
8. Upload the sketch.
9. Power the device and complete the startup test.

Place the RFID tags in their intended investigation locations before starting the game.

---

## 🧠 What I Learned

     This project helped me learn how to debug big projects, as this project had over 1,000 lines of code. Not only that, but I learned how to use the RFID-RC522 reader and the MFRC522 library in the process. I also learned how to use the 2.42" OLED using multiple libraries. 
     Most importantly, it taught me that electronics can be used to create interactive experiences; not just circuits that perform individual tasks.
     

---

## 🎃 Project Completion

PROJECT PHANTOM was created as a Halloween-themed birthday surprise.

The finished device and investigation materials were used in a real playthrough, and the birthday player successfully enjoyed the experience.

**Mission accomplished.**

---

## 📜 Credits

Designed, programmed, assembled, and documented by **Thomasthemaster**.

**Project:** PROJECT PHANTOM  
**Organization:** Birthday Present Theft Prevention Institute (BPTPI)  
**Case Number:** 013  
**Theme:** Halloween / Spy Mystery / Escape Room

**The story is created by Thomasthemaster, and it is a fictional mission.**
