#!/usr/bin/env python3
"""
generate_day.py — builds one day's reading text + vocab cards for the
reTerminal Sticky Arabic tutor, and renders everything to the tiny 1bpp
bitmap format the firmware expects.

Two ways to feed it content:

  --from-file today.json   Use text you already have (e.g. pasted from a
                            chat with Claude). See sample_day.json for the
                            expected shape.

  --auto                   Call the Claude API directly (needs
                            ANTHROPIC_API_KEY set) to generate a new
                            Egyptian-Arabic reading text + vocab list in the
                            established style. This is what the scheduled
                            GitHub Action uses.

Output goes to:
  docs/content/latest/          <- what the device fetches (overwritten daily)
  docs/content/history/<date>/  <- kept archive, not fetched by the device

Run `pip install -r requirements.txt` first, and see fonts/README.md.
"""

import argparse
import json
import os
import shutil
import struct
import sys
from datetime import date, datetime
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont
import arabic_reshaper
from bidi.algorithm import get_display

HERE = Path(__file__).parent
FONTS = HERE / "fonts"
OUTPUT_LATEST = HERE.parent / "docs" / "content" / "latest"
OUTPUT_HISTORY = HERE.parent / "docs" / "content" / "history"

SCREEN_W, SCREEN_H = 800, 480
MARGIN = 24

# Must match MENU_BAR_HEIGHT in firmware/src/config.h — the firmware uses
# this purely as a touch hit-zone, the pixels themselves are drawn here.
MENU_H = 56
CONTENT_H = SCREEN_H - MENU_H  # usable height above the menu bar

FONT_AR = FONTS / "NotoNaskhArabic-Regular.ttf"
FONT_LATIN = FONTS / "NotoSans-Regular.ttf"
FONT_CJK = FONTS / "NotoSansSC-Regular.otf"


# ---------------------------------------------------------------- rendering

def shape_arabic(text: str) -> str:
    """Reshape Arabic into contextual letter forms and reorder for display.
    Needed because PIL draws text left-to-right with isolated glyph forms
    otherwise — this is the step that makes it look like real Arabic."""
    reshaped = arabic_reshaper.reshape(text)
    return get_display(reshaped)


def load_font(path: Path, size: int) -> ImageFont.FreeTypeFont:
    if not path.exists():
        sys.exit(f"Missing font: {path} — see fonts/README.md")
    return ImageFont.truetype(str(path), size)


def new_page() -> Image.Image:
    return Image.new("1", (SCREEN_W, SCREEN_H), color=1)  # 1 = white


def draw_wrapped(draw, text, font, x, y, max_width, rtl=False, fill=0):
    """Very simple word-wrap. Returns the y position after the block.
    For Arabic, caller should pass already-shaped text and we still wrap
    on whitespace of the *original* logical text — to keep this simple we
    expect short dialogue/vocab lines (true of this content), not long
    paragraphs, so naive wrapping is fine here."""
    words = text.split(" ")
    line = ""
    line_height = font.size + 6
    for word in words:
        trial = (word + " " + line) if rtl else (line + " " + word)
        trial = trial.strip()
        w = draw.textlength(trial, font=font)
        if w > max_width and line:
            draw.text((x, y), line, font=font, fill=fill,
                       anchor="ra" if rtl else "la")
            y += line_height
            line = word
        else:
            line = trial
    if line:
        draw.text((x, y), line, font=font, fill=fill,
                   anchor="ra" if rtl else "la")
        y += line_height
    return y


def draw_menu_bar(draw: ImageDraw.ImageDraw, active: str):
    """Bakes the persistent Home/Read/Train bar into the bottom MENU_H
    pixels of every bitmap. `active` is 'home', 'reading', or 'trainer' —
    that third is drawn inverted (filled black, white text) since there's
    no color to rely on with monochrome e-ink. This is static per-bitmap,
    not live UI — the firmware just needs to know where the three tap
    zones are (see main.cpp's handleMenuBarTap), matching this 3-way split."""
    f = load_font(FONT_LATIN, 18)
    bar_top = SCREEN_H - MENU_H
    draw.line([(0, bar_top), (SCREEN_W, bar_top)], fill=0, width=1)

    labels = [("home", "Home"), ("reading", "Read"), ("trainer", "Train")]
    seg_w = SCREEN_W // 3
    for i, (key, label) in enumerate(labels):
        x0, x1 = i * seg_w, (i + 1) * seg_w
        cx = (x0 + x1) // 2
        cy = bar_top + MENU_H // 2
        if key == active:
            pad = 10
            draw.rectangle([x0 + pad, bar_top + 8, x1 - pad, SCREEN_H - 8], fill=0)
            draw.text((cx, cy), label, font=f, fill=1, anchor="mm")
        else:
            draw.text((cx, cy), label, font=f, fill=0, anchor="mm")


