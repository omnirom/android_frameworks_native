#!/usr/bin/env python3
#
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

"""Generates the vkjson files.
"""
import dataclasses
import os
import re
from typing import get_origin

import generator_common as gencom
import vk as VK

dataclass_field = dataclasses.field


COPYRIGHT_WARNINGS = """///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015-2016 The Khronos Group Inc.
// Copyright (c) 2015-2016 Valve Corporation
// Copyright (c) 2015-2016 LunarG, Inc.
// Copyright (c) 2015-2016 Google, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
///////////////////////////////////////////////////////////////////////////////
"""


def get_copyright_warnings():
  return COPYRIGHT_WARNINGS


def get_vkjson_struct_name(extension_name):
  """Gets the corresponding structure name from a Vulkan extension name.
  Example: "VK_KHR_shader_float16_int8" → "VkJsonKHRShaderFloat16Int8"
  """
  prefix_map = {
    "VK_KHR": "VkJsonKHR",
    "VK_EXT": "VkJsonExt",
    "VK_IMG": "VkJsonIMG"
  }

  for prefix, replacement in prefix_map.items():
    if extension_name.startswith(prefix):
      struct_name = replacement + extension_name[len(prefix):]
      break
  else:
    struct_name = f"VkJsonExt{extension_name}"

  # Convert underscores to camel case
  # Example: "VK_KHR_shader_float16_int8" → "VkJsonKHRShaderFloat16Int8"
  struct_name = re.sub(r"_(.)", lambda m: m.group(1).upper(), struct_name)

  return struct_name


def get_vkjson_struct_variable_name(extension_name):
  """Gets corresponding instance name from a Vulkan extension name.
  Example: "VK_KHR_shader_float16_int8" → "khr_shader_float16_int8"
  """
  prefix_map = {
    "VK_KHR_": "khr_",
    "VK_EXT_": "ext_",
    "VK_IMG_": "img_"
  }

  for prefix, replacement in prefix_map.items():
    if extension_name.startswith(prefix):
      return replacement + extension_name[len(prefix):]

  return extension_name.lower()  # Default case if no known prefix matches


def get_struct_name(struct_name):
  """Gets corresponding instance name
  Example: "VkPhysicalDeviceShaderFloat16Int8FeaturesKHR" → "shader_float16_int8_features_khr"
  """
  # Remove "VkPhysicalDevice" prefix and any of the known suffixes
  base_name = struct_name.removeprefix("VkPhysicalDevice").removesuffix("KHR").removesuffix("EXT").removesuffix("IMG")

  # Convert CamelCase to snake_case
  # Example: "ShaderFloat16Int8Features" → "shader_float16_int8_features"
  variable_name = re.sub(r"(?<!^)(?=[A-Z])", "_", base_name).lower()

  # Fix special cases
  variable_name = variable_name.replace("2_d_", "_2d_").replace("3_d_", "_3d_")

  # Add back the correct suffix if it was removed
  suffix_map = {"KHR": "_khr", "EXT": "_ext", "IMG": "_img"}
  for suffix, replacement in suffix_map.items():
    if struct_name.endswith(suffix):
      variable_name += replacement
      break

  # Handle specific exceptions
  special_cases = {
    "8_bit_storage_features_khr": "bit8_storage_features_khr",
    "memory_properties": "memory",
    "16_bit_storage_features": "bit16_storage_features",
    "i_d_properties": "id_properties"
  }

  return special_cases.get(variable_name, variable_name)


def generate_extension_struct_definition(f):
  """Generates struct definition code for extension based structs
  Example:
  struct VkJsonKHRShaderFloatControls {
    VkJsonKHRShaderFloatControls() {
      reported = false;
      memset(&float_controls_properties_khr, 0,
            sizeof(VkPhysicalDeviceFloatControlsPropertiesKHR));
    }
    bool reported;
    VkPhysicalDeviceFloatControlsPropertiesKHR float_controls_properties_khr;
  };
  """
  vkJson_entries = []

  for extension_name, struct_list in VK.VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING["extensions"].items():
    vkjson_struct_name = get_vkjson_struct_name(extension_name)
    vkjson_struct_variable_name = get_vkjson_struct_variable_name(extension_name)
    vkJson_entries.append(f"{vkjson_struct_name} {vkjson_struct_variable_name}")

    struct_entries = []

    f.write(f"struct {vkjson_struct_name} {{\n")
    f.write(f"  {vkjson_struct_name}() {{\n")
    f.write("    reported = false;\n")

    for struct_map in struct_list:
      for struct_name, _ in struct_map.items():
        variable_name = get_struct_name(struct_name)
        f.write(f"    memset(&{variable_name}, 0, sizeof({struct_name}));\n")
        struct_entries.append(f"{struct_name} {variable_name}")

    f.write("  }\n")  # End of constructor
    f.write("  bool reported;\n")

    for entry in struct_entries:
      f.write(f"  {entry};\n")

    f.write("};\n\n")  # End of struct

  return vkJson_entries


def generate_vk_core_struct_definition(f):
  """Generates struct definition code for vulkan cores
  Example:
  struct VkJsonCore11 {
    VkPhysicalDeviceVulkan11Properties properties;
    VkPhysicalDeviceVulkan11Features features;
  };
  """
  vkJson_core_entries = []

  for version, items in VK.VULKAN_CORES_AND_STRUCTS_MAPPING["versions"].items():
    struct_name = f"VkJson{version}"
    vkJson_core_entries.append(f"{struct_name} {version.lower()}")

    f.write(f"struct {struct_name} {{\n")
    f.write(f"  {struct_name}() {{\n") # Start of constructor
    for item in items:
      for struct_type, _ in item.items():
        field_name = "properties" if "Properties" in struct_type else "features"
        f.write(f" memset(&{field_name}, 0, sizeof({struct_type}));\n")
    f.write("  }\n")  # End of constructor

    for item in items:
      for struct_type, _ in item.items():
        field_name = "properties" if "Properties" in struct_type else "features"
        f.write(f"  {struct_type} {field_name};\n")

    if version == "Core14":
      f.write(f"std::vector<VkImageLayout> copy_src_layouts;\n")
      f.write(f"std::vector<VkImageLayout> copy_dst_layouts;\n")

    f.write("};\n\n")

  return vkJson_core_entries


def generate_memset_statements(f):
  """Generates memset statements for all independent Vulkan structs and core Vulkan versions.
  This initializes struct instances to zero before use.

  Example:
    memset(&properties, 0, sizeof(VkPhysicalDeviceProperties));
    VkPhysicalDeviceProperties properties;
  """
  entries = []

  # Process independent structs
  for dataclass_type in VK.EXTENSION_INDEPENDENT_STRUCTS:
    class_name = dataclass_type.__name__
    variable_name = get_struct_name(class_name)
    f.write(f"memset(&{variable_name}, 0, sizeof({class_name}));\n")
    entries.append(f"{class_name} {variable_name}")

  return entries


