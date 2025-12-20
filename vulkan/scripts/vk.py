# Copyright 2025 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from dataclasses import dataclass
from enum import Enum
import dataclasses
dataclass = dataclasses.dataclass
from typing import List
import ctypes

# --- Adding Pre-Defined Constants ---
uint8_t = ctypes.c_uint8
uint32_t = ctypes.c_uint32
VkFlags = uint32_t
int32_t = int
uint64_t = ctypes.c_uint64
VkBool32 = bool
VkDeviceSize = ctypes.c_uint64
size_t = ctypes.c_uint64
float_t = ctypes.c_float
int64_t = ctypes.c_int64
uint16_t = ctypes.c_uint16
VkFlags64 = uint64_t

# --- Enum Definitions ---
class VkImageLayout(Enum):
    VK_IMAGE_LAYOUT_UNDEFINED = 0
    VK_IMAGE_LAYOUT_GENERAL = 1
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL = 2
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL = 3
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL = 4
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL = 5
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL = 6
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL = 7
    VK_IMAGE_LAYOUT_PREINITIALIZED = 8

class VkImageType(Enum):
    VK_IMAGE_TYPE_1D = 0
    VK_IMAGE_TYPE_2D = 1
    VK_IMAGE_TYPE_3D = 2

class VkImageTiling(Enum):
    VK_IMAGE_TILING_OPTIMAL = 0
    VK_IMAGE_TILING_LINEAR = 1

class VkPhysicalDeviceType(Enum):
    VK_PHYSICAL_DEVICE_TYPE_OTHER = 0
    VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU = 1
    VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU = 2
    VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU = 3
    VK_PHYSICAL_DEVICE_TYPE_CPU = 4

class VkFormat(Enum):
    VK_FORMAT_UNDEFINED = 0
    VK_FORMAT_R4G4_UNORM_PACK8 = 1
    VK_FORMAT_R4G4B4A4_UNORM_PACK16 = 2
    VK_FORMAT_B4G4R4A4_UNORM_PACK16 = 3
    VK_FORMAT_R5G6B5_UNORM_PACK16 = 4
    VK_FORMAT_B5G6R5_UNORM_PACK16 = 5
    VK_FORMAT_R5G5B5A1_UNORM_PACK16 = 6
    VK_FORMAT_B5G5R5A1_UNORM_PACK16 = 7
    VK_FORMAT_A1R5G5B5_UNORM_PACK16 = 8
    VK_FORMAT_R8_UNORM = 9
    VK_FORMAT_R8_SNORM = 10
    VK_FORMAT_R8_USCALED = 11
    VK_FORMAT_R8_SSCALED = 12
    VK_FORMAT_R8_UINT = 13
    VK_FORMAT_R8_SINT = 14
    VK_FORMAT_R8_SRGB = 15
    VK_FORMAT_R8G8_UNORM = 16
    VK_FORMAT_R8G8_SNORM = 17
    VK_FORMAT_R8G8_USCALED = 18
    VK_FORMAT_R8G8_SSCALED = 19
    VK_FORMAT_R8G8_UINT = 20
    VK_FORMAT_R8G8_SINT = 21
    VK_FORMAT_R8G8_SRGB = 22
    VK_FORMAT_R8G8B8_UNORM = 23
    VK_FORMAT_R8G8B8_SNORM = 24
    VK_FORMAT_R8G8B8_USCALED = 25
    VK_FORMAT_R8G8B8_SSCALED = 26
    VK_FORMAT_R8G8B8_UINT = 27
    VK_FORMAT_R8G8B8_SINT = 28
    VK_FORMAT_R8G8B8_SRGB = 29
    VK_FORMAT_B8G8R8_UNORM = 30
    VK_FORMAT_B8G8R8_SNORM = 31
    VK_FORMAT_B8G8R8_USCALED = 32
    VK_FORMAT_B8G8R8_SSCALED = 33
    VK_FORMAT_B8G8R8_UINT = 34
    VK_FORMAT_B8G8R8_SINT = 35
    VK_FORMAT_B8G8R8_SRGB = 36
    VK_FORMAT_R8G8B8A8_UNORM = 37
    VK_FORMAT_R8G8B8A8_SNORM = 38
    VK_FORMAT_R8G8B8A8_USCALED = 39
    VK_FORMAT_R8G8B8A8_SSCALED = 40
    VK_FORMAT_R8G8B8A8_UINT = 41
    VK_FORMAT_R8G8B8A8_SINT = 42
    VK_FORMAT_R8G8B8A8_SRGB = 43
    VK_FORMAT_B8G8R8A8_UNORM = 44
    VK_FORMAT_B8G8R8A8_SNORM = 45
    VK_FORMAT_B8G8R8A8_USCALED = 46
    VK_FORMAT_B8G8R8A8_SSCALED = 47
    VK_FORMAT_B8G8R8A8_UINT = 48
    VK_FORMAT_B8G8R8A8_SINT = 49
    VK_FORMAT_B8G8R8A8_SRGB = 50
    VK_FORMAT_A8B8G8R8_UNORM_PACK32 = 51
    VK_FORMAT_A8B8G8R8_SNORM_PACK32 = 52
    VK_FORMAT_A8B8G8R8_USCALED_PACK32 = 53
    VK_FORMAT_A8B8G8R8_SSCALED_PACK32 = 54
    VK_FORMAT_A8B8G8R8_UINT_PACK32 = 55
    VK_FORMAT_A8B8G8R8_SINT_PACK32 = 56
    VK_FORMAT_A8B8G8R8_SRGB_PACK32 = 57
    VK_FORMAT_A2R10G10B10_UNORM_PACK32 = 58
    VK_FORMAT_A2R10G10B10_SNORM_PACK32 = 59
    VK_FORMAT_A2R10G10B10_USCALED_PACK32 = 60
    VK_FORMAT_A2R10G10B10_SSCALED_PACK32 = 61
    VK_FORMAT_A2R10G10B10_UINT_PACK32 = 62
    VK_FORMAT_A2R10G10B10_SINT_PACK32 = 63
    VK_FORMAT_A2B10G10R10_UNORM_PACK32 = 64
    VK_FORMAT_A2B10G10R10_SNORM_PACK32 = 65
    VK_FORMAT_A2B10G10R10_USCALED_PACK32 = 66
    VK_FORMAT_A2B10G10R10_SSCALED_PACK32 = 67
    VK_FORMAT_A2B10G10R10_UINT_PACK32 = 68
    VK_FORMAT_A2B10G10R10_SINT_PACK32 = 69
    VK_FORMAT_R16_UNORM = 70
    VK_FORMAT_R16_SNORM = 71
    VK_FORMAT_R16_USCALED = 72
    VK_FORMAT_R16_SSCALED = 73
    VK_FORMAT_R16_UINT = 74
    VK_FORMAT_R16_SINT = 75
    VK_FORMAT_R16_SFLOAT = 76
    VK_FORMAT_R16G16_UNORM = 77
    VK_FORMAT_R16G16_SNORM = 78
    VK_FORMAT_R16G16_USCALED = 79
    VK_FORMAT_R16G16_SSCALED = 80
    VK_FORMAT_R16G16_UINT = 81
    VK_FORMAT_R16G16_SINT = 82
    VK_FORMAT_R16G16_SFLOAT = 83
    VK_FORMAT_R16G16B16_UNORM = 84
    VK_FORMAT_R16G16B16_SNORM = 85
    VK_FORMAT_R16G16B16_USCALED = 86
    VK_FORMAT_R16G16B16_SSCALED = 87
    VK_FORMAT_R16G16B16_UINT = 88
    VK_FORMAT_R16G16B16_SINT = 89
    VK_FORMAT_R16G16B16_SFLOAT = 90
    VK_FORMAT_R16G16B16A16_UNORM = 91
    VK_FORMAT_R16G16B16A16_SNORM = 92
    VK_FORMAT_R16G16B16A16_USCALED = 93
    VK_FORMAT_R16G16B16A16_SSCALED = 94
    VK_FORMAT_R16G16B16A16_UINT = 95
    VK_FORMAT_R16G16B16A16_SINT = 96
    VK_FORMAT_R16G16B16A16_SFLOAT = 97
    VK_FORMAT_R32_UINT = 98
    VK_FORMAT_R32_SINT = 99
    VK_FORMAT_R32_SFLOAT = 100
    VK_FORMAT_R32G32_UINT = 101
    VK_FORMAT_R32G32_SINT = 102
    VK_FORMAT_R32G32_SFLOAT = 103
    VK_FORMAT_R32G32B32_UINT = 104
    VK_FORMAT_R32G32B32_SINT = 105
    VK_FORMAT_R32G32B32_SFLOAT = 106
    VK_FORMAT_R32G32B32A32_UINT = 107
    VK_FORMAT_R32G32B32A32_SINT = 108
    VK_FORMAT_R32G32B32A32_SFLOAT = 109
    VK_FORMAT_R64_UINT = 110
    VK_FORMAT_R64_SINT = 111
    VK_FORMAT_R64_SFLOAT = 112
    VK_FORMAT_R64G64_UINT = 113
    VK_FORMAT_R64G64_SINT = 114
    VK_FORMAT_R64G64_SFLOAT = 115
    VK_FORMAT_R64G64B64_UINT = 116
    VK_FORMAT_R64G64B64_SINT = 117
    VK_FORMAT_R64G64B64_SFLOAT = 118
    VK_FORMAT_R64G64B64A64_UINT = 119
    VK_FORMAT_R64G64B64A64_SINT = 120
    VK_FORMAT_R64G64B64A64_SFLOAT = 121
    VK_FORMAT_B10G11R11_UFLOAT_PACK32 = 122
    VK_FORMAT_E5B9G9R9_UFLOAT_PACK32 = 123
    VK_FORMAT_D16_UNORM = 124
    VK_FORMAT_X8_D24_UNORM_PACK32 = 125
    VK_FORMAT_D32_SFLOAT = 126
    VK_FORMAT_S8_UINT = 127
    VK_FORMAT_D16_UNORM_S8_UINT = 128
    VK_FORMAT_D24_UNORM_S8_UINT = 129
    VK_FORMAT_D32_SFLOAT_S8_UINT = 130
    VK_FORMAT_BC1_RGB_UNORM_BLOCK = 131
    VK_FORMAT_BC1_RGB_SRGB_BLOCK = 132
    VK_FORMAT_BC1_RGBA_UNORM_BLOCK = 133
    VK_FORMAT_BC1_RGBA_SRGB_BLOCK = 134
    VK_FORMAT_BC2_UNORM_BLOCK = 135
    VK_FORMAT_BC2_SRGB_BLOCK = 136
    VK_FORMAT_BC3_UNORM_BLOCK = 137
    VK_FORMAT_BC3_SRGB_BLOCK = 138
    VK_FORMAT_BC4_UNORM_BLOCK = 139
    VK_FORMAT_BC4_SNORM_BLOCK = 140
    VK_FORMAT_BC5_UNORM_BLOCK = 141
    VK_FORMAT_BC5_SNORM_BLOCK = 142
    VK_FORMAT_BC6H_UFLOAT_BLOCK = 143
    VK_FORMAT_BC6H_SFLOAT_BLOCK = 144
    VK_FORMAT_BC7_UNORM_BLOCK = 145
    VK_FORMAT_BC7_SRGB_BLOCK = 146
    VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK = 147
    VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK = 148
    VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK = 149
    VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK = 150
    VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK = 151
    VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK = 152
    VK_FORMAT_EAC_R11_UNORM_BLOCK = 153
    VK_FORMAT_EAC_R11_SNORM_BLOCK = 154
    VK_FORMAT_EAC_R11G11_UNORM_BLOCK = 155
    VK_FORMAT_EAC_R11G11_SNORM_BLOCK = 156
    VK_FORMAT_ASTC_4x4_UNORM_BLOCK = 157
    VK_FORMAT_ASTC_4x4_SRGB_BLOCK = 158
    VK_FORMAT_ASTC_5x4_UNORM_BLOCK = 159
    VK_FORMAT_ASTC_5x4_SRGB_BLOCK = 160
    VK_FORMAT_ASTC_5x5_UNORM_BLOCK = 161
    VK_FORMAT_ASTC_5x5_SRGB_BLOCK = 162
    VK_FORMAT_ASTC_6x5_UNORM_BLOCK = 163
    VK_FORMAT_ASTC_6x5_SRGB_BLOCK = 164
    VK_FORMAT_ASTC_6x6_UNORM_BLOCK = 165
    VK_FORMAT_ASTC_6x6_SRGB_BLOCK = 166
    VK_FORMAT_ASTC_8x5_UNORM_BLOCK = 167
    VK_FORMAT_ASTC_8x5_SRGB_BLOCK = 168
    VK_FORMAT_ASTC_8x6_UNORM_BLOCK = 169
    VK_FORMAT_ASTC_8x6_SRGB_BLOCK = 170
    VK_FORMAT_ASTC_8x8_UNORM_BLOCK = 171
    VK_FORMAT_ASTC_8x8_SRGB_BLOCK = 172
    VK_FORMAT_ASTC_10x5_UNORM_BLOCK = 173
    VK_FORMAT_ASTC_10x5_SRGB_BLOCK = 174
    VK_FORMAT_ASTC_10x6_UNORM_BLOCK = 175
    VK_FORMAT_ASTC_10x6_SRGB_BLOCK = 176
    VK_FORMAT_ASTC_10x8_UNORM_BLOCK = 177
    VK_FORMAT_ASTC_10x8_SRGB_BLOCK = 178
    VK_FORMAT_ASTC_10x10_UNORM_BLOCK = 179
    VK_FORMAT_ASTC_10x10_SRGB_BLOCK = 180
    VK_FORMAT_ASTC_12x10_UNORM_BLOCK = 181
    VK_FORMAT_ASTC_12x10_SRGB_BLOCK = 182
    VK_FORMAT_ASTC_12x12_UNORM_BLOCK = 183
    VK_FORMAT_ASTC_12x12_SRGB_BLOCK = 184
    VK_FORMAT_G8B8G8R8_422_UNORM = 1000156000
    VK_FORMAT_B8G8R8G8_422_UNORM = 1000156001
    VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM = 1000156002
    VK_FORMAT_G8_B8R8_2PLANE_420_UNORM = 1000156003
    VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM = 1000156004
    VK_FORMAT_G8_B8R8_2PLANE_422_UNORM = 1000156005
    VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM = 1000156006
    VK_FORMAT_R10X6_UNORM_PACK16 = 1000156007
    VK_FORMAT_R10X6G10X6_UNORM_2PACK16 = 1000156008
    VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16 = 1000156009
    VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16 = 1000156010
    VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16 = 1000156011
    VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16 = 1000156012
    VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16 = 1000156013
    VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16 = 1000156014
    VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16 = 1000156015
    VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16 = 1000156016
    VK_FORMAT_R12X4_UNORM_PACK16 = 1000156017
    VK_FORMAT_R12X4G12X4_UNORM_2PACK16 = 1000156018
    VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16 = 1000156019
    VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16 = 1000156020
    VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16 = 1000156021
    VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16 = 1000156022
    VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16 = 1000156023
    VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16 = 1000156024
    VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16 = 1000156025
    VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16 = 1000156026
    VK_FORMAT_G16B16G16R16_422_UNORM = 1000156027
    VK_FORMAT_B16G16R16G16_422_UNORM = 1000156028
    VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM = 1000156029
    VK_FORMAT_G16_B16R16_2PLANE_420_UNORM = 1000156030
    VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM = 1000156031
    VK_FORMAT_G16_B16R16_2PLANE_422_UNORM = 1000156032
    VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM = 1000156033
    VK_FORMAT_G8_B8R8_2PLANE_444_UNORM = 1000330000
    VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16 = 1000330001
    VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16 = 1000330002
    VK_FORMAT_G16_B16R16_2PLANE_444_UNORM = 1000330003
    VK_FORMAT_A4R4G4B4_UNORM_PACK16 = 1000340000
    VK_FORMAT_A4B4G4R4_UNORM_PACK16 = 1000340001
    VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK = 1000066000
    VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK = 1000066001
    VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK = 1000066002
    VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK = 1000066003
    VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK = 1000066004
    VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK = 1000066005
    VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK = 1000066006
    VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK = 1000066007
    VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK = 1000066008
    VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK = 1000066009
    VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK = 1000066010
    VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK = 1000066011
    VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK = 1000066012
    VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK = 1000066013
    VK_FORMAT_A1B5G5R5_UNORM_PACK16 = 1000470000
    VK_FORMAT_A8_UNORM = 1000470001
    VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG = 1000054000
    VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG = 1000054001
    VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG = 1000054002
    VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG = 1000054003
    VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG = 1000054004
    VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG = 1000054005
    VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG = 1000054006
    VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG = 1000054007
    VK_FORMAT_R8_BOOL_ARM = 1000460000
    VK_FORMAT_R16G16_SFIXED5_NV = 1000464000
    VK_FORMAT_R10X6_UINT_PACK16_ARM = 1000609000
    VK_FORMAT_R10X6G10X6_UINT_2PACK16_ARM = 1000609001
    VK_FORMAT_R10X6G10X6B10X6A10X6_UINT_4PACK16_ARM = 1000609002
    VK_FORMAT_R12X4_UINT_PACK16_ARM = 1000609003
    VK_FORMAT_R12X4G12X4_UINT_2PACK16_ARM = 1000609004
    VK_FORMAT_R12X4G12X4B12X4A12X4_UINT_4PACK16_ARM = 1000609005
    VK_FORMAT_R14X2_UINT_PACK16_ARM = 1000609006
    VK_FORMAT_R14X2G14X2_UINT_2PACK16_ARM = 1000609007
    VK_FORMAT_R14X2G14X2B14X2A14X2_UINT_4PACK16_ARM = 1000609008
    VK_FORMAT_R14X2_UNORM_PACK16_ARM = 1000609009
    VK_FORMAT_R14X2G14X2_UNORM_2PACK16_ARM = 1000609010
    VK_FORMAT_R14X2G14X2B14X2A14X2_UNORM_4PACK16_ARM = 1000609011
    VK_FORMAT_G14X2_B14X2R14X2_2PLANE_420_UNORM_3PACK16_ARM = 1000609012
    VK_FORMAT_G14X2_B14X2R14X2_2PLANE_422_UNORM_3PACK16_ARM = 1000609013

class VkRayTracingInvocationReorderModeNV(Enum):
    VK_RAY_TRACING_INVOCATION_REORDER_MODE_NONE_NV = 0
    VK_RAY_TRACING_INVOCATION_REORDER_MODE_REORDER_NV = 1

class VkSampleCountFlagBits(Enum):
    VK_SAMPLE_COUNT_1_BIT = 0
    VK_SAMPLE_COUNT_2_BIT = 1
    VK_SAMPLE_COUNT_4_BIT = 2
    VK_SAMPLE_COUNT_8_BIT = 3
    VK_SAMPLE_COUNT_16_BIT = 4
    VK_SAMPLE_COUNT_32_BIT = 5
    VK_SAMPLE_COUNT_64_BIT = 6

class VkExternalMemoryHandleTypeFlagBits(Enum):
    VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT = 0
    VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT = 1
    VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT = 2
    VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT = 3
    VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT = 4
    VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_HEAP_BIT = 5
    VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT = 6

class VkExternalSemaphoreHandleTypeFlagBits(Enum):
    VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT = 0
    VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT = 1
    VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT = 2
    VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE_BIT = 3
    VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT = 4

class VkExternalFenceHandleTypeFlagBits(Enum):
    VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT = 0
    VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT = 1
    VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT = 2
    VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT = 3

class VkPointClippingBehavior(Enum):
    VK_POINT_CLIPPING_BEHAVIOR_ALL_CLIP_PLANES = 0
    VK_POINT_CLIPPING_BEHAVIOR_USER_CLIP_PLANES_ONLY = 1

class VkChromaLocation(Enum):
    VK_CHROMA_LOCATION_COSITED_EVEN = 0
    VK_CHROMA_LOCATION_MIDPOINT = 1

class VkDriverId(Enum):
    VK_DRIVER_ID_AMD_PROPRIETARY = 1
    VK_DRIVER_ID_AMD_OPEN_SOURCE = 2
    VK_DRIVER_ID_MESA_RADV = 3
    VK_DRIVER_ID_NVIDIA_PROPRIETARY = 4
    VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS = 5
    VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA = 6
    VK_DRIVER_ID_IMAGINATION_PROPRIETARY = 7
    VK_DRIVER_ID_QUALCOMM_PROPRIETARY = 8
    VK_DRIVER_ID_ARM_PROPRIETARY = 9
    VK_DRIVER_ID_GOOGLE_SWIFTSHADER = 10
    VK_DRIVER_ID_GGP_PROPRIETARY = 11
    VK_DRIVER_ID_BROADCOM_PROPRIETARY = 12
    VK_DRIVER_ID_MESA_LLVMPIPE = 13
    VK_DRIVER_ID_MOLTENVK = 14
    VK_DRIVER_ID_COREAVI_PROPRIETARY = 15
    VK_DRIVER_ID_JUICE_PROPRIETARY = 16
    VK_DRIVER_ID_VERISILICON_PROPRIETARY = 17
    VK_DRIVER_ID_MESA_TURNIP = 18
    VK_DRIVER_ID_MESA_V3DV = 19
    VK_DRIVER_ID_MESA_PANVK = 20
    VK_DRIVER_ID_SAMSUNG_PROPRIETARY = 21
    VK_DRIVER_ID_MESA_VENUS = 22
    VK_DRIVER_ID_MESA_DOZEN = 23
    VK_DRIVER_ID_MESA_NVK = 24
    VK_DRIVER_ID_IMAGINATION_OPEN_SOURCE_MESA = 25
    VK_DRIVER_ID_MESA_HONEYKRISP = 26
    VK_DRIVER_ID_VULKAN_SC_EMULATION_ON_VULKAN = 27

class VkShaderFloatControlsIndependence(Enum):
    VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_32_BIT_ONLY = 0
    VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL = 1
    VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_NONE = 2

class VkPipelineRobustnessBufferBehavior(Enum):
    VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DEVICE_DEFAULT = 0
    VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED = 1
    VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS = 2
    VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_2 = 3

class VkPipelineRobustnessImageBehavior(Enum):
    VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_DEVICE_DEFAULT = 0
    VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_DISABLED = 1
    VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_ROBUST_IMAGE_ACCESS = 2
    VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_ROBUST_IMAGE_ACCESS_2 = 3

class VkPhysicalDeviceLayeredApiKHR(Enum):
    VK_PHYSICAL_DEVICE_LAYERED_API_VULKAN_KHR = 0
    VK_PHYSICAL_DEVICE_LAYERED_API_D3D12_KHR = 1
    VK_PHYSICAL_DEVICE_LAYERED_API_METAL_KHR = 2
    VK_PHYSICAL_DEVICE_LAYERED_API_OPENGL_KHR = 3
    VK_PHYSICAL_DEVICE_LAYERED_API_OPENGLES_KHR = 4

class VkLayeredDriverUnderlyingApiMSFT(Enum):
    VK_LAYERED_DRIVER_UNDERLYING_API_NONE_MSFT = 0
    VK_LAYERED_DRIVER_UNDERLYING_API_D3D12_MSFT = 1

class VkDefaultVertexAttributeValueKHR(Enum):
    VK_DEFAULT_VERTEX_ATTRIBUTE_VALUE_ZERO_ZERO_ZERO_ZERO_KHR = 0
    VK_DEFAULT_VERTEX_ATTRIBUTE_VALUE_ZERO_ZERO_ZERO_ONE_KHR = 1


# --- API Constant values extracted from vk.xml ---
VK_MAX_PHYSICAL_DEVICE_NAME_SIZE = 256
VK_UUID_SIZE = 16
VK_LUID_SIZE = 8
VK_MAX_EXTENSION_NAME_SIZE = 256
VK_MAX_DESCRIPTION_SIZE = 256
VK_MAX_MEMORY_TYPES = 32
VK_MAX_MEMORY_HEAPS = 16
VK_LOD_CLAMP_NONE = 1000.0
VK_REMAINING_MIP_LEVELS = 4294967295
VK_REMAINING_ARRAY_LAYERS = 4294967295
VK_REMAINING_3D_SLICES_EXT = 4294967295
VK_WHOLE_SIZE = 0xFFFFFFFFFFFFFFFF
VK_ATTACHMENT_UNUSED = 4294967295
VK_TRUE = 1
VK_FALSE = 0
VK_QUEUE_FAMILY_IGNORED = 4294967295
VK_QUEUE_FAMILY_EXTERNAL = 4294967294
VK_QUEUE_FAMILY_FOREIGN_EXT = 4294967293
VK_SUBPASS_EXTERNAL = 4294967295
VK_MAX_DEVICE_GROUP_SIZE = 32
VK_MAX_DRIVER_NAME_SIZE = 256
VK_MAX_DRIVER_INFO_SIZE = 256
VK_SHADER_UNUSED_KHR = 4294967295
VK_MAX_GLOBAL_PRIORITY_SIZE = 16
VK_MAX_SHADER_MODULE_IDENTIFIER_SIZE_EXT = 32
VK_MAX_PIPELINE_BINARY_KEY_SIZE_KHR = 32
VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR = 7
VK_MAX_VIDEO_VP9_REFERENCES_PER_FRAME_KHR = 3
VK_SHADER_INDEX_UNUSED_AMDX = 4294967295
VK_PARTITIONED_ACCELERATION_STRUCTURE_PARTITION_INDEX_GLOBAL_NV = 4294967295


# --- Computed VK_API_VERSION Constants ---
VK_API_VERSION_MAP = {
    "VK_API_VERSION_1_0": 4194304,
    "VK_API_VERSION_1_1": 4198400,
    "VK_API_VERSION_1_2": 4202496,
    "VK_API_VERSION_1_3": 4206592,
    "VK_API_VERSION_1_4": 4210688,
}

# --- VkFlags Type Aliases ---
VkFramebufferCreateFlags = VkFlags
VkQueryPoolCreateFlags = VkFlags
VkRenderPassCreateFlags = VkFlags
VkSamplerCreateFlags = VkFlags
VkPipelineCacheCreateFlags = VkFlags
VkPipelineDepthStencilStateCreateFlags = VkFlags
VkPipelineDynamicStateCreateFlags = VkFlags
VkPipelineColorBlendStateCreateFlags = VkFlags
VkPipelineMultisampleStateCreateFlags = VkFlags
VkPipelineRasterizationStateCreateFlags = VkFlags
VkPipelineViewportStateCreateFlags = VkFlags
VkPipelineTessellationStateCreateFlags = VkFlags
VkPipelineInputAssemblyStateCreateFlags = VkFlags
VkPipelineVertexInputStateCreateFlags = VkFlags
VkPipelineShaderStageCreateFlags = VkFlags
VkDescriptorSetLayoutCreateFlags = VkFlags
VkBufferViewCreateFlags = VkFlags
VkInstanceCreateFlags = VkFlags
VkDeviceCreateFlags = VkFlags
VkDeviceQueueCreateFlags = VkFlags
VkQueueFlags = VkFlags
VkMemoryPropertyFlags = VkFlags
VkMemoryHeapFlags = VkFlags
VkAccessFlags = VkFlags
VkBufferUsageFlags = VkFlags
VkBufferCreateFlags = VkFlags
VkShaderStageFlags = VkFlags
VkImageUsageFlags = VkFlags
VkImageCreateFlags = VkFlags
VkImageViewCreateFlags = VkFlags
VkPipelineCreateFlags = VkFlags
VkColorComponentFlags = VkFlags
VkFenceCreateFlags = VkFlags
VkSemaphoreCreateFlags = VkFlags
VkFormatFeatureFlags = VkFlags
VkQueryControlFlags = VkFlags
VkEventCreateFlags = VkFlags
VkCommandPoolCreateFlags = VkFlags
VkCommandBufferUsageFlags = VkFlags
VkQueryPipelineStatisticFlags = VkFlags
VkMemoryMapFlags = VkFlags
VkMemoryUnmapFlags = VkFlags
VkImageAspectFlags = VkFlags
VkSparseMemoryBindFlags = VkFlags
VkSparseImageFormatFlags = VkFlags
VkSubpassDescriptionFlags = VkFlags
VkPipelineStageFlags = VkFlags
VkSampleCountFlags = VkFlags
VkAttachmentDescriptionFlags = VkFlags
VkCullModeFlags = VkFlags
VkDescriptorPoolCreateFlags = VkFlags
VkDependencyFlags = VkFlags
VkSubgroupFeatureFlags = VkFlags
VkPrivateDataSlotCreateFlags = VkFlags
VkDescriptorUpdateTemplateCreateFlags = VkFlags
VkPipelineCreationFeedbackFlags = VkFlags
VkSemaphoreWaitFlags = VkFlags
VkShaderCorePropertiesFlagsAMD = VkFlags
VkAccessFlags2 = VkFlags64
VkPipelineStageFlags2 = VkFlags64
VkFormatFeatureFlags2 = VkFlags64
VkRenderingFlags = VkFlags
VkMemoryDecompressionMethodFlagsNV = VkFlags64
VkIndirectCommandsInputModeFlagsEXT = VkFlags
VkSurfaceTransformFlagsKHR = VkFlags
VkExternalMemoryHandleTypeFlagsNV = VkFlags
VkExternalMemoryFeatureFlagsNV = VkFlags
VkExternalMemoryHandleTypeFlags = VkFlags
VkExternalMemoryFeatureFlags = VkFlags
VkExternalSemaphoreHandleTypeFlags = VkFlags
VkExternalSemaphoreFeatureFlags = VkFlags
VkExternalFenceHandleTypeFlags = VkFlags
VkExternalFenceFeatureFlags = VkFlags
VkResolveModeFlags = VkFlags
VkToolPurposeFlags = VkFlags
VkSubmitFlags = VkFlags
VkHostImageCopyFlags = VkFlags
VkOpticalFlowGridSizeFlagsNV = VkFlags
VkPhysicalDeviceSchedulingControlsFlagsARM = VkFlags64


# --- Empty Handle Dataclasses ---
@dataclass
class VkPhysicalDevice:
    pass

# --- Pre-defined Struct Definitions ---

@dataclass
class VkExtent3D:
    width: uint32_t
    height: uint32_t
    depth: uint32_t

@dataclass
class VkImageFormatProperties:
    maxExtent: VkExtent3D
    maxMipLevels: uint32_t
    maxArrayLayers: uint32_t
    sampleCounts: VkSampleCountFlags
    maxResourceSize: VkDeviceSize

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


# --- Vulkan Struct Definitions (Dependencies first, then PhysicalDevice structs) ---
@dataclass
class VkExtent2D:
    width: uint32_t
    height: uint32_t

@dataclass
class VkMemoryType:
    propertyFlags: VkMemoryPropertyFlags
    heapIndex: uint32_t

@dataclass
class VkMemoryHeap:
    size: VkDeviceSize
    flags: VkMemoryHeapFlags

@dataclass
class VkPhysicalDeviceSparseProperties:
    residencyStandard2DBlockShape: VkBool32
    residencyStandard2DMultisampleBlockShape: VkBool32
    residencyStandard3DBlockShape: VkBool32
    residencyAlignedMipSize: VkBool32
    residencyNonResidentStrict: VkBool32

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
    maxComputeWorkGroupCount: uint32_t * 3
    maxComputeWorkGroupInvocations: uint32_t
    maxComputeWorkGroupSize: uint32_t * 3
    subPixelPrecisionBits: uint32_t
    subTexelPrecisionBits: uint32_t
    mipmapPrecisionBits: uint32_t
    maxDrawIndexedIndexValue: uint32_t
    maxDrawIndirectCount: uint32_t
    maxSamplerLodBias: float
    maxSamplerAnisotropy: float
    maxViewports: uint32_t
    maxViewportDimensions: uint32_t * 2
    viewportBoundsRange: float_t * 2
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
    pointSizeRange: float_t * 2
    lineWidthRange: float_t * 2
    pointSizeGranularity: float
    lineWidthGranularity: float
    strictLines: VkBool32
    standardSampleLocations: VkBool32
    optimalBufferCopyOffsetAlignment: VkDeviceSize
    optimalBufferCopyRowPitchAlignment: VkDeviceSize
    nonCoherentAtomSize: VkDeviceSize

@dataclass
class VkConformanceVersion:
    major: uint8_t
    minor: uint8_t
    subminor: uint8_t
    patch: uint8_t

@dataclass
class VkPhysicalDeviceLayeredApiPropertiesKHR:
    vendorID: uint32_t
    deviceID: uint32_t
    layeredAPI: VkPhysicalDeviceLayeredApiKHR
    deviceName: str

