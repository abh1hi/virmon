#include "Encoder.h"
#include <wincodec.h>
#include <shlwapi.h>
#include <iostream>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "shlwapi.lib")

// Helper for Release
template<typename T>
void SafeRelease(T** ppT) {
    if (*ppT) {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

class EncoderWIC : public IVideoEncoder {
    IWICImagingFactory* m_pFactory;
    CComPtr<ID3D11Device> m_Device;
    CComPtr<ID3D11DeviceContext> m_Context;
    
    // CPU staging texture to copy GPU texture to, so WIC can read it
    ID3D11Texture2D* m_StagingTexture;
    
    EncodedPacket m_LastPacket;
    int m_Width;
    int m_Height;

public:
    EncoderWIC() : m_pFactory(NULL), m_StagingTexture(NULL), m_Width(0), m_Height(0) {}

    ~EncoderWIC() {
        if (m_StagingTexture) SafeRelease(&m_StagingTexture);
        if (m_pFactory) SafeRelease(&m_pFactory);
        CoUninitialize();
    }

    bool Initialize(ID3D11Device* device, int width, int height, int fps) override {
        m_Device = device;
        m_Device->GetImmediateContext(&m_Context);
        m_Width = width;
        m_Height = height;

        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        
        hr = CoCreateInstance(
            CLSID_WICImagingFactory,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&m_pFactory)
        );

        if (FAILED(hr)) {
            std::cerr << "Failed to create WIC Factory: " << std::hex << hr << std::endl;
            return false;
        }

        // Create Staging Texture (CPU Readable)
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // WIC likes BGRA
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags = 0;

        hr = m_Device->CreateTexture2D(&desc, NULL, &m_StagingTexture);
        if (FAILED(hr)) {
            std::cerr << "Failed to create staging texture: " << std::hex << hr << std::endl;
            return false;
        }

        std::cout << "WIC Encoder Initialized (JPEG)." << std::endl;
        return true;
    }

    bool EncodeFrame(ID3D11Texture2D* texture, uint64_t timestamp) override {
        if (!m_pFactory || !m_StagingTexture) return false;

        // 1. Copy GPU Texture to Staging
        m_Context->CopyResource(m_StagingTexture, texture);

        // 2. Map Staging Texture
        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = m_Context->Map(m_StagingTexture, 0, D3D11_MAP_READ, 0, &mapped);
        if (FAILED(hr)) return false;

        // 3. Create WIC Bitmap from Memory
        IWICBitmap* pBitmap = NULL;
        hr = m_pFactory->CreateBitmapFromMemory(
            m_Width,
            m_Height,
            GUID_WICPixelFormat32bppBGRA,
            mapped.RowPitch,
            mapped.RowPitch * m_Height,
            (BYTE*)mapped.pData,
            &pBitmap
        );
        
        // Unmap immediately after creating bitmap wrapper (usually safe if WIC copies, but verify)
        // WIC CreateBitmapFromMemory usually copies. If CreateBitmapFromMemory doesn't copy, we must unmap later.
        // Docs: "pBuffer - The size of the buffer." - Wait, it copies. 
        // "This method creates a bitmap ... from the memory buffer." 
        // Actually, for performance we might want wrapping, but CreateBitmapFromMemory copies.
        
        m_Context->Unmap(m_StagingTexture, 0);

        if (FAILED(hr)) {
             std::cerr << "WIC CreateBitmap failed" << std::endl;
             return false;
        }

        // 4. Encode to Memory Stream
        IStream* pStream = NULL;
        CreateStreamOnHGlobal(NULL, TRUE, &pStream);
        
        IWICBitmapEncoder* pEncoder = NULL;
        m_pFactory->CreateEncoder(GUID_ContainerFormatJpeg, NULL, &pEncoder);
        pEncoder->Initialize(pStream, (WICBitmapEncoderCacheOption)0x2); // WICBitmapEncoderNoCache
        
        IWICBitmapFrameEncode* pFrame = NULL;
        pEncoder->CreateNewFrame(&pFrame, NULL);
        pFrame->Initialize(NULL);
        
        // Quality Setting (Optional)
        // IPropertyBag2* pPropertyBag = NULL; ...

        pFrame->SetSize(m_Width, m_Height);
        WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
        pFrame->SetPixelFormat(&format);
        pFrame->WriteSource(pBitmap, NULL);
        
        pFrame->Commit();
        pEncoder->Commit();

        // 5. Get Data
        HGLOBAL hMem = NULL;
        GetHGlobalFromStream(pStream, &hMem);
        void* pData = GlobalLock(hMem);
        DWORD size = 0;
        // GlobalSize might be larger than actual stream usage, but Stream Seek/Stat is better.
        STATSTG stat;
        pStream->Stat(&stat, STATFLAG_NONAME);
        size = (DWORD)stat.cbSize.QuadPart;

        m_LastPacket.data.resize(size);
        memcpy(m_LastPacket.data.data(), pData, size);
        
        GlobalUnlock(hMem);
        
        // Cleanup
        SafeRelease(&pFrame);
        SafeRelease(&pEncoder);
        SafeRelease(&pBitmap);
        SafeRelease(&pStream);
        
        m_LastPacket.timestamp = timestamp;
        m_LastPacket.isKeyFrame = true; // Every JPEG is a keyframe

        return true;
    }

    bool GetPacket(EncodedPacket& outPacket) override {
        outPacket = m_LastPacket;
        // Clear old packet data to avoid duplicate sends if needed, but sender loop handles "Get" calls implies consumption
        return !outPacket.data.empty();
    }
};
