# openfst-py

Multiplatform Python library for **[OpenFst](https://www.openfst.org/)** (Weighted Finite-State Transducers), fully compatible with OpenFst's official `pywrapfst` API across all major operating systems and CPU architectures.

[![Build and Release Wheels](https://github.com/google-research/openfst/actions/workflows/release-py.yml/badge.svg)](https://github.com/google-research/openfst/actions/workflows/release-py.yml)
[![PyPI version](https://img.shields.io/pypi/v/openfst-py.svg)](https://pypi.org/project/openfst-py/)

---

## 🚀 Key Features

- **100% OpenFst `pywrapfst` Compatible**: Seamlessly drop into existing workflows (ASR, NLP, Kaldi, Pynini) that expect `import pywrapfst as fst` or `import openfst`.
- **Multiplatform & Multi-Architecture**: Prebuilt wheels for:
  - **Linux**: `x86_64`, `aarch64` (ARM64), `i686` (32-bit), `ppc64le`, `s390x` (manylinux & musllinux)
  - **macOS**: Apple Silicon (`arm64`), Intel (`x86_64`), and `universal2`
  - **Windows**: `AMD64` (64-bit), `x86` (32-bit), `ARM64`
- **Self-Contained**: Statically bundles OpenFst and Abseil with zero external runtime library dependencies.
- **Full Type Hints**: Built-in type stub definitions (`pywrapfst.pyi`, `py.typed`) for IDE auto-completion and static type checkers (`mypy`, `pyright`).

---

## 📦 Installation

```bash
pip install openfst-py
```

---

## 💡 Quick Start

You can import and use the library in any of the following standard ways:

### 1. Standard OpenFst syntax (`pywrapfst`)

```python
import pywrapfst as fst

# Create a mutable Vector FST
f = fst.VectorFst()

# Add states
s0 = f.add_state()
s1 = f.add_state()
f.set_start(s0)

# Add transition: state 0 -> state 1 (input 'a', output 'b', weight 0.5)
f.add_arc(s0, fst.Arc(97, 98, 0.5, s1))
f.set_final(s1, 1.0)

# Verify validity
assert f.verify()
print("Number of states:", f.num_states())
```

### 2. Standard `openfst` package syntax

```python
import openfst as fst

compiler = fst.Compiler()
print("0 1 a b 0.5", file=compiler)
print("1 0.0", file=compiler)
f = compiler.compile()
print(f.text())
```

### 3. Direct `openfst_py` namespace

```python
import openfst_py as fst

v = fst.VectorFst()
```

---

## 🛠️ Supported Architectures & Platforms

| Platform | Architectures | Wheel Type |
| :--- | :--- | :--- |
| **Linux** | `x86_64`, `aarch64`, `i686`, `ppc64le`, `s390x` | `manylinux2014`, `manylinux_2_28`, `musllinux_1_2` |
| **macOS** | `arm64` (Apple Silicon M1/M2/M3/M4), `x86_64` (Intel), `universal2` | `macosx_10_15_x86_64`, `macosx_11_0_arm64`, `macosx_11_0_universal2` |
| **Windows** | `AMD64` (x86_64), `x86` (32-bit), `ARM64` | `win_amd64`, `win32`, `win_arm64` |

---

## 📄 License

Apache License 2.0.
