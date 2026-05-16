/** \file device_faceMotion_layout.cxx */

#include "device_faceMotion_layout.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <windows.h>

#include "protocol.h"

#define CDEVICEFACEMOTION__LAYOUT  CDevice_FaceMotion_Layout

FBDeviceLayoutImplementation(CDEVICEFACEMOTION__LAYOUT);
FBRegisterDeviceLayout(CDEVICEFACEMOTION__LAYOUT,
                       CDEVICEFACEMOTION__CLASSSTR,
                       "character_actor.png");

using namespace mobufacemotion;

namespace {

// Encode (protocol, transport) -> Source picker index. Order matches the
// items added in UIConfigureLayoutCommunication.
//
// v1 is the default and "iFacialMocap (UDP)" entry: it's the format Warudo
// and most other receivers actually decode. v2 is exposed as an explicit
// advanced option for users who need signed blendshape values.
int SourceIndexFor(int protocol, int transport)
{
    const EProtocol  p = static_cast<EProtocol>(protocol);
    const ETransport t = static_cast<ETransport>(transport);
    if (p == EProtocol::iFacialMocapV1 && t == ETransport::UDP) return 0;
    if (p == EProtocol::iFacialMocapV1 && t == ETransport::TCP) return 1;
    if (p == EProtocol::FaceMotion3D   && t == ETransport::UDP) return 2;
    if (p == EProtocol::FaceMotion3D   && t == ETransport::TCP) return 3;
    if (p == EProtocol::iFacialMocapV2 && t == ETransport::UDP) return 4;
    if (p == EProtocol::iFacialMocapV2 && t == ETransport::TCP) return 5;
    return 0;
}

// Decode Source picker index -> (protocol, transport).
void DecodeSource(int idx, EProtocol& proto, ETransport& trans)
{
    switch (idx)
    {
        default:
        case 0: proto = EProtocol::iFacialMocapV1; trans = ETransport::UDP; break;
        case 1: proto = EProtocol::iFacialMocapV1; trans = ETransport::TCP; break;
        case 2: proto = EProtocol::FaceMotion3D;   trans = ETransport::UDP; break;
        case 3: proto = EProtocol::FaceMotion3D;   trans = ETransport::TCP; break;
        case 4: proto = EProtocol::iFacialMocapV2; trans = ETransport::UDP; break;
        case 5: proto = EProtocol::iFacialMocapV2; trans = ETransport::TCP; break;
    }
}

// Pretty-format an age in milliseconds for the Live tab's "last packet" line.
// Tiers: <1s "N ms", <1min "S.S s", <1h "Mm Ss", <1d "Hh Mm", else "Dd Hh".
void FormatAge(int64_t ms, char* out, size_t cap)
{
    if (ms < 0)              { std::snprintf(out, cap, "-");                                       return; }
    if (ms < 1000)           { std::snprintf(out, cap, "%lld ms",   static_cast<long long>(ms));   return; }
    if (ms < 60000)          { std::snprintf(out, cap, "%.1f s",    ms / 1000.0);                  return; }
    if (ms < 3600000)        { std::snprintf(out, cap, "%lldm %llds",
                                  static_cast<long long>(ms / 60000),
                                  static_cast<long long>((ms / 1000) % 60));                       return; }
    if (ms < 86400000LL)     { std::snprintf(out, cap, "%lldh %lldm",
                                  static_cast<long long>(ms / 3600000),
                                  static_cast<long long>((ms / 60000) % 60));                      return; }
    std::snprintf(out, cap, "%lldd %lldh",
                  static_cast<long long>(ms / 86400000LL),
                  static_cast<long long>((ms / 3600000) % 24));
}

} // namespace

bool CDevice_FaceMotion_Layout::FBCreate()
{
    mDevice = static_cast<CDevice_FaceMotion*>(static_cast<FBDevice*>(Device));

    UICreate();
    UIConfigure();
    UIReset();

    mDevice->OnStatusChange.Add(this, (FBCallback)&CDevice_FaceMotion_Layout::EventDeviceStatusChange);
    OnIdle.Add                  (this, (FBCallback)&CDevice_FaceMotion_Layout::EventUIIdle);
    return true;
}

