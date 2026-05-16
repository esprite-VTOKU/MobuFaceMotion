/** \file device_faceMotion_device.cxx */

#include "device_faceMotion_device.h"

#define CDEVICEFACEMOTION__CLASS    CDEVICEFACEMOTION__CLASSNAME
#define CDEVICEFACEMOTION__NAME     CDEVICEFACEMOTION__CLASSSTR
#define CDEVICEFACEMOTION__LABEL    "FaceMotion (iFacialMocap / FaceMotion3D)"
#define CDEVICEFACEMOTION__DESC     "Streams ARKit face capture from iFacialMocap or FaceMotion3D iPhone apps"
#define CDEVICEFACEMOTION__PREFIX   "FaceMotion"

FBDeviceImplementation(CDEVICEFACEMOTION__CLASS);
FBRegisterDevice(CDEVICEFACEMOTION__NAME,
                 CDEVICEFACEMOTION__CLASS,
                 CDEVICEFACEMOTION__LABEL,
                 CDEVICEFACEMOTION__DESC,
                 "character_actor.png");

using namespace mobufacemotion;

bool CDevice_FaceMotion::FBCreate()
{
    mHardware.SetParent(this);

    FBPropertyPublish(this, SpaceScale,       "Space Scale",       nullptr, nullptr);
    FBPropertyPublish(this, ShapeValueMult,   "Shape Value Mult",  nullptr, nullptr);
    FBPropertyPublish(this, BlendshapeTarget, "Blendshape Target", nullptr, nullptr);

    SpaceScale     = 100.0;
    ShapeValueMult = 1.0;
    BlendshapeTarget.SetSingleConnect(true);

    mNodeHead_InT     = AnimationNodeOutCreate(0, "Translation",       ANIMATIONNODE_TYPE_LOCAL_TRANSLATION);
    mNodeHead_InR     = AnimationNodeOutCreate(1, "Rotation",          ANIMATIONNODE_TYPE_LOCAL_ROTATION);
    mNodeLeftEye_InR  = AnimationNodeOutCreate(2, "LeftEye Rotation",  ANIMATIONNODE_TYPE_LOCAL_ROTATION);
    mNodeRightEye_InR = AnimationNodeOutCreate(3, "RightEye Rotation", ANIMATIONNODE_TYPE_LOCAL_ROTATION);

    for (uint32_t i = 0; i < kBlendshapeCount; ++i)
    {
        mNodeBlendshape[i] = AnimationNodeOutCreate(
            static_cast<int>(i + 4), kArkitBlendshapes[i], ANIMATIONNODE_TYPE_NUMBER);
    }

    mNodeHead_InT->SetCandidate(FBVector3d(0.0, 5.0, 0.0));

    mTemplateRoot     = new FBModelTemplate(CDEVICEFACEMOTION__PREFIX, "Reference", kFBModelTemplateRoot);
    mTemplateHead     = new FBModelTemplate(CDEVICEFACEMOTION__PREFIX, "Head",      kFBModelTemplateMarker);
    mTemplateLeftEye  = new FBModelTemplate(CDEVICEFACEMOTION__PREFIX, "LeftEye",   kFBModelTemplateMarker);
    mTemplateRightEye = new FBModelTemplate(CDEVICEFACEMOTION__PREFIX, "RightEye",  kFBModelTemplateMarker);

    ModelTemplate.Children.Add(mTemplateRoot);
    mTemplateRoot->Children.Add(mTemplateHead);
    mTemplateHead->Children.Add(mTemplateLeftEye);
    mTemplateHead->Children.Add(mTemplateRightEye);

    mTemplateHead->Bindings.Add(mNodeHead_InR);
    mTemplateHead->Bindings.Add(mNodeHead_InT);
    mTemplateLeftEye->Bindings.Add(mNodeLeftEye_InR);
    mTemplateRightEye->Bindings.Add(mNodeRightEye_InR);

    mTemplateHead->DefaultTranslation     = FBVector3d( 0.0, 5.0, 0.0);
    mTemplateLeftEye->DefaultTranslation  = FBVector3d(-5.0, 5.0, 0.0);
    mTemplateRightEye->DefaultTranslation = FBVector3d( 5.0, 5.0, 0.0);

    FBTime period;
    period.SetSecondDouble(1.0 / 60.0);
    SamplingPeriod = period;
    CommType = kFBCommTypeNetworkUDP;

    // Default to iFacialMocap v2 over UDP. The hardware constructor already
    // sets these, but make defaults explicit here for clarity.
    mHardware.SetProtocol(EProtocol::iFacialMocapV2);
    mHardware.SetTransport(ETransport::UDP);
    mHardware.SetPhonePort(IPhonePortFor(EProtocol::iFacialMocapV2));
    mHardware.SetListenPort(DefaultListenPortFor(EProtocol::iFacialMocapV2, ETransport::UDP));

    return true;
}

