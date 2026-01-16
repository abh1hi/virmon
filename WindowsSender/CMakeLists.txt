cmake_minimum_required(VERSION 3.10)
project(VirMonSender)

set(CMAKE_CXX_STANDARD 17)

# Sources
set(SOURCES
    CaptureService.cpp
    EncoderNVENC.cpp
    Transport.cpp
    NetworkTransport.cpp
    NetworkDiscovery.cpp
    # Driver.cpp is NOT part of the user-mode service
)

# Executable
# Main Capture Service
add_executable(CaptureService CaptureService.cpp)
target_link_libraries(CaptureService d3d11 dxgi windowscodecs shlwapi ws2_32 setupapi)

# Device Installer (Modern PnP)
add_executable(DeviceInstaller DeviceInstaller.cpp)
target_link_libraries(DeviceInstaller setupapi newdev)

# Libraries
if(MSVC)
    # The libraries for CaptureService are now linked directly above.
    # This block can be used for other MSVC-specific libraries if needed.
    # For now, it's empty as per the instruction's implied change.
endif()

# Output Directory
set_target_properties(CaptureService PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/x64"
)
