#!/usr/bin/env python3
#
# Copyright 2019 The Android Open Source Project
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

"""
This file contains all supporting utility functions for vkjson_generator.py
"""

import dataclasses
import re
import vk as VK
import generator_common as gencom
from typing import List, get_origin, Tuple, Optional, get_args

CONST_PHYSICAL_DEVICE_FEATURE_2 = "VkPhysicalDeviceFeatures2"
CONST_PHYSICAL_DEVICE_PROPERTIES_2 = "VkPhysicalDeviceProperties2"
PROPERTIES = "Properties"
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

VK_FORMAT_RANGE_MAPPING = {}

def get_copyright_warnings():
    return COPYRIGHT_WARNINGS


def get_vkjson_struct_name(extension_name):
    """Gets the corresponding structure name from a Vulkan extension name.
    Example: "VK_KHR_shader_float16_int8" → "VkJsonKHRShaderFloat16Int8"
    """
    prefix_map = {
        "VK_KHR": "VkJsonKHR",
        "VK_EXT": "VkJsonExt",
        "VK_IMG": "VkJsonIMG",
        "VK_ANDROID": "VkJsonANDROID",
        "VK_AMD": "VkJsonAMD",
        "VK_MESA": "VkJsonMESA",
        "VK_NV": "VkJsonNV",
        "VK_QCOM": "VkJsonQCOM",
        "VK_ARM": "VkJsonARM",
        "VK_HUAWEI": "VkJsonHUAWEI",
        "VK_VALVE": "VkJsonVALVE",
        "VK_MSFT": "VkJsonMSFT",
        "VK_SEC": "VkJsonSEC",
        "VK_NVX": "VKJsonNVX",
    }

    for prefix, replacement in prefix_map.items():
        if extension_name.startswith(prefix):
            struct_name = replacement + extension_name[len(prefix) :]
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
        "VK_IMG_": "img_",
        "VK_ANDROID_": "android_",
        "VK_AMD_": "amd_",
        "VK_MESA_": "mesa_",
        "VK_NV_": "nv_",
        "VK_QCOM_": "qcom_",
        "VK_ARM_": "arm_",
        "VK_HUAWEI_": "huawei_",
        "VK_VALVE_": "valve_",
        "VK_MSFT_": "msft_",
        "VK_SEC_": "sec_",
        "VK_NVX_": "nvx_",
    }

    for prefix, replacement in prefix_map.items():
        if extension_name.startswith(prefix):
            # Match: word1_digits_word2 => word1_word2_digits (Example: ext_4444_formats -> ext_formats_4444)
            return re.sub(r"^([a-zA-Z]+)_(\d+)_([a-zA-Z]+)", r"\1_\3_\2", replacement + extension_name[len(prefix) :])
    return extension_name.lower()  # Default case if no known prefix matches


def normalize_variable_name(name, base_name):
    # Convert CamelCase to snake_case
    # Example: "ShaderFloat16Int8Features" → "shader_float16_int8_features"

    # Add underscore around digit+D (e.g., 2D, 3D)
    base_name = re.sub(r"(\d)(D)", r"_\1\2_", base_name)

    # Add underscore before any uppercase letter that follows a lowercase letter or digit
    initialString = re.sub(r"(?<=[a-z0-9])([A-Z])", r"_\1", base_name)

    # Add underscore between consecutive uppercase letters followed by a lowercase letter
    name = re.sub(r"([A-Z]+)([A-Z][a-z])", r"\1_\2", initialString).lower()

    # Replace X_d_ → Xd_
    name = re.sub(r"(\d+)_d_", r"\1d_", name)

    # Replace i_d_ or similar → id_
    name = re.sub(r"\bi_d_", "id_", name)

    # Replace X_bit_ → bitX_
    name = re.sub(r"(\d+)_bit_", r"bit\1_", name)

    # Replace 9_XX → XX_9
    if re.match(r"^\d", name):
        name = re.sub(r"^(\d+)_([a-zA-Z]+)", r"\2_\1", name)

    # Prefix with underscore if starting with digit
    if re.match(r"^\d", name):
        name = "_" + name
    return name


