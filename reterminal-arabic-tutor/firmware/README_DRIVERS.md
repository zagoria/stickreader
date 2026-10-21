# Filling in the hardware drivers

Three files in `src/` are interfaces, not implementations:

- `hal_display.h` → needs `hal_display.cpp` (SSD1677 e-ink panel driver)
- `hal_touch.h` → needs `hal_touch.cpp` (GT911 touch controller driver)
- `hal_buzzer.h` → needs `hal_buzzer.cpp` (simple PWM beeper — easiest one, you
  can probably just write this directly with `ledcAttach`/`ledcWriteTone` on
  `PIN_BUZZER`)

I didn't write the display and touch drivers myself because getting a panel
init sequence subtly wrong is a common source of "compiles fine, shows
nothing" bugs that are painful to debug without the physical hardware in
front of you — and I don't have it. Two community firmware projects already
have working, MIT-licensed drivers for this *exact* board:

- **`LowFlowIO/sticky-micronotes`** (GitHub) — e-ink notes app, confirmed
  working on "reTerminal Sticky production hardware" per its own test log.
  Good reference for the SSD1677 init/refresh sequence and SD card handling.
- **`limengdu/reTerminal_Sticky_Bunny`** (GitHub) — a fuller app with GT911
  touch (taps + swipe paths), buzzer patterns, RTC and battery gauge all
  wired up. Good reference for touch and buzzer especially.

## Steps

1. Clone both repos somewhere temporary.
2. Find their display driver source (likely under `src/` or `components/`,
   named something like `sticky_display.*` or an SSD1677-specific file).
   Adapt its init/clear/draw/refresh/sleep calls into this project's
   `hal_display.cpp`, matching the function signatures in `hal_display.h`.
3. Do the same for touch — find their GT911 driver (likely
   `sticky_touch.*`), adapt into `hal_touch.cpp`. You mainly need "give me
   the current touch point" and ideally swipe detection; simplify their
   gesture logic if it does more than you need.
4. Pin numbers in `config.h` were taken from the 2048-game repo's own
   documented hardware mapping table — cross-check them against whichever
   driver source you pull from in case of any revision differences.
5. Build with `pio run -e sticky` and fix compile errors as they come up —
   expect some, this is a genuine integration step, not a drop-in.

## Flashing

```
pio run -e sticky -t upload
pio device monitor -b 115200
```

Watch the serial monitor on first boot — `main.cpp` logs each stage (SD init,
sync, content load) so you can see exactly where anything goes wrong.