void CDevice_FaceMotion_Layout::FBDestroy()
{
    OnIdle.Remove                  (this, (FBCallback)&CDevice_FaceMotion_Layout::EventUIIdle);
    mDevice->OnStatusChange.Remove(this, (FBCallback)&CDevice_FaceMotion_Layout::EventDeviceStatusChange);
}

// -- UI construction ---------------------------------------------------------

void CDevice_FaceMotion_Layout::UICreate()
{
    const int lS = 4, lH = 25;

    AddRegion("TabPanel",   "TabPanel",
              0, kFBAttachLeft, "", 1.0,
              0, kFBAttachTop,  "", 1.0,
              0, kFBAttachRight,"", 1.0,
              lH,kFBAttachNone, nullptr, 1.0);
    AddRegion("MainLayout", "MainLayout",
              lS,  kFBAttachLeft,   "TabPanel", 1.0,
              lS,  kFBAttachBottom, "TabPanel", 1.0,
              -lS, kFBAttachRight,  "TabPanel", 1.0,
              -lS, kFBAttachBottom, "",         1.0);

    SetControl("TabPanel",   mTabPanel);
    SetControl("MainLayout", mLayoutGeneral);

    UICreateLayoutGeneral();
    UICreateLayoutCommunication();
    UICreateLayoutLive();
}

void CDevice_FaceMotion_Layout::UICreateLayoutGeneral()
{
    const int lS = 4, lH = 18;

    mLayoutGeneral.AddRegion("LabelSamplingRate", "LabelSamplingRate",
        lS,  kFBAttachLeft,  "",    1.0,
        lS,  kFBAttachTop,   "",    1.0,
        100, kFBAttachNone,  nullptr, 1.0,
        lH,  kFBAttachNone,  nullptr, 1.0);
    mLayoutGeneral.AddRegion("EditNumberSamplingRate", "EditNumberSamplingRate",
        lS,  kFBAttachRight, "LabelSamplingRate", 1.0,
        0,   kFBAttachTop,   "LabelSamplingRate", 1.0,
        100, kFBAttachNone,  nullptr,             1.0,
        0,   kFBAttachHeight,"LabelSamplingRate", 1.0);
    mLayoutGeneral.AddRegion("LabelSamplingType", "LabelSamplingType",
        0,   kFBAttachLeft,  "LabelSamplingRate", 1.0,
        lS,  kFBAttachBottom,"LabelSamplingRate", 1.0,
        0,   kFBAttachWidth, "LabelSamplingRate", 1.0,
        0,   kFBAttachHeight,"LabelSamplingRate", 1.0);
    mLayoutGeneral.AddRegion("ListSamplingType", "ListSamplingType",
        lS,  kFBAttachRight, "LabelSamplingType", 1.0,
        0,   kFBAttachTop,   "LabelSamplingType", 1.0,
        150, kFBAttachNone,  nullptr,             1.0,
        0,   kFBAttachHeight,"LabelSamplingType", 1.0);
    mLayoutGeneral.AddRegion("ButtonSetCandidate", "ButtonSetCandidate",
        0,   kFBAttachLeft,  "ListSamplingType", 1.0,
        lS,  kFBAttachBottom,"ListSamplingType", 1.0,
        100, kFBAttachNone,  "ListSamplingType", 1.0,
        0,   kFBAttachHeight,"ListSamplingType", 1.0);
    mLayoutGeneral.AddRegion("ButtonAbout", "ButtonAbout",
        0,   kFBAttachLeft,  "ListSamplingType", 1.0,
        lS,  kFBAttachBottom,"ButtonSetCandidate", 1.0,
        100, kFBAttachNone,  "",                 1.0,
        0,   kFBAttachHeight,"ListSamplingType", 1.0);

    mLayoutGeneral.SetControl("LabelSamplingRate",      mLabelSamplingRate);
    mLayoutGeneral.SetControl("EditNumberSamplingRate", mEditNumberSamplingRate);
    mLayoutGeneral.SetControl("LabelSamplingType",      mLabelSamplingType);
    mLayoutGeneral.SetControl("ListSamplingType",       mListSamplingType);
    mLayoutGeneral.SetControl("ButtonSetCandidate",     mButtonSetCandidate);
    mLayoutGeneral.SetControl("ButtonAbout",            mButtonAbout);
}

