from __future__ import annotations

import io
import math
import struct
from pathlib import Path

from PIL import Image, ImageDraw, ImageEnhance, ImageFilter, ImageFont


ROOT = Path(__file__).resolve().parents[1]
ART_DIR = ROOT / "JTClientLibrary" / "artwork" / "kmt-guide-icons"
PREVIEW_DIR = ART_DIR / "game-ready"
GUIDE_DIR = ROOT / "JTClientLibrary" / "clientlibrary" / "guides"
ICON_DIR = ROOT / "JTClientLibrary" / "clientlibrary" / "icon"
SIZE = (40, 40)
SCALE = 4

GOLD = (255, 222, 48, 255)
GOLD_DARK = (171, 94, 15, 255)
ORANGE = (245, 139, 22, 255)
IVORY = (255, 232, 177, 255)
RED = (222, 31, 18, 255)
RED_DARK = (102, 8, 4, 255)
CYAN = (66, 220, 223, 255)
INK = (12, 12, 12, 255)
SHADOW = (0, 0, 0, 210)

OUTPUTS = {
    "menu": ("kmt_menu", ICON_DIR),
    "special_offers": ("kmt_special_offers", GUIDE_DIR),
    "drop_logs": ("kmt_drop_logs", GUIDE_DIR),
    "pvp_challenge": ("kmt_pvp_challenge", GUIDE_DIR),
    "killer_animation": ("kmt_killer_animation", GUIDE_DIR),
    "lucky_spin": ("kmt_lucky_spin", GUIDE_DIR),
    "auto_equip": ("kmt_auto_equip", GUIDE_DIR),
    "web_viewer": ("kmt_web_viewer", GUIDE_DIR),
}


def s(value: int) -> int:
    return value * SCALE


def p(points: list[tuple[int, int]]) -> list[tuple[int, int]]:
    return [(s(x), s(y)) for x, y in points]


def canvas() -> Image.Image:
    return Image.new("RGBA", (s(40), s(40)), (0, 0, 0, 0))


def rounded(draw: ImageDraw.ImageDraw, xy: tuple[int, int, int, int], radius: int, fill, outline=None, width: int = 1) -> None:
    box = tuple(s(v) for v in xy)
    draw.rounded_rectangle(box, radius=s(radius), fill=fill, outline=outline, width=s(width))


def line(draw: ImageDraw.ImageDraw, points: list[tuple[int, int]], fill, width: int = 1, joint: str = "curve") -> None:
    draw.line(p(points), fill=fill, width=s(width), joint=joint)


def frame() -> Image.Image:
    img = canvas()
    d = ImageDraw.Draw(img)
    rounded(d, (2, 2, 37, 37), 8, (24, 24, 20, 255), GOLD, 2)
    rounded(d, (5, 5, 34, 34), 5, (45, 25, 12, 255), ORANGE, 1)
    rounded(d, (7, 7, 32, 32), 4, (16, 16, 17, 255), (65, 65, 58, 255), 1)
    d.rectangle((s(9), s(8), s(31), s(20)), fill=(29, 29, 30, 255))
    d.rectangle((s(9), s(21), s(31), s(31)), fill=(24, 13, 9, 255))
    return img


def add_glow(img: Image.Image, color=(255, 214, 36, 255), radius: float = 1.15) -> Image.Image:
    alpha = img.getchannel("A").filter(ImageFilter.GaussianBlur(radius * SCALE))
    alpha = alpha.point(lambda value: min(160, int(value * 0.55)))
    glow = Image.new("RGBA", img.size, color[:3] + (0,))
    glow.putalpha(alpha)
    out = Image.new("RGBA", img.size, (0, 0, 0, 0))
    out.alpha_composite(glow)
    out.alpha_composite(img)
    return out


