#pragma once
#include <vulkan/vulkan.h>

struct FExtensions
{
    // SetDebugName
    static PFN_vkSetDebugUtilsObjectNameEXT    vkSetDebugUtilsObjectNameEXT;
    static PFN_vkCreateDebugUtilsMessengerEXT  vkCreateDebugUtilsMessengerEXT;
    static PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;

    // RayTracing
    static PFN_vkCreateAccelerationStructureKHR                 vkCreateAccelerationStructureKHR;
    static PFN_vkDestroyAccelerationStructureKHR                vkDestroyAccelerationStructureKHR;
    static PFN_vkCmdBuildAccelerationStructuresKHR              vkCmdBuildAccelerationStructuresKHR;
    static PFN_vkCmdBuildAccelerationStructuresIndirectKHR      vkCmdBuildAccelerationStructuresIndirectKHR;
    static PFN_vkBuildAccelerationStructuresKHR                 vkBuildAccelerationStructuresKHR;
    static PFN_vkCopyAccelerationStructureKHR                   vkCopyAccelerationStructureKHR;
    static PFN_vkCopyAccelerationStructureToMemoryKHR           vkCopyAccelerationStructureToMemoryKHR;
    static PFN_vkCopyMemoryToAccelerationStructureKHR           vkCopyMemoryToAccelerationStructureKHR;
    static PFN_vkWriteAccelerationStructuresPropertiesKHR       vkWriteAccelerationStructuresPropertiesKHR;
    static PFN_vkCmdCopyAccelerationStructureKHR                vkCmdCopyAccelerationStructureKHR;
    static PFN_vkCmdCopyAccelerationStructureToMemoryKHR        vkCmdCopyAccelerationStructureToMemoryKHR;
    static PFN_vkCmdCopyMemoryToAccelerationStructureKHR        vkCmdCopyMemoryToAccelerationStructureKHR;
    static PFN_vkGetAccelerationStructureDeviceAddressKHR       vkGetAccelerationStructureDeviceAddressKHR;
    static PFN_vkCmdWriteAccelerationStructuresPropertiesKHR    vkCmdWriteAccelerationStructuresPropertiesKHR;
    static PFN_vkGetDeviceAccelerationStructureCompatibilityKHR vkGetDeviceAccelerationStructureCompatibilityKHR;
    static PFN_vkGetAccelerationStructureBuildSizesKHR          vkGetAccelerationStructureBuildSizesKHR;
    static PFN_vkGetRayTracingShaderGroupHandlesKHR             vkGetRayTracingShaderGroupHandlesKHR;

    static PFN_vkCmdTraceRaysKHR                                 vkCmdTraceRaysKHR;
    static PFN_vkCreateRayTracingPipelinesKHR                    vkCreateRayTracingPipelinesKHR;
    static PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR vkGetRayTracingCaptureReplayShaderGroupHandlesKHR;
    static PFN_vkCmdTraceRaysIndirectKHR                         vkCmdTraceRaysIndirectKHR;
    static PFN_vkGetRayTracingShaderGroupStackSizeKHR            vkGetRayTracingShaderGroupStackSizeKHR;
    static PFN_vkCmdSetRayTracingPipelineStackSizeKHR            vkCmdSetRayTracingPipelineStackSizeKHR;
};