void CDevice_FaceMotion_Layout::UICreateLayoutCommunication()
{
    const int lS = 4, lH = 18;
    const int lblW = 130, valW = 200;

    auto rowLabel = [&](const char* id, const char* anchor) {
        if (anchor[0])
        {
            mLayoutCommunication.AddRegion(id, id,
                lS,   kFBAttachLeft,   "",     1.0,
                lS,   kFBAttachBottom, anchor, 1.0,
                lblW, kFBAttachNone,   nullptr, 1.0,
                lH,   kFBAttachNone,   nullptr, 1.0);
        }
        else
        {
            mLayoutCommunication.AddRegion(id, id,
                lS,   kFBAttachLeft, "",      1.0,
                8,    kFBAttachTop,  "",      1.0,
                lblW, kFBAttachNone, nullptr, 1.0,
                lH,   kFBAttachNone, nullptr, 1.0);
        }
    };
    auto rowField = [&](const char* id, const char* anchor, int width) {
        mLayoutCommunication.AddRegion(id, id,
            lS,    kFBAttachRight, anchor, 1.0,
            0,     kFBAttachTop,   anchor, 1.0,
            width, kFBAttachNone,  nullptr,1.0,
            0,     kFBAttachHeight,anchor, 1.0);
    };

    // Source app
    rowLabel("LabelSource",     "");
    rowField("ListSource",      "LabelSource",     valW);

    // iPhone connection
    rowLabel("LabelPhoneIp",    "LabelSource");
    rowField("EditPhoneIp",     "LabelPhoneIp",    valW);
    rowLabel("LabelPhonePort",  "LabelPhoneIp");
    rowField("EditPhonePort",   "LabelPhonePort",  80);

    // Blendshape target (label updates with current binding)
    rowLabel("LabelTarget",     "LabelPhonePort");
    mLayoutCommunication.AddRegion("ButtonAssignTarget", "ButtonAssignTarget",
        lS,  kFBAttachLeft,   "LabelTarget",     1.0,
        lS,  kFBAttachBottom, "LabelTarget",     1.0,
        140, kFBAttachNone,   nullptr,            1.0,
        lH,  kFBAttachNone,   nullptr,            1.0);
    mLayoutCommunication.AddRegion("ButtonClearTarget", "ButtonClearTarget",
        lS,  kFBAttachRight,  "ButtonAssignTarget", 1.0,
        0,   kFBAttachTop,    "ButtonAssignTarget", 1.0,
        60,  kFBAttachNone,   nullptr,               1.0,
        0,   kFBAttachHeight, "ButtonAssignTarget",  1.0);

    // Relay section
    rowLabel("LabelRelayHdr",   "ButtonAssignTarget");
    mLayoutCommunication.AddRegion("ButtonRelayEnable", "ButtonRelayEnable",
        lS,  kFBAttachLeft,   "LabelRelayHdr",   1.0,
        lS,  kFBAttachBottom, "LabelRelayHdr",   1.0,
        200, kFBAttachNone,   nullptr,            1.0,
        lH,  kFBAttachNone,   nullptr,            1.0);
    rowLabel("LabelRelayIp",    "ButtonRelayEnable");
    rowField("EditRelayIp",     "LabelRelayIp",    valW);
    rowLabel("LabelRelayPort",  "LabelRelayIp");
    rowField("EditRelayPort",   "LabelRelayPort",  80);

    mLayoutCommunication.SetControl("LabelSource",         mLabelSource);
    mLayoutCommunication.SetControl("ListSource",          mListSource);
    mLayoutCommunication.SetControl("LabelPhoneIp",        mLabelPhoneIp);
    mLayoutCommunication.SetControl("EditPhoneIp",         mEditPhoneIp);
    mLayoutCommunication.SetControl("LabelPhonePort",      mLabelPhonePort);
    mLayoutCommunication.SetControl("EditPhonePort",       mEditPhonePort);
    mLayoutCommunication.SetControl("LabelTarget",         mLabelTarget);
    mLayoutCommunication.SetControl("ButtonAssignTarget",  mButtonAssignTarget);
    mLayoutCommunication.SetControl("ButtonClearTarget",   mButtonClearTarget);
    mLayoutCommunication.SetControl("LabelRelayHdr",       mLabelRelayHdr);
    mLayoutCommunication.SetControl("ButtonRelayEnable",   mButtonRelayEnable);
    mLayoutCommunication.SetControl("LabelRelayIp",        mLabelRelayIp);
    mLayoutCommunication.SetControl("EditRelayIp",         mEditRelayIp);
    mLayoutCommunication.SetControl("LabelRelayPort",      mLabelRelayPort);
    mLayoutCommunication.SetControl("EditRelayPort",       mEditRelayPort);
}

