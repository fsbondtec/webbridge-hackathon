#!/usr/bin/env python3
"""
webbridge Parser Module

Tree-sitter based parser for extracting class information from C++ headers.
Extracts properties, events, and methods from explicitly specified classes.

Voraussetzungen:
    pip install tree-sitter tree-sitter-cpp
"""

import sys
from pathlib import Path
from dataclasses import dataclass, field
from typing import List, Optional, Tuple
import tree_sitter_cpp as tscpp
from tree_sitter import Language, Parser, Node


# =============================================================================
# Data Classes
# =============================================================================

@dataclass
class PropertyInfo:
    """Information über eine Property<T>"""
    name: str
    type_name: str


@dataclass
class EventInfo:
    """Information über ein Event<Args...>"""
    name: str
    arg_types: List[str]


@dataclass
class ConstInfo:
    """Information über eine Konstante"""
    name: str
    type_name: str
    is_static: bool


@dataclass
class EnumInfo:
    """Information über ein Enum"""
    name: str
    enum_values: List[str]
    is_enum_class: bool


@dataclass
class MethodInfo:
    """Information über eine Methode"""
    name: str
    return_type: str
    parameters: List[Tuple[str, str]]
    is_async: bool = False


@dataclass
class ClassInfo:
    """Gesammelte Information über eine C++-Klasse"""
    name: str
    properties: List[PropertyInfo] = field(default_factory=list)
    events: List[EventInfo] = field(default_factory=list)
    constants: List[ConstInfo] = field(default_factory=list)
    enums: List[EnumInfo] = field(default_factory=list)
    constructors: List[MethodInfo] = field(default_factory=list)
    sync_methods: List[MethodInfo] = field(default_factory=list)
    async_methods: List[MethodInfo] = field(default_factory=list)


# =============================================================================
# Helper Functions
# =============================================================================

def _get_node_text(source_code: bytes, node: Node) -> str:
    """Extrahiert den Text eines Nodes"""
    return source_code[node.start_byte:node.end_byte].decode('utf-8')


def _normalize_type(source_code: bytes, node: Node) -> str:
    """Extrahiert Typ-Text normalisiert mit korrekten Trennzeichen"""
    if node.type == 'comment' or not node:
        return ''

    if not node.children:
        return _get_node_text(source_code, node).strip()

    parts = []
    prev_needs_space = False

    for child in node.children:
        if child.type in ('comment', 'initializer_list', 'argument_list'):
            continue
        if _get_node_text(source_code, child) == '{':
            break

        text = _normalize_type(source_code, child)
        if text:
            if parts and prev_needs_space and text[0].isalnum():
                parts.append(' ')
            parts.append(text)
            prev_needs_space = text[-1].isalnum()

    return ''.join(parts)


def _find_child_by_type(node: Node, *types) -> Optional[Node]:
    """Findet das erste Kind mit einem der angegebenen Typen"""
    return next((child for child in node.children if child.type in types), None)


def _extract_template_info(source_code: bytes, type_node: Node) -> Tuple[Optional[str], Optional[Node]]:
    """Extrahiert Template-Name und Template-Argumente"""
    template_name_node = _find_child_by_type(type_node, 'type_identifier')
    template_args = _find_child_by_type(type_node, 'template_argument_list')

    template_name = _get_node_text(source_code, template_name_node) if template_name_node else None
    return template_name, template_args


def _parse_parameters(source_code: bytes, param_list: Node) -> List[Tuple[str, str]]:
    """Parst Parameter-Liste durch AST-Traversal"""
    parameters = []

    for child in param_list.children:
        if child.type != 'parameter_declaration':
            continue

        # Extrahiere Type
        type_node = _find_child_by_type(child, 'primitive_type', 'type_identifier', 'qualified_identifier', 'template_type')
        param_type = _normalize_type(source_code, type_node) if type_node else 'unknown'

        # Extrahiere Name
        param_name = 'arg'
        for node_type in ('identifier', 'reference_declarator', 'pointer_declarator'):
            name_node = _find_child_by_type(child, node_type)
            if name_node:
                if node_type == 'identifier':
                    param_name = _get_node_text(source_code, name_node)
                else:
                    # Für reference/pointer: suche Identifier in Kindern
                    identifier = _find_child_by_type(name_node, 'identifier')
                    if identifier:
                        param_name = _get_node_text(source_code, identifier)
                break

        if param_type != 'unknown':
            parameters.append((param_type, param_name))

    return parameters