@dataclass
class VkPhysicalDeviceProperties:
    apiVersion: uint32_t
    driverVersion: uint32_t
    vendorID: uint32_t
    deviceID: uint32_t
    deviceType: VkPhysicalDeviceType
    deviceName: str
    pipelineCacheUUID: uint8_t * VK_UUID_SIZE
    limits: VkPhysicalDeviceLimits
    sparseProperties: VkPhysicalDeviceSparseProperties

@dataclass
class VkPhysicalDeviceMemoryProperties:
    memoryTypeCount: uint32_t
    memoryTypes: List[VkMemoryType]
    memoryHeapCount: uint32_t
    memoryHeaps: List[VkMemoryHeap]

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
class VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV:
    deviceGeneratedCommands: VkBool32

@dataclass
class VkPhysicalDeviceDeviceGeneratedCommandsComputeFeaturesNV:
    deviceGeneratedCompute: VkBool32
    deviceGeneratedComputePipelines: VkBool32
    deviceGeneratedComputeCaptureReplay: VkBool32

@dataclass
class VkPhysicalDevicePrivateDataFeatures:
    privateData: VkBool32

@dataclass
class VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV:
    maxGraphicsShaderGroupCount: uint32_t
    maxIndirectSequenceCount: uint32_t
    maxIndirectCommandsTokenCount: uint32_t
    maxIndirectCommandsStreamCount: uint32_t
    maxIndirectCommandsTokenOffset: uint32_t
    maxIndirectCommandsStreamStride: uint32_t
    minSequencesCountBufferOffsetAlignment: uint32_t
    minSequencesIndexBufferOffsetAlignment: uint32_t
    minIndirectCommandsBufferOffsetAlignment: uint32_t

@dataclass
class VkPhysicalDeviceClusterAccelerationStructureFeaturesNV:
    clusterAccelerationStructure: VkBool32

@dataclass
class VkPhysicalDeviceClusterAccelerationStructurePropertiesNV:
    maxVerticesPerCluster: uint32_t
    maxTrianglesPerCluster: uint32_t
    clusterScratchByteAlignment: uint32_t
    clusterByteAlignment: uint32_t
    clusterTemplateByteAlignment: uint32_t
    clusterBottomLevelByteAlignment: uint32_t
    clusterTemplateBoundsByteAlignment: uint32_t
    maxClusterGeometryIndex: uint32_t

@dataclass
class VkPhysicalDeviceMultiDrawPropertiesEXT:
    maxMultiDrawCount: uint32_t

@dataclass
class VkPhysicalDeviceProperties2:
    properties: VkPhysicalDeviceProperties

@dataclass
class VkPhysicalDeviceImageFormatInfo2:
    format: VkFormat
    type: VkImageType
    tiling: VkImageTiling
    usage: VkImageUsageFlags
    flags: VkImageCreateFlags

@dataclass
class VkPhysicalDeviceMemoryProperties2:
    memoryProperties: VkPhysicalDeviceMemoryProperties

@dataclass
class VkPhysicalDeviceSparseImageFormatInfo2:
    format: VkFormat
    type: VkImageType
    samples: VkSampleCountFlagBits
    usage: VkImageUsageFlags
    tiling: VkImageTiling

@dataclass
class VkPhysicalDevicePushDescriptorProperties:
    maxPushDescriptors: uint32_t

@dataclass
class VkPhysicalDeviceDriverProperties:
    driverID: VkDriverId
    driverName: str
    driverInfo: str
    conformanceVersion: VkConformanceVersion

@dataclass
class VkPhysicalDeviceVariablePointersFeatures:
    variablePointersStorageBuffer: VkBool32
    variablePointers: VkBool32

@dataclass
class VkPhysicalDeviceExternalBufferInfo:
    flags: VkBufferCreateFlags
    usage: VkBufferUsageFlags
    handleType: VkExternalMemoryHandleTypeFlagBits

@dataclass
class VkPhysicalDeviceIDProperties:
    deviceUUID: uint8_t * VK_UUID_SIZE
    driverUUID: uint8_t * VK_UUID_SIZE
    deviceLUID: uint8_t * VK_LUID_SIZE
    deviceNodeMask: uint32_t
    deviceLUIDValid: VkBool32

@dataclass
class VkPhysicalDeviceExternalSemaphoreInfo:
    handleType: VkExternalSemaphoreHandleTypeFlagBits

@dataclass
class VkPhysicalDeviceExternalFenceInfo:
    handleType: VkExternalFenceHandleTypeFlagBits

@dataclass
class VkPhysicalDeviceMultiviewFeatures:
    multiview: VkBool32
    multiviewGeometryShader: VkBool32
    multiviewTessellationShader: VkBool32

@dataclass
class VkPhysicalDeviceMultiviewProperties:
    maxMultiviewViewCount: uint32_t
    maxMultiviewInstanceIndex: uint32_t

@dataclass
class VkPhysicalDeviceGroupProperties:
    physicalDeviceCount: uint32_t
    physicalDevices: List[VkPhysicalDevice]
    subsetAllocation: VkBool32

@dataclass
class VkPhysicalDevicePresentIdFeaturesKHR:
    presentId: VkBool32

@dataclass
class VkPhysicalDevicePresentId2FeaturesKHR:
    presentId2: VkBool32

@dataclass
class VkPhysicalDevicePresentWaitFeaturesKHR:
    presentWait: VkBool32

@dataclass
class VkPhysicalDevicePresentWait2FeaturesKHR:
    presentWait2: VkBool32

@dataclass
class VkPhysicalDeviceDiscardRectanglePropertiesEXT:
    maxDiscardRectangles: uint32_t

@dataclass
class VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX:
    perViewPositionAllComponents: VkBool32

@dataclass
class VkPhysicalDevice16BitStorageFeatures:
    storageBuffer16BitAccess: VkBool32
    uniformAndStorageBuffer16BitAccess: VkBool32
    storagePushConstant16: VkBool32
    storageInputOutput16: VkBool32

@dataclass
class VkPhysicalDeviceSubgroupProperties:
    subgroupSize: uint32_t
    supportedStages: VkShaderStageFlags
    supportedOperations: VkSubgroupFeatureFlags
    quadOperationsInAllStages: VkBool32

@dataclass
class VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures:
    shaderSubgroupExtendedTypes: VkBool32

@dataclass
class VkPhysicalDevicePointClippingProperties:
    pointClippingBehavior: VkPointClippingBehavior

@dataclass
class VkPhysicalDeviceSamplerYcbcrConversionFeatures:
    samplerYcbcrConversion: VkBool32

@dataclass
class VkPhysicalDeviceProtectedMemoryFeatures:
    protectedMemory: VkBool32

@dataclass
class VkPhysicalDeviceProtectedMemoryProperties:
    protectedNoFault: VkBool32

@dataclass
class VkPhysicalDeviceSamplerFilterMinmaxProperties:
    filterMinmaxSingleComponentFormats: VkBool32
    filterMinmaxImageComponentMapping: VkBool32

@dataclass
class VkPhysicalDeviceSampleLocationsPropertiesEXT:
    sampleLocationSampleCounts: VkSampleCountFlags
    maxSampleLocationGridSize: VkExtent2D
    sampleLocationCoordinateRange: float_t * 2
    sampleLocationSubPixelBits: uint32_t
    variableSampleLocations: VkBool32

@dataclass
class VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT:
    advancedBlendCoherentOperations: VkBool32

@dataclass
class VkPhysicalDeviceMultiDrawFeaturesEXT:
    multiDraw: VkBool32

@dataclass
class VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT:
    advancedBlendMaxColorAttachments: uint32_t
    advancedBlendIndependentBlend: VkBool32
    advancedBlendNonPremultipliedSrcColor: VkBool32
    advancedBlendNonPremultipliedDstColor: VkBool32
    advancedBlendCorrelatedOverlap: VkBool32
    advancedBlendAllOperations: VkBool32

@dataclass
class VkPhysicalDeviceInlineUniformBlockFeatures:
    inlineUniformBlock: VkBool32
    descriptorBindingInlineUniformBlockUpdateAfterBind: VkBool32

@dataclass
class VkPhysicalDeviceInlineUniformBlockProperties:
    maxInlineUniformBlockSize: uint32_t
    maxPerStageDescriptorInlineUniformBlocks: uint32_t
    maxPerStageDescriptorUpdateAfterBindInlineUniformBlocks: uint32_t
    maxDescriptorSetInlineUniformBlocks: uint32_t
    maxDescriptorSetUpdateAfterBindInlineUniformBlocks: uint32_t

@dataclass
class VkPhysicalDeviceMaintenance3Properties:
    maxPerSetDescriptors: uint32_t
    maxMemoryAllocationSize: VkDeviceSize

@dataclass
class VkPhysicalDeviceMaintenance4Features:
    maintenance4: VkBool32

@dataclass
class VkPhysicalDeviceMaintenance4Properties:
    maxBufferSize: VkDeviceSize

@dataclass
class VkPhysicalDeviceMaintenance5Features:
    maintenance5: VkBool32

@dataclass
class VkPhysicalDeviceMaintenance5Properties:
    earlyFragmentMultisampleCoverageAfterSampleCounting: VkBool32
    earlyFragmentSampleMaskTestBeforeSampleCounting: VkBool32
    depthStencilSwizzleOneSupport: VkBool32
    polygonModePointSize: VkBool32
    nonStrictSinglePixelWideLinesUseParallelogram: VkBool32
    nonStrictWideLinesUseParallelogram: VkBool32

@dataclass
class VkPhysicalDeviceMaintenance6Features:
    maintenance6: VkBool32

@dataclass
class VkPhysicalDeviceMaintenance6Properties:
    blockTexelViewCompatibleMultipleLayers: VkBool32
    maxCombinedImageSamplerDescriptorCount: uint32_t
    fragmentShadingRateClampCombinerInputs: VkBool32

@dataclass
class VkPhysicalDeviceMaintenance7FeaturesKHR:
    maintenance7: VkBool32

@dataclass
class VkPhysicalDeviceMaintenance7PropertiesKHR:
    robustFragmentShadingRateAttachmentAccess: VkBool32
    separateDepthStencilAttachmentAccess: VkBool32
    maxDescriptorSetTotalUniformBuffersDynamic: uint32_t
    maxDescriptorSetTotalStorageBuffersDynamic: uint32_t
    maxDescriptorSetTotalBuffersDynamic: uint32_t
    maxDescriptorSetUpdateAfterBindTotalUniformBuffersDynamic: uint32_t
    maxDescriptorSetUpdateAfterBindTotalStorageBuffersDynamic: uint32_t
    maxDescriptorSetUpdateAfterBindTotalBuffersDynamic: uint32_t

@dataclass
class VkPhysicalDeviceLayeredApiPropertiesListKHR:
    layeredApiCount: uint32_t
    pLayeredApis: List[VkPhysicalDeviceLayeredApiPropertiesKHR]

@dataclass
class VkPhysicalDeviceMaintenance8FeaturesKHR:
    maintenance8: VkBool32

@dataclass
class VkPhysicalDeviceMaintenance9FeaturesKHR:
    maintenance9: VkBool32

@dataclass
class VkPhysicalDeviceMaintenance9PropertiesKHR:
    image2DViewOf3DSparse: VkBool32
    defaultVertexAttributeValue: VkDefaultVertexAttributeValueKHR

@dataclass
class VkPhysicalDeviceShaderDrawParametersFeatures:
    shaderDrawParameters: VkBool32

@dataclass
class VkPhysicalDeviceShaderFloat16Int8Features:
    shaderFloat16: VkBool32
    shaderInt8: VkBool32

@dataclass
class VkPhysicalDeviceFloatControlsProperties:
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

@dataclass
class VkPhysicalDeviceHostQueryResetFeatures:
    hostQueryReset: VkBool32

@dataclass
class VkPhysicalDeviceGlobalPriorityQueryFeatures:
    globalPriorityQuery: VkBool32

@dataclass
class VkPhysicalDeviceDeviceMemoryReportFeaturesEXT:
    deviceMemoryReport: VkBool32

@dataclass
class VkPhysicalDeviceExternalMemoryHostPropertiesEXT:
    minImportedHostPointerAlignment: VkDeviceSize

@dataclass
class VkPhysicalDeviceConservativeRasterizationPropertiesEXT:
    primitiveOverestimationSize: float
    maxExtraPrimitiveOverestimationSize: float
    extraPrimitiveOverestimationSizeGranularity: float
    primitiveUnderestimation: VkBool32
    conservativePointAndLineRasterization: VkBool32
    degenerateTrianglesRasterized: VkBool32
    degenerateLinesRasterized: VkBool32
    fullyCoveredFragmentShaderInputVariable: VkBool32
    conservativeRasterizationPostDepthCoverage: VkBool32

@dataclass
class VkPhysicalDeviceShaderCorePropertiesAMD:
    shaderEngineCount: uint32_t
    shaderArraysPerEngineCount: uint32_t
    computeUnitsPerShaderArray: uint32_t
    simdPerComputeUnit: uint32_t
    wavefrontsPerSimd: uint32_t
    wavefrontSize: uint32_t
    sgprsPerSimd: uint32_t
    minSgprAllocation: uint32_t
    maxSgprAllocation: uint32_t
    sgprAllocationGranularity: uint32_t
    vgprsPerSimd: uint32_t
    minVgprAllocation: uint32_t
    maxVgprAllocation: uint32_t
    vgprAllocationGranularity: uint32_t

@dataclass
class VkPhysicalDeviceShaderCoreProperties2AMD:
    shaderCoreFeatures: VkShaderCorePropertiesFlagsAMD
    activeComputeUnitCount: uint32_t

@dataclass
class VkPhysicalDeviceDescriptorIndexingFeatures:
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

@dataclass
class VkPhysicalDeviceDescriptorIndexingProperties:
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

@dataclass
class VkPhysicalDeviceTimelineSemaphoreFeatures:
    timelineSemaphore: VkBool32

@dataclass
class VkPhysicalDeviceTimelineSemaphoreProperties:
    maxTimelineSemaphoreValueDifference: uint64_t

@dataclass
class VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT:
    maxVertexAttribDivisor: uint32_t

@dataclass
class VkPhysicalDeviceVertexAttributeDivisorProperties:
    maxVertexAttribDivisor: uint32_t
    supportsNonZeroFirstInstance: VkBool32

@dataclass
class VkPhysicalDevicePCIBusInfoPropertiesEXT:
    pciDomain: uint32_t
    pciBus: uint32_t
    pciDevice: uint32_t
    pciFunction: uint32_t

@dataclass
class VkPhysicalDevice8BitStorageFeatures:
    storageBuffer8BitAccess: VkBool32
    uniformAndStorageBuffer8BitAccess: VkBool32
    storagePushConstant8: VkBool32

@dataclass
class VkPhysicalDeviceConditionalRenderingFeaturesEXT:
    conditionalRendering: VkBool32
    inheritedConditionalRendering: VkBool32

@dataclass
class VkPhysicalDeviceVulkanMemoryModelFeatures:
    vulkanMemoryModel: VkBool32
    vulkanMemoryModelDeviceScope: VkBool32
    vulkanMemoryModelAvailabilityVisibilityChains: VkBool32

@dataclass
class VkPhysicalDeviceShaderAtomicInt64Features:
    shaderBufferInt64Atomics: VkBool32
    shaderSharedInt64Atomics: VkBool32

@dataclass
class VkPhysicalDeviceShaderAtomicFloatFeaturesEXT:
    shaderBufferFloat32Atomics: VkBool32
    shaderBufferFloat32AtomicAdd: VkBool32
    shaderBufferFloat64Atomics: VkBool32
    shaderBufferFloat64AtomicAdd: VkBool32
    shaderSharedFloat32Atomics: VkBool32
    shaderSharedFloat32AtomicAdd: VkBool32
    shaderSharedFloat64Atomics: VkBool32
    shaderSharedFloat64AtomicAdd: VkBool32
    shaderImageFloat32Atomics: VkBool32
    shaderImageFloat32AtomicAdd: VkBool32
    sparseImageFloat32Atomics: VkBool32
    sparseImageFloat32AtomicAdd: VkBool32

@dataclass
class VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT:
    shaderBufferFloat16Atomics: VkBool32
    shaderBufferFloat16AtomicAdd: VkBool32
    shaderBufferFloat16AtomicMinMax: VkBool32
    shaderBufferFloat32AtomicMinMax: VkBool32
    shaderBufferFloat64AtomicMinMax: VkBool32
    shaderSharedFloat16Atomics: VkBool32
    shaderSharedFloat16AtomicAdd: VkBool32
    shaderSharedFloat16AtomicMinMax: VkBool32
    shaderSharedFloat32AtomicMinMax: VkBool32
    shaderSharedFloat64AtomicMinMax: VkBool32
    shaderImageFloat32AtomicMinMax: VkBool32
    sparseImageFloat32AtomicMinMax: VkBool32

@dataclass
class VkPhysicalDeviceVertexAttributeDivisorFeatures:
    vertexAttributeInstanceRateDivisor: VkBool32
    vertexAttributeInstanceRateZeroDivisor: VkBool32

@dataclass
class VkPhysicalDeviceDepthStencilResolveProperties:
    supportedDepthResolveModes: VkResolveModeFlags
    supportedStencilResolveModes: VkResolveModeFlags
    independentResolveNone: VkBool32
    independentResolve: VkBool32

@dataclass
class VkPhysicalDeviceASTCDecodeFeaturesEXT:
    decodeModeSharedExponent: VkBool32

@dataclass
class VkPhysicalDeviceTransformFeedbackFeaturesEXT:
    transformFeedback: VkBool32
    geometryStreams: VkBool32

@dataclass
class VkPhysicalDeviceTransformFeedbackPropertiesEXT:
    maxTransformFeedbackStreams: uint32_t
    maxTransformFeedbackBuffers: uint32_t
    maxTransformFeedbackBufferSize: VkDeviceSize
    maxTransformFeedbackStreamDataSize: uint32_t
    maxTransformFeedbackBufferDataSize: uint32_t
    maxTransformFeedbackBufferDataStride: uint32_t
    transformFeedbackQueries: VkBool32
    transformFeedbackStreamsLinesTriangles: VkBool32
    transformFeedbackRasterizationStreamSelect: VkBool32
    transformFeedbackDraw: VkBool32

@dataclass
class VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV:
    representativeFragmentTest: VkBool32

@dataclass
class VkPhysicalDeviceExclusiveScissorFeaturesNV:
    exclusiveScissor: VkBool32

@dataclass
class VkPhysicalDeviceCornerSampledImageFeaturesNV:
    cornerSampledImage: VkBool32

@dataclass
class VkPhysicalDeviceComputeShaderDerivativesFeaturesKHR:
    computeDerivativeGroupQuads: VkBool32
    computeDerivativeGroupLinear: VkBool32

@dataclass
class VkPhysicalDeviceComputeShaderDerivativesPropertiesKHR:
    meshAndTaskShaderDerivatives: VkBool32

@dataclass
class VkPhysicalDeviceShaderImageFootprintFeaturesNV:
    imageFootprint: VkBool32

@dataclass
class VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV:
    dedicatedAllocationImageAliasing: VkBool32

@dataclass
class VkPhysicalDeviceCopyMemoryIndirectFeaturesNV:
    indirectCopy: VkBool32

@dataclass
class VkPhysicalDeviceCopyMemoryIndirectPropertiesNV:
    supportedQueues: VkQueueFlags

@dataclass
class VkPhysicalDeviceMemoryDecompressionFeaturesNV:
    memoryDecompression: VkBool32

@dataclass
class VkPhysicalDeviceMemoryDecompressionPropertiesNV:
    decompressionMethods: VkMemoryDecompressionMethodFlagsNV
    maxDecompressionIndirectCount: uint64_t

@dataclass
class VkPhysicalDeviceShadingRateImageFeaturesNV:
    shadingRateImage: VkBool32
    shadingRateCoarseSampleOrder: VkBool32

@dataclass
class VkPhysicalDeviceShadingRateImagePropertiesNV:
    shadingRateTexelSize: VkExtent2D
    shadingRatePaletteSize: uint32_t
    shadingRateMaxCoarseSamples: uint32_t

@dataclass
class VkPhysicalDeviceInvocationMaskFeaturesHUAWEI:
    invocationMask: VkBool32

@dataclass
class VkPhysicalDeviceMeshShaderFeaturesNV:
    taskShader: VkBool32
    meshShader: VkBool32

@dataclass
class VkPhysicalDeviceMeshShaderPropertiesNV:
    maxDrawMeshTasksCount: uint32_t
    maxTaskWorkGroupInvocations: uint32_t
    maxTaskWorkGroupSize: uint32_t * 3
    maxTaskTotalMemorySize: uint32_t
    maxTaskOutputCount: uint32_t
    maxMeshWorkGroupInvocations: uint32_t
    maxMeshWorkGroupSize: uint32_t * 3
    maxMeshTotalMemorySize: uint32_t
    maxMeshOutputVertices: uint32_t
    maxMeshOutputPrimitives: uint32_t
    maxMeshMultiviewViewCount: uint32_t
    meshOutputPerVertexGranularity: uint32_t
    meshOutputPerPrimitiveGranularity: uint32_t

@dataclass
class VkPhysicalDeviceMeshShaderFeaturesEXT:
    taskShader: VkBool32
    meshShader: VkBool32
    multiviewMeshShader: VkBool32
    primitiveFragmentShadingRateMeshShader: VkBool32
    meshShaderQueries: VkBool32

@dataclass
class VkPhysicalDeviceMeshShaderPropertiesEXT:
    maxTaskWorkGroupTotalCount: uint32_t
    maxTaskWorkGroupCount: uint32_t * 3
    maxTaskWorkGroupInvocations: uint32_t
    maxTaskWorkGroupSize: uint32_t * 3
    maxTaskPayloadSize: uint32_t
    maxTaskSharedMemorySize: uint32_t
    maxTaskPayloadAndSharedMemorySize: uint32_t
    maxMeshWorkGroupTotalCount: uint32_t
    maxMeshWorkGroupCount: uint32_t * 3
    maxMeshWorkGroupInvocations: uint32_t
    maxMeshWorkGroupSize: uint32_t * 3
    maxMeshSharedMemorySize: uint32_t
    maxMeshPayloadAndSharedMemorySize: uint32_t
    maxMeshOutputMemorySize: uint32_t
    maxMeshPayloadAndOutputMemorySize: uint32_t
    maxMeshOutputComponents: uint32_t
    maxMeshOutputVertices: uint32_t
    maxMeshOutputPrimitives: uint32_t
    maxMeshOutputLayers: uint32_t
    maxMeshMultiviewViewCount: uint32_t
    meshOutputPerVertexGranularity: uint32_t
    meshOutputPerPrimitiveGranularity: uint32_t
    maxPreferredTaskWorkGroupInvocations: uint32_t
    maxPreferredMeshWorkGroupInvocations: uint32_t
    prefersLocalInvocationVertexOutput: VkBool32
    prefersLocalInvocationPrimitiveOutput: VkBool32
    prefersCompactVertexOutput: VkBool32
    prefersCompactPrimitiveOutput: VkBool32

@dataclass
class VkPhysicalDeviceAccelerationStructureFeaturesKHR:
    accelerationStructure: VkBool32
    accelerationStructureCaptureReplay: VkBool32
    accelerationStructureIndirectBuild: VkBool32
    accelerationStructureHostCommands: VkBool32
    descriptorBindingAccelerationStructureUpdateAfterBind: VkBool32

@dataclass
class VkPhysicalDeviceRayTracingPipelineFeaturesKHR:
    rayTracingPipeline: VkBool32
    rayTracingPipelineShaderGroupHandleCaptureReplay: VkBool32
    rayTracingPipelineShaderGroupHandleCaptureReplayMixed: VkBool32
    rayTracingPipelineTraceRaysIndirect: VkBool32
    rayTraversalPrimitiveCulling: VkBool32

@dataclass
class VkPhysicalDeviceRayQueryFeaturesKHR:
    rayQuery: VkBool32

@dataclass
class VkPhysicalDeviceAccelerationStructurePropertiesKHR:
    maxGeometryCount: uint64_t
    maxInstanceCount: uint64_t
    maxPrimitiveCount: uint64_t
    maxPerStageDescriptorAccelerationStructures: uint32_t
    maxPerStageDescriptorUpdateAfterBindAccelerationStructures: uint32_t
    maxDescriptorSetAccelerationStructures: uint32_t
    maxDescriptorSetUpdateAfterBindAccelerationStructures: uint32_t
    minAccelerationStructureScratchOffsetAlignment: uint32_t

@dataclass
class VkPhysicalDeviceRayTracingPipelinePropertiesKHR:
    shaderGroupHandleSize: uint32_t
    maxRayRecursionDepth: uint32_t
    maxShaderGroupStride: uint32_t
    shaderGroupBaseAlignment: uint32_t
    shaderGroupHandleCaptureReplaySize: uint32_t
    maxRayDispatchInvocationCount: uint32_t
    shaderGroupHandleAlignment: uint32_t
    maxRayHitAttributeSize: uint32_t

@dataclass
class VkPhysicalDeviceRayTracingPropertiesNV:
    shaderGroupHandleSize: uint32_t
    maxRecursionDepth: uint32_t
    maxShaderGroupStride: uint32_t
    shaderGroupBaseAlignment: uint32_t
    maxGeometryCount: uint64_t
    maxInstanceCount: uint64_t
    maxTriangleCount: uint64_t
    maxDescriptorSetAccelerationStructures: uint32_t

@dataclass
class VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR:
    rayTracingMaintenance1: VkBool32
    rayTracingPipelineTraceRaysIndirect2: VkBool32

@dataclass
class VkPhysicalDeviceFragmentDensityMapFeaturesEXT:
    fragmentDensityMap: VkBool32
    fragmentDensityMapDynamic: VkBool32
    fragmentDensityMapNonSubsampledImages: VkBool32

@dataclass
class VkPhysicalDeviceFragmentDensityMap2FeaturesEXT:
    fragmentDensityMapDeferred: VkBool32

@dataclass
class VkPhysicalDeviceFragmentDensityMapOffsetFeaturesEXT:
    fragmentDensityMapOffset: VkBool32

@dataclass
class VkPhysicalDeviceFragmentDensityMapPropertiesEXT:
    minFragmentDensityTexelSize: VkExtent2D
    maxFragmentDensityTexelSize: VkExtent2D
    fragmentDensityInvocations: VkBool32

@dataclass
class VkPhysicalDeviceFragmentDensityMap2PropertiesEXT:
    subsampledLoads: VkBool32
    subsampledCoarseReconstructionEarlyAccess: VkBool32
    maxSubsampledArrayLayers: uint32_t
    maxDescriptorSetSubsampledSamplers: uint32_t

@dataclass
class VkPhysicalDeviceFragmentDensityMapOffsetPropertiesEXT:
    fragmentDensityOffsetGranularity: VkExtent2D

@dataclass
class VkPhysicalDeviceScalarBlockLayoutFeatures:
    scalarBlockLayout: VkBool32

@dataclass
class VkPhysicalDeviceUniformBufferStandardLayoutFeatures:
    uniformBufferStandardLayout: VkBool32

@dataclass
class VkPhysicalDeviceDepthClipEnableFeaturesEXT:
    depthClipEnable: VkBool32

@dataclass
class VkPhysicalDeviceMemoryPriorityFeaturesEXT:
    memoryPriority: VkBool32

@dataclass
class VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT:
    pageableDeviceLocalMemory: VkBool32

@dataclass
class VkPhysicalDeviceBufferDeviceAddressFeatures:
    bufferDeviceAddress: VkBool32
    bufferDeviceAddressCaptureReplay: VkBool32
    bufferDeviceAddressMultiDevice: VkBool32

@dataclass
class VkPhysicalDeviceBufferDeviceAddressFeaturesEXT:
    bufferDeviceAddress: VkBool32
    bufferDeviceAddressCaptureReplay: VkBool32
    bufferDeviceAddressMultiDevice: VkBool32

@dataclass
class VkPhysicalDeviceImagelessFramebufferFeatures:
    imagelessFramebuffer: VkBool32

@dataclass
class VkPhysicalDeviceTextureCompressionASTCHDRFeatures:
    textureCompressionASTC_HDR: VkBool32

@dataclass
class VkPhysicalDeviceCooperativeMatrixFeaturesNV:
    cooperativeMatrix: VkBool32
    cooperativeMatrixRobustBufferAccess: VkBool32

@dataclass
class VkPhysicalDeviceCooperativeMatrixPropertiesNV:
    cooperativeMatrixSupportedStages: VkShaderStageFlags

@dataclass
class VkPhysicalDeviceYcbcrImageArraysFeaturesEXT:
    ycbcrImageArrays: VkBool32

@dataclass
class VkPhysicalDevicePresentBarrierFeaturesNV:
    presentBarrier: VkBool32

@dataclass
class VkPhysicalDevicePerformanceQueryFeaturesKHR:
    performanceCounterQueryPools: VkBool32
    performanceCounterMultipleQueryPools: VkBool32

@dataclass
class VkPhysicalDevicePerformanceQueryPropertiesKHR:
    allowCommandBufferQueryCopies: VkBool32

@dataclass
class VkPhysicalDeviceCoverageReductionModeFeaturesNV:
    coverageReductionMode: VkBool32

@dataclass
class VkPhysicalDeviceShaderIntegerFunctions2FeaturesINTEL:
    shaderIntegerFunctions2: VkBool32

@dataclass
class VkPhysicalDeviceShaderClockFeaturesKHR:
    shaderSubgroupClock: VkBool32
    shaderDeviceClock: VkBool32

@dataclass
class VkPhysicalDeviceIndexTypeUint8Features:
    indexTypeUint8: VkBool32

@dataclass
class VkPhysicalDeviceShaderSMBuiltinsPropertiesNV:
    shaderSMCount: uint32_t
    shaderWarpsPerSM: uint32_t

@dataclass
class VkPhysicalDeviceShaderSMBuiltinsFeaturesNV:
    shaderSMBuiltins: VkBool32

@dataclass
class VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT:
    fragmentShaderSampleInterlock: VkBool32
    fragmentShaderPixelInterlock: VkBool32
    fragmentShaderShadingRateInterlock: VkBool32

@dataclass
class VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures:
    separateDepthStencilLayouts: VkBool32

@dataclass
class VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT:
    primitiveTopologyListRestart: VkBool32
    primitiveTopologyPatchListRestart: VkBool32

@dataclass
class VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR:
    pipelineExecutableInfo: VkBool32

@dataclass
class VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures:
    shaderDemoteToHelperInvocation: VkBool32

@dataclass
class VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT:
    texelBufferAlignment: VkBool32

@dataclass
class VkPhysicalDeviceTexelBufferAlignmentProperties:
    storageTexelBufferOffsetAlignmentBytes: VkDeviceSize
    storageTexelBufferOffsetSingleTexelAlignment: VkBool32
    uniformTexelBufferOffsetAlignmentBytes: VkDeviceSize
    uniformTexelBufferOffsetSingleTexelAlignment: VkBool32

@dataclass
class VkPhysicalDeviceSubgroupSizeControlFeatures:
    subgroupSizeControl: VkBool32
    computeFullSubgroups: VkBool32

@dataclass
class VkPhysicalDeviceSubgroupSizeControlProperties:
    minSubgroupSize: uint32_t
    maxSubgroupSize: uint32_t
    maxComputeWorkgroupSubgroups: uint32_t
    requiredSubgroupSizeStages: VkShaderStageFlags

@dataclass
class VkPhysicalDeviceSubpassShadingPropertiesHUAWEI:
    maxSubpassShadingWorkgroupSizeAspectRatio: uint32_t

@dataclass
class VkPhysicalDeviceClusterCullingShaderPropertiesHUAWEI:
    maxWorkGroupCount: uint32_t * 3
    maxWorkGroupSize: uint32_t * 3
    maxOutputClusterCount: uint32_t
    indirectBufferOffsetAlignment: VkDeviceSize

@dataclass
class VkPhysicalDeviceLineRasterizationFeatures:
    rectangularLines: VkBool32
    bresenhamLines: VkBool32
    smoothLines: VkBool32
    stippledRectangularLines: VkBool32
    stippledBresenhamLines: VkBool32
    stippledSmoothLines: VkBool32

@dataclass
class VkPhysicalDeviceLineRasterizationProperties:
    lineSubPixelPrecisionBits: uint32_t