void CDevice_FaceMotion_Layout::UICreateLayoutLive()
{
    const int lS = 4, lH = 18;
    const int rowW = 560;

    auto rowAt = [&](const char* id, const char* anchor) {
        if (anchor[0])
        {
            mLayoutLive.AddRegion(id, id,
                lS,    kFBAttachLeft,   "",     1.0,
                lS,    kFBAttachBottom, anchor, 1.0,
                rowW,  kFBAttachNone,   nullptr,1.0,
                lH,    kFBAttachNone,   nullptr,1.0);
        }
        else
        {
            mLayoutLive.AddRegion(id, id,
                lS,    kFBAttachLeft, "",      1.0,
                8,     kFBAttachTop,  "",      1.0,
                rowW,  kFBAttachNone, nullptr, 1.0,
                lH,    kFBAttachNone, nullptr, 1.0);
        }
    };

    rowAt("LabelLiveStatus",        "");
    rowAt("LabelLiveFrames",        "LabelLiveStatus");
    rowAt("LabelLiveRelay",         "LabelLiveFrames");
    rowAt("LabelLiveHeadRot",       "LabelLiveRelay");
    rowAt("LabelLiveHeadPos",       "LabelLiveHeadRot");
    rowAt("LabelLiveLeftEye",       "LabelLiveHeadPos");
    rowAt("LabelLiveRightEye",      "LabelLiveLeftEye");
    rowAt("LabelLiveBlendshapesHdr","LabelLiveRightEye");
    rowAt("LabelLiveBlendshape0",   "LabelLiveBlendshapesHdr");
    rowAt("LabelLiveBlendshape1",   "LabelLiveBlendshape0");
    rowAt("LabelLiveBlendshape2",   "LabelLiveBlendshape1");
    rowAt("LabelLiveBlendshape3",   "LabelLiveBlendshape2");
    rowAt("LabelLiveBlendshape4",   "LabelLiveBlendshape3");
    rowAt("LabelLiveBlendshape5",   "LabelLiveBlendshape4");
    rowAt("LabelLiveBlendshape6",   "LabelLiveBlendshape5");
    rowAt("LabelLiveBlendshape7",   "LabelLiveBlendshape6");

    mLayoutLive.SetControl("LabelLiveStatus",         mLabelLiveStatus);
    mLayoutLive.SetControl("LabelLiveFrames",         mLabelLiveFrames);
    mLayoutLive.SetControl("LabelLiveRelay",          mLabelLiveRelay);
    mLayoutLive.SetControl("LabelLiveHeadRot",        mLabelLiveHeadRot);
    mLayoutLive.SetControl("LabelLiveHeadPos",        mLabelLiveHeadPos);
    mLayoutLive.SetControl("LabelLiveLeftEye",        mLabelLiveLeftEye);
    mLayoutLive.SetControl("LabelLiveRightEye",       mLabelLiveRightEye);
    mLayoutLive.SetControl("LabelLiveBlendshapesHdr", mLabelLiveBlendshapesHdr);
    mLayoutLive.SetControl("LabelLiveBlendshape0",    mLabelLiveBlendshape[0]);
    mLayoutLive.SetControl("LabelLiveBlendshape1",    mLabelLiveBlendshape[1]);
    mLayoutLive.SetControl("LabelLiveBlendshape2",    mLabelLiveBlendshape[2]);
    mLayoutLive.SetControl("LabelLiveBlendshape3",    mLabelLiveBlendshape[3]);
    mLayoutLive.SetControl("LabelLiveBlendshape4",    mLabelLiveBlendshape[4]);
    mLayoutLive.SetControl("LabelLiveBlendshape5",    mLabelLiveBlendshape[5]);
    mLayoutLive.SetControl("LabelLiveBlendshape6",    mLabelLiveBlendshape[6]);
    mLayoutLive.SetControl("LabelLiveBlendshape7",    mLabelLiveBlendshape[7]);
}

