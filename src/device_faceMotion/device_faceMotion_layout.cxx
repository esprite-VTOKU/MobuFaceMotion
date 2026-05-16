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
    const int lblW = 90, valW = 160;

    auto rowLabel = [&](const char* id, const char* anchor) {
        // First row attaches to the top edge of the parent layout (anchor="");
        // subsequent rows stack below the previous label.
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

    rowLabel("LabelProtocol",  "");           rowField("ListProtocol",    "LabelProtocol",  valW);
    rowLabel("LabelTransport", "LabelProtocol");  rowField("ListTransport",   "LabelTransport", valW);
    rowLabel("LabelPhoneIp",   "LabelTransport"); rowField("EditPhoneIp",     "LabelPhoneIp",   valW);
    rowLabel("LabelPhonePort", "LabelPhoneIp");   rowField("EditPhonePort",   "LabelPhonePort", 80);
    rowLabel("LabelListenPort","LabelPhonePort"); rowField("EditListenPort",  "LabelListenPort",80);
    rowLabel("LabelTarget",    "LabelListenPort");rowField("EditTargetName",  "LabelTarget",    valW);

    // Buttons under the target row
    mLayoutCommunication.AddRegion("ButtonAssignTarget", "ButtonAssignTarget",
        lS,  kFBAttachRight,  "LabelTarget",     1.0,
        lS,  kFBAttachBottom, "EditTargetName",  1.0,
        140, kFBAttachNone,   nullptr,            1.0,
        lH,  kFBAttachNone,   nullptr,            1.0);
    mLayoutCommunication.AddRegion("ButtonClearTarget", "ButtonClearTarget",
        lS,  kFBAttachRight,  "ButtonAssignTarget", 1.0,
        0,   kFBAttachTop,    "ButtonAssignTarget", 1.0,
        60,  kFBAttachNone,   nullptr,               1.0,
        0,   kFBAttachHeight, "ButtonAssignTarget",  1.0);

    mLayoutCommunication.SetControl("LabelProtocol",       mLabelProtocol);
    mLayoutCommunication.SetControl("ListProtocol",        mListProtocol);
    mLayoutCommunication.SetControl("LabelTransport",      mLabelTransport);
    mLayoutCommunication.SetControl("ListTransport",       mListTransport);
    mLayoutCommunication.SetControl("LabelPhoneIp",        mLabelPhoneIp);
    mLayoutCommunication.SetControl("EditPhoneIp",         mEditPhoneIp);
    mLayoutCommunication.SetControl("LabelPhonePort",      mLabelPhonePort);
    mLayoutCommunication.SetControl("EditPhonePort",       mEditPhonePort);
    mLayoutCommunication.SetControl("LabelListenPort",     mLabelListenPort);
    mLayoutCommunication.SetControl("EditListenPort",      mEditListenPort);
    mLayoutCommunication.SetControl("LabelTarget",         mLabelTarget);
    mLayoutCommunication.SetControl("EditTargetName",      mEditTargetName);
    mLayoutCommunication.SetControl("ButtonAssignTarget",  mButtonAssignTarget);
    mLayoutCommunication.SetControl("ButtonClearTarget",   mButtonClearTarget);
}

void CDevice_FaceMotion_Layout::UICreateLayoutLive()
{
    const int lS = 4, lH = 18;
    const int rowW = 540;

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
    rowAt("LabelLiveHeadRot",       "LabelLiveFrames");
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
    mLabelProtocol.Caption   = "Protocol :";
    mListProtocol.Items.Add("iFacialMocap v1 (legacy)",   static_cast<int>(EProtocol::iFacialMocapV1));
    mListProtocol.Items.Add("iFacialMocap v2",            static_cast<int>(EProtocol::iFacialMocapV2));
    mListProtocol.Items.Add("FaceMotion3D",               static_cast<int>(EProtocol::FaceMotion3D));
    mListProtocol.OnChange.Add(this, (FBCallback)&CDevice_FaceMotion_Layout::EventListProtocolChange);

    mLabelTransport.Caption  = "Transport :";
    mListTransport.Items.Add("UDP", static_cast<int>(ETransport::UDP));
    mListTransport.Items.Add("TCP", static_cast<int>(ETransport::TCP));
    mListTransport.OnChange.Add(this, (FBCallback)&CDevice_FaceMotion_Layout::EventListTransportChange);

    mLabelPhoneIp.Caption    = "Phone IP :";
    mEditPhoneIp.OnChange.Add(this, (FBCallback)&CDevice_FaceMotion_Layout::EventEditPhoneIpChange);

    mLabelPhonePort.Caption  = "Phone Port :";
    mEditPhonePort.OnChange.Add(this, (FBCallback)&CDevice_FaceMotion_Layout::EventEditPhonePortChange);

    mLabelListenPort.Caption = "Listen Port :";
    mEditListenPort.OnChange.Add(this, (FBCallback)&CDevice_FaceMotion_Layout::EventEditListenPortChange);

    mLabelTarget.Caption     = "Blendshape Target :";
    mEditTargetName.Enabled  = false;

    mButtonAssignTarget.Caption = "Assign From Selection";
    mButtonAssignTarget.OnClick.Add(this, (FBCallback)&CDevice_FaceMotion_Layout::EventButtonAssignTargetClick);
    mButtonClearTarget.Caption  = "Clear";
    mButtonClearTarget.OnClick.Add(this, (FBCallback)&CDevice_FaceMotion_Layout::EventButtonClearTargetClick);
}

