#!/usr/bin/env python3
"""Genera los íconos propios de color de las herramientas (assets/icons/<nombre>-<tema>.svg).

Dibujo original de msepaint; solo se toma la idea de color de cada herramienta (lápiz amarillo, goma rosada...).
Uso: python3 scripts/gen-icons.py
"""
import pathlib

OUT = pathlib.Path(__file__).resolve().parent.parent / "assets" / "icons"
THEMES = {"light": "#1F1F1F", "dark": "#F2F2F2"}

ICONS = {
    "pencil": (16, """
<path d="M3 13 L4 10 L11 3 L13 5 L6 12 Z" fill="#EECE82" stroke="{S}" stroke-linejoin="round"/>
<path d="M3 13 L4 10 L6 12 Z" fill="#F7E9C9" stroke="{S}" stroke-linejoin="round"/>
<path d="M3 13 L3.5 11.6 L4.4 12.5 Z" fill="{S}"/>
<path d="M11 3 L12 2 C12.4 1.6 13 1.6 13.4 2 L14 2.6 C14.4 3 14.4 3.6 14 4 L13 5 Z" fill="#F6AEAC" stroke="{S}" stroke-linejoin="round"/>
"""),
    "fill": (16, """
<path d="M2.5 7.5 L7 3 L12 8 L7.5 12.5 C7 13 6.2 13 5.7 12.5 L2.5 9.3 C2 8.8 2 8 2.5 7.5 Z" fill="#7CC4F2" stroke="{S}" stroke-linejoin="round"/>
<path d="M3 7.6 L11.4 7.6" stroke="{S}"/>
<path d="M7 3 L5.2 1.2" stroke="{S}" stroke-linecap="round"/>
<path d="M13 9.2 C13 9.2 14.6 11.2 14.6 12.3 C14.6 13.2 13.9 13.9 13 13.9 C12.1 13.9 11.4 13.2 11.4 12.3 C11.4 11.2 13 9.2 13 9.2 Z" fill="#FF6327" stroke="{S}" stroke-linejoin="round"/>
"""),
    "text": (16, """
<path d="M2.8 14 L8 2 L13.2 14" fill="none" stroke="{S}" stroke-width="1.3" stroke-linejoin="round"/>
<path d="M4.9 9.5 L11.1 9.5" stroke="{S}" stroke-width="1.3"/>
"""),
    "eraser": (16, """
<path d="M1.9 10.6 L8.6 3.9 C9.1 3.4 9.9 3.4 10.4 3.9 L13.6 7.1 C14.1 7.6 14.1 8.4 13.6 8.9 L9 13.5 L5 13.5 L1.9 10.4 Z" fill="#F6AEAC" stroke="{S}" stroke-linejoin="round"/>
<path d="M5.3 7.2 L10.6 12.5" stroke="{S}"/>
<path d="M2 10.5 L5 13.5 L9 13.5" fill="#FDE3E2" stroke="{S}" stroke-linejoin="round"/>
<path d="M9 13.5 L14 13.5" stroke="{S}" stroke-linecap="round"/>
"""),
    "picker": (16, """
<path d="M2 14 L2.6 11.4 L8.4 5.6 L10.4 7.6 L4.6 13.4 Z" fill="#9CD3FF" stroke="{S}" stroke-linejoin="round"/>
<path d="M9.3 4.7 L11.3 2.7 C11.9 2.1 12.9 2.1 13.4 2.7 C14 3.3 14 4.2 13.4 4.8 L11.4 6.8 Z" fill="{S}"/>
<path d="M8.2 3.8 L12.2 7.8" stroke="{S}" stroke-linecap="round"/>
"""),
    "magnifier": (16, """
<circle cx="6.5" cy="6.5" r="4.6" fill="#D9EEFF" stroke="{S}"/>
<path d="M10 10 L14 14" stroke="{S}" stroke-width="2" stroke-linecap="round"/>
"""),
    "brush": (24, """
<path d="M20.5 2.5 C21.3 3.3 21.3 4.4 20.5 5.2 L13 12.7 L11.3 11 L18.8 3.5 C19.6 2.7 19.9 1.9 20.5 2.5 Z" fill="#C8956B" stroke="{S}" stroke-linejoin="round"/>
<path d="M11.3 11 L13 12.7 L11.6 14.1 L9.9 12.4 Z" fill="#B0B0B0" stroke="{S}" stroke-linejoin="round"/>
<path d="M9.9 12.4 L11.6 14.1 C11.6 17.5 8.6 20.5 3.5 20.5 C5 19 4.4 17.4 5.4 15.6 C6.3 14 8 12.4 9.9 12.4 Z" fill="#3A96DD" stroke="{S}" stroke-linejoin="round"/>
"""),
    "layers": (24, """
<path d="M12 3 L21 8 L12 13 L3 8 Z" fill="#7CC4F2" stroke="{S}" stroke-linejoin="round"/>
<path d="M5 11.5 L3 12.6 L12 17.6 L21 12.6 L19 11.5" fill="none" stroke="{S}" stroke-linejoin="round"/>
<path d="M5 16 L3 17.1 L12 22.1 L21 17.1 L19 16" fill="none" stroke="{S}" stroke-linejoin="round"/>
"""),
}


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    for name, (size, body) in ICONS.items():
        for theme, stroke in THEMES.items():
            svg = (f'<svg xmlns="http://www.w3.org/2000/svg" width="{size}" height="{size}" '
                   f'viewBox="0 0 {size} {size}">{body.replace("{S}", stroke)}</svg>\n')
            (OUT / f"{name}-{theme}.svg").write_text(svg)
    print(f"{len(ICONS) * len(THEMES)} íconos en {OUT}")


if __name__ == "__main__":
    main()
