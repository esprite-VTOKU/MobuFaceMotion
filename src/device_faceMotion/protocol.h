#pragma once

/** \file protocol.h
 *
 *  Wire-format definitions and parser entry point for the iFacialMocap and
 *  FaceMotion3D iPhone face-capture streaming protocols.
 *
 *  Both protocols transmit a single text packet per frame. The packet body
 *  is identical except for the blendshape pair delimiter ('-' in iFacialMocap
 *  v1, '&' in iFacialMocap v2 and FaceMotion3D).
 *
 *  iFacialMocap reference: https://www.ifacialmocap.com/for-developer/
 *  FaceMotion3D reference: https://www.facemotion3d.net/english/for-developer/
 */

#include <cstdint>
#include <string_view>

namespace mobufacemotion {

enum class EProtocol : uint8_t
{
    iFacialMocapV1 = 0,  // legacy: blendshape pairs use '-' separator
    iFacialMocapV2 = 1,  // signed values, '&' separator
    FaceMotion3D   = 2,  // same body grammar as v2, different port/handshake
};

enum class ETransport : uint8_t
{
    UDP = 0,
    TCP = 1,
};

constexpr uint32_t kBlendshapeCount = 52;

/// ARKit blendshape names in canonical camelCase, in the order MotionBuilder
/// animation nodes are created. Indexing into kArkitBlendshapes[] gives the
/// name; BlendshapeIndex() does the reverse lookup.
extern const char* const kArkitBlendshapes[kBlendshapeCount];

/// One parsed frame. All blendshape values are in the [-100, 100] range
/// (iFacialMocap v2 / FaceMotion3D may emit negatives; v1 is always [0,100]).
struct ParsedFrame
{
    double   blendshape[kBlendshapeCount] = {};
    double   headRotDeg[3] = {};   // Euler XYZ in degrees
    double   headPosCm[3]  = {};   // translation in centimeters
    double   rightEyeDeg[3] = {};  // Euler XYZ in degrees
    double   leftEyeDeg[3]  = {};
    uint64_t blendshapeMask = 0;   // bit i set => blendshape[i] was present
    bool     hasHead  = false;
    bool     hasLeftEye  = false;
    bool     hasRightEye = false;
};

/// Look up an ARKit blendshape name (case-sensitive camelCase). Returns
/// kBlendshapeCount if not found.
uint32_t BlendshapeIndex(std::string_view name);

/// Handshake payload to send to the iPhone to begin streaming.
std::string_view HandshakeFor(EProtocol p, ETransport t);

/// Port on the iPhone the handshake must be sent to (always UDP, even for
/// TCP transport — the handshake itself is always UDP).
int IPhonePortFor(EProtocol p);

/// Suggested PC-side listen port for a given (protocol, transport) pair.
/// iPhone apps stream back to this port. For TCP transport this is always
/// 49986; for UDP it matches the iPhone port (49983 or 49993).
int DefaultListenPortFor(EProtocol p, ETransport t);

/// Parse one received packet into `out`. Returns true if at least one field
/// (any blendshape, head, or eye block) was successfully read.
///
/// The parser is permissive: unknown tokens are skipped; both '&' and '-'
/// pair delimiters are accepted regardless of the declared protocol, so a
/// mismatched UI setting still produces useful data.
bool ParsePacket(const char* buf, int len, EProtocol p, ParsedFrame& out);

} // namespace mobufacemotion
