#include "Extensions.h"

// SetDebugName
PFN_vkSetDebugUtilsObjectNameEXT    FExtensions::vkSetDebugUtilsObjectNameEXT    = nullptr;
PFN_vkCreateDebugUtilsMessengerEXT  FExtensions::vkCreateDebugUtilsMessengerEXT  = nullptr;
PFN_vkDestroyDebugUtilsMessengerEXT FExtensions::vkDestroyDebugUtilsMessengerEXT = nullptr;

// RayTracing
PFN_vkCreateAccelerationStructureKHR                 FExtensions::vkCreateAccelerationStructureKHR                 = nullptr;
PFN_vkDestroyAccelerationStructureKHR                FExtensions::vkDestroyAccelerationStructureKHR                = nullptr;
PFN_vkCmdBuildAccelerationStructuresKHR              FExtensions::vkCmdBuildAccelerationStructuresKHR              = nullptr;
PFN_vkCmdBuildAccelerationStructuresIndirectKHR      FExtensions::vkCmdBuildAccelerationStructuresIndirectKHR      = nullptr;
PFN_vkBuildAccelerationStructuresKHR                 FExtensions::vkBuildAccelerationStructuresKHR                 = nullptr;
PFN_vkCopyAccelerationStructureKHR                   FExtensions::vkCopyAccelerationStructureKHR                   = nullptr;
PFN_vkCopyAccelerationStructureToMemoryKHR           FExtensions::vkCopyAccelerationStructureToMemoryKHR           = nullptr;
PFN_vkCopyMemoryToAccelerationStructureKHR           FExtensions::vkCopyMemoryToAccelerationStructureKHR           = nullptr;
PFN_vkWriteAccelerationStructuresPropertiesKHR       FExtensions::vkWriteAccelerationStructuresPropertiesKHR       = nullptr;
PFN_vkCmdCopyAccelerationStructureKHR                FExtensions::vkCmdCopyAccelerationStructureKHR                = nullptr;
PFN_vkCmdCopyAccelerationStructureToMemoryKHR        FExtensions::vkCmdCopyAccelerationStructureToMemoryKHR        = nullptr;
PFN_vkCmdCopyMemoryToAccelerationStructureKHR        FExtensions::vkCmdCopyMemoryToAccelerationStructureKHR        = nullptr;
PFN_vkGetAccelerationStructureDeviceAddressKHR       FExtensions::vkGetAccelerationStructureDeviceAddressKHR       = nullptr;
PFN_vkCmdWriteAccelerationStructuresPropertiesKHR    FExtensions::vkCmdWriteAccelerationStructuresPropertiesKHR    = nullptr;
PFN_vkGetDeviceAccelerationStructureCompatibilityKHR FExtensions::vkGetDeviceAccelerationStructureCompatibilityKHR = nullptr;
PFN_vkGetAccelerationStructureBuildSizesKHR          FExtensions::vkGetAccelerationStructureBuildSizesKHR          = nullptr;
PFN_vkGetRayTracingShaderGroupHandlesKHR             FExtensions::vkGetRayTracingShaderGroupHandlesKHR             = nullptr;

PFN_vkCmdTraceRaysKHR                                 FExtensions::vkCmdTraceRaysKHR                                 = nullptr;
PFN_vkCreateRayTracingPipelinesKHR                    FExtensions::vkCreateRayTracingPipelinesKHR                    = nullptr;
PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR FExtensions::vkGetRayTracingCaptureReplayShaderGroupHandlesKHR = nullptr;
PFN_vkCmdTraceRaysIndirectKHR                         FExtensions::vkCmdTraceRaysIndirectKHR                         = nullptr;
PFN_vkGetRayTracingShaderGroupStackSizeKHR            FExtensions::vkGetRayTracingShaderGroupStackSizeKHR            = nullptr;
PFN_vkCmdSetRayTracingPipelineStackSizeKHR            FExtensions::vkCmdSetRayTracingPipelineStackSizeKHR            = nullptr;