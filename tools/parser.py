#!/usr/bin/env python3
"""
webbridge Parser Module

Tree-sitter based parser for extracting class information from C++ headers.
Extracts properties, events, and methods from explicitly specified classes.

Note on Implementation:
    Tree-sitter provides a powerful query API using S-expressions
    (see https://tree-sitter.github.io/tree-sitter/using-parsers/queries/1-syntax.html)
    which would allow declarative pattern matching like:
    
        (field_declaration
          (template_type name: (type_identifier) @template_name)
          declarator: (field_identifier) @member_name)
    
    However, the Python bindings for tree-sitter queries were not working reliably
    in our testing environment. Therefore, this module uses manual AST traversal
    instead. While more verbose, this approach provides full control over the
    parsing logic and handles edge cases more explicitly.

Requirements:
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
    """Information about a Property<T> member."""
    name: str
    type_name: str


@dataclass
class EventInfo:
    """Information about an Event<Args...> member."""
    name: str
    arg_types: List[str]


@dataclass
class ConstInfo:
    """Information about a constant member."""
    name: str
    type_name: str
    is_static: bool


@dataclass
class EnumInfo:
    """Information about an enum definition."""
    name: str
    enum_values: List[str]
    is_enum_class: bool


@dataclass
class MethodInfo:
    """Information about a method."""
    name: str
    return_type: str
    parameters: List[Tuple[str, str]]
    is_async: bool = False


@dataclass
class ClassInfo:
    """Collected information about a C++ class."""
    name: str
    namespace: List[str] = field(default_factory=list)
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
    """Extract the text content of an AST node."""
    return source_code[node.start_byte:node.end_byte].decode('utf-8')


def _normalize_type(source_code: bytes, node: Node) -> str:
    """Extract and normalize type text with correct spacing."""
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
    """Find the first child node matching any of the given types."""
    return next((child for child in node.children if child.type in types), None)


def _extract_template_info(source_code: bytes, type_node: Node) -> Tuple[Optional[str], Optional[Node]]:
    """Extract template name and template argument list from a template type node."""
    template_name_node = _find_child_by_type(type_node, 'type_identifier')
    template_args = _find_child_by_type(type_node, 'template_argument_list')

    template_name = _get_node_text(source_code, template_name_node) if template_name_node else None
    return template_name, template_args


def _parse_parameters(source_code: bytes, param_list: Node) -> List[Tuple[str, str]]:
    """Parse a parameter list node into a list of (type, name) tuples."""
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


def _parse_method(source_code: bytes, node: Node, func_declarator: Node, 
                  class_name: str, is_inline: bool = False) -> Optional[MethodInfo]:
    """Parse a method from either function_definition or field_declaration.
    
    Returns None if the node should be skipped (destructor, operator, etc.).
    Returns a tuple (method_info, is_constructor) otherwise.
    """
    name_types = ('field_identifier', 'identifier') if is_inline else ('field_identifier',)
    method_name_node = _find_child_by_type(func_declarator, *name_types)
    params_node = _find_child_by_type(func_declarator, 'parameter_list')
    
    if not (method_name_node and params_node):
        return None
    
    method_name = _get_node_text(source_code, method_name_node)
    
    # Skip destructors and operators
    if method_name.startswith(('~', 'operator')):
        return None
    
    is_constructor = (method_name == class_name)
    
    # Determine return type based on node type
    if is_constructor:
        return_type = ''
    elif is_inline:
        # For function_definition: return type is before function_declarator
        skip_types = ('function_declarator', 'compound_statement', 'type_qualifier')
        return_type_node = next((c for c in node.children if c.type not in skip_types), None)
        return_type = _normalize_type(source_code, return_type_node) if return_type_node else 'void'
    else:
        # For field_declaration: skip attributes and declarators
        skip_types = ('attribute_declaration', 'field_identifier', 'function_declarator', ';')
        return_type_node = next((c for c in node.children if c.type not in skip_types), None)
        return_type = _normalize_type(source_code, return_type_node) if return_type_node else 'void'
    
    # Check for async attribute (only in field_declaration, not inline)
    is_async = False
    if not is_inline:
        is_async = any('async' in _get_node_text(source_code, child)
                       for child in node.children if child.type == 'attribute_declaration')
    
    return MethodInfo(
        name=method_name,
        return_type=return_type,
        parameters=_parse_parameters(source_code, params_node),
        is_async=is_async
    )


def _extract_all_members(source_code: bytes, class_body: Node, class_info: ClassInfo):
    """Extract all members (properties, events, methods) in a single pass."""

    current_access = 'private'  # Default access in class is private

    def process_field(node: Node, access: str):
        if node.type not in ('field_declaration', 'function_definition', 'enum_specifier'):
            return

        # Ignore non-public members
        if access != 'public':
            return

        # Handle enum_specifier
        if node.type == 'enum_specifier':
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

            # Extract enum values
            enum_values = []
            if enumerator_list:
                for child in enumerator_list.children:
                    if child.type == 'enumerator':
                        identifier = _find_child_by_type(child, 'identifier')
                        if identifier:
                            enum_values.append(_get_node_text(source_code, identifier))

            class_info.enums.append(EnumInfo(
                name=enum_name or '<anonymous>',
                enum_values=enum_values,
                is_enum_class=is_enum_class
            ))
            return

        # Handle function_definition (inline method with body)
        if node.type == 'function_definition':
            func_declarator = _find_child_by_type(node, 'function_declarator')
            if not func_declarator:
                return
            
            method = _parse_method(source_code, node, func_declarator, class_info.name, is_inline=True)
            if method:
                if method.name == class_info.name:
                    class_info.constructors.append(method)
                else:
                    class_info.sync_methods.append(method)
            return

        # Handle field_declaration (could be method declaration, property, event, or constant)
        func_declarator = _find_child_by_type(node, 'function_declarator')

        if func_declarator:
            method = _parse_method(source_code, node, func_declarator, class_info.name, is_inline=False)
            if method:
                if method.name == class_info.name:
                    class_info.constructors.append(method)
                elif method.is_async:
                    class_info.async_methods.append(method)
                else:
                    class_info.sync_methods.append(method)
            return

        # Sonst ist es Property, Event oder Konstante
        type_node = _find_child_by_type(node, 'template_type')
        declarator_node = _find_child_by_type(node, 'field_identifier')

        # Wenn es ein template_type ist, prüfe auf Property/Event
        if type_node and declarator_node:
            template_name, template_args = _extract_template_info(source_code, type_node)
            member_name = _get_node_text(source_code, declarator_node)

            # Property oder Event? (supports both old PascalCase and new snake_case)
            if template_name in ('Property', 'property'):
                prop_type = 'unknown'
                if template_args:
                    type_desc = _find_child_by_type(template_args, 'type_descriptor')
                    if type_desc:
                        prop_type = _normalize_type(source_code, type_desc)
                class_info.properties.append(PropertyInfo(name=member_name, type_name=prop_type))
                return

            elif template_name in ('Event', 'event'):
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

    # Traverse with access specifier tracking
    def walk(node: Node):
        nonlocal current_access

        if node.type == 'access_specifier':
            access_text = _get_node_text(source_code, node)
            for access in ('public', 'private', 'protected'):
                if access in access_text:
                    current_access = access
                    break
        else:
            process_field(node, current_access)

        for child in node.children:
            walk(child)

    walk(class_body)


def _parse_class(source_code: bytes, node: Node, target_class_name: str, namespace: List[str] = None) -> Optional[ClassInfo]:
    """Parse a class definition node.

    Args:
        source_code: The source code as bytes
        node: The AST node of the class
        target_class_name: Name of the class to parse
        namespace: List of namespace names (for nested namespaces)
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

    class_info = ClassInfo(name=class_name, namespace=namespace or [])
    _extract_all_members(source_code, class_body, class_info)
    
    # Add implicit default constructor if none found
    if not class_info.constructors:
        class_info.constructors.append(MethodInfo(
            name=class_name,
            return_type='',
            parameters=[],
            is_async=False
        ))
    
    return class_info