def get_struct_name(struct_name):
    """Gets corresponding instance name
    Example: "VkPhysicalDeviceShaderFloat16Int8FeaturesKHR" → "shader_float16_int8_features_khr"
    """
    # Remove "VkPhysicalDevice" prefix and any of the known suffixes
    prefixes_suffixes_to_remove = ("VkPhysicalDevice", "KHR", "EXT", "IMG", "ANDROID", "AMD", "MESA", "NV", "QCOM", "ARM", "HUAWEI", "VALVE", "MSFT")
    base_name = struct_name

    for affix in prefixes_suffixes_to_remove:
        base_name = base_name.removeprefix(affix).removesuffix(affix)
    variable_name = ""
    # Fix special cases
    variable_name = normalize_variable_name(variable_name, base_name)

    # Add back the correct suffix if it was removed
    suffix_map = {
        "KHR": "_khr",
        "EXT": "_ext",
        "IMG": "_img",
        "ANDROID": "_android",
        "AMD": "_amd",
        "NV": "_nv",
        "MESA": "_mesa",
        "VALVE": "_valve",
        "MSFT": "_msft",
        "QCOM": "_qcom",
        "ARM": "_arm",
        "HUAWEI": "_huawei",
    }
    for suffix, replacement in suffix_map.items():
        if struct_name.endswith(suffix):
            variable_name += replacement
            break

    # Handle specific exceptions
    special_cases = {"memory_properties": "memory", "ycbcr2_plane444_formats_features_ext": "ycbcr_2plane_444_formats_features_ext"}

    return special_cases.get(variable_name, variable_name)

def get_dataclass_list_members(class_name: str) -> List[Tuple[str, str]]:
    """Retrieves names and types of list members within a dataclass.
        A list of tuples, where each tuple contains (list_variable_name, list_element_type).
    """
    dataclass_type = get_dataclass_type(class_name)

    if not dataclass_type:
        return []
    """Catches the TypeError thrown by dataclasses.fields if dataclass_type
    is not a valid dataclass class or instance.
    """
    try:
        class_member_list = dataclasses.fields(dataclass_type)
    except TypeError:
        return []
    return [
        (field.name, get_args(field.type)[0].__name__)
        for field in class_member_list
        if get_origin(field.type) is list and get_args(field.type)
    ]


def get_class_name_from_alias(class_name):
    dataclass_type = get_dataclass_type(class_name)

    """Handles an exception that is thrown only when running unit tests.
    """
    if dataclass_type:
        try:
            return dataclass_type.__name__
        except AttributeError:
            return class_name
    return class_name

def get_dataclass_type(class_name):
    return getattr(VK, class_name, None)

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
    vkjson_entries = []

    for extension_name, struct_list in VK.VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING["extensions"].items():
        vkjson_struct_name = get_vkjson_struct_name(extension_name)
        vkjson_struct_variable_name = get_vkjson_struct_variable_name(extension_name)
        vkjson_entries.append(f"{vkjson_struct_name} {vkjson_struct_variable_name}")

        struct_entries = []

        f.write(f"struct {vkjson_struct_name} {{\n")
        f.write(f"  {vkjson_struct_name}() {{\n")
        f.write("    reported = false;\n")

        for struct_map in struct_list:
            for struct_name, _ in struct_map.items():
                variable_name = get_struct_name(struct_name)
                f.write(f"    memset(&{variable_name}, 0, sizeof({struct_name}));\n")
                struct_entries.append([struct_name, variable_name])

        f.write("  }\n")  # End of constructor
        f.write("  bool reported;\n")
        # Writing structure variables in vkjson_struct
        for struct_type, variable_name in struct_entries:
            variable_declaration = struct_type + " " + variable_name
            f.write(f"  {variable_declaration};\n")

        # Populating vkjson_struct with list variables from properties structure.
        properties_and_feature_structures = [struct_name for struct_name, _ in struct_entries]
        cpp_list_variables = get_dynamic_size_list_member_from_structures(
            properties_and_feature_structures,
        )
        for list_type, variable_name in cpp_list_variables:
            list_variable_declaration = "std::vector<" + list_type + ">" + " " + variable_name + ";"
            f.write(f"  {list_variable_declaration}\n")
        f.write("};\n\n")  # End of struct

    return vkjson_entries


