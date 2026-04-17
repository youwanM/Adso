# <img src="assets/logo.ico" width="80" height="80"> Adso – Anatomical Data Slice Observer 


A small Qt6 desktop application for loading and visualizing NIfTI medical images (`.nii`, `.nii.gz`).

## Features

- Open NIfTI volumes from disk
- Display 3 orthogonal views:
  - Sagittal (X)
  - Coronal (Y)
  - Axial (Z)
- Navigate slices with per-axis sliders
- Adjust image brightness and contrast in real time
- Show red crosshairs synchronized across all views
- Load/save volume data through the `NiftiVolume` utility

## Tech Stack

- C++17
- Qt 6 (`Core`, `Gui`, `Widgets`)
- CMake (FetchContent)
- NIfTI C library (`nifti_clib`)
- zlib
- Eigen 3

## Project Structure

- `main.cpp` — Qt application entry point
- `MainWindow.h/.cpp` — UI and rendering logic
- `niftiVolume.h/.cpp` — NIfTI load/save and tensor container
- `CMakeLists.txt` — build configuration and dependency fetching

## Build

> Note: `CMakeLists.txt` currently contains a Windows-specific Qt path:
> `C:/Qt/6.6.1/msvc2019_64`.
> Update `CMAKE_PREFIX_PATH` to match your local Qt installation.

### Windows (Qt + MSVC)

```bash
cmake -S . -B build -G "Ninja"
cmake --build build
```

The project includes a post-build `windeployqt` step (on Windows) to copy required Qt runtime DLLs next to the executable.

## Run

After building, run the generated executable (`Adso`), then:

1. Click **Open NIfTI Image**
2. Select a `.nii` or `.nii.gz` file
3. Explore slices with the sagittal/coronal/axial sliders
4. Tune brightness and contrast as needed

## License

This project is licensed under the terms in [`LICENSE.txt`](./LICENSE.txt).