@dataclass
class VkPhysicalDevicePipelineCreationCacheControlFeatures:
    pipelineCreationCacheControl: VkBool32

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
class VkPhysicalDeviceVulkan11Properties:
    deviceUUID: uint8_t * VK_UUID_SIZE
    driverUUID: uint8_t * VK_UUID_SIZE
    deviceLUID: uint8_t * VK_LUID_SIZE
    deviceNodeMask: uint32_t
    deviceLUIDValid: VkBool32
    subgroupSize: uint32_t
    subgroupSupportedStages: VkShaderStageFlags
    subgroupSupportedOperations: VkSubgroupFeatureFlags
    subgroupQuadOperationsInAllStages: VkBool32
    pointClippingBehavior: VkPointClippingBehavior
    maxMultiviewViewCount: uint32_t
    maxMultiviewInstanceIndex: uint32_t
    protectedNoFault: VkBool32
    maxPerSetDescriptors: uint32_t
    maxMemoryAllocationSize: VkDeviceSize

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
class VkPhysicalDeviceVulkan12Properties:
    driverID: VkDriverId
    driverName: str
    driverInfo: str
    conformanceVersion: VkConformanceVersion
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
    defaultRobustnessImages: VkPipelineRobustnessImageBehavior
    copySrcLayoutCount: uint32_t
    pCopySrcLayouts: List[VkImageLayout]
    copyDstLayoutCount: uint32_t
    pCopyDstLayouts: List[VkImageLayout]
    optimalTilingLayoutUUID: uint8_t * VK_UUID_SIZE
    identicalMemoryTypeRequirements: VkBool32

@dataclass
class VkPhysicalDeviceCoherentMemoryFeaturesAMD:
    deviceCoherentMemory: VkBool32

@dataclass
class VkPhysicalDeviceToolProperties:
    name: str
    version: str
    purposes: VkToolPurposeFlags
    description: str
    layer: str

@dataclass
class VkPhysicalDeviceCustomBorderColorPropertiesEXT:
    maxCustomBorderColorSamplers: uint32_t

@dataclass
class VkPhysicalDeviceCustomBorderColorFeaturesEXT:
    customBorderColors: VkBool32
    customBorderColorWithoutFormat: VkBool32

@dataclass
class VkPhysicalDeviceBorderColorSwizzleFeaturesEXT:
    borderColorSwizzle: VkBool32
    borderColorSwizzleFromImage: VkBool32

@dataclass
class VkPhysicalDeviceExtendedDynamicStateFeaturesEXT:
    extendedDynamicState: VkBool32

@dataclass
class VkPhysicalDeviceExtendedDynamicState2FeaturesEXT:
    extendedDynamicState2: VkBool32
    extendedDynamicState2LogicOp: VkBool32
    extendedDynamicState2PatchControlPoints: VkBool32

@dataclass
class VkPhysicalDeviceExtendedDynamicState3FeaturesEXT:
    extendedDynamicState3TessellationDomainOrigin: VkBool32
    extendedDynamicState3DepthClampEnable: VkBool32
    extendedDynamicState3PolygonMode: VkBool32
    extendedDynamicState3RasterizationSamples: VkBool32
    extendedDynamicState3SampleMask: VkBool32
    extendedDynamicState3AlphaToCoverageEnable: VkBool32
    extendedDynamicState3AlphaToOneEnable: VkBool32
    extendedDynamicState3LogicOpEnable: VkBool32
    extendedDynamicState3ColorBlendEnable: VkBool32
    extendedDynamicState3ColorBlendEquation: VkBool32
    extendedDynamicState3ColorWriteMask: VkBool32
    extendedDynamicState3RasterizationStream: VkBool32
    extendedDynamicState3ConservativeRasterizationMode: VkBool32
    extendedDynamicState3ExtraPrimitiveOverestimationSize: VkBool32
    extendedDynamicState3DepthClipEnable: VkBool32
    extendedDynamicState3SampleLocationsEnable: VkBool32
    extendedDynamicState3ColorBlendAdvanced: VkBool32
    extendedDynamicState3ProvokingVertexMode: VkBool32
    extendedDynamicState3LineRasterizationMode: VkBool32
    extendedDynamicState3LineStippleEnable: VkBool32
    extendedDynamicState3DepthClipNegativeOneToOne: VkBool32
    extendedDynamicState3ViewportWScalingEnable: VkBool32
    extendedDynamicState3ViewportSwizzle: VkBool32
    extendedDynamicState3CoverageToColorEnable: VkBool32
    extendedDynamicState3CoverageToColorLocation: VkBool32
    extendedDynamicState3CoverageModulationMode: VkBool32
    extendedDynamicState3CoverageModulationTableEnable: VkBool32
    extendedDynamicState3CoverageModulationTable: VkBool32
    extendedDynamicState3CoverageReductionMode: VkBool32
    extendedDynamicState3RepresentativeFragmentTestEnable: VkBool32
    extendedDynamicState3ShadingRateImageEnable: VkBool32

@dataclass
class VkPhysicalDeviceExtendedDynamicState3PropertiesEXT:
    dynamicPrimitiveTopologyUnrestricted: VkBool32

@dataclass
class VkPhysicalDevicePartitionedAccelerationStructureFeaturesNV:
    partitionedAccelerationStructure: VkBool32

@dataclass
class VkPhysicalDevicePartitionedAccelerationStructurePropertiesNV:
    maxPartitionCount: uint32_t

@dataclass
class VkPhysicalDeviceDiagnosticsConfigFeaturesNV:
    diagnosticsConfig: VkBool32

@dataclass
class VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures:
    shaderZeroInitializeWorkgroupMemory: VkBool32

@dataclass
class VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR:
    shaderSubgroupUniformControlFlow: VkBool32

@dataclass
class VkPhysicalDeviceRobustness2FeaturesKHR:
    robustBufferAccess2: VkBool32
    robustImageAccess2: VkBool32
    nullDescriptor: VkBool32

@dataclass
class VkPhysicalDeviceRobustness2PropertiesKHR:
    robustStorageBufferAccessSizeAlignment: VkDeviceSize
    robustUniformBufferAccessSizeAlignment: VkDeviceSize

@dataclass
class VkPhysicalDeviceImageRobustnessFeatures:
    robustImageAccess: VkBool32

@dataclass
class VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR:
    workgroupMemoryExplicitLayout: VkBool32
    workgroupMemoryExplicitLayoutScalarBlockLayout: VkBool32
    workgroupMemoryExplicitLayout8BitAccess: VkBool32
    workgroupMemoryExplicitLayout16BitAccess: VkBool32

@dataclass
class VkPhysicalDevice4444FormatsFeaturesEXT:
    formatA4R4G4B4: VkBool32
    formatA4B4G4R4: VkBool32

@dataclass
class VkPhysicalDeviceSubpassShadingFeaturesHUAWEI:
    subpassShading: VkBool32

@dataclass
class VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI:
    clustercullingShader: VkBool32
    multiviewClusterCullingShader: VkBool32

@dataclass
class VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT:
    shaderImageInt64Atomics: VkBool32
    sparseImageInt64Atomics: VkBool32

@dataclass
class VkPhysicalDeviceFragmentShadingRateFeaturesKHR:
    pipelineFragmentShadingRate: VkBool32
    primitiveFragmentShadingRate: VkBool32
    attachmentFragmentShadingRate: VkBool32

@dataclass
class VkPhysicalDeviceFragmentShadingRatePropertiesKHR:
    minFragmentShadingRateAttachmentTexelSize: VkExtent2D
    maxFragmentShadingRateAttachmentTexelSize: VkExtent2D
    maxFragmentShadingRateAttachmentTexelSizeAspectRatio: uint32_t
    primitiveFragmentShadingRateWithMultipleViewports: VkBool32
    layeredShadingRateAttachments: VkBool32
    fragmentShadingRateNonTrivialCombinerOps: VkBool32
    maxFragmentSize: VkExtent2D
    maxFragmentSizeAspectRatio: uint32_t
    maxFragmentShadingRateCoverageSamples: uint32_t
    maxFragmentShadingRateRasterizationSamples: VkSampleCountFlagBits
    fragmentShadingRateWithShaderDepthStencilWrites: VkBool32
    fragmentShadingRateWithSampleMask: VkBool32
    fragmentShadingRateWithShaderSampleMask: VkBool32
    fragmentShadingRateWithConservativeRasterization: VkBool32
    fragmentShadingRateWithFragmentShaderInterlock: VkBool32
    fragmentShadingRateWithCustomSampleLocations: VkBool32
    fragmentShadingRateStrictMultiplyCombiner: VkBool32

@dataclass
class VkPhysicalDeviceShaderTerminateInvocationFeatures:
    shaderTerminateInvocation: VkBool32

@dataclass
class VkPhysicalDeviceFragmentShadingRateEnumsFeaturesNV:
    fragmentShadingRateEnums: VkBool32
    supersampleFragmentShadingRates: VkBool32
    noInvocationFragmentShadingRates: VkBool32

@dataclass
class VkPhysicalDeviceFragmentShadingRateEnumsPropertiesNV:
    maxFragmentShadingRateInvocationCount: VkSampleCountFlagBits

@dataclass
class VkPhysicalDeviceImage2DViewOf3DFeaturesEXT:
    image2DViewOf3D: VkBool32
    sampler2DViewOf3D: VkBool32

@dataclass
class VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT:
    imageSlicedViewOf3D: VkBool32

@dataclass
class VkPhysicalDeviceAttachmentFeedbackLoopDynamicStateFeaturesEXT:
    attachmentFeedbackLoopDynamicState: VkBool32

@dataclass
class VkPhysicalDeviceLegacyVertexAttributesFeaturesEXT:
    legacyVertexAttributes: VkBool32

@dataclass
class VkPhysicalDeviceLegacyVertexAttributesPropertiesEXT:
    nativeUnalignedPerformance: VkBool32

@dataclass
class VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT:
    mutableDescriptorType: VkBool32

@dataclass
class VkPhysicalDeviceDepthClipControlFeaturesEXT:
    depthClipControl: VkBool32

@dataclass
class VkPhysicalDeviceZeroInitializeDeviceMemoryFeaturesEXT:
    zeroInitializeDeviceMemory: VkBool32

@dataclass
class VkPhysicalDeviceDeviceGeneratedCommandsFeaturesEXT:
    deviceGeneratedCommands: VkBool32
    dynamicGeneratedPipelineLayout: VkBool32

@dataclass
class VkPhysicalDeviceDeviceGeneratedCommandsPropertiesEXT:
    maxIndirectPipelineCount: uint32_t
    maxIndirectShaderObjectCount: uint32_t
    maxIndirectSequenceCount: uint32_t
    maxIndirectCommandsTokenCount: uint32_t
    maxIndirectCommandsTokenOffset: uint32_t
    maxIndirectCommandsIndirectStride: uint32_t
    supportedIndirectCommandsInputModes: VkIndirectCommandsInputModeFlagsEXT
    supportedIndirectCommandsShaderStages: VkShaderStageFlags
    supportedIndirectCommandsShaderStagesPipelineBinding: VkShaderStageFlags
    supportedIndirectCommandsShaderStagesShaderBinding: VkShaderStageFlags
    deviceGeneratedCommandsTransformFeedback: VkBool32
    deviceGeneratedCommandsMultiDrawIndirectCount: VkBool32

@dataclass
class VkPhysicalDeviceDepthClampControlFeaturesEXT:
    depthClampControl: VkBool32

@dataclass
class VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT:
    vertexInputDynamicState: VkBool32

@dataclass
class VkPhysicalDeviceExternalMemoryRDMAFeaturesNV:
    externalMemoryRDMA: VkBool32

@dataclass
class VkPhysicalDeviceShaderRelaxedExtendedInstructionFeaturesKHR:
    shaderRelaxedExtendedInstruction: VkBool32

@dataclass
class VkPhysicalDeviceColorWriteEnableFeaturesEXT:
    colorWriteEnable: VkBool32

@dataclass
class VkPhysicalDeviceSynchronization2Features:
    synchronization2: VkBool32

@dataclass
class VkPhysicalDeviceUnifiedImageLayoutsFeaturesKHR:
    unifiedImageLayouts: VkBool32
    unifiedImageLayoutsVideo: VkBool32

@dataclass
class VkPhysicalDeviceHostImageCopyFeatures:
    hostImageCopy: VkBool32

@dataclass
class VkPhysicalDeviceHostImageCopyProperties:
    copySrcLayoutCount: uint32_t
    pCopySrcLayouts: List[VkImageLayout]
    copyDstLayoutCount: uint32_t
    pCopyDstLayouts: List[VkImageLayout]
    optimalTilingLayoutUUID: uint8_t * VK_UUID_SIZE
    identicalMemoryTypeRequirements: VkBool32

@dataclass
class VkPhysicalDevicePrimitivesGeneratedQueryFeaturesEXT:
    primitivesGeneratedQuery: VkBool32
    primitivesGeneratedQueryWithRasterizerDiscard: VkBool32
    primitivesGeneratedQueryWithNonZeroStreams: VkBool32

@dataclass
class VkPhysicalDeviceLegacyDitheringFeaturesEXT:
    legacyDithering: VkBool32

@dataclass
class VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT:
    multisampledRenderToSingleSampled: VkBool32

@dataclass
class VkPhysicalDevicePipelineProtectedAccessFeatures:
    pipelineProtectedAccess: VkBool32

@dataclass
class VkPhysicalDeviceVideoMaintenance1FeaturesKHR:
    videoMaintenance1: VkBool32

@dataclass
class VkPhysicalDeviceVideoMaintenance2FeaturesKHR:
    videoMaintenance2: VkBool32

@dataclass
class VkPhysicalDeviceVideoDecodeVP9FeaturesKHR:
    videoDecodeVP9: VkBool32

@dataclass
class VkPhysicalDeviceVideoEncodeQuantizationMapFeaturesKHR:
    videoEncodeQuantizationMap: VkBool32

@dataclass
class VkPhysicalDeviceVideoEncodeAV1FeaturesKHR:
    videoEncodeAV1: VkBool32

@dataclass
class VkPhysicalDeviceInheritedViewportScissorFeaturesNV:
    inheritedViewportScissor2D: VkBool32

@dataclass
class VkPhysicalDeviceYcbcr2Plane444FormatsFeaturesEXT:
    ycbcr2plane444Formats: VkBool32

@dataclass
class VkPhysicalDeviceProvokingVertexFeaturesEXT:
    provokingVertexLast: VkBool32
    transformFeedbackPreservesProvokingVertex: VkBool32

@dataclass
class VkPhysicalDeviceProvokingVertexPropertiesEXT:
    provokingVertexModePerPipeline: VkBool32
    transformFeedbackPreservesTriangleFanProvokingVertex: VkBool32

@dataclass
class VkPhysicalDeviceDescriptorBufferFeaturesEXT:
    descriptorBuffer: VkBool32
    descriptorBufferCaptureReplay: VkBool32
    descriptorBufferImageLayoutIgnored: VkBool32
    descriptorBufferPushDescriptors: VkBool32

@dataclass
class VkPhysicalDeviceDescriptorBufferPropertiesEXT:
    combinedImageSamplerDescriptorSingleArray: VkBool32
    bufferlessPushDescriptors: VkBool32
    allowSamplerImageViewPostSubmitCreation: VkBool32
    descriptorBufferOffsetAlignment: VkDeviceSize
    maxDescriptorBufferBindings: uint32_t
    maxResourceDescriptorBufferBindings: uint32_t
    maxSamplerDescriptorBufferBindings: uint32_t
    maxEmbeddedImmutableSamplerBindings: uint32_t
    maxEmbeddedImmutableSamplers: uint32_t
    bufferCaptureReplayDescriptorDataSize: size_t
    imageCaptureReplayDescriptorDataSize: size_t
    imageViewCaptureReplayDescriptorDataSize: size_t
    samplerCaptureReplayDescriptorDataSize: size_t
    accelerationStructureCaptureReplayDescriptorDataSize: size_t
    samplerDescriptorSize: size_t
    combinedImageSamplerDescriptorSize: size_t
    sampledImageDescriptorSize: size_t
    storageImageDescriptorSize: size_t
    uniformTexelBufferDescriptorSize: size_t
    robustUniformTexelBufferDescriptorSize: size_t
    storageTexelBufferDescriptorSize: size_t
    robustStorageTexelBufferDescriptorSize: size_t
    uniformBufferDescriptorSize: size_t
    robustUniformBufferDescriptorSize: size_t
    storageBufferDescriptorSize: size_t
    robustStorageBufferDescriptorSize: size_t
    inputAttachmentDescriptorSize: size_t
    accelerationStructureDescriptorSize: size_t
    maxSamplerDescriptorBufferRange: VkDeviceSize
    maxResourceDescriptorBufferRange: VkDeviceSize
    samplerDescriptorBufferAddressSpaceSize: VkDeviceSize
    resourceDescriptorBufferAddressSpaceSize: VkDeviceSize
    descriptorBufferAddressSpaceSize: VkDeviceSize

@dataclass
class VkPhysicalDeviceDescriptorBufferDensityMapPropertiesEXT:
    combinedImageSamplerDensityMapDescriptorSize: size_t

@dataclass
class VkPhysicalDeviceShaderIntegerDotProductFeatures:
    shaderIntegerDotProduct: VkBool32

@dataclass
class VkPhysicalDeviceShaderIntegerDotProductProperties:
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

@dataclass
class VkPhysicalDeviceDrmPropertiesEXT:
    hasPrimary: VkBool32
    hasRender: VkBool32
    primaryMajor: int64_t
    primaryMinor: int64_t
    renderMajor: int64_t
    renderMinor: int64_t

@dataclass
class VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR:
    fragmentShaderBarycentric: VkBool32

@dataclass
class VkPhysicalDeviceFragmentShaderBarycentricPropertiesKHR:
    triStripVertexOrderIndependentOfProvokingVertex: VkBool32

@dataclass
class VkPhysicalDeviceRayTracingMotionBlurFeaturesNV:
    rayTracingMotionBlur: VkBool32
    rayTracingMotionBlurPipelineTraceRaysIndirect: VkBool32

@dataclass
class VkPhysicalDeviceRayTracingValidationFeaturesNV:
    rayTracingValidation: VkBool32

@dataclass
class VkPhysicalDeviceRayTracingLinearSweptSpheresFeaturesNV:
    spheres: VkBool32
    linearSweptSpheres: VkBool32

@dataclass
class VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT:
    formatRgba10x6WithoutYCbCrSampler: VkBool32

@dataclass
class VkPhysicalDeviceDynamicRenderingFeatures:
    dynamicRendering: VkBool32

@dataclass
class VkPhysicalDeviceImageViewMinLodFeaturesEXT:
    minLod: VkBool32

@dataclass
class VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT:
    rasterizationOrderColorAttachmentAccess: VkBool32
    rasterizationOrderDepthAttachmentAccess: VkBool32
    rasterizationOrderStencilAttachmentAccess: VkBool32

@dataclass
class VkPhysicalDeviceLinearColorAttachmentFeaturesNV:
    linearColorAttachment: VkBool32

@dataclass
class VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT:
    graphicsPipelineLibrary: VkBool32

@dataclass
class VkPhysicalDevicePipelineBinaryFeaturesKHR:
    pipelineBinaries: VkBool32

@dataclass
class VkPhysicalDevicePipelineBinaryPropertiesKHR:
    pipelineBinaryInternalCache: VkBool32
    pipelineBinaryInternalCacheControl: VkBool32
    pipelineBinaryPrefersInternalCache: VkBool32
    pipelineBinaryPrecompiledInternalCache: VkBool32
    pipelineBinaryCompressedData: VkBool32

@dataclass
class VkPhysicalDeviceGraphicsPipelineLibraryPropertiesEXT:
    graphicsPipelineLibraryFastLinking: VkBool32
    graphicsPipelineLibraryIndependentInterpolationDecoration: VkBool32

@dataclass
class VkPhysicalDeviceDescriptorSetHostMappingFeaturesVALVE:
    descriptorSetHostMapping: VkBool32

@dataclass
class VkPhysicalDeviceNestedCommandBufferFeaturesEXT:
    nestedCommandBuffer: VkBool32
    nestedCommandBufferRendering: VkBool32
    nestedCommandBufferSimultaneousUse: VkBool32

@dataclass
class VkPhysicalDeviceNestedCommandBufferPropertiesEXT:
    maxCommandBufferNestingLevel: uint32_t

@dataclass
class VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT:
    shaderModuleIdentifier: VkBool32

@dataclass
class VkPhysicalDeviceShaderModuleIdentifierPropertiesEXT:
    shaderModuleIdentifierAlgorithmUUID: uint8_t * VK_UUID_SIZE

@dataclass
class VkPhysicalDeviceImageCompressionControlFeaturesEXT:
    imageCompressionControl: VkBool32

@dataclass
class VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT:
    imageCompressionControlSwapchain: VkBool32

@dataclass
class VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT:
    subpassMergeFeedback: VkBool32

@dataclass
class VkPhysicalDeviceOpacityMicromapFeaturesEXT:
    micromap: VkBool32
    micromapCaptureReplay: VkBool32
    micromapHostCommands: VkBool32

@dataclass
class VkPhysicalDeviceOpacityMicromapPropertiesEXT:
    maxOpacity2StateSubdivisionLevel: uint32_t
    maxOpacity4StateSubdivisionLevel: uint32_t

@dataclass
class VkPhysicalDevicePipelinePropertiesFeaturesEXT:
    pipelinePropertiesIdentifier: VkBool32

@dataclass
class VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD:
    shaderEarlyAndLateFragmentTests: VkBool32

@dataclass
class VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT:
    nonSeamlessCubeMap: VkBool32

@dataclass
class VkPhysicalDevicePipelineRobustnessFeatures:
    pipelineRobustness: VkBool32

@dataclass
class VkPhysicalDevicePipelineRobustnessProperties:
    defaultRobustnessStorageBuffers: VkPipelineRobustnessBufferBehavior
    defaultRobustnessUniformBuffers: VkPipelineRobustnessBufferBehavior
    defaultRobustnessVertexInputs: VkPipelineRobustnessBufferBehavior
    defaultRobustnessImages: VkPipelineRobustnessImageBehavior

@dataclass
class VkPhysicalDeviceImageProcessingFeaturesQCOM:
    textureSampleWeighted: VkBool32
    textureBoxFilter: VkBool32
    textureBlockMatch: VkBool32

@dataclass
class VkPhysicalDeviceImageProcessingPropertiesQCOM:
    maxWeightFilterPhases: uint32_t
    maxWeightFilterDimension: VkExtent2D
    maxBlockMatchRegion: VkExtent2D
    maxBoxFilterBlockSize: VkExtent2D

@dataclass
class VkPhysicalDeviceTilePropertiesFeaturesQCOM:
    tileProperties: VkBool32

@dataclass
class VkPhysicalDeviceAmigoProfilingFeaturesSEC:
    amigoProfiling: VkBool32

@dataclass
class VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT:
    attachmentFeedbackLoopLayout: VkBool32

@dataclass
class VkPhysicalDeviceAddressBindingReportFeaturesEXT:
    reportAddressBinding: VkBool32

@dataclass
class VkPhysicalDeviceOpticalFlowFeaturesNV:
    opticalFlow: VkBool32

@dataclass
class VkPhysicalDeviceOpticalFlowPropertiesNV:
    supportedOutputGridSizes: VkOpticalFlowGridSizeFlagsNV
    supportedHintGridSizes: VkOpticalFlowGridSizeFlagsNV
    hintSupported: VkBool32
    costSupported: VkBool32
    bidirectionalFlowSupported: VkBool32
    globalFlowSupported: VkBool32
    minWidth: uint32_t
    minHeight: uint32_t
    maxWidth: uint32_t
    maxHeight: uint32_t
    maxNumRegionsOfInterest: uint32_t

@dataclass
class VkPhysicalDeviceFaultFeaturesEXT:
    deviceFault: VkBool32
    deviceFaultVendorBinary: VkBool32

@dataclass
class VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT:
    pipelineLibraryGroupHandles: VkBool32

@dataclass
class VkPhysicalDeviceShaderCoreBuiltinsPropertiesARM:
    shaderCoreMask: uint64_t
    shaderCoreCount: uint32_t
    shaderWarpsPerCore: uint32_t

@dataclass
class VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM:
    shaderCoreBuiltins: VkBool32

@dataclass
class VkPhysicalDeviceFrameBoundaryFeaturesEXT:
    frameBoundary: VkBool32

@dataclass
class VkPhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT:
    dynamicRenderingUnusedAttachments: VkBool32

@dataclass
class VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT:
    swapchainMaintenance1: VkBool32

@dataclass
class VkPhysicalDeviceDepthBiasControlFeaturesEXT:
    depthBiasControl: VkBool32
    leastRepresentableValueForceUnormRepresentation: VkBool32
    floatRepresentation: VkBool32
    depthBiasExact: VkBool32

@dataclass
class VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV:
    rayTracingInvocationReorder: VkBool32

@dataclass
class VkPhysicalDeviceRayTracingInvocationReorderPropertiesNV:
    rayTracingInvocationReorderReorderingHint: VkRayTracingInvocationReorderModeNV

@dataclass
class VkPhysicalDeviceExtendedSparseAddressSpaceFeaturesNV:
    extendedSparseAddressSpace: VkBool32

@dataclass
class VkPhysicalDeviceExtendedSparseAddressSpacePropertiesNV:
    extendedSparseAddressSpaceSize: VkDeviceSize
    extendedSparseImageUsageFlags: VkImageUsageFlags
    extendedSparseBufferUsageFlags: VkBufferUsageFlags

@dataclass
class VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM:
    multiviewPerViewViewports: VkBool32

@dataclass
class VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR:
    rayTracingPositionFetch: VkBool32

@dataclass
class VkPhysicalDeviceShaderCorePropertiesARM:
    pixelRate: uint32_t
    texelRate: uint32_t
    fmaRate: uint32_t

@dataclass
class VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM:
    multiviewPerViewRenderAreas: VkBool32

@dataclass
class VkPhysicalDeviceShaderObjectFeaturesEXT:
    shaderObject: VkBool32

@dataclass
class VkPhysicalDeviceShaderObjectPropertiesEXT:
    shaderBinaryUUID: uint8_t * VK_UUID_SIZE
    shaderBinaryVersion: uint32_t

@dataclass
class VkPhysicalDeviceShaderTileImageFeaturesEXT:
    shaderTileImageColorReadAccess: VkBool32
    shaderTileImageDepthReadAccess: VkBool32
    shaderTileImageStencilReadAccess: VkBool32

@dataclass
class VkPhysicalDeviceShaderTileImagePropertiesEXT:
    shaderTileImageCoherentReadAccelerated: VkBool32
    shaderTileImageReadSampleFromPixelRateInvocation: VkBool32
    shaderTileImageReadFromHelperInvocation: VkBool32

@dataclass
class VkPhysicalDeviceCooperativeMatrixFeaturesKHR:
    cooperativeMatrix: VkBool32
    cooperativeMatrixRobustBufferAccess: VkBool32

@dataclass
class VkPhysicalDeviceCooperativeMatrixPropertiesKHR:
    cooperativeMatrixSupportedStages: VkShaderStageFlags

@dataclass
class VkPhysicalDeviceAntiLagFeaturesAMD:
    antiLag: VkBool32

@dataclass
class VkPhysicalDeviceTileMemoryHeapFeaturesQCOM:
    tileMemoryHeap: VkBool32

@dataclass
class VkPhysicalDeviceTileMemoryHeapPropertiesQCOM:
    queueSubmitBoundary: VkBool32
    tileBufferTransfers: VkBool32

@dataclass
class VkPhysicalDeviceCubicClampFeaturesQCOM:
    cubicRangeClamp: VkBool32

@dataclass
class VkPhysicalDeviceYcbcrDegammaFeaturesQCOM:
    ycbcrDegamma: VkBool32

@dataclass
class VkPhysicalDeviceCubicWeightsFeaturesQCOM:
    selectableCubicWeights: VkBool32

@dataclass
class VkPhysicalDeviceImageProcessing2FeaturesQCOM:
    textureBlockMatch2: VkBool32

@dataclass
class VkPhysicalDeviceImageProcessing2PropertiesQCOM:
    maxBlockMatchWindow: VkExtent2D

@dataclass
class VkPhysicalDeviceDescriptorPoolOverallocationFeaturesNV:
    descriptorPoolOverallocation: VkBool32

@dataclass
class VkPhysicalDeviceLayeredDriverPropertiesMSFT:
    underlyingAPI: VkLayeredDriverUnderlyingApiMSFT

@dataclass
class VkPhysicalDevicePerStageDescriptorSetFeaturesNV:
    perStageDescriptorSet: VkBool32
    dynamicPipelineLayout: VkBool32

@dataclass
class VkPhysicalDeviceExternalFormatResolveFeaturesANDROID:
    externalFormatResolve: VkBool32

@dataclass
class VkPhysicalDeviceExternalFormatResolvePropertiesANDROID:
    nullColorAttachmentWithExternalFormatResolve: VkBool32
    externalFormatResolveChromaOffsetX: VkChromaLocation
    externalFormatResolveChromaOffsetY: VkChromaLocation

@dataclass
class VkPhysicalDeviceSchedulingControlsFeaturesARM:
    schedulingControls: VkBool32

@dataclass
class VkPhysicalDeviceSchedulingControlsPropertiesARM:
    schedulingControlsFlags: VkPhysicalDeviceSchedulingControlsFlagsARM

@dataclass
class VkPhysicalDeviceRelaxedLineRasterizationFeaturesIMG:
    relaxedLineRasterization: VkBool32

@dataclass
class VkPhysicalDeviceRenderPassStripedFeaturesARM:
    renderPassStriped: VkBool32

@dataclass
class VkPhysicalDeviceRenderPassStripedPropertiesARM:
    renderPassStripeGranularity: VkExtent2D
    maxRenderPassStripes: uint32_t

@dataclass
class VkPhysicalDevicePipelineOpacityMicromapFeaturesARM:
    pipelineOpacityMicromap: VkBool32

@dataclass
class VkPhysicalDeviceShaderMaximalReconvergenceFeaturesKHR:
    shaderMaximalReconvergence: VkBool32

@dataclass
class VkPhysicalDeviceShaderSubgroupRotateFeatures:
    shaderSubgroupRotate: VkBool32
    shaderSubgroupRotateClustered: VkBool32

@dataclass
class VkPhysicalDeviceShaderExpectAssumeFeatures:
    shaderExpectAssume: VkBool32

@dataclass
class VkPhysicalDeviceShaderFloatControls2Features:
    shaderFloatControls2: VkBool32

@dataclass
class VkPhysicalDeviceDynamicRenderingLocalReadFeatures:
    dynamicRenderingLocalRead: VkBool32

@dataclass
class VkPhysicalDeviceShaderQuadControlFeaturesKHR:
    shaderQuadControl: VkBool32

@dataclass
class VkPhysicalDeviceShaderAtomicFloat16VectorFeaturesNV:
    shaderFloat16VectorAtomics: VkBool32

@dataclass
class VkPhysicalDeviceMapMemoryPlacedFeaturesEXT:
    memoryMapPlaced: VkBool32
    memoryMapRangePlaced: VkBool32
    memoryUnmapReserve: VkBool32

@dataclass
class VkPhysicalDeviceMapMemoryPlacedPropertiesEXT:
    minPlacedMemoryMapAlignment: VkDeviceSize

@dataclass
class VkPhysicalDeviceShaderBfloat16FeaturesKHR:
    shaderBFloat16Type: VkBool32
    shaderBFloat16DotProduct: VkBool32
    shaderBFloat16CooperativeMatrix: VkBool32

@dataclass
class VkPhysicalDeviceRawAccessChainsFeaturesNV:
    shaderRawAccessChains: VkBool32

@dataclass
class VkPhysicalDeviceCommandBufferInheritanceFeaturesNV:
    commandBufferInheritance: VkBool32

@dataclass
class VkPhysicalDeviceImageAlignmentControlFeaturesMESA:
    imageAlignmentControl: VkBool32

@dataclass
class VkPhysicalDeviceImageAlignmentControlPropertiesMESA:
    supportedImageAlignmentMask: uint32_t

@dataclass
class VkPhysicalDeviceShaderReplicatedCompositesFeaturesEXT:
    shaderReplicatedComposites: VkBool32

@dataclass
class VkPhysicalDevicePresentModeFifoLatestReadyFeaturesEXT:
    presentModeFifoLatestReady: VkBool32

@dataclass
class VkPhysicalDeviceCooperativeMatrix2FeaturesNV:
    cooperativeMatrixWorkgroupScope: VkBool32
    cooperativeMatrixFlexibleDimensions: VkBool32
    cooperativeMatrixReductions: VkBool32
    cooperativeMatrixConversions: VkBool32
    cooperativeMatrixPerElementOperations: VkBool32
    cooperativeMatrixTensorAddressing: VkBool32
    cooperativeMatrixBlockLoads: VkBool32

@dataclass
class VkPhysicalDeviceCooperativeMatrix2PropertiesNV:
    cooperativeMatrixWorkgroupScopeMaxWorkgroupSize: uint32_t
    cooperativeMatrixFlexibleDimensionsMaxDimension: uint32_t
    cooperativeMatrixWorkgroupScopeReservedSharedMemory: uint32_t

