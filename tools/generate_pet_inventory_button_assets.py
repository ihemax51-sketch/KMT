from __future__ import annotations

import struct
from pathlib import Path

from PIL import Image, ImageDraw, ImageEnhance, ImageFilter


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "JTClientLibrary" / "clientlibrary" / "fellowpets"
SIZE = (40, 32)


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


def rounded_rect(draw: ImageDraw.ImageDraw, box: tuple[int, int, int, int], radius: int, **kw) -> None:
    draw.rounded_rectangle(box, radius=radius, **kw)


def base_button(state: str) -> tuple[Image.Image, ImageDraw.ImageDraw]:
    img = Image.new("RGBA", SIZE, (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    if state == "pressed":
        outer = (102, 70, 31, 255)
        rim = (193, 129, 42, 255)
        inner = (15, 21, 27, 246)
        glow = (117, 76, 18, 105)
    elif state == "focus":
        outer = (38, 28, 19, 255)
        rim = (255, 204, 93, 255)
        inner = (21, 32, 39, 248)
        glow = (255, 207, 89, 125)
    else:
        outer = (29, 24, 19, 255)
        rim = (210, 157, 65, 255)
        inner = (18, 26, 32, 246)
        glow = (160, 104, 30, 85)

    rounded_rect(draw, (1, 2, 38, 29), 5, fill=(4, 6, 8, 190))
    rounded_rect(draw, (1, 1, 38, 28), 5, fill=outer)
    rounded_rect(draw, (3, 3, 36, 26), 4, outline=rim, width=2)
    rounded_rect(draw, (5, 6, 34, 24), 3, fill=inner)
    draw.line((7, 7, 32, 7), fill=(255, 234, 151, 62), width=1)
    draw.line((7, 24, 32, 24), fill=(0, 0, 0, 105), width=1)

    glow_layer = Image.new("RGBA", SIZE, (0, 0, 0, 0))
    glow_draw = ImageDraw.Draw(glow_layer)
    rounded_rect(glow_draw, (8, 7, 31, 24), 7, fill=glow)
    img.alpha_composite(glow_layer.filter(ImageFilter.GaussianBlur(4)))
    return img, ImageDraw.Draw(img)


def draw_slot(draw: ImageDraw.ImageDraw, box: tuple[int, int, int, int], fill: tuple[int, int, int, int]) -> None:
    x1, y1, x2, y2 = box
    draw.rectangle((x1, y1, x2, y2), fill=(8, 11, 15, 230), outline=(0, 0, 0, 220))
    draw.rectangle((x1 + 1, y1 + 1, x2 - 1, y2 - 1), outline=(141, 111, 63, 230))
    draw.rectangle((x1 + 2, y1 + 2, x2 - 2, y2 - 2), fill=fill)
    draw.point((x1 + 2, y1 + 2), fill=(255, 255, 210, 190))


def draw_arrow(draw: ImageDraw.ImageDraw, points: list[tuple[int, int]], color: tuple[int, int, int, int]) -> None:
    shadow = [(x + 1, y + 1) for x, y in points]
    draw.line(shadow, fill=(0, 0, 0, 190), width=3, joint="curve")
    draw.line(points, fill=color, width=3, joint="curve")


def draw_arrow_head(draw: ImageDraw.ImageDraw, pts: tuple[tuple[int, int], tuple[int, int], tuple[int, int]], color: tuple[int, int, int, int]) -> None:
    shadow = tuple((x + 1, y + 1) for x, y in pts)
    draw.polygon(shadow, fill=(0, 0, 0, 190))
    draw.polygon(pts, fill=color)


def auto_sort_icon(state: str) -> Image.Image:
    img, draw = base_button(state)
    blue = (70, 158, 205, 255) if state != "pressed" else (53, 118, 158, 255)
    gold = (242, 193, 88, 255) if state != "pressed" else (181, 129, 52, 255)
    teal = (83, 205, 169, 255) if state != "pressed" else (54, 143, 122, 255)
    arrow = (255, 220, 107, 255) if state != "pressed" else (203, 151, 56, 255)

    draw_slot(draw, (10, 8, 16, 14), blue)
    draw_slot(draw, (18, 8, 24, 14), gold)
    draw_slot(draw, (10, 16, 16, 22), teal)
    draw_slot(draw, (18, 16, 24, 22), (126, 108, 205, 255))
    draw_arrow(draw, [(28, 9), (31, 12), (31, 19), (28, 22)], arrow)
    draw_arrow_head(draw, ((27, 22), (31, 25), (33, 20)), arrow)

    img = ImageEnhance.Sharpness(img).enhance(1.25)
    return img


def convert_icon(state: str) -> Image.Image:
    img, draw = base_button(state)
    sack = (112, 81, 49, 255) if state != "pressed" else (79, 58, 39, 255)
    strap = (218, 166, 80, 255) if state != "pressed" else (159, 113, 50, 255)
    arrow = (91, 216, 231, 255) if state != "pressed" else (57, 149, 168, 255)
    inv = (58, 128, 189, 255) if state != "pressed" else (44, 91, 133, 255)

    rounded_rect(draw, (8, 11, 18, 22), 3, fill=(0, 0, 0, 155))
    rounded_rect(draw, (7, 10, 17, 21), 3, fill=sack, outline=(31, 19, 10, 255), width=1)
    draw.arc((9, 7, 15, 14), 190, 350, fill=strap, width=2)
    draw.ellipse((10, 13, 12, 15), fill=(236, 203, 112, 235))
    draw.ellipse((14, 13, 16, 15), fill=(236, 203, 112, 235))

    draw_arrow(draw, [(18, 16), (23, 16), (25, 16)], arrow)
    draw_arrow_head(draw, ((25, 12), (31, 16), (25, 20)), arrow)

    rounded_rect(draw, (27, 9, 33, 22), 2, fill=(0, 0, 0, 165))
    rounded_rect(draw, (26, 8, 32, 21), 2, fill=inv, outline=(178, 215, 236, 210), width=1)
    draw.line((27, 11, 31, 11), fill=(230, 248, 255, 180), width=1)
    draw.line((27, 15, 31, 15), fill=(16, 56, 86, 155), width=1)

    img = ImageEnhance.Sharpness(img).enhance(1.25)
    return img


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    assets = {
        "pet_auto_sort_button": auto_sort_icon,
        "pet_convert_button": convert_icon,
    }
    states = [("", "normal"), ("_focus", "focus"), ("_pressed", "pressed")]

    for name, factory in assets.items():
        for suffix, state in states:
            image = factory(state)
            image.save(OUT_DIR / f"{name}{suffix}.png")
            save_ddj(image, OUT_DIR / f"{name}{suffix}.ddj")


if __name__ == "__main__":
    main()