void CDevice_FaceMotion_Layout::UIConfigureLayoutLive()
{
    mLabelLiveStatus.Caption         = "Status: Offline";
    mLabelLiveFrames.Caption         = "Frames received: 0";
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

    // Status line: combine device Online/Live + whether packets are flowing.
    const uint64_t frames  = mDevice->GetFrameCount();
    const int64_t  lastMs  = mDevice->GetLastFrameTickMs();
    const int64_t  nowMs   = static_cast<int64_t>(GetTickCount64());
    const int64_t  ageMs   = (lastMs > 0) ? (nowMs - lastMs) : -1;

    const char* state;
    if      (!mDevice->Online)             state = "Offline";
    else if (!mDevice->Live)               state = "Online (not Live)";
    else if (frames == 0)                  state = "Live, no packets yet -- check Phone IP + handshake";
    else if (ageMs >= 0 && ageMs > 1500)   state = "Live, stream stalled";
    else                                   state = "Live, receiving";

    const char* ip = mDevice->GetPhoneIp();
    std::snprintf(buf, sizeof(buf),
                  "Status: %s   |   Phone: %s:%d   |   Listen: %d",
                  state,
                  (ip && ip[0]) ? ip : "<not set>",
                  mDevice->GetPhonePort(),
                  mDevice->GetListenPort());
    mLabelLiveStatus.Caption = buf;

    if (lastMs > 0)
    {
        std::snprintf(buf, sizeof(buf),
                      "Frames received: %llu   |   Last packet: %lld ms ago",
                      static_cast<unsigned long long>(frames),
                      static_cast<long long>(ageMs));
    }
    else
    {
        std::snprintf(buf, sizeof(buf), "Frames received: 0");
    }
    mLabelLiveFrames.Caption = buf;

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

    // Top 8 blendshapes by absolute value.
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
    char buf[64];

    mEditNumberSamplingRate.Value = 1.0 / static_cast<FBTime>(mDevice->SamplingPeriod).GetSecondDouble();
    mListSamplingType.ItemIndex   = mListSamplingType.Items.Find(mDevice->SamplingMode.AsInt());
    mButtonSetCandidate.State     = mDevice->GetSetCandidate() ? 1 : 0;

    mListProtocol.ItemIndex  = mListProtocol.Items.Find(mDevice->GetProtocol());
    mListTransport.ItemIndex = mListTransport.Items.Find(mDevice->GetTransport());

    mEditPhoneIp.Text  = mDevice->GetPhoneIp();
    std::snprintf(buf, sizeof(buf), "%d", mDevice->GetPhonePort());
    mEditPhonePort.Text  = buf;
    std::snprintf(buf, sizeof(buf), "%d", mDevice->GetListenPort());
    mEditListenPort.Text = buf;

    FBModel* target = (mDevice->BlendshapeTarget.GetCount() > 0)
        ? static_cast<FBModel*>(mDevice->BlendshapeTarget.GetAt(0))
        : nullptr;
    mEditTargetName.Text = target ? target->LongName : "(none)";
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
    mDevice->SetSetCandidate(mButtonSetCandidate.State != 0);
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

void CDevice_FaceMotion_Layout::EventListProtocolChange(HISender, HKEvent)
{
    const int p = static_cast<int>(mListProtocol.Items.GetReferenceAt(mListProtocol.ItemIndex));
    mDevice->SetProtocol(p);
    UIReset();
}

void CDevice_FaceMotion_Layout::EventListTransportChange(HISender, HKEvent)
{
    const int t = static_cast<int>(mListTransport.Items.GetReferenceAt(mListTransport.ItemIndex));
    mDevice->SetTransport(t);
    UIReset();
}

void CDevice_FaceMotion_Layout::EventEditPhoneIpChange(HISender, HKEvent)
{
    mDevice->SetPhoneIp(mEditPhoneIp.Text.AsString());
}

void CDevice_FaceMotion_Layout::EventEditPhonePortChange(HISender, HKEvent)
{
    int port = std::atoi(mEditPhonePort.Text.AsString());
    if (port > 0) mDevice->SetPhonePort(port);
}

void CDevice_FaceMotion_Layout::EventEditListenPortChange(HISender, HKEvent)
{
    int port = std::atoi(mEditListenPort.Text.AsString());
    if (port > 0) mDevice->SetListenPort(port);
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
