#!/usr/bin/env python3
"""
webbridge Auto-Discovery Scanner

Scannt C++ Header-Dateien und findet alle Klassen, die von webbridge::object erben.
Wird von CMake aufgerufen, um automatisch Klassen für die Registrierung zu entdecken.

Verwendung:
    python webbridge_discoverer.py header1.h header2.h ...

Ausgabe:
    Pro gefundene Klasse: filename|classname (eine pro Zeile)
    CMake kann diese mit COMMAND_OUTPUT_VARIABLE weiterverwenden

Inspiriert von Qt's MOC Auto-Discovery Mechanismus.
"""

import sys
from pathlib import Path
from typing import List
import tree_sitter_cpp as tscpp
from tree_sitter import Parser, Language


def find_webbridge_classes(header_file: str) -> List[str]:
    """
    Findet alle Klassen in einer Header-Datei, die von webbridge::object erben.

    Args:
        header_file: Pfad zur Header-Datei

    Returns:
        Liste der webbridge-Klassennamen
    """
    try:
        path = Path(header_file)
        if not path.exists() or not path.suffix.lower() in ['.h', '.hpp']:
            return []

        with open(path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()

        # Schneller String-Check (optimiert für CMake Performance)
        # Supports both old PascalCase and new snake_case naming
        if 'webbridge::Object' not in content and 'webbridge::object' not in content:
            return []

        # Falls String-Check positiv: Parse mit tree-sitter
        try:
            parser = Parser(Language(tscpp.language()))
            tree = parser.parse(content.encode('utf-8'))

            # Sammle alle Klassennamen
            class_names = []
            _find_class_names(tree.root_node, content, class_names)
            return class_names
        except Exception as e:
            print(f"# Parse-Fehler bei {header_file}: {e}", file=sys.stderr)
            return []

    except Exception as e:
        # Fehlertoleranz - Datei überspringen
        print(f"# Fehler bei {header_file}: {e}", file=sys.stderr)
        return []


def _find_class_names(node, content: str, class_names: List[str]):
    """
    Rekursive Suche nach Klassennamen die von webbridge::object erben

    Args:
        node: AST-Knoten
        content: Source-Code
        class_names: Liste zum Sammeln der Klassennamen
    """
    if node.type == 'class_specifier':
        class_name = None
        inherits_webbridge = False

        # Suche type_identifier (Klassenname) und base_class_clause
        for child in node.children:
            if child.type == 'type_identifier':
                class_name = content[child.start_byte:child.end_byte].strip()
            elif child.type == 'base_class_clause':
                base_text = content[child.start_byte:child.end_byte]
                # Supports both old PascalCase and new snake_case naming
                if 'webbridge::Object' in base_text or 'webbridge::object' in base_text or 'Object' in base_text or 'object' in base_text:
                    inherits_webbridge = True

        # Nur hinzufügen wenn Name und webbridge::object Vererbung vorhanden
        if class_name and inherits_webbridge:
            class_names.append(class_name)

    # Rekurse in Kinder-Knoten
    for child in node.children:
        _find_class_names(child, content, class_names)


def main():
    """
    Main Entry-Point für CMake-Integration.

    Gibt gefundene Klassen zeilenweise aus im Format: filename|classname
    Exit-Code: 0 bei Erfolg, 1 bei Fehler
    """
    if len(sys.argv) < 2:
        print("# Verwendung: webbridge_discoverer.py <header1.h> [header2.h] ...",
              file=sys.stderr)
        sys.exit(1)

    results: List[str] = []

    for header_file in sys.argv[1:]:
        class_names = find_webbridge_classes(header_file)
        for class_name in class_names:
            results.append(f"{header_file}|{class_name}")

    # Output für CMake (eine Zeile pro Klasse: filename|classname)
    if results:
        for result in results:
            print(result)
        sys.exit(0)
    else:
        sys.exit(1)


if __name__ == "__main__":
    main()