def symbol_layer(kind: str) -> Image.Image:
    img = canvas()
    d = ImageDraw.Draw(img)

    if kind == "menu":
        d.ellipse((s(15), s(8), s(25), s(18)), outline=GOLD, width=s(2))
        line(d, [(20, 17), (20, 30)], IVORY, 3)
        line(d, [(13, 20), (27, 20)], IVORY, 3)
        line(d, [(15, 30), (20, 26), (25, 30)], RED, 3)
        line(d, [(13, 10), (20, 5), (27, 10)], GOLD_DARK, 2)

    elif kind == "special_offers":
        rounded(d, (11, 17, 29, 29), 2, RED_DARK, GOLD, 1)
        d.rectangle((s(10), s(14), s(30), s(19)), fill=RED, outline=GOLD)
        d.rectangle((s(18), s(13), s(22), s(30)), fill=GOLD)
        line(d, [(14, 13), (18, 10), (20, 14), (22, 10), (26, 13)], IVORY, 2)

    elif kind == "drop_logs":
        rounded(d, (12, 9, 28, 29), 2, (128, 79, 35, 255), IVORY, 1)
        line(d, [(16, 14), (25, 14)], INK, 1)
        line(d, [(16, 19), (25, 19)], INK, 1)
        line(d, [(16, 24), (22, 24)], INK, 1)
        line(d, [(20, 29), (20, 34), (16, 30)], CYAN, 2)
        line(d, [(20, 34), (24, 30)], CYAN, 2)

    elif kind == "pvp_challenge":
        d.polygon(p([(20, 10), (29, 14), (27, 25), (20, 31), (13, 25), (11, 14)]), fill=RED_DARK, outline=GOLD)
        line(d, [(11, 29), (29, 11)], IVORY, 2)
        line(d, [(29, 29), (11, 11)], IVORY, 2)
        line(d, [(14, 26), (11, 29)], GOLD, 2)
        line(d, [(26, 26), (29, 29)], GOLD, 2)

    elif kind == "killer_animation":
        d.ellipse((s(12), s(10), s(28), s(27)), fill=(214, 203, 181, 255), outline=GOLD_DARK, width=s(1))
        d.ellipse((s(15), s(16), s(18), s(19)), fill=INK)
        d.ellipse((s(22), s(16), s(25), s(19)), fill=INK)
        line(d, [(16, 24), (20, 26), (24, 24)], INK, 1)
        line(d, [(9, 30), (30, 9)], RED, 3)
        line(d, [(10, 32), (31, 11)], GOLD, 1)

    elif kind == "lucky_spin":
        d.ellipse((s(10), s(10), s(30), s(30)), fill=(34, 34, 36, 255), outline=GOLD, width=s(2))
        d.pieslice((s(12), s(12), s(28), s(28)), 270, 30, fill=RED)
        d.pieslice((s(12), s(12), s(28), s(28)), 30, 150, fill=GOLD_DARK)
        d.pieslice((s(12), s(12), s(28), s(28)), 150, 270, fill=CYAN)
        d.ellipse((s(17), s(17), s(23), s(23)), fill=IVORY, outline=INK)
        d.polygon(p([(20, 6), (16, 12), (24, 12)]), fill=GOLD, outline=INK)

    elif kind == "auto_equip":
        d.polygon(p([(14, 10), (18, 14), (22, 14), (26, 10), (29, 27), (24, 30), (16, 30), (11, 27)]), fill=(86, 88, 91, 255), outline=GOLD)
        d.polygon(p([(17, 15), (23, 15), (25, 26), (15, 26)]), fill=(42, 42, 44, 255), outline=IVORY)
        line(d, [(10, 18), (8, 14), (12, 12)], CYAN, 2)
        line(d, [(30, 22), (32, 26), (28, 28)], CYAN, 2)

    elif kind == "web_viewer":
        rounded(d, (10, 11, 30, 27), 2, (32, 42, 48, 255), GOLD, 1)
        d.rectangle((s(11), s(12), s(29), s(16)), fill=(72, 42, 24, 255))
        d.ellipse((s(15), s(17), s(25), s(27)), outline=CYAN, width=s(1))
        line(d, [(20, 17), (20, 27)], CYAN, 1)
        line(d, [(15, 22), (25, 22)], CYAN, 1)
        d.polygon(p([(25, 24), (32, 29), (28, 30), (30, 34), (27, 35), (25, 31)]), fill=IVORY, outline=INK)

    return img


def downsample(img: Image.Image) -> Image.Image:
    return img.resize(SIZE, Image.Resampling.LANCZOS).filter(
        ImageFilter.UnsharpMask(radius=0.45, percent=125, threshold=1)
    )