def _extract_all_members(source_code: bytes, class_body: Node, class_info: ClassInfo):
    """Extrahiert alle Members (Properties, Events, Methods) in einem Durchlauf"""

    current_access = 'private'  # Default access in class ist private

    def process_field(node: Node, access: str):
        if node.type not in ('field_declaration', 'function_definition', 'enum_specifier'):
            return

        # Ignoriere nicht-public Members
        if access != 'public':
            return

        # Behandle enum_specifier
        if node.type == 'enum_specifier':
            # Prüfe ob es enum class ist
            is_enum_class = False
            enum_name = None
            enumerator_list = None

            for child in node.children:
                text = _get_node_text(source_code, child)
                if text == 'class':
                    is_enum_class = True
                elif child.type == 'type_identifier':
                    enum_name = text
                elif child.type == 'enumerator_list':
                    enumerator_list = child

            # Extrahiere enum values
            enum_values = []
            if enumerator_list:
                for child in enumerator_list.children:
                    if child.type == 'enumerator':
                        # Hole nur den Namen, nicht den Wert
                        identifier = _find_child_by_type(child, 'identifier')
                        if identifier:
                            enum_values.append(_get_node_text(source_code, identifier))

            # Wenn kein Name, dann ist es anonym
            if not enum_name:
                enum_name = '<anonymous>'

            class_info.enums.append(EnumInfo(
                name=enum_name,
                enum_values=enum_values,
                is_enum_class=is_enum_class
            ))
            return

        # Für function_definition: andere Struktur
        if node.type == 'function_definition':
            func_declarator = _find_child_by_type(node, 'function_declarator')
            if not func_declarator:
                return

            method_name_node = _find_child_by_type(func_declarator, 'field_identifier', 'identifier')
            params_node = _find_child_by_type(func_declarator, 'parameter_list')

            if method_name_node and params_node:
                method_name = _get_node_text(source_code, method_name_node)

                # Skip Destruktoren und Operatoren
                if method_name.startswith(('~', 'operator')):
                    return

                # Prüfe ob es ein Konstruktor ist
                if method_name == class_info.name:
                    method_info = MethodInfo(
                        name=method_name,
                        return_type='',  # Konstruktoren haben keinen Return-Type
                        parameters=_parse_parameters(source_code, params_node),
                        is_async=False
                    )
                    class_info.constructors.append(method_info)
                    return

                # Return-Type ist erstes Kind vom node (vor function_declarator)
                return_type_node = next((child for child in node.children
                                       if child.type not in ('function_declarator', 'compound_statement', 'type_qualifier')), None)

                method_info = MethodInfo(
                    name=method_name,
                    return_type=_normalize_type(source_code, return_type_node) if return_type_node else 'void',
                    parameters=_parse_parameters(source_code, params_node),
                    is_async=False  # inline methods are never async
                )

                class_info.sync_methods.append(method_info)
            return

        # Für field_declaration: wie bisher
        # Prüfe zuerst ob es eine Methoden-Deklaration ist
        func_declarator = _find_child_by_type(node, 'function_declarator')

        # Wenn function_declarator existiert, ist es eine Methode
        if func_declarator:
            method_name_node = _find_child_by_type(func_declarator, 'field_identifier')
            params_node = _find_child_by_type(func_declarator, 'parameter_list')

            if method_name_node and params_node:
                method_name = _get_node_text(source_code, method_name_node)

                # Skip Destruktoren und Operatoren
                if method_name.startswith(('~', 'operator')):
                    return

                # Prüfe ob es ein Konstruktor ist
                if method_name == class_info.name:
                    method_info = MethodInfo(
                        name=method_name,
                        return_type='',  # Konstruktoren haben keinen Return-Type
                        parameters=_parse_parameters(source_code, params_node),
                        is_async=False
                    )
                    class_info.constructors.append(method_info)
                    return

                # Finde Return-Type (erstes child das kein attribute/identifier/function_declarator ist)
                return_type_node = next((child for child in node.children
                                         if child.type not in ('attribute_declaration', 'field_identifier', 'function_declarator', ';')), None)

                # Prüfe auf async
                is_async = any('async' in _get_node_text(source_code, child)
                               for child in node.children if child.type == 'attribute_declaration')

                method_info = MethodInfo(
                    name=method_name,
                    return_type=_normalize_type(source_code, return_type_node) if return_type_node else 'void',
                    parameters=_parse_parameters(source_code, params_node),
                    is_async=is_async
                )

                (class_info.async_methods if is_async else class_info.sync_methods).append(method_info)
            return

        # Sonst ist es Property, Event oder Konstante
        type_node = _find_child_by_type(node, 'template_type')
        declarator_node = _find_child_by_type(node, 'field_identifier')

        # Wenn es ein template_type ist, prüfe auf Property/Event
        if type_node and declarator_node:
            template_name, template_args = _extract_template_info(source_code, type_node)
            member_name = _get_node_text(source_code, declarator_node)

            # Property oder Event?
            if template_name == 'Property':
                prop_type = 'unknown'
                if template_args:
                    type_desc = _find_child_by_type(template_args, 'type_descriptor')
                    if type_desc:
                        prop_type = _normalize_type(source_code, type_desc)
                class_info.properties.append(PropertyInfo(name=member_name, type_name=prop_type))
                return

            elif template_name == 'Event':
                arg_types = []
                if template_args:
                    arg_types = [_normalize_type(source_code, child)
                               for child in template_args.children
                               if child.type == 'type_descriptor']
                class_info.events.append(EventInfo(name=member_name, arg_types=arg_types))
                return
        
        # Wenn nicht Property/Event, prüfe ob es eine Konstante ist
        if declarator_node:
            has_const = False
            is_static = False
            actual_type_node = None
            
            for child in node.children:
                text = _get_node_text(source_code, child)
                if text == 'const' or text == 'constexpr':
                    has_const = True
                elif child.type == 'type_qualifier' and ('const' in text or 'constexpr' in text):
                    has_const = True
                elif text == 'static':
                    is_static = True
                elif child.type in ('primitive_type', 'type_identifier', 'qualified_identifier', 'sized_type_specifier'):
                    actual_type_node = child
            
            if has_const and actual_type_node and declarator_node:
                const_type = _normalize_type(source_code, actual_type_node)
                const_name = _get_node_text(source_code, declarator_node)
                class_info.constants.append(ConstInfo(
                    name=const_name,
                    type_name=const_type,
                    is_static=is_static
                ))

    # Traversierung mit Access-Tracking
    def walk(node: Node):
        nonlocal current_access

        # Prüfe auf access_specifier
        if node.type == 'access_specifier':
            access_text = _get_node_text(source_code, node)
            if 'public' in access_text:
                current_access = 'public'
            elif 'private' in access_text:
                current_access = 'private'
            elif 'protected' in access_text:
                current_access = 'protected'
        else:
            process_field(node, current_access)

        for child in node.children:
            walk(child)

    walk(class_body)


