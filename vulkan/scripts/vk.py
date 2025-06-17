import ctypes
import dataclasses
import enum
from typing import List

dataclass = dataclasses.dataclass
Enum = enum.Enum

# TODO(b/401184058): Automate this file for generating the vulkan structs graph from vk.xml
VK_UUID_SIZE = 16
VK_LUID_SIZE = 16

VkImageLayout = Enum
uint8_t = ctypes.c_uint8
uint32_t = ctypes.c_uint32
VkFlags = uint32_t
VkMemoryPropertyFlags = VkFlags
VkMemoryHeapFlags = VkFlags
int32_t = int
uint64_t = ctypes.c_uint64
VkBool32 = bool
VkDeviceSize = ctypes.c_uint64
size_t = int
VkSampleCountFlags = ctypes.c_uint32
VkFormatFeatureFlags = ctypes.c_uint32
VkQueueFlags = ctypes.c_uint32
VkShaderStageFlags = ctypes.c_uint32
VkSubgroupFeatureFlags = ctypes.c_uint32
VkResolveModeFlags = ctypes.c_uint32
float_t = ctypes.c_float
VkShaderFloatControlsIndependence = Enum
VkPointClippingBehavior = Enum
VkPhysicalDeviceType = Enum
VkDriverId = Enum
VkPipelineRobustnessBufferBehavior = Enum


@dataclass
class ConformanceVersion:
  major: uint8_t
  minor: uint8_t
  subminor: uint8_t
  patch: uint8_t


@dataclass
class VkExtent3D:
  width: uint32_t
  height: uint32_t
  depth: uint32_t


@dataclass
class VkPhysicalDeviceLimits:
  maxImageDimension1D: uint32_t
  maxImageDimension2D: uint32_t
  maxImageDimension3D: uint32_t
  maxImageDimensionCube: uint32_t
  maxImageArrayLayers: uint32_t
  maxTexelBufferElements: uint32_t
  maxUniformBufferRange: uint32_t
  maxStorageBufferRange: uint32_t
  maxPushConstantsSize: uint32_t
  maxMemoryAllocationCount: uint32_t
  maxSamplerAllocationCount: uint32_t
  bufferImageGranularity: VkDeviceSize
  sparseAddressSpaceSize: VkDeviceSize
  maxBoundDescriptorSets: uint32_t
  maxPerStageDescriptorSamplers: uint32_t
  maxPerStageDescriptorUniformBuffers: uint32_t
  maxPerStageDescriptorStorageBuffers: uint32_t
  maxPerStageDescriptorSampledImages: uint32_t
  maxPerStageDescriptorStorageImages: uint32_t
  maxPerStageDescriptorInputAttachments: uint32_t
  maxPerStageResources: uint32_t
  maxDescriptorSetSamplers: uint32_t
  maxDescriptorSetUniformBuffers: uint32_t
  maxDescriptorSetUniformBuffersDynamic: uint32_t
  maxDescriptorSetStorageBuffers: uint32_t
  maxDescriptorSetStorageBuffersDynamic: uint32_t
  maxDescriptorSetSampledImages: uint32_t
  maxDescriptorSetStorageImages: uint32_t
  maxDescriptorSetInputAttachments: uint32_t
  maxVertexInputAttributes: uint32_t
  maxVertexInputBindings: uint32_t
  maxVertexInputAttributeOffset: uint32_t
  maxVertexInputBindingStride: uint32_t
  maxVertexOutputComponents: uint32_t
  maxTessellationGenerationLevel: uint32_t
  maxTessellationPatchSize: uint32_t
  maxTessellationControlPerVertexInputComponents: uint32_t
  maxTessellationControlPerVertexOutputComponents: uint32_t
  maxTessellationControlPerPatchOutputComponents: uint32_t
  maxTessellationControlTotalOutputComponents: uint32_t
  maxTessellationEvaluationInputComponents: uint32_t
  maxTessellationEvaluationOutputComponents: uint32_t
  maxGeometryShaderInvocations: uint32_t
  maxGeometryInputComponents: uint32_t
  maxGeometryOutputComponents: uint32_t
  maxGeometryOutputVertices: uint32_t
  maxGeometryTotalOutputComponents: uint32_t
  maxFragmentInputComponents: uint32_t
  maxFragmentOutputAttachments: uint32_t
  maxFragmentDualSrcAttachments: uint32_t
  maxFragmentCombinedOutputResources: uint32_t
  maxComputeSharedMemorySize: uint32_t
  maxComputeWorkGroupCount: uint32_t*3
  maxComputeWorkGroupInvocations: uint32_t
  maxComputeWorkGroupSize: uint32_t*3
  subPixelPrecisionBits: uint32_t
  subTexelPrecisionBits: uint32_t
  mipmapPrecisionBits: uint32_t
  maxDrawIndexedIndexValue: uint32_t
  maxDrawIndirectCount: uint32_t
  maxSamplerLodBias: float
  maxSamplerAnisotropy: float
  maxViewports: uint32_t
  maxViewportDimensions: uint32_t*2
  viewportBoundsRange: float_t*2
  viewportSubPixelBits: uint32_t
  minMemoryMapAlignment: size_t
  minTexelBufferOffsetAlignment: VkDeviceSize
  minUniformBufferOffsetAlignment: VkDeviceSize
  minStorageBufferOffsetAlignment: VkDeviceSize
  minTexelOffset: int32_t
  maxTexelOffset: uint32_t
  minTexelGatherOffset: int32_t
  maxTexelGatherOffset: uint32_t
  minInterpolationOffset: float
  maxInterpolationOffset: float
  subPixelInterpolationOffsetBits: uint32_t
  maxFramebufferWidth: uint32_t
  maxFramebufferHeight: uint32_t
  maxFramebufferLayers: uint32_t
  framebufferColorSampleCounts: VkSampleCountFlags
  framebufferDepthSampleCounts: VkSampleCountFlags
  framebufferStencilSampleCounts: VkSampleCountFlags
  framebufferNoAttachmentsSampleCounts: VkSampleCountFlags
  maxColorAttachments: uint32_t
  sampledImageColorSampleCounts: VkSampleCountFlags
  sampledImageIntegerSampleCounts: VkSampleCountFlags
  sampledImageDepthSampleCounts: VkSampleCountFlags
  sampledImageStencilSampleCounts: VkSampleCountFlags
  storageImageSampleCounts: VkSampleCountFlags
  maxSampleMaskWords: uint32_t
  timestampComputeAndGraphics: VkBool32
  timestampPeriod: float
  maxClipDistances: uint32_t
  maxCullDistances: uint32_t
  maxCombinedClipAndCullDistances: uint32_t
  discreteQueuePriorities: uint32_t
  pointSizeRange: float_t*2
  lineWidthRange: float_t*2
  pointSizeGranularity: float
  lineWidthGranularity: float
  strictLines: VkBool32
  standardSampleLocations: VkBool32
  optimalBufferCopyOffsetAlignment: VkDeviceSize
  optimalBufferCopyRowPitchAlignment: VkDeviceSize
  nonCoherentAtomSize: VkDeviceSize