def _find_class(source_code: bytes, node: Node, target_class_name: str, namespace: List[str] = None) -> Optional[ClassInfo]:
    """Search for a specific class in the AST.

    Args:
        source_code: The source code as bytes
        node: The current AST node
        target_class_name: Name of the class to find
        namespace: Current namespace hierarchy
    
    Returns:
        ClassInfo if the class was found, None otherwise
    """
    if namespace is None:
        namespace = []
    
    if node.type == 'class_specifier':
        class_info = _parse_class(source_code, node, target_class_name, namespace)
        if class_info:
            return class_info
    
    # Bei namespace_definition: extrahiere Namen und durchsuche Inhalt
    if node.type == 'namespace_definition':
        ns_name = None
        ns_body = None
        for child in node.children:
            if child.type == 'namespace_identifier':
                ns_name = _get_node_text(source_code, child)
            elif child.type == 'declaration_list':
                ns_body = child
        
        if ns_name and ns_body:
            new_namespace = namespace + [ns_name]
            for child in ns_body.children:
                result = _find_class(source_code, child, target_class_name, new_namespace)
                if result:
                    return result
            return None

    for child in node.children:
        result = _find_class(source_code, child, target_class_name, namespace)
        if result:
            return result
    
    return None


# =============================================================================
# Public API
# =============================================================================

def parse_header(header_path: str, class_name: str) -> Optional[ClassInfo]:
    """Parse a C++ header file and extract a specific class.

    Args:
        header_path: Path to the header file
        class_name: Name of the class to parse

    Returns:
        ClassInfo object if the class was found, None otherwise
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
    """Generate a detailed report about a parsed class."""
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
    """Main entry point for command line usage."""
    import argparse

    parser = argparse.ArgumentParser(
        description='webbridge Parser - Analyze C++ header files for specific classes',
        epilog='Example: python parser.py ../src/MyObject.h --class-name MyObject'
    )
    parser.add_argument('header_file', help='Path to the C++ header file')
    parser.add_argument('-c', '--class-name', required=True, help='Name of the class to parse')
    parser.add_argument('-o', '--output', help='Output file for report (default: stdout)')

    args = parser.parse_args()

    if not Path(args.header_file).exists():
        print(f"ERROR: Header file not found: {args.header_file}", file=sys.stderr)
        sys.exit(1)

    try:
        class_info = parse_header(args.header_file, args.class_name)
        output = generate_detailed_report(class_info, args.header_file)

        if args.output:
            Path(args.output).write_text(output, encoding='utf-8')
            print(f"Report written successfully: {args.output}")
        else:
            print(output)

        sys.exit(0 if class_info else 1)

    except Exception as e:
        print(f"ERROR while parsing: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        sys.exit(2)


if __name__ == '__main__':
    main()