def _parse_class(source_code: bytes, node: Node, target_class_name: str) -> Optional[ClassInfo]:
    """Parst eine Klassen-Definition

    Args:
        source_code: Der Quellcode als bytes
        node: Der AST-Node der Klasse
        target_class_name: Name der zu parsenden Klasse
    """
    class_name = None
    class_body = None

    for child in node.children:
        if child.type == 'type_identifier':
            class_name = _get_node_text(source_code, child)
        elif child.type == 'field_declaration_list':
            class_body = child

    if not (class_name and class_body):
        return None

    # Filtere nach Namen
    if class_name != target_class_name:
        return None

    class_info = ClassInfo(name=class_name)
    _extract_all_members(source_code, class_body, class_info)
    
    # Wenn keine Konstruktoren gefunden wurden, füge Default-Konstruktor hinzu
    if not class_info.constructors:
        class_info.constructors.append(MethodInfo(
            name=class_name,
            return_type='',
            parameters=[],
            is_async=False
        ))
    
    return class_info


def _find_class(source_code: bytes, node: Node, target_class_name: str) -> Optional[ClassInfo]:
    """Sucht eine spezifische Klasse im AST

    Args:
        source_code: Der Quellcode als bytes
        node: Der aktuelle AST-Node
        target_class_name: Name der zu parsenden Klasse
    
    Returns:
        ClassInfo wenn die Klasse gefunden wurde, sonst None
    """
    if node.type == 'class_specifier':
        class_info = _parse_class(source_code, node, target_class_name)
        if class_info:
            return class_info

    for child in node.children:
        result = _find_class(source_code, child, target_class_name)
        if result:
            return result
    
    return None


