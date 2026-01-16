#include <windows.h>
#include <winusb.h>
#include <iostream>
#include <vector>
#include <string>

#pragma comment(lib, "winusb.lib")
#pragma comment(lib, "setupapi.lib")

class UsbTransport {
    HANDLE m_DeviceHandle;
    WINUSB_INTERFACE_HANDLE m_WinUsbHandle;
    UCHAR m_BulkOutPipe;
    UCHAR m_BulkInPipe;

public:
    UsbTransport() : m_DeviceHandle(INVALID_HANDLE_VALUE), m_WinUsbHandle(NULL), m_BulkOutPipe(0x01), m_BulkInPipe(0x81) {}

    ~UsbTransport() {
        if (m_WinUsbHandle) WinUsb_Free(m_WinUsbHandle);
        if (m_DeviceHandle != INVALID_HANDLE_VALUE) CloseHandle(m_DeviceHandle);
    }

    bool Initialize(const std::wstring& devicePath) {
        // Open Device with Overlapped flag for async potential
        m_DeviceHandle = CreateFileW(devicePath.c_str(), 
            GENERIC_WRITE | GENERIC_READ, 
            FILE_SHARE_WRITE | FILE_SHARE_READ, 
            NULL, OPEN_EXISTING, 
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

        if (m_DeviceHandle == INVALID_HANDLE_VALUE) {
            // std::cerr << "Failed to open device handle. Error: " << GetLastError() << std::endl;
            return false;
        }

        if (!WinUsb_Initialize(m_DeviceHandle, &m_WinUsbHandle)) {
            std::cerr << "WinUsb_Initialize failed." << std::endl;
            CloseHandle(m_DeviceHandle);
            m_DeviceHandle = INVALID_HANDLE_VALUE;
            return false;
        }

        // Hardcoded generic endpoints for Accessory Mode
        // Usually Bulk IN is 0x81, Bulk OUT is 0x02 or 0x01
        USB_INTERFACE_DESCRIPTOR ifaceDesc;
        WinUsb_QueryInterfaceSettings(m_WinUsbHandle, 0, &ifaceDesc);
        
        for (int i=0; i<ifaceDesc.bNumEndpoints; i++) {
            WINUSB_PIPE_INFORMATION pipeInfo;
            WinUsb_QueryPipe(m_WinUsbHandle, 0, (UCHAR)i, &pipeInfo);
            if (USB_ENDPOINT_DIRECTION_OUT(pipeInfo.PipeId)) {
                m_BulkOutPipe = pipeInfo.PipeId;
            } else if (USB_ENDPOINT_DIRECTION_IN(pipeInfo.PipeId)) {
                m_BulkInPipe = pipeInfo.PipeId;
            }
        }
        
        std::cout << "USB Transsport Init. OUT:" << (int)m_BulkOutPipe << " IN:" << (int)m_BulkInPipe << std::endl;
        return true;
    }

    bool SendVideoPacket(const std::vector<uint8_t>& data) {
        if (!m_WinUsbHandle) return false;

        // Header: [Type:1][Len:4][TS:8][Seq:4] = 17 bytes
        // But for simplicity of this logic, let's just send [Type][Len][Data]
        
        uint32_t len = (uint32_t)data.size();
        std::vector<uint8_t> buffer(5 + len);
        buffer[0] = 0x01; // VIDEO Type
        memcpy(&buffer[1], &len, 4); // Little endian length
        memcpy(&buffer[5], data.data(), len);

        ULONG bytesWritten = 0;
        BOOL result = WinUsb_WritePipe(m_WinUsbHandle, m_BulkOutPipe, (PUCHAR)buffer.data(), (ULONG)buffer.size(), &bytesWritten, NULL);
        
        if (!result) {
            std::cerr << "USB Write Failed: " << GetLastError() << std::endl;
            return false;
        }
        return true;
    }
};
