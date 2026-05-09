# Nyxptr ♟️

Nyxptr is a C++20 chess engine built around a Torch-backed searcher, UCI support, self-play generation, PGN and puzzle conversion tools, and Syzygy tablebase probing.

## ✨ Highlights

- UCI engine mode for chess GUIs and analysis tools
- Monte Carlo tree search powered by a TorchScript model
- Self-play data generation with policy/value training data
- PGN-to-binary conversion helpers
- Lichess puzzle conversion helpers
- Syzygy probing for tablebase positions

## 🚀 Quick Start

1. Install CMake 3.19+, a C++20 compiler, and Torch / libtorch.
2. On Windows, put `libtorch` in the repository root. On Linux, install libtorch and update the CMake preset to point to its path (`Torch_DIR` in `CMakePresets.json`).
3. Configure and build with CMake presets.
4. Run the engine binary directly or through `run.bat` on Windows.

## 🗂️ Repository Layout

- `src/` - application entry point and engine/game implementation files
- `include/` - public headers for the engine and game modules
- `ext/Fathom/` - bundled Syzygy probing sources
- `run.bat` - Windows helper to launch the Release build
- `CMakeLists.txt` and `CMakePresets.json` - build configuration

## ⚙️ Requirements

- CMake 3.19 or newer
- A C++20 compiler
- Torch / libtorch
- On Windows, place `libtorch` in the repository root so CMake can find it
- A trained model file (`model/nyxptr_v3_epoch3.pt` by default) — required for the searcher to run
- Optional: a `tablebases/` directory for Syzygy support

## 🛠️ Build

### 🪟 Windows

From the repository root:

```powershell
cmake --preset vs2022-x64
cmake --build --preset vs2022-x64-release
```

The resulting executable is expected at `out/build/vs2022-x64/Release/Nyxptr.exe`.

### 🐧 Linux

The repository includes a Linux preset that expects libtorch at `/opt/libtorch`:

```bash
cmake --preset linux-rocm
cmake --build --preset linux-release
```

## ▶️ Run

By default, Nyxptr starts in UCI mode and waits for stdin commands such as `uci`, `isready`, `position`, `go`, and `quit`.

```bash
out/build/vs2022-x64/Release/Nyxptr.exe
```

On Windows, the provided helper launches the Release binary:

```bat
run.bat
```

If you want the engine in a GUI, point the GUI at the built executable and use standard UCI mode. Example: [Nibbler](https://github.com/rooklift/nibbler).

## ⌨️ Command Line Modes

Nyxptr supports a few utility modes from `src/main.cpp`:

- `--uci` - start the UCI loop explicitly
- `--selfplay <games> <simsPerMove> [outputFile]` - generate self-play data
- `--convert <pgnDir> <binDir>` - convert PGN files in a directory to binary files
- `--convert-puzzles <csvPath> <binPathBase>` - convert Lichess puzzle CSV data

Examples:

```bash
Nyxptr.exe --selfplay 50 800 selfplay_data.bin
Nyxptr.exe --convert data/pgn data/bin
Nyxptr.exe --convert-puzzles puzzles.csv puzzles
```

## 📝 Notes

- A trained model is **required** to run the engine. No public model is currently available, but Nyxptr V4 will be released on [GitHub Releases](https://github.com/FrantisekSilhan/Nyxptr/releases) once trained.
- The default model path is hard-coded in `src/main.cpp` as `model/nyxptr_v3_epoch3.pt`; update it if you store the model elsewhere.
- If Syzygy tablebases cannot be initialized, the engine prints a warning and continues.
- Self-play mode tunes the search parameters for data generation instead of regular play.

## 📄 License

Nyxptr is distributed under the GNU General Public License v3.0 or later. See [LICENSE](LICENSE).
