# Nyxptr ♟️

[![Release](https://img.shields.io/github/v/release/FrantisekSilhan/Nyxptr.svg)](https://github.com/FrantisekSilhan/Nyxptr/releases) [![License](https://img.shields.io/github/license/FrantisekSilhan/Nyxptr.svg)](https://github.com/FrantisekSilhan/Nyxptr/blob/master/LICENSE) [![Downloads](https://img.shields.io/github/downloads/FrantisekSilhan/Nyxptr/total.svg)](https://github.com/FrantisekSilhan/Nyxptr/releases) [![Stars](https://img.shields.io/github/stars/FrantisekSilhan/Nyxptr?style=social)](https://github.com/FrantisekSilhan/Nyxptr)

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20) [![CMake](https://img.shields.io/badge/CMake-3.19%2B-blue.svg)](CMakeLists.txt) [![Platforms](https://img.shields.io/badge/Platforms-CPU%20%7C%20CUDA%20%7C%20ROCm-yellow.svg)](#build-from-source) [![Model](https://img.shields.io/badge/Model-v4-brightgreen.svg)](https://github.com/FrantisekSilhan/Nyxptr/releases/latest/download/Nyxptr-Model-v4.zip)

Nyxptr is a C++20 chess engine built around a Torch-backed searcher, UCI support, self-play generation, PGN and puzzle conversion tools, and Syzygy tablebase probing.

## ✨ Highlights

- UCI engine mode for chess GUIs and analysis tools
- Monte Carlo tree search powered by a TorchScript model
- Self-play data generation with policy/value training data
- PGN-to-binary conversion helpers
- Lichess puzzle conversion helpers
- Syzygy probing for tablebase positions

## 📦 Releases

You can find pre-built binaries on the [Releases](https://github.com/FrantisekSilhan/Nyxptr/releases) page.

- **Windows (CPU):** The standard release (`Nyxptr-v0.0.4-windows-cpu.zip`) includes everything needed to run on Windows using the CPU version of libtorch.
- **Other Platforms/GPU:** Currently, there are no pre-built binaries for Linux or GPU-accelerated (CUDA/ROCm) versions. If you wish to use GPU acceleration or run on Linux, please follow the **Build** instructions below.

## 🚀 Quick Start

1. Download a release or build from source.
2. Model File: Ensure you have the model file at `model/nyxptr_v4.pt`. (This is already included in the Windows release ZIP).
3. Run the engine binary directly or through `run.bat` on Windows.
4. For GUI use (e.g., [Nibbler](https://github.com/rooklift/nibbler)), point the GUI to the executable.

## ⚙️ Requirements

- **Model File:** A trained TorchScript model (`model/nyxptr_v4.pt`) is required. You can download the latest version from the [Releases](https://github.com/FrantisekSilhan/Nyxptr/releases) page.
- **Tablebases:** (Optional) Syzygy tablebases in a `tablebases/` directory.
- **Build Dependencies:**
  - CMake 3.19+
  - C++20 compatible compiler (MSVC 2022, GCC 11+, or Clang 12+)
  - `libtorch` (Match the version to your hardware: CPU, CUDA, or ROCm)

## 🛠️ Build from Source

### 🪟 Windows

1. Download the `libtorch` c++ zip from the [PyTorch website](https://pytorch.org/get-started/locally/).
2. Extract it into the repository root so that the `libtorch` folder is visible to CMake.
3. Run
  ```powershell
  cmake --preset vs2022-x64
  cmake --build --preset vs2022-x64-release
  ```

The executable will be located at `out/build/vs2022-x64/Release/Nyxptr.exe`.

### 🐧 Linux

The repository includes a Linux preset that expects libtorch at `/opt/libtorch`:

```bash
cmake --preset linux-rocm
cmake --build --preset linux-release
```

Update `CMakePresets.json` or pass `-DTorch_DIR=...` if your libtorch is installed elsewhere.

## ▶️ Run & Command Line Modes

By default, Nyxptr starts in **UCI mode**. It also supports specific utility tasks:

- `--uci`: Start the UCI loop explicitly.
- `--selfplay <games> <simsPerMove> [outputFile]`: Generate training data.
- `--convert <pgnDir> <binDir>`: Convert PGN files to binary training format.
- `--convert-puzzles <csvPath> <binPathBase>`: Convert Lichess puzzle CSVs.

Example:
```bash
Nyxptr.exe --selfplay 50 800 selfplay_data.bin
```

## 🗂️ Repository Layout

- `src/` & `include/` - Core engine logic and entry points.
- `ext/Fathom/` - Syzygy probing source (MIT License).
- `run.bat` - Windows helper to launch the Release build
- `CMakeLists.txt` and `CMakePresets.json` - build configuration
- `out/` - Default build output directory.

## 📝 Notes

- **Hardware Acceleration:** While the Windows release uses the CPU, Nyxptr supports GPU acceleration. For maximum search speed, it is recommended to compile the project yourself against the **CUDA** (NVIDIA) or **ROCm** (AMD) versions of libtorch.

## 📄 License & Legal

- **Nyxptr Engine:** Distributed under the [GNU General Public License v3.0](LICENSE).
- **Third-Party Code:** This repository bundles [Fathom](https://github.com/jdart1/Fathom) for Syzygy probing (MIT License). See `ext/Fathom/` for details and third-party notices.
- **Binary Distributions:** Pre-built releases contain redistributable binaries from the PyTorch project and Intel. Full attribution and license texts for these dependencies are included in the `NOTICE` file within the release ZIP.
