#define WIN32_LEAN_AND_MEAN
#include <winsock2.h> // Include this before windows.h
#include <windows.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include <iostream>
#include <vector>
#include <thread>
#include <setupapi.h>
#include <devguid.h>
#include <initguid.h>
#include <usbiodef.h>

// Include our components
#include "Encoder.h"
#include "EncoderWIC.cpp" // Use WIC for JPEG (Real Video)
#include "NetworkTransport.cpp"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

class ScreenCapture {
    CComPtr<ID3D11Device> m_Device;
    CComPtr<ID3D11DeviceContext> m_Context;
    CComPtr<IDXGIOutputDuplication> m_DeskDupl;
    DXGI_OUTPUT_DESC m_OutputDesc;
    
    NetworkTransport m_Transport;
    EncoderWIC m_Encoder; 

public:
    bool Init() {
        // 1. Initialize Video Pipeline (D3D11)
        HRESULT hr = S_OK;
        D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            featureLevels, 1, D3D11_SDK_VERSION, &m_Device, nullptr, &m_Context);
        if (FAILED(hr)) {
            std::cerr << "D3D11CreateDevice Failed: " << std::hex << hr << std::endl;
            return false;
        }

        CComPtr<IDXGIDevice> dxgiDevice;
        m_Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
        CComPtr<IDXGIAdapter> dxgiAdapter;
        dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);

        // Find the correct output (Virtual Monitor)
        CComPtr<IDXGIOutput> dxgiOutput;
        if (dxgiAdapter->EnumOutputs(0, &dxgiOutput) == DXGI_ERROR_NOT_FOUND) {
             std::cerr << "No Outputs found on adapter" << std::endl;
             return false;
        }
        
        CComPtr<IDXGIOutput1> dxgiOutput1;
        dxgiOutput->QueryInterface(__uuidof(IDXGIOutput1), (void**)&dxgiOutput1);
        hr = dxgiOutput1->DuplicateOutput(m_Device, &m_DeskDupl);
        if (FAILED(hr)) {
             std::cerr << "DuplicateOutput Failed (HR=" << std::hex << hr << ")" << std::endl;
             std::cerr << "Make sure a monitor is connected and active." << std::endl;
             return false;
        }

        dxgiOutput->GetDesc(&m_OutputDesc);
        int w = m_OutputDesc.DesktopCoordinates.right - m_OutputDesc.DesktopCoordinates.left;
        int h = m_OutputDesc.DesktopCoordinates.bottom - m_OutputDesc.DesktopCoordinates.top;

        std::cout << "Screen Capture Initialized: " << w << "x" << h << std::endl;

        // 2. Initialize Encoder
        if (!m_Encoder.Initialize(m_Device, w, h, 60)) {
            std::cerr << "Encoder Init Failed" << std::endl;
            return false;
        }

        // 3. Initialize Network Transport
        if (!m_Transport.Start()) {
            std::cerr << "Network Transport Failed to Start" << std::endl;
            return false;
        }
        
        return true;
    }

    void Run() {
        std::cout << "Starting Wireless Capture Service..." << std::endl;
        std::cout << "Waiting for connections..." << std::endl;

        CaptureLoop();
    }

    void CaptureLoop() {
        uint64_t frameCount = 0;
        std::cout << "Streaming Loop Active." << std::endl;

        while (true) {
            CComPtr<IDXGIResource> desktopResource;
            DXGI_OUTDUPL_FRAME_INFO frameInfo;
            
            HRESULT hr = m_DeskDupl->AcquireNextFrame(50, &frameInfo, &desktopResource);
            if (hr == DXGI_ERROR_WAIT_TIMEOUT) continue;
            if (FAILED(hr)) {
                std::cerr << "Lost Desktop Duplication access. (HR=" << std::hex << hr << ")" << std::endl;
                break; 
            }

            if (frameInfo.LastPresentTime.QuadPart > 0) {
                CComPtr<ID3D11Texture2D> tex;
                desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&tex);
                
                m_Encoder.EncodeFrame(tex, frameCount++);
                
                EncodedPacket pkt;
                if (m_Encoder.GetPacket(pkt)) {
                    // Send via Network
                    // WIC (JPEG) is always Keyframe. Timestamps roughly 60fps interval.
                    bool keyframe = true; 
                    if (m_Transport.SendVideoPacket(pkt.data, frameCount * 16666, keyframe)) {
                         if (frameCount % 60 == 0) std::cout << "Sent Frame " << frameCount << " (Size: " << pkt.data.size() << ")" << "\r" << std::flush;
                    }
                }
            }
            m_DeskDupl->ReleaseFrame();
        }
    }
};

int main() {
    ScreenCapture app;
    if (app.Init()) {
        app.Run();
    } else {
        std::cerr << "Initialization Failed." << std::endl;
        Sleep(5000);
        return 1;
    }
    return 0;
}