# =============================================================================
# Public API
# =============================================================================

def parse_header(header_path: str, class_name: str) -> Optional[ClassInfo]:
    """Parst eine C++ Header-Datei und extrahiert eine spezifische Klasse

    Args:
        header_path: Pfad zur Header-Datei
        class_name: Name der zu parsenden Klasse

    Returns:
        ClassInfo-Objekt wenn die Klasse gefunden wurde, sonst None
    """
    with open(Path(header_path).resolve(), 'rb') as f:
        source_code = f.read()

    parser = Parser(Language(tscpp.language()))
    tree = parser.parse(source_code)
    return _find_class(source_code, tree.root_node, class_name)


# =============================================================================
# Report Generation
# =============================================================================

def generate_detailed_report(class_info: Optional[ClassInfo], header_path: str) -> str:
    """Generiert einen detaillierten Report über eine geparste Klasse"""
    lines = [
        "=" * 80,
        "webbridge Parser - Detaillierter Report",
        "=" * 80,
        f"Header-Datei: {header_path}",
        f"Klasse gefunden: {'Ja' if class_info else 'Nein'}",
        ""
    ]

    if not class_info:
        lines.extend([
            "WARNUNG: Klasse nicht gefunden!",
            "",
            "Mögliche Gründe:",
            "  - Klasse existiert nicht in der Header-Datei",
            "  - Falscher Klassenname angegeben",
            "  - Syntaxfehler in der Header-Datei",
            "=" * 80
        ])
        return "\n".join(lines)

    cls = class_info
    lines.extend([
        "-" * 80,
        f"Klasse: {cls.name}",
        "-" * 80,
        "",
        f"PROPERTIES ({len(cls.properties)})",
        "-" * 40
    ])

    if cls.properties:
        max_len = max(len(p.name) for p in cls.properties)
        lines.extend(f"  • {p.name.ljust(max_len)} : {p.type_name}" for p in cls.properties)
    else:
        lines.append("  (keine Properties gefunden)")

    lines.extend(["", f"EVENTS ({len(cls.events)})", "-" * 40])

    if cls.events:
        max_len = max(len(e.name) for e in cls.events)
        lines.extend(f"  • {e.name.ljust(max_len)} : Event<{', '.join(e.arg_types)}>"
                     if e.arg_types else f"  • {e.name.ljust(max_len)} : Event<>"
                     for e in cls.events)
    else:
        lines.append("  (keine Events gefunden)")

    lines.extend(["", f"CONSTANTS ({len(cls.constants)})", "-" * 40])

    if cls.constants:
        max_len = max(len(c.name) for c in cls.constants)
        for const in cls.constants:
            static_prefix = 'static ' if const.is_static else ''
            lines.append(f"  - {const.name.ljust(max_len)} : {static_prefix}{const.type_name}")
    else:
        lines.append("  (keine Konstanten gefunden)")

    lines.extend(["", f"ENUMS ({len(cls.enums)})", "-" * 40])

    if cls.enums:
        for enum in cls.enums:
            enum_type = 'enum class' if enum.is_enum_class else 'enum'
            values_str = ', '.join(enum.enum_values) if enum.enum_values else '(keine Werte)'
            lines.append(f"  • {enum.name} [{enum_type}]: {{{values_str}}}")
    else:
        lines.append("  (keine Enums gefunden)")

    lines.extend(["", f"CONSTRUCTORS ({len(cls.constructors)})", "-" * 40])

    if cls.constructors:
        for ctor in cls.constructors:
            if ctor.parameters:
                lines.append(f"  • {ctor.name}({', '.join(f'{t} {n}' for t, n in ctor.parameters)})")
            else:
                lines.append(f"  • {ctor.name}()")
    else:
        lines.append("  (keine Konstruktoren gefunden)")

    lines.extend(["", f"SYNCHRONE METHODEN ({len(cls.sync_methods)})", "-" * 40])

    if cls.sync_methods:
        lines.extend(f"  • {m.name}({', '.join(f'{t} {n}' for t, n in m.parameters)}) -> {m.return_type}"
                     for m in cls.sync_methods)
    else:
        lines.append("  (keine synchronen Methoden gefunden)")

    lines.extend(["", f"ASYNCHRONE METHODEN ({len(cls.async_methods)})", "-" * 40])

    if cls.async_methods:
        lines.extend(f"  • {m.name}({', '.join(f'{t} {n}' for t, n in m.parameters)}) -> {m.return_type} [ASYNC]"
                     for m in cls.async_methods)
    else:
        lines.append("  (keine asynchronen Methoden gefunden)")

    total = len(cls.properties) + len(cls.events) + len(cls.constants) + len(cls.enums) + len(cls.constructors) + len(cls.sync_methods) + len(cls.async_methods)
    lines.extend([
        "",
        "ZUSAMMENFASSUNG",
        "-" * 40,
        f"  Gesamtzahl Members: {total}",
        f"    - Properties:      {len(cls.properties)}",
        f"    - Events:          {len(cls.events)}",
        f"    - Constants:       {len(cls.constants)}",
        f"    - Enums:           {len(cls.enums)}",
        f"    - Constructors:    {len(cls.constructors)}",
        f"    - Sync Methoden:   {len(cls.sync_methods)}",
        f"    - Async Methoden:  {len(cls.async_methods)}",
        ""
    ])

    lines.extend(["=" * 80, "Report-Ende", "=" * 80])
    return "\n".join(lines)