def generate_vk_core_struct_definition(f):
    """Generates struct definition code for vulkan cores
    Example:
    struct VkJsonCore11 {
      VkPhysicalDeviceVulkan11Properties properties;
      VkPhysicalDeviceVulkan11Features features;
    };
    """
    vkjson_core_entries = []

    for version, items in VK.VULKAN_CORES_AND_STRUCTS_MAPPING["versions"].items():
        struct_name = f"VkJson{version}"
        vkjson_core_entries.append(f"{struct_name} {version.lower()}")

        f.write(f"struct {struct_name} {{\n")
        f.write(f"  {struct_name}() {{\n")  # Start of constructor
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

    return vkjson_core_entries


def generate_memset_statements(f):
    """Generates memset statements for all independent Vulkan structs and core Vulkan versions.
    This initializes struct instances to zero before use.

    Example:
      memset(&properties, 0, sizeof(VkPhysicalDeviceProperties));
      VkPhysicalDeviceProperties properties;
    """
    entries = []
    processed_entries = set()
    all_extension_independent_structs = VK.EXTENSION_INDEPENDENT_STRUCTS + VK.ADDITIONAL_EXTENSION_INDEPENDENT_STRUCTS
    # Process independent structs
    for dataclass_type in all_extension_independent_structs:
        class_name = dataclass_type
        variable_name = get_struct_name(class_name)
        # EDIT: updated as we are now taking the dataclass_type as 'string'
        key_str = f"{variable_name}@{class_name}"
        if key_str in processed_entries:
            continue
        f.write(f"memset(&{variable_name}, 0, sizeof({class_name}));\n")
        processed_entries.add(key_str)
        entries.append(f"{class_name} {variable_name}")

    return entries

def normalize_json_field_name(json_field_name):
    if json_field_name[0].isdigit():
        # This regex splits the name into three parts:
        # 1. The leading number (e.g., "16")
        # 2. The first "word" (e.g., "BIT" or "Bit")
        # 3. The rest of the string (e.g., "Format")
        pattern = re.compile(r"^(\d+)([A-Z]+(?=[A-Z]|$)|[A-Z][a-z]*)(.*)$")
        match = pattern.match(json_field_name)
        if not match:
            return json_field_name # Failsafe
        number, first_word, rest = match.groups()
        result = f"{first_word}{number}{rest}"
        is_acronym = len(first_word) > 1 and first_word.isupper()
        if is_acronym:
            return result
        else:
            return result[0].lower() + result[1:]
    else:
        if re.match(r"^[A-Z]{2,}", json_field_name):
            return json_field_name
        else:
            return json_field_name[0].lower() + json_field_name[1:]

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
                json_field_name = normalize_json_field_name(json_field_name)
                visitor_calls.append(f'visitor->Visit("{json_field_name}", &structs->{get_struct_name(struct_name)})')

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

        # Determine the correct variable name based on the struct type
        struct_var = "properties" if "Properties" in struct_name else "features" if "Features" in struct_name else "limits" if "Limits" in struct_name else None

        if not struct_var:
            continue  # Skip structs that don't match expected patterns

        template_code.append("template <typename Visitor>")
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
    """Emits visitor calls for Vulkan version structs"""
    for struct_map in VK.VULKAN_VERSIONS_AND_STRUCTS_MAPPING[version]:
        for struct_name, _ in struct_map.items():
            struct_var = get_struct_name(struct_name)
            # Converts struct_var from snake_case (e.g., point_clipping_properties)
            # to camelCase (e.g., pointClippingProperties) for struct_display_name.
            struct_display_name = re.sub(r"_([a-z])", lambda match: match.group(1).upper(), struct_var)
            f.write(f'visitor->Visit("{struct_display_name}", &device->{struct_var}) &&\n')