// -- UI configuration --------------------------------------------------------

void CDevice_FaceMotion_Layout::UIConfigure()
{
    SetBorder("MainLayout", kFBStandardBorder, false, true, 1, 0, 90, 0);
    mTabPanel.Items.SetString("General~Communication~Live");
    mTabPanel.OnChange.Add(this, (FBCallback)&CDevice_FaceMotion_Layout::EventTabPanelChange);

    UIConfigureLayoutGeneral();
    UIConfigureLayoutCommunication();
    UIConfigureLayoutLive();
}

void CDevice_FaceMotion_Layout::UIConfigureLayoutGeneral()
{
    mLabelSamplingRate.Caption = "Sampling Rate :";
    mLabelSamplingType.Caption = "Sampling Type :";

    mListSamplingType.Items.Add("kFBHardwareTimestamp", kFBHardwareTimestamp);
    mListSamplingType.Items.Add("kFBHardwareFrequency", kFBHardwareFrequency);
    mListSamplingType.Items.Add("kFBAutoFrequency",     kFBAutoFrequency);
    mListSamplingType.Items.Add("kFBSoftwareTimestamp", kFBSoftwareTimestamp);

    mEditNumberSamplingRate.LargeStep = 0.0;
    mEditNumberSamplingRate.SmallStep = 0.0;
    mEditNumberSamplingRate.OnChange.Add(this, (FBCallback)&CDevice_FaceMotion_Layout::EventEditNumberSamplingRateChange);
    mListSamplingType.OnChange.Add     (this, (FBCallback)&CDevice_FaceMotion_Layout::EventListSamplingTypeChange);

    mButtonSetCandidate.Style.SetPropertyValue(kFB2States);
    mButtonSetCandidate.OnClick.Add(this, (FBCallback)&CDevice_FaceMotion_Layout::EventButtonSetCandidateClick);
    mButtonSetCandidate.Caption = "Set Candidate";

    mButtonAbout.OnClick.Add(this, (FBCallback)&CDevice_FaceMotion_Layout::EventButtonAboutClick);
    mButtonAbout.Caption = "About";
}

void CDevice_FaceMotion_Layout::UIConfigureLayoutCommunication()
{
    mLabelSource.Caption = "Source :";
    // Order here MUST match SourceIndexFor / DecodeSource above.
    // Default item (index 0) is iFacialMocap v1 -- the format Warudo and
    // most other downstream receivers can actually decode.
    mListSource.Items.Add("iFacialMocap (UDP)",                       0);
    mListSource.Items.Add("iFacialMocap (TCP)",                       1);
    mListSource.Items.Add("FaceMotion3D (UDP)",                       2);
    mListSource.Items.Add("FaceMotion3D (TCP)",                       3);
    mListSource.Items.Add("iFacialMocap v2 (UDP, signed values)",     4);
    mListSource.Items.Add("iFacialMocap v2 (TCP, signed values)",     5);
    mListSource.OnChange.Add(this, (FBCallback)&CDevice_FaceMotion_Layout::EventListSourceChange);

    mLabelPhoneIp.Caption = "iPhone IP Address :";
    mEditPhoneIp.OnChange.Add(this, (FBCallback)&CDevice_FaceMotion_Layout::EventEditPhoneIpChange);

    mLabelPhonePort.Caption = "iPhone Port :";
    mEditPhonePort.OnChange.Add(this, (FBCallback)&CDevice_FaceMotion_Layout::EventEditPhonePortChange);

    // mLabelTarget gets its caption rewritten by UIReset to include current name.
    mButtonAssignTarget.Caption = "Assign From Selection";
    mButtonAssignTarget.OnClick.Add(this, (FBCallback)&CDevice_FaceMotion_Layout::EventButtonAssignTargetClick);
    mButtonClearTarget.Caption  = "Clear";
    mButtonClearTarget.OnClick.Add(this, (FBCallback)&CDevice_FaceMotion_Layout::EventButtonClearTargetClick);

    mLabelRelayHdr.Caption = "--- Relay (forward each packet to another app, e.g. Warudo) ---";
    mButtonRelayEnable.Style.SetPropertyValue(kFB2States);
    mButtonRelayEnable.Caption = "Enable Relay";
    mButtonRelayEnable.OnClick.Add(this, (FBCallback)&CDevice_FaceMotion_Layout::EventButtonRelayEnableClick);
    mLabelRelayIp.Caption   = "Relay To IP :";
    mEditRelayIp.OnChange.Add(this, (FBCallback)&CDevice_FaceMotion_Layout::EventEditRelayIpChange);
    mLabelRelayPort.Caption = "Relay To Port :";
    mEditRelayPort.OnChange.Add(this, (FBCallback)&CDevice_FaceMotion_Layout::EventEditRelayPortChange);
}