void CDevice_FaceMotion::FBDestroy() {}

void CDevice_FaceMotion::SetProtocol(int p)
{
    const auto proto = static_cast<EProtocol>(p);
    mHardware.SetProtocol(proto);
    // Update default ports for the newly selected protocol; the user may
    // override them again from the UI afterwards.
    mHardware.SetPhonePort(IPhonePortFor(proto));
    mHardware.SetListenPort(DefaultListenPortFor(proto, mHardware.GetTransport()));
}

void CDevice_FaceMotion::SetTransport(int t)
{
    const auto trans = static_cast<ETransport>(t);
    mHardware.SetTransport(trans);
    mHardware.SetListenPort(DefaultListenPortFor(mHardware.GetProtocol(), trans));
    CommType = (trans == ETransport::TCP) ? kFBCommTypeNetworkTCP : kFBCommTypeNetworkUDP;
}

bool CDevice_FaceMotion::DeviceOperation(kDeviceOperations pOp)
{
    switch (pOp)
    {
        case kOpInit:  return Init();
        case kOpStart: return Start();
        case kOpStop:  return Stop();
        case kOpReset: return Reset();
        case kOpDone:  return Done();
    }
    return FBDevice::DeviceOperation(pOp);
}

bool CDevice_FaceMotion::Init()
{
    FBProgress p;
    p.Caption = "FaceMotion";
    p.Text    = "Initializing device";
    return true;
}

bool CDevice_FaceMotion::Start()
{
    FBProgress p;
    p.Caption = "Starting FaceMotion";

    p.Text = "Opening device communications";
    Status = "Opening device communications";
    if (!mHardware.Open()) { Status = "Could not open device"; return false; }

    Status = "Getting setup information";
    if (!mHardware.GetSetupInfo()) { Status = "Could not get setup information"; return false; }
    HardwareVersionInfo = "MobuFaceMotion v0.1";
    Information         = "";

    if (mHardware.GetStreaming())
    {
        p.Text = "Sending handshake to iPhone";
        Status = "Starting device streaming";
        if (!mHardware.StartStream()) { Status = "Could not start stream"; return false; }
    }

    Status = "Ok";
    return true;
}

bool CDevice_FaceMotion::Stop()
{
    FBProgress p;
    p.Caption = "Stopping FaceMotion";

    if (mHardware.GetStreaming())
    {
        Status = "Stopping device streaming";
        if (!mHardware.StopStream()) { Status = "Could not stop streaming"; return false; }
    }

    Status = "Stopping device communications";
    if (!mHardware.Close()) { Status = "Could not close device"; return false; }

    Status = "?";
    return false;
}

bool CDevice_FaceMotion::Done()  { return false; }
bool CDevice_FaceMotion::Reset() { Stop(); return Start(); }

bool CDevice_FaceMotion::AnimationNodeNotify(FBAnimationNode* /*pNode*/, FBEvaluateInfo* pInfo)
{
    double pos[3];
    double rot[3];

    const double scale = 0.01 * SpaceScale;

    mHardware.GetPosition(pos);
    mHardware.GetRotation(rot);
    for (int i = 0; i < 3; ++i) pos[i] *= scale;

    mNodeHead_InT->WriteData(pos, pInfo);
    mNodeHead_InR->WriteData(rot, pInfo);

    mHardware.GetLeftEyeRotation(rot);
    rot[2] = 0.0;
    mNodeLeftEye_InR->WriteData(rot, pInfo);

    mHardware.GetRightEyeRotation(rot);
    rot[2] = 0.0;
    mNodeRightEye_InR->WriteData(rot, pInfo);

    for (int i = 0; i < mHardware.GetNumberOfBlendshapes(); ++i)
    {
        double v = ShapeValueMult * mHardware.GetBlendshapeValue(i);
        mNodeBlendshape[i]->WriteData(&v, pInfo);
    }

    // Optional pass-through: drive a target mesh's blendshape deformer
    // channels directly. Driver no-ops if no target is bound.
    FBModel* target = (BlendshapeTarget.GetCount() > 0)
        ? static_cast<FBModel*>(BlendshapeTarget.GetAt(0))
        : nullptr;
    mDriver.Rebind(target);
    if (target)
    {
        mobufacemotion::ParsedFrame snap;
        for (int i = 0; i < static_cast<int>(mobufacemotion::kBlendshapeCount); ++i)
        {
            snap.blendshape[i] = mHardware.GetBlendshapeValue(i);
            snap.blendshapeMask |= (uint64_t(1) << i);
        }
        mDriver.Apply(snap, ShapeValueMult);
    }

    return true;
}