# =============================================================================
# Command Line Interface
# =============================================================================

def main():
    """Main Entry Point für Command Line Usage"""
    import argparse

    parser = argparse.ArgumentParser(
        description='webbridge Parser - Analysiert C++ Header-Dateien für spezifische Klassen',
        epilog='Beispiel: python webbridge_parser.py ../src/MyObject.h --class-name MyObject'
    )
    parser.add_argument('header_file', help='Pfad zur C++ Header-Datei')
    parser.add_argument('-c', '--class-name', required=True, help='Name der zu parsenden Klasse')
    parser.add_argument('-o', '--output', help='Ausgabedatei für Report (Standard: stdout)')

    args = parser.parse_args()

    if not Path(args.header_file).exists():
        print(f"FEHLER: Header-Datei nicht gefunden: {args.header_file}", file=sys.stderr)
        sys.exit(1)

    try:
        class_info = parse_header(args.header_file, args.class_name)
        output = generate_detailed_report(class_info, args.header_file)

        if args.output:
            Path(args.output).write_text(output, encoding='utf-8')
            print(f"Report erfolgreich geschrieben: {args.output}")
        else:
            print(output)

        sys.exit(0 if class_info else 1)

    except Exception as e:
        print(f"FEHLER beim Parsen: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        sys.exit(2)


if __name__ == '__main__':
    main()