def render_reading_line(ar: str, gr: str, zh: str, page_num: int, page_total: int) -> Image.Image:
    """One scrollable 'slide' per sentence, matching the stacked format
    used in chat: Arabic (large), then Greek, then Chinese. Menu bar +
    page counter baked in at the bottom."""
    img = new_page()
    draw = ImageDraw.Draw(img)

    f_ar = load_font(FONT_AR, 32)
    f_gr = load_font(FONT_LATIN, 21)
    f_zh = load_font(FONT_CJK, 21)
    f_small = load_font(FONT_LATIN, 16)

    y = MARGIN
    ar_shaped = shape_arabic(ar)
    y = draw_wrapped(draw, ar_shaped, f_ar, SCREEN_W - MARGIN, y,
                      SCREEN_W - 2 * MARGIN, rtl=True)
    y += 16
    y = draw_wrapped(draw, gr, f_gr, MARGIN, y, SCREEN_W - 2 * MARGIN)
    y += 10
    draw_wrapped(draw, zh, f_zh, MARGIN, y, SCREEN_W - 2 * MARGIN)

    draw.text((SCREEN_W - MARGIN, CONTENT_H - 20), f"{page_num} / {page_total}",
               font=f_small, fill=0, anchor="ra")

    draw_menu_bar(draw, active="reading")
    return img


def render_vocab_front(ar: str) -> Image.Image:
    img = new_page()
    draw = ImageDraw.Draw(img)
    f_ar = load_font(FONT_AR, 60)
    f_small = load_font(FONT_LATIN, 16)
    ar_shaped = shape_arabic(ar)
    draw.text((SCREEN_W / 2, CONTENT_H / 2), ar_shaped, font=f_ar,
               fill=0, anchor="mm")
    draw.text((SCREEN_W / 2, 30), "tap to flip", font=f_small, fill=0, anchor="mm")
    draw_menu_bar(draw, active="trainer")
    return img


def render_vocab_back(ar: str, translit: str, gr: str, zh: str) -> Image.Image:
    img = new_page()
    draw = ImageDraw.Draw(img)
    f_ar = load_font(FONT_AR, 34)
    f_small = load_font(FONT_LATIN, 19)
    f_zh = load_font(FONT_CJK, 19)

    y = MARGIN
    ar_shaped = shape_arabic(ar)
    y = draw_wrapped(draw, ar_shaped, f_ar, SCREEN_W - MARGIN, y,
                      SCREEN_W - 2 * MARGIN, rtl=True)
    y += 10
    y = draw_wrapped(draw, translit, f_small, MARGIN, y, SCREEN_W - 2 * MARGIN)
    y += 14
    y = draw_wrapped(draw, "GR: " + gr, f_small, MARGIN, y, SCREEN_W - 2 * MARGIN)
    y += 10
    draw_wrapped(draw, "ZH: " + zh, f_zh, MARGIN, y, SCREEN_W - 2 * MARGIN)

    draw_menu_bar(draw, active="trainer")
    return img


def render_home_page(title: str, num_sentences: int, num_vocab: int, date_str: str) -> Image.Image:
    img = new_page()
    draw = ImageDraw.Draw(img)
    f_ar = load_font(FONT_AR, 36)
    f_mid = load_font(FONT_LATIN, 20)
    f_small = load_font(FONT_LATIN, 15)

    cy = CONTENT_H / 2
    ahlan = shape_arabic("أهلاً")
    draw.text((SCREEN_W / 2, cy - 60), ahlan, font=f_ar, fill=0, anchor="mm")
    draw.text((SCREEN_W / 2, cy - 5), f"Today: {title}", font=f_mid, fill=0, anchor="mm")
    draw.text((SCREEN_W / 2, cy + 30),
               f"{num_sentences} sentences · {num_vocab} vocab words",
               font=f_small, fill=0, anchor="mm")
    draw.text((SCREEN_W / 2, cy + 55), date_str, font=f_small, fill=0, anchor="mm")

    draw_menu_bar(draw, active="home")
    return img