@dataclass
class VkPhysicalDeviceShaderDrawParameterFeatures:
  shaderDrawParameters: VkBool32


@dataclass
class VkExtensionProperties:
  extensionName: str
  specVersion: uint32_t


@dataclass
class VkFormatProperties:
  linearTilingFeatures: VkFormatFeatureFlags
  optimalTilingFeatures: VkFormatFeatureFlags
  bufferFeatures: VkFormatFeatureFlags


@dataclass
class VkLayerProperties:
  layerName: str
  specVersion: uint32_t
  implementationVersion: uint32_t
  description: str


@dataclass
class VkQueueFamilyProperties:
  queueFlags: VkQueueFlags
  queueCount: uint32_t
  timestampValidBits: uint32_t
  minImageTransferGranularity: VkExtent3D


@dataclass
class VkPhysicalDeviceSparseProperties:
  residencyStandard2DBlockShape: VkBool32
  residencyStandard2DMultisampleBlockShape: VkBool32
  residencyStandard3DBlockShape: VkBool32
  residencyAlignedMipSize: VkBool32
  residencyNonResidentStrict: VkBool32


@dataclass
class VkImageFormatProperties:
  maxExtent: VkExtent3D
  maxMipLevels: uint32_t
  maxArrayLayers: uint32_t
  sampleCounts: VkSampleCountFlags
  maxResourceSize: VkDeviceSize


@dataclass
class VkPhysicalDeviceSamplerYcbcrConversionFeatures:
  samplerYcbcrConversion: VkBool32


@dataclass
class VkPhysicalDeviceIDProperties:
  deviceUUID: uint8_t*VK_UUID_SIZE
  driverUUID: uint8_t*VK_UUID_SIZE
  deviceLUID: uint8_t*VK_LUID_SIZE
  deviceNodeMask: uint32_t
  deviceLUIDValid: VkBool32


@dataclass
class VkPhysicalDeviceMaintenance3Properties:
  maxPerSetDescriptors: uint32_t
  maxMemoryAllocationSize: VkDeviceSize


@dataclass
class VkPhysicalDevice16BitStorageFeatures:
  storageBuffer16BitAccess: VkBool32
  uniformAndStorageBuffer16BitAccess: VkBool32
  storagePushConstant16: VkBool32
  storageInputOutput16: VkBool32


@dataclass
class VkPhysicalDeviceMultiviewFeatures:
  multiview: VkBool32
  multiviewGeometryShader: VkBool32
  multiviewTessellationShader: VkBool32


@dataclass
class VkPhysicalDeviceSubgroupProperties:
  subgroupSize: uint32_t
  supportedStages: VkShaderStageFlags
  supportedOperations: VkSubgroupFeatureFlags
  quadOperationsInAllStages: VkBool32


@dataclass
class VkPhysicalDevicePointClippingProperties:
  pointClippingBehavior: VkPointClippingBehavior


@dataclass
class VkPhysicalDeviceMultiviewProperties:
  maxMultiviewViewCount: uint32_t
  maxMultiviewInstanceIndex: uint32_t


@dataclass
class VkMemoryType:
  propertyFlags: VkMemoryPropertyFlags
  heapIndex: uint32_t


@dataclass
class VkMemoryHeap:
  size: VkDeviceSize
  flags: VkMemoryHeapFlags


@dataclass
class VkPhysicalDeviceMemoryProperties:
  memoryTypeCount: uint32_t
  memoryTypes: List[VkMemoryType]
  memoryHeapCount: uint32_t
  memoryHeaps: List[VkMemoryHeap]


@dataclass
class VkPhysicalDeviceProperties:
  apiVersion: uint32_t
  driverVersion: uint32_t
  vendorID: uint32_t
  deviceID: uint32_t
  deviceType: VkPhysicalDeviceType
  deviceName: str
  pipelineCacheUUID: uint8_t
  limits: VkPhysicalDeviceLimits
  sparseProperties: VkPhysicalDeviceSparseProperties