def gen_h():
  """Generates vkjson.h file.
  """
  genfile = os.path.join(os.path.dirname(__file__),
                         "..", "vkjson", "vkjson.h")

  with open(genfile, "w") as f:
    f.write(f'{get_copyright_warnings()}\n')

    f.write("""\
#ifndef VKJSON_H_
#define VKJSON_H_

#include <string.h>
#include <vulkan/vulkan.h>

#include <map>
#include <string>
#include <vector>

#ifdef WIN32
#undef min
#undef max
#endif

/*
 * This file is autogenerated by vkjson_generator.py. Do not edit directly.
 */
struct VkJsonLayer {
  VkLayerProperties properties;
  std::vector<VkExtensionProperties> extensions;
};

\n""")

    vkjson_extension_structs = generate_extension_struct_definition(f)
    vkjson_core_structs = generate_vk_core_struct_definition(f)

    f.write("""\
struct VkJsonDevice {
  VkJsonDevice() {""")

    feature_property_structs = generate_memset_statements(f)

    f.write("""\
  }\n""")
    for struct_entries in (vkjson_extension_structs, vkjson_core_structs, feature_property_structs):
      for entry in struct_entries:
        f.write(entry + ";\n")

    f.write("""\
  std::vector<VkQueueFamilyProperties> queues;
  std::vector<VkExtensionProperties> extensions;
  std::vector<VkLayerProperties> layers;
  std::map<VkFormat, VkFormatProperties> formats;
  std::map<VkExternalFenceHandleTypeFlagBits, VkExternalFenceProperties>
      external_fence_properties;
  std::map<VkExternalSemaphoreHandleTypeFlagBits, VkExternalSemaphoreProperties>
      external_semaphore_properties;
};

struct VkJsonDeviceGroup {
  VkJsonDeviceGroup() {
    memset(&properties, 0, sizeof(VkPhysicalDeviceGroupProperties));
  }
  VkPhysicalDeviceGroupProperties properties;
  std::vector<uint32_t> device_inds;
};

struct VkJsonInstance {
  VkJsonInstance() : api_version(0) {}
  uint32_t api_version;
  std::vector<VkJsonLayer> layers;
  std::vector<VkExtensionProperties> extensions;
  std::vector<VkJsonDevice> devices;
  std::vector<VkJsonDeviceGroup> device_groups;
};

VkJsonInstance VkJsonGetInstance();
std::string VkJsonInstanceToJson(const VkJsonInstance& instance);
bool VkJsonInstanceFromJson(const std::string& json,
                            VkJsonInstance* instance,
                            std::string* errors);

VkJsonDevice VkJsonGetDevice(VkPhysicalDevice device);
std::string VkJsonDeviceToJson(const VkJsonDevice& device);
bool VkJsonDeviceFromJson(const std::string& json,
                          VkJsonDevice* device,
                          std::string* errors);

std::string VkJsonImageFormatPropertiesToJson(
    const VkImageFormatProperties& properties);
bool VkJsonImageFormatPropertiesFromJson(const std::string& json,
                                         VkImageFormatProperties* properties,
                                         std::string* errors);

// Backward-compatibility aliases
typedef VkJsonDevice VkJsonAllProperties;
inline VkJsonAllProperties VkJsonGetAllProperties(
    VkPhysicalDevice physicalDevice) {
  return VkJsonGetDevice(physicalDevice);
}
inline std::string VkJsonAllPropertiesToJson(
    const VkJsonAllProperties& properties) {
  return VkJsonDeviceToJson(properties);
}
inline bool VkJsonAllPropertiesFromJson(const std::string& json,
                                        VkJsonAllProperties* properties,
                                        std::string* errors) {
  return VkJsonDeviceFromJson(json, properties, errors);
}

#endif  // VKJSON_H_""")

    f.close()
  gencom.run_clang_format(genfile)


def generate_extension_struct_template():
  """Generates templates for extensions
  Example:
    template <typename Visitor>
    inline bool Iterate(Visitor* visitor, VkJsonKHRVariablePointers* structs) {
      return visitor->Visit("variablePointerFeaturesKHR",
                            &structs->variable_pointer_features_khr) &&
            visitor->Visit("variablePointersFeaturesKHR",
                            &structs->variable_pointers_features_khr);
    }
  """
  template_code = []

  for extension, struct_mappings in VK.VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING["extensions"].items():
    struct_type = get_vkjson_struct_name(extension)

    template_code.append(f"template <typename Visitor>")
    template_code.append(f"inline bool Iterate(Visitor* visitor, {struct_type}* structs) {{")
    template_code.append("  return ")

    visitor_calls = []
    for struct_map in struct_mappings:
      for struct_name in struct_map:
        json_field_name = struct_name.replace("VkPhysicalDevice", "")
        json_field_name = json_field_name[0].lower() + json_field_name[1:]

        # Special case renaming
        if json_field_name == "8BitStorageFeaturesKHR":
          json_field_name = "bit8StorageFeaturesKHR"

        visitor_calls.append(
            f'visitor->Visit("{json_field_name}", &structs->{get_struct_name(struct_name)})'
        )

    template_code.append(" &&\n         ".join(visitor_calls) + ";")
    template_code.append("}\n")

  return "\n".join(template_code)


def generate_core_template():
  """Generates templates for vulkan cores.
  template <typename Visitor>
  inline bool Iterate(Visitor* visitor, VkJsonCore11* core) {
    return visitor->Visit("properties", &core->properties) &&
          visitor->Visit("features", &core->features);
  }
  """
  template_code = []

  for version, struct_list in VK.VULKAN_CORES_AND_STRUCTS_MAPPING["versions"].items():
    struct_type = f"VkJson{version}"

    template_code.append(f"template <typename Visitor>")
    template_code.append(f"inline bool Iterate(Visitor* visitor, {struct_type}* core) {{")
    template_code.append("  return")

    visitor_calls = []
    for struct_map in struct_list:
      for struct_name in struct_map:
        member_name = "properties" if "Properties" in struct_name else "features"
        visitor_calls.append(f'visitor->Visit("{member_name}", &core->{member_name})')

    template_code.append(" &&\n         ".join(visitor_calls) + ";")
    template_code.append("}\n")

  return "\n".join(template_code)


def generate_struct_template(data_classes):
  """Generates templates for all the structs
  template <typename Visitor>
  inline bool Iterate(Visitor* visitor,
                      VkPhysicalDevicePointClippingProperties* properties) {
    return visitor->Visit("pointClippingBehavior",
                          &properties->pointClippingBehavior);
  }
  """
  template_code = []
  processed_classes = set()  # Track processed class names

  for dataclass_type in data_classes:
    struct_name = dataclass_type.__name__

    if struct_name in processed_classes:
      continue  # Skip already processed struct
    processed_classes.add(struct_name)

    struct_fields = dataclasses.fields(dataclass_type)
    template_code.append("template <typename Visitor>")

    # Determine the correct variable name based on the struct type
    struct_var = "properties" if "Properties" in struct_name else "features" if "Features" in struct_name else "limits" if "Limits" in struct_name else None

    if not struct_var:
      continue  # Skip structs that don't match expected patterns

    template_code.append(f"inline bool Iterate(Visitor* visitor, {struct_name}* {struct_var}) {{")
    template_code.append(f"return\n")

    visitor_calls = []
    for struct_field in struct_fields:
      field_name = struct_field.name
      field_type = struct_field.type

      if get_origin(field_type) is list:
        # Handle list types (VisitArray)
        size_field_name = VK.LIST_TYPE_FIELD_AND_SIZE_MAPPING[field_name]
        visitor_calls.append(f'visitor->VisitArray("{field_name}", {struct_var}->{size_field_name}, &{struct_var}->{field_name})')
      else:
        # Handle other types (Visit)
        visitor_calls.append(f'visitor->Visit("{field_name}", &{struct_var}->{field_name})')

    template_code.append(" &&\n         ".join(visitor_calls) + ";")
    template_code.append("}\n\n")

  return "\n".join(template_code)


def emit_struct_visits_by_vk_version(f, version):
  """Emits visitor calls for Vulkan version structs
  """
  for struct_map in VK.VULKAN_VERSIONS_AND_STRUCTS_MAPPING[version]:
    for struct_name, _ in struct_map.items():
      struct_var = get_struct_name(struct_name)
      # Converts struct_var from snake_case (e.g., point_clipping_properties)
      # to camelCase (e.g., pointClippingProperties) for struct_display_name.
      struct_display_name = re.sub(r"_([a-z])", lambda match: match.group(1).upper(), struct_var)
      f.write(f'visitor->Visit("{struct_display_name}", &device->{struct_var}) &&\n')