@dataclass
class VkPhysicalDeviceHdrVividFeaturesHUAWEI:
    hdrVivid: VkBool32

@dataclass
class VkPhysicalDeviceVertexAttributeRobustnessFeaturesEXT:
    vertexAttributeRobustness: VkBool32

@dataclass
class VkPhysicalDeviceDepthClampZeroOneFeaturesKHR:
    depthClampZeroOne: VkBool32

@dataclass
class VkPhysicalDeviceCooperativeVectorFeaturesNV:
    cooperativeVector: VkBool32
    cooperativeVectorTraining: VkBool32

@dataclass
class VkPhysicalDeviceCooperativeVectorPropertiesNV:
    cooperativeVectorSupportedStages: VkShaderStageFlags
    cooperativeVectorTrainingFloat16Accumulation: VkBool32
    cooperativeVectorTrainingFloat32Accumulation: VkBool32
    maxCooperativeVectorComponents: uint32_t

@dataclass
class VkPhysicalDeviceTileShadingFeaturesQCOM:
    tileShading: VkBool32
    tileShadingFragmentStage: VkBool32
    tileShadingColorAttachments: VkBool32
    tileShadingDepthAttachments: VkBool32
    tileShadingStencilAttachments: VkBool32
    tileShadingInputAttachments: VkBool32
    tileShadingSampledAttachments: VkBool32
    tileShadingPerTileDraw: VkBool32
    tileShadingPerTileDispatch: VkBool32
    tileShadingDispatchTile: VkBool32
    tileShadingApron: VkBool32
    tileShadingAnisotropicApron: VkBool32
    tileShadingAtomicOps: VkBool32
    tileShadingImageProcessing: VkBool32

@dataclass
class VkPhysicalDeviceTileShadingPropertiesQCOM:
    maxApronSize: uint32_t
    preferNonCoherent: VkBool32
    tileGranularity: VkExtent2D
    maxTileShadingRate: VkExtent2D

@dataclass
class VkPhysicalDeviceExternalComputeQueuePropertiesNV:
    externalDataSize: uint32_t
    maxExternalQueues: uint32_t

@dataclass
class VkPhysicalDeviceFormatPackFeaturesARM:
    formatPack: VkBool32

@dataclass
class VkPhysicalDeviceTensorPropertiesARM:
    maxTensorDimensionCount: uint32_t
    maxTensorElements: uint64_t
    maxPerDimensionTensorElements: uint64_t
    maxTensorStride: int64_t
    maxTensorSize: uint64_t
    maxTensorShaderAccessArrayLength: uint32_t
    maxTensorShaderAccessSize: uint32_t
    maxDescriptorSetStorageTensors: uint32_t
    maxPerStageDescriptorSetStorageTensors: uint32_t
    maxDescriptorSetUpdateAfterBindStorageTensors: uint32_t
    maxPerStageDescriptorUpdateAfterBindStorageTensors: uint32_t
    shaderStorageTensorArrayNonUniformIndexingNative: VkBool32
    shaderTensorSupportedStages: VkShaderStageFlags

@dataclass
class VkPhysicalDeviceTensorFeaturesARM:
    tensorNonPacked: VkBool32
    shaderTensorAccess: VkBool32
    shaderStorageTensorArrayDynamicIndexing: VkBool32
    shaderStorageTensorArrayNonUniformIndexing: VkBool32
    descriptorBindingStorageTensorUpdateAfterBind: VkBool32
    tensors: VkBool32

@dataclass
class VkPhysicalDeviceDescriptorBufferTensorPropertiesARM:
    tensorCaptureReplayDescriptorDataSize: size_t
    tensorViewCaptureReplayDescriptorDataSize: size_t
    tensorDescriptorSize: size_t

@dataclass
class VkPhysicalDeviceDescriptorBufferTensorFeaturesARM:
    descriptorBufferTensorDescriptors: VkBool32

@dataclass
class VkPhysicalDeviceShaderFloat8FeaturesEXT:
    shaderFloat8: VkBool32
    shaderFloat8CooperativeMatrix: VkBool32


# --- Physical Device Struct Aliases ---
VkPhysicalDevicePrivateDataFeaturesEXT = VkPhysicalDevicePrivateDataFeatures
VkPhysicalDeviceProperties2KHR = VkPhysicalDeviceProperties2
VkPhysicalDeviceMemoryProperties2KHR = VkPhysicalDeviceMemoryProperties2
VkPhysicalDevicePushDescriptorPropertiesKHR = VkPhysicalDevicePushDescriptorProperties
VkPhysicalDeviceDriverPropertiesKHR = VkPhysicalDeviceDriverProperties
VkPhysicalDeviceVariablePointersFeaturesKHR = VkPhysicalDeviceVariablePointersFeatures
VkPhysicalDeviceVariablePointerFeaturesKHR = VkPhysicalDeviceVariablePointersFeatures
VkPhysicalDeviceVariablePointerFeatures = VkPhysicalDeviceVariablePointersFeatures
VkPhysicalDeviceIDPropertiesKHR = VkPhysicalDeviceIDProperties
VkPhysicalDeviceMultiviewFeaturesKHR = VkPhysicalDeviceMultiviewFeatures
VkPhysicalDeviceMultiviewPropertiesKHR = VkPhysicalDeviceMultiviewProperties
VkPhysicalDeviceGroupPropertiesKHR = VkPhysicalDeviceGroupProperties
VkPhysicalDevice16BitStorageFeaturesKHR = VkPhysicalDevice16BitStorageFeatures
VkPhysicalDeviceShaderSubgroupExtendedTypesFeaturesKHR = VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures
VkPhysicalDevicePointClippingPropertiesKHR = VkPhysicalDevicePointClippingProperties
VkPhysicalDeviceSamplerYcbcrConversionFeaturesKHR = VkPhysicalDeviceSamplerYcbcrConversionFeatures
VkPhysicalDeviceSamplerFilterMinmaxPropertiesEXT = VkPhysicalDeviceSamplerFilterMinmaxProperties
VkPhysicalDeviceInlineUniformBlockFeaturesEXT = VkPhysicalDeviceInlineUniformBlockFeatures
VkPhysicalDeviceInlineUniformBlockPropertiesEXT = VkPhysicalDeviceInlineUniformBlockProperties
VkPhysicalDeviceMaintenance3PropertiesKHR = VkPhysicalDeviceMaintenance3Properties
VkPhysicalDeviceMaintenance4FeaturesKHR = VkPhysicalDeviceMaintenance4Features
VkPhysicalDeviceMaintenance4PropertiesKHR = VkPhysicalDeviceMaintenance4Properties
VkPhysicalDeviceMaintenance5FeaturesKHR = VkPhysicalDeviceMaintenance5Features
VkPhysicalDeviceMaintenance5PropertiesKHR = VkPhysicalDeviceMaintenance5Properties
VkPhysicalDeviceMaintenance6FeaturesKHR = VkPhysicalDeviceMaintenance6Features
VkPhysicalDeviceMaintenance6PropertiesKHR = VkPhysicalDeviceMaintenance6Properties
VkPhysicalDeviceShaderDrawParameterFeatures = VkPhysicalDeviceShaderDrawParametersFeatures
VkPhysicalDeviceShaderFloat16Int8FeaturesKHR = VkPhysicalDeviceShaderFloat16Int8Features
VkPhysicalDeviceFloat16Int8FeaturesKHR = VkPhysicalDeviceShaderFloat16Int8Features
VkPhysicalDeviceFloatControlsPropertiesKHR = VkPhysicalDeviceFloatControlsProperties
VkPhysicalDeviceHostQueryResetFeaturesEXT = VkPhysicalDeviceHostQueryResetFeatures
VkPhysicalDeviceGlobalPriorityQueryFeaturesKHR = VkPhysicalDeviceGlobalPriorityQueryFeatures
VkPhysicalDeviceGlobalPriorityQueryFeaturesEXT = VkPhysicalDeviceGlobalPriorityQueryFeatures
VkPhysicalDeviceDescriptorIndexingFeaturesEXT = VkPhysicalDeviceDescriptorIndexingFeatures
VkPhysicalDeviceDescriptorIndexingPropertiesEXT = VkPhysicalDeviceDescriptorIndexingProperties
VkPhysicalDeviceTimelineSemaphoreFeaturesKHR = VkPhysicalDeviceTimelineSemaphoreFeatures
VkPhysicalDeviceTimelineSemaphorePropertiesKHR = VkPhysicalDeviceTimelineSemaphoreProperties
VkPhysicalDeviceVertexAttributeDivisorPropertiesKHR = VkPhysicalDeviceVertexAttributeDivisorProperties
VkPhysicalDevice8BitStorageFeaturesKHR = VkPhysicalDevice8BitStorageFeatures
VkPhysicalDeviceVulkanMemoryModelFeaturesKHR = VkPhysicalDeviceVulkanMemoryModelFeatures
VkPhysicalDeviceShaderAtomicInt64FeaturesKHR = VkPhysicalDeviceShaderAtomicInt64Features
VkPhysicalDeviceVertexAttributeDivisorFeaturesKHR = VkPhysicalDeviceVertexAttributeDivisorFeatures
VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT = VkPhysicalDeviceVertexAttributeDivisorFeatures
VkPhysicalDeviceDepthStencilResolvePropertiesKHR = VkPhysicalDeviceDepthStencilResolveProperties
VkPhysicalDeviceComputeShaderDerivativesFeaturesNV = VkPhysicalDeviceComputeShaderDerivativesFeaturesKHR
VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV = VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR
VkPhysicalDeviceFragmentDensityMapOffsetFeaturesQCOM = VkPhysicalDeviceFragmentDensityMapOffsetFeaturesEXT
VkPhysicalDeviceFragmentDensityMapOffsetPropertiesQCOM = VkPhysicalDeviceFragmentDensityMapOffsetPropertiesEXT
VkPhysicalDeviceScalarBlockLayoutFeaturesEXT = VkPhysicalDeviceScalarBlockLayoutFeatures
VkPhysicalDeviceUniformBufferStandardLayoutFeaturesKHR = VkPhysicalDeviceUniformBufferStandardLayoutFeatures
VkPhysicalDeviceBufferDeviceAddressFeaturesKHR = VkPhysicalDeviceBufferDeviceAddressFeatures
VkPhysicalDeviceBufferAddressFeaturesEXT = VkPhysicalDeviceBufferDeviceAddressFeaturesEXT
VkPhysicalDeviceImagelessFramebufferFeaturesKHR = VkPhysicalDeviceImagelessFramebufferFeatures
VkPhysicalDeviceTextureCompressionASTCHDRFeaturesEXT = VkPhysicalDeviceTextureCompressionASTCHDRFeatures
VkPhysicalDeviceIndexTypeUint8FeaturesKHR = VkPhysicalDeviceIndexTypeUint8Features
VkPhysicalDeviceIndexTypeUint8FeaturesEXT = VkPhysicalDeviceIndexTypeUint8Features
VkPhysicalDeviceSeparateDepthStencilLayoutsFeaturesKHR = VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures
VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT = VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures
VkPhysicalDeviceTexelBufferAlignmentPropertiesEXT = VkPhysicalDeviceTexelBufferAlignmentProperties
VkPhysicalDeviceSubgroupSizeControlFeaturesEXT = VkPhysicalDeviceSubgroupSizeControlFeatures
VkPhysicalDeviceSubgroupSizeControlPropertiesEXT = VkPhysicalDeviceSubgroupSizeControlProperties
VkPhysicalDeviceLineRasterizationFeaturesKHR = VkPhysicalDeviceLineRasterizationFeatures
VkPhysicalDeviceLineRasterizationFeaturesEXT = VkPhysicalDeviceLineRasterizationFeatures
VkPhysicalDeviceLineRasterizationPropertiesKHR = VkPhysicalDeviceLineRasterizationProperties
VkPhysicalDeviceLineRasterizationPropertiesEXT = VkPhysicalDeviceLineRasterizationProperties
VkPhysicalDevicePipelineCreationCacheControlFeaturesEXT = VkPhysicalDevicePipelineCreationCacheControlFeatures
VkPhysicalDeviceToolPropertiesEXT = VkPhysicalDeviceToolProperties
VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeaturesKHR = VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures
VkPhysicalDeviceRobustness2FeaturesEXT = VkPhysicalDeviceRobustness2FeaturesKHR
VkPhysicalDeviceRobustness2PropertiesEXT = VkPhysicalDeviceRobustness2PropertiesKHR
VkPhysicalDeviceImageRobustnessFeaturesEXT = VkPhysicalDeviceImageRobustnessFeatures
VkPhysicalDeviceShaderTerminateInvocationFeaturesKHR = VkPhysicalDeviceShaderTerminateInvocationFeatures
VkPhysicalDeviceMutableDescriptorTypeFeaturesVALVE = VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT
VkPhysicalDeviceSynchronization2FeaturesKHR = VkPhysicalDeviceSynchronization2Features
VkPhysicalDeviceHostImageCopyFeaturesEXT = VkPhysicalDeviceHostImageCopyFeatures
VkPhysicalDeviceHostImageCopyPropertiesEXT = VkPhysicalDeviceHostImageCopyProperties
VkPhysicalDevicePipelineProtectedAccessFeaturesEXT = VkPhysicalDevicePipelineProtectedAccessFeatures
VkPhysicalDeviceShaderIntegerDotProductFeaturesKHR = VkPhysicalDeviceShaderIntegerDotProductFeatures
VkPhysicalDeviceShaderIntegerDotProductPropertiesKHR = VkPhysicalDeviceShaderIntegerDotProductProperties
VkPhysicalDeviceDynamicRenderingFeaturesKHR = VkPhysicalDeviceDynamicRenderingFeatures
VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesARM = VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT
VkPhysicalDevicePipelineRobustnessFeaturesEXT = VkPhysicalDevicePipelineRobustnessFeatures
VkPhysicalDevicePipelineRobustnessPropertiesEXT = VkPhysicalDevicePipelineRobustnessProperties
VkPhysicalDeviceDepthClampZeroOneFeaturesEXT = VkPhysicalDeviceDepthClampZeroOneFeaturesKHR
VkPhysicalDeviceShaderSubgroupRotateFeaturesKHR = VkPhysicalDeviceShaderSubgroupRotateFeatures
VkPhysicalDeviceShaderExpectAssumeFeaturesKHR = VkPhysicalDeviceShaderExpectAssumeFeatures
VkPhysicalDeviceShaderFloatControls2FeaturesKHR = VkPhysicalDeviceShaderFloatControls2Features
VkPhysicalDeviceDynamicRenderingLocalReadFeaturesKHR = VkPhysicalDeviceDynamicRenderingLocalReadFeatures


# --- List of All Processed Physical Device Structs ---
# Includes structs that:
# 1. Are not in 'disabled_structs' (implicitly, via VK_PHYSICAL_STRUCT_NAMES population).
# 2. Extend "VkPhysicalDeviceProperties2" or "VkPhysicalDeviceFeatures2"
#    (i.e., are in 'structs_with_valid_extends').
ALL_STRUCTS_EXTENDING_FEATURES_OR_PROPERTIES = [
    VkPhysicalDevice16BitStorageFeatures,
    VkPhysicalDevice16BitStorageFeaturesKHR,
    VkPhysicalDevice4444FormatsFeaturesEXT,
    VkPhysicalDevice8BitStorageFeatures,
    VkPhysicalDevice8BitStorageFeaturesKHR,
    VkPhysicalDeviceASTCDecodeFeaturesEXT,
    VkPhysicalDeviceAccelerationStructureFeaturesKHR,
    VkPhysicalDeviceAccelerationStructurePropertiesKHR,
    VkPhysicalDeviceAddressBindingReportFeaturesEXT,
    VkPhysicalDeviceAmigoProfilingFeaturesSEC,
    VkPhysicalDeviceAntiLagFeaturesAMD,
    VkPhysicalDeviceAttachmentFeedbackLoopDynamicStateFeaturesEXT,
    VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT,
    VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT,
    VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT,
    VkPhysicalDeviceBorderColorSwizzleFeaturesEXT,
    VkPhysicalDeviceBufferAddressFeaturesEXT,
    VkPhysicalDeviceBufferDeviceAddressFeatures,
    VkPhysicalDeviceBufferDeviceAddressFeaturesEXT,
    VkPhysicalDeviceBufferDeviceAddressFeaturesKHR,
    VkPhysicalDeviceClusterAccelerationStructureFeaturesNV,
    VkPhysicalDeviceClusterAccelerationStructurePropertiesNV,
    VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI,
    VkPhysicalDeviceClusterCullingShaderPropertiesHUAWEI,
    VkPhysicalDeviceCoherentMemoryFeaturesAMD,
    VkPhysicalDeviceColorWriteEnableFeaturesEXT,
    VkPhysicalDeviceCommandBufferInheritanceFeaturesNV,
    VkPhysicalDeviceComputeShaderDerivativesFeaturesKHR,
    VkPhysicalDeviceComputeShaderDerivativesFeaturesNV,
    VkPhysicalDeviceComputeShaderDerivativesPropertiesKHR,
    VkPhysicalDeviceConditionalRenderingFeaturesEXT,
    VkPhysicalDeviceConservativeRasterizationPropertiesEXT,
    VkPhysicalDeviceCooperativeMatrix2FeaturesNV,
    VkPhysicalDeviceCooperativeMatrix2PropertiesNV,
    VkPhysicalDeviceCooperativeMatrixFeaturesKHR,
    VkPhysicalDeviceCooperativeMatrixFeaturesNV,
    VkPhysicalDeviceCooperativeMatrixPropertiesKHR,
    VkPhysicalDeviceCooperativeMatrixPropertiesNV,
    VkPhysicalDeviceCooperativeVectorFeaturesNV,
    VkPhysicalDeviceCooperativeVectorPropertiesNV,
    VkPhysicalDeviceCopyMemoryIndirectFeaturesNV,
    VkPhysicalDeviceCopyMemoryIndirectPropertiesNV,
    VkPhysicalDeviceCornerSampledImageFeaturesNV,
    VkPhysicalDeviceCoverageReductionModeFeaturesNV,
    VkPhysicalDeviceCubicClampFeaturesQCOM,
    VkPhysicalDeviceCubicWeightsFeaturesQCOM,
    VkPhysicalDeviceCustomBorderColorFeaturesEXT,
    VkPhysicalDeviceCustomBorderColorPropertiesEXT,
    VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV,
    VkPhysicalDeviceDepthBiasControlFeaturesEXT,
    VkPhysicalDeviceDepthClampControlFeaturesEXT,
    VkPhysicalDeviceDepthClampZeroOneFeaturesKHR,
    VkPhysicalDeviceDepthClipControlFeaturesEXT,
    VkPhysicalDeviceDepthClipEnableFeaturesEXT,
    VkPhysicalDeviceDepthStencilResolveProperties,
    VkPhysicalDeviceDepthStencilResolvePropertiesKHR,
    VkPhysicalDeviceDescriptorBufferDensityMapPropertiesEXT,
    VkPhysicalDeviceDescriptorBufferFeaturesEXT,
    VkPhysicalDeviceDescriptorBufferPropertiesEXT,
    VkPhysicalDeviceDescriptorBufferTensorFeaturesARM,
    VkPhysicalDeviceDescriptorBufferTensorPropertiesARM,
    VkPhysicalDeviceDescriptorIndexingFeatures,
    VkPhysicalDeviceDescriptorIndexingFeaturesEXT,
    VkPhysicalDeviceDescriptorIndexingProperties,
    VkPhysicalDeviceDescriptorIndexingPropertiesEXT,
    VkPhysicalDeviceDescriptorPoolOverallocationFeaturesNV,
    VkPhysicalDeviceDescriptorSetHostMappingFeaturesVALVE,
    VkPhysicalDeviceDeviceGeneratedCommandsComputeFeaturesNV,
    VkPhysicalDeviceDeviceGeneratedCommandsFeaturesEXT,
    VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV,
    VkPhysicalDeviceDeviceGeneratedCommandsPropertiesEXT,
    VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV,
    VkPhysicalDeviceDeviceMemoryReportFeaturesEXT,
    VkPhysicalDeviceDiagnosticsConfigFeaturesNV,
    VkPhysicalDeviceDiscardRectanglePropertiesEXT,
    VkPhysicalDeviceDriverProperties,
    VkPhysicalDeviceDriverPropertiesKHR,
    VkPhysicalDeviceDrmPropertiesEXT,
    VkPhysicalDeviceDynamicRenderingFeatures,
    VkPhysicalDeviceDynamicRenderingFeaturesKHR,
    VkPhysicalDeviceDynamicRenderingLocalReadFeatures,
    VkPhysicalDeviceDynamicRenderingLocalReadFeaturesKHR,
    VkPhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT,
    VkPhysicalDeviceExclusiveScissorFeaturesNV,
    VkPhysicalDeviceExtendedDynamicState2FeaturesEXT,
    VkPhysicalDeviceExtendedDynamicState3FeaturesEXT,
    VkPhysicalDeviceExtendedDynamicState3PropertiesEXT,
    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT,
    VkPhysicalDeviceExtendedSparseAddressSpaceFeaturesNV,
    VkPhysicalDeviceExtendedSparseAddressSpacePropertiesNV,
    VkPhysicalDeviceExternalComputeQueuePropertiesNV,
    VkPhysicalDeviceExternalFormatResolveFeaturesANDROID,
    VkPhysicalDeviceExternalFormatResolvePropertiesANDROID,
    VkPhysicalDeviceExternalMemoryHostPropertiesEXT,
    VkPhysicalDeviceExternalMemoryRDMAFeaturesNV,
    VkPhysicalDeviceFaultFeaturesEXT,
    VkPhysicalDeviceFloat16Int8FeaturesKHR,
    VkPhysicalDeviceFloatControlsProperties,
    VkPhysicalDeviceFloatControlsPropertiesKHR,
    VkPhysicalDeviceFormatPackFeaturesARM,
    VkPhysicalDeviceFragmentDensityMap2FeaturesEXT,
    VkPhysicalDeviceFragmentDensityMap2PropertiesEXT,
    VkPhysicalDeviceFragmentDensityMapFeaturesEXT,
    VkPhysicalDeviceFragmentDensityMapOffsetFeaturesEXT,
    VkPhysicalDeviceFragmentDensityMapOffsetFeaturesQCOM,
    VkPhysicalDeviceFragmentDensityMapOffsetPropertiesEXT,
    VkPhysicalDeviceFragmentDensityMapOffsetPropertiesQCOM,
    VkPhysicalDeviceFragmentDensityMapPropertiesEXT,
    VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR,
    VkPhysicalDeviceFragmentShaderBarycentricPropertiesKHR,
    VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT,
    VkPhysicalDeviceFragmentShadingRateEnumsFeaturesNV,
    VkPhysicalDeviceFragmentShadingRateEnumsPropertiesNV,
    VkPhysicalDeviceFragmentShadingRateFeaturesKHR,
    VkPhysicalDeviceFragmentShadingRatePropertiesKHR,
    VkPhysicalDeviceFrameBoundaryFeaturesEXT,
    VkPhysicalDeviceGlobalPriorityQueryFeatures,
    VkPhysicalDeviceGlobalPriorityQueryFeaturesEXT,
    VkPhysicalDeviceGlobalPriorityQueryFeaturesKHR,
    VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT,
    VkPhysicalDeviceGraphicsPipelineLibraryPropertiesEXT,
    VkPhysicalDeviceHdrVividFeaturesHUAWEI,
    VkPhysicalDeviceHostImageCopyFeatures,
    VkPhysicalDeviceHostImageCopyFeaturesEXT,
    VkPhysicalDeviceHostImageCopyProperties,
    VkPhysicalDeviceHostImageCopyPropertiesEXT,
    VkPhysicalDeviceHostQueryResetFeatures,
    VkPhysicalDeviceHostQueryResetFeaturesEXT,
    VkPhysicalDeviceIDProperties,
    VkPhysicalDeviceIDPropertiesKHR,
    VkPhysicalDeviceImage2DViewOf3DFeaturesEXT,
    VkPhysicalDeviceImageAlignmentControlFeaturesMESA,
    VkPhysicalDeviceImageAlignmentControlPropertiesMESA,
    VkPhysicalDeviceImageCompressionControlFeaturesEXT,
    VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT,
    VkPhysicalDeviceImageProcessing2FeaturesQCOM,
    VkPhysicalDeviceImageProcessing2PropertiesQCOM,
    VkPhysicalDeviceImageProcessingFeaturesQCOM,
    VkPhysicalDeviceImageProcessingPropertiesQCOM,
    VkPhysicalDeviceImageRobustnessFeatures,
    VkPhysicalDeviceImageRobustnessFeaturesEXT,
    VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT,
    VkPhysicalDeviceImageViewMinLodFeaturesEXT,
    VkPhysicalDeviceImagelessFramebufferFeatures,
    VkPhysicalDeviceImagelessFramebufferFeaturesKHR,
    VkPhysicalDeviceIndexTypeUint8Features,
    VkPhysicalDeviceIndexTypeUint8FeaturesEXT,
    VkPhysicalDeviceIndexTypeUint8FeaturesKHR,
    VkPhysicalDeviceInheritedViewportScissorFeaturesNV,
    VkPhysicalDeviceInlineUniformBlockFeatures,
    VkPhysicalDeviceInlineUniformBlockFeaturesEXT,
    VkPhysicalDeviceInlineUniformBlockProperties,
    VkPhysicalDeviceInlineUniformBlockPropertiesEXT,
    VkPhysicalDeviceInvocationMaskFeaturesHUAWEI,
    VkPhysicalDeviceLayeredApiPropertiesListKHR,
    VkPhysicalDeviceLayeredDriverPropertiesMSFT,
    VkPhysicalDeviceLegacyDitheringFeaturesEXT,
    VkPhysicalDeviceLegacyVertexAttributesFeaturesEXT,
    VkPhysicalDeviceLegacyVertexAttributesPropertiesEXT,
    VkPhysicalDeviceLineRasterizationFeatures,
    VkPhysicalDeviceLineRasterizationFeaturesEXT,
    VkPhysicalDeviceLineRasterizationFeaturesKHR,
    VkPhysicalDeviceLineRasterizationProperties,
    VkPhysicalDeviceLineRasterizationPropertiesEXT,
    VkPhysicalDeviceLineRasterizationPropertiesKHR,
    VkPhysicalDeviceLinearColorAttachmentFeaturesNV,
    VkPhysicalDeviceMaintenance3Properties,
    VkPhysicalDeviceMaintenance3PropertiesKHR,
    VkPhysicalDeviceMaintenance4Features,
    VkPhysicalDeviceMaintenance4FeaturesKHR,
    VkPhysicalDeviceMaintenance4Properties,
    VkPhysicalDeviceMaintenance4PropertiesKHR,
    VkPhysicalDeviceMaintenance5Features,
    VkPhysicalDeviceMaintenance5FeaturesKHR,
    VkPhysicalDeviceMaintenance5Properties,
    VkPhysicalDeviceMaintenance5PropertiesKHR,
    VkPhysicalDeviceMaintenance6Features,
    VkPhysicalDeviceMaintenance6FeaturesKHR,
    VkPhysicalDeviceMaintenance6Properties,
    VkPhysicalDeviceMaintenance6PropertiesKHR,
    VkPhysicalDeviceMaintenance7FeaturesKHR,
    VkPhysicalDeviceMaintenance7PropertiesKHR,
    VkPhysicalDeviceMaintenance8FeaturesKHR,
    VkPhysicalDeviceMaintenance9FeaturesKHR,
    VkPhysicalDeviceMaintenance9PropertiesKHR,
    VkPhysicalDeviceMapMemoryPlacedFeaturesEXT,
    VkPhysicalDeviceMapMemoryPlacedPropertiesEXT,
    VkPhysicalDeviceMemoryDecompressionFeaturesNV,
    VkPhysicalDeviceMemoryDecompressionPropertiesNV,
    VkPhysicalDeviceMemoryPriorityFeaturesEXT,
    VkPhysicalDeviceMeshShaderFeaturesEXT,
    VkPhysicalDeviceMeshShaderFeaturesNV,
    VkPhysicalDeviceMeshShaderPropertiesEXT,
    VkPhysicalDeviceMeshShaderPropertiesNV,
    VkPhysicalDeviceMultiDrawFeaturesEXT,
    VkPhysicalDeviceMultiDrawPropertiesEXT,
    VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT,
    VkPhysicalDeviceMultiviewFeatures,
    VkPhysicalDeviceMultiviewFeaturesKHR,
    VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX,
    VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM,
    VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM,
    VkPhysicalDeviceMultiviewProperties,
    VkPhysicalDeviceMultiviewPropertiesKHR,
    VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT,
    VkPhysicalDeviceMutableDescriptorTypeFeaturesVALVE,
    VkPhysicalDeviceNestedCommandBufferFeaturesEXT,
    VkPhysicalDeviceNestedCommandBufferPropertiesEXT,
    VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT,
    VkPhysicalDeviceOpacityMicromapFeaturesEXT,
    VkPhysicalDeviceOpacityMicromapPropertiesEXT,
    VkPhysicalDeviceOpticalFlowFeaturesNV,
    VkPhysicalDeviceOpticalFlowPropertiesNV,
    VkPhysicalDevicePCIBusInfoPropertiesEXT,
    VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT,
    VkPhysicalDevicePartitionedAccelerationStructureFeaturesNV,
    VkPhysicalDevicePartitionedAccelerationStructurePropertiesNV,
    VkPhysicalDevicePerStageDescriptorSetFeaturesNV,
    VkPhysicalDevicePerformanceQueryFeaturesKHR,
    VkPhysicalDevicePerformanceQueryPropertiesKHR,
    VkPhysicalDevicePipelineBinaryFeaturesKHR,
    VkPhysicalDevicePipelineBinaryPropertiesKHR,
    VkPhysicalDevicePipelineCreationCacheControlFeatures,
    VkPhysicalDevicePipelineCreationCacheControlFeaturesEXT,
    VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR,
    VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT,
    VkPhysicalDevicePipelineOpacityMicromapFeaturesARM,
    VkPhysicalDevicePipelinePropertiesFeaturesEXT,
    VkPhysicalDevicePipelineProtectedAccessFeatures,
    VkPhysicalDevicePipelineProtectedAccessFeaturesEXT,
    VkPhysicalDevicePipelineRobustnessFeatures,
    VkPhysicalDevicePipelineRobustnessFeaturesEXT,
    VkPhysicalDevicePipelineRobustnessProperties,
    VkPhysicalDevicePipelineRobustnessPropertiesEXT,
    VkPhysicalDevicePointClippingProperties,
    VkPhysicalDevicePointClippingPropertiesKHR,
    VkPhysicalDevicePresentBarrierFeaturesNV,
    VkPhysicalDevicePresentId2FeaturesKHR,
    VkPhysicalDevicePresentIdFeaturesKHR,
    VkPhysicalDevicePresentModeFifoLatestReadyFeaturesEXT,
    VkPhysicalDevicePresentWait2FeaturesKHR,
    VkPhysicalDevicePresentWaitFeaturesKHR,
    VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT,
    VkPhysicalDevicePrimitivesGeneratedQueryFeaturesEXT,
    VkPhysicalDevicePrivateDataFeatures,
    VkPhysicalDevicePrivateDataFeaturesEXT,
    VkPhysicalDeviceProtectedMemoryFeatures,
    VkPhysicalDeviceProtectedMemoryProperties,
    VkPhysicalDeviceProvokingVertexFeaturesEXT,
    VkPhysicalDeviceProvokingVertexPropertiesEXT,
    VkPhysicalDevicePushDescriptorProperties,
    VkPhysicalDevicePushDescriptorPropertiesKHR,
    VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT,
    VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesARM,
    VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT,
    VkPhysicalDeviceRawAccessChainsFeaturesNV,
    VkPhysicalDeviceRayQueryFeaturesKHR,
    VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV,
    VkPhysicalDeviceRayTracingInvocationReorderPropertiesNV,
    VkPhysicalDeviceRayTracingLinearSweptSpheresFeaturesNV,
    VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR,
    VkPhysicalDeviceRayTracingMotionBlurFeaturesNV,
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR,
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR,
    VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR,
    VkPhysicalDeviceRayTracingPropertiesNV,
    VkPhysicalDeviceRayTracingValidationFeaturesNV,
    VkPhysicalDeviceRelaxedLineRasterizationFeaturesIMG,
    VkPhysicalDeviceRenderPassStripedFeaturesARM,
    VkPhysicalDeviceRenderPassStripedPropertiesARM,
    VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV,
    VkPhysicalDeviceRobustness2FeaturesEXT,
    VkPhysicalDeviceRobustness2FeaturesKHR,
    VkPhysicalDeviceRobustness2PropertiesEXT,
    VkPhysicalDeviceRobustness2PropertiesKHR,
    VkPhysicalDeviceSampleLocationsPropertiesEXT,
    VkPhysicalDeviceSamplerFilterMinmaxProperties,
    VkPhysicalDeviceSamplerFilterMinmaxPropertiesEXT,
    VkPhysicalDeviceSamplerYcbcrConversionFeatures,
    VkPhysicalDeviceSamplerYcbcrConversionFeaturesKHR,
    VkPhysicalDeviceScalarBlockLayoutFeatures,
    VkPhysicalDeviceScalarBlockLayoutFeaturesEXT,
    VkPhysicalDeviceSchedulingControlsFeaturesARM,
    VkPhysicalDeviceSchedulingControlsPropertiesARM,
    VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures,
    VkPhysicalDeviceSeparateDepthStencilLayoutsFeaturesKHR,
    VkPhysicalDeviceShaderAtomicFloat16VectorFeaturesNV,
    VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT,
    VkPhysicalDeviceShaderAtomicFloatFeaturesEXT,
    VkPhysicalDeviceShaderAtomicInt64Features,
    VkPhysicalDeviceShaderAtomicInt64FeaturesKHR,
    VkPhysicalDeviceShaderBfloat16FeaturesKHR,
    VkPhysicalDeviceShaderClockFeaturesKHR,
    VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM,
    VkPhysicalDeviceShaderCoreBuiltinsPropertiesARM,
    VkPhysicalDeviceShaderCoreProperties2AMD,
    VkPhysicalDeviceShaderCorePropertiesAMD,
    VkPhysicalDeviceShaderCorePropertiesARM,
    VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures,
    VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT,
    VkPhysicalDeviceShaderDrawParameterFeatures,
    VkPhysicalDeviceShaderDrawParametersFeatures,
    VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD,
    VkPhysicalDeviceShaderExpectAssumeFeatures,
    VkPhysicalDeviceShaderExpectAssumeFeaturesKHR,
    VkPhysicalDeviceShaderFloat16Int8Features,
    VkPhysicalDeviceShaderFloat16Int8FeaturesKHR,
    VkPhysicalDeviceShaderFloat8FeaturesEXT,
    VkPhysicalDeviceShaderFloatControls2Features,
    VkPhysicalDeviceShaderFloatControls2FeaturesKHR,
    VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT,
    VkPhysicalDeviceShaderImageFootprintFeaturesNV,
    VkPhysicalDeviceShaderIntegerDotProductFeatures,
    VkPhysicalDeviceShaderIntegerDotProductFeaturesKHR,
    VkPhysicalDeviceShaderIntegerDotProductProperties,
    VkPhysicalDeviceShaderIntegerDotProductPropertiesKHR,
    VkPhysicalDeviceShaderIntegerFunctions2FeaturesINTEL,
    VkPhysicalDeviceShaderMaximalReconvergenceFeaturesKHR,
    VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT,
    VkPhysicalDeviceShaderModuleIdentifierPropertiesEXT,
    VkPhysicalDeviceShaderObjectFeaturesEXT,
    VkPhysicalDeviceShaderObjectPropertiesEXT,
    VkPhysicalDeviceShaderQuadControlFeaturesKHR,
    VkPhysicalDeviceShaderRelaxedExtendedInstructionFeaturesKHR,
    VkPhysicalDeviceShaderReplicatedCompositesFeaturesEXT,
    VkPhysicalDeviceShaderSMBuiltinsFeaturesNV,
    VkPhysicalDeviceShaderSMBuiltinsPropertiesNV,
    VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures,
    VkPhysicalDeviceShaderSubgroupExtendedTypesFeaturesKHR,
    VkPhysicalDeviceShaderSubgroupRotateFeatures,
    VkPhysicalDeviceShaderSubgroupRotateFeaturesKHR,
    VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR,
    VkPhysicalDeviceShaderTerminateInvocationFeatures,
    VkPhysicalDeviceShaderTerminateInvocationFeaturesKHR,
    VkPhysicalDeviceShaderTileImageFeaturesEXT,
    VkPhysicalDeviceShaderTileImagePropertiesEXT,
    VkPhysicalDeviceShadingRateImageFeaturesNV,
    VkPhysicalDeviceShadingRateImagePropertiesNV,
    VkPhysicalDeviceSubgroupProperties,
    VkPhysicalDeviceSubgroupSizeControlFeatures,
    VkPhysicalDeviceSubgroupSizeControlFeaturesEXT,
    VkPhysicalDeviceSubgroupSizeControlProperties,
    VkPhysicalDeviceSubgroupSizeControlPropertiesEXT,
    VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT,
    VkPhysicalDeviceSubpassShadingFeaturesHUAWEI,
    VkPhysicalDeviceSubpassShadingPropertiesHUAWEI,
    VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT,
    VkPhysicalDeviceSynchronization2Features,
    VkPhysicalDeviceSynchronization2FeaturesKHR,
    VkPhysicalDeviceTensorFeaturesARM,
    VkPhysicalDeviceTensorPropertiesARM,
    VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT,
    VkPhysicalDeviceTexelBufferAlignmentProperties,
    VkPhysicalDeviceTexelBufferAlignmentPropertiesEXT,
    VkPhysicalDeviceTextureCompressionASTCHDRFeatures,
    VkPhysicalDeviceTextureCompressionASTCHDRFeaturesEXT,
    VkPhysicalDeviceTileMemoryHeapFeaturesQCOM,
    VkPhysicalDeviceTileMemoryHeapPropertiesQCOM,
    VkPhysicalDeviceTilePropertiesFeaturesQCOM,
    VkPhysicalDeviceTileShadingFeaturesQCOM,
    VkPhysicalDeviceTileShadingPropertiesQCOM,
    VkPhysicalDeviceTimelineSemaphoreFeatures,
    VkPhysicalDeviceTimelineSemaphoreFeaturesKHR,
    VkPhysicalDeviceTimelineSemaphoreProperties,
    VkPhysicalDeviceTimelineSemaphorePropertiesKHR,
    VkPhysicalDeviceTransformFeedbackFeaturesEXT,
    VkPhysicalDeviceTransformFeedbackPropertiesEXT,
    VkPhysicalDeviceUnifiedImageLayoutsFeaturesKHR,
    VkPhysicalDeviceUniformBufferStandardLayoutFeatures,
    VkPhysicalDeviceUniformBufferStandardLayoutFeaturesKHR,
    VkPhysicalDeviceVariablePointerFeatures,
    VkPhysicalDeviceVariablePointerFeaturesKHR,
    VkPhysicalDeviceVariablePointersFeatures,
    VkPhysicalDeviceVariablePointersFeaturesKHR,
    VkPhysicalDeviceVertexAttributeDivisorFeatures,
    VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT,
    VkPhysicalDeviceVertexAttributeDivisorFeaturesKHR,
    VkPhysicalDeviceVertexAttributeDivisorProperties,
    VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT,
    VkPhysicalDeviceVertexAttributeDivisorPropertiesKHR,
    VkPhysicalDeviceVertexAttributeRobustnessFeaturesEXT,
    VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT,
    VkPhysicalDeviceVideoDecodeVP9FeaturesKHR,
    VkPhysicalDeviceVideoEncodeAV1FeaturesKHR,
    VkPhysicalDeviceVideoEncodeQuantizationMapFeaturesKHR,
    VkPhysicalDeviceVideoMaintenance1FeaturesKHR,
    VkPhysicalDeviceVideoMaintenance2FeaturesKHR,
    VkPhysicalDeviceVulkan11Features,
    VkPhysicalDeviceVulkan11Properties,
    VkPhysicalDeviceVulkan12Features,
    VkPhysicalDeviceVulkan12Properties,
    VkPhysicalDeviceVulkan13Features,
    VkPhysicalDeviceVulkan13Properties,
    VkPhysicalDeviceVulkan14Features,
    VkPhysicalDeviceVulkan14Properties,
    VkPhysicalDeviceVulkanMemoryModelFeatures,
    VkPhysicalDeviceVulkanMemoryModelFeaturesKHR,
    VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR,
    VkPhysicalDeviceYcbcr2Plane444FormatsFeaturesEXT,
    VkPhysicalDeviceYcbcrDegammaFeaturesQCOM,
    VkPhysicalDeviceYcbcrImageArraysFeaturesEXT,
    VkPhysicalDeviceZeroInitializeDeviceMemoryFeaturesEXT,
    VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures,
    VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeaturesKHR,
]