@dataclass
class VkPhysicalDeviceFeatures:
  robustBufferAccess: VkBool32
  fullDrawIndexUint32: VkBool32
  imageCubeArray: VkBool32
  independentBlend: VkBool32
  geometryShader: VkBool32
  tessellationShader: VkBool32
  sampleRateShading: VkBool32
  dualSrcBlend: VkBool32
  logicOp: VkBool32
  multiDrawIndirect: VkBool32
  drawIndirectFirstInstance: VkBool32
  depthClamp: VkBool32
  depthBiasClamp: VkBool32
  fillModeNonSolid: VkBool32
  depthBounds: VkBool32
  wideLines: VkBool32
  largePoints: VkBool32
  alphaToOne: VkBool32
  multiViewport: VkBool32
  samplerAnisotropy: VkBool32
  textureCompressionETC2: VkBool32
  textureCompressionASTC_LDR: VkBool32
  textureCompressionBC: VkBool32
  occlusionQueryPrecise: VkBool32
  pipelineStatisticsQuery: VkBool32
  vertexPipelineStoresAndAtomics: VkBool32
  fragmentStoresAndAtomics: VkBool32
  shaderTessellationAndGeometryPointSize: VkBool32
  shaderImageGatherExtended: VkBool32
  shaderStorageImageExtendedFormats: VkBool32
  shaderStorageImageMultisample: VkBool32
  shaderStorageImageReadWithoutFormat: VkBool32
  shaderStorageImageWriteWithoutFormat: VkBool32
  shaderUniformBufferArrayDynamicIndexing: VkBool32
  shaderSampledImageArrayDynamicIndexing: VkBool32
  shaderStorageBufferArrayDynamicIndexing: VkBool32
  shaderStorageImageArrayDynamicIndexing: VkBool32
  shaderClipDistance: VkBool32
  shaderCullDistance: VkBool32
  shaderFloat64: VkBool32
  shaderInt64: VkBool32
  shaderInt16: VkBool32
  shaderResourceResidency: VkBool32
  shaderResourceMinLod: VkBool32
  sparseBinding: VkBool32
  sparseResidencyBuffer: VkBool32
  sparseResidencyImage2D: VkBool32
  sparseResidencyImage3D: VkBool32
  sparseResidency2Samples: VkBool32
  sparseResidency4Samples: VkBool32
  sparseResidency8Samples: VkBool32
  sparseResidency16Samples: VkBool32
  sparseResidencyAliased: VkBool32
  variableMultisampleRate: VkBool32
  inheritedQueries: VkBool32


@dataclass
class VkPhysicalDeviceShaderFloat16Int8Features:
  shaderFloat16: VkBool32
  shaderInt8: VkBool32


@dataclass
class VkPhysicalDeviceProtectedMemoryFeatures:
  protectedMemory: VkBool32


@dataclass
class VkPhysicalDeviceVariablePointersFeatures:
  variablePointersStorageBuffer: VkBool32
  variablePointers: VkBool32


@dataclass
class VkPhysicalDeviceImage2DViewOf3DFeaturesEXT:
  image2DViewOf3D: VkBool32
  sampler2DViewOf3D: VkBool32


@dataclass
class VkPhysicalDeviceCustomBorderColorFeaturesEXT:
  customBorderColors: VkBool32
  customBorderColorWithoutFormat: VkBool32


@dataclass
class VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT:
  primitiveTopologyListRestart: VkBool32
  primitiveTopologyPatchListRestart: VkBool32


@dataclass
class VkPhysicalDeviceProvokingVertexFeaturesEXT:
  provokingVertexLast: VkBool32
  transformFeedbackPreservesProvokingVertex: VkBool32


@dataclass
class VkPhysicalDeviceIndexTypeUint8Features:
  indexTypeUint8: VkBool32


@dataclass
class VkPhysicalDeviceVertexAttributeDivisorFeatures:
  vertexAttributeInstanceRateDivisor: VkBool32
  vertexAttributeInstanceRateZeroDivisor: VkBool32


@dataclass
class VkPhysicalDeviceTransformFeedbackFeaturesEXT:
  transformFeedback: VkBool32
  geometryStreams: VkBool32


@dataclass
class VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR:
  shaderSubgroupUniformControlFlow: VkBool32


@dataclass
class VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures:
  shaderSubgroupExtendedTypes: VkBool32


@dataclass
class VkPhysicalDevice8BitStorageFeatures:
  storageBuffer8BitAccess: VkBool32
  uniformAndStorageBuffer8BitAccess: VkBool32
  storagePushConstant8: VkBool32


@dataclass
class VkPhysicalDeviceShaderIntegerDotProductFeatures:
  shaderIntegerDotProduct: VkBool32


@dataclass
class VkPhysicalDeviceRelaxedLineRasterizationFeaturesIMG:
  relaxedLineRasterization: VkBool32


@dataclass
class VkPhysicalDeviceLineRasterizationFeatures:
  rectangularLines: VkBool32
  bresenhamLines: VkBool32
  smoothLines: VkBool32
  stippledRectangularLines: VkBool32
  stippledBresenhamLines: VkBool32
  stippledSmoothLines: VkBool32


@dataclass
class VkPhysicalDevicePrimitivesGeneratedQueryFeaturesEXT:
  primitivesGeneratedQuery: VkBool32
  primitivesGeneratedQueryWithRasterizerDiscard: VkBool32
  primitivesGeneratedQueryWithNonZeroStreams: VkBool32


