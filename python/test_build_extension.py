#!/usr/bin/env python3
# Copyright (c) ZeroC, Inc.
#
# Test script to verify the Python extension can be built.
# This script tests the build process without creating a full distribution.

import os
import subprocess
import sys
import shutil


def run_command(cmd, cwd=None, check=True):
    """Run a command and return the result."""
    print(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    if result.stdout:
        print(result.stdout)
    if result.stderr:
        print(result.stderr, file=sys.stderr)
    if check and result.returncode != 0:
        raise RuntimeError(f"Command failed with return code {result.returncode}")
    return result


def test_sdist():
    """Test creating a source distribution."""
    print("\n=== Testing source distribution (sdist) ===")
    script_dir = os.path.dirname(os.path.abspath(__file__))

    # Check if C++ build artifacts exist
    cpp_dir = os.path.join(script_dir, "..", "cpp")
    slice2py_exists = (
        os.path.exists(os.path.join(cpp_dir, "bin", "slice2py")) or
        os.path.exists(os.path.join(cpp_dir, "bin", "slice2py.exe"))
    )

    if not slice2py_exists:
        print("⚠ slice2py not found in ../cpp/bin/")
        print("  The C++ sources need to be built first.")
        print("  Run: make -C ../cpp slice2py slice2cpp generate-srcs")
        return None

    # Clean previous builds
    dist_dir = os.path.join(script_dir, "dist")
    if os.path.exists(dist_dir):
        print(f"Cleaning {dist_dir}")
        shutil.rmtree(dist_dir)

    # Create dist directory
    os.makedirs(dist_dir, exist_ok=True)

    # Run sdist
    result = run_command(
        [sys.executable, "-m", "build", "--sdist"],
        cwd=script_dir,
        check=False
    )

    if result.returncode == 0:
        print("\n✓ Source distribution created successfully!")
        return True
    else:
        print("\n✗ Source distribution creation failed")
        return False


def test_build_ext():
    """Test building the extension module."""
    print("\n=== Testing extension build (build_ext) ===")
    script_dir = os.path.dirname(os.path.abspath(__file__))

    # Check if dist directory exists, which is required for build_ext
    dist_lib_dir = os.path.join(script_dir, "dist", "lib")
    if not os.path.exists(dist_lib_dir):
        print("⚠ dist/lib directory does not exist.")
        print("  This is expected - the directory is created during 'sdist'.")
        print("  To build the extension, first run: python -m build --sdist")
        return None

    result = run_command(
        [sys.executable, "setup.py", "build_ext", "--inplace"],
        cwd=script_dir,
        check=False
    )

    if result.returncode == 0:
        print("\n✓ Extension built successfully!")
        return True
    else:
        print("\n✗ Extension build failed")
        return False


def test_import():
    """Test importing the built extension."""
    print("\n=== Testing import ===")
    try:
        import IcePy
        print(f"✓ IcePy module imported successfully!")
        print(f"  Version info available: {hasattr(IcePy, 'stringVersion')}")
        return True
    except ImportError as e:
        print(f"✗ Failed to import IcePy: {e}")
        return False


def test_pyproject_validity():
    """Test that pyproject.toml is valid."""
    print("\n=== Testing pyproject.toml validity ===")
    script_dir = os.path.dirname(os.path.abspath(__file__))
    pyproject_path = os.path.join(script_dir, "pyproject.toml")

    if not os.path.exists(pyproject_path):
        print("✗ pyproject.toml not found")
        return False

    try:
        import tomllib
    except ImportError:
        try:
            import tomli as tomllib
        except ImportError:
            print("⚠ tomllib/tomli not available, skipping TOML validation")
            return True

    try:
        with open(pyproject_path, "rb") as f:
            config = tomllib.load(f)
        print(f"✓ pyproject.toml is valid TOML")
        print(f"  Project name: {config.get('project', {}).get('name', 'N/A')}")
        print(f"  Version: {config.get('project', {}).get('version', 'N/A')}")
        return True
    except Exception as e:
        print(f"✗ pyproject.toml parsing failed: {e}")
        return False


def main():
    """Run all tests."""
    print("=" * 60)
    print("Ice Python Extension Build Test")
    print("=" * 60)

    results = {}

    # Test pyproject.toml validity
    results["pyproject"] = test_pyproject_validity()

    # Check if we can run build tests (requires the C++ sources to be built)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    cpp_dir = os.path.join(script_dir, "..", "cpp")

    if os.path.exists(cpp_dir):
        print("\nC++ source directory found. Full build tests available.")

        # Check for build module
        try:
            import build
            results["sdist"] = test_sdist()
        except ImportError:
            print("\n⚠ 'build' module not installed. Install with: pip install build")
            results["sdist"] = None

        # Test build_ext
        results["build_ext"] = test_build_ext()

        # Test import (only if build_ext succeeded)
        if results.get("build_ext"):
            results["import"] = test_import()
        else:
            results["import"] = None
    else:
        print("\n⚠ C++ source directory not found. Skipping build tests.")
        print("   Build tests require the full Ice source tree.")
        results["sdist"] = None
        results["build_ext"] = None
        results["import"] = None

    # Summary
    print("\n" + "=" * 60)
    print("Test Summary")
    print("=" * 60)

    for test_name, result in results.items():
        if result is True:
            status = "✓ PASS"
        elif result is False:
            status = "✗ FAIL"
        else:
            status = "⚠ SKIP"
        print(f"  {test_name}: {status}")

    # Return exit code
    failures = sum(1 for r in results.values() if r is False)
    return failures


if __name__ == "__main__":
    sys.exit(main())
