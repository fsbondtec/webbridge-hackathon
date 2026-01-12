#!/usr/bin/env python3
"""
Burn test for tstypes (C++ → TypeScript type mapping)

Usage:
    python test_tstypes.py
    python -m pytest test_tstypes.py -v
"""

import pytest
from tstypes import cpp_to_ts_type

@pytest.mark.parametrize("cpp,ts", [
    ("int", "number"),
    ("double", "number"),
    ("bool", "boolean"),
    ("std::string", "string"),
    ("std::vector<int>", "number[]"),
    ("std::vector<std::string>", "string[]"),
    ("std::array<double, 5>", "number[]"),
    ("std::map<std::string, int>", "Record<string, number>"),
    ("std::unordered_map<std::string, bool>", "Record<string, boolean>"),
    ("std::vector<std::vector<int>>", "number[][]"),
    ("std::map<std::string, std::vector<double>>", "Record<string, number[]>"),
    ("std::map<int, int>", "unknown"),  # falscher Key
    ("std::pair<int, int>", "unknown"), # nicht unterstützt
    ("const std::vector<int>&", "number[]"),
    ("unsigned long long", "number"),
    ("nullptr_t", "null"),
])

def test_cpp_to_ts_type_burn(cpp, ts):
    assert cpp_to_ts_type(cpp) == ts

# =============================================================================
# Main Entry Point (Burntest-Runner)
# =============================================================================

def run_burntest():
    """Run all tests and print a summary report."""
    print("=" * 80)
    print("tstypes - Burntest")
    print("=" * 80)
    print()
    import pytest
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