def generate_enum_traits():
    """Writes necessary enum definitions as Python Enum classes and generates C++ traits string."""
    cpp_enum_traits_content = ""
    # Iterate through all parsed enum types
    for enum_name, members in VK.ENUM_TRAITS_MAPPING.items():
        cpp_enum_traits_content += "\n"
        # --- C++ EnumTraits generation (skip for these enums as they are differently defined in xml) ---
        if enum_name == "VkImageLayout":
            continue
        cpp_enum_traits_content += f"template <>\nstruct EnumTraits<{enum_name}> {{\n"
        cpp_enum_traits_content += f"  static bool exist(uint32_t e) {{\n"
        cpp_enum_traits_content += f"    switch (e) {{\n"
        if members:
            for member, value in members.items():
                if value is not None and member:
                    cpp_enum_traits_content += f"      case {member}:\n"
        cpp_enum_traits_content += f"        return true;\n"
        cpp_enum_traits_content += f"    }}\n"
        cpp_enum_traits_content += f"    return false;\n"
        cpp_enum_traits_content += f"  }}\n"
        cpp_enum_traits_content += f"}};\n\n"

    return cpp_enum_traits_content


def generate_vk_core_structs_init_code(version):
    """Generates code to initialize properties and features
    for structs based on its vulkan API version dependency.
    """
    properties_code, features_code = [], []

    for item in VK.VULKAN_CORES_AND_STRUCTS_MAPPING["versions"].get(version, []):
        for struct_name, struct_type in item.items():
            version_lower = version.lower()
            target_code_list = None
            struct_field_name = None
            mapping_name = VK.STRUCT_EXTENDS_MAPPING.get(struct_name)
            if mapping_name and CONST_PHYSICAL_DEVICE_FEATURE_2 in mapping_name:
                target_code_list = features_code
                struct_field_name = "features"
            elif mapping_name and CONST_PHYSICAL_DEVICE_PROPERTIES_2 in mapping_name:
                target_code_list = properties_code
                struct_field_name = "properties"
            else:
                # TODO: b/415707715 (Add tests for exceptions for structs not extending (VkPhysicalDeviceProperties2/VkPhysicalDeviceFeatures2))
                raise Exception(f"Warning: Unknown Mapping: ' " f"for struct '{struct_name}'")

            if target_code_list is not None:
                target_code_list.extend(
                    [
                        f"device.{version_lower}.{struct_field_name}.sType = {struct_type};",
                        f"device.{version_lower}.{struct_field_name}.pNext = {struct_field_name}.pNext;",
                        f"{struct_field_name}.pNext = &device.{version_lower}.{struct_field_name};\n\n",
                    ]
                )

    return "\n".join(properties_code), "\n".join(features_code)


def generate_vk_extension_structs_init_code(mapping, struct_category):
    """Generates Vulkan struct initialization code for given struct category (Properties/Features)
    based on its extension dependency.
    """
    generated_code = []
    next_pointer = struct_category.lower()

    for extension, struct_mappings in mapping.items():

        if extension in gencom._VKJSON_BLOCKED_EXTENSIONS:
            continue

        struct_var_name = get_vkjson_struct_variable_name(extension)
        extension_code = [f'  if (HasExtension("{extension}", device.extensions)) {{', f"    device.{struct_var_name}.reported = true;"]
        struct_count = 0
        for struct_mapping in struct_mappings:
            for struct_name, struct_type in struct_mapping.items():
                # EDIT: Category detection improved using structextends mapping (VK.STRUCT_EXTENDS_MAPPING).
                # Correctly handles structs with ambiguous names (e.g., VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR)
                # that might contain 'Properties'/'Features' in name but extend only one (Features2/Properties2).
                mapping_name = VK.STRUCT_EXTENDS_MAPPING.get(struct_name)
                is_valid_struct_category = False
                if mapping_name:
                    is_valid_struct_category = (CONST_PHYSICAL_DEVICE_PROPERTIES_2 in mapping_name and struct_category == "Properties") or (
                        CONST_PHYSICAL_DEVICE_FEATURE_2 in mapping_name and struct_category == "Features"
                    )
                elif struct_category.lower() in struct_name.lower():
                    raise Exception(f"Warning: Unknown Mapping: ' " f"for struct '{struct_name}'")

                if is_valid_struct_category:
                    struct_count += 1
                    struct_instance = get_struct_name(struct_name)
                    extension_code.extend(
                        [
                            f"    device.{struct_var_name}.{struct_instance}.sType = {struct_type};",
                            f"    device.{struct_var_name}.{struct_instance}.pNext = {next_pointer}.pNext;",
                            f"    {next_pointer}.pNext = &device.{struct_var_name}.{struct_instance};",
                        ]
                    )

        extension_code.append("  }\n")

        if struct_count > 0:
            generated_code.extend(extension_code)

    return "\n".join(generated_code)