@dataclass
class VkPhysicalDeviceFloatControlsProperties:
  denormBehaviorIndependence : VkShaderFloatControlsIndependence
  roundingModeIndependence : VkShaderFloatControlsIndependence
  shaderSignedZeroInfNanPreserveFloat16 : VkBool32
  shaderSignedZeroInfNanPreserveFloat32 : VkBool32
  shaderSignedZeroInfNanPreserveFloat64 : VkBool32
  shaderDenormPreserveFloat16 : VkBool32
  shaderDenormPreserveFloat32 : VkBool32
  shaderDenormPreserveFloat64 : VkBool32
  shaderDenormFlushToZeroFloat16 : VkBool32
  shaderDenormFlushToZeroFloat32 : VkBool32
  shaderDenormFlushToZeroFloat64 : VkBool32
  shaderRoundingModeRTEFloat16 : VkBool32
  shaderRoundingModeRTEFloat32 : VkBool32
  shaderRoundingModeRTEFloat64 :VkBool32
  shaderRoundingModeRTZFloat16 : VkBool32
  shaderRoundingModeRTZFloat32 : VkBool32
  shaderRoundingModeRTZFloat64 : VkBool32


@dataclass
class VkPhysicalDeviceVulkan11Properties:
  deviceUUID : uint8_t*VK_UUID_SIZE
  driverUUID : uint8_t*VK_UUID_SIZE
  deviceLUID : uint8_t*VK_LUID_SIZE
  deviceNodeMask : uint32_t
  deviceLUIDValid : VkBool32
  subgroupSize : uint32_t
  subgroupSupportedStages : VkShaderStageFlags
  subgroupSupportedOperations : VkSubgroupFeatureFlags
  subgroupQuadOperationsInAllStages : VkBool32
  pointClippingBehavior : VkPointClippingBehavior
  maxMultiviewViewCount : uint32_t
  maxMultiviewInstanceIndex :uint32_t
  protectedNoFault : VkBool32
  maxPerSetDescriptors : uint32_t
  maxMemoryAllocationSize : VkDeviceSize


@dataclass
class VkPhysicalDeviceVulkan11Features:
  storageBuffer16BitAccess: VkBool32
  uniformAndStorageBuffer16BitAccess: VkBool32
  storagePushConstant16: VkBool32
  storageInputOutput16: VkBool32
  multiview: VkBool32
  multiviewGeometryShader: VkBool32
  multiviewTessellationShader: VkBool32
  variablePointersStorageBuffer: VkBool32
  variablePointers: VkBool32
  protectedMemory: VkBool32
  samplerYcbcrConversion: VkBool32
  shaderDrawParameters: VkBool32


@dataclass
class VkPhysicalDeviceVulkan12Properties:
  driverID: VkDriverId
  driverName: str
  driverInfo: str
  conformanceVersion: ConformanceVersion
  denormBehaviorIndependence: VkShaderFloatControlsIndependence
  roundingModeIndependence: VkShaderFloatControlsIndependence
  shaderSignedZeroInfNanPreserveFloat16: VkBool32
  shaderSignedZeroInfNanPreserveFloat32: VkBool32
  shaderSignedZeroInfNanPreserveFloat64: VkBool32
  shaderDenormPreserveFloat16: VkBool32
  shaderDenormPreserveFloat32: VkBool32
  shaderDenormPreserveFloat64: VkBool32
  shaderDenormFlushToZeroFloat16: VkBool32
  shaderDenormFlushToZeroFloat32: VkBool32
  shaderDenormFlushToZeroFloat64: VkBool32
  shaderRoundingModeRTEFloat16: VkBool32
  shaderRoundingModeRTEFloat32: VkBool32
  shaderRoundingModeRTEFloat64: VkBool32
  shaderRoundingModeRTZFloat16: VkBool32
  shaderRoundingModeRTZFloat32: VkBool32
  shaderRoundingModeRTZFloat64: VkBool32
  maxUpdateAfterBindDescriptorsInAllPools: uint32_t
  shaderUniformBufferArrayNonUniformIndexingNative: VkBool32
  shaderSampledImageArrayNonUniformIndexingNative: VkBool32
  shaderStorageBufferArrayNonUniformIndexingNative: VkBool32
  shaderStorageImageArrayNonUniformIndexingNative: VkBool32
  shaderInputAttachmentArrayNonUniformIndexingNative: VkBool32
  robustBufferAccessUpdateAfterBind: VkBool32
  quadDivergentImplicitLod: VkBool32
  maxPerStageDescriptorUpdateAfterBindSamplers: uint32_t
  maxPerStageDescriptorUpdateAfterBindUniformBuffers: uint32_t
  maxPerStageDescriptorUpdateAfterBindStorageBuffers: uint32_t
  maxPerStageDescriptorUpdateAfterBindSampledImages: uint32_t
  maxPerStageDescriptorUpdateAfterBindStorageImages: uint32_t
  maxPerStageDescriptorUpdateAfterBindInputAttachments: uint32_t
  maxPerStageUpdateAfterBindResources: uint32_t
  maxDescriptorSetUpdateAfterBindSamplers: uint32_t
  maxDescriptorSetUpdateAfterBindUniformBuffers: uint32_t
  maxDescriptorSetUpdateAfterBindUniformBuffersDynamic: uint32_t
  maxDescriptorSetUpdateAfterBindStorageBuffers: uint32_t
  maxDescriptorSetUpdateAfterBindStorageBuffersDynamic: uint32_t
  maxDescriptorSetUpdateAfterBindSampledImages: uint32_t
  maxDescriptorSetUpdateAfterBindStorageImages: uint32_t
  maxDescriptorSetUpdateAfterBindInputAttachments: uint32_t
  supportedDepthResolveModes: VkResolveModeFlags
  supportedStencilResolveModes: VkResolveModeFlags
  independentResolveNone: VkBool32
  independentResolve: VkBool32
  filterMinmaxSingleComponentFormats: VkBool32
  filterMinmaxImageComponentMapping: VkBool32
  maxTimelineSemaphoreValueDifference: uint64_t
  framebufferIntegerColorSampleCounts: VkSampleCountFlags