def gen_cc():
  """Generates vkjson.cc file.
  """
  genfile = os.path.join(os.path.dirname(__file__),
                         "..", "vkjson", "vkjson.cc")

  with open(genfile, "w") as f:

    f.write(get_copyright_warnings())
    f.write("\n")

    f.write("""\
#include "vkjson.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <json/json.h>

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <limits>
#include <memory>
#include <sstream>
#include <type_traits>
#include <utility>

/*
 * This file is autogenerated by vkjson_generator.py. Do not edit directly.
 */
namespace {

/*
 * Annotation to tell clang that we intend to fall through from one case to
 * another in a switch. Sourced from android-base/macros.h.
 */
#define FALLTHROUGH_INTENDED [[clang::fallthrough]]

inline bool IsIntegral(double value) {
#if defined(ANDROID)
  // Android NDK doesn't provide std::trunc yet
  return trunc(value) == value;
#else
  return std::trunc(value) == value;
#endif
}

// Floating point fields of Vulkan structure use single precision. The string
// output of max double value in c++ will be larger than Java double's infinity
// value. Below fake double max/min values are only to serve the safe json text
// parsing in between C++ and Java, because Java json library simply cannot
// handle infinity.
static const double SAFE_DOUBLE_MAX = 0.99 * std::numeric_limits<double>::max();
static const double SAFE_DOUBLE_MIN = -SAFE_DOUBLE_MAX;

template <typename T> struct EnumTraits;
template <> struct EnumTraits<VkPhysicalDeviceType> {
  static bool exist(uint32_t e) {
    switch (e) {
      case VK_PHYSICAL_DEVICE_TYPE_OTHER:
      case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
      case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
      case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
      case VK_PHYSICAL_DEVICE_TYPE_CPU:
        return true;
    }
    return false;
  }
};

template <> struct EnumTraits<VkFormat> {
  static bool exist(uint32_t e) {
    switch (e) {
      case VK_FORMAT_UNDEFINED:
      case VK_FORMAT_R4G4_UNORM_PACK8:
      case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
      case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
      case VK_FORMAT_R5G6B5_UNORM_PACK16:
      case VK_FORMAT_B5G6R5_UNORM_PACK16:
      case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
      case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
      case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
      case VK_FORMAT_R8_UNORM:
      case VK_FORMAT_R8_SNORM:
      case VK_FORMAT_R8_USCALED:
      case VK_FORMAT_R8_SSCALED:
      case VK_FORMAT_R8_UINT:
      case VK_FORMAT_R8_SINT:
      case VK_FORMAT_R8_SRGB:
      case VK_FORMAT_R8G8_UNORM:
      case VK_FORMAT_R8G8_SNORM:
      case VK_FORMAT_R8G8_USCALED:
      case VK_FORMAT_R8G8_SSCALED:
      case VK_FORMAT_R8G8_UINT:
      case VK_FORMAT_R8G8_SINT:
      case VK_FORMAT_R8G8_SRGB:
      case VK_FORMAT_R8G8B8_UNORM:
      case VK_FORMAT_R8G8B8_SNORM:
      case VK_FORMAT_R8G8B8_USCALED:
      case VK_FORMAT_R8G8B8_SSCALED:
      case VK_FORMAT_R8G8B8_UINT:
      case VK_FORMAT_R8G8B8_SINT:
      case VK_FORMAT_R8G8B8_SRGB:
      case VK_FORMAT_B8G8R8_UNORM:
      case VK_FORMAT_B8G8R8_SNORM:
      case VK_FORMAT_B8G8R8_USCALED:
      case VK_FORMAT_B8G8R8_SSCALED:
      case VK_FORMAT_B8G8R8_UINT:
      case VK_FORMAT_B8G8R8_SINT:
      case VK_FORMAT_B8G8R8_SRGB:
      case VK_FORMAT_R8G8B8A8_UNORM:
      case VK_FORMAT_R8G8B8A8_SNORM:
      case VK_FORMAT_R8G8B8A8_USCALED:
      case VK_FORMAT_R8G8B8A8_SSCALED:
      case VK_FORMAT_R8G8B8A8_UINT:
      case VK_FORMAT_R8G8B8A8_SINT:
      case VK_FORMAT_R8G8B8A8_SRGB:
      case VK_FORMAT_B8G8R8A8_UNORM:
      case VK_FORMAT_B8G8R8A8_SNORM:
      case VK_FORMAT_B8G8R8A8_USCALED:
      case VK_FORMAT_B8G8R8A8_SSCALED:
      case VK_FORMAT_B8G8R8A8_UINT:
      case VK_FORMAT_B8G8R8A8_SINT:
      case VK_FORMAT_B8G8R8A8_SRGB:
      case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
      case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
      case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
      case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
      case VK_FORMAT_A8B8G8R8_UINT_PACK32:
      case VK_FORMAT_A8B8G8R8_SINT_PACK32:
      case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
      case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
      case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
      case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
      case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
      case VK_FORMAT_A2R10G10B10_UINT_PACK32:
      case VK_FORMAT_A2R10G10B10_SINT_PACK32:
      case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
      case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
      case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
      case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
      case VK_FORMAT_A2B10G10R10_UINT_PACK32:
      case VK_FORMAT_A2B10G10R10_SINT_PACK32:
      case VK_FORMAT_R16_UNORM:
      case VK_FORMAT_R16_SNORM:
      case VK_FORMAT_R16_USCALED:
      case VK_FORMAT_R16_SSCALED:
      case VK_FORMAT_R16_UINT:
      case VK_FORMAT_R16_SINT:
      case VK_FORMAT_R16_SFLOAT:
      case VK_FORMAT_R16G16_UNORM:
      case VK_FORMAT_R16G16_SNORM:
      case VK_FORMAT_R16G16_USCALED:
      case VK_FORMAT_R16G16_SSCALED:
      case VK_FORMAT_R16G16_UINT:
      case VK_FORMAT_R16G16_SINT:
      case VK_FORMAT_R16G16_SFLOAT:
      case VK_FORMAT_R16G16B16_UNORM:
      case VK_FORMAT_R16G16B16_SNORM:
      case VK_FORMAT_R16G16B16_USCALED:
      case VK_FORMAT_R16G16B16_SSCALED:
      case VK_FORMAT_R16G16B16_UINT:
      case VK_FORMAT_R16G16B16_SINT:
      case VK_FORMAT_R16G16B16_SFLOAT:
      case VK_FORMAT_R16G16B16A16_UNORM:
      case VK_FORMAT_R16G16B16A16_SNORM:
      case VK_FORMAT_R16G16B16A16_USCALED:
      case VK_FORMAT_R16G16B16A16_SSCALED:
      case VK_FORMAT_R16G16B16A16_UINT:
      case VK_FORMAT_R16G16B16A16_SINT:
      case VK_FORMAT_R16G16B16A16_SFLOAT:
      case VK_FORMAT_R32_UINT:
      case VK_FORMAT_R32_SINT:
      case VK_FORMAT_R32_SFLOAT:
      case VK_FORMAT_R32G32_UINT:
      case VK_FORMAT_R32G32_SINT:
      case VK_FORMAT_R32G32_SFLOAT:
      case VK_FORMAT_R32G32B32_UINT:
      case VK_FORMAT_R32G32B32_SINT:
      case VK_FORMAT_R32G32B32_SFLOAT:
      case VK_FORMAT_R32G32B32A32_UINT:
      case VK_FORMAT_R32G32B32A32_SINT:
      case VK_FORMAT_R32G32B32A32_SFLOAT:
      case VK_FORMAT_R64_UINT:
      case VK_FORMAT_R64_SINT:
      case VK_FORMAT_R64_SFLOAT:
      case VK_FORMAT_R64G64_UINT:
      case VK_FORMAT_R64G64_SINT:
      case VK_FORMAT_R64G64_SFLOAT:
      case VK_FORMAT_R64G64B64_UINT:
      case VK_FORMAT_R64G64B64_SINT:
      case VK_FORMAT_R64G64B64_SFLOAT:
      case VK_FORMAT_R64G64B64A64_UINT:
      case VK_FORMAT_R64G64B64A64_SINT:
      case VK_FORMAT_R64G64B64A64_SFLOAT:
      case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
      case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
      case VK_FORMAT_D16_UNORM:
      case VK_FORMAT_X8_D24_UNORM_PACK32:
      case VK_FORMAT_D32_SFLOAT:
      case VK_FORMAT_S8_UINT:
      case VK_FORMAT_D16_UNORM_S8_UINT:
      case VK_FORMAT_D24_UNORM_S8_UINT:
      case VK_FORMAT_D32_SFLOAT_S8_UINT:
      case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
      case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
      case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
      case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
      case VK_FORMAT_BC2_UNORM_BLOCK:
      case VK_FORMAT_BC2_SRGB_BLOCK:
      case VK_FORMAT_BC3_UNORM_BLOCK:
      case VK_FORMAT_BC3_SRGB_BLOCK:
      case VK_FORMAT_BC4_UNORM_BLOCK:
      case VK_FORMAT_BC4_SNORM_BLOCK:
      case VK_FORMAT_BC5_UNORM_BLOCK:
      case VK_FORMAT_BC5_SNORM_BLOCK:
      case VK_FORMAT_BC6H_UFLOAT_BLOCK:
      case VK_FORMAT_BC6H_SFLOAT_BLOCK:
      case VK_FORMAT_BC7_UNORM_BLOCK:
      case VK_FORMAT_BC7_SRGB_BLOCK:
      case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
      case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
      case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
      case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
      case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
      case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
      case VK_FORMAT_EAC_R11_UNORM_BLOCK:
      case VK_FORMAT_EAC_R11_SNORM_BLOCK:
      case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
      case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
      case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
      case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
      case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
      case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
      case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
      case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
      case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
      case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
      case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
      case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
      case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
      case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
      case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
      case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
      case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
      case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
      case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
      case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
      case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
      case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
      case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
      case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
      case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
      case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
      case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
      case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
      case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
      case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
      case VK_FORMAT_G8B8G8R8_422_UNORM:
      case VK_FORMAT_B8G8R8G8_422_UNORM:
      case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
      case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
      case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
      case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
      case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
      case VK_FORMAT_R10X6_UNORM_PACK16:
      case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
      case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
      case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
      case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
      case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
      case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
      case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
      case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
      case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
      case VK_FORMAT_R12X4_UNORM_PACK16:
      case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
      case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:
      case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
      case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
      case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
      case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
      case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
      case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
      case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
      case VK_FORMAT_G16B16G16R16_422_UNORM:
      case VK_FORMAT_B16G16R16G16_422_UNORM:
      case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
      case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
      case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
      case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
      case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
      case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
      case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
      case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
      case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
      case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
      case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
      case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
      case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
      case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT:
      case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT:
      case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT:
      case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT:
      case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT:
      case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT:
      case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT:
      case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT:
      case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT:
      case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT:
      case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT:
      case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT:
      case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT:
      case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT:
      case VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT:
      case VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT:
        return true;
    }
    return false;
  }
};

template <>
struct EnumTraits<VkPointClippingBehavior> {
  static bool exist(uint32_t e) {
    switch (e) {
      case VK_POINT_CLIPPING_BEHAVIOR_ALL_CLIP_PLANES:
      case VK_POINT_CLIPPING_BEHAVIOR_USER_CLIP_PLANES_ONLY:
        return true;
    }
    return false;
  }
};

template <>
struct EnumTraits<VkExternalFenceHandleTypeFlagBits> {
  static bool exist(uint32_t e) {
    switch (e) {
      case VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT:
      case VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT:
      case VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT:
      case VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT:
        return true;
    }
    return false;
  }
};

template <>
struct EnumTraits<VkExternalSemaphoreHandleTypeFlagBits> {
  static bool exist(uint32_t e) {
    switch (e) {
      case VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT:
      case VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT:
      case VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT:
      case VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE_BIT:
      case VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT:
        return true;
    }
    return false;
  }
};

template <>
struct EnumTraits<VkDriverIdKHR> {
  static bool exist(uint32_t e) {
    switch (e) {
      case VK_DRIVER_ID_AMD_PROPRIETARY:
      case VK_DRIVER_ID_AMD_OPEN_SOURCE:
      case VK_DRIVER_ID_MESA_RADV:
      case VK_DRIVER_ID_NVIDIA_PROPRIETARY:
      case VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS:
      case VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA:
      case VK_DRIVER_ID_IMAGINATION_PROPRIETARY:
      case VK_DRIVER_ID_QUALCOMM_PROPRIETARY:
      case VK_DRIVER_ID_ARM_PROPRIETARY:
      case VK_DRIVER_ID_GOOGLE_SWIFTSHADER:
      case VK_DRIVER_ID_GGP_PROPRIETARY:
      case VK_DRIVER_ID_BROADCOM_PROPRIETARY:
      case VK_DRIVER_ID_MESA_LLVMPIPE:
      case VK_DRIVER_ID_MOLTENVK:
        return true;
    }
    return false;
  }
};

template <>
struct EnumTraits<VkShaderFloatControlsIndependence> {
  static bool exist(uint32_t e) {
    switch (e) {
      case VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_32_BIT_ONLY:
      case VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL:
      case VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_NONE:
        return true;
    }
    return false;
  }
};

template <>
struct EnumTraits<VkPipelineRobustnessBufferBehavior> {
  static bool exist(uint32_t e) {
    switch (e) {
      case VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DEVICE_DEFAULT:
      case VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED:
      case VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS:
      case VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_2:
        return true;
    }
    return false;
  }
};

template <>
struct EnumTraits<VkPipelineRobustnessImageBehavior> {
  static bool exist(uint32_t e) {
    switch (e) {
      case VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_DEVICE_DEFAULT:
      case VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_DISABLED:
      case VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_ROBUST_IMAGE_ACCESS:
      case VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_ROBUST_IMAGE_ACCESS_2:
        return true;
    }
    return false;
  }
};

template <>
struct EnumTraits<VkImageLayout> {
  static bool exist(uint32_t e) {
    switch (e) {
      case VK_IMAGE_LAYOUT_UNDEFINED:
      case VK_IMAGE_LAYOUT_GENERAL:
      case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
      case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
      case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
      case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      case VK_IMAGE_LAYOUT_PREINITIALIZED:
      case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
      case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
      case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
      case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
      case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
      case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
      case VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL:
      case VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL:
      case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
      case VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR:
      case VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR:
      case VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR:
      case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR:
      case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT:
      case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
#ifdef VK_ENABLE_BETA_EXTENSIONS
      case VK_IMAGE_LAYOUT_VIDEO_ENCODE_DST_KHR:
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
      case VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR:
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
      case VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR:
#endif
      case VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT:
        return true;
    }
    return false;
  }
};

// VkSparseImageFormatProperties

template <typename Visitor>
inline bool Iterate(Visitor* visitor, VkExtent3D* extents) {
  return
    visitor->Visit("width", &extents->width) &&
    visitor->Visit("height", &extents->height) &&
    visitor->Visit("depth", &extents->depth);
}

template <typename Visitor>
inline bool Iterate(Visitor* visitor,
                    VkConformanceVersionKHR* version) {
  return visitor->Visit("major", &version->major) &&
         visitor->Visit("minor", &version->minor) &&
         visitor->Visit("subminor", &version->subminor) &&
         visitor->Visit("patch", &version->patch);
}

template <typename Visitor>
inline bool Iterate(Visitor* visitor, VkMemoryType* type) {
  return
    visitor->Visit("propertyFlags", &type->propertyFlags) &&
    visitor->Visit("heapIndex", &type->heapIndex);
}

template <typename Visitor>
inline bool Iterate(Visitor* visitor, VkMemoryHeap* heap) {
  return
    visitor->Visit("size", &heap->size) &&
    visitor->Visit("flags", &heap->flags);
}\n\n""")

    f.write(f"{generate_core_template()}\n\n{generate_extension_struct_template()}\n\n")
    f.write(generate_struct_template(VK.ALL_STRUCTS))

    f.write("""\
template <typename Visitor>
inline bool Iterate(Visitor* visitor, VkExternalFenceProperties* properties) {
  return visitor->Visit("exportFromImportedHandleTypes",
                        &properties->exportFromImportedHandleTypes) &&
         visitor->Visit("compatibleHandleTypes",
                        &properties->compatibleHandleTypes) &&
         visitor->Visit("externalFenceFeatures",
                        &properties->externalFenceFeatures);
}

template <typename Visitor>
inline bool Iterate(Visitor* visitor,
                    VkExternalSemaphoreProperties* properties) {
  return visitor->Visit("exportFromImportedHandleTypes",
                        &properties->exportFromImportedHandleTypes) &&
         visitor->Visit("compatibleHandleTypes",
                        &properties->compatibleHandleTypes) &&
         visitor->Visit("externalSemaphoreFeatures",
                        &properties->externalSemaphoreFeatures);
}

template <typename Visitor>
inline bool Iterate(Visitor* visitor, VkJsonLayer* layer) {
  return visitor->Visit("properties", &layer->properties) &&
         visitor->Visit("extensions", &layer->extensions);
}

template <typename Visitor>
inline bool Iterate(Visitor* visitor, VkJsonDeviceGroup* device_group) {
  return visitor->Visit("devices", &device_group->device_inds) &&
         visitor->Visit("subsetAllocation",
                        &device_group->properties.subsetAllocation);
}

template <typename Visitor>
inline bool Iterate(Visitor* visitor, VkJsonDevice* device) {
  bool ret = true;
  switch (device->properties.apiVersion ^
          VK_API_VERSION_PATCH(device->properties.apiVersion)) {
    case VK_API_VERSION_1_4:
      ret &= visitor->Visit("core14", &device->core14);
      FALLTHROUGH_INTENDED;
    case VK_API_VERSION_1_3:
      ret &= visitor->Visit("core13", &device->core13);
      FALLTHROUGH_INTENDED;
    case VK_API_VERSION_1_2:
      ret &= visitor->Visit("core11", &device->core11);
      ret &= visitor->Visit("core12", &device->core12);
      FALLTHROUGH_INTENDED;
    case VK_API_VERSION_1_1:
      ret &=\n""")

    emit_struct_visits_by_vk_version(f, "VK_VERSION_1_1")

    f.write("""\
          visitor->Visit("externalFenceProperties",
                         &device->external_fence_properties) &&
          visitor->Visit("externalSemaphoreProperties",
                         &device->external_semaphore_properties);
      FALLTHROUGH_INTENDED;
    case VK_API_VERSION_1_0:
      ret &=\n""")

    emit_struct_visits_by_vk_version(f, "VK_VERSION_1_0")

    f.write("""\
             visitor->Visit("queues", &device->queues) &&
             visitor->Visit("extensions", &device->extensions) &&
             visitor->Visit("layers", &device->layers) &&
             visitor->Visit("formats", &device->formats);\n\n""")

    for extension_name, _ in VK.VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING["extensions"].items():
      struct_var = get_vkjson_struct_variable_name(extension_name)
      f.write(f"  if (device->{struct_var}.reported) {{\n")
      f.write(f"    ret &= visitor->Visit(\"{extension_name}\", &device->{struct_var});\n")
      f.write("  }\n")

    f.write("""\
    } return ret; }

template <typename Visitor>
inline bool Iterate(Visitor* visitor, VkJsonInstance* instance) {
  bool ret = true;
  switch (instance->api_version ^ VK_API_VERSION_PATCH(instance->api_version)) {
    case VK_API_VERSION_1_4:
      FALLTHROUGH_INTENDED;
    case VK_API_VERSION_1_3:
      FALLTHROUGH_INTENDED;
    case VK_API_VERSION_1_2:
      ret &= visitor->Visit("apiVersion", &instance->api_version);
      FALLTHROUGH_INTENDED;
    case VK_API_VERSION_1_1:
      ret &= visitor->Visit("deviceGroups", &instance->device_groups);
      FALLTHROUGH_INTENDED;
    case VK_API_VERSION_1_0:
      char depString[] =
          "vkjson is deprecated, and will be replaced in a future release";
      ret &= visitor->Visit("layers", &instance->layers) &&
             visitor->Visit("extensions", &instance->extensions) &&
             visitor->Visit("devices", &instance->devices) &&
             visitor->Visit("_comment", &depString);
  }
  return ret;
}

template <typename T>
using EnableForArithmetic =
    typename std::enable_if<std::is_arithmetic<T>::value, void>::type;

template <typename T>
using EnableForStruct =
    typename std::enable_if<std::is_class<T>::value, void>::type;

template <typename T>
using EnableForEnum =
    typename std::enable_if<std::is_enum<T>::value, void>::type;

template <typename T, typename = EnableForStruct<T>, typename = void>
Json::Value ToJsonValue(const T& value);

template <typename T, typename = EnableForArithmetic<T>>
inline Json::Value ToJsonValue(const T& value) {
  return Json::Value(
      std::clamp(static_cast<double>(value), SAFE_DOUBLE_MIN, SAFE_DOUBLE_MAX));
}

inline Json::Value ToJsonValue(const uint64_t& value) {
  char string[19] = {0};  // "0x" + 16 digits + terminal \\0
  snprintf(string, sizeof(string), "0x%016" PRIx64, value);
  return Json::Value(string);
}

template <typename T, typename = EnableForEnum<T>, typename = void,
          typename = void>
inline Json::Value ToJsonValue(const T& value) {
  return Json::Value(static_cast<double>(value));
}

template <typename T>
inline Json::Value ArrayToJsonValue(uint32_t count, const T* values) {
  Json::Value array(Json::arrayValue);
  for (unsigned int i = 0; i < count; ++i) array.append(ToJsonValue(values[i]));
  return array;
}

template <typename T, size_t N>
inline Json::Value ToJsonValue(const T (&value)[N]) {
  return ArrayToJsonValue(N, value);
}

template <size_t N>
inline Json::Value ToJsonValue(const char (&value)[N]) {
  assert(strlen(value) < N);
  return Json::Value(value);
}

template <typename T>
inline Json::Value ToJsonValue(const std::vector<T>& value) {
  assert(value.size() <= std::numeric_limits<uint32_t>::max());
  return ArrayToJsonValue(static_cast<uint32_t>(value.size()), value.data());
}

template <typename F, typename S>
inline Json::Value ToJsonValue(const std::pair<F, S>& value) {
  Json::Value array(Json::arrayValue);
  array.append(ToJsonValue(value.first));
  array.append(ToJsonValue(value.second));
  return array;
}

template <typename F, typename S>
inline Json::Value ToJsonValue(const std::map<F, S>& value) {
  Json::Value array(Json::arrayValue);
  for (auto& kv : value) array.append(ToJsonValue(kv));
  return array;
}

class JsonWriterVisitor {
 public:
  JsonWriterVisitor() : object_(Json::objectValue) {}

  ~JsonWriterVisitor() {}

  template <typename T> bool Visit(const char* key, const T* value) {
    object_[key] = ToJsonValue(*value);
    return true;
  }

  template <typename T, uint32_t N>
  bool VisitArray(const char* key, uint32_t count, const T (*value)[N]) {
    assert(count <= N);
    object_[key] = ArrayToJsonValue(count, *value);
    return true;
  }

  template <typename T>
  bool VisitArray(const char* key, uint32_t count, const T *value) {
    object_[key] = ArrayToJsonValue(count, *value);
    return true;
  }

  Json::Value get_object() const { return object_; }

 private:
  Json::Value object_;
};

template <typename Visitor, typename T>
inline void VisitForWrite(Visitor* visitor, const T& t) {
  Iterate(visitor, const_cast<T*>(&t));
}

template <typename T, typename /*= EnableForStruct<T>*/, typename /*= void*/>
Json::Value ToJsonValue(const T& value) {
  JsonWriterVisitor visitor;
  VisitForWrite(&visitor, value);
  return visitor.get_object();
}

template <typename T, typename = EnableForStruct<T>>
bool AsValue(Json::Value* json_value, T* t);

inline bool AsValue(Json::Value* json_value, int32_t* value) {
  if (json_value->type() != Json::realValue) return false;
  double d = json_value->asDouble();
  if (!IsIntegral(d) ||
      d < static_cast<double>(std::numeric_limits<int32_t>::min()) ||
      d > static_cast<double>(std::numeric_limits<int32_t>::max()))
    return false;
  *value = static_cast<int32_t>(d);
  return true;
}

inline bool AsValue(Json::Value* json_value, uint64_t* value) {
  if (json_value->type() != Json::stringValue) return false;
  int result =
      std::sscanf(json_value->asString().c_str(), "0x%016" PRIx64, value);
  return result == 1;
}

inline bool AsValue(Json::Value* json_value, uint32_t* value) {
  if (json_value->type() != Json::realValue) return false;
  double d = json_value->asDouble();
  if (!IsIntegral(d) || d < 0.0 ||
      d > static_cast<double>(std::numeric_limits<uint32_t>::max()))
    return false;
  *value = static_cast<uint32_t>(d);
  return true;
}

inline bool AsValue(Json::Value* json_value, uint8_t* value) {
  uint32_t value32 = 0;
  AsValue(json_value, &value32);
  if (value32 > std::numeric_limits<uint8_t>::max())
    return false;
  *value = static_cast<uint8_t>(value32);
  return true;
}

inline bool AsValue(Json::Value* json_value, float* value) {
  if (json_value->type() != Json::realValue) return false;
  *value = static_cast<float>(json_value->asDouble());
  return true;
}

inline bool AsValue(Json::Value* json_value, VkImageLayout* t) {
  uint32_t value = 0;
  if (!AsValue(json_value, &value))
    return false;
  if (!EnumTraits<VkImageLayout>::exist(value)) return false;
  *t = static_cast<VkImageLayout>(value);
  return true;
}

template <typename T>
inline bool AsArray(Json::Value* json_value, uint32_t count, T* values) {
  if (json_value->type() != Json::arrayValue || json_value->size() != count)
    return false;
  for (uint32_t i = 0; i < count; ++i) {
    if (!AsValue(&(*json_value)[i], values + i)) return false;
  }
  return true;
}

template <typename T, size_t N>
inline bool AsValue(Json::Value* json_value, T (*value)[N]) {
  return AsArray(json_value, N, *value);
}

template <size_t N>
inline bool AsValue(Json::Value* json_value, char (*value)[N]) {
  if (json_value->type() != Json::stringValue) return false;
  size_t len = json_value->asString().length();
  if (len >= N)
    return false;
  memcpy(*value, json_value->asString().c_str(), len);
  memset(*value + len, 0, N-len);
  return true;
}

template <typename T, typename = EnableForEnum<T>, typename = void>
inline bool AsValue(Json::Value* json_value, T* t) {
  uint32_t value = 0;
  if (!AsValue(json_value, &value))
      return false;
  if (!EnumTraits<T>::exist(value)) return false;
  *t = static_cast<T>(value);
  return true;
}

template <typename T>
inline bool AsValue(Json::Value* json_value, std::vector<T>* value) {
  if (json_value->type() != Json::arrayValue) return false;
  int size = json_value->size();
  value->resize(size);
  return AsArray(json_value, size, value->data());
}

template <typename F, typename S>
inline bool AsValue(Json::Value* json_value, std::pair<F, S>* value) {
  if (json_value->type() != Json::arrayValue || json_value->size() != 2)
    return false;
  return AsValue(&(*json_value)[0], &value->first) &&
         AsValue(&(*json_value)[1], &value->second);
}

template <typename F, typename S>
inline bool AsValue(Json::Value* json_value, std::map<F, S>* value) {
  if (json_value->type() != Json::arrayValue) return false;
  int size = json_value->size();
  for (int i = 0; i < size; ++i) {
    std::pair<F, S> elem;
    if (!AsValue(&(*json_value)[i], &elem)) return false;
    if (!value->insert(elem).second)
      return false;
  }
  return true;
}

template <typename T>
bool ReadValue(Json::Value* object, const char* key, T* value,
               std::string* errors) {
  Json::Value json_value = (*object)[key];
  if (!json_value) {
    if (errors)
      *errors = std::string(key) + " missing.";
    return false;
  }
  if (AsValue(&json_value, value)) return true;
  if (errors)
    *errors = std::string("Wrong type for ") + std::string(key) + ".";
  return false;
}

template <typename Visitor, typename T>
inline bool VisitForRead(Visitor* visitor, T* t) {
  return Iterate(visitor, t);
}

class JsonReaderVisitor {
 public:
  JsonReaderVisitor(Json::Value* object, std::string* errors)
      : object_(object), errors_(errors) {}

  template <typename T> bool Visit(const char* key, T* value) const {
    return ReadValue(object_, key, value, errors_);
  }

  template <typename T, uint32_t N>
  bool VisitArray(const char* key, uint32_t count, T (*value)[N]) {
    if (count > N)
      return false;
    Json::Value json_value = (*object_)[key];
    if (!json_value) {
      if (errors_)
        *errors_ = std::string(key) + " missing.";
      return false;
    }
    if (AsArray(&json_value, count, *value)) return true;
    if (errors_)
      *errors_ = std::string("Wrong type for ") + std::string(key) + ".";
    return false;
  }

  template <typename T>
  bool VisitArray(const char* key, uint32_t count, T *value) {
    Json::Value json_value = (*object_)[key];
    if (!json_value) {
      if (errors_)
        *errors_ = std::string(key) + " missing.";
      return false;
    }
    if (AsArray(&json_value, count, *value)) return true;
    if (errors_)
      *errors_ = std::string("Wrong type for ") + std::string(key) + ".";
    return false;
  }


 private:
  Json::Value* object_;
  std::string* errors_;
};

template <typename T, typename /*= EnableForStruct<T>*/>
bool AsValue(Json::Value* json_value, T* t) {
  if (json_value->type() != Json::objectValue) return false;
  JsonReaderVisitor visitor(json_value, nullptr);
  return VisitForRead(&visitor, t);
}


template <typename T> std::string VkTypeToJson(const T& t) {
  JsonWriterVisitor visitor;
  VisitForWrite(&visitor, t);
  return visitor.get_object().toStyledString();
}

template <typename T> bool VkTypeFromJson(const std::string& json,
                                          T* t,
                                          std::string* errors) {
  *t = T();
  Json::Value object(Json::objectValue);
  Json::CharReaderBuilder builder;
  builder["collectComments"] = false;
  std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  if (!reader->parse(json.data(), json.data() + json.size(), &object, errors)) {
    return false;
  }
  return AsValue(&object, t);
}

}  // anonymous namespace

std::string VkJsonInstanceToJson(const VkJsonInstance& instance) {
  return VkTypeToJson(instance);
}

bool VkJsonInstanceFromJson(const std::string& json,
                            VkJsonInstance* instance,
                            std::string* errors) {
  return VkTypeFromJson(json, instance, errors);
}

std::string VkJsonDeviceToJson(const VkJsonDevice& device) {
  return VkTypeToJson(device);
}

bool VkJsonDeviceFromJson(const std::string& json,
                          VkJsonDevice* device,
                          std::string* errors) {
  return VkTypeFromJson(json, device, errors);
};

std::string VkJsonImageFormatPropertiesToJson(
    const VkImageFormatProperties& properties) {
  return VkTypeToJson(properties);
}

bool VkJsonImageFormatPropertiesFromJson(const std::string& json,
                                         VkImageFormatProperties* properties,
                                         std::string* errors) {
  return VkTypeFromJson(json, properties, errors);
};
""")
    f.close()
  gencom.run_clang_format(genfile)