# --- Vulkan Extension to Struct Mappings ---
# VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING: Maps enabled extension names to their PhysicalDevice structs.
# Extension Filters:
# - 'supported' is not "disabled".
# - 'platform' (if present) is "android".
# Struct Filters (per extension):
# - Not in global 'disabled_structs'.
# - Extends "VkPhysicalDeviceProperties2" or "VkPhysicalDeviceFeatures2".
# Format: {ext_name: [{struct_name: sType_enum_value}, ...]}
VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING = {"extensions":
{   'VK_AMD_anti_lag': [   {   'VkPhysicalDeviceAntiLagFeaturesAMD': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ANTI_LAG_FEATURES_AMD'}],
    'VK_AMD_device_coherent_memory': [   {   'VkPhysicalDeviceCoherentMemoryFeaturesAMD': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD'}],
    'VK_AMD_shader_core_properties': [   {   'VkPhysicalDeviceShaderCorePropertiesAMD': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_AMD'}],
    'VK_AMD_shader_core_properties2': [   {   'VkPhysicalDeviceShaderCoreProperties2AMD': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_2_AMD'}],
    'VK_AMD_shader_early_and_late_fragment_tests': [   {   'VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_EARLY_AND_LATE_FRAGMENT_TESTS_FEATURES_AMD'}],
    'VK_ANDROID_external_format_resolve': [   {   'VkPhysicalDeviceExternalFormatResolveFeaturesANDROID': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_FORMAT_RESOLVE_FEATURES_ANDROID'},
                                              {   'VkPhysicalDeviceExternalFormatResolvePropertiesANDROID': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_FORMAT_RESOLVE_PROPERTIES_ANDROID'}],
    'VK_ARM_format_pack': [   {   'VkPhysicalDeviceFormatPackFeaturesARM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FORMAT_PACK_FEATURES_ARM'}],
    'VK_ARM_pipeline_opacity_micromap': [   {   'VkPhysicalDevicePipelineOpacityMicromapFeaturesARM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_OPACITY_MICROMAP_FEATURES_ARM'}],
    'VK_ARM_rasterization_order_attachment_access': [   {   'VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesARM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_FEATURES_EXT'}],
    'VK_ARM_render_pass_striped': [   {   'VkPhysicalDeviceRenderPassStripedFeaturesARM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RENDER_PASS_STRIPED_FEATURES_ARM'},
                                      {   'VkPhysicalDeviceRenderPassStripedPropertiesARM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RENDER_PASS_STRIPED_PROPERTIES_ARM'}],
    'VK_ARM_scheduling_controls': [   {   'VkPhysicalDeviceSchedulingControlsFeaturesARM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCHEDULING_CONTROLS_FEATURES_ARM'},
                                      {   'VkPhysicalDeviceSchedulingControlsPropertiesARM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCHEDULING_CONTROLS_PROPERTIES_ARM'}],
    'VK_ARM_shader_core_builtins': [   {   'VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_BUILTINS_FEATURES_ARM'},
                                       {   'VkPhysicalDeviceShaderCoreBuiltinsPropertiesARM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_BUILTINS_PROPERTIES_ARM'}],
    'VK_ARM_shader_core_properties': [   {   'VkPhysicalDeviceShaderCorePropertiesARM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_ARM'}],
    'VK_ARM_tensors': [   {   'VkPhysicalDeviceTensorPropertiesARM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TENSOR_PROPERTIES_ARM'},
                          {   'VkPhysicalDeviceTensorFeaturesARM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TENSOR_FEATURES_ARM'},
                          {   'VkPhysicalDeviceDescriptorBufferTensorFeaturesARM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_TENSOR_FEATURES_ARM'},
                          {   'VkPhysicalDeviceDescriptorBufferTensorPropertiesARM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_TENSOR_PROPERTIES_ARM'}],
    'VK_EXT_4444_formats': [   {   'VkPhysicalDevice4444FormatsFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_4444_FORMATS_FEATURES_EXT'}],
    'VK_EXT_astc_decode_mode': [   {   'VkPhysicalDeviceASTCDecodeFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ASTC_DECODE_FEATURES_EXT'}],
    'VK_EXT_attachment_feedback_loop_dynamic_state': [   {   'VkPhysicalDeviceAttachmentFeedbackLoopDynamicStateFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ATTACHMENT_FEEDBACK_LOOP_DYNAMIC_STATE_FEATURES_EXT'}],
    'VK_EXT_attachment_feedback_loop_layout': [   {   'VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ATTACHMENT_FEEDBACK_LOOP_LAYOUT_FEATURES_EXT'}],
    'VK_EXT_blend_operation_advanced': [   {   'VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_FEATURES_EXT'},
                                           {   'VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_PROPERTIES_EXT'}],
    'VK_EXT_border_color_swizzle': [   {   'VkPhysicalDeviceBorderColorSwizzleFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BORDER_COLOR_SWIZZLE_FEATURES_EXT'}],
    'VK_EXT_buffer_device_address': [   {   'VkPhysicalDeviceBufferAddressFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT'},
                                        {   'VkPhysicalDeviceBufferDeviceAddressFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT'}],
    'VK_EXT_color_write_enable': [   {   'VkPhysicalDeviceColorWriteEnableFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COLOR_WRITE_ENABLE_FEATURES_EXT'}],
    'VK_EXT_conditional_rendering': [   {   'VkPhysicalDeviceConditionalRenderingFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT'}],
    'VK_EXT_conservative_rasterization': [   {   'VkPhysicalDeviceConservativeRasterizationPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT'}],
    'VK_EXT_custom_border_color': [   {   'VkPhysicalDeviceCustomBorderColorPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_PROPERTIES_EXT'},
                                      {   'VkPhysicalDeviceCustomBorderColorFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT'}],
    'VK_EXT_depth_bias_control': [   {   'VkPhysicalDeviceDepthBiasControlFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_BIAS_CONTROL_FEATURES_EXT'}],
    'VK_EXT_depth_clamp_control': [   {   'VkPhysicalDeviceDepthClampControlFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLAMP_CONTROL_FEATURES_EXT'}],
    'VK_EXT_depth_clip_control': [   {   'VkPhysicalDeviceDepthClipControlFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_CONTROL_FEATURES_EXT'}],
    'VK_EXT_depth_clip_enable': [   {   'VkPhysicalDeviceDepthClipEnableFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT'}],
    'VK_EXT_descriptor_buffer': [   {   'VkPhysicalDeviceDescriptorBufferPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT'},
                                    {   'VkPhysicalDeviceDescriptorBufferDensityMapPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_DENSITY_MAP_PROPERTIES_EXT'},
                                    {   'VkPhysicalDeviceDescriptorBufferFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT'}],
    'VK_EXT_descriptor_indexing': [   {   'VkPhysicalDeviceDescriptorIndexingFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES'},
                                      {   'VkPhysicalDeviceDescriptorIndexingPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES'}],
    'VK_EXT_device_address_binding_report': [   {   'VkPhysicalDeviceAddressBindingReportFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ADDRESS_BINDING_REPORT_FEATURES_EXT'}],
    'VK_EXT_device_fault': [   {   'VkPhysicalDeviceFaultFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT'}],
    'VK_EXT_device_generated_commands': [   {   'VkPhysicalDeviceDeviceGeneratedCommandsFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_FEATURES_EXT'},
                                            {   'VkPhysicalDeviceDeviceGeneratedCommandsPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_PROPERTIES_EXT'}],
    'VK_EXT_device_memory_report': [   {   'VkPhysicalDeviceDeviceMemoryReportFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_MEMORY_REPORT_FEATURES_EXT'}],
    'VK_EXT_discard_rectangles': [   {   'VkPhysicalDeviceDiscardRectanglePropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DISCARD_RECTANGLE_PROPERTIES_EXT'}],
    'VK_EXT_dynamic_rendering_unused_attachments': [   {   'VkPhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_FEATURES_EXT'}],
    'VK_EXT_extended_dynamic_state': [   {   'VkPhysicalDeviceExtendedDynamicStateFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT'}],
    'VK_EXT_extended_dynamic_state2': [   {   'VkPhysicalDeviceExtendedDynamicState2FeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT'}],
    'VK_EXT_extended_dynamic_state3': [   {   'VkPhysicalDeviceExtendedDynamicState3FeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT'},
                                          {   'VkPhysicalDeviceExtendedDynamicState3PropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_PROPERTIES_EXT'}],
    'VK_EXT_external_memory_host': [   {   'VkPhysicalDeviceExternalMemoryHostPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT'}],
    'VK_EXT_fragment_density_map': [   {   'VkPhysicalDeviceFragmentDensityMapFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT'},
                                       {   'VkPhysicalDeviceFragmentDensityMapPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_PROPERTIES_EXT'}],
    'VK_EXT_fragment_density_map2': [   {   'VkPhysicalDeviceFragmentDensityMap2FeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_FEATURES_EXT'},
                                        {   'VkPhysicalDeviceFragmentDensityMap2PropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_PROPERTIES_EXT'}],
    'VK_EXT_fragment_density_map_offset': [   {   'VkPhysicalDeviceFragmentDensityMapOffsetFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_OFFSET_FEATURES_EXT'},
                                              {   'VkPhysicalDeviceFragmentDensityMapOffsetPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_OFFSET_PROPERTIES_EXT'}],
    'VK_EXT_fragment_shader_interlock': [   {   'VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT'}],
    'VK_EXT_frame_boundary': [   {   'VkPhysicalDeviceFrameBoundaryFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAME_BOUNDARY_FEATURES_EXT'}],
    'VK_EXT_global_priority_query': [   {   'VkPhysicalDeviceGlobalPriorityQueryFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GLOBAL_PRIORITY_QUERY_FEATURES'}],
    'VK_EXT_graphics_pipeline_library': [   {   'VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT'},
                                            {   'VkPhysicalDeviceGraphicsPipelineLibraryPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_PROPERTIES_EXT'}],
    'VK_EXT_host_image_copy': [   {   'VkPhysicalDeviceHostImageCopyFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_IMAGE_COPY_FEATURES'},
                                  {   'VkPhysicalDeviceHostImageCopyPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_IMAGE_COPY_PROPERTIES'}],
    'VK_EXT_host_query_reset': [   {   'VkPhysicalDeviceHostQueryResetFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES'}],
    'VK_EXT_image_2d_view_of_3d': [   {   'VkPhysicalDeviceImage2DViewOf3DFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_2D_VIEW_OF_3D_FEATURES_EXT'}],
    'VK_EXT_image_compression_control': [   {   'VkPhysicalDeviceImageCompressionControlFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_COMPRESSION_CONTROL_FEATURES_EXT'}],
    'VK_EXT_image_compression_control_swapchain': [   {   'VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_COMPRESSION_CONTROL_SWAPCHAIN_FEATURES_EXT'}],
    'VK_EXT_image_robustness': [   {   'VkPhysicalDeviceImageRobustnessFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ROBUSTNESS_FEATURES'}],
    'VK_EXT_image_sliced_view_of_3d': [   {   'VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_SLICED_VIEW_OF_3D_FEATURES_EXT'}],
    'VK_EXT_image_view_min_lod': [   {   'VkPhysicalDeviceImageViewMinLodFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_MIN_LOD_FEATURES_EXT'}],
    'VK_EXT_index_type_uint8': [   {   'VkPhysicalDeviceIndexTypeUint8FeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES'}],
    'VK_EXT_inline_uniform_block': [   {   'VkPhysicalDeviceInlineUniformBlockFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES'},
                                       {   'VkPhysicalDeviceInlineUniformBlockPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_PROPERTIES'}],
    'VK_EXT_legacy_dithering': [   {   'VkPhysicalDeviceLegacyDitheringFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LEGACY_DITHERING_FEATURES_EXT'}],
    'VK_EXT_legacy_vertex_attributes': [   {   'VkPhysicalDeviceLegacyVertexAttributesFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LEGACY_VERTEX_ATTRIBUTES_FEATURES_EXT'},
                                           {   'VkPhysicalDeviceLegacyVertexAttributesPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LEGACY_VERTEX_ATTRIBUTES_PROPERTIES_EXT'}],
    'VK_EXT_line_rasterization': [   {   'VkPhysicalDeviceLineRasterizationFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES'},
                                     {   'VkPhysicalDeviceLineRasterizationPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_PROPERTIES'}],
    'VK_EXT_map_memory_placed': [   {   'VkPhysicalDeviceMapMemoryPlacedFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAP_MEMORY_PLACED_FEATURES_EXT'},
                                    {   'VkPhysicalDeviceMapMemoryPlacedPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAP_MEMORY_PLACED_PROPERTIES_EXT'}],
    'VK_EXT_memory_priority': [   {   'VkPhysicalDeviceMemoryPriorityFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT'}],
    'VK_EXT_mesh_shader': [   {   'VkPhysicalDeviceMeshShaderFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT'},
                              {   'VkPhysicalDeviceMeshShaderPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT'}],
    'VK_EXT_multi_draw': [   {   'VkPhysicalDeviceMultiDrawFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_FEATURES_EXT'},
                             {   'VkPhysicalDeviceMultiDrawPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_PROPERTIES_EXT'}],
    'VK_EXT_multisampled_render_to_single_sampled': [   {   'VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_FEATURES_EXT'}],
    'VK_EXT_mutable_descriptor_type': [   {   'VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT'}],
    'VK_EXT_nested_command_buffer': [   {   'VkPhysicalDeviceNestedCommandBufferFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_NESTED_COMMAND_BUFFER_FEATURES_EXT'},
                                        {   'VkPhysicalDeviceNestedCommandBufferPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_NESTED_COMMAND_BUFFER_PROPERTIES_EXT'}],
    'VK_EXT_non_seamless_cube_map': [   {   'VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_NON_SEAMLESS_CUBE_MAP_FEATURES_EXT'}],
    'VK_EXT_opacity_micromap': [   {   'VkPhysicalDeviceOpacityMicromapFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPACITY_MICROMAP_FEATURES_EXT'},
                                   {   'VkPhysicalDeviceOpacityMicromapPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPACITY_MICROMAP_PROPERTIES_EXT'}],
    'VK_EXT_pageable_device_local_memory': [   {   'VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PAGEABLE_DEVICE_LOCAL_MEMORY_FEATURES_EXT'}],
    'VK_EXT_pci_bus_info': [   {   'VkPhysicalDevicePCIBusInfoPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PCI_BUS_INFO_PROPERTIES_EXT'}],
    'VK_EXT_physical_device_drm': [   {   'VkPhysicalDeviceDrmPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRM_PROPERTIES_EXT'}],
    'VK_EXT_pipeline_creation_cache_control': [   {   'VkPhysicalDevicePipelineCreationCacheControlFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES'}],
    'VK_EXT_pipeline_library_group_handles': [   {   'VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_LIBRARY_GROUP_HANDLES_FEATURES_EXT'}],
    'VK_EXT_pipeline_properties': [   {   'VkPhysicalDevicePipelinePropertiesFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_PROPERTIES_FEATURES_EXT'}],
    'VK_EXT_pipeline_protected_access': [   {   'VkPhysicalDevicePipelineProtectedAccessFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_PROTECTED_ACCESS_FEATURES'}],
    'VK_EXT_pipeline_robustness': [   {   'VkPhysicalDevicePipelineRobustnessFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_ROBUSTNESS_FEATURES'},
                                      {   'VkPhysicalDevicePipelineRobustnessPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_ROBUSTNESS_PROPERTIES'}],
    'VK_EXT_present_mode_fifo_latest_ready': [   {   'VkPhysicalDevicePresentModeFifoLatestReadyFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_MODE_FIFO_LATEST_READY_FEATURES_EXT'}],
    'VK_EXT_primitive_topology_list_restart': [   {   'VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVE_TOPOLOGY_LIST_RESTART_FEATURES_EXT'}],
    'VK_EXT_primitives_generated_query': [   {   'VkPhysicalDevicePrimitivesGeneratedQueryFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVES_GENERATED_QUERY_FEATURES_EXT'}],
    'VK_EXT_private_data': [   {   'VkPhysicalDevicePrivateDataFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIVATE_DATA_FEATURES'}],
    'VK_EXT_provoking_vertex': [   {   'VkPhysicalDeviceProvokingVertexFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROVOKING_VERTEX_FEATURES_EXT'},
                                   {   'VkPhysicalDeviceProvokingVertexPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROVOKING_VERTEX_PROPERTIES_EXT'}],
    'VK_EXT_rasterization_order_attachment_access': [   {   'VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_FEATURES_EXT'}],
    'VK_EXT_rgba10x6_formats': [   {   'VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RGBA10X6_FORMATS_FEATURES_EXT'}],
    'VK_EXT_robustness2': [   {   'VkPhysicalDeviceRobustness2FeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_KHR'},
                              {   'VkPhysicalDeviceRobustness2PropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_PROPERTIES_KHR'}],
    'VK_EXT_sample_locations': [   {   'VkPhysicalDeviceSampleLocationsPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLE_LOCATIONS_PROPERTIES_EXT'}],
    'VK_EXT_sampler_filter_minmax': [   {   'VkPhysicalDeviceSamplerFilterMinmaxPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_FILTER_MINMAX_PROPERTIES'}],
    'VK_EXT_scalar_block_layout': [   {   'VkPhysicalDeviceScalarBlockLayoutFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES'}],
    'VK_EXT_shader_atomic_float': [   {   'VkPhysicalDeviceShaderAtomicFloatFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT'}],
    'VK_EXT_shader_atomic_float2': [   {   'VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT'}],
    'VK_EXT_shader_demote_to_helper_invocation': [   {   'VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES'}],
    'VK_EXT_shader_float8': [   {   'VkPhysicalDeviceShaderFloat8FeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT8_FEATURES_EXT'}],
    'VK_EXT_shader_image_atomic_int64': [   {   'VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT'}],
    'VK_EXT_shader_module_identifier': [   {   'VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_MODULE_IDENTIFIER_FEATURES_EXT'},
                                           {   'VkPhysicalDeviceShaderModuleIdentifierPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_MODULE_IDENTIFIER_PROPERTIES_EXT'}],
    'VK_EXT_shader_object': [   {   'VkPhysicalDeviceShaderObjectFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT'},
                                {   'VkPhysicalDeviceShaderObjectPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_PROPERTIES_EXT'}],
    'VK_EXT_shader_replicated_composites': [   {   'VkPhysicalDeviceShaderReplicatedCompositesFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_REPLICATED_COMPOSITES_FEATURES_EXT'}],
    'VK_EXT_shader_tile_image': [   {   'VkPhysicalDeviceShaderTileImageFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TILE_IMAGE_FEATURES_EXT'},
                                    {   'VkPhysicalDeviceShaderTileImagePropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TILE_IMAGE_PROPERTIES_EXT'}],
    'VK_EXT_subgroup_size_control': [   {   'VkPhysicalDeviceSubgroupSizeControlFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES'},
                                        {   'VkPhysicalDeviceSubgroupSizeControlPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES'}],
    'VK_EXT_subpass_merge_feedback': [   {   'VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_MERGE_FEEDBACK_FEATURES_EXT'}],
    'VK_EXT_swapchain_maintenance1': [   {   'VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT'}],
    'VK_EXT_texel_buffer_alignment': [   {   'VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_FEATURES_EXT'},
                                         {   'VkPhysicalDeviceTexelBufferAlignmentPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_PROPERTIES'}],
    'VK_EXT_texture_compression_astc_hdr': [   {   'VkPhysicalDeviceTextureCompressionASTCHDRFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXTURE_COMPRESSION_ASTC_HDR_FEATURES'}],
    'VK_EXT_transform_feedback': [   {   'VkPhysicalDeviceTransformFeedbackFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT'},
                                     {   'VkPhysicalDeviceTransformFeedbackPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_PROPERTIES_EXT'}],
    'VK_EXT_vertex_attribute_divisor': [   {   'VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_PROPERTIES_EXT'},
                                           {   'VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES'}],
    'VK_EXT_vertex_attribute_robustness': [   {   'VkPhysicalDeviceVertexAttributeRobustnessFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_ROBUSTNESS_FEATURES_EXT'}],
    'VK_EXT_vertex_input_dynamic_state': [   {   'VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT'}],
    'VK_EXT_ycbcr_2plane_444_formats': [   {   'VkPhysicalDeviceYcbcr2Plane444FormatsFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_2_PLANE_444_FORMATS_FEATURES_EXT'}],
    'VK_EXT_ycbcr_image_arrays': [   {   'VkPhysicalDeviceYcbcrImageArraysFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_IMAGE_ARRAYS_FEATURES_EXT'}],
    'VK_EXT_zero_initialize_device_memory': [   {   'VkPhysicalDeviceZeroInitializeDeviceMemoryFeaturesEXT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ZERO_INITIALIZE_DEVICE_MEMORY_FEATURES_EXT'}],
    'VK_HUAWEI_cluster_culling_shader': [   {   'VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CLUSTER_CULLING_SHADER_FEATURES_HUAWEI'},
                                            {   'VkPhysicalDeviceClusterCullingShaderPropertiesHUAWEI': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CLUSTER_CULLING_SHADER_PROPERTIES_HUAWEI'}],
    'VK_HUAWEI_hdr_vivid': [   {   'VkPhysicalDeviceHdrVividFeaturesHUAWEI': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HDR_VIVID_FEATURES_HUAWEI'}],
    'VK_HUAWEI_invocation_mask': [   {   'VkPhysicalDeviceInvocationMaskFeaturesHUAWEI': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INVOCATION_MASK_FEATURES_HUAWEI'}],
    'VK_HUAWEI_subpass_shading': [   {   'VkPhysicalDeviceSubpassShadingFeaturesHUAWEI': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_SHADING_FEATURES_HUAWEI'},
                                     {   'VkPhysicalDeviceSubpassShadingPropertiesHUAWEI': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_SHADING_PROPERTIES_HUAWEI'}],
    'VK_IMG_relaxed_line_rasterization': [   {   'VkPhysicalDeviceRelaxedLineRasterizationFeaturesIMG': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RELAXED_LINE_RASTERIZATION_FEATURES_IMG'}],
    'VK_INTEL_shader_integer_functions2': [   {   'VkPhysicalDeviceShaderIntegerFunctions2FeaturesINTEL': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_FUNCTIONS_2_FEATURES_INTEL'}],
    'VK_KHR_16bit_storage': [   {   'VkPhysicalDevice16BitStorageFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES'}],
    'VK_KHR_8bit_storage': [   {   'VkPhysicalDevice8BitStorageFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES'}],
    'VK_KHR_acceleration_structure': [   {   'VkPhysicalDeviceAccelerationStructureFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR'},
                                         {   'VkPhysicalDeviceAccelerationStructurePropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR'}],
    'VK_KHR_buffer_device_address': [   {   'VkPhysicalDeviceBufferDeviceAddressFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES'}],
    'VK_KHR_compute_shader_derivatives': [   {   'VkPhysicalDeviceComputeShaderDerivativesFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_KHR'},
                                             {   'VkPhysicalDeviceComputeShaderDerivativesPropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_PROPERTIES_KHR'}],
    'VK_KHR_cooperative_matrix': [   {   'VkPhysicalDeviceCooperativeMatrixFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_KHR'},
                                     {   'VkPhysicalDeviceCooperativeMatrixPropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_PROPERTIES_KHR'}],
    'VK_KHR_depth_clamp_zero_one': [   {   'VkPhysicalDeviceDepthClampZeroOneFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLAMP_ZERO_ONE_FEATURES_KHR'}],
    'VK_KHR_depth_stencil_resolve': [   {   'VkPhysicalDeviceDepthStencilResolvePropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES'}],
    'VK_KHR_driver_properties': [   {   'VkPhysicalDeviceDriverPropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES'}],
    'VK_KHR_dynamic_rendering': [   {   'VkPhysicalDeviceDynamicRenderingFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES'}],
    'VK_KHR_dynamic_rendering_local_read': [   {   'VkPhysicalDeviceDynamicRenderingLocalReadFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_LOCAL_READ_FEATURES'}],
    'VK_KHR_external_fence_capabilities': [   {   'VkPhysicalDeviceIDPropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES'}],
    'VK_KHR_external_memory_capabilities': [   {   'VkPhysicalDeviceIDPropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES'}],
    'VK_KHR_external_semaphore_capabilities': [   {   'VkPhysicalDeviceIDPropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES'}],
    'VK_KHR_fragment_shader_barycentric': [   {   'VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR'},
                                              {   'VkPhysicalDeviceFragmentShaderBarycentricPropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_PROPERTIES_KHR'}],
    'VK_KHR_fragment_shading_rate': [   {   'VkPhysicalDeviceFragmentShadingRateFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR'},
                                        {   'VkPhysicalDeviceFragmentShadingRatePropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR'}],
    'VK_KHR_global_priority': [   {   'VkPhysicalDeviceGlobalPriorityQueryFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GLOBAL_PRIORITY_QUERY_FEATURES'}],
    'VK_KHR_imageless_framebuffer': [   {   'VkPhysicalDeviceImagelessFramebufferFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES'}],
    'VK_KHR_index_type_uint8': [   {   'VkPhysicalDeviceIndexTypeUint8FeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES'}],
    'VK_KHR_line_rasterization': [   {   'VkPhysicalDeviceLineRasterizationFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES'},
                                     {   'VkPhysicalDeviceLineRasterizationPropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_PROPERTIES'}],
    'VK_KHR_maintenance2': [   {   'VkPhysicalDevicePointClippingPropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_POINT_CLIPPING_PROPERTIES'}],
    'VK_KHR_maintenance3': [   {   'VkPhysicalDeviceMaintenance3PropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES'}],
    'VK_KHR_maintenance4': [   {   'VkPhysicalDeviceMaintenance4FeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES'},
                               {   'VkPhysicalDeviceMaintenance4PropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES'}],
    'VK_KHR_maintenance5': [   {   'VkPhysicalDeviceMaintenance5FeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES'},
                               {   'VkPhysicalDeviceMaintenance5PropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_PROPERTIES'}],
    'VK_KHR_maintenance6': [   {   'VkPhysicalDeviceMaintenance6FeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_6_FEATURES'},
                               {   'VkPhysicalDeviceMaintenance6PropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_6_PROPERTIES'}],
    'VK_KHR_maintenance7': [   {   'VkPhysicalDeviceMaintenance7FeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_7_FEATURES_KHR'},
                               {   'VkPhysicalDeviceMaintenance7PropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_7_PROPERTIES_KHR'},
                               {   'VkPhysicalDeviceLayeredApiPropertiesListKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LAYERED_API_PROPERTIES_LIST_KHR'}],
    'VK_KHR_maintenance8': [   {   'VkPhysicalDeviceMaintenance8FeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_8_FEATURES_KHR'}],
    'VK_KHR_maintenance9': [   {   'VkPhysicalDeviceMaintenance9FeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_9_FEATURES_KHR'},
                               {   'VkPhysicalDeviceMaintenance9PropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_9_PROPERTIES_KHR'}],
    'VK_KHR_multiview': [   {   'VkPhysicalDeviceMultiviewFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES'},
                            {   'VkPhysicalDeviceMultiviewPropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES'}],
    'VK_KHR_performance_query': [   {   'VkPhysicalDevicePerformanceQueryFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR'},
                                    {   'VkPhysicalDevicePerformanceQueryPropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_PROPERTIES_KHR'}],
    'VK_KHR_pipeline_binary': [   {   'VkPhysicalDevicePipelineBinaryFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_BINARY_FEATURES_KHR'},
                                  {   'VkPhysicalDevicePipelineBinaryPropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_BINARY_PROPERTIES_KHR'}],
    'VK_KHR_pipeline_executable_properties': [   {   'VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR'}],
    'VK_KHR_present_id': [   {   'VkPhysicalDevicePresentIdFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR'}],
    'VK_KHR_present_id2': [   {   'VkPhysicalDevicePresentId2FeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_2_FEATURES_KHR'}],
    'VK_KHR_present_wait': [   {   'VkPhysicalDevicePresentWaitFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR'}],
    'VK_KHR_present_wait2': [   {   'VkPhysicalDevicePresentWait2FeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_2_FEATURES_KHR'}],
    'VK_KHR_push_descriptor': [   {   'VkPhysicalDevicePushDescriptorPropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES'}],
    'VK_KHR_ray_query': [   {   'VkPhysicalDeviceRayQueryFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR'}],
    'VK_KHR_ray_tracing_maintenance1': [   {   'VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MAINTENANCE_1_FEATURES_KHR'}],
    'VK_KHR_ray_tracing_pipeline': [   {   'VkPhysicalDeviceRayTracingPipelineFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR'},
                                       {   'VkPhysicalDeviceRayTracingPipelinePropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR'}],
    'VK_KHR_ray_tracing_position_fetch': [   {   'VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_POSITION_FETCH_FEATURES_KHR'}],
    'VK_KHR_robustness2': [   {   'VkPhysicalDeviceRobustness2FeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_KHR'},
                              {   'VkPhysicalDeviceRobustness2PropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_PROPERTIES_KHR'}],
    'VK_KHR_sampler_ycbcr_conversion': [   {   'VkPhysicalDeviceSamplerYcbcrConversionFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES'}],
    'VK_KHR_separate_depth_stencil_layouts': [   {   'VkPhysicalDeviceSeparateDepthStencilLayoutsFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES'}],
    'VK_KHR_shader_atomic_int64': [   {   'VkPhysicalDeviceShaderAtomicInt64FeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES'}],
    'VK_KHR_shader_bfloat16': [   {   'VkPhysicalDeviceShaderBfloat16FeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_BFLOAT16_FEATURES_KHR'}],
    'VK_KHR_shader_clock': [   {   'VkPhysicalDeviceShaderClockFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR'}],
    'VK_KHR_shader_expect_assume': [   {   'VkPhysicalDeviceShaderExpectAssumeFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_EXPECT_ASSUME_FEATURES'}],
    'VK_KHR_shader_float16_int8': [   {   'VkPhysicalDeviceShaderFloat16Int8FeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES'},
                                      {   'VkPhysicalDeviceFloat16Int8FeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES'}],
    'VK_KHR_shader_float_controls': [   {   'VkPhysicalDeviceFloatControlsPropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES'}],
    'VK_KHR_shader_float_controls2': [   {   'VkPhysicalDeviceShaderFloatControls2FeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT_CONTROLS_2_FEATURES'}],
    'VK_KHR_shader_integer_dot_product': [   {   'VkPhysicalDeviceShaderIntegerDotProductFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_FEATURES'},
                                             {   'VkPhysicalDeviceShaderIntegerDotProductPropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_PROPERTIES'}],
    'VK_KHR_shader_maximal_reconvergence': [   {   'VkPhysicalDeviceShaderMaximalReconvergenceFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_MAXIMAL_RECONVERGENCE_FEATURES_KHR'}],
    'VK_KHR_shader_quad_control': [   {   'VkPhysicalDeviceShaderQuadControlFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_QUAD_CONTROL_FEATURES_KHR'}],
    'VK_KHR_shader_relaxed_extended_instruction': [   {   'VkPhysicalDeviceShaderRelaxedExtendedInstructionFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_RELAXED_EXTENDED_INSTRUCTION_FEATURES_KHR'}],
    'VK_KHR_shader_subgroup_extended_types': [   {   'VkPhysicalDeviceShaderSubgroupExtendedTypesFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES'}],
    'VK_KHR_shader_subgroup_rotate': [   {   'VkPhysicalDeviceShaderSubgroupRotateFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_ROTATE_FEATURES'}],
    'VK_KHR_shader_subgroup_uniform_control_flow': [   {   'VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_UNIFORM_CONTROL_FLOW_FEATURES_KHR'}],
    'VK_KHR_shader_terminate_invocation': [   {   'VkPhysicalDeviceShaderTerminateInvocationFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TERMINATE_INVOCATION_FEATURES'}],
    'VK_KHR_synchronization2': [   {   'VkPhysicalDeviceSynchronization2FeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES'}],
    'VK_KHR_timeline_semaphore': [   {   'VkPhysicalDeviceTimelineSemaphoreFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES'},
                                     {   'VkPhysicalDeviceTimelineSemaphorePropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_PROPERTIES'}],
    'VK_KHR_unified_image_layouts': [   {   'VkPhysicalDeviceUnifiedImageLayoutsFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFIED_IMAGE_LAYOUTS_FEATURES_KHR'}],
    'VK_KHR_uniform_buffer_standard_layout': [   {   'VkPhysicalDeviceUniformBufferStandardLayoutFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES'}],
    'VK_KHR_variable_pointers': [   {   'VkPhysicalDeviceVariablePointerFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES'},
                                    {   'VkPhysicalDeviceVariablePointersFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES'}],
    'VK_KHR_vertex_attribute_divisor': [   {   'VkPhysicalDeviceVertexAttributeDivisorPropertiesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_PROPERTIES'},
                                           {   'VkPhysicalDeviceVertexAttributeDivisorFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES'}],
    'VK_KHR_video_decode_vp9': [   {   'VkPhysicalDeviceVideoDecodeVP9FeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VIDEO_DECODE_VP9_FEATURES_KHR'}],
    'VK_KHR_video_encode_av1': [   {   'VkPhysicalDeviceVideoEncodeAV1FeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VIDEO_ENCODE_AV1_FEATURES_KHR'}],
    'VK_KHR_video_encode_quantization_map': [   {   'VkPhysicalDeviceVideoEncodeQuantizationMapFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VIDEO_ENCODE_QUANTIZATION_MAP_FEATURES_KHR'}],
    'VK_KHR_video_maintenance1': [   {   'VkPhysicalDeviceVideoMaintenance1FeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VIDEO_MAINTENANCE_1_FEATURES_KHR'}],
    'VK_KHR_video_maintenance2': [   {   'VkPhysicalDeviceVideoMaintenance2FeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VIDEO_MAINTENANCE_2_FEATURES_KHR'}],
    'VK_KHR_vulkan_memory_model': [   {   'VkPhysicalDeviceVulkanMemoryModelFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES'}],
    'VK_KHR_workgroup_memory_explicit_layout': [   {   'VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_WORKGROUP_MEMORY_EXPLICIT_LAYOUT_FEATURES_KHR'}],
    'VK_KHR_zero_initialize_workgroup_memory': [   {   'VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeaturesKHR': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ZERO_INITIALIZE_WORKGROUP_MEMORY_FEATURES'}],
    'VK_MESA_image_alignment_control': [   {   'VkPhysicalDeviceImageAlignmentControlFeaturesMESA': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ALIGNMENT_CONTROL_FEATURES_MESA'},
                                           {   'VkPhysicalDeviceImageAlignmentControlPropertiesMESA': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ALIGNMENT_CONTROL_PROPERTIES_MESA'}],
    'VK_MSFT_layered_driver': [   {   'VkPhysicalDeviceLayeredDriverPropertiesMSFT': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LAYERED_DRIVER_PROPERTIES_MSFT'}],
    'VK_NVX_multiview_per_view_attributes': [   {   'VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PER_VIEW_ATTRIBUTES_PROPERTIES_NVX'}],
    'VK_NV_cluster_acceleration_structure': [   {   'VkPhysicalDeviceClusterAccelerationStructureFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CLUSTER_ACCELERATION_STRUCTURE_FEATURES_NV'},
                                                {   'VkPhysicalDeviceClusterAccelerationStructurePropertiesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CLUSTER_ACCELERATION_STRUCTURE_PROPERTIES_NV'}],
    'VK_NV_command_buffer_inheritance': [   {   'VkPhysicalDeviceCommandBufferInheritanceFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMMAND_BUFFER_INHERITANCE_FEATURES_NV'}],
    'VK_NV_compute_shader_derivatives': [   {   'VkPhysicalDeviceComputeShaderDerivativesFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_KHR'}],
    'VK_NV_cooperative_matrix': [   {   'VkPhysicalDeviceCooperativeMatrixFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_NV'},
                                    {   'VkPhysicalDeviceCooperativeMatrixPropertiesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_PROPERTIES_NV'}],
    'VK_NV_cooperative_matrix2': [   {   'VkPhysicalDeviceCooperativeMatrix2FeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_2_FEATURES_NV'},
                                     {   'VkPhysicalDeviceCooperativeMatrix2PropertiesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_2_PROPERTIES_NV'}],
    'VK_NV_cooperative_vector': [   {   'VkPhysicalDeviceCooperativeVectorPropertiesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_VECTOR_PROPERTIES_NV'},
                                    {   'VkPhysicalDeviceCooperativeVectorFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_VECTOR_FEATURES_NV'}],
    'VK_NV_copy_memory_indirect': [   {   'VkPhysicalDeviceCopyMemoryIndirectFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COPY_MEMORY_INDIRECT_FEATURES_NV'},
                                      {   'VkPhysicalDeviceCopyMemoryIndirectPropertiesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COPY_MEMORY_INDIRECT_PROPERTIES_NV'}],
    'VK_NV_corner_sampled_image': [   {   'VkPhysicalDeviceCornerSampledImageFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CORNER_SAMPLED_IMAGE_FEATURES_NV'}],
    'VK_NV_coverage_reduction_mode': [   {   'VkPhysicalDeviceCoverageReductionModeFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COVERAGE_REDUCTION_MODE_FEATURES_NV'}],
    'VK_NV_dedicated_allocation_image_aliasing': [   {   'VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEDICATED_ALLOCATION_IMAGE_ALIASING_FEATURES_NV'}],
    'VK_NV_descriptor_pool_overallocation': [   {   'VkPhysicalDeviceDescriptorPoolOverallocationFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_POOL_OVERALLOCATION_FEATURES_NV'}],
    'VK_NV_device_diagnostics_config': [   {   'VkPhysicalDeviceDiagnosticsConfigFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DIAGNOSTICS_CONFIG_FEATURES_NV'}],
    'VK_NV_device_generated_commands': [   {   'VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_PROPERTIES_NV'},
                                           {   'VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_FEATURES_NV'}],
    'VK_NV_device_generated_commands_compute': [   {   'VkPhysicalDeviceDeviceGeneratedCommandsComputeFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_COMPUTE_FEATURES_NV'}],
    'VK_NV_extended_sparse_address_space': [   {   'VkPhysicalDeviceExtendedSparseAddressSpaceFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_SPARSE_ADDRESS_SPACE_FEATURES_NV'},
                                               {   'VkPhysicalDeviceExtendedSparseAddressSpacePropertiesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_SPARSE_ADDRESS_SPACE_PROPERTIES_NV'}],
    'VK_NV_external_compute_queue': [   {   'VkPhysicalDeviceExternalComputeQueuePropertiesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_COMPUTE_QUEUE_PROPERTIES_NV'}],
    'VK_NV_external_memory_rdma': [   {   'VkPhysicalDeviceExternalMemoryRDMAFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_RDMA_FEATURES_NV'}],
    'VK_NV_fragment_shading_rate_enums': [   {   'VkPhysicalDeviceFragmentShadingRateEnumsFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_ENUMS_FEATURES_NV'},
                                             {   'VkPhysicalDeviceFragmentShadingRateEnumsPropertiesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_ENUMS_PROPERTIES_NV'}],
    'VK_NV_inherited_viewport_scissor': [   {   'VkPhysicalDeviceInheritedViewportScissorFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INHERITED_VIEWPORT_SCISSOR_FEATURES_NV'}],
    'VK_NV_linear_color_attachment': [   {   'VkPhysicalDeviceLinearColorAttachmentFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINEAR_COLOR_ATTACHMENT_FEATURES_NV'}],
    'VK_NV_memory_decompression': [   {   'VkPhysicalDeviceMemoryDecompressionFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_DECOMPRESSION_FEATURES_NV'},
                                      {   'VkPhysicalDeviceMemoryDecompressionPropertiesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_DECOMPRESSION_PROPERTIES_NV'}],
    'VK_NV_mesh_shader': [   {   'VkPhysicalDeviceMeshShaderFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV'},
                             {   'VkPhysicalDeviceMeshShaderPropertiesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV'}],
    'VK_NV_optical_flow': [   {   'VkPhysicalDeviceOpticalFlowFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPTICAL_FLOW_FEATURES_NV'},
                              {   'VkPhysicalDeviceOpticalFlowPropertiesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPTICAL_FLOW_PROPERTIES_NV'}],
    'VK_NV_partitioned_acceleration_structure': [   {   'VkPhysicalDevicePartitionedAccelerationStructureFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PARTITIONED_ACCELERATION_STRUCTURE_FEATURES_NV'},
                                                    {   'VkPhysicalDevicePartitionedAccelerationStructurePropertiesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PARTITIONED_ACCELERATION_STRUCTURE_PROPERTIES_NV'}],
    'VK_NV_per_stage_descriptor_set': [   {   'VkPhysicalDevicePerStageDescriptorSetFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PER_STAGE_DESCRIPTOR_SET_FEATURES_NV'}],
    'VK_NV_present_barrier': [   {   'VkPhysicalDevicePresentBarrierFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_BARRIER_FEATURES_NV'}],
    'VK_NV_raw_access_chains': [   {   'VkPhysicalDeviceRawAccessChainsFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAW_ACCESS_CHAINS_FEATURES_NV'}],
    'VK_NV_ray_tracing': [   {   'VkPhysicalDeviceRayTracingPropertiesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV'}],
    'VK_NV_ray_tracing_invocation_reorder': [   {   'VkPhysicalDeviceRayTracingInvocationReorderPropertiesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_INVOCATION_REORDER_PROPERTIES_NV'},
                                                {   'VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_INVOCATION_REORDER_FEATURES_NV'}],
    'VK_NV_ray_tracing_linear_swept_spheres': [   {   'VkPhysicalDeviceRayTracingLinearSweptSpheresFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_LINEAR_SWEPT_SPHERES_FEATURES_NV'}],
    'VK_NV_ray_tracing_motion_blur': [   {   'VkPhysicalDeviceRayTracingMotionBlurFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MOTION_BLUR_FEATURES_NV'}],
    'VK_NV_ray_tracing_validation': [   {   'VkPhysicalDeviceRayTracingValidationFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_VALIDATION_FEATURES_NV'}],
    'VK_NV_representative_fragment_test': [   {   'VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_REPRESENTATIVE_FRAGMENT_TEST_FEATURES_NV'}],
    'VK_NV_scissor_exclusive': [   {   'VkPhysicalDeviceExclusiveScissorFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXCLUSIVE_SCISSOR_FEATURES_NV'}],
    'VK_NV_shader_atomic_float16_vector': [   {   'VkPhysicalDeviceShaderAtomicFloat16VectorFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT16_VECTOR_FEATURES_NV'}],
    'VK_NV_shader_image_footprint': [   {   'VkPhysicalDeviceShaderImageFootprintFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_FOOTPRINT_FEATURES_NV'}],
    'VK_NV_shader_sm_builtins': [   {   'VkPhysicalDeviceShaderSMBuiltinsPropertiesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_PROPERTIES_NV'},
                                    {   'VkPhysicalDeviceShaderSMBuiltinsFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_FEATURES_NV'}],
    'VK_NV_shading_rate_image': [   {   'VkPhysicalDeviceShadingRateImageFeaturesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV'},
                                    {   'VkPhysicalDeviceShadingRateImagePropertiesNV': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_PROPERTIES_NV'}],
    'VK_QCOM_filter_cubic_clamp': [   {   'VkPhysicalDeviceCubicClampFeaturesQCOM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUBIC_CLAMP_FEATURES_QCOM'}],
    'VK_QCOM_filter_cubic_weights': [   {   'VkPhysicalDeviceCubicWeightsFeaturesQCOM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUBIC_WEIGHTS_FEATURES_QCOM'}],
    'VK_QCOM_fragment_density_map_offset': [   {   'VkPhysicalDeviceFragmentDensityMapOffsetFeaturesQCOM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_OFFSET_FEATURES_EXT'},
                                               {   'VkPhysicalDeviceFragmentDensityMapOffsetPropertiesQCOM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_OFFSET_PROPERTIES_EXT'}],
    'VK_QCOM_image_processing': [   {   'VkPhysicalDeviceImageProcessingFeaturesQCOM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_PROCESSING_FEATURES_QCOM'},
                                    {   'VkPhysicalDeviceImageProcessingPropertiesQCOM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_PROCESSING_PROPERTIES_QCOM'}],
    'VK_QCOM_image_processing2': [   {   'VkPhysicalDeviceImageProcessing2FeaturesQCOM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_PROCESSING_2_FEATURES_QCOM'},
                                     {   'VkPhysicalDeviceImageProcessing2PropertiesQCOM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_PROCESSING_2_PROPERTIES_QCOM'}],
    'VK_QCOM_multiview_per_view_render_areas': [   {   'VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PER_VIEW_RENDER_AREAS_FEATURES_QCOM'}],
    'VK_QCOM_multiview_per_view_viewports': [   {   'VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PER_VIEW_VIEWPORTS_FEATURES_QCOM'}],
    'VK_QCOM_tile_memory_heap': [   {   'VkPhysicalDeviceTileMemoryHeapFeaturesQCOM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TILE_MEMORY_HEAP_FEATURES_QCOM'},
                                    {   'VkPhysicalDeviceTileMemoryHeapPropertiesQCOM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TILE_MEMORY_HEAP_PROPERTIES_QCOM'}],
    'VK_QCOM_tile_properties': [   {   'VkPhysicalDeviceTilePropertiesFeaturesQCOM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TILE_PROPERTIES_FEATURES_QCOM'}],
    'VK_QCOM_tile_shading': [   {   'VkPhysicalDeviceTileShadingFeaturesQCOM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TILE_SHADING_FEATURES_QCOM'},
                                {   'VkPhysicalDeviceTileShadingPropertiesQCOM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TILE_SHADING_PROPERTIES_QCOM'}],
    'VK_QCOM_ycbcr_degamma': [   {   'VkPhysicalDeviceYcbcrDegammaFeaturesQCOM': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_DEGAMMA_FEATURES_QCOM'}],
    'VK_SEC_amigo_profiling': [   {   'VkPhysicalDeviceAmigoProfilingFeaturesSEC': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_AMIGO_PROFILING_FEATURES_SEC'}],
    'VK_VALVE_descriptor_set_host_mapping': [   {   'VkPhysicalDeviceDescriptorSetHostMappingFeaturesVALVE': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_SET_HOST_MAPPING_FEATURES_VALVE'}],
    'VK_VALVE_mutable_descriptor_type': [   {   'VkPhysicalDeviceMutableDescriptorTypeFeaturesVALVE': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT'}]}}

# --- Vulkan Feature to Struct Mappings ---
# VULKAN_VERSIONS_AND_STRUCTS_MAPPING: Maps API features (e.g., "VK_API_VERSION_1_1")
# from "vulkan" API to PhysicalDevice structs they introduce.
# Excludes core version structs (e.g., VkPhysicalDeviceVulkan11Properties), see VULKAN_CORES_AND_STRUCTS_MAPPING.
# Struct Filters (per feature):
# - Name: Starts "VkPhysicalDevice".
# - Not in global 'disabled_structs'.
# - Extends "VkPhysicalDeviceProperties2" or "VkPhysicalDeviceFeatures2".
# - Not in CORE_MAPPING_STRUCT_LIST.
# NOTE:
# VK_VERSION_1_0" is empty as it does not map to any structure which passes our structure-filter criteria.
# We have hardcoded the code-block for VK_VERSION_1_0 in vkjson_generator.py
# Format: {feature_name: [{struct_name: sType_enum_value}, ...]}
VULKAN_VERSIONS_AND_STRUCTS_MAPPING = {   'VK_VERSION_1_0': [],
    'VK_VERSION_1_1': [   {   'VkPhysicalDeviceSubgroupProperties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES'},
                          {   'VkPhysicalDevice16BitStorageFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES'},
                          {   'VkPhysicalDevicePointClippingProperties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_POINT_CLIPPING_PROPERTIES'},
                          {   'VkPhysicalDeviceMultiviewFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES'},
                          {   'VkPhysicalDeviceMultiviewProperties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES'},
                          {   'VkPhysicalDeviceVariablePointerFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES'},
                          {   'VkPhysicalDeviceVariablePointersFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES'},
                          {   'VkPhysicalDeviceProtectedMemoryFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES'},
                          {   'VkPhysicalDeviceProtectedMemoryProperties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_PROPERTIES'},
                          {   'VkPhysicalDeviceSamplerYcbcrConversionFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES'},
                          {   'VkPhysicalDeviceIDProperties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES'},
                          {   'VkPhysicalDeviceMaintenance3Properties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES'},
                          {   'VkPhysicalDeviceShaderDrawParameterFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES'},
                          {   'VkPhysicalDeviceShaderDrawParametersFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES'}],
    'VK_VERSION_1_2': [   {   'VkPhysicalDevice8BitStorageFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES'},
                          {   'VkPhysicalDeviceDriverProperties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES'},
                          {   'VkPhysicalDeviceShaderAtomicInt64Features': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES'},
                          {   'VkPhysicalDeviceShaderFloat16Int8Features': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES'},
                          {   'VkPhysicalDeviceFloatControlsProperties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES'},
                          {   'VkPhysicalDeviceDescriptorIndexingFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES'},
                          {   'VkPhysicalDeviceDescriptorIndexingProperties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES'},
                          {   'VkPhysicalDeviceDepthStencilResolveProperties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES'},
                          {   'VkPhysicalDeviceScalarBlockLayoutFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES'},
                          {   'VkPhysicalDeviceSamplerFilterMinmaxProperties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_FILTER_MINMAX_PROPERTIES'},
                          {   'VkPhysicalDeviceVulkanMemoryModelFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES'},
                          {   'VkPhysicalDeviceImagelessFramebufferFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES'},
                          {   'VkPhysicalDeviceUniformBufferStandardLayoutFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES'},
                          {   'VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES'},
                          {   'VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES'},
                          {   'VkPhysicalDeviceHostQueryResetFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES'},
                          {   'VkPhysicalDeviceTimelineSemaphoreFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES'},
                          {   'VkPhysicalDeviceTimelineSemaphoreProperties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_PROPERTIES'},
                          {   'VkPhysicalDeviceBufferDeviceAddressFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES'}],
    'VK_VERSION_1_3': [   {   'VkPhysicalDeviceShaderTerminateInvocationFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TERMINATE_INVOCATION_FEATURES'},
                          {   'VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES'},
                          {   'VkPhysicalDevicePrivateDataFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIVATE_DATA_FEATURES'},
                          {   'VkPhysicalDevicePipelineCreationCacheControlFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES'},
                          {   'VkPhysicalDeviceSynchronization2Features': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES'},
                          {   'VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ZERO_INITIALIZE_WORKGROUP_MEMORY_FEATURES'},
                          {   'VkPhysicalDeviceImageRobustnessFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ROBUSTNESS_FEATURES'},
                          {   'VkPhysicalDeviceSubgroupSizeControlFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES'},
                          {   'VkPhysicalDeviceSubgroupSizeControlProperties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES'},
                          {   'VkPhysicalDeviceInlineUniformBlockFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES'},
                          {   'VkPhysicalDeviceInlineUniformBlockProperties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_PROPERTIES'},
                          {   'VkPhysicalDeviceTextureCompressionASTCHDRFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXTURE_COMPRESSION_ASTC_HDR_FEATURES'},
                          {   'VkPhysicalDeviceDynamicRenderingFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES'},
                          {   'VkPhysicalDeviceShaderIntegerDotProductFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_FEATURES'},
                          {   'VkPhysicalDeviceShaderIntegerDotProductProperties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_PROPERTIES'},
                          {   'VkPhysicalDeviceTexelBufferAlignmentProperties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_PROPERTIES'},
                          {   'VkPhysicalDeviceMaintenance4Features': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES'},
                          {   'VkPhysicalDeviceMaintenance4Properties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES'}],
    'VK_VERSION_1_4': [   {   'VkPhysicalDeviceGlobalPriorityQueryFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GLOBAL_PRIORITY_QUERY_FEATURES'},
                          {   'VkPhysicalDeviceShaderSubgroupRotateFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_ROTATE_FEATURES'},
                          {   'VkPhysicalDeviceShaderFloatControls2Features': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT_CONTROLS_2_FEATURES'},
                          {   'VkPhysicalDeviceShaderExpectAssumeFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_EXPECT_ASSUME_FEATURES'},
                          {   'VkPhysicalDeviceLineRasterizationFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES'},
                          {   'VkPhysicalDeviceLineRasterizationProperties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_PROPERTIES'},
                          {   'VkPhysicalDeviceVertexAttributeDivisorProperties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_PROPERTIES'},
                          {   'VkPhysicalDeviceVertexAttributeDivisorFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES'},
                          {   'VkPhysicalDeviceIndexTypeUint8Features': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES'},
                          {   'VkPhysicalDeviceMaintenance5Features': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES'},
                          {   'VkPhysicalDeviceMaintenance5Properties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_PROPERTIES'},
                          {   'VkPhysicalDevicePushDescriptorProperties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES'},
                          {   'VkPhysicalDeviceDynamicRenderingLocalReadFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_LOCAL_READ_FEATURES'},
                          {   'VkPhysicalDeviceMaintenance6Features': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_6_FEATURES'},
                          {   'VkPhysicalDeviceMaintenance6Properties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_6_PROPERTIES'},
                          {   'VkPhysicalDevicePipelineProtectedAccessFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_PROTECTED_ACCESS_FEATURES'},
                          {   'VkPhysicalDevicePipelineRobustnessFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_ROBUSTNESS_FEATURES'},
                          {   'VkPhysicalDevicePipelineRobustnessProperties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_ROBUSTNESS_PROPERTIES'},
                          {   'VkPhysicalDeviceHostImageCopyFeatures': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_IMAGE_COPY_FEATURES'},
                          {   'VkPhysicalDeviceHostImageCopyProperties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_IMAGE_COPY_PROPERTIES'}]}


# --- Extension Independent Structs ---
# These are PhysicalDevice structs that meet the following criteria:
# 1. Sourced from VK_PHYSICAL_STRUCT_NAMES.
# 2. Not in the global 'disabled_structs' list.
# 3. Extend "VkPhysicalDeviceProperties2" or "VkPhysicalDeviceFeatures2" (in 'structs_with_valid_extends').
# 4. Not core version-specific (not in CORE_MAPPING_STRUCT_LIST).
# 5. Not required by any enabled Vulkan extension (not in 'structs_in_extensions').
EXTENSION_INDEPENDENT_STRUCTS = [   'VkPhysicalDevice16BitStorageFeatures',
    'VkPhysicalDevice8BitStorageFeatures',
    'VkPhysicalDeviceBufferDeviceAddressFeatures',
    'VkPhysicalDeviceDepthStencilResolveProperties',
    'VkPhysicalDeviceDescriptorIndexingFeatures',
    'VkPhysicalDeviceDescriptorIndexingProperties',
    'VkPhysicalDeviceDriverProperties',
    'VkPhysicalDeviceDynamicRenderingFeatures',
    'VkPhysicalDeviceDynamicRenderingLocalReadFeatures',
    'VkPhysicalDeviceFloatControlsProperties',
    'VkPhysicalDeviceGlobalPriorityQueryFeatures',
    'VkPhysicalDeviceHostImageCopyFeatures',
    'VkPhysicalDeviceHostImageCopyProperties',
    'VkPhysicalDeviceHostQueryResetFeatures',
    'VkPhysicalDeviceIDProperties',
    'VkPhysicalDeviceImageRobustnessFeatures',
    'VkPhysicalDeviceImagelessFramebufferFeatures',
    'VkPhysicalDeviceIndexTypeUint8Features',
    'VkPhysicalDeviceInlineUniformBlockFeatures',
    'VkPhysicalDeviceInlineUniformBlockProperties',
    'VkPhysicalDeviceLineRasterizationFeatures',
    'VkPhysicalDeviceLineRasterizationProperties',
    'VkPhysicalDeviceMaintenance3Properties',
    'VkPhysicalDeviceMaintenance4Features',
    'VkPhysicalDeviceMaintenance4Properties',
    'VkPhysicalDeviceMaintenance5Features',
    'VkPhysicalDeviceMaintenance5Properties',
    'VkPhysicalDeviceMaintenance6Features',
    'VkPhysicalDeviceMaintenance6Properties',
    'VkPhysicalDeviceMultiviewFeatures',
    'VkPhysicalDeviceMultiviewProperties',
    'VkPhysicalDevicePipelineCreationCacheControlFeatures',
    'VkPhysicalDevicePipelineProtectedAccessFeatures',
    'VkPhysicalDevicePipelineRobustnessFeatures',
    'VkPhysicalDevicePipelineRobustnessProperties',
    'VkPhysicalDevicePointClippingProperties',
    'VkPhysicalDevicePrivateDataFeatures',
    'VkPhysicalDeviceProtectedMemoryFeatures',
    'VkPhysicalDeviceProtectedMemoryProperties',
    'VkPhysicalDevicePushDescriptorProperties',
    'VkPhysicalDeviceSamplerFilterMinmaxProperties',
    'VkPhysicalDeviceSamplerYcbcrConversionFeatures',
    'VkPhysicalDeviceScalarBlockLayoutFeatures',
    'VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures',
    'VkPhysicalDeviceShaderAtomicInt64Features',
    'VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures',
    'VkPhysicalDeviceShaderDrawParameterFeatures',
    'VkPhysicalDeviceShaderDrawParametersFeatures',
    'VkPhysicalDeviceShaderExpectAssumeFeatures',
    'VkPhysicalDeviceShaderFloat16Int8Features',
    'VkPhysicalDeviceShaderFloatControls2Features',
    'VkPhysicalDeviceShaderIntegerDotProductFeatures',
    'VkPhysicalDeviceShaderIntegerDotProductProperties',
    'VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures',
    'VkPhysicalDeviceShaderSubgroupRotateFeatures',
    'VkPhysicalDeviceShaderTerminateInvocationFeatures',
    'VkPhysicalDeviceSubgroupProperties',
    'VkPhysicalDeviceSubgroupSizeControlFeatures',
    'VkPhysicalDeviceSubgroupSizeControlProperties',
    'VkPhysicalDeviceSynchronization2Features',
    'VkPhysicalDeviceTexelBufferAlignmentProperties',
    'VkPhysicalDeviceTextureCompressionASTCHDRFeatures',
    'VkPhysicalDeviceTimelineSemaphoreFeatures',
    'VkPhysicalDeviceTimelineSemaphoreProperties',
    'VkPhysicalDeviceUniformBufferStandardLayoutFeatures',
    'VkPhysicalDeviceVariablePointerFeatures',
    'VkPhysicalDeviceVariablePointersFeatures',
    'VkPhysicalDeviceVertexAttributeDivisorFeatures',
    'VkPhysicalDeviceVertexAttributeDivisorProperties',
    'VkPhysicalDeviceVulkanMemoryModelFeatures',
    'VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures']


# --- Vulkan Core Version to Struct Mappings ---
# VULKAN_CORES_AND_STRUCTS_MAPPING: Maps core Vulkan versions (e.g., "Core11") to their
# specific PhysicalDevice Properties and Features structs.
# Struct Filters:
# - Name matches "VkPhysicalDeviceVulkan<Version><Properties|Features>" (from CORE_MAPPING_STRUCT_LIST).
# - sType is programmatically derived.
# Format: {"versions": {core_version_key: [{struct_name: sType_enum_value}, ...]}}
VULKAN_CORES_AND_STRUCTS_MAPPING = {"versions": {   'Core11': [   {   'VkPhysicalDeviceVulkan11Features': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES'},
                  {   'VkPhysicalDeviceVulkan11Properties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES'}],
    'Core12': [   {   'VkPhysicalDeviceVulkan12Features': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES'},
                  {   'VkPhysicalDeviceVulkan12Properties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES'}],
    'Core13': [   {   'VkPhysicalDeviceVulkan13Features': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES'},
                  {   'VkPhysicalDeviceVulkan13Properties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES'}],
    'Core14': [   {   'VkPhysicalDeviceVulkan14Features': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES'},
                  {   'VkPhysicalDeviceVulkan14Properties': 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES'}]}}

# --- List Size Mappings (Field name to size field name) ---
LIST_TYPE_FIELD_AND_SIZE_MAPPING = {   'memoryHeaps': 'memoryHeapCount',
    'memoryTypes': 'memoryTypeCount',
    'pCopyDstLayouts': 'copyDstLayoutCount',
    'pCopySrcLayouts': 'copySrcLayoutCount',
    'pLayeredApis': 'layeredApiCount',
    'physicalDevices': 'physicalDeviceCount'}

STRUCT_WITH_DYNAMIC_SIZE_LIST_MAPPING = {   'VkPhysicalDeviceHostImageCopyProperties': ['pCopyDstLayouts', 'pCopySrcLayouts'],
    'VkPhysicalDeviceLayeredApiPropertiesListKHR': ['pLayeredApis'],
    'VkPhysicalDeviceVulkan14Properties': ['pCopyDstLayouts', 'pCopySrcLayouts']}

# --- STRUCTS USED BY VULKAN_API_1_0 ---
VULKAN_API_1_0_STRUCTS = [
    VkPhysicalDeviceProperties,
    VkPhysicalDeviceMemoryProperties,
    VkPhysicalDeviceSparseProperties,
    VkImageFormatProperties,
    VkQueueFamilyProperties,
    VkExtensionProperties,
    VkLayerProperties,
    VkFormatProperties,
    VkPhysicalDeviceLimits,
    VkPhysicalDeviceFeatures]

# --- ADDITIONAL EXTENSION INDEPENDENT STRUCTS ---
ADDITIONAL_EXTENSION_INDEPENDENT_STRUCTS = ['VkPhysicalDeviceProperties', 'VkPhysicalDeviceFeatures', 'VkPhysicalDeviceMemoryProperties']

# --- STRUCT EXTENDS MAPPINGS ---
STRUCT_EXTENDS_MAPPING = {   'VkPhysicalDevice16BitStorageFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevice16BitStorageFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevice4444FormatsFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevice8BitStorageFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevice8BitStorageFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceASTCDecodeFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceAccelerationStructureFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceAccelerationStructurePropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceAddressBindingReportFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceAmigoProfilingFeaturesSEC': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceAntiLagFeaturesAMD': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceAttachmentFeedbackLoopDynamicStateFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceBorderColorSwizzleFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceBufferAddressFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceBufferDeviceAddressFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceBufferDeviceAddressFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceBufferDeviceAddressFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceClusterAccelerationStructureFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceClusterAccelerationStructurePropertiesNV': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceClusterCullingShaderPropertiesHUAWEI': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceCoherentMemoryFeaturesAMD': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceColorWriteEnableFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceCommandBufferInheritanceFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceComputeShaderDerivativesFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceComputeShaderDerivativesFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceComputeShaderDerivativesPropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceConditionalRenderingFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceConservativeRasterizationPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceCooperativeMatrix2FeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceCooperativeMatrix2PropertiesNV': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceCooperativeMatrixFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceCooperativeMatrixFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceCooperativeMatrixPropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceCooperativeMatrixPropertiesNV': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceCooperativeVectorFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceCooperativeVectorPropertiesNV': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceCopyMemoryIndirectFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceCopyMemoryIndirectPropertiesNV': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceCornerSampledImageFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceCoverageReductionModeFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceCubicClampFeaturesQCOM': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceCubicWeightsFeaturesQCOM': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceCustomBorderColorFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceCustomBorderColorPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceDepthBiasControlFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceDepthClampControlFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceDepthClampZeroOneFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceDepthClampZeroOneFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceDepthClipControlFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceDepthClipEnableFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceDepthStencilResolveProperties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceDepthStencilResolvePropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceDescriptorBufferDensityMapPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceDescriptorBufferFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceDescriptorBufferPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceDescriptorBufferTensorFeaturesARM': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceDescriptorBufferTensorPropertiesARM': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceDescriptorIndexingFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceDescriptorIndexingFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceDescriptorIndexingProperties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceDescriptorIndexingPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceDescriptorPoolOverallocationFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceDescriptorSetHostMappingFeaturesVALVE': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceDeviceGeneratedCommandsComputeFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceDeviceGeneratedCommandsFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceDeviceGeneratedCommandsPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceDeviceMemoryReportFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceDiagnosticsConfigFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceDiscardRectanglePropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceDriverProperties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceDriverPropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceDrmPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceDynamicRenderingFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceDynamicRenderingFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceDynamicRenderingLocalReadFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceDynamicRenderingLocalReadFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceExclusiveScissorFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceExtendedDynamicState2FeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceExtendedDynamicState3FeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceExtendedDynamicState3PropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceExtendedDynamicStateFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceExtendedSparseAddressSpaceFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceExtendedSparseAddressSpacePropertiesNV': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceExternalComputeQueuePropertiesNV': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceExternalFormatResolveFeaturesANDROID': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceExternalFormatResolvePropertiesANDROID': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceExternalMemoryHostPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceExternalMemoryRDMAFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceFaultFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceFloat16Int8FeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceFloatControlsProperties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceFloatControlsPropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceFormatPackFeaturesARM': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceFragmentDensityMap2FeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceFragmentDensityMap2PropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceFragmentDensityMapFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceFragmentDensityMapOffsetFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceFragmentDensityMapOffsetFeaturesQCOM': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceFragmentDensityMapOffsetPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceFragmentDensityMapOffsetPropertiesQCOM': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceFragmentDensityMapPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceFragmentShaderBarycentricPropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceFragmentShadingRateEnumsFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceFragmentShadingRateEnumsPropertiesNV': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceFragmentShadingRateFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceFragmentShadingRatePropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceFrameBoundaryFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceGlobalPriorityQueryFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceGlobalPriorityQueryFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceGlobalPriorityQueryFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceGraphicsPipelineLibraryPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceHdrVividFeaturesHUAWEI': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceHostImageCopyFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceHostImageCopyFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceHostImageCopyProperties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceHostImageCopyPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceHostQueryResetFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceHostQueryResetFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceIDProperties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceIDPropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceImage2DViewOf3DFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceImageAlignmentControlFeaturesMESA': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceImageAlignmentControlPropertiesMESA': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceImageCompressionControlFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceImageProcessing2FeaturesQCOM': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceImageProcessing2PropertiesQCOM': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceImageProcessingFeaturesQCOM': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceImageProcessingPropertiesQCOM': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceImageRobustnessFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceImageRobustnessFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceImageViewMinLodFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceImagelessFramebufferFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceImagelessFramebufferFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceIndexTypeUint8Features': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceIndexTypeUint8FeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceIndexTypeUint8FeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceInheritedViewportScissorFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceInlineUniformBlockFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceInlineUniformBlockFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceInlineUniformBlockProperties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceInlineUniformBlockPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceInvocationMaskFeaturesHUAWEI': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceLayeredApiPropertiesListKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceLayeredDriverPropertiesMSFT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceLegacyDitheringFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceLegacyVertexAttributesFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceLegacyVertexAttributesPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceLineRasterizationFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceLineRasterizationFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceLineRasterizationFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceLineRasterizationProperties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceLineRasterizationPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceLineRasterizationPropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceLinearColorAttachmentFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceMaintenance3Properties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceMaintenance3PropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceMaintenance4Features': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceMaintenance4FeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceMaintenance4Properties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceMaintenance4PropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceMaintenance5Features': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceMaintenance5FeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceMaintenance5Properties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceMaintenance5PropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceMaintenance6Features': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceMaintenance6FeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceMaintenance6Properties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceMaintenance6PropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceMaintenance7FeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceMaintenance7PropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceMaintenance8FeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceMaintenance9FeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceMaintenance9PropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceMapMemoryPlacedFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceMapMemoryPlacedPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceMemoryDecompressionFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceMemoryDecompressionPropertiesNV': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceMemoryPriorityFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceMeshShaderFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceMeshShaderFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceMeshShaderPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceMeshShaderPropertiesNV': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceMultiDrawFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceMultiDrawPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceMultiviewFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceMultiviewFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceMultiviewProperties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceMultiviewPropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceMutableDescriptorTypeFeaturesVALVE': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceNestedCommandBufferFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceNestedCommandBufferPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceOpacityMicromapFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceOpacityMicromapPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceOpticalFlowFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceOpticalFlowPropertiesNV': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDevicePCIBusInfoPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePartitionedAccelerationStructureFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePartitionedAccelerationStructurePropertiesNV': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDevicePerStageDescriptorSetFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePerformanceQueryFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePerformanceQueryPropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDevicePipelineBinaryFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePipelineBinaryPropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDevicePipelineCreationCacheControlFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePipelineCreationCacheControlFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePipelineOpacityMicromapFeaturesARM': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePipelinePropertiesFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePipelineProtectedAccessFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePipelineProtectedAccessFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePipelineRobustnessFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePipelineRobustnessFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePipelineRobustnessProperties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDevicePipelineRobustnessPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDevicePointClippingProperties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDevicePointClippingPropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDevicePresentBarrierFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePresentId2FeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePresentIdFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePresentModeFifoLatestReadyFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePresentWait2FeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePresentWaitFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePrimitivesGeneratedQueryFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePrivateDataFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDevicePrivateDataFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceProtectedMemoryFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceProtectedMemoryProperties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceProvokingVertexFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceProvokingVertexPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDevicePushDescriptorProperties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDevicePushDescriptorPropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesARM': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceRawAccessChainsFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceRayQueryFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceRayTracingInvocationReorderPropertiesNV': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceRayTracingLinearSweptSpheresFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceRayTracingMotionBlurFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceRayTracingPipelineFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceRayTracingPipelinePropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceRayTracingPropertiesNV': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceRayTracingValidationFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceRelaxedLineRasterizationFeaturesIMG': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceRenderPassStripedFeaturesARM': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceRenderPassStripedPropertiesARM': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceRobustness2FeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceRobustness2FeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceRobustness2PropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceRobustness2PropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceSampleLocationsPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceSamplerFilterMinmaxProperties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceSamplerFilterMinmaxPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceSamplerYcbcrConversionFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceSamplerYcbcrConversionFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceScalarBlockLayoutFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceScalarBlockLayoutFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceSchedulingControlsFeaturesARM': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceSchedulingControlsPropertiesARM': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceSeparateDepthStencilLayoutsFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderAtomicFloat16VectorFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderAtomicFloatFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderAtomicInt64Features': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderAtomicInt64FeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderBfloat16FeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderClockFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderCoreBuiltinsPropertiesARM': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceShaderCoreProperties2AMD': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceShaderCorePropertiesAMD': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceShaderCorePropertiesARM': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderDrawParameterFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderDrawParametersFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderExpectAssumeFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderExpectAssumeFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderFloat16Int8Features': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderFloat16Int8FeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderFloat8FeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderFloatControls2Features': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderFloatControls2FeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderImageFootprintFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderIntegerDotProductFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderIntegerDotProductFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderIntegerDotProductProperties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceShaderIntegerDotProductPropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceShaderIntegerFunctions2FeaturesINTEL': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderMaximalReconvergenceFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderModuleIdentifierPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceShaderObjectFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderObjectPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceShaderQuadControlFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderRelaxedExtendedInstructionFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderReplicatedCompositesFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderSMBuiltinsFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderSMBuiltinsPropertiesNV': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderSubgroupExtendedTypesFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderSubgroupRotateFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderSubgroupRotateFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderTerminateInvocationFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderTerminateInvocationFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderTileImageFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShaderTileImagePropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceShadingRateImageFeaturesNV': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceShadingRateImagePropertiesNV': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceSubgroupProperties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceSubgroupSizeControlFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceSubgroupSizeControlFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceSubgroupSizeControlProperties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceSubgroupSizeControlPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceSubpassShadingFeaturesHUAWEI': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceSubpassShadingPropertiesHUAWEI': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceSynchronization2Features': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceSynchronization2FeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceTensorFeaturesARM': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceTensorPropertiesARM': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceTexelBufferAlignmentProperties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceTexelBufferAlignmentPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceTextureCompressionASTCHDRFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceTextureCompressionASTCHDRFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceTileMemoryHeapFeaturesQCOM': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceTileMemoryHeapPropertiesQCOM': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceTilePropertiesFeaturesQCOM': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceTileShadingFeaturesQCOM': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceTileShadingPropertiesQCOM': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceTimelineSemaphoreFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceTimelineSemaphoreFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceTimelineSemaphoreProperties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceTimelineSemaphorePropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceTransformFeedbackFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceTransformFeedbackPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceUnifiedImageLayoutsFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceUniformBufferStandardLayoutFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceUniformBufferStandardLayoutFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceVariablePointerFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceVariablePointerFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceVariablePointersFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceVariablePointersFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceVertexAttributeDivisorFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceVertexAttributeDivisorFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceVertexAttributeDivisorProperties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceVertexAttributeDivisorPropertiesKHR': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceVertexAttributeRobustnessFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceVideoDecodeVP9FeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceVideoEncodeAV1FeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceVideoEncodeQuantizationMapFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceVideoMaintenance1FeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceVideoMaintenance2FeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceVulkan11Features': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceVulkan11Properties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceVulkan12Features': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceVulkan12Properties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceVulkan13Features': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceVulkan13Properties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceVulkan14Features': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceVulkan14Properties': 'VkPhysicalDeviceProperties2',
    'VkPhysicalDeviceVulkanMemoryModelFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceVulkanMemoryModelFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceYcbcr2Plane444FormatsFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceYcbcrDegammaFeaturesQCOM': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceYcbcrImageArraysFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceZeroInitializeDeviceMemoryFeaturesEXT': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo',
    'VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeaturesKHR': 'VkPhysicalDeviceFeatures2,VkDeviceCreateInfo'}


# --- Enum Traits Mapping ---
ENUM_TRAITS_MAPPING = {   'VkImageLayout': {   'VK_IMAGE_LAYOUT_UNDEFINED': '0',
                         'VK_IMAGE_LAYOUT_GENERAL': '1',
                         'VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL': '2',
                         'VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL': '3',
                         'VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL': '4',
                         'VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL': '5',
                         'VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL': '6',
                         'VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL': '7',
                         'VK_IMAGE_LAYOUT_PREINITIALIZED': '8'},
    'VkImageType': {'VK_IMAGE_TYPE_1D': '0', 'VK_IMAGE_TYPE_2D': '1', 'VK_IMAGE_TYPE_3D': '2'},
    'VkImageTiling': {'VK_IMAGE_TILING_OPTIMAL': '0', 'VK_IMAGE_TILING_LINEAR': '1'},
    'VkPhysicalDeviceType': {   'VK_PHYSICAL_DEVICE_TYPE_OTHER': '0',
                                'VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU': '1',
                                'VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU': '2',
                                'VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU': '3',
                                'VK_PHYSICAL_DEVICE_TYPE_CPU': '4'},
    'VkFormat': {   'VK_FORMAT_UNDEFINED': '0',
                    'VK_FORMAT_R4G4_UNORM_PACK8': '1',
                    'VK_FORMAT_R4G4B4A4_UNORM_PACK16': '2',
                    'VK_FORMAT_B4G4R4A4_UNORM_PACK16': '3',
                    'VK_FORMAT_R5G6B5_UNORM_PACK16': '4',
                    'VK_FORMAT_B5G6R5_UNORM_PACK16': '5',
                    'VK_FORMAT_R5G5B5A1_UNORM_PACK16': '6',
                    'VK_FORMAT_B5G5R5A1_UNORM_PACK16': '7',
                    'VK_FORMAT_A1R5G5B5_UNORM_PACK16': '8',
                    'VK_FORMAT_R8_UNORM': '9',
                    'VK_FORMAT_R8_SNORM': '10',
                    'VK_FORMAT_R8_USCALED': '11',
                    'VK_FORMAT_R8_SSCALED': '12',
                    'VK_FORMAT_R8_UINT': '13',
                    'VK_FORMAT_R8_SINT': '14',
                    'VK_FORMAT_R8_SRGB': '15',
                    'VK_FORMAT_R8G8_UNORM': '16',
                    'VK_FORMAT_R8G8_SNORM': '17',
                    'VK_FORMAT_R8G8_USCALED': '18',
                    'VK_FORMAT_R8G8_SSCALED': '19',
                    'VK_FORMAT_R8G8_UINT': '20',
                    'VK_FORMAT_R8G8_SINT': '21',
                    'VK_FORMAT_R8G8_SRGB': '22',
                    'VK_FORMAT_R8G8B8_UNORM': '23',
                    'VK_FORMAT_R8G8B8_SNORM': '24',
                    'VK_FORMAT_R8G8B8_USCALED': '25',
                    'VK_FORMAT_R8G8B8_SSCALED': '26',
                    'VK_FORMAT_R8G8B8_UINT': '27',
                    'VK_FORMAT_R8G8B8_SINT': '28',
                    'VK_FORMAT_R8G8B8_SRGB': '29',
                    'VK_FORMAT_B8G8R8_UNORM': '30',
                    'VK_FORMAT_B8G8R8_SNORM': '31',
                    'VK_FORMAT_B8G8R8_USCALED': '32',
                    'VK_FORMAT_B8G8R8_SSCALED': '33',
                    'VK_FORMAT_B8G8R8_UINT': '34',
                    'VK_FORMAT_B8G8R8_SINT': '35',
                    'VK_FORMAT_B8G8R8_SRGB': '36',
                    'VK_FORMAT_R8G8B8A8_UNORM': '37',
                    'VK_FORMAT_R8G8B8A8_SNORM': '38',
                    'VK_FORMAT_R8G8B8A8_USCALED': '39',
                    'VK_FORMAT_R8G8B8A8_SSCALED': '40',
                    'VK_FORMAT_R8G8B8A8_UINT': '41',
                    'VK_FORMAT_R8G8B8A8_SINT': '42',
                    'VK_FORMAT_R8G8B8A8_SRGB': '43',
                    'VK_FORMAT_B8G8R8A8_UNORM': '44',
                    'VK_FORMAT_B8G8R8A8_SNORM': '45',
                    'VK_FORMAT_B8G8R8A8_USCALED': '46',
                    'VK_FORMAT_B8G8R8A8_SSCALED': '47',
                    'VK_FORMAT_B8G8R8A8_UINT': '48',
                    'VK_FORMAT_B8G8R8A8_SINT': '49',
                    'VK_FORMAT_B8G8R8A8_SRGB': '50',
                    'VK_FORMAT_A8B8G8R8_UNORM_PACK32': '51',
                    'VK_FORMAT_A8B8G8R8_SNORM_PACK32': '52',
                    'VK_FORMAT_A8B8G8R8_USCALED_PACK32': '53',
                    'VK_FORMAT_A8B8G8R8_SSCALED_PACK32': '54',
                    'VK_FORMAT_A8B8G8R8_UINT_PACK32': '55',
                    'VK_FORMAT_A8B8G8R8_SINT_PACK32': '56',
                    'VK_FORMAT_A8B8G8R8_SRGB_PACK32': '57',
                    'VK_FORMAT_A2R10G10B10_UNORM_PACK32': '58',
                    'VK_FORMAT_A2R10G10B10_SNORM_PACK32': '59',
                    'VK_FORMAT_A2R10G10B10_USCALED_PACK32': '60',
                    'VK_FORMAT_A2R10G10B10_SSCALED_PACK32': '61',
                    'VK_FORMAT_A2R10G10B10_UINT_PACK32': '62',
                    'VK_FORMAT_A2R10G10B10_SINT_PACK32': '63',
                    'VK_FORMAT_A2B10G10R10_UNORM_PACK32': '64',
                    'VK_FORMAT_A2B10G10R10_SNORM_PACK32': '65',
                    'VK_FORMAT_A2B10G10R10_USCALED_PACK32': '66',
                    'VK_FORMAT_A2B10G10R10_SSCALED_PACK32': '67',
                    'VK_FORMAT_A2B10G10R10_UINT_PACK32': '68',
                    'VK_FORMAT_A2B10G10R10_SINT_PACK32': '69',
                    'VK_FORMAT_R16_UNORM': '70',
                    'VK_FORMAT_R16_SNORM': '71',
                    'VK_FORMAT_R16_USCALED': '72',
                    'VK_FORMAT_R16_SSCALED': '73',
                    'VK_FORMAT_R16_UINT': '74',
                    'VK_FORMAT_R16_SINT': '75',
                    'VK_FORMAT_R16_SFLOAT': '76',
                    'VK_FORMAT_R16G16_UNORM': '77',
                    'VK_FORMAT_R16G16_SNORM': '78',
                    'VK_FORMAT_R16G16_USCALED': '79',
                    'VK_FORMAT_R16G16_SSCALED': '80',
                    'VK_FORMAT_R16G16_UINT': '81',
                    'VK_FORMAT_R16G16_SINT': '82',
                    'VK_FORMAT_R16G16_SFLOAT': '83',
                    'VK_FORMAT_R16G16B16_UNORM': '84',
                    'VK_FORMAT_R16G16B16_SNORM': '85',
                    'VK_FORMAT_R16G16B16_USCALED': '86',
                    'VK_FORMAT_R16G16B16_SSCALED': '87',
                    'VK_FORMAT_R16G16B16_UINT': '88',
                    'VK_FORMAT_R16G16B16_SINT': '89',
                    'VK_FORMAT_R16G16B16_SFLOAT': '90',
                    'VK_FORMAT_R16G16B16A16_UNORM': '91',
                    'VK_FORMAT_R16G16B16A16_SNORM': '92',
                    'VK_FORMAT_R16G16B16A16_USCALED': '93',
                    'VK_FORMAT_R16G16B16A16_SSCALED': '94',
                    'VK_FORMAT_R16G16B16A16_UINT': '95',
                    'VK_FORMAT_R16G16B16A16_SINT': '96',
                    'VK_FORMAT_R16G16B16A16_SFLOAT': '97',
                    'VK_FORMAT_R32_UINT': '98',
                    'VK_FORMAT_R32_SINT': '99',
                    'VK_FORMAT_R32_SFLOAT': '100',
                    'VK_FORMAT_R32G32_UINT': '101',
                    'VK_FORMAT_R32G32_SINT': '102',
                    'VK_FORMAT_R32G32_SFLOAT': '103',
                    'VK_FORMAT_R32G32B32_UINT': '104',
                    'VK_FORMAT_R32G32B32_SINT': '105',
                    'VK_FORMAT_R32G32B32_SFLOAT': '106',
                    'VK_FORMAT_R32G32B32A32_UINT': '107',
                    'VK_FORMAT_R32G32B32A32_SINT': '108',
                    'VK_FORMAT_R32G32B32A32_SFLOAT': '109',
                    'VK_FORMAT_R64_UINT': '110',
                    'VK_FORMAT_R64_SINT': '111',
                    'VK_FORMAT_R64_SFLOAT': '112',
                    'VK_FORMAT_R64G64_UINT': '113',
                    'VK_FORMAT_R64G64_SINT': '114',
                    'VK_FORMAT_R64G64_SFLOAT': '115',
                    'VK_FORMAT_R64G64B64_UINT': '116',
                    'VK_FORMAT_R64G64B64_SINT': '117',
                    'VK_FORMAT_R64G64B64_SFLOAT': '118',
                    'VK_FORMAT_R64G64B64A64_UINT': '119',
                    'VK_FORMAT_R64G64B64A64_SINT': '120',
                    'VK_FORMAT_R64G64B64A64_SFLOAT': '121',
                    'VK_FORMAT_B10G11R11_UFLOAT_PACK32': '122',
                    'VK_FORMAT_E5B9G9R9_UFLOAT_PACK32': '123',
                    'VK_FORMAT_D16_UNORM': '124',
                    'VK_FORMAT_X8_D24_UNORM_PACK32': '125',
                    'VK_FORMAT_D32_SFLOAT': '126',
                    'VK_FORMAT_S8_UINT': '127',
                    'VK_FORMAT_D16_UNORM_S8_UINT': '128',
                    'VK_FORMAT_D24_UNORM_S8_UINT': '129',
                    'VK_FORMAT_D32_SFLOAT_S8_UINT': '130',
                    'VK_FORMAT_BC1_RGB_UNORM_BLOCK': '131',
                    'VK_FORMAT_BC1_RGB_SRGB_BLOCK': '132',
                    'VK_FORMAT_BC1_RGBA_UNORM_BLOCK': '133',
                    'VK_FORMAT_BC1_RGBA_SRGB_BLOCK': '134',
                    'VK_FORMAT_BC2_UNORM_BLOCK': '135',
                    'VK_FORMAT_BC2_SRGB_BLOCK': '136',
                    'VK_FORMAT_BC3_UNORM_BLOCK': '137',
                    'VK_FORMAT_BC3_SRGB_BLOCK': '138',
                    'VK_FORMAT_BC4_UNORM_BLOCK': '139',
                    'VK_FORMAT_BC4_SNORM_BLOCK': '140',
                    'VK_FORMAT_BC5_UNORM_BLOCK': '141',
                    'VK_FORMAT_BC5_SNORM_BLOCK': '142',
                    'VK_FORMAT_BC6H_UFLOAT_BLOCK': '143',
                    'VK_FORMAT_BC6H_SFLOAT_BLOCK': '144',
                    'VK_FORMAT_BC7_UNORM_BLOCK': '145',
                    'VK_FORMAT_BC7_SRGB_BLOCK': '146',
                    'VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK': '147',
                    'VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK': '148',
                    'VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK': '149',
                    'VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK': '150',
                    'VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK': '151',
                    'VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK': '152',
                    'VK_FORMAT_EAC_R11_UNORM_BLOCK': '153',
                    'VK_FORMAT_EAC_R11_SNORM_BLOCK': '154',
                    'VK_FORMAT_EAC_R11G11_UNORM_BLOCK': '155',
                    'VK_FORMAT_EAC_R11G11_SNORM_BLOCK': '156',
                    'VK_FORMAT_ASTC_4x4_UNORM_BLOCK': '157',
                    'VK_FORMAT_ASTC_4x4_SRGB_BLOCK': '158',
                    'VK_FORMAT_ASTC_5x4_UNORM_BLOCK': '159',
                    'VK_FORMAT_ASTC_5x4_SRGB_BLOCK': '160',
                    'VK_FORMAT_ASTC_5x5_UNORM_BLOCK': '161',
                    'VK_FORMAT_ASTC_5x5_SRGB_BLOCK': '162',
                    'VK_FORMAT_ASTC_6x5_UNORM_BLOCK': '163',
                    'VK_FORMAT_ASTC_6x5_SRGB_BLOCK': '164',
                    'VK_FORMAT_ASTC_6x6_UNORM_BLOCK': '165',
                    'VK_FORMAT_ASTC_6x6_SRGB_BLOCK': '166',
                    'VK_FORMAT_ASTC_8x5_UNORM_BLOCK': '167',
                    'VK_FORMAT_ASTC_8x5_SRGB_BLOCK': '168',
                    'VK_FORMAT_ASTC_8x6_UNORM_BLOCK': '169',
                    'VK_FORMAT_ASTC_8x6_SRGB_BLOCK': '170',
                    'VK_FORMAT_ASTC_8x8_UNORM_BLOCK': '171',
                    'VK_FORMAT_ASTC_8x8_SRGB_BLOCK': '172',
                    'VK_FORMAT_ASTC_10x5_UNORM_BLOCK': '173',
                    'VK_FORMAT_ASTC_10x5_SRGB_BLOCK': '174',
                    'VK_FORMAT_ASTC_10x6_UNORM_BLOCK': '175',
                    'VK_FORMAT_ASTC_10x6_SRGB_BLOCK': '176',
                    'VK_FORMAT_ASTC_10x8_UNORM_BLOCK': '177',
                    'VK_FORMAT_ASTC_10x8_SRGB_BLOCK': '178',
                    'VK_FORMAT_ASTC_10x10_UNORM_BLOCK': '179',
                    'VK_FORMAT_ASTC_10x10_SRGB_BLOCK': '180',
                    'VK_FORMAT_ASTC_12x10_UNORM_BLOCK': '181',
                    'VK_FORMAT_ASTC_12x10_SRGB_BLOCK': '182',
                    'VK_FORMAT_ASTC_12x12_UNORM_BLOCK': '183',
                    'VK_FORMAT_ASTC_12x12_SRGB_BLOCK': '184',
                    'VK_FORMAT_G8B8G8R8_422_UNORM': '1000156000',
                    'VK_FORMAT_B8G8R8G8_422_UNORM': '1000156001',
                    'VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM': '1000156002',
                    'VK_FORMAT_G8_B8R8_2PLANE_420_UNORM': '1000156003',
                    'VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM': '1000156004',
                    'VK_FORMAT_G8_B8R8_2PLANE_422_UNORM': '1000156005',
                    'VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM': '1000156006',
                    'VK_FORMAT_R10X6_UNORM_PACK16': '1000156007',
                    'VK_FORMAT_R10X6G10X6_UNORM_2PACK16': '1000156008',
                    'VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16': '1000156009',
                    'VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16': '1000156010',
                    'VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16': '1000156011',
                    'VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16': '1000156012',
                    'VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16': '1000156013',
                    'VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16': '1000156014',
                    'VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16': '1000156015',
                    'VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16': '1000156016',
                    'VK_FORMAT_R12X4_UNORM_PACK16': '1000156017',
                    'VK_FORMAT_R12X4G12X4_UNORM_2PACK16': '1000156018',
                    'VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16': '1000156019',
                    'VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16': '1000156020',
                    'VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16': '1000156021',
                    'VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16': '1000156022',
                    'VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16': '1000156023',
                    'VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16': '1000156024',
                    'VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16': '1000156025',
                    'VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16': '1000156026',
                    'VK_FORMAT_G16B16G16R16_422_UNORM': '1000156027',
                    'VK_FORMAT_B16G16R16G16_422_UNORM': '1000156028',
                    'VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM': '1000156029',
                    'VK_FORMAT_G16_B16R16_2PLANE_420_UNORM': '1000156030',
                    'VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM': '1000156031',
                    'VK_FORMAT_G16_B16R16_2PLANE_422_UNORM': '1000156032',
                    'VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM': '1000156033',
                    'VK_FORMAT_G8_B8R8_2PLANE_444_UNORM': '1000330000',
                    'VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16': '1000330001',
                    'VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16': '1000330002',
                    'VK_FORMAT_G16_B16R16_2PLANE_444_UNORM': '1000330003',
                    'VK_FORMAT_A4R4G4B4_UNORM_PACK16': '1000340000',
                    'VK_FORMAT_A4B4G4R4_UNORM_PACK16': '1000340001',
                    'VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK': '1000066000',
                    'VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK': '1000066001',
                    'VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK': '1000066002',
                    'VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK': '1000066003',
                    'VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK': '1000066004',
                    'VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK': '1000066005',
                    'VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK': '1000066006',
                    'VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK': '1000066007',
                    'VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK': '1000066008',
                    'VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK': '1000066009',
                    'VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK': '1000066010',
                    'VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK': '1000066011',
                    'VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK': '1000066012',
                    'VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK': '1000066013',
                    'VK_FORMAT_A1B5G5R5_UNORM_PACK16': '1000470000',
                    'VK_FORMAT_A8_UNORM': '1000470001',
                    'VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG': '1000054000',
                    'VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG': '1000054001',
                    'VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG': '1000054002',
                    'VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG': '1000054003',
                    'VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG': '1000054004',
                    'VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG': '1000054005',
                    'VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG': '1000054006',
                    'VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG': '1000054007',
                    'VK_FORMAT_R8_BOOL_ARM': '1000460000',
                    'VK_FORMAT_R16G16_SFIXED5_NV': '1000464000',
                    'VK_FORMAT_R10X6_UINT_PACK16_ARM': '1000609000',
                    'VK_FORMAT_R10X6G10X6_UINT_2PACK16_ARM': '1000609001',
                    'VK_FORMAT_R10X6G10X6B10X6A10X6_UINT_4PACK16_ARM': '1000609002',
                    'VK_FORMAT_R12X4_UINT_PACK16_ARM': '1000609003',
                    'VK_FORMAT_R12X4G12X4_UINT_2PACK16_ARM': '1000609004',
                    'VK_FORMAT_R12X4G12X4B12X4A12X4_UINT_4PACK16_ARM': '1000609005',
                    'VK_FORMAT_R14X2_UINT_PACK16_ARM': '1000609006',
                    'VK_FORMAT_R14X2G14X2_UINT_2PACK16_ARM': '1000609007',
                    'VK_FORMAT_R14X2G14X2B14X2A14X2_UINT_4PACK16_ARM': '1000609008',
                    'VK_FORMAT_R14X2_UNORM_PACK16_ARM': '1000609009',
                    'VK_FORMAT_R14X2G14X2_UNORM_2PACK16_ARM': '1000609010',
                    'VK_FORMAT_R14X2G14X2B14X2A14X2_UNORM_4PACK16_ARM': '1000609011',
                    'VK_FORMAT_G14X2_B14X2R14X2_2PLANE_420_UNORM_3PACK16_ARM': '1000609012',
                    'VK_FORMAT_G14X2_B14X2R14X2_2PLANE_422_UNORM_3PACK16_ARM': '1000609013'},
    'VkRayTracingInvocationReorderModeNV': {   'VK_RAY_TRACING_INVOCATION_REORDER_MODE_NONE_NV': '0',
                                               'VK_RAY_TRACING_INVOCATION_REORDER_MODE_REORDER_NV': '1'},
    'VkSampleCountFlagBits': {   'VK_SAMPLE_COUNT_1_BIT': '0',
                                 'VK_SAMPLE_COUNT_2_BIT': '1',
                                 'VK_SAMPLE_COUNT_4_BIT': '2',
                                 'VK_SAMPLE_COUNT_8_BIT': '3',
                                 'VK_SAMPLE_COUNT_16_BIT': '4',
                                 'VK_SAMPLE_COUNT_32_BIT': '5',
                                 'VK_SAMPLE_COUNT_64_BIT': '6'},
    'VkExternalMemoryHandleTypeFlagBits': {   'VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT': '0',
                                              'VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT': '1',
                                              'VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT': '2',
                                              'VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT': '3',
                                              'VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT': '4',
                                              'VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_HEAP_BIT': '5',
                                              'VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT': '6'},
    'VkExternalSemaphoreHandleTypeFlagBits': {   'VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT': '0',
                                                 'VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT': '1',
                                                 'VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT': '2',
                                                 'VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE_BIT': '3',
                                                 'VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT': '4'},
    'VkExternalFenceHandleTypeFlagBits': {   'VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT': '0',
                                             'VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT': '1',
                                             'VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT': '2',
                                             'VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT': '3'},
    'VkPointClippingBehavior': {   'VK_POINT_CLIPPING_BEHAVIOR_ALL_CLIP_PLANES': '0',
                                   'VK_POINT_CLIPPING_BEHAVIOR_USER_CLIP_PLANES_ONLY': '1'},
    'VkChromaLocation': {   'VK_CHROMA_LOCATION_COSITED_EVEN': '0',
                            'VK_CHROMA_LOCATION_MIDPOINT': '1'},
    'VkDriverId': {   'VK_DRIVER_ID_AMD_PROPRIETARY': '1',
                      'VK_DRIVER_ID_AMD_OPEN_SOURCE': '2',
                      'VK_DRIVER_ID_MESA_RADV': '3',
                      'VK_DRIVER_ID_NVIDIA_PROPRIETARY': '4',
                      'VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS': '5',
                      'VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA': '6',
                      'VK_DRIVER_ID_IMAGINATION_PROPRIETARY': '7',
                      'VK_DRIVER_ID_QUALCOMM_PROPRIETARY': '8',
                      'VK_DRIVER_ID_ARM_PROPRIETARY': '9',
                      'VK_DRIVER_ID_GOOGLE_SWIFTSHADER': '10',
                      'VK_DRIVER_ID_GGP_PROPRIETARY': '11',
                      'VK_DRIVER_ID_BROADCOM_PROPRIETARY': '12',
                      'VK_DRIVER_ID_MESA_LLVMPIPE': '13',
                      'VK_DRIVER_ID_MOLTENVK': '14',
                      'VK_DRIVER_ID_COREAVI_PROPRIETARY': '15',
                      'VK_DRIVER_ID_JUICE_PROPRIETARY': '16',
                      'VK_DRIVER_ID_VERISILICON_PROPRIETARY': '17',
                      'VK_DRIVER_ID_MESA_TURNIP': '18',
                      'VK_DRIVER_ID_MESA_V3DV': '19',
                      'VK_DRIVER_ID_MESA_PANVK': '20',
                      'VK_DRIVER_ID_SAMSUNG_PROPRIETARY': '21',
                      'VK_DRIVER_ID_MESA_VENUS': '22',
                      'VK_DRIVER_ID_MESA_DOZEN': '23',
                      'VK_DRIVER_ID_MESA_NVK': '24',
                      'VK_DRIVER_ID_IMAGINATION_OPEN_SOURCE_MESA': '25',
                      'VK_DRIVER_ID_MESA_HONEYKRISP': '26',
                      'VK_DRIVER_ID_VULKAN_SC_EMULATION_ON_VULKAN': '27'},
    'VkShaderFloatControlsIndependence': {   'VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_32_BIT_ONLY': '0',
                                             'VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL': '1',
                                             'VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_NONE': '2'},
    'VkPipelineRobustnessBufferBehavior': {   'VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DEVICE_DEFAULT': '0',
                                              'VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED': '1',
                                              'VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS': '2',
                                              'VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_2': '3'},
    'VkPipelineRobustnessImageBehavior': {   'VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_DEVICE_DEFAULT': '0',
                                             'VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_DISABLED': '1',
                                             'VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_ROBUST_IMAGE_ACCESS': '2',
                                             'VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_ROBUST_IMAGE_ACCESS_2': '3'},
    'VkPhysicalDeviceLayeredApiKHR': {   'VK_PHYSICAL_DEVICE_LAYERED_API_VULKAN_KHR': '0',
                                         'VK_PHYSICAL_DEVICE_LAYERED_API_D3D12_KHR': '1',
                                         'VK_PHYSICAL_DEVICE_LAYERED_API_METAL_KHR': '2',
                                         'VK_PHYSICAL_DEVICE_LAYERED_API_OPENGL_KHR': '3',
                                         'VK_PHYSICAL_DEVICE_LAYERED_API_OPENGLES_KHR': '4'},
    'VkLayeredDriverUnderlyingApiMSFT': {   'VK_LAYERED_DRIVER_UNDERLYING_API_NONE_MSFT': '0',
                                            'VK_LAYERED_DRIVER_UNDERLYING_API_D3D12_MSFT': '1'},
    'VkDefaultVertexAttributeValueKHR': {   'VK_DEFAULT_VERTEX_ATTRIBUTE_VALUE_ZERO_ZERO_ZERO_ZERO_KHR': '0',
                                            'VK_DEFAULT_VERTEX_ATTRIBUTE_VALUE_ZERO_ZERO_ZERO_ONE_KHR': '1'}}


# --- VK Format Mapping ---
VK_FORMAT_MAPPING = {   'VK_VERSION_1_1': [   ('VK_FORMAT_G8B8G8R8_422_UNORM', 1000156000),
                          ('VK_FORMAT_B8G8R8G8_422_UNORM', 1000156001),
                          ('VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM', 1000156002),
                          ('VK_FORMAT_G8_B8R8_2PLANE_420_UNORM', 1000156003),
                          ('VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM', 1000156004),
                          ('VK_FORMAT_G8_B8R8_2PLANE_422_UNORM', 1000156005),
                          ('VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM', 1000156006),
                          ('VK_FORMAT_R10X6_UNORM_PACK16', 1000156007),
                          ('VK_FORMAT_R10X6G10X6_UNORM_2PACK16', 1000156008),
                          ('VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16', 1000156009),
                          ('VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16', 1000156010),
                          ('VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16', 1000156011),
                          ('VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16', 1000156012),
                          ('VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16', 1000156013),
                          ('VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16', 1000156014),
                          ('VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16', 1000156015),
                          ('VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16', 1000156016),
                          ('VK_FORMAT_R12X4_UNORM_PACK16', 1000156017),
                          ('VK_FORMAT_R12X4G12X4_UNORM_2PACK16', 1000156018),
                          ('VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16', 1000156019),
                          ('VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16', 1000156020),
                          ('VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16', 1000156021),
                          ('VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16', 1000156022),
                          ('VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16', 1000156023),
                          ('VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16', 1000156024),
                          ('VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16', 1000156025),
                          ('VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16', 1000156026),
                          ('VK_FORMAT_G16B16G16R16_422_UNORM', 1000156027),
                          ('VK_FORMAT_B16G16R16G16_422_UNORM', 1000156028),
                          ('VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM', 1000156029),
                          ('VK_FORMAT_G16_B16R16_2PLANE_420_UNORM', 1000156030),
                          ('VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM', 1000156031),
                          ('VK_FORMAT_G16_B16R16_2PLANE_422_UNORM', 1000156032),
                          ('VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM', 1000156033)],
    'VK_VERSION_1_3': [   ('VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK', 1000066000),
                          ('VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK', 1000066001),
                          ('VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK', 1000066002),
                          ('VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK', 1000066003),
                          ('VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK', 1000066004),
                          ('VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK', 1000066005),
                          ('VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK', 1000066006),
                          ('VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK', 1000066007),
                          ('VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK', 1000066008),
                          ('VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK', 1000066009),
                          ('VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK', 1000066010),
                          ('VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK', 1000066011),
                          ('VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK', 1000066012),
                          ('VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK', 1000066013),
                          ('VK_FORMAT_G8_B8R8_2PLANE_444_UNORM', 1000330000),
                          ('VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16', 1000330001),
                          ('VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16', 1000330002),
                          ('VK_FORMAT_G16_B16R16_2PLANE_444_UNORM', 1000330003),
                          ('VK_FORMAT_A4R4G4B4_UNORM_PACK16', 1000340000),
                          ('VK_FORMAT_A4B4G4R4_UNORM_PACK16', 1000340001)],
    'VK_VERSION_1_4': [   ('VK_FORMAT_A1B5G5R5_UNORM_PACK16', 1000470000),
                          ('VK_FORMAT_A8_UNORM', 1000470001)],
    'VK_IMG_format_pvrtc': [   ('VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG', 1000054000),
                               ('VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG', 1000054001),
                               ('VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG', 1000054002),
                               ('VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG', 1000054003),
                               ('VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG', 1000054004),
                               ('VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG', 1000054005),
                               ('VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG', 1000054006),
                               ('VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG', 1000054007)],
    'VK_EXT_texture_compression_astc_hdr': [   ('VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT', 1000066000),
                                               ('VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT', 1000066001),
                                               ('VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT', 1000066002),
                                               ('VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT', 1000066003),
                                               ('VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT', 1000066004),
                                               ('VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT', 1000066005),
                                               ('VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT', 1000066006),
                                               ('VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT', 1000066007),
                                               ('VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT', 1000066008),
                                               ('VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT', 1000066009),
                                               ('VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT', 1000066010),
                                               (   'VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT',
                                                   1000066011),
                                               (   'VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT',
                                                   1000066012),
                                               (   'VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT',
                                                   1000066013)],
    'VK_KHR_sampler_ycbcr_conversion': [   ('VK_FORMAT_G8B8G8R8_422_UNORM_KHR', 1000156000),
                                           ('VK_FORMAT_B8G8R8G8_422_UNORM_KHR', 1000156001),
                                           ('VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR', 1000156002),
                                           ('VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR', 1000156003),
                                           ('VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR', 1000156004),
                                           ('VK_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR', 1000156005),
                                           ('VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR', 1000156006),
                                           ('VK_FORMAT_R10X6_UNORM_PACK16_KHR', 1000156007),
                                           ('VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR', 1000156008),
                                           (   'VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR',
                                               1000156009),
                                           (   'VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR',
                                               1000156010),
                                           (   'VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR',
                                               1000156011),
                                           (   'VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR',
                                               1000156012),
                                           (   'VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR',
                                               1000156013),
                                           (   'VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR',
                                               1000156014),
                                           (   'VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR',
                                               1000156015),
                                           (   'VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR',
                                               1000156016),
                                           ('VK_FORMAT_R12X4_UNORM_PACK16_KHR', 1000156017),
                                           ('VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR', 1000156018),
                                           (   'VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR',
                                               1000156019),
                                           (   'VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR',
                                               1000156020),
                                           (   'VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR',
                                               1000156021),
                                           (   'VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR',
                                               1000156022),
                                           (   'VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR',
                                               1000156023),
                                           (   'VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR',
                                               1000156024),
                                           (   'VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR',
                                               1000156025),
                                           (   'VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR',
                                               1000156026),
                                           ('VK_FORMAT_G16B16G16R16_422_UNORM_KHR', 1000156027),
                                           ('VK_FORMAT_B16G16R16G16_422_UNORM_KHR', 1000156028),
                                           (   'VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR',
                                               1000156029),
                                           (   'VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR',
                                               1000156030),
                                           (   'VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR',
                                               1000156031),
                                           (   'VK_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR',
                                               1000156032),
                                           (   'VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR',
                                               1000156033)],
    'VK_EXT_ycbcr_2plane_444_formats': [   ('VK_FORMAT_G8_B8R8_2PLANE_444_UNORM_EXT', 1000330000),
                                           (   'VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16_EXT',
                                               1000330001),
                                           (   'VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16_EXT',
                                               1000330002),
                                           (   'VK_FORMAT_G16_B16R16_2PLANE_444_UNORM_EXT',
                                               1000330003)],
    'VK_EXT_4444_formats': [   ('VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT', 1000340000),
                               ('VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT', 1000340001)],
    'VK_ARM_tensors': [('VK_FORMAT_R8_BOOL_ARM', 1000460000)],
    'VK_NV_optical_flow': [   ('VK_FORMAT_R16G16_SFIXED5_NV', 1000464000),
                              ('VK_FORMAT_R16G16_S10_5_NV', 1000464000)],
    'VK_KHR_maintenance5': [   ('VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR', 1000470000),
                               ('VK_FORMAT_A8_UNORM_KHR', 1000470001)],
    'VK_ARM_format_pack': [   ('VK_FORMAT_R10X6_UINT_PACK16_ARM', 1000609000),
                              ('VK_FORMAT_R10X6G10X6_UINT_2PACK16_ARM', 1000609001),
                              ('VK_FORMAT_R10X6G10X6B10X6A10X6_UINT_4PACK16_ARM', 1000609002),
                              ('VK_FORMAT_R12X4_UINT_PACK16_ARM', 1000609003),
                              ('VK_FORMAT_R12X4G12X4_UINT_2PACK16_ARM', 1000609004),
                              ('VK_FORMAT_R12X4G12X4B12X4A12X4_UINT_4PACK16_ARM', 1000609005),
                              ('VK_FORMAT_R14X2_UINT_PACK16_ARM', 1000609006),
                              ('VK_FORMAT_R14X2G14X2_UINT_2PACK16_ARM', 1000609007),
                              ('VK_FORMAT_R14X2G14X2B14X2A14X2_UINT_4PACK16_ARM', 1000609008),
                              ('VK_FORMAT_R14X2_UNORM_PACK16_ARM', 1000609009),
                              ('VK_FORMAT_R14X2G14X2_UNORM_2PACK16_ARM', 1000609010),
                              ('VK_FORMAT_R14X2G14X2B14X2A14X2_UNORM_4PACK16_ARM', 1000609011),
                              (   'VK_FORMAT_G14X2_B14X2R14X2_2PLANE_420_UNORM_3PACK16_ARM',
                                  1000609012),
                              (   'VK_FORMAT_G14X2_B14X2R14X2_2PLANE_422_UNORM_3PACK16_ARM',
                                  1000609013)],
    'VK_VERSION_1_0': [   ('VK_FORMAT_R4G4_UNORM_PACK8', 1),
                          ('VK_FORMAT_R4G4B4A4_UNORM_PACK16', 2),
                          ('VK_FORMAT_B4G4R4A4_UNORM_PACK16', 3),
                          ('VK_FORMAT_R5G6B5_UNORM_PACK16', 4),
                          ('VK_FORMAT_B5G6R5_UNORM_PACK16', 5),
                          ('VK_FORMAT_R5G5B5A1_UNORM_PACK16', 6),
                          ('VK_FORMAT_B5G5R5A1_UNORM_PACK16', 7),
                          ('VK_FORMAT_A1R5G5B5_UNORM_PACK16', 8),
                          ('VK_FORMAT_R8_UNORM', 9),
                          ('VK_FORMAT_R8_SNORM', 10),
                          ('VK_FORMAT_R8_USCALED', 11),
                          ('VK_FORMAT_R8_SSCALED', 12),
                          ('VK_FORMAT_R8_UINT', 13),
                          ('VK_FORMAT_R8_SINT', 14),
                          ('VK_FORMAT_R8_SRGB', 15),
                          ('VK_FORMAT_R8G8_UNORM', 16),
                          ('VK_FORMAT_R8G8_SNORM', 17),
                          ('VK_FORMAT_R8G8_USCALED', 18),
                          ('VK_FORMAT_R8G8_SSCALED', 19),
                          ('VK_FORMAT_R8G8_UINT', 20),
                          ('VK_FORMAT_R8G8_SINT', 21),
                          ('VK_FORMAT_R8G8_SRGB', 22),
                          ('VK_FORMAT_R8G8B8_UNORM', 23),
                          ('VK_FORMAT_R8G8B8_SNORM', 24),
                          ('VK_FORMAT_R8G8B8_USCALED', 25),
                          ('VK_FORMAT_R8G8B8_SSCALED', 26),
                          ('VK_FORMAT_R8G8B8_UINT', 27),
                          ('VK_FORMAT_R8G8B8_SINT', 28),
                          ('VK_FORMAT_R8G8B8_SRGB', 29),
                          ('VK_FORMAT_B8G8R8_UNORM', 30),
                          ('VK_FORMAT_B8G8R8_SNORM', 31),
                          ('VK_FORMAT_B8G8R8_USCALED', 32),
                          ('VK_FORMAT_B8G8R8_SSCALED', 33),
                          ('VK_FORMAT_B8G8R8_UINT', 34),
                          ('VK_FORMAT_B8G8R8_SINT', 35),
                          ('VK_FORMAT_B8G8R8_SRGB', 36),
                          ('VK_FORMAT_R8G8B8A8_UNORM', 37),
                          ('VK_FORMAT_R8G8B8A8_SNORM', 38),
                          ('VK_FORMAT_R8G8B8A8_USCALED', 39),
                          ('VK_FORMAT_R8G8B8A8_SSCALED', 40),
                          ('VK_FORMAT_R8G8B8A8_UINT', 41),
                          ('VK_FORMAT_R8G8B8A8_SINT', 42),
                          ('VK_FORMAT_R8G8B8A8_SRGB', 43),
                          ('VK_FORMAT_B8G8R8A8_UNORM', 44),
                          ('VK_FORMAT_B8G8R8A8_SNORM', 45),
                          ('VK_FORMAT_B8G8R8A8_USCALED', 46),
                          ('VK_FORMAT_B8G8R8A8_SSCALED', 47),
                          ('VK_FORMAT_B8G8R8A8_UINT', 48),
                          ('VK_FORMAT_B8G8R8A8_SINT', 49),
                          ('VK_FORMAT_B8G8R8A8_SRGB', 50),
                          ('VK_FORMAT_A8B8G8R8_UNORM_PACK32', 51),
                          ('VK_FORMAT_A8B8G8R8_SNORM_PACK32', 52),
                          ('VK_FORMAT_A8B8G8R8_USCALED_PACK32', 53),
                          ('VK_FORMAT_A8B8G8R8_SSCALED_PACK32', 54),
                          ('VK_FORMAT_A8B8G8R8_UINT_PACK32', 55),
                          ('VK_FORMAT_A8B8G8R8_SINT_PACK32', 56),
                          ('VK_FORMAT_A8B8G8R8_SRGB_PACK32', 57),
                          ('VK_FORMAT_A2R10G10B10_UNORM_PACK32', 58),
                          ('VK_FORMAT_A2R10G10B10_SNORM_PACK32', 59),
                          ('VK_FORMAT_A2R10G10B10_USCALED_PACK32', 60),
                          ('VK_FORMAT_A2R10G10B10_SSCALED_PACK32', 61),
                          ('VK_FORMAT_A2R10G10B10_UINT_PACK32', 62),
                          ('VK_FORMAT_A2R10G10B10_SINT_PACK32', 63),
                          ('VK_FORMAT_A2B10G10R10_UNORM_PACK32', 64),
                          ('VK_FORMAT_A2B10G10R10_SNORM_PACK32', 65),
                          ('VK_FORMAT_A2B10G10R10_USCALED_PACK32', 66),
                          ('VK_FORMAT_A2B10G10R10_SSCALED_PACK32', 67),
                          ('VK_FORMAT_A2B10G10R10_UINT_PACK32', 68),
                          ('VK_FORMAT_A2B10G10R10_SINT_PACK32', 69),
                          ('VK_FORMAT_R16_UNORM', 70),
                          ('VK_FORMAT_R16_SNORM', 71),
                          ('VK_FORMAT_R16_USCALED', 72),
                          ('VK_FORMAT_R16_SSCALED', 73),
                          ('VK_FORMAT_R16_UINT', 74),
                          ('VK_FORMAT_R16_SINT', 75),
                          ('VK_FORMAT_R16_SFLOAT', 76),
                          ('VK_FORMAT_R16G16_UNORM', 77),
                          ('VK_FORMAT_R16G16_SNORM', 78),
                          ('VK_FORMAT_R16G16_USCALED', 79),
                          ('VK_FORMAT_R16G16_SSCALED', 80),
                          ('VK_FORMAT_R16G16_UINT', 81),
                          ('VK_FORMAT_R16G16_SINT', 82),
                          ('VK_FORMAT_R16G16_SFLOAT', 83),
                          ('VK_FORMAT_R16G16B16_UNORM', 84),
                          ('VK_FORMAT_R16G16B16_SNORM', 85),
                          ('VK_FORMAT_R16G16B16_USCALED', 86),
                          ('VK_FORMAT_R16G16B16_SSCALED', 87),
                          ('VK_FORMAT_R16G16B16_UINT', 88),
                          ('VK_FORMAT_R16G16B16_SINT', 89),
                          ('VK_FORMAT_R16G16B16_SFLOAT', 90),
                          ('VK_FORMAT_R16G16B16A16_UNORM', 91),
                          ('VK_FORMAT_R16G16B16A16_SNORM', 92),
                          ('VK_FORMAT_R16G16B16A16_USCALED', 93),
                          ('VK_FORMAT_R16G16B16A16_SSCALED', 94),
                          ('VK_FORMAT_R16G16B16A16_UINT', 95),
                          ('VK_FORMAT_R16G16B16A16_SINT', 96),
                          ('VK_FORMAT_R16G16B16A16_SFLOAT', 97),
                          ('VK_FORMAT_R32_UINT', 98),
                          ('VK_FORMAT_R32_SINT', 99),
                          ('VK_FORMAT_R32_SFLOAT', 100),
                          ('VK_FORMAT_R32G32_UINT', 101),
                          ('VK_FORMAT_R32G32_SINT', 102),
                          ('VK_FORMAT_R32G32_SFLOAT', 103),
                          ('VK_FORMAT_R32G32B32_UINT', 104),
                          ('VK_FORMAT_R32G32B32_SINT', 105),
                          ('VK_FORMAT_R32G32B32_SFLOAT', 106),
                          ('VK_FORMAT_R32G32B32A32_UINT', 107),
                          ('VK_FORMAT_R32G32B32A32_SINT', 108),
                          ('VK_FORMAT_R32G32B32A32_SFLOAT', 109),
                          ('VK_FORMAT_R64_UINT', 110),
                          ('VK_FORMAT_R64_SINT', 111),
                          ('VK_FORMAT_R64_SFLOAT', 112),
                          ('VK_FORMAT_R64G64_UINT', 113),
                          ('VK_FORMAT_R64G64_SINT', 114),
                          ('VK_FORMAT_R64G64_SFLOAT', 115),
                          ('VK_FORMAT_R64G64B64_UINT', 116),
                          ('VK_FORMAT_R64G64B64_SINT', 117),
                          ('VK_FORMAT_R64G64B64_SFLOAT', 118),
                          ('VK_FORMAT_R64G64B64A64_UINT', 119),
                          ('VK_FORMAT_R64G64B64A64_SINT', 120),
                          ('VK_FORMAT_R64G64B64A64_SFLOAT', 121),
                          ('VK_FORMAT_B10G11R11_UFLOAT_PACK32', 122),
                          ('VK_FORMAT_E5B9G9R9_UFLOAT_PACK32', 123),
                          ('VK_FORMAT_D16_UNORM', 124),
                          ('VK_FORMAT_X8_D24_UNORM_PACK32', 125),
                          ('VK_FORMAT_D32_SFLOAT', 126),
                          ('VK_FORMAT_S8_UINT', 127),
                          ('VK_FORMAT_D16_UNORM_S8_UINT', 128),
                          ('VK_FORMAT_D24_UNORM_S8_UINT', 129),
                          ('VK_FORMAT_D32_SFLOAT_S8_UINT', 130),
                          ('VK_FORMAT_BC1_RGB_UNORM_BLOCK', 131),
                          ('VK_FORMAT_BC1_RGB_SRGB_BLOCK', 132),
                          ('VK_FORMAT_BC1_RGBA_UNORM_BLOCK', 133),
                          ('VK_FORMAT_BC1_RGBA_SRGB_BLOCK', 134),
                          ('VK_FORMAT_BC2_UNORM_BLOCK', 135),
                          ('VK_FORMAT_BC2_SRGB_BLOCK', 136),
                          ('VK_FORMAT_BC3_UNORM_BLOCK', 137),
                          ('VK_FORMAT_BC3_SRGB_BLOCK', 138),
                          ('VK_FORMAT_BC4_UNORM_BLOCK', 139),
                          ('VK_FORMAT_BC4_SNORM_BLOCK', 140),
                          ('VK_FORMAT_BC5_UNORM_BLOCK', 141),
                          ('VK_FORMAT_BC5_SNORM_BLOCK', 142),
                          ('VK_FORMAT_BC6H_UFLOAT_BLOCK', 143),
                          ('VK_FORMAT_BC6H_SFLOAT_BLOCK', 144),
                          ('VK_FORMAT_BC7_UNORM_BLOCK', 145),
                          ('VK_FORMAT_BC7_SRGB_BLOCK', 146),
                          ('VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK', 147),
                          ('VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK', 148),
                          ('VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK', 149),
                          ('VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK', 150),
                          ('VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK', 151),
                          ('VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK', 152),
                          ('VK_FORMAT_EAC_R11_UNORM_BLOCK', 153),
                          ('VK_FORMAT_EAC_R11_SNORM_BLOCK', 154),
                          ('VK_FORMAT_EAC_R11G11_UNORM_BLOCK', 155),
                          ('VK_FORMAT_EAC_R11G11_SNORM_BLOCK', 156),
                          ('VK_FORMAT_ASTC_4x4_UNORM_BLOCK', 157),
                          ('VK_FORMAT_ASTC_4x4_SRGB_BLOCK', 158),
                          ('VK_FORMAT_ASTC_5x4_UNORM_BLOCK', 159),
                          ('VK_FORMAT_ASTC_5x4_SRGB_BLOCK', 160),
                          ('VK_FORMAT_ASTC_5x5_UNORM_BLOCK', 161),
                          ('VK_FORMAT_ASTC_5x5_SRGB_BLOCK', 162),
                          ('VK_FORMAT_ASTC_6x5_UNORM_BLOCK', 163),
                          ('VK_FORMAT_ASTC_6x5_SRGB_BLOCK', 164),
                          ('VK_FORMAT_ASTC_6x6_UNORM_BLOCK', 165),
                          ('VK_FORMAT_ASTC_6x6_SRGB_BLOCK', 166),
                          ('VK_FORMAT_ASTC_8x5_UNORM_BLOCK', 167),
                          ('VK_FORMAT_ASTC_8x5_SRGB_BLOCK', 168),
                          ('VK_FORMAT_ASTC_8x6_UNORM_BLOCK', 169),
                          ('VK_FORMAT_ASTC_8x6_SRGB_BLOCK', 170),
                          ('VK_FORMAT_ASTC_8x8_UNORM_BLOCK', 171),
                          ('VK_FORMAT_ASTC_8x8_SRGB_BLOCK', 172),
                          ('VK_FORMAT_ASTC_10x5_UNORM_BLOCK', 173),
                          ('VK_FORMAT_ASTC_10x5_SRGB_BLOCK', 174),
                          ('VK_FORMAT_ASTC_10x6_UNORM_BLOCK', 175),
                          ('VK_FORMAT_ASTC_10x6_SRGB_BLOCK', 176),
                          ('VK_FORMAT_ASTC_10x8_UNORM_BLOCK', 177),
                          ('VK_FORMAT_ASTC_10x8_SRGB_BLOCK', 178),
                          ('VK_FORMAT_ASTC_10x10_UNORM_BLOCK', 179),
                          ('VK_FORMAT_ASTC_10x10_SRGB_BLOCK', 180),
                          ('VK_FORMAT_ASTC_12x10_UNORM_BLOCK', 181),
                          ('VK_FORMAT_ASTC_12x10_SRGB_BLOCK', 182),
                          ('VK_FORMAT_ASTC_12x12_UNORM_BLOCK', 183),
                          ('VK_FORMAT_ASTC_12x12_SRGB_BLOCK', 184)]}