@dataclass
class VkPhysicalDeviceVulkan12Features:
  samplerMirrorClampToEdge: VkBool32
  drawIndirectCount: VkBool32
  storageBuffer8BitAccess: VkBool32
  uniformAndStorageBuffer8BitAccess: VkBool32
  storagePushConstant8: VkBool32
  shaderBufferInt64Atomics: VkBool32
  shaderSharedInt64Atomics: VkBool32
  shaderFloat16: VkBool32
  shaderInt8: VkBool32
  descriptorIndexing: VkBool32
  shaderInputAttachmentArrayDynamicIndexing: VkBool32
  shaderUniformTexelBufferArrayDynamicIndexing: VkBool32
  shaderStorageTexelBufferArrayDynamicIndexing: VkBool32
  shaderUniformBufferArrayNonUniformIndexing: VkBool32
  shaderSampledImageArrayNonUniformIndexing: VkBool32
  shaderStorageBufferArrayNonUniformIndexing: VkBool32
  shaderStorageImageArrayNonUniformIndexing: VkBool32
  shaderInputAttachmentArrayNonUniformIndexing: VkBool32
  shaderUniformTexelBufferArrayNonUniformIndexing: VkBool32
  shaderStorageTexelBufferArrayNonUniformIndexing: VkBool32
  descriptorBindingUniformBufferUpdateAfterBind: VkBool32
  descriptorBindingSampledImageUpdateAfterBind: VkBool32
  descriptorBindingStorageImageUpdateAfterBind: VkBool32
  descriptorBindingStorageBufferUpdateAfterBind: VkBool32
  descriptorBindingUniformTexelBufferUpdateAfterBind: VkBool32
  descriptorBindingStorageTexelBufferUpdateAfterBind: VkBool32
  descriptorBindingUpdateUnusedWhilePending: VkBool32
  descriptorBindingPartiallyBound: VkBool32
  descriptorBindingVariableDescriptorCount: VkBool32
  runtimeDescriptorArray: VkBool32
  samplerFilterMinmax: VkBool32
  scalarBlockLayout: VkBool32
  imagelessFramebuffer: VkBool32
  uniformBufferStandardLayout: VkBool32
  shaderSubgroupExtendedTypes: VkBool32
  separateDepthStencilLayouts: VkBool32
  hostQueryReset: VkBool32
  timelineSemaphore: VkBool32
  bufferDeviceAddress: VkBool32
  bufferDeviceAddressCaptureReplay: VkBool32
  bufferDeviceAddressMultiDevice: VkBool32
  vulkanMemoryModel: VkBool32
  vulkanMemoryModelDeviceScope: VkBool32
  vulkanMemoryModelAvailabilityVisibilityChains: VkBool32
  shaderOutputViewportIndex: VkBool32
  shaderOutputLayer: VkBool32
  subgroupBroadcastDynamicId: VkBool32


@dataclass
class VkPhysicalDeviceVulkan13Properties:
  minSubgroupSize: uint32_t
  maxSubgroupSize: uint32_t
  maxComputeWorkgroupSubgroups: uint32_t
  requiredSubgroupSizeStages: VkShaderStageFlags
  maxInlineUniformBlockSize: uint32_t
  maxPerStageDescriptorInlineUniformBlocks: uint32_t
  maxPerStageDescriptorUpdateAfterBindInlineUniformBlocks: uint32_t
  maxDescriptorSetInlineUniformBlocks: uint32_t
  maxDescriptorSetUpdateAfterBindInlineUniformBlocks: uint32_t
  maxInlineUniformTotalSize: uint32_t
  integerDotProduct8BitUnsignedAccelerated: VkBool32
  integerDotProduct8BitSignedAccelerated: VkBool32
  integerDotProduct8BitMixedSignednessAccelerated: VkBool32
  integerDotProduct4x8BitPackedUnsignedAccelerated: VkBool32
  integerDotProduct4x8BitPackedSignedAccelerated: VkBool32
  integerDotProduct4x8BitPackedMixedSignednessAccelerated: VkBool32
  integerDotProduct16BitUnsignedAccelerated: VkBool32
  integerDotProduct16BitSignedAccelerated: VkBool32
  integerDotProduct16BitMixedSignednessAccelerated: VkBool32
  integerDotProduct32BitUnsignedAccelerated: VkBool32
  integerDotProduct32BitSignedAccelerated: VkBool32
  integerDotProduct32BitMixedSignednessAccelerated: VkBool32
  integerDotProduct64BitUnsignedAccelerated: VkBool32
  integerDotProduct64BitSignedAccelerated: VkBool32
  integerDotProduct64BitMixedSignednessAccelerated: VkBool32
  integerDotProductAccumulatingSaturating8BitUnsignedAccelerated: VkBool32
  integerDotProductAccumulatingSaturating8BitSignedAccelerated: VkBool32
  integerDotProductAccumulatingSaturating8BitMixedSignednessAccelerated: VkBool32
  integerDotProductAccumulatingSaturating4x8BitPackedUnsignedAccelerated: VkBool32
  integerDotProductAccumulatingSaturating4x8BitPackedSignedAccelerated: VkBool32
  integerDotProductAccumulatingSaturating4x8BitPackedMixedSignednessAccelerated: VkBool32
  integerDotProductAccumulatingSaturating16BitUnsignedAccelerated: VkBool32
  integerDotProductAccumulatingSaturating16BitSignedAccelerated: VkBool32
  integerDotProductAccumulatingSaturating16BitMixedSignednessAccelerated: VkBool32
  integerDotProductAccumulatingSaturating32BitUnsignedAccelerated: VkBool32
  integerDotProductAccumulatingSaturating32BitSignedAccelerated: VkBool32
  integerDotProductAccumulatingSaturating32BitMixedSignednessAccelerated: VkBool32
  integerDotProductAccumulatingSaturating64BitUnsignedAccelerated: VkBool32
  integerDotProductAccumulatingSaturating64BitSignedAccelerated: VkBool32
  integerDotProductAccumulatingSaturating64BitMixedSignednessAccelerated: VkBool32
  storageTexelBufferOffsetAlignmentBytes: VkDeviceSize
  storageTexelBufferOffsetSingleTexelAlignment: VkBool32
  uniformTexelBufferOffsetAlignmentBytes: VkDeviceSize
  uniformTexelBufferOffsetSingleTexelAlignment: VkBool32
  maxBufferSize: VkDeviceSize


