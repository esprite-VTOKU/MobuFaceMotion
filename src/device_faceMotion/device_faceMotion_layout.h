#pragma once

/** \file device_faceMotion_layout.h
 *
 *  The two-tab device layout. General tab mirrors OpenMoBu's facecap
 *  layout (sampling rate / type / set candidate / about). Communication
 *  tab is expanded with the iFacialMocap/FaceMotion3D-specific knobs
 *  (protocol, transport, phone IP, phone port, listen port) plus a
 *  drag-or-select target slot for the optional blendshape-driven mesh.
 */

#include "device_faceMotion_device.h"

class CDevice_FaceMotion_Layout : public FBDeviceLayout
{
    FBDeviceLayoutDeclare(CDevice_FaceMotion_Layout, FBDeviceLayout);

public:
    bool FBCreate() override;
    void FBDestroy() override;

    void UICreate();
    void   UICreateLayoutGeneral();
    void   UICreateLayoutCommunication();
    void   UICreateLayoutLive();
    void UIConfigure();
    void   UIConfigureLayoutGeneral();
    void   UIConfigureLayoutCommunication();
    void   UIConfigureLayoutLive();
    void UIRefresh();
    void UIReset();

    // Main events
    void EventDeviceStatusChange(HISender pSender, HKEvent pEvent);
    void EventUIIdle            (HISender pSender, HKEvent pEvent);
    void EventTabPanelChange    (HISender pSender, HKEvent pEvent);

    // General tab
    void EventEditNumberSamplingRateChange (HISender pSender, HKEvent pEvent);
    void EventListSamplingTypeChange       (HISender pSender, HKEvent pEvent);
    void EventButtonSetCandidateClick      (HISender pSender, HKEvent pEvent);
    void EventButtonAboutClick             (HISender pSender, HKEvent pEvent);

    // Communication tab
    void EventListSourceChange        (HISender pSender, HKEvent pEvent);
    void EventEditPhoneIpChange       (HISender pSender, HKEvent pEvent);
    void EventEditPhonePortChange     (HISender pSender, HKEvent pEvent);
    void EventButtonAssignTargetClick (HISender pSender, HKEvent pEvent);
    void EventButtonClearTargetClick  (HISender pSender, HKEvent pEvent);
    void EventButtonRelayEnableClick  (HISender pSender, HKEvent pEvent);
    void EventEditRelayIpChange       (HISender pSender, HKEvent pEvent);
    void EventEditRelayPortChange     (HISender pSender, HKEvent pEvent);

private:
    FBTabPanel  mTabPanel;

    FBLayout    mLayoutGeneral;
        FBLabel       mLabelSamplingRate;
        FBEditNumber  mEditNumberSamplingRate;
        FBLabel       mLabelSamplingType;
        FBList        mListSamplingType;
        FBButton      mButtonSetCandidate;
        FBButton      mButtonAbout;

    FBLayout    mLayoutCommunication;
        FBLabel       mLabelSource;
        FBList        mListSource;
        FBLabel       mLabelPhoneIp;
        FBEdit        mEditPhoneIp;
        FBLabel       mLabelPhonePort;
        FBEdit        mEditPhonePort;
        FBLabel       mLabelTarget;            // shows "Blendshape Target: <name>"
        FBButton      mButtonAssignTarget;
        FBButton      mButtonClearTarget;
        FBLabel       mLabelRelayHdr;          // divider/header
        FBButton      mButtonRelayEnable;      // 2-state toggle
        FBLabel       mLabelRelayIp;
        FBEdit        mEditRelayIp;
        FBLabel       mLabelRelayPort;
        FBEdit        mEditRelayPort;

    FBLayout    mLayoutLive;
        FBLabel       mLabelLiveStatus;
        FBLabel       mLabelLiveFrames;
        FBLabel       mLabelLiveRelay;
        FBLabel       mLabelLiveHeadRot;
        FBLabel       mLabelLiveHeadPos;
        FBLabel       mLabelLiveLeftEye;
        FBLabel       mLabelLiveRightEye;
        FBLabel       mLabelLiveBlendshapesHdr;
        FBLabel       mLabelLiveBlendshape[8];

    FBSystem            mSystem;
    CDevice_FaceMotion* mDevice = nullptr;
};
