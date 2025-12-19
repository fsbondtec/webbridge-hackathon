#!/usr/bin/env python3
"""
webbridge Registration Generator

Generiert _registration.h Dateien aus explizit angegebenen C++ Klassen.
Nutzt tree-sitter für schnelles C++ AST-Parsing und Jinja2 für Template-Rendering.

Verwendung:
    python generate.py <input.h> --class-name <ClassName> --cpp_out=<dir> --ts_out=<dir>

Beispiel:
    python generate.py ../src/MyObject.h --class-name MyObject --cpp_out=../build/src
    python generate.py ../src/MyObject.h --class-name MyObject --ts_out=../frontend/src
    python generate.py ../src/MyObject.h --class-name MyObject --cpp_out=../build/src --ts_out=../frontend/src

Voraussetzungen:
    pip install tree-sitter tree-sitter-cpp jinja2

Bis C++26 Reflection verfügbar ist, dient dieses Script als Übergangslösung.
"""

import sys
import argparse
from pathlib import Path
from jinja2 import Environment, FileSystemLoader
from typescript_types import cpp_to_ts_type
from webbridge_parser import ClassInfo, parse_header


# =============================================================================
# Jinja2-Filter (Hilfsfunktionen)
# =============================================================================

def json_names(collection):
    """Formatiert eine Collection von Objekten als JSON-Array ihrer Namen.

    Args:
        collection: Liste von Objekten mit 'name'-Attribut

    Returns:
        String wie: {"method1", "method2", "method3"}
    """
    if not collection:
        return '{}'
    names = [getattr(obj, 'name') for obj in collection]
    json_strings = [f'"{name}"' for name in names]
    return '{' + ', '.join(json_strings) + '}'


# =============================================================================
# Globale Jinja2-Umgebung (initialisiert einmal)
# =============================================================================

def _setup_jinja_env() -> Environment:
    """Initialisiert die globale Jinja2-Umgebung mit allen Filtern."""
    template_dir = Path(__file__).parent / "templates"
    if not template_dir.exists():
        raise FileNotFoundError(f"Template-Verzeichnis nicht gefunden: {template_dir}")

    env = Environment(
        loader=FileSystemLoader(str(template_dir)),
        autoescape=False,
        trim_blocks=True,
        lstrip_blocks=True,
        keep_trailing_newline=True,
    )

    # Filter registrieren
    env.filters['ts_type'] = cpp_to_ts_type
    env.filters['json_names'] = json_names

    return env


_JINJA_ENV = _setup_jinja_env()


# =============================================================================
# Code-Generatoren (Funktionen)
# =============================================================================

def generate_registration(cls: ClassInfo, header_path: str) -> str:
    """Generiert die C++ _registration.h Datei."""
    try:
        template = _JINJA_ENV.get_template("registration.h.j2")
    except Exception as e:
        raise FileNotFoundError(f"Konnte Template 'registration.h.j2' nicht laden: {e}") from e

    return template.render(
        cls=cls,
        header_path=Path(header_path).name,
    )


def generate_typescript(cls: ClassInfo, header_path: str) -> str:
    """Generiert die TypeScript .d.ts Type Definitionen."""
    try:
        template = _JINJA_ENV.get_template("types.d.ts.j2")
    except Exception as e:
        raise FileNotFoundError(f"Konnte Template 'types.d.ts.j2' nicht laden: {e}") from e

    return template.render(
        cls=cls,
        header_path=Path(header_path).name,
    )


# =============================================================================
# Main
# =============================================================================

def main():
    """
    Hauptfunktion für die Registrierungs-Generierung.

    Args:
        input_path: Eingabe-Header-Datei (.h)
        class_name: Name der zu verarbeitenden Klasse
        cpp_out: Ausgabe-Ordner für C++ Registration (optional)
        ts_out: Ausgabe-Ordner für TypeScript Types (optional)

    Exit Codes:
        0: Erfolgreich
        1: Fehler bei der Ausführung
    """

    parser = argparse.ArgumentParser(
        description="Generiert webbridge Registration und/oder TypeScript Types",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument('input_path', help='Eingabe-Header-Datei (.h)')
    parser.add_argument('--class-name', required=True, help='Name der zu verarbeitenden Klasse')
    parser.add_argument('--cpp_out', type=str, help='Ausgabe-Ordner für C++ Registration Header')
    parser.add_argument('--ts_out', type=str, help='Ausgabe-Ordner für TypeScript Type Definitionen')

    args = parser.parse_args()

    input_path = args.input_path

    # Validierung
    if not Path(input_path).exists():
        print(f"Fehler: Datei nicht gefunden: {input_path}", file=sys.stderr)
        sys.exit(1)

    if not args.cpp_out and not args.ts_out:
        print("Fehler: Mindestens --cpp_out oder --ts_out muss angegeben werden", file=sys.stderr)
        sys.exit(1)

    print(f"Parsing: {input_path} -> {args.class_name}")

    try:
        cls = parse_header(input_path, args.class_name)
    except ImportError as e:
        print(f"Fehler: webbridge_parser nicht verfügbar: {e}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Fehler beim Parsen: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        sys.exit(1)

    if not cls:
        print(f"Fehler: Klasse '{args.class_name}' nicht gefunden.", file=sys.stderr)
        sys.exit(1)

    print(f"[OK] Klasse gefunden: {cls.name}")
    print(f"  - Properties: {len(cls.properties)} {[p.name for p in cls.properties]}")
    print(f"  - Events: {len(cls.events)} {[e.name for e in cls.events]}")
    print(f"  - Sync Methods: {len(cls.sync_methods)} {[m.name for m in cls.sync_methods]}")
    print(f"  - Async Methods: {len(cls.async_methods)} {[m.name for m in cls.async_methods]}")

    try:
        # C++ Registration generieren (falls --cpp_out angegeben)
        if args.cpp_out:
            cpp_out_path = Path(args.cpp_out)
            cpp_out_path.mkdir(parents=True, exist_ok=True)
            reg_output = cpp_out_path / f"{cls.name}_registration.h"
            reg_code = generate_registration(cls, input_path)
            with open(reg_output, 'w', encoding='utf-8') as f:
                f.write(reg_code)
            print(f"  [OK] Generiert: {reg_output}")

        # TypeScript Type Definitionen generieren (falls --ts_out angegeben)
        if args.ts_out:
            ts_out_path = Path(args.ts_out)
            ts_out_path.mkdir(parents=True, exist_ok=True)
            ts_output = ts_out_path / f"{cls.name}.types.d.ts"
            ts_code = generate_typescript(cls, input_path)
            with open(ts_output, 'w', encoding='utf-8') as f:
                f.write(ts_code)
            print(f"  [OK] Generiert: {ts_output}")

    except Exception as e:
        print(f"  [ERROR] Fehler bei {cls.name}: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
