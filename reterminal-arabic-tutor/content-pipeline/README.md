# Content pipeline

Generates one day's reading text + vocab cards and renders them to the tiny
1bpp bitmap format the firmware reads directly off the SD card (or via sync).

## Setup

```
pip install -r requirements.txt
```

Then download the three fonts listed in `fonts/README.md` into `fonts/`.

## Manual mode (no API key needed)

Ask me for a new text in a chat, in this shape (or just use `sample_day.json`
as a starting template and edit it):

```json
{
  "title": "short title",
  "reading": [{"ar": "...", "gr": "...", "zh": "..."}, ...],
  "vocab": [{"ar": "...", "translit": "...", "gr": "...", "zh": "..."}, ...]
}
```

Save it as `today.json`, then:

```
python generate_day.py --from-file today.json
```

This writes bitmaps + manifest.json to `../docs/content/latest/`. From there
you can either:
- **Push over local WiFi** — if your firmware build includes a small upload
  endpoint (not included by default; the sticky-micronotes project linked in
  `firmwares/egyptian-arabic-tutor/README_DRIVERS.md` has a working example of this pattern you
  could borrow), or
- **Copy to SD card directly** — mount the card, copy the `latest` folder's
  contents into `/content/<date>/` on the card yourself. Simplest option if
  you don't want to build any device-side networking at all.

## Automatic mode (daily, no manual step)

```
export ANTHROPIC_API_KEY=sk-...
python generate_day.py --auto
```

This is what `automation/.github/workflows/daily-content.yml` runs on a
schedule — see `automation/README.md` to set that up so it happens by
itself every day and the device just pulls from a stable URL.
