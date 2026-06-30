# Why-Isnt-It-Done-Yet

An offline ESP32 e-paper excuse generator based on Seeed Studio EE05 and Arduino IDE for maker projects that are definitely almost done.

## Project Idea

**Why Isn't It Done Yet?** is a small e-paper desk board that randomly displays funny excuses for unfinished maker projects.

It is designed as a beginner-friendly, offline, one-button project.

## Hardware

- Seeed Studio EE05 e-paper DIY kit
- 4.26" monochrome e-paper display
- USB-C power
- One onboard user button
- 3D-printed enclosure

## V1 Features

- No Wi-Fi
- No API
- No sensors
- No battery for now
- Local text stored in code
- One-button random excuse refresh
- Full-screen e-paper refresh
- Retro terminal-style black-and-white UI

## How It Works

On startup, the board displays one random excuse.

When the button is pressed, it randomly selects another excuse from a local text array and refreshes the e-paper screen.

Example:

PROJECT: UNFINISHED

[EXCUSE GENERATED]
The code works, but only when nobody is watching.

> PRESS BUTTON TO GENERATE ANOTHER EXCUSE
[SYSTEM STATUS]
PROJECT: UNFINISHED

[EXCUSE GENERATED]
The code works, but only when nobody is watching.

> PRESS BUTTON TO GENERATE ANOTHER EXCUSE
Development Plan
 Run the official EE05 + 4.26" e-paper example
 Display custom text
 Add local excuse array
 Randomly select one excuse on startup
 Add one-button refresh
 Design terminal-style UI
 Adjust enclosure for USB-C access
 Add one button opening
 Record demo video
Status

Work in progress.

Note
This is a personal beginner maker project and not an official product demo.
