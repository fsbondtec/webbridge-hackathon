#!/usr/bin/env python3
# Unterstützte C++-Container (nlohmann::json)
#
# Sequenz-Container (werden als JSON-Arrays umgesetzt):
#
# std::vector<T>
# std::deque<T>
# std::list<T>
# std::array<T, N>
# – sowie alle iterierbaren Container – sind direkt kompatibel für Serialisierung
#  und Deserialisierung. [deepwiki.com], [json.nlohmann.me]
#
# Assoziative Container (werden als JSON-Objekte umgesetzt):
#
# std::map<std::string, T>
# std::unordered_map<std::string, T>
#
# – Schlüssel müssen std::string sein, da JSON-Objekte nur String-Keys erlauben.
# [github.com], [deepwiki.com]
#
# Primitive Typen: int, double, bool, std::string, nullptr_t usw., alles nativ abgebildet.
# [github.com], [deepwiki.com]
#
# Benutzerdefinierte Typen sind durch to_json / from_json Serialisierer ebenfalls
# voll integriert, aber das ist über C++ hinaus. [json.nlohmann.me]


SCALAR_MAP = {
    'bool': 'boolean',
    'char': 'number',
    'signed char': 'number',
    'unsigned char': 'number',
    'short': 'number',
    'short int': 'number',
    'signed short': 'number',
    'signed short int': 'number',
    'unsigned short': 'number',
    'unsigned short int': 'number',
    'int': 'number',
    'signed': 'number',
    'signed int': 'number',
    'unsigned': 'number',
    'unsigned int': 'number',
    'long': 'number',
    'long int': 'number',
    'signed long': 'number',
    'signed long int': 'number',
    'unsigned long': 'number',
    'unsigned long int': 'number',
    'long long': 'number',
    'long long int': 'number',
    'signed long long': 'number',
    'signed long long int': 'number',
    'unsigned long long': 'number',
    'unsigned long long int': 'number',
    'uint8_t': 'number',
    'uint16_t': 'number',
    'uint32_t': 'number',
    'uint64_t': 'number',
    'int8_t': 'number',
    'int16_t': 'number',
    'int32_t': 'number',
    'int64_t': 'number',
    'size_t': 'number',
    'ssize_t': 'number',
    'float': 'number',
    'double': 'number',
    'long double': 'number',
    'std::string': 'string',
    'nullptr_t': 'null',
}

SEQ_CONTAINER_MAP = {
    'std::vector': 'Array',
    'std::deque': 'Array',
    'std::list': 'Array',
    'std::array': 'Array',
}


ASSOC_CONTAINER_MAP = {
    'std::map': 'Record',
    'std::unordered_map': 'Record',
}


def cpp_to_ts_type(cpp_type: str) -> str:
    """Konvertiert C++ Typen zu TypeScript Typen."""
    cpp_type = cpp_type.strip()
    # Entferne const, &, * und zusätzliche Leerzeichen
    cpp_type = cpp_type.replace('const ', '').replace('&', '').replace('*', '').strip()

    # Prüfe auf skalare Typen
    if cpp_type in SCALAR_MAP:
        return SCALAR_MAP[cpp_type]

    # Prüfe auf Sequenz-Container (vector, list, deque, array)
    for container_name in SEQ_CONTAINER_MAP:
        if cpp_type.startswith(container_name + '<'):
            # Extrahiere den inneren Typ
            start = cpp_type.index('<') + 1
            end = cpp_type.rindex('>')
            inner = cpp_type[start:end].strip()

            # Für std::array, entferne die Größe (z.B. std::array<int, 10> -> int)
            if container_name == 'std::array' and ',' in inner:
                inner = inner[:inner.index(',')].strip()

            inner_ts = cpp_to_ts_type(inner)
            return f'{inner_ts}[]'

    # Prüfe auf assoziative Container (map, unordered_map)
    for container_name in ASSOC_CONTAINER_MAP:
        if cpp_type.startswith(container_name + '<'):
            # Extrahiere Key und Value Typen
            start = cpp_type.index('<') + 1
            end = cpp_type.rindex('>')
            types = cpp_type[start:end].strip()

            # Finde das Komma zwischen Key und Value (beachte verschachtelte Templates)
            depth = 0
            comma_pos = -1
            for i, c in enumerate(types):
                if c == '<':
                    depth += 1
                elif c == '>':
                    depth -= 1
                elif c == ',' and depth == 0:
                    comma_pos = i
                    break

            if comma_pos != -1:
                key_type = types[:comma_pos].strip()
                value_type = types[comma_pos+1:].strip()

                # Key muss std::string sein für JSON
                if key_type != 'std::string':
                    return 'unknown'

                value_ts = cpp_to_ts_type(value_type)
                return f'Record<string, {value_ts}>'

    return 'unknown'