def generate_vk_version_structs_initialization(version_data, struct_type_keyword):
    """Generates Vulkan struct initialization code for given struct category (Properties/Features)
    of vulkan api version s.
    """
    struct_initialization_code = []
    next_pointer = struct_type_keyword.lower()

    for struct_mapping in version_data:
        for struct_name, struct_type in struct_mapping.items():
            if struct_type_keyword in struct_name:
                struct_variable = get_struct_name(struct_name)
                struct_initialization_code.extend(
                    [f"device.{struct_variable}.sType = {struct_type};", f"device.{struct_variable}.pNext = {next_pointer}.pNext;", f"{next_pointer}.pNext = &device.{struct_variable};\n"]
                )

    return "\n".join(struct_initialization_code)

"""
    A list of pairs, where each pair is (element_type, transformed_member_name).
    [('VkQueueFamilyProperties', 'queue_families')]
"""
def get_dynamic_size_list_member_from_structures(properties_and_feature_set: List) -> List[Tuple[str, str]]:
    entries: List[Tuple[str, str]] = []
    for struct_name in properties_and_feature_set:
        updated_struct_name = get_class_name_from_alias(struct_name)
        if PROPERTIES in struct_name and updated_struct_name in VK.STRUCT_WITH_DYNAMIC_SIZE_LIST_MAPPING:
            all_list_variables = get_dataclass_list_members(struct_name)
            for list_name, list_type in all_list_variables:
                if list_name in VK.STRUCT_WITH_DYNAMIC_SIZE_LIST_MAPPING[updated_struct_name]:
                    name = transform_list_member_name(list_name)
                    entries.append((list_type, name))
    return entries



def transform_list_member_name(member_name: str) -> str:
    def convert_camel_to_snake_case(camel_case_string: str) -> str:
        s1 = re.sub(r'(?<!^)([A-Z][a-z])', r'_\1', camel_case_string)
        s2 = re.sub(r'(?<=[A-Z])([A-Z][a-z])', r'_\1', s1)
        return s2.lower()

    """Converts a list member name (e.g., 'pCopySrcLayouts') to a more
    Pythonic snake_case variable name (e.g., 'copy_src_layouts').

    Removes leading 'p_' if present after initial camel to snake conversion.
    Examples:
        'pCopySrcLayouts' -> 'copy_src_layouts'
        'pDemoName' -> 'demo_name'
        'TestName' -> 'test_name'
    """
    snake_case_name = convert_camel_to_snake_case(member_name)
    if snake_case_name.startswith('p_'):
        snake_case_name = snake_case_name[2:]
    return snake_case_name


def generate_ext_dependent_structs_list_resizing_logic(
        vk_properties_setter_line: str,
) -> str:
    """
    This function iterates through a mapping of Vulkan extensions to their associated
    structs. For each relevant struct (e.g., properties structs that extend
    VkPhysicalDeviceProperties2), it generates C++ 'if' blocks. These blocks
    handle resizing of member arrays and assigning data pointers, then call
    vkGetPhysicalDeviceProperties2.
    """
    generated_code_blocks: List[str] = []

    for extension_name, struct_definitions_list in VK.VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING["extensions"].items():
        device_extension_variable = get_vkjson_struct_variable_name(extension_name)
        structures = [list(struct_definition.keys())[0] for struct_definition in struct_definitions_list]
        code = generate_list_resizing_logic(structures,
                                            vk_properties_setter_line,
                                            device_extension_variable,
                                            )
        if code != "":
            generated_code_blocks.append(code)
    return "\n".join(generated_code_blocks)