def image_to_1bpp(img: Image.Image) -> bytes:
    """Device-side format: 2-byte LE width, 2-byte LE height, then packed
    1-bit rows (MSB first) — matches firmware/src/content.cpp readBitmap()."""
    w, h = img.size
    row_bytes = (w + 7) // 8
    out = bytearray(struct.pack("<HH", w, h))
    px = img.load()
    for yy in range(h):
        row = bytearray(row_bytes)
        for xx in range(w):
            # PIL mode "1": 0=black, 255=white after convert; our new_page()
            # uses 1-bit mode directly where 1=white, 0=black already.
            bit = 1 if px[xx, yy] else 0
            if not bit:  # black pixel -> set bit
                row[xx // 8] |= (0x80 >> (xx % 8))
        out += row
    return bytes(out)


# ------------------------------------------------------------------ content

def default_topics():
    """Rotating topic list for --auto mode so consecutive days don't repeat."""
    return [
        "visiting a neighbor", "a trip to the market", "watching football with family",
        "a wedding celebration", "fasting during Ramadan", "a taxi ride conversation",
        "complaining about hot weather", "planning a trip to Alexandria",
        "a child's birthday party", "haggling at a souk", "a phone call with a cousin",
        "discussing the news over tea", "a visit to the doctor", "cooking together",
        "an argument about politics that stays friendly", "going to a wedding",
    ]


def call_claude_api(topic: str) -> dict:
    """Generates one day's content via the Claude API. Needs your own
    ANTHROPIC_API_KEY. Mirrors the style established in chat: intermediate
    Egyptian Arabic, first-person where natural, Greek/Chinese as *functional*
    equivalents (not literal translations) for idioms."""
    import anthropic

    client = anthropic.Anthropic()  # reads ANTHROPIC_API_KEY from env

    prompt = f"""Write a short intermediate-level Egyptian Arabic reading text
(6-8 sentences) about: {topic}.

Requirements:
- Egyptian Arabic dialect (Masri), not MSA.
- Mix of descriptive sentences and first-person opinion/feeling where natural
  (e.g. "ana ha-ssa inn...", "fi ra'yi...").
- For each sentence also give: a Greek translation and a Chinese translation.
  For any formulaic/religious expressions (inshallah, alhamdulillah, etc.),
  give the natural equivalent a Greek or Chinese speaker would actually say
  in that situation, NOT a literal translation.
- Then list 6-10 useful vocabulary items from the text: Arabic, transliteration,
  Greek, Chinese.

Return ONLY valid JSON matching this shape, no other text:
{{
  "title": "short English title",
  "reading": [
    {{"ar": "...", "gr": "...", "zh": "..."}}
  ],
  "vocab": [
    {{"ar": "...", "translit": "...", "gr": "...", "zh": "..."}}
  ]
}}
"""

    msg = client.messages.create(
        model="claude-sonnet-4-5",
        max_tokens=2000,
        messages=[{"role": "user", "content": prompt}],
    )
    text = msg.content[0].text
    # Be forgiving of stray markdown fences.
    text = text.strip().removeprefix("```json").removeprefix("```").removesuffix("```")
    return json.loads(text)


# --------------------------------------------------------------------- main

def build_day(data: dict, day_str: str):
    title = data.get("title", "")
    all_files = []  # (filename, image) pairs, written as-is

    total_pages = len(data["reading"])
    for i, line in enumerate(data["reading"]):
        img = render_reading_line(line["ar"], line["gr"], line["zh"],
                                   page_num=i + 1, page_total=total_pages)
        all_files.append((f"reading_{i+1:02d}.bmp", img))

    vocab_cards = []
    for i, v in enumerate(data["vocab"]):
        front = render_vocab_front(v["ar"])
        back = render_vocab_back(v["ar"], v["translit"], v["gr"], v["zh"])
        front_name = f"vocab_{i+1:02d}_front.bmp"
        back_name = f"vocab_{i+1:02d}_back.bmp"
        vocab_cards.append({"front": front_name, "back": back_name})
        all_files.append((front_name, front))
        all_files.append((back_name, back))

    home_img = render_home_page(title, len(data["reading"]), len(data["vocab"]), day_str)
    all_files.append(("home.bmp", home_img))

    manifest = {
        "date": day_str,
        "title": title,
        "home_page": "home.bmp",
        "reading_pages": [name for name, _ in all_files
                           if name.startswith("reading_")],
        "vocab_cards": vocab_cards,
    }

    for out_dir in (OUTPUT_LATEST, OUTPUT_HISTORY / day_str):
        if out_dir.exists():
            shutil.rmtree(out_dir)
        out_dir.mkdir(parents=True)
        for name, img in all_files:
            (out_dir / name).write_bytes(image_to_1bpp(img))
        (out_dir / "manifest.json").write_text(
            json.dumps(manifest, ensure_ascii=False, indent=2))

    print(f"Wrote day {day_str}: {len(data['reading'])} reading lines, "
          f"{len(data['vocab'])} vocab cards, home page -> {OUTPUT_LATEST}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--from-file", type=Path, help="JSON file with reading+vocab (see sample_day.json)")
    ap.add_argument("--auto", action="store_true", help="Generate via Claude API")
    ap.add_argument("--topic", help="Force a topic for --auto instead of rotating automatically")
    args = ap.parse_args()

    day_str = date.today().isoformat()

    if args.from_file:
        data = json.loads(args.from_file.read_text())
    elif args.auto:
        import random
        topic = args.topic or random.choice(default_topics())
        print(f"Generating via Claude API, topic: {topic}")
        data = call_claude_api(topic)
    else:
        sys.exit("Specify --from-file today.json or --auto")

    build_day(data, day_str)


if __name__ == "__main__":
    main()
