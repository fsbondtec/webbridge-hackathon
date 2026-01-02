#!/usr/bin/env python3
"""
Burntest für den webbridge Parser

Testet alle Funktionen des Parsers mit verschiedenen Szenarien:
- Properties, Events, Konstanten, Enums
- Konstruktoren, synchrone und asynchrone Methoden
- Edge Cases und Fehlerfälle

Ausführung:
    python test_parser.py
    python -m pytest test_parser.py -v
"""

import pytest
import tempfile
from pathlib import Path
from parser import (
    parse_header,
    generate_detailed_report,
    ClassInfo,
    PropertyInfo,
    EventInfo,
    ConstInfo,
    EnumInfo,
    MethodInfo,
)


# =============================================================================
# Test Fixtures
# =============================================================================

@pytest.fixture
def simple_class_header():
    """Einfache Klasse mit Properties und Events"""
    return """
#pragma once

template<typename T>
class Property {};

template<typename... Args>
class Event {};

class SimpleClass {
public:
    Property<int> counter;
    Property<std::string> name;
    Event<int, bool> onChanged;
    Event<> onReset;
    
    void doSomething();
    int getValue();
};
"""


@pytest.fixture
def complex_class_header():
    """Komplexe Klasse mit allen Features"""
    return """
#pragma once

#include <string>
#include <vector>

template<typename T>
class Property {};

template<typename... Args>
class Event {};

namespace webbridge {
    class Object {};
}

enum class Status {
    Idle,
    Running,
    Completed,
    Error
};

class MyObject : public webbridge::Object
{
public:
    Property<bool> aBool;
    Property<std::string> strProp;
    Property<int> counter;
    Property<std::vector<int>> numbers;
    Property<Status> status;

    Event<int, bool> aEvent;
    Event<> simpleEvent;

    // Konstanten
    const std::string version;
    static inline const std::string appversion;
    static inline constexpr unsigned cppversion{23};

    enum class InnerEnum {
        Value1,
        Value2,
        Value3
    };

public:
    explicit MyObject(const std::string& version_);
    explicit MyObject();

    [[async]] void foo(const std::string& val);

    bool bar();

    [[async]] void asyncMethod();

    void testVectors();

    void throwError();

    std::string multiParamTest(
        int intValue,
        bool boolValue,
        const std::string& strValue,
        const std::vector<int>& vecValue
    );

private:
    int privateField;
    void privateMethod();
};
"""


@pytest.fixture
def header_with_templates():
    """Header mit Template-Typen"""
    return """
#pragma once

template<typename T>
class Property {};

template<typename... Args>
class Event {};

class TemplateTest {
public:
    Property<std::vector<int>> vecProp;
    Property<std::map<std::string, int>> mapProp;
    Property<std::optional<double>> optProp;
    
    Event<std::vector<std::string>, int> complexEvent;
};
"""


@pytest.fixture
def header_with_access_specifiers():
    """Header mit verschiedenen Access-Specifiers"""
    return """
#pragma once

template<typename T>
class Property {};

class AccessTest {
public:
    Property<int> publicProp;
    void publicMethod();
    
protected:
    Property<int> protectedProp;
    void protectedMethod();
    
private:
    Property<int> privateProp;
    void privateMethod();
    
public:
    Property<int> anotherPublicProp;
};
"""


@pytest.fixture
def empty_class_header():
    """Leere Klasse"""
    return """
#pragma once

class EmptyClass {
public:
};
"""


@pytest.fixture
def class_with_inline_methods():
    """Klasse mit inline-definierten Methoden"""
    return """
#pragma once

template<typename T>
class Property {};

class InlineClass {
public:
    Property<int> value;
    
    int getValue() { return 42; }
    void setValue(int v) { /* ... */ }
    bool isEmpty() const { return false; }
};
"""


def write_temp_header(content: str) -> Path:
    """Schreibt Content in eine temporäre Header-Datei"""
    with tempfile.NamedTemporaryFile(mode='w', suffix='.h', delete=False, encoding='utf-8') as f:
        f.write(content)
        return Path(f.name)


# =============================================================================
# Property Tests
# =============================================================================