void CDevice_FaceMotion_Layout::UIConfigureLayoutLive()
{
    mLabelLiveStatus.Caption         = "Status: Offline";
    mLabelLiveFrames.Caption         = "Frames received: 0";
    mLabelLiveRelay.Caption          = "Relay: disabled";
    mLabelLiveHeadRot.Caption        = "Head rotation : -";
    mLabelLiveHeadPos.Caption        = "Head position : -";
    mLabelLiveLeftEye.Caption        = "Left eye      : -";
    mLabelLiveRightEye.Caption       = "Right eye     : -";
    mLabelLiveBlendshapesHdr.Caption = "Top active blendshapes (sorted by magnitude):";
    for (int i = 0; i < 8; ++i) mLabelLiveBlendshape[i].Caption = "  -";
}

void CDevice_FaceMotion_Layout::UIRefresh()
{
    if (!mDevice) return;

    char buf[256];
    char ageBuf[32];

    const uint64_t frames = mDevice->GetFrameCount();
    const int64_t  lastMs = mDevice->GetLastFrameTickMs();
    const int64_t  nowMs  = static_cast<int64_t>(GetTickCount64());
    const int64_t  ageMs  = (lastMs > 0) ? (nowMs - lastMs) : -1;

    const char* state;
    if      (!mDevice->Online)             state = "Offline";
    else if (!mDevice->Live)               state = "Online (not Live)";
    else if (frames == 0)                  state = "Live, no packets yet -- check iPhone IP";
    else if (ageMs >= 0 && ageMs > 1500)   state = "Live, stream stalled";
    else                                   state = "Live, receiving";

    const char* ip = mDevice->GetPhoneIp();
    std::snprintf(buf, sizeof(buf),
                  "Status: %s   |   iPhone: %s:%d",
                  state,
                  (ip && ip[0]) ? ip : "<not set>",
                  mDevice->GetPhonePort());
    mLabelLiveStatus.Caption = buf;

    if (lastMs > 0)
    {
        FormatAge(ageMs, ageBuf, sizeof(ageBuf));
        std::snprintf(buf, sizeof(buf),
                      "Frames received: %llu   |   Last packet: %s ago",
                      static_cast<unsigned long long>(frames), ageBuf);
    }
    else
    {
        std::snprintf(buf, sizeof(buf), "Frames received: 0");
    }
    mLabelLiveFrames.Caption = buf;

    if (mDevice->GetRelayEnabled())
    {
        const char* rip = mDevice->GetRelayIp();
        std::snprintf(buf, sizeof(buf),
                      "Relay: ON  ->  %s:%d   |   Forwarded: %llu",
                      (rip && rip[0]) ? rip : "<not set>",
                      mDevice->GetRelayPort(),
                      static_cast<unsigned long long>(mDevice->GetRelayCount()));
    }
    else
    {
        std::snprintf(buf, sizeof(buf), "Relay: disabled");
    }
    mLabelLiveRelay.Caption = buf;

    double v3[3];
    mDevice->GetHeadRotation(v3);
    std::snprintf(buf, sizeof(buf), "Head rotation : X=%7.2f  Y=%7.2f  Z=%7.2f  deg",
                  v3[0], v3[1], v3[2]);
    mLabelLiveHeadRot.Caption = buf;

    mDevice->GetHeadPosition(v3);
    std::snprintf(buf, sizeof(buf), "Head position : X=%7.2f  Y=%7.2f  Z=%7.2f  cm",
                  v3[0], v3[1], v3[2]);
    mLabelLiveHeadPos.Caption = buf;

    mDevice->GetLeftEye(v3);
    std::snprintf(buf, sizeof(buf), "Left eye      : X=%7.2f  Y=%7.2f  Z=%7.2f  deg",
                  v3[0], v3[1], v3[2]);
    mLabelLiveLeftEye.Caption = buf;

    mDevice->GetRightEye(v3);
    std::snprintf(buf, sizeof(buf), "Right eye     : X=%7.2f  Y=%7.2f  Z=%7.2f  deg",
                  v3[0], v3[1], v3[2]);
    mLabelLiveRightEye.Caption = buf;

    struct Entry { int idx; double val; };
    Entry top[mobufacemotion::kBlendshapeCount];
    for (uint32_t i = 0; i < mobufacemotion::kBlendshapeCount; ++i)
    {
        top[i].idx = static_cast<int>(i);
        top[i].val = mDevice->GetBlendshape(static_cast<int>(i));
    }
    std::partial_sort(top, top + 8, top + mobufacemotion::kBlendshapeCount,
        [](const Entry& a, const Entry& b) {
            return std::abs(a.val) > std::abs(b.val);
        });

    for (int i = 0; i < 8; ++i)
    {
        if (std::abs(top[i].val) < 0.01)
        {
            mLabelLiveBlendshape[i].Caption = "  -";
        }
        else
        {
            std::snprintf(buf, sizeof(buf), "  %-22s : %7.2f",
                          mobufacemotion::kArkitBlendshapes[top[i].idx],
                          top[i].val);
            mLabelLiveBlendshape[i].Caption = buf;
        }
    }
}

