#include "Extensions.h"

// SetDebugName
PFN_vkSetDebugUtilsObjectNameEXT    Extensions::vkSetDebugUtilsObjectNameEXT    = nullptr;
PFN_vkCreateDebugUtilsMessengerEXT  Extensions::vkCreateDebugUtilsMessengerEXT  = nullptr;
PFN_vkDestroyDebugUtilsMessengerEXT Extensions::vkDestroyDebugUtilsMessengerEXT = nullptr;

// RayTracing
PFN_vkCreateAccelerationStructureKHR                 Extensions::vkCreateAccelerationStructureKHR                 = nullptr;
PFN_vkDestroyAccelerationStructureKHR                Extensions::vkDestroyAccelerationStructureKHR                = nullptr;
PFN_vkCmdBuildAccelerationStructuresKHR              Extensions::vkCmdBuildAccelerationStructuresKHR              = nullptr;
PFN_vkCmdBuildAccelerationStructuresIndirectKHR      Extensions::vkCmdBuildAccelerationStructuresIndirectKHR      = nullptr;
PFN_vkBuildAccelerationStructuresKHR                 Extensions::vkBuildAccelerationStructuresKHR                 = nullptr;
PFN_vkCopyAccelerationStructureKHR                   Extensions::vkCopyAccelerationStructureKHR                   = nullptr;
PFN_vkCopyAccelerationStructureToMemoryKHR           Extensions::vkCopyAccelerationStructureToMemoryKHR           = nullptr;
PFN_vkCopyMemoryToAccelerationStructureKHR           Extensions::vkCopyMemoryToAccelerationStructureKHR           = nullptr;
PFN_vkWriteAccelerationStructuresPropertiesKHR       Extensions::vkWriteAccelerationStructuresPropertiesKHR       = nullptr;
PFN_vkCmdCopyAccelerationStructureKHR                Extensions::vkCmdCopyAccelerationStructureKHR                = nullptr;
PFN_vkCmdCopyAccelerationStructureToMemoryKHR        Extensions::vkCmdCopyAccelerationStructureToMemoryKHR        = nullptr;
PFN_vkCmdCopyMemoryToAccelerationStructureKHR        Extensions::vkCmdCopyMemoryToAccelerationStructureKHR        = nullptr;
PFN_vkGetAccelerationStructureDeviceAddressKHR       Extensions::vkGetAccelerationStructureDeviceAddressKHR       = nullptr;
PFN_vkCmdWriteAccelerationStructuresPropertiesKHR    Extensions::vkCmdWriteAccelerationStructuresPropertiesKHR    = nullptr;
PFN_vkGetDeviceAccelerationStructureCompatibilityKHR Extensions::vkGetDeviceAccelerationStructureCompatibilityKHR = nullptr;
PFN_vkGetAccelerationStructureBuildSizesKHR          Extensions::vkGetAccelerationStructureBuildSizesKHR          = nullptr;
PFN_vkGetRayTracingShaderGroupHandlesKHR             Extensions::vkGetRayTracingShaderGroupHandlesKHR             = nullptr;

PFN_vkCmdTraceRaysKHR                                 Extensions::vkCmdTraceRaysKHR                                 = nullptr;
PFN_vkCreateRayTracingPipelinesKHR                    Extensions::vkCreateRayTracingPipelinesKHR                    = nullptr;
PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR Extensions::vkGetRayTracingCaptureReplayShaderGroupHandlesKHR = nullptr;
PFN_vkCmdTraceRaysIndirectKHR                         Extensions::vkCmdTraceRaysIndirectKHR                         = nullptr;
PFN_vkGetRayTracingShaderGroupStackSizeKHR            Extensions::vkGetRayTracingShaderGroupStackSizeKHR            = nullptr;
PFN_vkCmdSetRayTracingPipelineStackSizeKHR            Extensions::vkCmdSetRayTracingPipelineStackSizeKHR            = nullptr;

PFN_vkCmdBeginRenderingKHR                            Extensions::vkCmdBeginRenderingKHR                            = nullptr;
PFN_vkCmdEndRenderingKHR                              Extensions::vkCmdEndRenderingKHR                              = nullptr;

PFN_vkGetDescriptorSetLayoutSizeEXT                   Extensions::vkGetDescriptorSetLayoutSizeEXT                   = nullptr;
PFN_vkGetDescriptorSetLayoutBindingOffsetEXT          Extensions::vkGetDescriptorSetLayoutBindingOffsetEXT          = nullptr;
PFN_vkGetDescriptorEXT                                Extensions::vkGetDescriptorEXT                                = nullptr;
PFN_vkCmdBindDescriptorBuffersEXT                     Extensions::vkCmdBindDescriptorBuffersEXT                     = nullptr;
PFN_vkCmdSetDescriptorBufferOffsetsEXT                Extensions::vkCmdSetDescriptorBufferOffsetsEXT                = nullptr;
PFN_vkCmdBindDescriptorBufferEmbeddedSamplersEXT      Extensions::vkCmdBindDescriptorBufferEmbeddedSamplersEXT      = nullptr;