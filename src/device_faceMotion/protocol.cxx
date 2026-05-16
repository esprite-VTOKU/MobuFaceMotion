/** \file protocol.cxx */

#include "protocol.h"

#include <array>
#include <cstdlib>
#include <cstring>
#include <unordered_map>

namespace mobufacemotion {

const char* const kArkitBlendshapes[kBlendshapeCount] = {
    "browInnerUp",
    "browDownLeft",  "browDownRight",
    "browOuterUpLeft", "browOuterUpRight",
    "eyeLookUpLeft",   "eyeLookUpRight",
    "eyeLookDownLeft", "eyeLookDownRight",
    "eyeLookInLeft",   "eyeLookInRight",
    "eyeLookOutLeft",  "eyeLookOutRight",
    "eyeBlinkLeft",    "eyeBlinkRight",
    "eyeSquintLeft",   "eyeSquintRight",
    "eyeWideLeft",     "eyeWideRight",
    "cheekPuff",
    "cheekSquintLeft", "cheekSquintRight",
    "noseSneerLeft",   "noseSneerRight",
    "jawOpen", "jawForward", "jawLeft", "jawRight",
    "mouthFunnel", "mouthPucker",
    "mouthLeft", "mouthRight",
    "mouthRollUpper", "mouthRollLower",
    "mouthShrugUpper", "mouthShrugLower",
    "mouthClose",
    "mouthSmileLeft", "mouthSmileRight",
    "mouthFrownLeft", "mouthFrownRight",
    "mouthDimpleLeft", "mouthDimpleRight",
    "mouthUpperUpLeft", "mouthUpperUpRight",
    "mouthLowerDownLeft", "mouthLowerDownRight",
    "mouthPressLeft", "mouthPressRight",
    "mouthStretchLeft", "mouthStretchRight",
    "tongueOut",
};

namespace {

const std::unordered_map<std::string_view, uint32_t>& NameIndex()
{
    static const std::unordered_map<std::string_view, uint32_t> map = []() {
        std::unordered_map<std::string_view, uint32_t> m;
        m.reserve(kBlendshapeCount);
        for (uint32_t i = 0; i < kBlendshapeCount; ++i)
            m.emplace(kArkitBlendshapes[i], i);
        return m;
    }();
    return map;
}

/// Read up to `expected` comma-separated doubles from [p, end). Trailing
/// fields default to 0. Returns the number of fields actually parsed.
int ReadCommaDoubles(const char* p, const char* end, double* out, int expected)
{
    int n = 0;
    while (n < expected && p < end)
    {
        char* next = nullptr;
        out[n++] = std::strtod(p, &next);
        if (next == p) break;
        p = next;
        if (p < end && *p == ',') ++p;
    }
    for (int i = n; i < expected; ++i) out[i] = 0.0;
    return n;
}

/// Find the value-separator inside a blendshape token. Tries '&' first
/// (iFacialMocap v2 / FaceMotion3D), then '-' (iFacialMocap v1). Negative
/// values use a leading '-' on the value itself, which is fine because we
/// only split on the first occurrence (or the '&').
const char* FindPairSep(const char* p, const char* end)
{
    const char* amp = static_cast<const char*>(std::memchr(p, '&', end - p));
    if (amp) return amp;
    // Fall back to '-': skip the first char so a value like "-3.5" works only
    // when the name itself doesn't end with '-' (no ARKit name does).
    return static_cast<const char*>(std::memchr(p, '-', end - p));
}

} // namespace

uint32_t BlendshapeIndex(std::string_view name)
{
    const auto& map = NameIndex();
    auto it = map.find(name);
    return (it == map.end()) ? kBlendshapeCount : it->second;
}

std::string_view HandshakeFor(EProtocol p, ETransport t)
{
    switch (p)
    {
        case EProtocol::iFacialMocapV1:
            return t == ETransport::TCP
                ? std::string_view("iFacialMocap_UDPTCP_sahuasouryya9218sauhuiayeta91555dy3719")
                : std::string_view("iFacialMocap_sahuasouryya9218sauhuiayeta91555dy3719");
        case EProtocol::iFacialMocapV2:
            return t == ETransport::TCP
                ? std::string_view("iFacialMocap_UDPTCP_sahuasouryya9218sauhuiayeta91555dy3719|sendDataVersion=v2")
                : std::string_view("iFacialMocap_sahuasouryya9218sauhuiayeta91555dy3719|sendDataVersion=v2");
        case EProtocol::FaceMotion3D:
            return t == ETransport::TCP
                ? std::string_view("FACEMOTION3D_OtherStreaming|protocol=tcp")
                : std::string_view("FACEMOTION3D_OtherStreaming");
    }
    return {};
}

int IPhonePortFor(EProtocol p)
{
    return p == EProtocol::FaceMotion3D ? 49993 : 49983;
}

int DefaultListenPortFor(EProtocol p, ETransport t)
{
    return t == ETransport::TCP ? 49986 : IPhonePortFor(p);
}

bool ParsePacket(const char* buf, int len, EProtocol /*p*/, ParsedFrame& out)
{
    if (!buf || len <= 0) return false;

    out.blendshapeMask = 0;
    out.hasHead = out.hasLeftEye = out.hasRightEye = false;

    bool any = false;
    const char* cur = buf;
    const char* end = buf + len;

    while (cur < end)
    {
        const char* tok_end = static_cast<const char*>(std::memchr(cur, '|', end - cur));
        if (!tok_end) tok_end = end;

        const std::string_view tok(cur, tok_end - cur);

        if (tok.size() >= 6 && tok.compare(0, 6, "=head#") == 0)
        {
            const char* p = tok.data() + 6;
            double v[6];
            if (ReadCommaDoubles(p, tok.data() + tok.size(), v, 6) >= 3)
            {
                out.headRotDeg[0] = v[0];
                out.headRotDeg[1] = v[1];
                out.headRotDeg[2] = v[2];
                out.headPosCm[0]  = v[3];
                out.headPosCm[1]  = v[4];
                out.headPosCm[2]  = v[5];
                out.hasHead = true;
                any = true;
            }
        }
        else if (tok.size() >= 9 && tok.compare(0, 9, "rightEye#") == 0)
        {
            const char* p = tok.data() + 9;
            double v[3];
            if (ReadCommaDoubles(p, tok.data() + tok.size(), v, 3) >= 1)
            {
                out.rightEyeDeg[0] = v[0];
                out.rightEyeDeg[1] = v[1];
                out.rightEyeDeg[2] = v[2];
                out.hasRightEye = true;
                any = true;
            }
        }
        else if (tok.size() >= 8 && tok.compare(0, 8, "leftEye#") == 0)
        {
            const char* p = tok.data() + 8;
            double v[3];
            if (ReadCommaDoubles(p, tok.data() + tok.size(), v, 3) >= 1)
            {
                out.leftEyeDeg[0] = v[0];
                out.leftEyeDeg[1] = v[1];
                out.leftEyeDeg[2] = v[2];
                out.hasLeftEye = true;
                any = true;
            }
        }
        else if (tok.empty() || tok[0] == '=' || tok[0] == '_')
        {
            // Empty token, leading '=' (heading marker), or trailing
            // '___iFacialMocap' suffix sentinel: skip silently.
        }
        else
        {
            const char* sep = FindPairSep(tok.data(), tok.data() + tok.size());
            if (sep && sep > tok.data() && sep + 1 < tok.data() + tok.size())
            {
                const std::string_view name(tok.data(), sep - tok.data());
                const uint32_t idx = BlendshapeIndex(name);
                if (idx < kBlendshapeCount)
                {
                    char* endptr = nullptr;
                    const double value = std::strtod(sep + 1, &endptr);
                    if (endptr != sep + 1)
                    {
                        out.blendshape[idx] = value;
                        out.blendshapeMask |= (uint64_t(1) << idx);
                        any = true;
                    }
                }
            }
        }

        cur = (tok_end < end) ? tok_end + 1 : end;
    }

    return any;
}

} // namespace mobufacemotion