void CDevice_FaceMotion_Layout::UIReset()
{
    char buf[128];

    mEditNumberSamplingRate.Value = 1.0 / static_cast<FBTime>(mDevice->SamplingPeriod).GetSecondDouble();
    mListSamplingType.ItemIndex   = mListSamplingType.Items.Find(mDevice->SamplingMode.AsInt());
    mButtonSetCandidate.State     = mDevice->GetSetCandidate() ? 1 : 0;

    mListSource.ItemIndex = SourceIndexFor(mDevice->GetProtocol(), mDevice->GetTransport());

    mEditPhoneIp.Text = mDevice->GetPhoneIp();
    std::snprintf(buf, sizeof(buf), "%d", mDevice->GetPhonePort());
    mEditPhonePort.Text = buf;

    FBModel* target = (mDevice->BlendshapeTarget.GetCount() > 0)
        ? static_cast<FBModel*>(mDevice->BlendshapeTarget.GetAt(0))
        : nullptr;
    std::snprintf(buf, sizeof(buf), "Blendshape Target: %s",
                  target ? target->LongName.AsString() : "(none)");
    mLabelTarget.Caption = buf;

    mButtonRelayEnable.State = mDevice->GetRelayEnabled() ? 1 : 0;
    mButtonRelayEnable.Caption = mDevice->GetRelayEnabled() ? "Relay ON" : "Enable Relay";
    mEditRelayIp.Text = mDevice->GetRelayIp();
    std::snprintf(buf, sizeof(buf), "%d", mDevice->GetRelayPort());
    mEditRelayPort.Text = buf;
}

// -- Event handlers ----------------------------------------------------------

void CDevice_FaceMotion_Layout::EventTabPanelChange(HISender, HKEvent)
{
    switch (mTabPanel.ItemIndex)
    {
        case 0: SetControl("MainLayout", mLayoutGeneral);       break;
        case 1: SetControl("MainLayout", mLayoutCommunication); break;
        case 2: SetControl("MainLayout", mLayoutLive);          break;
    }
}

void CDevice_FaceMotion_Layout::EventDeviceStatusChange(HISender, HKEvent) { UIReset(); }

