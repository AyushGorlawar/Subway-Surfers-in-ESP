# Subway Surfers in ESP

A compact version of the popular game **Subway Surfers**, built on the ESP32 microcontroller. This project uses simple graphics and buttons/touch sensors to create an engaging and lightweight gaming experience.

## Features
- Endless runner gameplay.
- Dodge obstacles and collect coins.
- Compact design using ESP32 and an OLED display.

## Requirements
- **Hardware**:
  - ESP32 microcontroller.

- **Software**:
  - Arduino IDE or PlatformIO.
  - Required libraries:
    - Adafruit_GFX
    - Adafruit_SSD1306

## How to Play
1. Use buttons or touch sensors to move the character.
2. Avoid obstacles and collect coins to increase your score.
3. Aim for the highest score!

## Setup Instructions
1. Connect the OLED display to the ESP32:
   - SDA to GPIO21
   - SCL to GPIO22
2. Connect push buttons to GPIO pins for control.
3. Load the code onto the ESP32 using the Arduino IDE.
4. Power the ESP32 and start playing.

## Future Enhancements
- Add sound effects.
- Include a scoring leaderboard.
- Improve graphics and animations.