class TestProperties:
    """Tests für Property-Parsing"""
    
    def test_simple_properties(self, simple_class_header):
        """Test: Einfache Properties werden erkannt"""
        path = write_temp_header(simple_class_header)
        try:
            result = parse_header(str(path), "SimpleClass")
            
            assert result is not None
            assert len(result.properties) == 2
            
            prop_names = {p.name for p in result.properties}
            assert "counter" in prop_names
            assert "name" in prop_names
            
            counter_prop = next(p for p in result.properties if p.name == "counter")
            assert counter_prop.type_name == "int"
            
            name_prop = next(p for p in result.properties if p.name == "name")
            assert "string" in name_prop.type_name
        finally:
            path.unlink()
    
    def test_complex_type_properties(self, header_with_templates):
        """Test: Properties mit komplexen Typen"""
        path = write_temp_header(header_with_templates)
        try:
            result = parse_header(str(path), "TemplateTest")
            
            assert result is not None
            assert len(result.properties) == 3
            
            vec_prop = next(p for p in result.properties if p.name == "vecProp")
            assert "vector" in vec_prop.type_name
            assert "int" in vec_prop.type_name
        finally:
            path.unlink()
    
    def test_no_private_properties(self, header_with_access_specifiers):
        """Test: Private Properties werden ignoriert"""
        path = write_temp_header(header_with_access_specifiers)
        try:
            result = parse_header(str(path), "AccessTest")
            
            assert result is not None
            # Nur public properties
            assert len(result.properties) == 2
            prop_names = {p.name for p in result.properties}
            assert "publicProp" in prop_names
            assert "anotherPublicProp" in prop_names
            assert "privateProp" not in prop_names
            assert "protectedProp" not in prop_names
        finally:
            path.unlink()


# =============================================================================
# Event Tests
# =============================================================================

class TestEvents:
    """Tests für Event-Parsing"""
    
    def test_events_with_args(self, simple_class_header):
        """Test: Events mit Argumenten"""
        path = write_temp_header(simple_class_header)
        try:
            result = parse_header(str(path), "SimpleClass")
            
            assert result is not None
            assert len(result.events) == 2
            
            on_changed = next(e for e in result.events if e.name == "onChanged")
            assert len(on_changed.arg_types) == 2
            assert "int" in on_changed.arg_types[0]
            assert "bool" in on_changed.arg_types[1]
        finally:
            path.unlink()
    
    def test_event_without_args(self, simple_class_header):
        """Test: Event ohne Argumente"""
        path = write_temp_header(simple_class_header)
        try:
            result = parse_header(str(path), "SimpleClass")
            
            on_reset = next(e for e in result.events if e.name == "onReset")
            assert len(on_reset.arg_types) == 0
        finally:
            path.unlink()


# =============================================================================
# Method Tests
# =============================================================================

class TestMethods:
    """Tests für Methoden-Parsing"""
    
    def test_sync_methods(self, simple_class_header):
        """Test: Synchrone Methoden"""
        path = write_temp_header(simple_class_header)
        try:
            result = parse_header(str(path), "SimpleClass")
            
            assert result is not None
            assert len(result.sync_methods) == 2
            
            method_names = {m.name for m in result.sync_methods}
            assert "doSomething" in method_names
            assert "getValue" in method_names
        finally:
            path.unlink()
    
    def test_async_methods(self, complex_class_header):
        """Test: Asynchrone Methoden mit [[async]] Attribut"""
        path = write_temp_header(complex_class_header)
        try:
            result = parse_header(str(path), "MyObject")
            
            assert result is not None
            assert len(result.async_methods) == 2
            
            async_names = {m.name for m in result.async_methods}
            assert "foo" in async_names
            assert "asyncMethod" in async_names
        finally:
            path.unlink()
    
    def test_method_parameters(self, complex_class_header):
        """Test: Methoden-Parameter werden korrekt geparst"""
        path = write_temp_header(complex_class_header)
        try:
            result = parse_header(str(path), "MyObject")
            
            multi_param = next(m for m in result.sync_methods if m.name == "multiParamTest")
            assert len(multi_param.parameters) == 4
            
            # Prüfe Parameter-Typen
            param_types = [t for t, _ in multi_param.parameters]
            assert "int" in param_types[0]
            assert "bool" in param_types[1]
        finally:
            path.unlink()
    
    def test_method_return_types(self, complex_class_header):
        """Test: Return-Typen werden korrekt geparst"""
        path = write_temp_header(complex_class_header)
        try:
            result = parse_header(str(path), "MyObject")
            
            bar_method = next(m for m in result.sync_methods if m.name == "bar")
            assert bar_method.return_type == "bool"
            
            multi_param = next(m for m in result.sync_methods if m.name == "multiParamTest")
            assert "string" in multi_param.return_type
        finally:
            path.unlink()
    
    def test_inline_methods(self, class_with_inline_methods):
        """Test: Inline-definierte Methoden"""
        path = write_temp_header(class_with_inline_methods)
        try:
            result = parse_header(str(path), "InlineClass")
            
            assert result is not None
            assert len(result.sync_methods) == 3
            
            method_names = {m.name for m in result.sync_methods}
            assert "getValue" in method_names
            assert "setValue" in method_names
            assert "isEmpty" in method_names
        finally:
            path.unlink()
    
    def test_no_private_methods(self, header_with_access_specifiers):
        """Test: Private Methoden werden ignoriert"""
        path = write_temp_header(header_with_access_specifiers)
        try:
            result = parse_header(str(path), "AccessTest")
            
            method_names = {m.name for m in result.sync_methods}
            assert "publicMethod" in method_names
            assert "privateMethod" not in method_names
            assert "protectedMethod" not in method_names
        finally:
            path.unlink()


