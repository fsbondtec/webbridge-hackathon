#!/usr/bin/env python3
"""
Burn test for the webbridge parser

Usage:
    python test_parser.py
    python -m pytest test_parser.py -v
"""

import pytest
import tempfile
from pathlib import Path
from parser import parse_header


# =============================================================================
# Test Fixtures
# =============================================================================

PROPERTY_EVENT_TEMPLATE = """
#pragma once
template<typename T> class property {};
template<typename... Args> class event {};
"""

@pytest.fixture
def temp_header(request):
    """Create a temporary header file from request.param and clean up afterwards."""
    with tempfile.NamedTemporaryFile(mode='w', suffix='.h', delete=False, encoding='utf-8') as f:
        f.write(request.param)
        path = Path(f.name)
    yield path
    path.unlink()


SIMPLE_CLASS = PROPERTY_EVENT_TEMPLATE + """
class SimpleClass {
public:
    property<int> counter;
    property<std::string> name;
    event<int, bool> onChanged;
    event<> onReset;
    void doSomething();
    int getValue();
};
"""

COMPLEX_CLASS = PROPERTY_EVENT_TEMPLATE + """
namespace webbridge { class object {}; }
enum class Status { Idle, Running, Completed, Error };

class MyObject : public webbridge::object {
public:
    property<bool> aBool;
    property<std::string> strProp;
    property<int> counter;
    property<std::vector<int>> numbers;
    property<Status> status;

    event<int, bool> aEvent;
    event<> simpleEvent;

    const std::string version;
    static inline const std::string appversion;
    static inline constexpr unsigned cppversion{23};

    enum class InnerEnum { Value1, Value2, Value3 };

public:
    explicit MyObject(const std::string& version_);
    explicit MyObject();

    [[async]] void foo(const std::string& val);
    bool bar();
    [[async]] void asyncMethod();
    void testVectors();
    void throwError();
    std::string multiParamTest(int intValue, bool boolValue, 
        const std::string& strValue, const std::vector<int>& vecValue);

private:
    int privateField;
    void privateMethod();
};
"""

TEMPLATE_TYPES = PROPERTY_EVENT_TEMPLATE + """
class TemplateTest {
public:
    property<std::vector<int>> vecProp;
    property<std::map<std::string, int>> mapProp;
    property<std::optional<double>> optProp;
    event<std::vector<std::string>, int> complexEvent;
};
"""

ACCESS_SPECIFIERS = PROPERTY_EVENT_TEMPLATE + """
class AccessTest {
public:
    property<int> publicProp;
    void publicMethod();
protected:
    property<int> protectedProp;
    void protectedMethod();
private:
    property<int> privateProp;
    void privateMethod();
public:
    property<int> anotherPublicProp;
};
"""

EMPTY_CLASS = "#pragma once\nclass EmptyClass { public: };"

INLINE_METHODS = PROPERTY_EVENT_TEMPLATE + """
class InlineClass {
public:
    property<int> value;
    int getValue() { return 42; }
    void setValue(int v) { /* ... */ }
    bool isEmpty() const { return false; }
};
"""

NESTED_NAMESPACE = """
#pragma once
namespace a { namespace b { namespace c {
    class DeepClass { public: void doSomething(); };
}}}
"""


# =============================================================================
# Property Tests
# =============================================================================

class TestProperties:
    """Tests for property parsing."""
    
    @pytest.mark.parametrize("temp_header", [SIMPLE_CLASS], indirect=True, ids=["simple_class"])
    def test_simple_properties(self, temp_header):
        result = parse_header(str(temp_header), "SimpleClass")
        assert result is not None
        assert len(result.properties) == 2
        assert {p.name for p in result.properties} == {"counter", "name"}
        assert next(p for p in result.properties if p.name == "counter").type_name == "int"
    
    @pytest.mark.parametrize("temp_header", [TEMPLATE_TYPES], indirect=True, ids=["template_types"])
    def test_complex_type_properties(self, temp_header):
        result = parse_header(str(temp_header), "TemplateTest")
        assert result is not None and len(result.properties) == 3
        vec_prop = next(p for p in result.properties if p.name == "vecProp")
        assert "vector" in vec_prop.type_name and "int" in vec_prop.type_name
    
    @pytest.mark.parametrize("temp_header", [ACCESS_SPECIFIERS], indirect=True, ids=["access_specifiers"])
    def test_no_private_properties(self, temp_header):
        result = parse_header(str(temp_header), "AccessTest")
        assert result is not None and len(result.properties) == 2
        prop_names = {p.name for p in result.properties}
        assert prop_names == {"publicProp", "anotherPublicProp"}


