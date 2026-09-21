from __future__ import annotations

import struct
from pathlib import Path

from PIL import Image, ImageDraw, ImageEnhance, ImageFilter, ImageFont


ROOT = Path(__file__).resolve().parents[1]
MENU_DIR = ROOT / "JTClientLibrary" / "media" / "clientlibrary" / "menu"
CODEX_IMAGES = Path.home() / ".codex" / "generated_images" / "019efa28-cfc9-7a13-a8d1-64d1d0affd9c"

PANEL_SOURCE = CODEX_IMAGES / "ig_00c67c50f78c0f65016a405d5cb99481918c8af32f035bd1d5.png"
BUTTON_SOURCE = CODEX_IMAGES / "ig_00c67c50f78c0f65016a405dc77ebc819180311df8649f65a6.png"

MENU_SIZE = (280, 560)
BUTTON_SIZE = (200, 32)
CLOSE_SIZE = (32, 32)

BUTTONS = [
    ("btn_grant_name", "Grant Name"),
    ("btn_titles", "Titles"),
    ("btn_icons", "Icons"),
    ("btn_ranking", "Ranking"),
    ("btn_unique_log", "Unique Log"),
    ("btn_event_register", "Event Register"),
    ("btn_schedule", "Schedule"),
    ("btn_achievements", "Achievements"),
    ("btn_updates", "Updates"),
    ("btn_alchemy", "Alchemy"),
    ("btn_settings", "Settings"),
]


def crop_content(img: Image.Image, threshold: int = 10) -> Image.Image:
    rgb = img.convert("RGB")
    px = rgb.load()
    w, h = rgb.size
    left, top, right, bottom = w, h, 0, 0

    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y]
            if max(r, g, b) > threshold:
                left = min(left, x)
                top = min(top, y)
                right = max(right, x)
                bottom = max(bottom, y)

    if right <= left or bottom <= top:
        return img

    return img.crop((left, top, right + 1, bottom + 1))


def cover_resize(img: Image.Image, size: tuple[int, int]) -> Image.Image:
    target_w, target_h = size
    src_w, src_h = img.size
    scale = max(target_w / float(src_w), target_h / float(src_h))
    resized = img.resize((int(src_w * scale), int(src_h * scale)), Image.LANCZOS)
    x = (resized.width - target_w) // 2
    y = (resized.height - target_h) // 2
    return resized.crop((x, y, x + target_w, y + target_h))


def save_ddj(img: Image.Image, path: Path) -> None:
    dds_path = path.with_suffix(".dds")
    img.convert("RGBA").save(dds_path)
    dds = dds_path.read_bytes()
    header = b"JMXVDDJ 1000" + struct.pack("<I", len(dds) + 8) + struct.pack("<I", 3)
    path.write_bytes(header + dds)
    try:
        dds_path.unlink()
    except OSError:
        pass


def load_font(size: int) -> ImageFont.FreeTypeFont:
    candidates = [
        Path("C:/Windows/Fonts/georgiab.ttf"),
        Path("C:/Windows/Fonts/Georgia.ttf"),
        Path("C:/Windows/Fonts/cambriaz.ttf"),
        Path("C:/Windows/Fonts/timesbd.ttf"),
    ]
    for font_path in candidates:
        if font_path.exists():
            return ImageFont.truetype(str(font_path), size)
    return ImageFont.load_default()


def fit_font(text: str, max_width: int, start_size: int) -> ImageFont.FreeTypeFont:
    size = start_size
    while size >= 12:
        font = load_font(size)
        bbox = ImageDraw.Draw(Image.new("RGBA", (1, 1))).textbbox((0, 0), text, font=font)
        if bbox[2] - bbox[0] <= max_width:
            return font
        size -= 1
    return load_font(12)


def text_button(base: Image.Image, label: str, state: str) -> Image.Image:
    img = cover_resize(crop_content(base), BUTTON_SIZE).convert("RGBA")

    if state == "pressed":
        img = ImageEnhance.Brightness(img).enhance(0.72)
        img = ImageEnhance.Contrast(img).enhance(1.18)
    elif state == "disabled":
        gray = img.convert("LA").convert("RGBA")
        img = Image.blend(gray, img, 0.18)
        img = ImageEnhance.Brightness(img).enhance(0.52)

    overlay = Image.new("RGBA", BUTTON_SIZE, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)

    font = fit_font(label, BUTTON_SIZE[0] - 34, 18)
    bbox = draw.textbbox((0, 0), label, font=font)
    text_w = bbox[2] - bbox[0]
    text_h = bbox[3] - bbox[1]
    x = (BUTTON_SIZE[0] - text_w) // 2
    y = (BUTTON_SIZE[1] - text_h) // 2 - 2

    if state == "disabled":
        fill = (148, 135, 108, 230)
        glow = (0, 0, 0, 160)
    else:
        fill = (255, 235, 176, 255)
        glow = (17, 44, 75, 205)

    for dx, dy in [(-2, 0), (2, 0), (0, -2), (0, 2), (-1, -1), (1, 1), (-1, 1), (1, -1)]:
        draw.text((x + dx, y + dy), label, font=font, fill=(0, 0, 0, 210))
    draw.text((x, y + 1), label, font=font, fill=glow)
    draw.text((x, y), label, font=font, fill=fill)

    return Image.alpha_composite(img, overlay)


def close_button(state: str) -> Image.Image:
    img = Image.new("RGBA", CLOSE_SIZE, (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    if state == "pressed":
        border = (170, 115, 30, 255)
        fill = (15, 20, 28, 240)
        x_color = (255, 210, 100, 255)
    else:
        border = (235, 185, 72, 255)
        fill = (7, 11, 18, 235)
        x_color = (255, 235, 150, 255)

    draw.ellipse((2, 2, 29, 29), fill=fill, outline=(0, 0, 0, 230), width=2)
    draw.ellipse((4, 4, 27, 27), outline=border, width=2)
    draw.line((10, 10, 22, 22), fill=(0, 0, 0, 210), width=5)
    draw.line((22, 10, 10, 22), fill=(0, 0, 0, 210), width=5)
    draw.line((10, 10, 22, 22), fill=x_color, width=3)
    draw.line((22, 10, 10, 22), fill=x_color, width=3)

    return img.filter(ImageFilter.UnsharpMask(radius=0.8, percent=130, threshold=2))


def main() -> None:
    MENU_DIR.mkdir(parents=True, exist_ok=True)

    panel_source = PANEL_SOURCE if PANEL_SOURCE.exists() else MENU_DIR / "menu_bg.png"
    button_source = BUTTON_SOURCE if BUTTON_SOURCE.exists() else MENU_DIR / "menu_button.png"

    panel = Image.open(panel_source).convert("RGBA")
    panel = cover_resize(crop_content(panel), MENU_SIZE)
    panel.save(MENU_DIR / "menu_bg.png")
    save_ddj(panel, MENU_DIR / "menu_bg.ddj")

    button_base = Image.open(button_source).convert("RGBA")
    for name, label in BUTTONS:
        for suffix, state in [("", "normal"), ("_pressed", "pressed"), ("_disabled", "disabled")]:
            image = text_button(button_base, label, state)
            image.save(MENU_DIR / f"{name}{suffix}.png")
            save_ddj(image, MENU_DIR / f"{name}{suffix}.ddj")

    for suffix, state in [("", "normal"), ("_pressed", "pressed")]:
        image = close_button(state)
        image.save(MENU_DIR / f"menu_close{suffix}.png")
        save_ddj(image, MENU_DIR / f"menu_close{suffix}.ddj")


if __name__ == "__main__":
    main()