@dataclass
class VkPhysicalDeviceVulkan13Features:
  robustImageAccess: VkBool32
  inlineUniformBlock: VkBool32
  descriptorBindingInlineUniformBlockUpdateAfterBind: VkBool32
  pipelineCreationCacheControl: VkBool32
  privateData: VkBool32
  shaderDemoteToHelperInvocation: VkBool32
  shaderTerminateInvocation: VkBool32
  subgroupSizeControl: VkBool32
  computeFullSubgroups: VkBool32
  synchronization2: VkBool32
  textureCompressionASTC_HDR: VkBool32
  shaderZeroInitializeWorkgroupMemory: VkBool32
  dynamicRendering: VkBool32
  shaderIntegerDotProduct: VkBool32
  maintenance4: VkBool32


@dataclass
class VkPhysicalDeviceVulkan14Properties:
  lineSubPixelPrecisionBits: uint32_t
  maxVertexAttribDivisor: uint32_t
  supportsNonZeroFirstInstance: VkBool32
  maxPushDescriptors: uint32_t
  dynamicRenderingLocalReadDepthStencilAttachments: VkBool32
  dynamicRenderingLocalReadMultisampledAttachments: VkBool32
  earlyFragmentMultisampleCoverageAfterSampleCounting: VkBool32
  earlyFragmentSampleMaskTestBeforeSampleCounting: VkBool32
  depthStencilSwizzleOneSupport: VkBool32
  polygonModePointSize: VkBool32
  nonStrictSinglePixelWideLinesUseParallelogram: VkBool32
  nonStrictWideLinesUseParallelogram: VkBool32
  blockTexelViewCompatibleMultipleLayers: VkBool32
  maxCombinedImageSamplerDescriptorCount: uint32_t
  fragmentShadingRateClampCombinerInputs: VkBool32
  defaultRobustnessStorageBuffers: VkPipelineRobustnessBufferBehavior
  defaultRobustnessUniformBuffers: VkPipelineRobustnessBufferBehavior
  defaultRobustnessVertexInputs: VkPipelineRobustnessBufferBehavior
  defaultRobustnessImages: VkPipelineRobustnessBufferBehavior
  copySrcLayoutCount: uint32_t
  pCopySrcLayouts: List[VkImageLayout]
  copyDstLayoutCount: uint32_t
  pCopyDstLayouts: List[VkImageLayout]
  optimalTilingLayoutUUID: uint8_t
  identicalMemoryTypeRequirements: VkBool32


@dataclass
class VkPhysicalDeviceVulkan14Features:
  globalPriorityQuery: VkBool32
  shaderSubgroupRotate: VkBool32
  shaderSubgroupRotateClustered: VkBool32
  shaderFloatControls2: VkBool32
  shaderExpectAssume: VkBool32
  rectangularLines: VkBool32
  bresenhamLines: VkBool32
  smoothLines: VkBool32
  stippledRectangularLines: VkBool32
  stippledBresenhamLines: VkBool32
  stippledSmoothLines: VkBool32
  vertexAttributeInstanceRateDivisor: VkBool32
  vertexAttributeInstanceRateZeroDivisor: VkBool32
  indexTypeUint8: VkBool32
  dynamicRenderingLocalRead: VkBool32
  maintenance5: VkBool32
  maintenance6: VkBool32
  pipelineProtectedAccess: VkBool32
  pipelineRobustness: VkBool32
  hostImageCopy: VkBool32
  pushDescriptor: VkBool32


@dataclass
class VkPhysicalDeviceDriverProperties:
  driverID: VkDriverId
  driverName: str
  driverInfo: str
  conformanceVersion: ConformanceVersion

# Defining alias for structures
VkPhysicalDeviceLineRasterizationFeaturesEXT = VkPhysicalDeviceLineRasterizationFeatures
VkPhysicalDeviceLineRasterizationFeaturesKHR = VkPhysicalDeviceLineRasterizationFeatures
VkPhysicalDeviceShaderIntegerDotProductFeaturesKHR = VkPhysicalDeviceShaderIntegerDotProductFeatures
VkPhysicalDevice8BitStorageFeaturesKHR = VkPhysicalDevice8BitStorageFeatures
VkPhysicalDeviceShaderSubgroupExtendedTypesFeaturesKHR = VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures
VkPhysicalDeviceVertexAttributeDivisorFeaturesKHR = VkPhysicalDeviceVertexAttributeDivisorFeatures
VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT = VkPhysicalDeviceVertexAttributeDivisorFeatures
VkPhysicalDeviceIndexTypeUint8FeaturesKHR = VkPhysicalDeviceIndexTypeUint8Features
VkPhysicalDeviceIndexTypeUint8FeaturesEXT = VkPhysicalDeviceIndexTypeUint8Features
VkPhysicalDeviceVariablePointerFeatures = VkPhysicalDeviceVariablePointersFeatures
VkPhysicalDeviceVariablePointersFeaturesKHR = VkPhysicalDeviceVariablePointersFeatures
VkPhysicalDeviceVariablePointerFeaturesKHR = VkPhysicalDeviceVariablePointersFeatures
VkPhysicalDeviceFloat16Int8FeaturesKHR = VkPhysicalDeviceShaderFloat16Int8Features
VkPhysicalDeviceShaderFloat16Int8FeaturesKHR = VkPhysicalDeviceShaderFloat16Int8Features
VkPhysicalDeviceFloatControlsPropertiesKHR = VkPhysicalDeviceFloatControlsProperties
VkPhysicalDeviceShaderDrawParametersFeatures = VkPhysicalDeviceShaderDrawParameterFeatures
VkPhysicalDeviceDriverPropertiesKHR = VkPhysicalDeviceDriverProperties