def generate_vk_core_structs_init_code(version):
  """Generates code to initialize properties and features
  for structs based on its vulkan API version dependency.
  """
  properties_code, features_code = [], []

  for item in VK.VULKAN_CORES_AND_STRUCTS_MAPPING["versions"].get(version, []):
    for struct_name, struct_type in item.items():
      version_lower = version.lower()

      if "Properties" in struct_name:
        properties_code.extend([
            f"device.{version_lower}.properties.sType = {struct_type};",
            f"device.{version_lower}.properties.pNext = properties.pNext;",
            f"properties.pNext = &device.{version_lower}.properties;\n\n"
        ])

      elif "Features" in struct_name:
        features_code.extend([
            f"device.{version_lower}.features.sType = {struct_type};",
            f"device.{version_lower}.features.pNext = features.pNext;",
            f"features.pNext = &device.{version_lower}.features;\n\n"
        ])

  return "\n".join(properties_code), "\n".join(features_code)


def generate_vk_extension_structs_init_code(mapping, struct_category, next_pointer):
  """Generates Vulkan struct initialization code for given struct category (Properties/Features)
  based on its extension dependency.
  """
  generated_code = []

  for extension, struct_mappings in mapping.items():
    struct_var_name = get_vkjson_struct_variable_name(extension)
    extension_code = [
        f"  if (HasExtension(\"{extension}\", device.extensions)) {{",
        f"    device.{struct_var_name}.reported = true;"
    ]

    struct_count = 0
    for struct_mapping in struct_mappings:
      for struct_name, struct_type in struct_mapping.items():
        if struct_category in struct_name:
          struct_count += 1
          struct_instance = get_struct_name(struct_name)
          extension_code.extend([
              f"    device.{struct_var_name}.{struct_instance}.sType = {struct_type};",
              f"    device.{struct_var_name}.{struct_instance}.pNext = {next_pointer}.pNext;",
              f"    {next_pointer}.pNext = &device.{struct_var_name}.{struct_instance};"
          ])

    extension_code.append("  }\n")

    if struct_count > 0:
      generated_code.extend(extension_code)

  return "\n".join(generated_code)