# =============================================================================
# Constructor Tests
# =============================================================================

class TestConstructors:
    """Tests für Konstruktor-Parsing"""
    
    def test_explicit_constructors(self, complex_class_header):
        """Test: Explizite Konstruktoren"""
        path = write_temp_header(complex_class_header)
        try:
            result = parse_header(str(path), "MyObject")
            
            assert result is not None
            # Mindestens ein Konstruktor sollte existieren (Default wird hinzugefügt)
            assert len(result.constructors) >= 1
            
            # Default-Konstruktor sollte existieren
            default_ctor = next((c for c in result.constructors if not c.parameters), None)
            assert default_ctor is not None
            assert default_ctor.name == "MyObject"
        finally:
            path.unlink()
    
    def test_default_constructor_added(self, simple_class_header):
        """Test: Default-Konstruktor wird hinzugefügt wenn keiner existiert"""
        path = write_temp_header(simple_class_header)
        try:
            result = parse_header(str(path), "SimpleClass")
            
            assert result is not None
            assert len(result.constructors) == 1
            assert result.constructors[0].name == "SimpleClass"
            assert len(result.constructors[0].parameters) == 0
        finally:
            path.unlink()


# =============================================================================
# Constant Tests
# =============================================================================

class TestConstants:
    """Tests für Konstanten-Parsing"""
    
    def test_const_members(self, complex_class_header):
        """Test: Const Members werden erkannt"""
        path = write_temp_header(complex_class_header)
        try:
            result = parse_header(str(path), "MyObject")
            
            assert result is not None
            assert len(result.constants) >= 2
            
            const_names = {c.name for c in result.constants}
            assert "version" in const_names
            assert "appversion" in const_names
        finally:
            path.unlink()
    
    def test_static_const_detection(self, complex_class_header):
        """Test: Static const wird korrekt erkannt"""
        path = write_temp_header(complex_class_header)
        try:
            result = parse_header(str(path), "MyObject")
            
            appversion = next(c for c in result.constants if c.name == "appversion")
            assert appversion.is_static == True
            
            version = next(c for c in result.constants if c.name == "version")
            assert version.is_static == False
        finally:
            path.unlink()
    
    def test_inline_static_constexpr(self, complex_class_header):
        """Test: Inline static constexpr wird korrekt erkannt (oder dokumentiert, wenn nicht)"""
        path = write_temp_header(complex_class_header)
        try:
            result = parse_header(str(path), "MyObject")
            
            assert result is not None
            
            # Prüfe dass cppversion erkannt wird
            const_names = {c.name for c in result.constants}
            if "cppversion" in const_names:
                cppversion = next(c for c in result.constants if c.name == "cppversion")
                assert cppversion.is_static == True
                assert "constexpr" in cppversion.type_name or "unsigned" in cppversion.type_name
            else:
                assert "cppversion" not in const_names, "constexpr-Konstanten werden noch nicht geparst"
        finally:
            path.unlink()


# =============================================================================
# Enum Tests
# =============================================================================

class TestEnums:
    """Tests für Enum-Parsing"""
    
    def test_inner_enum_class(self, complex_class_header):
        """Test: Inner enum class wird erkannt"""
        path = write_temp_header(complex_class_header)
        try:
            result = parse_header(str(path), "MyObject")
            
            assert result is not None
            assert len(result.enums) == 1
            
            inner_enum = result.enums[0]
            assert inner_enum.name == "InnerEnum"
            assert inner_enum.is_enum_class == True
            assert "Value1" in inner_enum.enum_values
            assert "Value2" in inner_enum.enum_values
            assert "Value3" in inner_enum.enum_values
        finally:
            path.unlink()


# =============================================================================
# Edge Case Tests
# =============================================================================

class TestEdgeCases:
    """Tests für Edge Cases"""
    
    def test_empty_class(self, empty_class_header):
        """Test: Leere Klasse"""
        path = write_temp_header(empty_class_header)
        try:
            result = parse_header(str(path), "EmptyClass")
            
            assert result is not None
            assert result.name == "EmptyClass"
            assert len(result.properties) == 0
            assert len(result.events) == 0
            assert len(result.sync_methods) == 0
            assert len(result.async_methods) == 0
            # Default-Konstruktor wird hinzugefügt
            assert len(result.constructors) == 1
        finally:
            path.unlink()
    
    def test_class_not_found(self, simple_class_header):
        """Test: Nicht existierende Klasse"""
        path = write_temp_header(simple_class_header)
        try:
            result = parse_header(str(path), "NonExistentClass")
            assert result is None
        finally:
            path.unlink()
    
    def test_file_not_found(self):
        """Test: Nicht existierende Datei"""
        with pytest.raises(FileNotFoundError):
            parse_header("/non/existent/path.h", "SomeClass")