# Defining dependency of structures on extensions
VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING = {
  "extensions": {
    "VK_KHR_variable_pointers": [
      { "VkPhysicalDeviceVariablePointerFeaturesKHR": "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTER_FEATURES" },
      { "VkPhysicalDeviceVariablePointersFeaturesKHR" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTER_FEATURES"},
    ],
    "VK_KHR_shader_float16_int8": [
      { "VkPhysicalDeviceShaderFloat16Int8FeaturesKHR": "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES" },
      {"VkPhysicalDeviceFloat16Int8FeaturesKHR" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES"},
    ],
    "VK_EXT_image_2d_view_of_3d" : [
      {"VkPhysicalDeviceImage2DViewOf3DFeaturesEXT" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_2D_VIEW_OF_3D_FEATURES_EXT"},
    ],
    "VK_EXT_custom_border_color" : [
      {"VkPhysicalDeviceCustomBorderColorFeaturesEXT" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_PROPERTIES_EXT"},
    ],
    "VK_EXT_primitive_topology_list_restart": [
      {"VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVE_TOPOLOGY_LIST_RESTART_FEATURES_EXT"},
    ],
    "VK_EXT_provoking_vertex" : [
      {"VkPhysicalDeviceProvokingVertexFeaturesEXT" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROVOKING_VERTEX_FEATURES_EXT"},
    ],
    "VK_KHR_index_type_uint8" : [
      {"VkPhysicalDeviceIndexTypeUint8FeaturesKHR" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES"},
    ],
    "VK_EXT_index_type_uint8"  : [
      {"VkPhysicalDeviceIndexTypeUint8FeaturesEXT" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES"},
    ],
    "VK_KHR_vertex_attribute_divisor" : [
      {"VkPhysicalDeviceVertexAttributeDivisorFeaturesKHR" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES"},
    ],
    "VK_EXT_vertex_attribute_divisor" : [
      {"VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES"},
    ],
    "VK_EXT_transform_feedback" : [
      {"VkPhysicalDeviceTransformFeedbackFeaturesEXT" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT"},
    ],
    "VK_KHR_shader_subgroup_uniform_control_flow" : [
      {"VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_UNIFORM_CONTROL_FLOW_FEATURES_KHR"},
    ],
    "VK_KHR_shader_subgroup_extended_types" : [
      {"VkPhysicalDeviceShaderSubgroupExtendedTypesFeaturesKHR" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES"},
    ],
    "VK_KHR_8bit_storage" : [
      {"VkPhysicalDevice8BitStorageFeaturesKHR" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES"},
    ],
    "VK_KHR_shader_integer_dot_product" : [
      {"VkPhysicalDeviceShaderIntegerDotProductFeaturesKHR" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_FEATURES"},
    ],
    "VK_IMG_relaxed_line_rasterization" : [
      {"VkPhysicalDeviceRelaxedLineRasterizationFeaturesIMG" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RELAXED_LINE_RASTERIZATION_FEATURES_IMG"},
    ],
    "VK_KHR_line_rasterization" : [
      {"VkPhysicalDeviceLineRasterizationFeaturesKHR" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES"},
    ],
    "VK_EXT_line_rasterization" : [
      {"VkPhysicalDeviceLineRasterizationFeaturesEXT" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES"},
    ],
    "VK_EXT_primitives_generated_query" : [
      {"VkPhysicalDevicePrimitivesGeneratedQueryFeaturesEXT" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVES_GENERATED_QUERY_FEATURES_EXT"},
    ],
    "VK_KHR_shader_float_controls" : [
      {"VkPhysicalDeviceFloatControlsPropertiesKHR" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES"},
    ],
    "VK_KHR_driver_properties" : [
      {"VkPhysicalDeviceDriverPropertiesKHR" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES"},
    ]
  }
}

# Defining dependency of structures on vulkan cores
VULKAN_CORES_AND_STRUCTS_MAPPING = {
  "versions" : {
    "Core11" : [
      {"VkPhysicalDeviceVulkan11Properties" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES"},
      {"VkPhysicalDeviceVulkan11Features" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES"},
    ],
    "Core12" : [
      {"VkPhysicalDeviceVulkan12Properties" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES"},
      {"VkPhysicalDeviceVulkan12Features" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES"},
    ],
    "Core13" : [
      {"VkPhysicalDeviceVulkan13Properties" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES"},
      {"VkPhysicalDeviceVulkan13Features" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES"},
    ],
    "Core14" : [
      {"VkPhysicalDeviceVulkan14Properties" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES"},
      {"VkPhysicalDeviceVulkan14Features" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES"},
    ]
  }
}

# Defining map for list type members mapped to its size
LIST_TYPE_FIELD_AND_SIZE_MAPPING = {
  "pCopySrcLayouts": "copySrcLayoutCount",
  "pCopyDstLayouts": "copyDstLayoutCount",
  "memoryTypes": "memoryTypeCount",
  "memoryHeaps": "memoryHeapCount",
}

# Defining dependency of structures on vulkan api version
VULKAN_VERSIONS_AND_STRUCTS_MAPPING = {
  "VK_VERSION_1_0" : [
    {"VkPhysicalDeviceProperties" : "" },
    {"VkPhysicalDeviceFeatures" : ""},
    {"VkPhysicalDeviceMemoryProperties" : ""},
  ],
  "VK_VERSION_1_1" : [
    {"VkPhysicalDeviceSubgroupProperties" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES"},
    {"VkPhysicalDevicePointClippingProperties" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_POINT_CLIPPING_PROPERTIES"},
    {"VkPhysicalDeviceMultiviewProperties" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES"},
    {"VkPhysicalDeviceIDProperties" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES"},
    {"VkPhysicalDeviceMaintenance3Properties" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES"},
    {"VkPhysicalDeviceMultiviewFeatures" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES"},
    {"VkPhysicalDeviceVariablePointersFeatures" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES"},
    {"VkPhysicalDeviceProtectedMemoryFeatures" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES"},
    {"VkPhysicalDeviceSamplerYcbcrConversionFeatures" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES"},
    {"VkPhysicalDeviceShaderDrawParameterFeatures" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES"},
    {"VkPhysicalDevice16BitStorageFeatures" : "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES"},
  ]
}

# List of structures that are not dependent on extensions
EXTENSION_INDEPENDENT_STRUCTS = [
  VkPhysicalDeviceProperties,
  VkPhysicalDeviceFeatures,
  VkPhysicalDeviceMemoryProperties,
  VkPhysicalDeviceSubgroupProperties,
  VkPhysicalDevicePointClippingProperties,
  VkPhysicalDeviceMultiviewProperties,
  VkPhysicalDeviceIDProperties,
  VkPhysicalDeviceMaintenance3Properties,
  VkPhysicalDevice16BitStorageFeatures,
  VkPhysicalDeviceMultiviewFeatures,
  VkPhysicalDeviceVariablePointersFeatures,
  VkPhysicalDeviceProtectedMemoryFeatures,
  VkPhysicalDeviceSamplerYcbcrConversionFeatures,
  VkPhysicalDeviceShaderDrawParameterFeatures,
]

# List of all the structures for vkjson
ALL_STRUCTS = [
  VkPhysicalDeviceFloatControlsPropertiesKHR,
  VkPhysicalDeviceProperties,
  VkPhysicalDeviceMemoryProperties,
  VkPhysicalDeviceSubgroupProperties,
  VkPhysicalDevicePointClippingProperties,
  VkPhysicalDeviceMultiviewProperties,
  VkPhysicalDeviceIDProperties,
  VkPhysicalDeviceMaintenance3Properties,
  VkPhysicalDeviceSparseProperties,
  VkImageFormatProperties,
  VkQueueFamilyProperties,
  VkExtensionProperties,
  VkLayerProperties,
  VkFormatProperties,
  VkPhysicalDeviceVariablePointerFeaturesKHR,
  VkPhysicalDeviceVariablePointersFeaturesKHR,
  VkPhysicalDeviceShaderFloat16Int8FeaturesKHR,
  VkPhysicalDeviceFloat16Int8FeaturesKHR,
  VkPhysicalDeviceImage2DViewOf3DFeaturesEXT,
  VkPhysicalDeviceCustomBorderColorFeaturesEXT,
  VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT,
  VkPhysicalDeviceProvokingVertexFeaturesEXT,
  VkPhysicalDeviceIndexTypeUint8FeaturesKHR,
  VkPhysicalDeviceIndexTypeUint8FeaturesEXT,
  VkPhysicalDeviceVertexAttributeDivisorFeaturesKHR,
  VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT,
  VkPhysicalDeviceTransformFeedbackFeaturesEXT,
  VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR,
  VkPhysicalDeviceShaderSubgroupExtendedTypesFeaturesKHR,
  VkPhysicalDevice8BitStorageFeaturesKHR,
  VkPhysicalDeviceShaderIntegerDotProductFeaturesKHR,
  VkPhysicalDeviceRelaxedLineRasterizationFeaturesIMG,
  VkPhysicalDeviceLineRasterizationFeaturesKHR,
  VkPhysicalDeviceLineRasterizationFeaturesEXT,
  VkPhysicalDevicePrimitivesGeneratedQueryFeaturesEXT,
  VkPhysicalDevice16BitStorageFeatures,
  VkPhysicalDeviceMultiviewFeatures,
  VkPhysicalDeviceProtectedMemoryFeatures,
  VkPhysicalDeviceSamplerYcbcrConversionFeatures,
  VkPhysicalDeviceShaderDrawParameterFeatures,
  VkPhysicalDeviceLimits,
  VkPhysicalDeviceFeatures,
  VkPhysicalDeviceVulkan11Properties,
  VkPhysicalDeviceVulkan11Features,
  VkPhysicalDeviceVulkan12Properties,
  VkPhysicalDeviceVulkan12Features,
  VkPhysicalDeviceVulkan13Properties,
  VkPhysicalDeviceVulkan13Features,
  VkPhysicalDeviceVulkan14Properties,
  VkPhysicalDeviceVulkan14Features,
  VkPhysicalDeviceDriverProperties,
]
