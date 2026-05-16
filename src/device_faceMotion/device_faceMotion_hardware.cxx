/** \file device_faceMotion_hardware.cxx */

#include "device_faceMotion_hardware.h"

#include <cstring>

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

using namespace mobufacemotion;

namespace {

bool g_wsaInitialized = false;
int  g_wsaRefcount    = 0;

bool WsaAcquire()
{
    if (g_wsaRefcount++ == 0)
    {
        WSADATA d;
        if (WSAStartup(MAKEWORD(2, 2), &d) != 0) { g_wsaRefcount = 0; return false; }
        g_wsaInitialized = true;
    }
    return g_wsaInitialized;
}

void WsaRelease()
{
    if (g_wsaInitialized && --g_wsaRefcount == 0)
    {
        WSACleanup();
        g_wsaInitialized = false;
    }
}

void SetNonBlocking(int sock)
{
    u_long nb = 1;
    ioctlsocket(static_cast<SOCKET>(sock), FIONBIO, &nb);
}

} // namespace

CDevice_FaceMotion_Hardware::CDevice_FaceMotion_Hardware()
{
    WsaAcquire();
}

CDevice_FaceMotion_Hardware::~CDevice_FaceMotion_Hardware()
{
    CloseSockets();
    WsaRelease();
}

void CDevice_FaceMotion_Hardware::SetPhoneIp(const char* ip)
{
    if (!ip) { mPhoneIp[0] = 0; return; }
    std::strncpy(mPhoneIp, ip, sizeof(mPhoneIp) - 1);
    mPhoneIp[sizeof(mPhoneIp) - 1] = 0;
}

void CDevice_FaceMotion_Hardware::SetRelayIp(const char* ip)
{
    if (!ip) { mRelayIp[0] = 0; return; }
    std::strncpy(mRelayIp, ip, sizeof(mRelayIp) - 1);
    mRelayIp[sizeof(mRelayIp) - 1] = 0;
}

bool CDevice_FaceMotion_Hardware::Open()  { return true; }
bool CDevice_FaceMotion_Hardware::Close() { return StopStream(); }

bool CDevice_FaceMotion_Hardware::StartStream()
{
    if (mTransport == ETransport::TCP)
    {
        if (!StartTcpListen()) return false;
    }
    else
    {
        if (!StartUdpListen()) return false;
    }

    // Send handshake last, so by the time the phone replies we are listening.
    // FaceMotion3D's docs recommend triple-sending the handshake to defeat
    // single-packet UDP loss; iFacialMocap is fine with one, but triple-send
    // is harmless and uniform.
    if (mPhoneIp[0])
    {
        for (int i = 0; i < 3; ++i) SendHandshakeUDP();
    }

    if (mRelayEnabled) OpenRelaySocket();
    return true;
}

bool CDevice_FaceMotion_Hardware::StopStream()
{
    CloseSockets();
    return true;
}

void CDevice_FaceMotion_Hardware::CloseSockets()
{
    if (mSocket)    { closesocket(static_cast<SOCKET>(mSocket));    mSocket = 0; }
    if (mTcpServer) { closesocket(static_cast<SOCKET>(mTcpServer)); mTcpServer = 0; }
    CloseRelaySocket();
}

void CDevice_FaceMotion_Hardware::OpenRelaySocket()
{
    CloseRelaySocket();
    const SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) return;
    SetNonBlocking(static_cast<int>(s));
    mRelaySocket = static_cast<int>(s);
}

void CDevice_FaceMotion_Hardware::CloseRelaySocket()
{
    if (mRelaySocket)
    {
        closesocket(static_cast<SOCKET>(mRelaySocket));
        mRelaySocket = 0;
    }
}

void CDevice_FaceMotion_Hardware::ForwardRaw(const char* buf, int len)
{
    if (!mRelayEnabled || !mRelayIp[0] || mRelayPort <= 0) return;
    if (!mRelaySocket) OpenRelaySocket();
    if (!mRelaySocket) return;

    sockaddr_in dst = {};
    dst.sin_family = AF_INET;
    dst.sin_port   = htons(static_cast<uint16_t>(mRelayPort));
    if (inet_pton(AF_INET, mRelayIp, &dst.sin_addr) != 1) return;

    const int sent = sendto(static_cast<SOCKET>(mRelaySocket), buf, len, 0,
                            reinterpret_cast<sockaddr*>(&dst), sizeof(dst));
    if (sent == len) ++mRelayCount;
}

bool CDevice_FaceMotion_Hardware::SendHandshakeUDP()
{
    if (!mPhoneIp[0]) return false;

    const SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) return false;

    sockaddr_in dst = {};
    dst.sin_family = AF_INET;
    dst.sin_port   = htons(static_cast<uint16_t>(mPhonePort));
    if (inet_pton(AF_INET, mPhoneIp, &dst.sin_addr) != 1)
    {
        closesocket(s);
        return false;
    }

    const std::string_view payload = HandshakeFor(mProtocol, mTransport);
    const int sent = sendto(s, payload.data(), static_cast<int>(payload.size()), 0,
                            reinterpret_cast<sockaddr*>(&dst), sizeof(dst));
    closesocket(s);
    return sent == static_cast<int>(payload.size());
}