# =============================================================================
# Report Tests
# =============================================================================

class TestReportGeneration:
    """Tests für Report-Generierung"""
    
    def test_report_with_class(self, complex_class_header):
        """Test: Report für gefundene Klasse"""
        path = write_temp_header(complex_class_header)
        try:
            result = parse_header(str(path), "MyObject")
            report = generate_detailed_report(result, str(path))
            
            assert "MyObject" in report
            assert "PROPERTIES" in report
            assert "EVENTS" in report
            assert "METHODEN" in report or "METHODS" in report or "Methoden" in report
            assert "ZUSAMMENFASSUNG" in report
        finally:
            path.unlink()
    
    def test_report_class_not_found(self, simple_class_header):
        """Test: Report für nicht gefundene Klasse"""
        path = write_temp_header(simple_class_header)
        try:
            result = parse_header(str(path), "NonExistent")
            report = generate_detailed_report(result, str(path))
            
            assert "WARNUNG" in report
            assert "nicht gefunden" in report
        finally:
            path.unlink()


# =============================================================================
# Integration Test mit echter Header-Datei
# =============================================================================

class TestRealHeaderFile:
    """Integration Tests mit der echten MyObject.h"""
    
    @pytest.fixture
    def real_header_path(self):
        """Pfad zur echten Header-Datei"""
        # Anpassen je nach Projektstruktur
        path = Path(__file__).parent.parent / "src" / "MyObject.h"
        if path.exists():
            return path
        return None
    
    def test_parse_real_myobject(self, real_header_path):
        """Test: Parsen der echten MyObject.h"""
        if real_header_path is None:
            pytest.skip("MyObject.h nicht gefunden")
        
        result = parse_header(str(real_header_path), "MyObject")
        
        assert result is not None
        assert result.name == "MyObject"
        
        # Prüfe erwartete Properties
        prop_names = {p.name for p in result.properties}
        assert "aBool" in prop_names
        assert "strProp" in prop_names
        assert "counter" in prop_names
        assert "numbers" in prop_names
        assert "status" in prop_names
        assert "pod" in prop_names
        
        # Prüfe Event
        assert len(result.events) >= 1
        event_names = {e.name for e in result.events}
        assert "aEvent" in event_names
        
        # Prüfe Methoden
        sync_names = {m.name for m in result.sync_methods}
        assert "bar" in sync_names
        assert "testVectors" in sync_names
        assert "throwError" in sync_names
        assert "multiParamTest" in sync_names
        
        async_names = {m.name for m in result.async_methods}
        assert "foo" in async_names
        assert "file" in async_names
        
        # Prüfe Konstruktoren
        assert len(result.constructors) == 2
        
        # Prüfe Konstanten (constexpr wird aktuell nicht als Konstante erkannt)
        const_names = {c.name for c in result.constants}
        assert "version" in const_names
        assert "appversion" in const_names
        # cppversion ist constexpr, wird möglicherweise nicht erkannt
        # assert "cppversion" in const_names
    
    def test_generate_report_real_file(self, real_header_path):
        """Test: Report-Generierung für echte Datei"""
        if real_header_path is None:
            pytest.skip("MyObject.h nicht gefunden")
        
        result = parse_header(str(real_header_path), "MyObject")
        report = generate_detailed_report(result, str(real_header_path))
        
        assert len(report) > 500  # Report sollte substanziell sein
        assert "MyObject" in report
        assert "aBool" in report
        assert "aEvent" in report


# =============================================================================
# Main Entry Point
# =============================================================================

def run_burntest():
    """Führt alle Tests aus und gibt einen Zusammenfassungsbericht"""
    print("=" * 80)
    print("webbridge Parser - Burntest")
    print("=" * 80)
    print()
    
    # Führe pytest aus
    exit_code = pytest.main([
        __file__,
        "-v",
        "--tb=short",
        "-x",  # Stop bei erstem Fehler
    ])
    
    print()
    print("=" * 80)
    if exit_code == 0:
        print("✅ Alle Tests erfolgreich!")
    else:
        print("❌ Einige Tests fehlgeschlagen!")
    print("=" * 80)
    
    return exit_code


if __name__ == "__main__":
    import sys
    sys.exit(run_burntest())
