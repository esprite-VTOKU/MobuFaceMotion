#pragma once

/** \file device_faceMotion_hardware.h
 *
 *  Socket lifecycle, handshake send, and per-frame receive/parse for the
 *  iFacialMocap and FaceMotion3D protocols. Owned by CDevice_FaceMotion.
 */

#include <fbsdk/fbsdk.h>

#include "protocol.h"

namespace mobufacemotion { class BlendshapeDriver; }

#define MOBU_FACEMOTION_RECV_BUFFER  4096

class CDevice_FaceMotion;

class CDevice_FaceMotion_Hardware
{
public:
    CDevice_FaceMotion_Hardware();
    ~CDevice_FaceMotion_Hardware();

    void SetParent(FBDevice* pParent) { mParent = pParent; }

    // FBDevice IO surface ----------------------------------------------------
    bool Open();
    bool Close();
    bool GetSetupInfo() { return true; }
    bool StartStream();
    bool StopStream();
    bool PollData()   { return true; }
    int  FetchData();

    // Configuration (driven by the device's published properties) -----------
    void SetProtocol(mobufacemotion::EProtocol p)   { mProtocol = p; }
    mobufacemotion::EProtocol GetProtocol() const   { return mProtocol; }

    void SetTransport(mobufacemotion::ETransport t) { mTransport = t; }
    mobufacemotion::ETransport GetTransport() const { return mTransport; }

    void  SetPhoneIp(const char* ip);
    const char* GetPhoneIp() const                  { return mPhoneIp; }

    void SetPhonePort(int port)   { mPhonePort = port; }
    int  GetPhonePort() const     { return mPhonePort; }

    void SetListenPort(int port)  { mListenPort = port; }
    int  GetListenPort() const    { return mListenPort; }

    void SetStreaming(bool b)     { mStreaming = b; }
    bool GetStreaming() const     { return mStreaming; }

    // Latest parsed sample (filled by FetchData) ----------------------------
    void GetPosition(double* out) const;
    void GetRotation(double* out) const;
    void GetLeftEyeRotation(double* out) const;
    void GetRightEyeRotation(double* out) const;
    int  GetNumberOfBlendshapes() const;
    double GetBlendshapeValue(int index) const;

    // Diagnostics ------------------------------------------------------------
    uint64_t GetFrameCount() const     { return mFrameCount; }
    int64_t  GetLastFrameTickMs() const{ return mLastFrameTickMs; }

private:
    bool SendHandshakeUDP();
    void CloseSockets();
    bool StartUdpListen();
    bool StartTcpListen();
    int  AcceptTcpClient();   // non-blocking; returns socket or 0
    int  ReceiveOnce(char* buf, int cap);

    FBDevice* mParent = nullptr;

    mobufacemotion::EProtocol  mProtocol  = mobufacemotion::EProtocol::iFacialMocapV2;
    mobufacemotion::ETransport mTransport = mobufacemotion::ETransport::UDP;

    char mPhoneIp[64] = "";
    int  mPhonePort   = 49983;
    int  mListenPort  = 49983;
    bool mStreaming   = true;

    // Sockets: mSocket is the active per-frame socket (UDP listener, or
    // accepted TCP client). mTcpServer is only used for TCP transport, to
    // hold the listening socket between accept() calls.
    int mSocket    = 0;
    int mTcpServer = 0;

    char mBuffer[MOBU_FACEMOTION_RECV_BUFFER] = {};

    mobufacemotion::ParsedFrame mLatest;

    uint64_t mFrameCount       = 0;
    int64_t  mLastFrameTickMs  = 0;
};
