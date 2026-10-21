# Egyptian Arabic Tutor for reTerminal Sticky

A reading + vocabulary-flashcard trainer for the Seeed Studio **reTerminal Sticky**
(ESP32-S3, 3.97" 800×480 e-ink, GT911 touch), showing Egyptian Arabic alongside
Greek and Chinese, with a new reading text published every day.

## Architecture

```
┌─────────────────────┐      daily cron       ┌──────────────────────┐
│   GitHub Actions     │ ───────────────────▶  │  generate_day.py     │
│  (scheduled, free)   │                       │  calls Claude API    │
└─────────────────────┘                        │  renders bitmaps     │
           │                                    └──────────┬───────────┘
           │ commits output to /docs                        │
           ▼                                                 │
┌─────────────────────┐                                      │
│   GitHub Pages       │◀─────────────────────────────────────┘
│  (free static host)  │
│  /content/latest/... │
└──────────┬────────────┘
           │ HTTPS GET (device polls once a day / on button press)
           ▼
┌─────────────────────┐
│  reTerminal Sticky    │  caches bitmaps to SD card, displays
│  firmware             │  reading view + flashcard trainer
└─────────────────────┘
```

## Why it's built this way (read this first)

I can't personally run on a daily schedule — I only act inside a conversation
when you talk to me. So "a new text every day, automatically" needs something
that *is* always running. A scheduled GitHub Action is the simplest free way to
get that: it wakes up once a day, calls the Claude API using **your own API
key**, generates a new dialogue/reading text in the established style
(Arabic + Greek + Chinese), renders it to e-ink-ready bitmaps, and publishes it
to a GitHub Pages URL. The device just does a plain HTTPS GET of that URL —
no server you have to run yourself, no local network dependency.

**If you'd rather not set up the API key / Action**, the manual alternative is:
ask me for a new text in a chat whenever you want one, run
`content-pipeline/generate_day.py --from-file today.json` yourself, and push the
output to the device over your local WiFi (see `content-pipeline/README.md`).
The firmware doesn't care which path fed it — it just reads whatever's at the
URL (or, in manual mode, whatever you drop on the SD card).

## What I could and couldn't verify

I don't have physical reTerminal Sticky hardware to compile and flash against,
so I've split this into two trust levels:

- **App logic (confident):** WiFi, HTTPS fetch, SD caching, JSON parsing,
  flashcard state machine, pagination — all standard ESP32 Arduino APIs I'm
  confident in.
- **Low-level display/touch/buzzer drivers (needs your verification):** The
  exact SSD1677 e-ink init sequence and GT911 touch driver are hardware-specific
  and easy to get subtly wrong by guessing. Rather than fabricate untested
  register-level code, I built the firmware against a clean hardware
  abstraction layer (`hal_display.h`, `hal_touch.h`, `hal_buzzer.h`) and the
  firmware's own README tells you exactly which files to pull in from two
  working, MIT-licensed community firmware projects built for this exact
  device (`LowFlowIO/sticky-micronotes` and `limengdu/reTerminal_Sticky_Bunny`
  on GitHub) to implement those three files. That's a ~30-minute copy-adapt
  step, not a rewrite.

## Folders

- `firmwares/egyptian-arabic-tutor/` — the firmware, packaged for both local
  building and submission to the Sticky Playground community registry (see
  "Submitting to Sticky Playground" below). The buildable PlatformIO project
  is at `firmwares/egyptian-arabic-tutor/source/`.
- `content-pipeline/` — the Python script that generates a day's content and renders it to bitmaps
- `automation/` — the GitHub Action that runs the pipeline daily and publishes it

See each folder's own README for setup steps.

## Submitting to Sticky Playground

`firmwares/egyptian-arabic-tutor/` is packaged to match Seeed's
[contributing-firmware.md](https://github.com/Seeed-Projects/reterminal-sticky-playground-registry/blob/main/docs/contributing-firmware.md)
source-contribution layout (`firmware.json` + `README.md` + `assets/` +
`source/` with `platformio.ini`, `src/`, and `LICENSE`), so it's structurally
ready to drop into a fork of their registry repo as
`firmwares/egyptian-arabic-tutor/`.

**It is not yet ready to actually open as a pull request.** See the ⚠️ section
at the top of `firmwares/egyptian-arabic-tutor/README.md` — in short: the
drivers still need integrating, a physical-device test still needs doing and
recording, several `firmware.json` fields are placeholders (`YOUR_..._HERE`),
and the preview image is a software mockup, not a real device photo/screenshot
as the guide requires. I packaged the structure so none of that is blocking
work once you have the hardware in hand — the remaining steps are things only
possible with the physical device.
