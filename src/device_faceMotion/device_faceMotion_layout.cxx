/** \file device_faceMotion_layout.cxx */

#include "device_faceMotion_layout.h"

#include <cstdio>
#include <cstdlib>

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

// -- UI configuration --------------------------------------------------------

void CDevice_FaceMotion_Layout::UIConfigure()
{
    SetBorder("MainLayout", kFBStandardBorder, false, true, 1, 0, 90, 0);
    mTabPanel.Items.SetString("General~Communication");
    mTabPanel.OnChange.Add(this, (FBCallback)&CDevice_FaceMotion_Layout::EventTabPanelChange);

    UIConfigureLayoutGeneral();
    UIConfigureLayoutCommunication();
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

void CDevice_FaceMotion_Layout::UIRefresh() {}

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
    }
}

void CDevice_FaceMotion_Layout::EventDeviceStatusChange(HISender, HKEvent) { UIReset(); }

void CDevice_FaceMotion_Layout::EventUIIdle(HISender, HKEvent)
{
    if (mDevice->Online && mDevice->Live)
    {
        if (mDevice->GetSetCandidate()) mDevice->SetCandidates();
        UIRefresh();
    }
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