def generate_vk_version_structs_initialization(version_data, struct_type_keyword, next_pointer):
  """Generates Vulkan struct initialization code for given struct category (Properties/Features)
  of vulkan api version s.
  """
  struct_initialization_code = []

  for struct_mapping in version_data:
    for struct_name, struct_type in struct_mapping.items():
      if struct_type_keyword in struct_name:
        struct_variable = get_struct_name(struct_name)
        struct_initialization_code.extend([
            f"device.{struct_variable}.sType = {struct_type};",
            f"device.{struct_variable}.pNext = {next_pointer}.pNext;",
            f"{next_pointer}.pNext = &device.{struct_variable};\n"
        ])

  return "\n".join(struct_initialization_code)


def gen_instance_cc():
  """Generates vkjson_instance.cc file.
  """
  genfile = os.path.join(os.path.dirname(__file__),
                         "..", "vkjson", "vkjson_instance.cc")

  with open(genfile, "w") as f:
    f.write(get_copyright_warnings())
    f.write("\n")

    f.write("""\
#ifndef VK_PROTOTYPES
#define VK_PROTOTYPES
#endif

#include "vkjson.h"

#include <algorithm>
#include <utility>

/*
 * This file is autogenerated by vkjson_generator.py. Do not edit directly.
 */
namespace {

bool EnumerateExtensions(const char* layer_name,
                         std::vector<VkExtensionProperties>* extensions) {
  VkResult result;
  uint32_t count = 0;
  result = vkEnumerateInstanceExtensionProperties(layer_name, &count, nullptr);
  if (result != VK_SUCCESS)
    return false;
  extensions->resize(count);
  result = vkEnumerateInstanceExtensionProperties(layer_name, &count,
                                                  extensions->data());
  if (result != VK_SUCCESS)
    return false;
  return true;
}

bool HasExtension(const char* extension_name,
                  const std::vector<VkExtensionProperties>& extensions) {
  return std::find_if(extensions.cbegin(), extensions.cend(),
                      [extension_name](const VkExtensionProperties& extension) {
                        return strcmp(extension.extensionName,
                                      extension_name) == 0;
                      }) != extensions.cend();
}
}  // anonymous namespace

VkJsonDevice VkJsonGetDevice(VkPhysicalDevice physical_device) {
  VkJsonDevice device;

  uint32_t extension_count = 0;
  vkEnumerateDeviceExtensionProperties(physical_device, nullptr,
                                       &extension_count, nullptr);
  if (extension_count > 0) {
    device.extensions.resize(extension_count);
    vkEnumerateDeviceExtensionProperties(
        physical_device, nullptr, &extension_count, device.extensions.data());
  }

  uint32_t layer_count = 0;
  vkEnumerateDeviceLayerProperties(physical_device, &layer_count, nullptr);
  if (layer_count > 0) {
    device.layers.resize(layer_count);
    vkEnumerateDeviceLayerProperties(physical_device, &layer_count,
                                     device.layers.data());
  }

  VkPhysicalDeviceProperties2 properties = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
      nullptr,
      {},
  };\n\n""")

    cc_code_properties = generate_vk_extension_structs_init_code(
    VK.VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING["extensions"], "Properties", "properties"
    )
    f.write(f'{cc_code_properties}\n')

    f.write("""\

  vkGetPhysicalDeviceProperties2(physical_device, &properties);
  device.properties = properties.properties;

  VkPhysicalDeviceFeatures2 features = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
      nullptr,
      {},
  };\n\n""")

    cc_code_features = generate_vk_extension_structs_init_code(
      VK.VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING["extensions"], "Features", "features"
    )
    f.write(f'{cc_code_features}\n')

    f.write("""\

  vkGetPhysicalDeviceFeatures2(physical_device, &features);
  device.features = features.features;

  vkGetPhysicalDeviceMemoryProperties(physical_device, &device.memory);

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count,
                                           nullptr);
  if (queue_family_count > 0) {
    device.queues.resize(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device, &queue_family_count, device.queues.data());
  }

  VkFormatProperties format_properties = {};
  for (VkFormat format = VK_FORMAT_R4G4_UNORM_PACK8;
       // TODO(http://b/171403054): avoid hard-coding last value in the
       // contiguous range
       format <= VK_FORMAT_ASTC_12x12_SRGB_BLOCK;
       format = static_cast<VkFormat>(format + 1)) {
    vkGetPhysicalDeviceFormatProperties(physical_device, format,
                                        &format_properties);
    if (format_properties.linearTilingFeatures ||
        format_properties.optimalTilingFeatures ||
        format_properties.bufferFeatures) {
      device.formats.insert(std::make_pair(format, format_properties));
    }
  }

  if (device.properties.apiVersion >= VK_API_VERSION_1_1) {
    for (VkFormat format = VK_FORMAT_G8B8G8R8_422_UNORM;
         // TODO(http://b/171403054): avoid hard-coding last value in the
         // contiguous range
         format <= VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM;
         format = static_cast<VkFormat>(format + 1)) {
      vkGetPhysicalDeviceFormatProperties(physical_device, format,
                                          &format_properties);
      if (format_properties.linearTilingFeatures ||
          format_properties.optimalTilingFeatures ||
          format_properties.bufferFeatures) {
        device.formats.insert(std::make_pair(format, format_properties));
      }
    }\n\n""")

    # Vulkan version data for VK_VERSION_1_1
    vk_version_data = VK.VULKAN_VERSIONS_AND_STRUCTS_MAPPING["VK_VERSION_1_1"]
    f.write(generate_vk_version_structs_initialization(vk_version_data, "Properties", "properties") + "\n")

    f.write("""\
    vkGetPhysicalDeviceProperties2(physical_device, &properties);\n\n""")

    features_initialization_code = generate_vk_version_structs_initialization(vk_version_data, "Features", "features")
    f.write(features_initialization_code)

    f.write("""\

    vkGetPhysicalDeviceFeatures2(physical_device, &features);

    VkPhysicalDeviceExternalFenceInfo external_fence_info = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_FENCE_INFO, nullptr,
        VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT};
    VkExternalFenceProperties external_fence_properties = {};

    for (VkExternalFenceHandleTypeFlagBits handle_type =
             VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT;
         handle_type <= VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT;
         handle_type =
             static_cast<VkExternalFenceHandleTypeFlagBits>(handle_type << 1)) {
      external_fence_info.handleType = handle_type;
      vkGetPhysicalDeviceExternalFenceProperties(
          physical_device, &external_fence_info, &external_fence_properties);
      if (external_fence_properties.exportFromImportedHandleTypes ||
          external_fence_properties.compatibleHandleTypes ||
          external_fence_properties.externalFenceFeatures) {
        device.external_fence_properties.insert(
            std::make_pair(handle_type, external_fence_properties));
      }
    }

    VkPhysicalDeviceExternalSemaphoreInfo external_semaphore_info = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO, nullptr,
        VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT};
    VkExternalSemaphoreProperties external_semaphore_properties = {};

    for (VkExternalSemaphoreHandleTypeFlagBits handle_type =
             VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
         handle_type <= VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT;
         handle_type = static_cast<VkExternalSemaphoreHandleTypeFlagBits>(
             handle_type << 1)) {
      external_semaphore_info.handleType = handle_type;
      vkGetPhysicalDeviceExternalSemaphoreProperties(
          physical_device, &external_semaphore_info,
          &external_semaphore_properties);
      if (external_semaphore_properties.exportFromImportedHandleTypes ||
          external_semaphore_properties.compatibleHandleTypes ||
          external_semaphore_properties.externalSemaphoreFeatures) {
        device.external_semaphore_properties.insert(
            std::make_pair(handle_type, external_semaphore_properties));
      }
    }
  }

  if (device.properties.apiVersion >= VK_API_VERSION_1_2) {\n""")

    cc_code_properties_11, cc_code_features_11 = generate_vk_core_structs_init_code("Core11")
    cc_code_properties_12, cc_code_features_12 = generate_vk_core_structs_init_code("Core12")
    cc_code_properties_13, cc_code_features_13 = generate_vk_core_structs_init_code("Core13")
    cc_code_properties_14, cc_code_features_14 = generate_vk_core_structs_init_code("Core14")

    f.write(cc_code_properties_11)
    f.write(cc_code_properties_12)
    f.write(f"vkGetPhysicalDeviceProperties2(physical_device, &properties);\n\n")
    f.write(cc_code_features_11)
    f.write(cc_code_features_12)
    f.write(f"vkGetPhysicalDeviceFeatures2(physical_device, &features);\n\n")
    f.write("""\
  }

  if (device.properties.apiVersion >= VK_API_VERSION_1_3) {\n""")
    f.write(cc_code_properties_13)
    f.write(f"vkGetPhysicalDeviceProperties2(physical_device, &properties);\n\n")
    f.write(cc_code_features_13)
    f.write(f"vkGetPhysicalDeviceFeatures2(physical_device, &features);\n\n")
    f.write("""\
  }

  if (device.properties.apiVersion >= VK_API_VERSION_1_4) {\n""")
    f.write(cc_code_properties_14)
    f.write(f"vkGetPhysicalDeviceProperties2(physical_device, &properties);\n\n")

    f.write("""\
if (device.core14.properties.copySrcLayoutCount > 0 || device.core14.properties.copyDstLayoutCount > 0 ) {
  if (device.core14.properties.copySrcLayoutCount > 0) {
    device.core14.copy_src_layouts.resize(device.core14.properties.copySrcLayoutCount);
    device.core14.properties.pCopySrcLayouts = device.core14.copy_src_layouts.data();
  }
  if (device.core14.properties.copyDstLayoutCount > 0) {
    device.core14.copy_dst_layouts.resize(device.core14.properties.copyDstLayoutCount);
    device.core14.properties.pCopyDstLayouts = device.core14.copy_dst_layouts.data();
  }
  vkGetPhysicalDeviceProperties2(physical_device, &properties);
}
    \n""")

    f.write(cc_code_features_14)
    f.write(f"vkGetPhysicalDeviceFeatures2(physical_device, &features);\n\n")
    f.write("""\
  }

  return device;
}

VkJsonInstance VkJsonGetInstance() {
  VkJsonInstance instance;
  VkResult result;
  uint32_t count;

  count = 0;
  result = vkEnumerateInstanceLayerProperties(&count, nullptr);
  if (result != VK_SUCCESS)
    return VkJsonInstance();
  if (count > 0) {
    std::vector<VkLayerProperties> layers(count);
    result = vkEnumerateInstanceLayerProperties(&count, layers.data());
    if (result != VK_SUCCESS)
      return VkJsonInstance();
    instance.layers.reserve(count);
    for (auto& layer : layers) {
      instance.layers.push_back(VkJsonLayer{layer, std::vector<VkExtensionProperties>()});
      if (!EnumerateExtensions(layer.layerName,
                               &instance.layers.back().extensions))
        return VkJsonInstance();
    }
  }

  if (!EnumerateExtensions(nullptr, &instance.extensions))
    return VkJsonInstance();

  const VkApplicationInfo app_info = {
      VK_STRUCTURE_TYPE_APPLICATION_INFO,
      nullptr,
      "vkjson_info",
      1,
      "",
      0,
      VK_API_VERSION_1_1,
  };
  VkInstanceCreateInfo instance_info = {
      VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      nullptr,
      0,
      &app_info,
      0,
      nullptr,
      0,
      nullptr,
  };
  VkInstance vkinstance;
  result = vkCreateInstance(&instance_info, nullptr, &vkinstance);
  if (result != VK_SUCCESS)
    return VkJsonInstance();

  count = 0;
  result = vkEnumeratePhysicalDevices(vkinstance, &count, nullptr);
  if (result != VK_SUCCESS) {
    vkDestroyInstance(vkinstance, nullptr);
    return VkJsonInstance();
  }

  std::vector<VkPhysicalDevice> devices(count, VK_NULL_HANDLE);
  result = vkEnumeratePhysicalDevices(vkinstance, &count, devices.data());
  if (result != VK_SUCCESS) {
    vkDestroyInstance(vkinstance, nullptr);
    return VkJsonInstance();
  }

  std::map<VkPhysicalDevice, uint32_t> device_map;
  const uint32_t sz = devices.size();
  instance.devices.reserve(sz);
  for (uint32_t i = 0; i < sz; ++i) {
    device_map.insert(std::make_pair(devices[i], i));
    instance.devices.emplace_back(VkJsonGetDevice(devices[i]));
  }

  result = vkEnumerateInstanceVersion(&instance.api_version);
  if (result != VK_SUCCESS) {
    vkDestroyInstance(vkinstance, nullptr);
    return VkJsonInstance();
  }

  count = 0;
  result = vkEnumeratePhysicalDeviceGroups(vkinstance, &count, nullptr);
  if (result != VK_SUCCESS) {
    vkDestroyInstance(vkinstance, nullptr);
    return VkJsonInstance();
  }

  VkJsonDeviceGroup device_group;
  std::vector<VkPhysicalDeviceGroupProperties> group_properties;
  group_properties.resize(count);
  for (auto& properties : group_properties) {
    properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES;
    properties.pNext = nullptr;
  }
  result = vkEnumeratePhysicalDeviceGroups(vkinstance, &count,
                                           group_properties.data());
  if (result != VK_SUCCESS) {
    vkDestroyInstance(vkinstance, nullptr);
    return VkJsonInstance();
  }
  for (auto properties : group_properties) {
    device_group.properties = properties;
    for (uint32_t i = 0; i < properties.physicalDeviceCount; ++i) {
      device_group.device_inds.push_back(
          device_map[properties.physicalDevices[i]]);
    }
    instance.device_groups.push_back(device_group);
  }

  vkDestroyInstance(vkinstance, nullptr);
  return instance;
}

\n""")

    f.close()
  gencom.run_clang_format(genfile)