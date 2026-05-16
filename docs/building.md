# Building MobuFaceMotion

Windows only. MotionBuilder is a Windows-only product in the supported
2022-2026 range, and the plugin uses Winsock directly.

## Prerequisites

- MotionBuilder (2022, 2023, 2024, 2025, or 2026) with the
  **OpenRealitySDK** component installed. The installer drops it at:
  `C:\Program Files\Autodesk\MotionBuilder <version>\OpenRealitySDK\`
- Visual Studio 2019 or 2022 with the C++ desktop workload (MSVC v142 or
  v143). Match the toolset Autodesk used for the target version:
  - MoBu 2022: VS 2019 (v142)
  - MoBu 2023-2026: VS 2022 (v143)
- CMake 3.19 or newer.

## Build (default MoBu version from `PRODUCT_VERSION.txt`)

`PRODUCT_VERSION.txt` controls which MoBu the build targets. Default
is `2026`. To build for a different version, either edit that file or
pass `-DMOBU_VERSION=<ver>` at configure time.

From the repo root:

```powershell
# Build for the version named in PRODUCT_VERSION.txt
cmake -S . -B build/2026 -G "Visual Studio 17 2022" -A x64
cmake --build build/2026 --config Release
```

`COPY_TO_PLUGINS` defaults to `ON`, so the resulting `.dll` is copied
into `C:\Program Files\Autodesk\MotionBuilder 2026\bin\x64\plugins\`
automatically. **Close MotionBuilder first** if it's running — the copy
will fail if MoBu has the previous `.dll` loaded.

## Build for a specific MoBu version

```powershell
cmake -S . -B build/2024 -G "Visual Studio 17 2022" -A x64 -DMOBU_VERSION=2024
cmake --build build/2024 --config Release
```

## Build for all supported versions

```powershell
foreach ($v in 2022, 2023, 2024, 2025, 2026) {
    cmake -S . -B "build/$v" -G "Visual Studio 17 2022" -A x64 -DMOBU_VERSION=$v
    cmake --build "build/$v" --config Release
}
```

## Custom SDK location

If your MotionBuilder install is not under `C:\Program Files\Autodesk\...`,
override `MOBU_ROOT` (and optionally `OPENREALITY_ROOT`) at configure:

```powershell
cmake -S . -B build/2026 -G "Visual Studio 17 2022" -A x64 `
    -DMOBU_VERSION=2026 `
    -DMOBU_ROOT="D:/Tools/MotionBuilder 2026" `
    -DOPENREALITY_ROOT="D:/Tools/MotionBuilder 2026/OpenRealitySDK"
```

## Disabling auto-install

Pass `-DCOPY_TO_PLUGINS=OFF` to leave the built `.dll` only in the
CMake binary directory:

```powershell
cmake -S . -B build/2026 -G "Visual Studio 17 2022" -A x64 -DCOPY_TO_PLUGINS=OFF
```

The `.dll` will land in `build/2026/src/device_faceMotion/Release/device_faceMotion.dll`.
Copy it manually into `<MoBu>\bin\x64\plugins\` when ready.

## Verifying the install

1. Launch MotionBuilder.
2. Open **Asset Browser -> Devices**. A "FaceMotion (iFacialMocap /
   FaceMotion3D)" entry should appear.
3. Drag it into the scene. A `FaceMotion` Reference null with `Head`,
   `LeftEye`, and `RightEye` children should be created.
4. Open the device, switch to the Communication tab. Toggle Protocol
   between the three entries and confirm Phone Port flips between 49983
   and 49993; toggle Transport to TCP and confirm Listen Port flips to
   49986.

## Troubleshooting

- **`fbsdk.h` not found at configure time**: the OpenRealitySDK component
  wasn't installed with MotionBuilder. Re-run the Autodesk installer and
  add it.
- **Link error on `ws2_32`**: shouldn't happen since the CMake target
  links it explicitly; if it does, ensure you're configuring a Windows
  generator (Visual Studio, not MinGW).
- **`.dll` is built but the device doesn't appear in MoBu**: confirm the
  copy hit the right plugins folder (the build output shows the
  destination); restart MotionBuilder (plugins are loaded once at
  startup); check `<MoBu>\bin\x64\config\Plugins\` log files if any exist.