def generate_ext_independent_structs_list_resizing_logic(
        vulkan_api_version_mapping: List[dict[str, str]],
        vk_properties_setter_line: str,
) -> str:
    vulkan_api_version_structures = {structure for mapping in vulkan_api_version_mapping for structure in
                                     mapping.keys()}
    filtered_structures = []
    for structure in VK.EXTENSION_INDEPENDENT_STRUCTS:
        if structure in vulkan_api_version_structures:
            filtered_structures.append(structure)

    generated_code_blocks = generate_list_resizing_logic(filtered_structures,
                                                         vk_properties_setter_line,
                                                         )
    return generated_code_blocks


def generate_list_resizing_logic(
        structures: List[str],
        vk_properties_setter_line: str,
        extension: Optional[str] = None
) -> str:
    def _generate_if_condition_codeblock(
            if_condition: str,
            list_resize_codeblock: str,
            properties_setter_line: Optional[str] = None,
    ) -> str:
        vk_properties_setter_line_code = f"\n{properties_setter_line}" if properties_setter_line else ""
        return f"""
            if({if_condition}) {{
                {list_resize_codeblock}{vk_properties_setter_line_code}
            }}
        """

    def _generate_merged_if_blocks(
            outer_if_conditions_list: List[str],
            internal_resize_assignment_logic: List[str],
            properties_setter_line: str,
    ) -> str:
        formatted_conditions = [f"{cond} > 0" for cond in outer_if_conditions_list]
        combined_cpp_if_condition = " || ".join(formatted_conditions)
        combined_internal_logic_body = "\n".join(internal_resize_assignment_logic)

        return f"""{_generate_if_condition_codeblock(combined_cpp_if_condition,
                                                     combined_internal_logic_body,
                                                     properties_setter_line)}"""

    generated_code_blocks: List[str] = []
    dot_extension = f".{extension}" if extension else ""
    for structure in structures:
        struct_category_mapping = VK.STRUCT_EXTENDS_MAPPING.get(structure)
        resolved_structure_name = get_class_name_from_alias(structure)
        is_valid_struct = struct_category_mapping and (
                CONST_PHYSICAL_DEVICE_PROPERTIES_2 in struct_category_mapping
        ) and resolved_structure_name in VK.STRUCT_WITH_DYNAMIC_SIZE_LIST_MAPPING
        if is_valid_struct:
            dynamic_size_list_variables = VK.STRUCT_WITH_DYNAMIC_SIZE_LIST_MAPPING[resolved_structure_name]
            struct_instance_name = get_struct_name(structure)
            code_block_list = [] # list of pair(if condition logic, resizing code)
            for list_name in dynamic_size_list_variables:
                if list_name in VK.LIST_TYPE_FIELD_AND_SIZE_MAPPING:
                    list_size_field_name = VK.LIST_TYPE_FIELD_AND_SIZE_MAPPING[list_name]
                    condition_expression = f"device{dot_extension}.{struct_instance_name}.{list_size_field_name}"
                    cpp_list_variable_name = transform_list_member_name(list_name)
                    resize_code_block = f"""device{dot_extension}.{cpp_list_variable_name}.resize({condition_expression});
device{dot_extension}.{struct_instance_name}.{list_name} = device{dot_extension}.{cpp_list_variable_name}.data();"""
                    code_block_list.append((condition_expression, resize_code_block))

            list_resize_code_blocks: List[str] = []
            set_property_setter_line_inside = len(code_block_list) == 1
            for condition, resize_code_block in code_block_list:
                list_resize_code_blocks.append(
                    _generate_if_condition_codeblock(
                        f"{condition} > 0",
                        resize_code_block,
                        vk_properties_setter_line if set_property_setter_line_inside else None,
                    )
                )

            if len(list_resize_code_blocks) == 1:
                generated_code_blocks.append(list_resize_code_blocks[0])

            elif len(list_resize_code_blocks) > 1:
                generated_code_blocks.append(
                    _generate_merged_if_blocks(
                        [condition_expression for condition_expression, _ in code_block_list],
                        list_resize_code_blocks,
                        vk_properties_setter_line,
                    )
                )
    return "\n".join(generated_code_blocks)

