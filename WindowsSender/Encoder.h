#pragma once
#include <d3d11.h>
#include <vector>
#include <cstdint>

struct EncodedPacket {
    std::vector<uint8_t> data;
    uint64_t timestamp;
    bool isKeyFrame;
};

// Helper for CComPtr if ATL not available
template <typename T>
class CComPtr {
    T* p;
public:
    CComPtr() : p(nullptr) {}
    ~CComPtr() { if (p) p->Release(); }
    T** operator&() { return &p; }
    operator T*() { return p; }
    T* operator->() { return p; }
    T* operator=(T* ptr) { if (p) p->Release(); p = ptr; if(p) p->AddRef(); return p; }
    bool operator!() { return !p; }
};

class IVideoEncoder {
public:
    virtual ~IVideoEncoder() = default;

    virtual bool Initialize(ID3D11Device* device, int width, int height, int fps) = 0;
    virtual bool EncodeFrame(ID3D11Texture2D* texture, uint64_t timestamp) = 0;
    virtual bool GetPacket(EncodedPacket& outPacket) = 0;
};