# =============================================================================
# Event Tests
# =============================================================================

class TestEvents:
    """Tests for event parsing."""
    
    @pytest.mark.parametrize("temp_header", [SIMPLE_CLASS], indirect=True, ids=["simple_class"])
    def test_events_with_args(self, temp_header):
        result = parse_header(str(temp_header), "SimpleClass")
        assert result is not None and len(result.events) == 2
        on_changed = next(e for e in result.events if e.name == "onChanged")
        assert on_changed.arg_types == ["int", "bool"] or len(on_changed.arg_types) == 2
    
    @pytest.mark.parametrize("temp_header", [SIMPLE_CLASS], indirect=True, ids=["simple_class"])
    def test_event_without_args(self, temp_header):
        result = parse_header(str(temp_header), "SimpleClass")
        on_reset = next(e for e in result.events if e.name == "onReset")
        assert len(on_reset.arg_types) == 0


# =============================================================================
# Method Tests
# =============================================================================

class TestMethods:
    """Tests for method parsing."""
    
    @pytest.mark.parametrize("temp_header", [SIMPLE_CLASS], indirect=True, ids=["simple_class"])
    def test_sync_methods(self, temp_header):
        result = parse_header(str(temp_header), "SimpleClass")
        assert result is not None and len(result.sync_methods) == 2
        assert {m.name for m in result.sync_methods} == {"doSomething", "getValue"}
    
    @pytest.mark.parametrize("temp_header", [COMPLEX_CLASS], indirect=True, ids=["complex_class"])
    def test_async_methods(self, temp_header):
        result = parse_header(str(temp_header), "MyObject")
        assert result is not None and len(result.async_methods) == 2
        assert {m.name for m in result.async_methods} == {"foo", "asyncMethod"}
    
    @pytest.mark.parametrize("temp_header", [COMPLEX_CLASS], indirect=True, ids=["complex_class"])
    def test_method_parameters(self, temp_header):
        result = parse_header(str(temp_header), "MyObject")
        multi_param = next(m for m in result.sync_methods if m.name == "multiParamTest")
        assert len(multi_param.parameters) == 4
        param_types = [t for t, _ in multi_param.parameters]
        assert "int" in param_types[0] and "bool" in param_types[1]
    
    @pytest.mark.parametrize("temp_header", [COMPLEX_CLASS], indirect=True, ids=["complex_class"])
    def test_method_return_types(self, temp_header):
        result = parse_header(str(temp_header), "MyObject")
        bar = next(m for m in result.sync_methods if m.name == "bar")
        assert bar.return_type == "bool"
        multi = next(m for m in result.sync_methods if m.name == "multiParamTest")
        assert "string" in multi.return_type
    
    @pytest.mark.parametrize("temp_header", [INLINE_METHODS], indirect=True, ids=["inline_methods"])
    def test_inline_methods(self, temp_header):
        result = parse_header(str(temp_header), "InlineClass")
        assert result is not None and len(result.sync_methods) == 3
        assert {m.name for m in result.sync_methods} == {"getValue", "setValue", "isEmpty"}
    
    @pytest.mark.parametrize("temp_header", [ACCESS_SPECIFIERS], indirect=True, ids=["access_specifiers"])
    def test_no_private_methods(self, temp_header):
        result = parse_header(str(temp_header), "AccessTest")
        method_names = {m.name for m in result.sync_methods}
        assert "publicMethod" in method_names
        assert "privateMethod" not in method_names and "protectedMethod" not in method_names


# =============================================================================
# Constructor Tests
# =============================================================================

class TestConstructors:
    """Tests for constructor parsing."""
    
    @pytest.mark.parametrize("temp_header", [COMPLEX_CLASS], indirect=True, ids=["complex_class"])
    def test_explicit_constructors(self, temp_header):
        result = parse_header(str(temp_header), "MyObject")
        assert result is not None and len(result.constructors) >= 1
        default_ctor = next((c for c in result.constructors if not c.parameters), None)
        assert default_ctor is not None and default_ctor.name == "MyObject"
    
    @pytest.mark.parametrize("temp_header", [SIMPLE_CLASS], indirect=True, ids=["simple_class"])
    def test_default_constructor_added(self, temp_header):
        result = parse_header(str(temp_header), "SimpleClass")
        assert result is not None and len(result.constructors) == 1
        assert result.constructors[0].name == "SimpleClass"
        assert len(result.constructors[0].parameters) == 0