bool CDevice_FaceMotion_Hardware::StartUdpListen()
{
    const SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) return false;

    sockaddr_in addr = {};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(static_cast<uint16_t>(mListenPort));
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
    {
        closesocket(s);
        return false;
    }
    SetNonBlocking(static_cast<int>(s));
    mSocket = static_cast<int>(s);
    return true;
}

bool CDevice_FaceMotion_Hardware::StartTcpListen()
{
    const SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return false;

    BOOL reuse = TRUE;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in addr = {};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(static_cast<uint16_t>(mListenPort));
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 ||
        listen(s, 1) != 0)
    {
        closesocket(s);
        return false;
    }
    SetNonBlocking(static_cast<int>(s));
    mTcpServer = static_cast<int>(s);
    mSocket    = 0;
    return true;
}

int CDevice_FaceMotion_Hardware::AcceptTcpClient()
{
    if (!mTcpServer) return 0;
    sockaddr_in client = {};
    int len = sizeof(client);
    const SOCKET s = accept(static_cast<SOCKET>(mTcpServer),
                            reinterpret_cast<sockaddr*>(&client), &len);
    if (s == INVALID_SOCKET) return 0;
    SetNonBlocking(static_cast<int>(s));
    return static_cast<int>(s);
}

int CDevice_FaceMotion_Hardware::ReceiveOnce(char* buf, int cap)
{
    if (!mSocket) return 0;
    sockaddr_in from = {};
    int fromLen = sizeof(from);
    const int n = (mTransport == ETransport::TCP)
        ? recv(static_cast<SOCKET>(mSocket), buf, cap, 0)
        : recvfrom(static_cast<SOCKET>(mSocket), buf, cap, 0,
                   reinterpret_cast<sockaddr*>(&from), &fromLen);
    return (n == SOCKET_ERROR) ? 0 : n;
}

int CDevice_FaceMotion_Hardware::FetchData()
{
    int frames = 0;

    if (mTransport == ETransport::TCP && !mSocket && mTcpServer)
    {
        mSocket = AcceptTcpClient();
        if (!mSocket) return 0;
    }

    if (!mSocket) return 0;

    // Drain everything pending on the socket. For UDP, each datagram is one
    // frame and we keep the most recent (newer wins). For TCP, ReceiveOnce
    // returns whatever a single recv() yielded; we treat it as a complete
    // frame (current iFacialMocap/FaceMotion3D senders fit a frame into a
    // single segment in practice).
    for (;;)
    {
        const int n = ReceiveOnce(mBuffer, sizeof(mBuffer) - 1);
        if (n <= 0) break;
        mBuffer[n] = 0;

        ParsedFrame f;
        if (ParsePacket(mBuffer, n, mProtocol, f))
        {
            mLatest = f;
            ++mFrameCount;
            mLastFrameTickMs = static_cast<int64_t>(GetTickCount64());
            frames = 1;

            // Forward raw bytes to the relay destination (if enabled). We
            // forward AFTER parse success so malformed packets don't pollute
            // the downstream listener.
            ForwardRaw(mBuffer, n);
        }
    }

    return frames;
}

void CDevice_FaceMotion_Hardware::GetPosition(double* out) const
{
    out[0] = mLatest.headPosCm[0];
    out[1] = mLatest.headPosCm[1];
    out[2] = mLatest.headPosCm[2];
}

void CDevice_FaceMotion_Hardware::GetRotation(double* out) const
{
    out[0] = mLatest.headRotDeg[0];
    out[1] = mLatest.headRotDeg[1];
    out[2] = mLatest.headRotDeg[2];
}

void CDevice_FaceMotion_Hardware::GetLeftEyeRotation(double* out) const
{
    out[0] = mLatest.leftEyeDeg[0];
    out[1] = mLatest.leftEyeDeg[1];
    out[2] = mLatest.leftEyeDeg[2];
}

void CDevice_FaceMotion_Hardware::GetRightEyeRotation(double* out) const
{
    out[0] = mLatest.rightEyeDeg[0];
    out[1] = mLatest.rightEyeDeg[1];
    out[2] = mLatest.rightEyeDeg[2];
}

int CDevice_FaceMotion_Hardware::GetNumberOfBlendshapes() const
{
    return static_cast<int>(kBlendshapeCount);
}

double CDevice_FaceMotion_Hardware::GetBlendshapeValue(int index) const
{
    if (index < 0 || static_cast<uint32_t>(index) >= kBlendshapeCount) return 0.0;
    return mLatest.blendshape[index];
}