def make_icon(kind: str, hover: bool = False) -> Image.Image:
    base = frame()
    symbol = symbol_layer(kind)
    shadow = Image.new("RGBA", base.size, (0, 0, 0, 0))
    shadow.alpha_composite(symbol.filter(ImageFilter.GaussianBlur(0.5 * SCALE)), (s(1), s(1)))
    base.alpha_composite(shadow)
    base.alpha_composite(add_glow(symbol, radius=0.55 if hover else 0.25))
    icon = downsample(base)

    if hover:
        halo = icon.getchannel("A").filter(ImageFilter.GaussianBlur(1.1))
        halo = halo.point(lambda value: min(130, int(value * 0.50)))
        glow = Image.new("RGBA", SIZE, (255, 218, 45, 0))
        glow.putalpha(halo)
        out = Image.new("RGBA", SIZE, (0, 0, 0, 0))
        out.alpha_composite(glow)
        out.alpha_composite(ImageEnhance.Brightness(icon).enhance(1.16))
        return ImageEnhance.Contrast(out).enhance(1.06)

    return icon


def save_ddj(image: Image.Image, path: Path) -> None:
    buffer = io.BytesIO()
    image.convert("RGBA").save(buffer, format="DDS")
    dds = buffer.getvalue()
    header = b"JMXVDDJ 1000" + struct.pack("<I", len(dds) + 8) + struct.pack("<I", 3)
    path.write_bytes(header + dds)


def validate_ddj(path: Path) -> None:
    data = path.read_bytes()
    if not data.startswith(b"JMXVDDJ 1000"):
        raise ValueError(f"{path.name}: invalid DDJ signature")
    if struct.unpack_from("<I", data, 12)[0] != len(data) - 12:
        raise ValueError(f"{path.name}: invalid DDJ payload length")
    image = Image.open(io.BytesIO(data[20:]))
    if image.size != SIZE:
        raise ValueError(f"{path.name}: expected {SIZE}, got {image.size}")


def save_legacy_menu_copies(normal: Image.Image, hover: Image.Image) -> None:
    for folder in (ICON_DIR, GUIDE_DIR):
        save_ddj(normal, folder / "vfilterguide.ddj")
        save_ddj(hover, folder / "vfilterguide1.ddj")
        validate_ddj(folder / "vfilterguide.ddj")
        validate_ddj(folder / "vfilterguide1.ddj")


def build_contact_sheet(images: list[tuple[str, Image.Image, Image.Image]]) -> None:
    columns = 4
    cell_width = 160
    cell_height = 116
    rows = math.ceil(len(images) / columns)
    sheet = Image.new("RGBA", (columns * cell_width, rows * cell_height), (27, 27, 27, 255))
    draw = ImageDraw.Draw(sheet)
    font = ImageFont.load_default()

    for index, (label, normal, hover) in enumerate(images):
        col = index % columns
        row = index // columns
        left = col * cell_width
        top = row * cell_height
        sheet.alpha_composite(normal.resize((80, 80), Image.Resampling.NEAREST), (left + 8, top + 8))
        sheet.alpha_composite(hover.resize((80, 80), Image.Resampling.NEAREST), (left + 78, top + 8))
        draw.text((left + 8, top + 92), label.replace("_", " "), fill=(236, 224, 190, 255), font=font)

    sheet.save(ART_DIR / "kmt-guide-icons-preview.png")


def main() -> None:
    PREVIEW_DIR.mkdir(parents=True, exist_ok=True)
    GUIDE_DIR.mkdir(parents=True, exist_ok=True)
    ICON_DIR.mkdir(parents=True, exist_ok=True)
    preview_images: list[tuple[str, Image.Image, Image.Image]] = []

    for kind, (output_name, output_dir) in OUTPUTS.items():
        normal = make_icon(kind, hover=False)
        hover = make_icon(kind, hover=True)
        normal_png = PREVIEW_DIR / f"{output_name}_1.png"
        hover_png = PREVIEW_DIR / f"{output_name}_2.png"
        normal_ddj = output_dir / f"{output_name}_1.ddj"
        hover_ddj = output_dir / f"{output_name}_2.ddj"

        normal.save(normal_png)
        hover.save(hover_png)
        save_ddj(normal, normal_ddj)
        save_ddj(hover, hover_ddj)
        validate_ddj(normal_ddj)
        validate_ddj(hover_ddj)
        preview_images.append((kind, normal, hover))

        if kind == "menu":
            save_legacy_menu_copies(normal, hover)

    build_contact_sheet(preview_images)
    print(f"Generated {len(OUTPUTS) * 2} DDJ icons plus legacy menu copies")


if __name__ == "__main__":
    main()
