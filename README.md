# Why Isn't It Done Yet? — Excuse Cat

A tiny USB-powered e-paper excuse generator that lets a cat explain why your project still isn't done.

This project is a small offline desk gadget built with an e-paper display and a button.  
Plug it into USB power, wait for the startup page, then press the button to ask the cat for an excuse.

No Wi-Fi.  
No cloud.  
No API.  
Just one dramatic cat and 60 locally stored excuses.

## What it does

- Shows a startup page when powered on
- Press the button to enter the excuse generator page
- Generates one of 60 local excuses
- Displays different ASCII cat moods with the excuse
- Returns to the startup page after 60 seconds of inactivity
- Includes an impatient-cat easter egg if you press the button too many times
- Uses several bitmap cat pages, including sleep, play ball, eating, hissing, and a Cheems-style easter egg

## Project idea

I wanted to make a small desk companion for makers, procrastinators, and people with too many unfinished projects.

Instead of being a serious productivity tool, this is more like a tiny emotional support cat.  
When your project is not done, the cat gives you a reason. Maybe it is the weather. Maybe it is the vibe. Maybe your brain is buffering.

## Hardware

- E-paper display board
- ESP32-based controller
- One onboard button
- USB power

## How to use

1. Open `Code/ExcuseCatV4/ExcuseCatV4.ino` in Arduino IDE.
2. Make sure all `.h` files are in the same `ExcuseCatV4` folder.
3. Select the correct board and port.
4. Upload the sketch.
5. Plug in USB power.
6. Press the button to generate excuses.

## File structure

```text
Code/
└─ Excuse-Cat/
   ├─ ExcuseCatV4.ino
   ├─ driver.h
   ├─ sleep_cat_bitmap.h
   ├─ play_ball_cat_bitmap.h
   ├─ eat_cat_bitmap.h
   ├─ hiss_cat_bitmap.h
   └─ cheems_bitmap.h
```
## Current features

- 60 offline excuses
- Startup page
- Excuse generator page
- 10 ASCII cat moods
- Pixel-style UI
- Cat paw icon
- 60-second auto return to home
- Multi-press easter egg mode

## Future ideas

- Add motion or tilt interaction
- Add gesture-based cat reactions
- Add more cat moods
- Add sensor-based reactions
- Make it feel more like a tiny desktop pet

## Notes

This is a beginner-friendly Arduino project and still a work in progress.

The main goal is to make a small e-paper object that feels fun, personal, and slightly ridiculous.