void CDevice_FaceMotion_Layout::EventUIIdle(HISender, HKEvent)
{
    if (mDevice->Online && mDevice->Live)
    {
        if (mDevice->GetSetCandidate()) mDevice->SetCandidates();
    }
    UIRefresh();
}

void CDevice_FaceMotion_Layout::EventEditNumberSamplingRateChange(HISender, HKEvent)
{
    const bool online = mDevice->Online;
    const double r = mEditNumberSamplingRate.Value;
    if (r <= 0.0) return;

    if (online) mDevice->DeviceSendCommand(FBDevice::kOpStop);

    FBTime t;
    t.SetSecondDouble(1.0 / r);
    mDevice->SamplingPeriod = t;

    if (online) mDevice->DeviceSendCommand(FBDevice::kOpStart);
    UIReset();
}

void CDevice_FaceMotion_Layout::EventListSamplingTypeChange(HISender, HKEvent)
{
    mDevice->SamplingMode.SetPropertyValue(
        static_cast<FBDeviceSamplingMode>(
            mListSamplingType.Items.GetReferenceAt(mListSamplingType.ItemIndex)));
    UIReset();
}

void CDevice_FaceMotion_Layout::EventButtonSetCandidateClick(HISender, HKEvent)
{
    // FBButton's 2-state toggle behaviour doesn't survive in MoBu 2027 (State
    // stays 0 on click). Flip from the device's stored state instead so we
    // work regardless of whether the visual button stays depressed.
    mDevice->SetSetCandidate(!mDevice->GetSetCandidate());
    UIReset();
}

void CDevice_FaceMotion_Layout::EventButtonAboutClick(HISender, HKEvent)
{
    FBMessageBox("MobuFaceMotion",
                 "MotionBuilder device plugin for iFacialMocap and FaceMotion3D.\n\n"
                 "Adapted from the OpenMoBu device_faceCap plugin (BSD 3-clause).\n"
                 "See LICENSE for full attribution.",
                 "Ok");
}

void CDevice_FaceMotion_Layout::EventListSourceChange(HISender, HKEvent)
{
    EProtocol  p;
    ETransport t;
    DecodeSource(mListSource.ItemIndex, p, t);
    mDevice->SetProtocol(static_cast<int>(p));
    mDevice->SetTransport(static_cast<int>(t));
    UIReset();
}

void CDevice_FaceMotion_Layout::EventEditPhoneIpChange(HISender, HKEvent)
{
    mDevice->SetPhoneIp(mEditPhoneIp.Text.AsString());
}

void CDevice_FaceMotion_Layout::EventEditPhonePortChange(HISender, HKEvent)
{
    int port = std::atoi(mEditPhonePort.Text.AsString());
    if (port > 0) mDevice->SetIPhonePort(port);
}

void CDevice_FaceMotion_Layout::EventButtonAssignTargetClick(HISender, HKEvent)
{
    FBModelList list;
    FBGetSelectedModels(list, nullptr, true);
    if (list.GetCount() == 0)
    {
        FBMessageBox("MobuFaceMotion",
                     "Select a face model in the scene first, then click Assign.",
                     "Ok");
        return;
    }
    mDevice->BlendshapeTarget.Clear();
    mDevice->BlendshapeTarget.Add(list[0]);
    UIReset();
}

void CDevice_FaceMotion_Layout::EventButtonClearTargetClick(HISender, HKEvent)
{
    mDevice->BlendshapeTarget.Clear();
    UIReset();
}

void CDevice_FaceMotion_Layout::EventButtonRelayEnableClick(HISender, HKEvent)
{
    mDevice->SetRelayEnabled(!mDevice->GetRelayEnabled());
    UIReset();
}

void CDevice_FaceMotion_Layout::EventEditRelayIpChange(HISender, HKEvent)
{
    mDevice->SetRelayIp(mEditRelayIp.Text.AsString());
}

void CDevice_FaceMotion_Layout::EventEditRelayPortChange(HISender, HKEvent)
{
    int port = std::atoi(mEditRelayPort.Text.AsString());
    if (port > 0) mDevice->SetRelayPort(port);
}