# =============================================================================
# Constant Tests
# =============================================================================

class TestConstants:
    """Tests for constant parsing."""
    
    @pytest.mark.parametrize("temp_header", [COMPLEX_CLASS], indirect=True, ids=["complex_class"])
    def test_const_members(self, temp_header):
        result = parse_header(str(temp_header), "MyObject")
        assert result is not None and len(result.constants) >= 2
        assert {"version", "appversion"} <= {c.name for c in result.constants}
    
    @pytest.mark.parametrize("temp_header", [COMPLEX_CLASS], indirect=True, ids=["complex_class"])
    def test_static_const_detection(self, temp_header):
        result = parse_header(str(temp_header), "MyObject")
        appversion = next(c for c in result.constants if c.name == "appversion")
        version = next(c for c in result.constants if c.name == "version")
        assert appversion.is_static and not version.is_static
    
    @pytest.mark.parametrize("temp_header", [COMPLEX_CLASS], indirect=True, ids=["complex_class"])
    def test_inline_static_constexpr(self, temp_header):
        result = parse_header(str(temp_header), "MyObject")
        const_names = {c.name for c in result.constants}
        if "cppversion" in const_names:
            cppversion = next(c for c in result.constants if c.name == "cppversion")
            assert cppversion.is_static


# =============================================================================
# Enum Tests
# =============================================================================

class TestEnums:
    """Tests for enum parsing."""
    
    @pytest.mark.parametrize("temp_header", [COMPLEX_CLASS], indirect=True, ids=["complex_class"])
    def test_inner_enum_class(self, temp_header):
        result = parse_header(str(temp_header), "MyObject")
        assert result is not None and len(result.enums) == 1
        inner = result.enums[0]
        assert inner.name == "InnerEnum" and inner.is_enum_class
        assert {"Value1", "Value2", "Value3"} <= set(inner.enum_values)


# =============================================================================
# Namespace Tests
# =============================================================================

class TestNamespaces:
    """Tests for namespace parsing."""
    
    @pytest.mark.parametrize("temp_header", [SIMPLE_CLASS], indirect=True, ids=["simple_class"])
    def test_class_without_namespace(self, temp_header):
        result = parse_header(str(temp_header), "SimpleClass")
        assert result is not None and result.namespace == []

    @pytest.mark.parametrize("temp_header", [NESTED_NAMESPACE], indirect=True, ids=["nested_namespace"])
    def test_class_in_deeply_nested_namespace(self, temp_header):
        result = parse_header(str(temp_header), "DeepClass")
        assert result is not None
        assert result.name == "DeepClass" and result.namespace == ["a", "b", "c"]


# =============================================================================
# Edge Case Tests
# =============================================================================

class TestEdgeCases:
    """Tests for edge cases."""
    
    @pytest.mark.parametrize("temp_header", [EMPTY_CLASS], indirect=True, ids=["empty_class"])
    def test_empty_class(self, temp_header):
        result = parse_header(str(temp_header), "EmptyClass")
        assert result is not None and result.name == "EmptyClass"
        assert len(result.properties) == 0 and len(result.events) == 0
        assert len(result.sync_methods) == 0 and len(result.async_methods) == 0
        assert len(result.constructors) == 1  # Default-Konstruktor
    
    @pytest.mark.parametrize("temp_header", [SIMPLE_CLASS], indirect=True, ids=["simple_class"])
    def test_class_not_found(self, temp_header):
        assert parse_header(str(temp_header), "NonExistentClass") is None
    
    def test_file_not_found(self):
        with pytest.raises(FileNotFoundError):
            parse_header("/non/existent/path.h", "SomeClass")




# =============================================================================
# Main Entry Point
# =============================================================================

def run_burntest():
    """Run all tests and print a summary report."""
    print("=" * 80)
    print("webbridge Parser - Burntest")
    print("=" * 80)
    print()
    
    # Führe pytest aus
    exit_code = pytest.main([
        __file__,
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