def find_contiguous_ranges(format_data):
  """Finds contiguous ranges of format values from a list of enums for a given extension or Vulkan API version.
    For example, for the format data for the below extension:
        "VK_EXT_ycbcr_2plane_444_formats": [
            ("VK_FORMAT_G8_B8R8_2PLANE_444_UNORM_EXT", 1000330000),
            ("VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16_EXT", 1000330001),
            ("VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16_EXT", 1000330002),
            ("VK_FORMAT_G16_B16R16_2PLANE_444_UNORM_EXT", 1000330003),
        ]

    The range of formats is:
    [(VK_FORMAT_G8_B8R8_2PLANE_444_UNORM_EXT,VK_FORMAT_G16_B16R16_2PLANE_444_UNORM_EXT)]
    """
  sorted_data = sorted(format_data, key=lambda x: x[1])
  contiguous_ranges = []
  start_name, start_val = sorted_data[0]
  prev_name, prev_val = start_name, start_val
  for name, val in sorted_data[1:]:
    if val != prev_val + 1:
      # Range break
      contiguous_ranges.append((start_name, prev_name))
      start_name = name
    prev_name, prev_val = name, val

  # Append the final range
  contiguous_ranges.append((start_name, prev_name))
  return contiguous_ranges


def generate_format_range_map():
  """Returns a map from Vulkan API versions and extensions to their contiguous ranges of format enums.

    For example, for 'VK_VERSION_1_3', the map contains:
    'VK_VERSION_1_3': [
        ('VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK', 'VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK'),
        ('VK_FORMAT_G8_B8R8_2PLANE_444_UNORM', 'VK_FORMAT_G16_B16R16_2PLANE_444_UNORM'),
        ('VK_FORMAT_A4R4G4B4_UNORM_PACK16', 'VK_FORMAT_A4B4G4R4_UNORM_PACK16')
    ]
    """
  for key, format_data in VK.VK_FORMAT_MAPPING.items():
    VK_FORMAT_RANGE_MAPPING[key] = find_contiguous_ranges(format_data)


def generate_vk_format_init_code(vk_version_api: str = None):
    """
    Generates C++ code to initialize Vulkan format enums based on either a Vulkan API version
    or an extension name.

    If `vk_version_api` is None, the function processes all extensions in `VK_FORMAT_RANGE_MAPPING`.
    Otherwise, it generates code for the specified Vulkan version.

    Example output for an extension:
        if (HasExtension("VK_EXT_texture_compression_astc_hdr", device.extensions)) {
          ...
        }
    """
    generated_code = []

    def generate_format_block(start_format: str, end_format: str):
        if start_format != end_format:
            return [
                f"  for (VkFormat format = {start_format};",
                f"       format <= {end_format};",
                f"       format = static_cast<VkFormat>(format + 1)) {{",
                f"    vkGetPhysicalDeviceFormatProperties(physical_device, format,",
                f"                                       &format_properties);",
                f"    device.formats.insert(std::make_pair(format, format_properties));",
                f"  }}"
            ]
        else:
            return [
                f"  VkFormat format = {start_format};",
                f"  vkGetPhysicalDeviceFormatProperties(physical_device, format,",
                f"                                       &format_properties);",
                f"  device.formats.insert(std::make_pair(format, format_properties));"
            ]

    if not vk_version_api:
        # Process all extensions
        for key, format_ranges in VK_FORMAT_RANGE_MAPPING.items():
            if key.startswith("VK_VERSION_"):
                continue
            for start_format, end_format in format_ranges:
                format_code = [f'if (HasExtension("{key}", device.extensions)) {{']
                format_code.extend(generate_format_block(start_format, end_format))
                format_code.append("}\n")
                generated_code.extend(format_code)
    else:
        # Process a specific Vulkan version
        if vk_version_api not in VK_FORMAT_RANGE_MAPPING:
            return ""
        for start_format, end_format in VK_FORMAT_RANGE_MAPPING[vk_version_api]:
            format_code = generate_format_block(start_format, end_format)
            format_code.append("")  # Add newline after each block
            generated_code.extend(format_code)

    return "\n".join(generated_code + [""])