bool CDevice_FaceMotion::DeviceEvaluationNotify(kTransportMode /*pMode*/, FBEvaluateInfo* /*pInfo*/)
{
    return true;
}

void CDevice_FaceMotion::DeviceIONotify(kDeviceIOs pAction, FBDeviceNotifyInfo& pInfo)
{
    switch (pAction)
    {
        case kIOPlayModeWrite:
        case kIOStopModeWrite:
            break;

        case kIOStopModeRead:
        case kIOPlayModeRead:
        {
            const int n = mHardware.FetchData();
            for (int i = 0; i < n; ++i)
            {
                DeviceRecordFrame(pInfo);
                AckOneSampleReceived();
            }
            if (!mHardware.GetStreaming()) mHardware.PollData();
            break;
        }
    }
}

void CDevice_FaceMotion::DeviceRecordFrame(FBDeviceNotifyInfo& pInfo)
{
    if (mPlayerControl.GetTransportMode() != kFBTransportPlay) return;

    const FBTime t = pInfo.GetLocalTime();
    const double scale = 0.01 * SpaceScale;

    double pos[3];
    double rot[3];
    mHardware.GetPosition(pos);
    mHardware.GetRotation(rot);
    for (int i = 0; i < 3; ++i) pos[i] *= scale;

    const bool stamped = (SamplingMode.AsInt() == kFBHardwareTimestamp ||
                          SamplingMode.AsInt() == kFBSoftwareTimestamp);

    auto key = [&](FBAnimationNode* node, double* v) {
        if (FBAnimationNode* dst = node->GetAnimationToRecord())
            stamped ? dst->KeyAdd(t, v) : dst->KeyAdd(v);
    };

    key(mNodeHead_InT, pos);
    key(mNodeHead_InR, rot);

    double tmp[3];
    mHardware.GetLeftEyeRotation(tmp);  tmp[2] = 0.0; key(mNodeLeftEye_InR,  tmp);
    mHardware.GetRightEyeRotation(tmp); tmp[2] = 0.0; key(mNodeRightEye_InR, tmp);

    for (int i = 0; i < mHardware.GetNumberOfBlendshapes(); ++i)
    {
        double v = ShapeValueMult * mHardware.GetBlendshapeValue(i);
        if (FBAnimationNode* dst = mNodeBlendshape[i]->GetAnimationToRecord())
            stamped ? dst->KeyAdd(t, &v) : dst->KeyAdd(&v);
    }
}

void CDevice_FaceMotion::SetCandidates()
{
    double pos[3];
    double rot[3];
    const double scale = 0.01 * SpaceScale;

    mHardware.GetPosition(pos);
    mHardware.GetRotation(rot);
    for (int i = 0; i < 3; ++i) pos[i] *= scale;

    mNodeHead_InT->SetCandidate(pos);
    mNodeHead_InR->SetCandidate(rot);

    mHardware.GetLeftEyeRotation(rot);  rot[2] = 0.0; mNodeLeftEye_InR->SetCandidate(rot);
    mHardware.GetRightEyeRotation(rot); rot[2] = 0.0; mNodeRightEye_InR->SetCandidate(rot);

    for (int i = 0; i < mHardware.GetNumberOfBlendshapes(); ++i)
    {
        double v = ShapeValueMult * mHardware.GetBlendshapeValue(i);
        mNodeBlendshape[i]->SetCandidate(&v);
    }
}
