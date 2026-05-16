# MobuFaceMotion

A native Autodesk MotionBuilder device plugin that streams ARKit facial
capture data from the **iFacialMocap** and **FaceMotion3D** iPhone apps.

Inspired by — and structurally derived from — the `device_faceCap` plugin
in [OpenMoBu](https://github.com/Neill3d/OpenMoBu), which streams from the
FaceCap iPhone app over OSC. MobuFaceMotion targets the two iFacialMocap-
family text protocols instead (no OSC dependency).

## Status

**v0.1.0 — first tester release.** A built `.dll` for MotionBuilder 2027 is
attached to the [latest GitHub release](https://github.com/esprite-VTOKU/MobuFaceMotion/releases/latest);
see *Quick install* below.

End-to-end streaming verified against iFacialMocap on iPhone (UDP).
FaceMotion3D / TCP paths share the same parser and are expected to work;
testing feedback welcome.

## Quick install (MoBu 2027)

1. **Close MotionBuilder** if it's running.
2. Download `device_faceMotion.dll` from the
   [latest release](https://github.com/esprite-VTOKU/MobuFaceMotion/releases/latest).
3. Copy it into `C:\Program Files\Autodesk\MotionBuilder 2027\bin\x64\plugins\`
   (requires admin).
4. Launch MotionBuilder. Asset Browser -> Devices -> drag *FaceMotion
   (iFacialMocap / FaceMotion3D)* into the scene.
5. On the device's Communication tab: set Protocol, enter your iPhone's
   LAN IP, toggle **Online** then **Live**.
6. The **Live** tab shows the streaming status, frame counter, and the
   live head/eye/blendshape values.

## Supported MotionBuilder versions

2022 - 2027 (source-level). Pre-built `.dll` currently attached only for
**2027**. To build for a different version, set `PRODUCT_VERSION.txt`
(or pass `-DMOBU_VERSION=<ver>` at configure) and see
[docs/building.md](docs/building.md).

## Supported source apps

| App | Transport | iPhone port | PC listen port |
|---|---|---|---|
| iFacialMocap (v1, legacy `-` delimiter) | UDP | 49983 | 49983 |
| iFacialMocap (v2, `&` delimiter, signed values) | UDP | 49983 | 49983 |
| iFacialMocap | TCP | 49983 (handshake) | 49986 |
| FaceMotion3D | UDP | 49993 | 49993 |
| FaceMotion3D | TCP | 49993 (handshake) | 49986 |

Default on first launch: **iFacialMocap v2 over UDP** (handles negative
blendshape values; otherwise identical wire payload to v1).

See [docs/protocols.md](docs/protocols.md) for the full wire format.

## What lands in MotionBuilder

Once the device is added and brought Online + Live, the plugin exposes:

- A **`FaceMotion` Reference** node with `Head`, `LeftEye`, `RightEye` markers.
  Head receives translation + rotation; eyes receive rotation.
- 52 ARKit blendshape weights as named `FBAnimationNode` outputs on the device
  (`browInnerUp`, `eyeBlinkLeft`, `jawOpen`, etc. — camelCase, matching the
  on-the-wire names exactly).
- *Optional:* drag any face mesh into the device's `Blendshape Target` slot
  and the plugin will look up that mesh's Blendshape deformer channels by name
  (case-insensitive) and drive them live — no Relations constraint wiring
  required.

Use Set Candidate to key the animation; otherwise data drives the bound
markers/properties in real time.

## Quick start (once built and installed)

1. **iPhone**: open iFacialMocap or FaceMotion3D, note the device's LAN IP.
2. **MotionBuilder**: Asset Browser -> Devices -> drag *FaceMotion Device*
   into the scene.
3. In the device's Communication tab, select your **Protocol** and
   **Transport**, enter the iPhone's IP in **Phone IP**.
4. Toggle the device **Online**, then **Live**.
5. (Optional) Drag a face mesh into **Blendshape Target** to drive its
   deformer channels directly.

## Layout

```
MobuFaceMotion/
  CMakeLists.txt              root build (per-MoBu-version)
  cmake/OpenReality.cmake     fbsdk import helper
  PRODUCT_VERSION.txt         selects the MoBu version to build against
  src/device_faceMotion/      plugin sources (one .dll target)
  docs/                       protocol + build documentation
```

## License

3-clause BSD. See [LICENSE](LICENSE). The scaffolding is adapted from
OpenMoBu (also 3-clause BSD); the original Neill3d copyright is preserved
alongside the new project copyright.
