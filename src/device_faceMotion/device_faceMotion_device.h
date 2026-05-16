#pragma once

/** \file device_faceMotion_device.h
 *
 *  CDevice_FaceMotion - the FBDevice subclass that MotionBuilder loads.
 *  Owns one hardware instance, 56 animation node outputs (head T/R, two
 *  eye R, 52 blendshape weights), and an optional BlendshapeDriver for
 *  driving a face mesh's deformer channels directly.
 *
 *  Scaffold adapted from OpenMoBu device_faceCap (BSD 3-clause); the
 *  hardware layer, protocol parsing and blendshape-target driver are
 *  original to MobuFaceMotion.
 */

#include <fbsdk/fbsdk.h>

#include "blendshape_driver.h"
#include "device_faceMotion_hardware.h"
#include "protocol.h"

#define CDEVICEFACEMOTION__CLASSNAME    CDevice_FaceMotion
#define CDEVICEFACEMOTION__CLASSSTR     "CDevice_FaceMotion"

class CDevice_FaceMotion : public FBDevice
{
    FBDeviceDeclare(CDevice_FaceMotion, FBDevice);

public:
    bool FBCreate() override;
    void FBDestroy() override;

    bool AnimationNodeNotify   (FBAnimationNode*  pNode, FBEvaluateInfo* pInfo)            override;
    void DeviceIONotify        (kDeviceIOs        pAction, FBDeviceNotifyInfo& pInfo)       override;
    bool DeviceEvaluationNotify(kTransportMode    pMode,  FBEvaluateInfo* pInfo)            override;
    bool DeviceOperation       (kDeviceOperations pOp)                                      override;

    bool Init();
    bool Start();
    bool Reset();
    bool Stop();
    bool Done();

    void DeviceRecordFrame(FBDeviceNotifyInfo& pInfo);

    // Configuration proxies (the layout edits these; they push down to the
    // hardware which is the source of truth for network/protocol state).
    void SetProtocol(int p);
    int  GetProtocol() const   { return static_cast<int>(mHardware.GetProtocol()); }

    void SetTransport(int t);
    int  GetTransport() const  { return static_cast<int>(mHardware.GetTransport()); }

    void  SetPhoneIp(const char* ip) { mHardware.SetPhoneIp(ip); }
    const char* GetPhoneIp() const   { return mHardware.GetPhoneIp(); }

    void SetPhonePort(int p)  { mHardware.SetPhonePort(p); }
    int  GetPhonePort() const { return mHardware.GetPhonePort(); }

    void SetListenPort(int p) { mHardware.SetListenPort(p); }
    int  GetListenPort() const{ return mHardware.GetListenPort(); }

    bool GetSetCandidate() const         { return mSetCandidate; }
    void SetSetCandidate(bool b)         { mSetCandidate = b; }

    void SetCandidates();

    // Diagnostics surface (used by the Live tab).
    uint64_t GetFrameCount() const          { return mHardware.GetFrameCount(); }
    int64_t  GetLastFrameTickMs() const     { return mHardware.GetLastFrameTickMs(); }
    void     GetHeadRotation(double* o) const { mHardware.GetRotation(o); }
    void     GetHeadPosition(double* o) const { mHardware.GetPosition(o); }
    void     GetLeftEye(double* o) const      { mHardware.GetLeftEyeRotation(o); }
    void     GetRightEye(double* o) const     { mHardware.GetRightEyeRotation(o); }
    double   GetBlendshape(int i) const       { return mHardware.GetBlendshapeValue(i); }

    // Published, scriptable properties.
    FBPropertyDouble       SpaceScale;        // cm scale multiplier (default 100 -> *1.0)
    FBPropertyDouble       ShapeValueMult;    // blendshape weight multiplier (default 1.0)
    FBPropertyListObject   BlendshapeTarget;  // optional FBModel to drive directly

public:
    // Model templates (visible in the device IO panel for binding)
    FBModelTemplate* mTemplateRoot     = nullptr;
    FBModelTemplate* mTemplateHead     = nullptr;
    FBModelTemplate* mTemplateLeftEye  = nullptr;
    FBModelTemplate* mTemplateRightEye = nullptr;

    // Animation node outputs (the device's data sink)
    FBAnimationNode* mNodeHead_InT       = nullptr;
    FBAnimationNode* mNodeHead_InR       = nullptr;
    FBAnimationNode* mNodeLeftEye_InR    = nullptr;
    FBAnimationNode* mNodeRightEye_InR   = nullptr;
    FBAnimationNode* mNodeBlendshape[mobufacemotion::kBlendshapeCount] = {};

private:
    bool                                mSetCandidate = false;
    CDevice_FaceMotion_Hardware         mHardware;
    mobufacemotion::BlendshapeDriver    mDriver;
    FBPlayerControl                     mPlayerControl;
};
