# iFacialMocap and FaceMotion3D wire protocols

This is a self-contained reference for the two text-based streaming
protocols MobuFaceMotion implements. Source pages:

- [iFacialMocap for developers](https://www.ifacialmocap.com/for-developer/)
- [Facemotion3D for developers](https://www.facemotion3d.net/english/for-developer/)

Both apps use the same body grammar but different handshakes, ports, and
delimiter rules. The plugin's parser ([protocol.cxx](../src/device_faceMotion/protocol.cxx))
treats them as a single dialect with a flexible delimiter rule.

## Handshake

All handshakes are sent **over UDP** from the PC to the iPhone, even when
the actual frame stream will be TCP. The phone responds by opening the
return stream.

| App + transport | iPhone port | Handshake payload | PC return port |
|---|---|---|---|
| iFacialMocap UDP, v1 (legacy) | 49983 | `iFacialMocap_sahuasouryya9218sauhuiayeta91555dy3719` | 49983 (UDP) |
| iFacialMocap UDP, v2          | 49983 | `iFacialMocap_sahuasouryya9218sauhuiayeta91555dy3719|sendDataVersion=v2` | 49983 (UDP) |
| iFacialMocap TCP, v1          | 49983 | `iFacialMocap_UDPTCP_sahuasouryya9218sauhuiayeta91555dy3719` | 49986 (TCP) |
| iFacialMocap TCP, v2          | 49983 | `iFacialMocap_UDPTCP_sahuasouryya9218sauhuiayeta91555dy3719|sendDataVersion=v2` | 49986 (TCP) |
| FaceMotion3D UDP              | 49993 | `FACEMOTION3D_OtherStreaming` | 49993 (UDP) |
| FaceMotion3D TCP              | 49993 | `FACEMOTION3D_OtherStreaming|protocol=tcp` | 49986 (TCP) |

UDP datagrams can be lost; FaceMotion3D's docs recommend sending the
handshake three times for safety. MobuFaceMotion's hardware layer sends
every handshake three times unconditionally.

## Frame payload

One packet per frame, ~60 FPS. Pipe-separated tokens of three kinds.

### Blendshape tokens

```
<name><sep><value>
```

`<name>` is an ARKit blendshape in camelCase (`browInnerUp`, `eyeBlinkLeft`,
`jawOpen`, etc.). `<sep>` is:

- `-` in iFacialMocap v1
- `&` in iFacialMocap v2 and FaceMotion3D (chosen because `-` collides with
  the value's leading minus sign when negative values are allowed)

`<value>` is a real number, conventionally in `0..100` for v1 and
`-100..100` for v2 / FaceMotion3D.

The parser tries `&` first, then `-`, so mismatched protocol settings still
decode correctly for v1-style packets.

### Head transform

```
=head#<rx>,<ry>,<rz>,<tx>,<ty>,<tz>
```

Six comma-separated reals: Euler XYZ rotation in **degrees**, then XYZ
translation in centimeters. The leading `=` is a literal `=` character.

### Eye rotations

```
rightEye#<rx>,<ry>,<rz>
leftEye#<rx>,<ry>,<rz>
```

Euler XYZ rotation in degrees. (Z is usually zero; MobuFaceMotion forces
Z to zero on the way out, matching the OpenMoBu reference behaviour.)

### Suffix

iFacialMocap packets end with `|___iFacialMocap`. FaceMotion3D packets do
not include this. The parser silently ignores any token that begins with
`___`, so both forms parse identically.

## Example packets

### iFacialMocap v1 (UDP, `-` delimiter)

```
browInnerUp-0|browDownLeft-0|...|jawOpen-25|...|=head#1.2,-3.4,0.0,0.0,0.0,30.0|rightEye#0,0,0|leftEye#0,0,0|___iFacialMocap
```

### iFacialMocap v2 / FaceMotion3D (`&` delimiter)

```
browInnerUp&0|browDownLeft&0|...|jawOpen&25|...|=head#1.2,-3.4,0.0,0.0,0.0,30.0|rightEye#0,0,0|leftEye#0,0,0
```

## TCP framing notes

In practice, each frame arrives in a single TCP segment with current
iPhone-side senders. Receivers should be prepared to buffer across
segments if framing is observed in the wild — MobuFaceMotion's TCP path
currently treats each `recv()` return as one frame and will need a small
buffering loop if that assumption ever breaks.

## 52 ARKit blendshape names (canonical order)

Used as both animation-node output names on the device and as the lookup
table for `BlendshapeIndex()`:

```
browInnerUp
browDownLeft, browDownRight
browOuterUpLeft, browOuterUpRight
eyeLookUpLeft, eyeLookUpRight
eyeLookDownLeft, eyeLookDownRight
eyeLookInLeft, eyeLookInRight
eyeLookOutLeft, eyeLookOutRight
eyeBlinkLeft, eyeBlinkRight
eyeSquintLeft, eyeSquintRight
eyeWideLeft, eyeWideRight
cheekPuff
cheekSquintLeft, cheekSquintRight
noseSneerLeft, noseSneerRight
jawOpen, jawForward, jawLeft, jawRight
mouthFunnel, mouthPucker
mouthLeft, mouthRight
mouthRollUpper, mouthRollLower
mouthShrugUpper, mouthShrugLower
mouthClose
mouthSmileLeft, mouthSmileRight
mouthFrownLeft, mouthFrownRight
mouthDimpleLeft, mouthDimpleRight
mouthUpperUpLeft, mouthUpperUpRight
mouthLowerDownLeft, mouthLowerDownRight
mouthPressLeft, mouthPressRight
mouthStretchLeft, mouthStretchRight
tongueOut
```
