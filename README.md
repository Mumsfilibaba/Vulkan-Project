# Vulkan Path Tracer

## Overview

This project is a Vulkan-based path tracer developed in C++ and HLSL, featuring both software and hardware-accelerated ray tracing. Initially,
it focused on constructing and optimizing Bounding Volume Hierarchies (BVH) using Surface Area Heuristics (SAH) to enhance ray tracing performance.
It has since evolved into a platform for experimenting with various ray tracing and path tracing techniques.

## Current Features
- **Simple Path Tracing**: Simple non-physically based path tracing.
- **Russian Roulette**: Uses Russian roulette to speed up tracing and avoid paths that do not significantly contribute to the final result.
- **Software Ray Tracing**: Implements a triangle path tracer in compute shaders using a custom-built BVH and traversal.
- **Hardware Accelerated Ray Tracing**: Supports a hardware-accelerated path (using `VK_KHR_acceleration_structure` and `VK_KHR_ray_tracing_pipeline` Vulkan extensions).
- **Unified Shader Code Paths**: The software and hardware ray tracing paths share a common shading and tracing core, with backend-specific modules for each path.
- **Triangle Mesh Instancing**: Supports instancing of triangle meshes in the software ray tracing path with optimized TLAS traversal.
- **HLSL Shaders**: All shaders are written in HLSL and compiled to SPIR-V.
- **BVH Construction**: Constructs a BVH using surface area heuristics (SAH).
- **Descriptor Buffer**: Supports `VK_EXT_descriptor_buffer` for efficient descriptor management.
- **Dynamic Rendering**: Supports `VK_KHR_dynamic_rendering` to avoid the use of render pass objects.
- **Mouse Camera Movement**: Interactive camera control via mouse input.
- **Cross-Platform**: Can be run on both Windows and macOS.
- **Debug Views**: Contains various debug views such as normals, texcoords, albedo, barycentric coords etc. The software version also contains bvh-intersection debug-views and the ability to view the BVH.

## Planned Features
- **Physically Based Light Model**: The current path tracer is very simple and is not necessarily physically based, even though things such as Fresnel are taken into account. However, a more physically accurate BRDF would improve the look.
- **Importance Sampling**: Generate more useful rays using importance sampling.
- **D3D12 Port**: Port to D3D12.

## Getting Started

### Prerequisites

Before you begin, ensure you have the following installed:

- Visual Studio 2022 or 2019 (for Windows), Xcode (for macOS)
- A compiler that support C++20 or later
- Vulkan SDK (1.3.250.1)

### Building the Project

1. **Clone the Repository**

    ```bash
    git clone -b Path-Tracer https://github.com/Mumsfilibaba/Vulkan-Project.git
    cd Vulkan-Project
    ```

2. **Update Dependencies**

    - For Windows:
        ```bash
        ./Update_Dependencies.bat
        ```

    - For macOS:
        ```bash
        ./Update_Dependencies.command
        ```

3. **Generate Project Files**

    - For Visual Studio 2022:
        ```bash
        ./Premake_VS2022.bat
        ```
    - For Visual Studio 2019:
        ```bash
        ./Premake_VS2019.bat
        ```
    - For Xcode:
        ```bash
        ./Premake_xcode.command
        ```

4. **Build and Run the Project**

    - Open the generated solution or workspace in Visual Studio (for Windows) or Xcode (for macOS).
    - Build and run the project from your chosen IDE.

## Screenshots
![Sponza](Screenshots/screen0.png)

![Spheres1](Screenshots/screen1.png)

![Spheres2](Screenshots/screen2.png)

![Spheres3](Screenshots/screen3.png)

![Cornell](Screenshots/screen4.png)

![SoftwareIntersection](Screenshots/screen5.png)

![BVHDebug](Screenshots/screen6.png)
