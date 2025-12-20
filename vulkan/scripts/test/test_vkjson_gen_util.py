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

"""
This file contains unit tests for vkjson_gen_util.py
Each test class focuses on one specific util function.
"""
import ctypes
import unittest

from dataclasses import dataclass
from enum import Enum
from typing import List
from unittest.mock import patch

import base_test_helper as helper
import vkjson_gen_util as src


class TestGetCopyrightWarnings(unittest.TestCase):

    def test_checks_for_c_comment_format(self):
        stripped_block = src.get_copyright_warnings().strip()
        is_block_comment = stripped_block.startswith("/*") and stripped_block.endswith("*/")

        is_line_comment = all(
            not line.strip() or line.strip().startswith("//")
            for line in stripped_block.splitlines()
        )

        self.assertTrue(is_block_comment or is_line_comment)

    def test_checks_for_essential_components(self):
        result = src.get_copyright_warnings()

        self.assertTrue("Copyright (c) 2015-2016 The Khronos Group Inc." in result)
        self.assertTrue("Copyright (c) 2015-2016 Valve Corporation" in result)
        self.assertTrue("Copyright (c) 2015-2016 LunarG, Inc." in result)
        self.assertTrue("Copyright (c) 2015-2016 Google, Inc." in result)
        self.assertTrue("Licensed under the Apache License, Version 2.0" in result)
        self.assertTrue("http://www.apache.org/licenses/LICENSE-2.0" in result)
        self.assertTrue("WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND" in result)


class TestGetVkjsonStructName(unittest.TestCase):

    def test_KHR_extension(self):
        self.assertEqual(
            "VkJsonKHRShaderFloat16Int8",
            src.get_vkjson_struct_name("VK_KHR_shader_float16_int8")
        )

    def test_EXT_extension(self):
        self.assertEqual(
            "VkJsonExtTransformFeedback",
            src.get_vkjson_struct_name("VK_EXT_transform_feedback"),
        )

    def test_IMG_extension(self):
        self.assertEqual(
            "VkJsonIMGRelaxedLineRasterization",
            src.get_vkjson_struct_name("VK_IMG_relaxed_line_rasterization")
        )

    def test_extension_with_unknown_prefix(self):
        self.assertEqual(
            "VkJsonExtVKFUCHSIAExternalSemaphore",
            src.get_vkjson_struct_name("VK_FUCHSIA_external_semaphore")
        )

    def test_extension_with_numeric_suffixes(self):
        self.assertEqual(
            "VkJsonExtImage2dViewOf3d",
            src.get_vkjson_struct_name("VK_EXT_image_2d_view_of_3d")
        )


class TestGetVkjsonStructVariableName(unittest.TestCase):

    def test_KHR_extension(self):
        self.assertEqual(
            "khr_shader_float16_int8",
            src.get_vkjson_struct_variable_name("VK_KHR_shader_float16_int8"),
        )

    def test_EXT_extension(self):
        self.assertEqual(
            "ext_transform_feedback",
            src.get_vkjson_struct_variable_name("VK_EXT_transform_feedback"),
        )

    def test_IMG_extension(self):
        self.assertEqual(
            "img_relaxed_line_rasterization",
            src.get_vkjson_struct_variable_name("VK_IMG_relaxed_line_rasterization")
        )

    def test_extension_with_unknown_prefix(self):
        self.assertEqual(
            "vk_fuchsia_external_semaphore",
            src.get_vkjson_struct_variable_name("VK_FUCHSIA_external_semaphore")
        )

    def test_extension_with_numeric_prefixes(self):
        self.assertEqual(
            "ext_image_2d_view_of_3d",
            src.get_vkjson_struct_variable_name("VK_EXT_image_2d_view_of_3d"),
        )


class TestGetStructName(unittest.TestCase):

    def test_single_word_suffix(self):
        self.assertEqual(
            "properties",
            src.get_struct_name("VkPhysicalDeviceProperties"),
        )

    def test_multi_word_suffix(self):
        self.assertEqual(
            "sampler_ycbcr_conversion_features",
            src.get_struct_name("VkPhysicalDeviceSamplerYcbcrConversionFeatures"),
        )

    def test_KHR_suffix(self):
        self.assertEqual(
            "float_controls_properties_khr",
            src.get_struct_name("VkPhysicalDeviceFloatControlsPropertiesKHR"),
        )

    def test_EXT_suffix(self):
        self.assertEqual(
            "line_rasterization_features_ext",
            src.get_struct_name("VkPhysicalDeviceLineRasterizationFeaturesEXT")
        )

    def test_IMG_suffix(self):
        self.assertEqual(
            "relaxed_line_rasterization_features_img",
            src.get_struct_name("VkPhysicalDeviceRelaxedLineRasterizationFeaturesIMG")
        )

    def test_suffix_starting_with_ID(self):
        self.assertEqual(
            "id_properties",
            src.get_struct_name("VkPhysicalDeviceIDProperties")
        )
        # TODO (b/401184058)- Fix source code
        # self.assertEqual(
        #     "id_features",
        #     src.get_struct_name("VkPhysicalDeviceIDFeatures")
        # )

    def test_suffix_containing_number(self):
        self.assertEqual(
            "vulkan11_properties",
            src.get_struct_name("VkPhysicalDeviceVulkan11Properties"),
        )

    def test_suffix_starting_with_2d(self):
        self.assertEqual(
            "_2d_view_features_ext",
            src.get_struct_name("VkPhysicalDevice2DViewFeaturesEXT"),
        )

    def test_suffix_starting_with_3d(self):
        self.assertEqual(
            "_3d_features_ext",
            src.get_struct_name("VkPhysicalDevice3DFeaturesEXT"),
        )

    def test_suffix_containing_2d_3d(self):
        self.assertEqual(
            "image_2d_view_of_3d_features_ext",
            src.get_struct_name("VkPhysicalDeviceImage2DViewOf3DFeaturesEXT")
        )

    def test_suffix_starting_with_8bit_16bit(self):
        self.assertEqual(
            "bit8_storage_features_khr",
            src.get_struct_name("VkPhysicalDevice8BitStorageFeaturesKHR"),
        )
        # TODO (b/401184058)- Fix source code
        # self.assertEqual(
        #     "bit16_storage_properties",
        #     src.get_struct_name("VkPhysicalDevice16BitStorageProperties")
        # )

    def test_special_case_of_memory_properties(self):
        self.assertEqual(
            "memory",
            src.get_struct_name("VkPhysicalDeviceMemoryProperties"),
        )


class TestGenerateExtensionStructDefinition(helper.BaseMockCodeFileTest):

    @patch('vkjson_gen_util.VK')
    def test_extension_with_single_struct(self, mock_vk):
        mock_vk.VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING = {
            "extensions": {
                "VK_KHR_driver_state": [
                    {"VkPhysicalDeviceDriverPropertiesKHR": "VK_STRUCT_DRIVER"}
                ]
            }
        }

        extension_struct = src.get_vkjson_struct_name("VK_KHR_driver_state")
        struct_name = src.get_struct_name("VkPhysicalDeviceDriverPropertiesKHR")

        expected_lines = (
            f"""struct {extension_struct} {{
              {extension_struct}() {{
                reported = false;
                memset(&{struct_name}, 0, sizeof(VkPhysicalDeviceDriverPropertiesKHR));
              }}
              bool reported;
              VkPhysicalDeviceDriverPropertiesKHR {struct_name};
            }};"""
        )

        self.assertEqual(
            [f"{extension_struct} {src.get_vkjson_struct_variable_name("VK_KHR_driver_state")}"],
            src.generate_extension_struct_definition(self.mock_file)
        )
        self.assertCodeFileWrite(expected_lines)

    @patch('vkjson_gen_util.VK')
    def test_extension_with_multiple_structs(self, mock_vk):
        mock_vk.VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING = {
            "extensions": {
                "VK_KHR_variable_pointers": [
                    {"VkPhysicalDeviceVariablePointerFeaturesKHR": "VK_STRUCT_VARIABLE",
                     "VkPhysicalDeviceVariablePointersFeaturesKHR": "VK_STRUCT_VARIABLES"},
                    {"SomeThingElse": "SOME_OTHER_STRUCT"}
                ]
            }
        }

        extension_struct = src.get_vkjson_struct_name("VK_KHR_variable_pointers")
        struct_1_name = src.get_struct_name("VkPhysicalDeviceVariablePointerFeaturesKHR")
        struct_2_name = src.get_struct_name("VkPhysicalDeviceVariablePointersFeaturesKHR")
        struct_3_name = src.get_struct_name("SomeThingElse")

        expected_lines = (
            f"""struct {extension_struct} {{
              {extension_struct}() {{
                reported = false;
                memset(&{struct_1_name}, 0, sizeof(VkPhysicalDeviceVariablePointerFeaturesKHR));
                memset(&{struct_2_name}, 0, sizeof(VkPhysicalDeviceVariablePointersFeaturesKHR));
                memset(&{struct_3_name}, 0, sizeof(SomeThingElse));
              }}
              bool reported;
              VkPhysicalDeviceVariablePointerFeaturesKHR {struct_1_name};
              VkPhysicalDeviceVariablePointersFeaturesKHR {struct_2_name};
              SomeThingElse {struct_3_name};
            }};"""
        )

        self.assertEqual(
            [
                f"{extension_struct} {src.get_vkjson_struct_variable_name("VK_KHR_variable_pointers")}"],
            src.generate_extension_struct_definition(self.mock_file)
        )
        self.assertCodeFileWrite(expected_lines)

    @patch('vkjson_gen_util.VK')
    def test_extension_without_structs(self, mock_vk):
        mock_vk.VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING = {
            "extensions": {
                "VK_EXT_image_2d_view_of_3d": []
            }
        }

        extension_struct = src.get_vkjson_struct_name("VK_EXT_image_2d_view_of_3d")

        expected_lines = (
            f"""struct {extension_struct} {{
              {extension_struct}() {{
                reported = false;
              }}
              bool reported;
            }};"""
        )

        self.assertEqual(
            [
                f"{extension_struct} {src.get_vkjson_struct_variable_name("VK_EXT_image_2d_view_of_3d")}"],
            src.generate_extension_struct_definition(self.mock_file)
        )
        self.assertCodeFileWrite(expected_lines)

    @patch('vkjson_gen_util.VK')
    def test_multiple_extensions(self, mock_vk):
        mock_vk.VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING = {
            "extensions": {
                "VK_EXT_custom_border_color": [
                    {"VkPhysicalDeviceCustomBorderColorFeatsEXT": "VK_STRUCT_COLOR"}
                ],
                "VK_KHR_shader_float_controls": [
                    {"VkPhysicalDeviceFloatControlsFeatsKHR": "VK_STRUCT_FLOAT"}
                ]
            }
        }

        extension_1_struct = src.get_vkjson_struct_name("VK_EXT_custom_border_color")
        struct_1_name = src.get_struct_name("VkPhysicalDeviceCustomBorderColorFeatsEXT")
        extension_2_struct = src.get_vkjson_struct_name("VK_KHR_shader_float_controls")
        struct_2_name = src.get_struct_name("VkPhysicalDeviceFloatControlsFeatsKHR")

        expected_lines = (
            f"""struct {extension_1_struct} {{
              {extension_1_struct}() {{
                reported = false;
                memset(&{struct_1_name}, 0, sizeof(VkPhysicalDeviceCustomBorderColorFeatsEXT));
              }}
              bool reported;
              VkPhysicalDeviceCustomBorderColorFeatsEXT {struct_1_name};
            }};

            struct {extension_2_struct} {{
              {extension_2_struct}() {{
                reported = false;
                memset(&{struct_2_name}, 0, sizeof(VkPhysicalDeviceFloatControlsFeatsKHR));
              }}
              bool reported;
              VkPhysicalDeviceFloatControlsFeatsKHR {struct_2_name};
            }};"""
        )

        self.assertEqual(
            [
                f"{extension_1_struct} {src.get_vkjson_struct_variable_name("VK_EXT_custom_border_color")}",
                f"{extension_2_struct} {src.get_vkjson_struct_variable_name("VK_KHR_shader_float_controls")}",
            ],
            src.generate_extension_struct_definition(self.mock_file)
        )
        self.assertCodeFileWrite(expected_lines)

    @patch('vkjson_gen_util.VK')
    def test_empty_extensions(self, mock_vk):
        mock_vk.VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING = {
            "extensions": {}
        }

        self.assertEqual([], src.generate_extension_struct_definition(self.mock_file))
        self.assertCodeFileWrite("")


class TestGenerateVkCoreStructDefinition(helper.BaseMockCodeFileTest):

    @patch('vkjson_gen_util.VK')
    def test_core_with_single_struct(self, mock_vk):
        mock_vk.VULKAN_CORES_AND_STRUCTS_MAPPING = {
            "versions": {
                "Core11": [
                    {"VkVulkan11Properties": "VK_STRUCT_TYPE_VULKAN_1_1_PROPERTIES"}
                ]
            }
        }

        expected_lines = (
            """struct VkJsonCore11 {
              VkJsonCore11() {
                memset(&properties, 0, sizeof(VkVulkan11Properties));
              }
              VkVulkan11Properties properties;
            };"""
        )

        self.assertEqual(
            ["VkJsonCore11 core11"],
            src.generate_vk_core_struct_definition(self.mock_file)
        )
        self.assertCodeFileWrite(expected_lines)

    @patch('vkjson_gen_util.VK')
    def test_core_with_multiple_structs(self, mock_vk):
        mock_vk.VULKAN_CORES_AND_STRUCTS_MAPPING = {
            "versions": {
                "Core13": [
                    {"VkVulkan13Features": "VK_STRUCT_TYPE_VULKAN_1_3_FEATURES",
                     "VkVulkan13Properties": "VK_STRUCT_TYPE_VULKAN_1_3_PROPERTIES"}
                ]
            }
        }

        expected_lines = (
            """struct VkJsonCore13 {
              VkJsonCore13() {
                memset(&features, 0, sizeof(VkVulkan13Features));
                memset(&properties, 0, sizeof(VkVulkan13Properties));
              }
              VkVulkan13Features features;
              VkVulkan13Properties properties;
            };"""
        )

        self.assertEqual(
            ["VkJsonCore13 core13"],
            src.generate_vk_core_struct_definition(self.mock_file)
        )
        self.assertCodeFileWrite(expected_lines)

    @patch('vkjson_gen_util.VK')
    def test_core14(self, mock_vk):
        mock_vk.VULKAN_CORES_AND_STRUCTS_MAPPING = {
            "versions": {
                "Core14": [
                    {"VkVulkan14Properties": "VK_STRUCT_TYPE_VULKAN_1_4_PROPERTIES"},
                    {"VkVulkan14Features": "VK_STRUCT_TYPE_VULKAN_1_4_FEATURES"}
                ]
            }
        }

        expected_lines = (
            """struct VkJsonCore14 {
              VkJsonCore14() {
                memset(&properties, 0, sizeof(VkVulkan14Properties));
                memset(&features, 0, sizeof(VkVulkan14Features));
              }
              VkVulkan14Properties properties;
              VkVulkan14Features features;
              std::vector<VkImageLayout> copy_src_layouts;
              std::vector<VkImageLayout> copy_dst_layouts;
            };"""
        )

        self.assertEqual(
            ["VkJsonCore14 core14"],
            src.generate_vk_core_struct_definition(self.mock_file)
        )
        self.assertCodeFileWrite(expected_lines)

    @patch('vkjson_gen_util.VK')
    def test_core_without_structs(self, mock_vk):
        mock_vk.VULKAN_CORES_AND_STRUCTS_MAPPING = {
            "versions": {
                "Core10": []
            }
        }

        expected_lines = (
            """struct VkJsonCore10 {
              VkJsonCore10() { }
            };"""
        )

        self.assertEqual(
            ["VkJsonCore10 core10"],
            src.generate_vk_core_struct_definition(self.mock_file)
        )
        self.assertCodeFileWrite(expected_lines)

    @patch('vkjson_gen_util.VK')
    def test_multiple_cores(self, mock_vk):
        mock_vk.VULKAN_CORES_AND_STRUCTS_MAPPING = {
            "versions": {
                "Core11": [
                    {"VkVulkan11Properties": "VK_STRUCT_TYPE_VULKAN_1_1_PROPERTIES"}
                ],
                "Core12": [
                    {"VkVulkan12Features": "VK_STRUCT_TYPE_VULKAN_1_2_FEATURES"}
                ]
            }
        }

        expected_lines = (
            """struct VkJsonCore11 {
              VkJsonCore11() {
                memset(&properties, 0, sizeof(VkVulkan11Properties));
              }
              VkVulkan11Properties properties;
            };

            struct VkJsonCore12 {
              VkJsonCore12() {
                memset(&features, 0, sizeof(VkVulkan12Features));
              }
              VkVulkan12Features features;
            };"""
        )

        self.assertEqual(
            [
                "VkJsonCore11 core11",
                "VkJsonCore12 core12"
            ],
            src.generate_vk_core_struct_definition(self.mock_file)
        )
        self.assertCodeFileWrite(expected_lines)

    @patch('vkjson_gen_util.VK')
    def test_no_cores(self, mock_vk):
        mock_vk.VULKAN_CORES_AND_STRUCTS_MAPPING = {
            "versions": {}
        }

        self.assertEqual([], src.generate_vk_core_struct_definition(self.mock_file))
        self.assertCodeFileWrite("")


class TestGenerateMemsetStatements(helper.BaseMockCodeFileTest):

    @patch('vkjson_gen_util.VK')
    def test_multiple_structs(self, mock_vk):
        mock_vk.EXTENSION_INDEPENDENT_STRUCTS = [
            'VkCustomExtension1',
            'VkCustomExtension2',
            'VkCustomExtension3'
        ]
        mock_vk.ADDITIONAL_EXTENSION_INDEPENDENT_STRUCTS = []

        struct_1_name = src.get_struct_name("VkCustomExtension1")
        struct_2_name = src.get_struct_name("VkCustomExtension2")
        struct_3_name = src.get_struct_name("VkCustomExtension3")

        expected_lines = (
            f"""memset(&{struct_1_name}, 0, sizeof(VkCustomExtension1));
            memset(&{struct_2_name}, 0, sizeof(VkCustomExtension2));
            memset(&{struct_3_name}, 0, sizeof(VkCustomExtension3));
            """
        )

        self.assertEqual(
            [
                f"VkCustomExtension1 {struct_1_name}",
                f"VkCustomExtension2 {struct_2_name}",
                f"VkCustomExtension3 {struct_3_name}",
            ],
            src.generate_memset_statements(self.mock_file)
        )

        self.assertCodeFileWrite(expected_lines)

    @patch('vkjson_gen_util.VK')
    def test_no_structs(self, mock_vk):
        mock_vk.EXTENSION_INDEPENDENT_STRUCTS = []

        self.assertEqual([], src.generate_memset_statements(self.mock_file))
        self.assertCodeFileWrite("")


class TestGenerateExtensionStructTemplate(helper.BaseCodeAssertTest):

    @patch('vkjson_gen_util.VK')
    def test_extension_with_single_struct(self, mock_vk):
        mock_vk.VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING = {
            "extensions": {
                "VK_KHR_driver_state": [
                    {"VkPhysicalDeviceDriverPropertiesKHR": "VK_STRUCT_DRIVER"}
                ]
            }
        }

        self.assertCodeEqual(
            f"""template <typename Visitor>
            inline bool Iterate(Visitor* visitor, {src.get_vkjson_struct_name("VK_KHR_driver_state")}* structs) {{
              return visitor->Visit("driverPropertiesKHR", &structs->{src.get_struct_name("VkPhysicalDeviceDriverPropertiesKHR")});
            }}""",
            src.generate_extension_struct_template()
        )

    # TODO (b/401184058)- Fix source code
    @patch('vkjson_gen_util.VK')
    def extension_with_multiple_structs(self, mock_vk):
        mock_vk.VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING = {
            "extensions": {
                "VK_KHR_variable_pointers": [
                    {"VkPhysicalDeviceShader": "VK_STRUCT_SHADER",
                     "SomeThingElse": "SOME_OTHER_STRUCT"},
                    {"VkPhysicalDeviceVulkan11Properties": "VK_STRUCT_VULKAN"},
                    {"VkPhysicalDevice16BitStorageFeaturesKHR": "VK_STRUCT_STORAGE"}
                ]
            }
        }

        self.assertCodeEqual(
            f"""template <typename Visitor>
            inline bool Iterate(Visitor* visitor, {src.get_vkjson_struct_name("VK_KHR_variable_pointers")}* structs) {{
              return visitor->Visit("shader", &structs->{src.get_struct_name("VkPhysicalDeviceShader")})&&
                    visitor->Visit("someThingElse", &structs->{src.get_struct_name("SomeThingElse")})&&
                    visitor->Visit("vulkan11Properties", &structs->{src.get_struct_name("VkPhysicalDeviceVulkan11Properties")})&&
                    visitor->Visit("bit16StorageFeaturesKHR", &structs->{src.get_struct_name("VkPhysicalDevice16BitStorageFeaturesKHR")});
            }}""",
            src.generate_extension_struct_template()
        )

    @patch('vkjson_gen_util.VK')
    def test_extension_without_structs(self, mock_vk):
        mock_vk.VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING = {
            "extensions": {
                "VK_EXT_image_2d_view_of_3d": []
            }
        }

        self.assertCodeEqual(
            f"""template <typename Visitor>
            inline bool Iterate(Visitor* visitor, {src.get_vkjson_struct_name("VK_EXT_image_2d_view_of_3d")}* structs) {{
              return;
            }}""",
            src.generate_extension_struct_template()
        )

    @patch('vkjson_gen_util.VK')
    def test_multiple_extensions(self, mock_vk):
        mock_vk.VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING = {
            "extensions": {
                "VK_EXT_custom_border_color": [
                    {"VkPhysicalDeviceCustomBorderColorFeatsEXT": "VK_STRUCT_COLOR"}
                ],
                "VK_KHR_shader_float_controls": [
                    {"VkPhysicalDeviceFloatControlsFeatsKHR": "VK_STRUCT_FLOAT"}
                ]
            }
        }

        self.assertCodeEqual(
            f"""template <typename Visitor>
            inline bool Iterate(Visitor* visitor, {src.get_vkjson_struct_name("VK_EXT_custom_border_color")}* structs) {{
              return visitor->Visit("customBorderColorFeatsEXT", &structs->{src.get_struct_name("VkPhysicalDeviceCustomBorderColorFeatsEXT")});
            }}

            template <typename Visitor>
            inline bool Iterate(Visitor* visitor, {src.get_vkjson_struct_name("VK_KHR_shader_float_controls")}* structs) {{
              return visitor->Visit("floatControlsFeatsKHR", &structs->{src.get_struct_name("VkPhysicalDeviceFloatControlsFeatsKHR")});
            }}""",
            src.generate_extension_struct_template()
        )

    @patch('vkjson_gen_util.VK')
    def test_empty_extensions(self, mock_vk):
        mock_vk.VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING = {
            "extensions": {}
        }

        self.assertEqual("", src.generate_extension_struct_template())


class TestGenerateCoreTemplate(helper.BaseCodeAssertTest):

    @patch('vkjson_gen_util.VK')
    def test_core_with_single_struct(self, mock_vk):
        mock_vk.VULKAN_CORES_AND_STRUCTS_MAPPING = {
            "versions": {
                "Core11": [
                    {"VkVulkan11Properties": "VK_STRUCT_TYPE_VULKAN_1_1_PROPERTIES"}
                ]
            }
        }

        self.assertCodeEqual(
            f"""template <typename Visitor>
            inline bool Iterate(Visitor* visitor, VkJsonCore11* core) {{
              return visitor->Visit("properties", &core->properties);
            }}""",
            src.generate_core_template()
        )

    @patch('vkjson_gen_util.VK')
    def test_core_with_multiple_structs(self, mock_vk):
        mock_vk.VULKAN_CORES_AND_STRUCTS_MAPPING = {
            "versions": {
                "Core13": [
                    {"VkVulkan13Features": "VK_STRUCT_TYPE_VULKAN_1_3_FEATURES",
                     "VkVulkan13Properties": "VK_STRUCT_TYPE_VULKAN_1_3_PROPERTIES"}
                ]
            }
        }

        self.assertCodeEqual(
            f"""template <typename Visitor>
            inline bool Iterate(Visitor* visitor, VkJsonCore13* core) {{
              return visitor->Visit("features", &core->features) &&
                    visitor->Visit("properties", &core->properties);
            }}""",
            src.generate_core_template()
        )

    @patch('vkjson_gen_util.VK')
    def test_multiple_cores(self, mock_vk):
        mock_vk.VULKAN_CORES_AND_STRUCTS_MAPPING = {
            "versions": {
                "Core11": [
                    {"VkVulkan11Properties": "VK_STRUCT_TYPE_VULKAN_1_1_PROPERTIES"}
                ],
                "Core12": [
                    {"VkVulkan12Features": "VK_STRUCT_TYPE_VULKAN_1_2_FEATURES"}
                ]
            }
        }

        self.assertCodeEqual(
            f"""template <typename Visitor>
            inline bool Iterate(Visitor* visitor, VkJsonCore11* core) {{
              return visitor->Visit("properties", &core->properties);
            }}

            template <typename Visitor>
            inline bool Iterate(Visitor* visitor, VkJsonCore12* core) {{
              return visitor->Visit("features", &core->features);
            }}""",
            src.generate_core_template()
        )

    @patch('vkjson_gen_util.VK')
    def test_core_without_structs(self, mock_vk):
        mock_vk.VULKAN_CORES_AND_STRUCTS_MAPPING = {
            "versions": {
                "Core10": []
            }
        }

        self.assertCodeEqual(
            f"""template <typename Visitor>
            inline bool Iterate(Visitor* visitor, VkJsonCore10* core) {{
              return;
            }}""",
            src.generate_core_template()
        )


# Testing custom field types
test_uint8 = ctypes.c_uint8
test_uint32 = ctypes.c_uint32
VkTestBool = bool
VkTestFlag = VkTestBool


class VkTestEnum(Enum):
    VK_TEST_TYPE_1D = 0
    VK_TEST_TYPE_2D = 1
    VK_IMAGE_TYPE_3D = 2


class VkTestField:
    pass


class TestGenerateStructTemplate(helper.BaseCodeAssertTest):
    @dataclass
    class SimpleProperties:
        intField: int

    @dataclass
    class SimpleLimits:
        strField: str

    @dataclass
    class TwoFieldFeatures:
        boolField: bool
        floatField: float

    @dataclass
    class MultiFieldProperties:
        field1: test_uint32 * 2
        field2: test_uint8
        field3: VkTestBool
        field4: VkTestFlag
        field5: VkTestEnum
        field6: VkTestField

    @dataclass
    class TestSomethingElse:
        anyField: VkTestBool

    @dataclass
    class NoFieldLimits:
        pass

    @dataclass
    class ListFieldFeatures:
        listField: List[VkTestFlag]
        anyField: VkTestFlag

    # Testing struct aliases
    SimplePropertiesAlias1 = SimpleProperties
    SimplePropertiesAlias2 = SimplePropertiesAlias1

    def test_struct_with_single_field(self):
        mock_input = [self.SimpleProperties]

        self.assertCodeEqual(
            f"""template <typename Visitor>
            inline bool Iterate(Visitor* visitor, SimpleProperties* properties) {{
              return visitor->Visit("intField", &properties->intField);
            }}""",
            src.generate_struct_template(mock_input)
        )

    def test_struct_with_multiple_fields(self):
        mock_input = [self.MultiFieldProperties]

        self.assertCodeEqual(
            f"""template <typename Visitor>
            inline bool Iterate(Visitor* visitor, MultiFieldProperties* properties) {{
              return visitor->Visit("field1", &properties->field1) &&
                  visitor->Visit("field2", &properties->field2) &&
                  visitor->Visit("field3", &properties->field3) &&
                  visitor->Visit("field4", &properties->field4) &&
                  visitor->Visit("field5", &properties->field5) &&
                  visitor->Visit("field6", &properties->field6);
            }}""",
            src.generate_struct_template(mock_input)
        )

    @patch('vkjson_gen_util.VK')
    def test_struct_with_list_field(self, mock_vk):
        mock_vk.LIST_TYPE_FIELD_AND_SIZE_MAPPING = {
            "listField": "copyListField"
        }

        mock_input = [self.ListFieldFeatures]

        self.assertCodeEqual(
            f"""template <typename Visitor>
            inline bool Iterate(Visitor* visitor, ListFieldFeatures* features) {{
              return visitor->VisitArray("listField", features->copyListField, &features->listField) &&
                    visitor->Visit("anyField", &features->anyField);
            }}""",
            src.generate_struct_template(mock_input)
        )

    def test_struct_without_fields(self):
        mock_input = [self.NoFieldLimits]

        self.assertCodeEqual(
            f"""template <typename Visitor>
            inline bool Iterate(Visitor* visitor, NoFieldLimits* limits) {{
              return;
            }}""",
            src.generate_struct_template(mock_input)
        )

    def test_multiple_known_structs(self):
        mock_input = [
            self.SimpleLimits,
            self.TwoFieldFeatures,
            self.SimpleProperties
        ]

        self.assertCodeEqual(
            f"""template <typename Visitor>
            inline bool Iterate(Visitor* visitor, SimpleLimits* limits) {{
              return visitor->Visit("strField", &limits->strField);
            }}

            template <typename Visitor>
            inline bool Iterate(Visitor* visitor, TwoFieldFeatures* features) {{
              return visitor->Visit("boolField", &features->boolField) &&
                    visitor->Visit("floatField", &features->floatField);
            }}

            template <typename Visitor>
            inline bool Iterate(Visitor* visitor, SimpleProperties* properties) {{
              return visitor->Visit("intField", &properties->intField);
            }}""",
            src.generate_struct_template(mock_input)
        )

    # TODO (b/401184058): Fix source code
    def disabled_test_filters_out_unknown_structs(self):
        mock_input = [
            self.TestSomethingElse,
            self.SimpleLimits
        ]
        self.assertCodeEqual(
            f"""template <typename Visitor>
            inline bool Iterate(Visitor* visitor, SimpleLimits* limits) {{
              return visitor->Visit("strField", &limits->strField);
            }}""",
            src.generate_struct_template(mock_input)
        )

    def test_duplicate_structs(self):
        mock_input = [
            self.SimplePropertiesAlias1,
            self.SimplePropertiesAlias2,
            self.SimpleProperties
        ]

        self.assertCodeEqual(
            f"""template <typename Visitor>
            inline bool Iterate(Visitor* visitor, SimpleProperties* properties) {{
              return visitor->Visit("intField", &properties->intField);
            }}""",
            src.generate_struct_template(mock_input)
        )

    def test_no_structs(self):
        mock_input = []
        self.assertCodeEqual("", src.generate_struct_template(mock_input))


class TestEmitStructVisitsByVkVersion(helper.BaseMockCodeFileTest):

    def setUp(self):
        super().setUp()
        self.mock_vk_patcher = patch('vkjson_gen_util.VK')
        self.mock_get_struct_name_patcher = patch('vkjson_gen_util.get_struct_name')

        get_struct_name_return_map = {
            "VkPhysicalDeviceLineFeatures": "line_features",
            "VkPhysicalDeviceSubgroupProperties": "subgroup_properties",
            "VkPhysicalDeviceMultiviewProperties": "multiview_properties",
            "VkPhysicalDeviceIDProperties": "id_properties",
            "VkPhysicalDeviceMultiviewFeatures": "multiview_features"
        }
        self.mock_get_struct_name = self.mock_get_struct_name_patcher.start()
        self.mock_get_struct_name.side_effect = lambda struct_name: get_struct_name_return_map[struct_name]

        mock_vk = self.mock_vk_patcher.start()
        mock_vk.VULKAN_VERSIONS_AND_STRUCTS_MAPPING = {
            "VK_VERSION_1_0": [
                {"VkPhysicalDeviceLineFeatures": ""}
            ],
            "VK_VERSION_1_1": [
                {
                    "VkPhysicalDeviceSubgroupProperties": "VK_STRUCT_SUBGROUP"
                }
            ],
            "VK_VERSION_1_2": [
                {
                    "VkPhysicalDeviceMultiviewProperties": "VK_STRUCT_MULTIVIEW",
                    "VkPhysicalDeviceIDProperties": "VK_STRUCT_DEVICE_ID"
                },
                {
                    "VkPhysicalDeviceMultiviewFeatures": "VK_STRUCT_MULTIVIEW_FEATURES"
                }
            ]
        }

    def tearDown(self):
        self.mock_vk_patcher.stop()
        self.mock_get_struct_name_patcher.stop()

    def test_handles_single_matching_struct(self):
        expected_lines = (
            'visitor->Visit("subgroupProperties", &device->subgroup_properties) &&'
        )

        src.emit_struct_visits_by_vk_version(self.mock_file, "VK_VERSION_1_1")
        self.assertCodeFileWrite(expected_lines)

    def test_handles_multiple_matching_structs(self):
        expected_lines = (
            """visitor->Visit("multiviewProperties", &device->multiview_properties) &&
            visitor->Visit("idProperties", &device->id_properties) &&
            visitor->Visit("multiviewFeatures", &device->multiview_features) &&
            """
        )

        src.emit_struct_visits_by_vk_version(self.mock_file, "VK_VERSION_1_2")
        self.assertCodeFileWrite(expected_lines)

    def test_handles_empty_struct_type(self):
        expected_lines = 'visitor->Visit("lineFeatures", &device->line_features) &&'

        src.emit_struct_visits_by_vk_version(self.mock_file, "VK_VERSION_1_0")
        self.assertCodeFileWrite(expected_lines)

    def test_handles_struct_var_with_single_word(self):
        self.mock_get_struct_name.side_effect = None
        self.mock_get_struct_name.return_value = "memory"
        expected_lines = 'visitor->Visit("memory", &device->memory) &&'

        src.emit_struct_visits_by_vk_version(self.mock_file, "VK_VERSION_1_1")
        self.assertCodeFileWrite(expected_lines)

    def test_handles_struct_var_contains_number(self):
        self.mock_get_struct_name.side_effect = None
        self.mock_get_struct_name.return_value = "image_2d_view_of_3d_features_ext"

        # Considered acceptable
        expected_lines = 'visitor->Visit("image_2dViewOf_3dFeaturesExt", &device->image_2d_view_of_3d_features_ext) &&'

        src.emit_struct_visits_by_vk_version(self.mock_file, "VK_VERSION_1_1")
        self.assertCodeFileWrite(expected_lines)

    def test_handles_struct_var_starts_with_underscore(self):
        self.mock_get_struct_name.side_effect = None
        self.mock_get_struct_name.return_value = "_3d_features_ext"
        expected_lines = 'visitor->Visit("_3dFeaturesExt", &device->_3d_features_ext) &&'

        src.emit_struct_visits_by_vk_version(self.mock_file, "VK_VERSION_1_1")
        self.assertCodeFileWrite(expected_lines)


class TestGenerateVkCoreStructsInitCode(unittest.TestCase):

    def setUp(self):
        self.mock_vk_patcher = patch('vkjson_gen_util.VK')
        mock_vk = self.mock_vk_patcher.start()
        mock_vk.STRUCT_EXTENDS_MAPPING = {
            "VkPhysicalDeviceVulkan11Features": "VkPhysicalDeviceFeatures2,VkDeviceCreateInfo",
            "VkPhysicalDeviceVulkan11Properties": "VkPhysicalDeviceProperties2",
            "VkPhysicalDeviceVulkan12Features1": "VkPhysicalDeviceFeatures2,VkDeviceCreateInfo",
            "VkPhysicalDeviceVulkan12Properties1": "VkPhysicalDeviceProperties2",
            "VkPhysicalDeviceVulkan12Features2": "VkPhysicalDeviceFeatures2",
            "VkPhysicalDeviceVulkan12Properties2":"VkPhysicalDeviceProperties2",
        }
        mock_vk.VULKAN_CORES_AND_STRUCTS_MAPPING = {
            "versions": {
                "Core11": [
                    {"VkPhysicalDeviceVulkan11Properties": "VK_TYPE_11_PROPS"},
                    {"VkPhysicalDeviceVulkan11Features": "VK_TYPE_11_FEATS"}
                ],
                "Core12": [
                    {"VkPhysicalDeviceVulkan12Properties1": "VK_TYPE_12_PROPS_1"},
                    {"VkPhysicalDeviceVulkan12Features1": "VK_TYPE_12_FEATS_1"},
                    {"VkPhysicalDeviceVulkan12Properties2": "VK_TYPE_12_PROPS_2"},
                    {"VkPhysicalDeviceVulkan12Features2": "VK_TYPE_12_FEATS_2"}
                ]
            }
        }

    def tearDown(self):
        self.mock_vk_patcher.stop()

    def test_handles_single_matching_struct(self):
        actual_props, actual_feats = src.generate_vk_core_structs_init_code("Core11")

        self.assertEqual(
            (
                "device.core11.properties.sType = VK_TYPE_11_PROPS;\n"
                "device.core11.properties.pNext = properties.pNext;\n"
                "properties.pNext = &device.core11.properties;\n\n"
            ),
            actual_props
        )
        self.assertEqual(
            (
                "device.core11.features.sType = VK_TYPE_11_FEATS;\n"
                "device.core11.features.pNext = features.pNext;\n"
                "features.pNext = &device.core11.features;\n\n"
            ),
            actual_feats
        )

    def test_handles_multiple_matching_structs(self):
        actual_props, actual_feats = src.generate_vk_core_structs_init_code("Core12")

        self.assertEqual(
            (
                "device.core12.properties.sType = VK_TYPE_12_PROPS_1;\n"
                "device.core12.properties.pNext = properties.pNext;\n"
                "properties.pNext = &device.core12.properties;\n\n\n"
                "device.core12.properties.sType = VK_TYPE_12_PROPS_2;\n"
                "device.core12.properties.pNext = properties.pNext;\n"
                "properties.pNext = &device.core12.properties;\n\n"
            ),
            actual_props
        )
        self.assertEqual(
            (
                "device.core12.features.sType = VK_TYPE_12_FEATS_1;\n"
                "device.core12.features.pNext = features.pNext;\n"
                "features.pNext = &device.core12.features;\n\n\n"
                "device.core12.features.sType = VK_TYPE_12_FEATS_2;\n"
                "device.core12.features.pNext = features.pNext;\n"
                "features.pNext = &device.core12.features;\n\n"
            ),
            actual_feats
        )

    def test_handles_no_matching_struct(self):
        actual_props, actual_feats = src.generate_vk_core_structs_init_code("Core13")

        self.assertEqual("", actual_props)
        self.assertEqual("", actual_feats)


class TestGenerateVkExtensionStructsInitCode(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.mapping = {
            "VK_KHR_driver_state": [
                {"VkPhysicalDeviceDriverPropertiesKHR": "VK_STRUCT_DRIVER"}
            ],
            "VK_KHR_variable_pointers": [
                {"VkPhysicalDeviceVariablePointerFeaturesKHR": "VK_STRUCT_VARIABLE"},
                {"VkPhysicalDeviceVariablePointersFeaturesKHR": "VK_STRUCT_VARIABLES"},
                {"SomeThingElse": "SOME_OTHER_STRUCT"}
            ]
        }

    def test_handles_single_extension_single_struct(self):
        extension_var = src.get_vkjson_struct_variable_name("VK_KHR_driver_state")
        struct_name = src.get_struct_name("VkPhysicalDeviceDriverPropertiesKHR")

        self.assertEqual(
            (
                '  if (HasExtension("VK_KHR_driver_state", device.extensions)) {\n'
                f"    device.{extension_var}.reported = true;\n"
                f"    device.{extension_var}.{struct_name}.sType = VK_STRUCT_DRIVER;\n"
                f"    device.{extension_var}.{struct_name}.pNext = properties.pNext;\n"
                f"    properties.pNext = &device.{extension_var}.{struct_name};\n"
                '  }\n'
            ),
            src.generate_vk_extension_structs_init_code(self.mapping, "Properties")
        )

    def test_handles_single_extension_multiple_structs(self):
        extension_var = src.get_vkjson_struct_variable_name("VK_KHR_variable_pointers")
        struct_name1 = src.get_struct_name("VkPhysicalDeviceVariablePointerFeaturesKHR")
        struct_name2 = src.get_struct_name("VkPhysicalDeviceVariablePointersFeaturesKHR")

        self.assertEqual(
            (
                '  if (HasExtension("VK_KHR_variable_pointers", device.extensions)) {\n'
                f"    device.{extension_var}.reported = true;\n"
                f"    device.{extension_var}.{struct_name1}.sType = VK_STRUCT_VARIABLE;\n"
                f"    device.{extension_var}.{struct_name1}.pNext = features.pNext;\n"
                f"    features.pNext = &device.{extension_var}.{struct_name1};\n"
                f"    device.{extension_var}.{struct_name2}.sType = VK_STRUCT_VARIABLES;\n"
                f"    device.{extension_var}.{struct_name2}.pNext = features.pNext;\n"
                f"    features.pNext = &device.{extension_var}.{struct_name2};\n"
                '  }\n'
            ),
            src.generate_vk_extension_structs_init_code(self.mapping, "Features")
        )

    def test_handles_no_matching_struct(self):
        self.assertRaises(
            Exception,
            src.generate_vk_extension_structs_init_code(self.mapping, "Extension")
        )

    def test_handles_multiple_extensions(self):
        # Resetting mapping as we only want extensions containing feature structs with valid structextends mapping
        self.mapping = {
            "VK_KHR_vulkan_memory_model": [{"VkPhysicalDeviceVulkanMemoryModelFeaturesKHR": "VK_STRUCT_MEMORY"}],
            "VK_KHR_video_maintenance1": [{"VkPhysicalDeviceVideoMaintenance1FeaturesKHR": "VK_STRUCT_VIDEO"}],
        }
        extension_var1 = src.get_vkjson_struct_variable_name("VK_KHR_vulkan_memory_model")
        struct_name1 = src.get_struct_name("VkPhysicalDeviceVulkanMemoryModelFeaturesKHR")

        extension_var2 = src.get_vkjson_struct_variable_name("VK_KHR_video_maintenance1")
        struct_name2 = src.get_struct_name("VkPhysicalDeviceVideoMaintenance1FeaturesKHR")
        self.assertEqual(
            (
                '  if (HasExtension("VK_KHR_vulkan_memory_model", device.extensions)) {\n'
                f"    device.{extension_var1}.reported = true;\n"
                f"    device.{extension_var1}.{struct_name1}.sType = VK_STRUCT_MEMORY;\n"
                f"    device.{extension_var1}.{struct_name1}.pNext = features.pNext;\n"
                f"    features.pNext = &device.{extension_var1}.{struct_name1};\n"
                '  }\n\n'
                '  if (HasExtension("VK_KHR_video_maintenance1", device.extensions)) {\n'
                f"    device.{extension_var2}.reported = true;\n"
                f"    device.{extension_var2}.{struct_name2}.sType = VK_STRUCT_VIDEO;\n"
                f"    device.{extension_var2}.{struct_name2}.pNext = features.pNext;\n"
                f"    features.pNext = &device.{extension_var2}.{struct_name2};\n"
                '  }\n'
            ),
            src.generate_vk_extension_structs_init_code(self.mapping, "Features")
        )


class TestGenerateVkVersionStructsInitialization(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.version_data = [
            {
                "VkPhysicalDeviceIDProperties": "VK_STRUCTURE_TYPE_DEVICE_ID",
                "VkPhysicalDeviceMultiviewFeatures": "VK_STRUCTURE_TYPE_MULTIVIEW"
            },
            {
                "VkPhysicalDeviceMaintenance3Properties": "VK_STRUCTURE_TYPE_MAINTENANCE_3"
            }
        ]

        cls.prop1_struct_name = src.get_struct_name("VkPhysicalDeviceIDProperties")
        cls.prop2_struct_name = src.get_struct_name("VkPhysicalDeviceMaintenance3Properties")
        cls.feat_struct_name = src.get_struct_name("VkPhysicalDeviceMultiviewFeatures")

    def test_handles_single_matching_struct(self):
        self.assertEqual(
            (
                f"device.{self.feat_struct_name}.sType = VK_STRUCTURE_TYPE_MULTIVIEW;\n"
                f"device.{self.feat_struct_name}.pNext = features.pNext;\n"
                f"features.pNext = &device.{self.feat_struct_name};\n"
            ),
            src.generate_vk_version_structs_initialization(self.version_data, "Features")
        )

    def test_handles_multiple_matching_structs(self):
        self.assertEqual(
            (
                f"device.{self.prop1_struct_name}.sType = VK_STRUCTURE_TYPE_DEVICE_ID;\n"
                f"device.{self.prop1_struct_name}.pNext = properties.pNext;\n"
                f"properties.pNext = &device.{self.prop1_struct_name};\n\n"
                f"device.{self.prop2_struct_name}.sType = VK_STRUCTURE_TYPE_MAINTENANCE_3;\n"
                f"device.{self.prop2_struct_name}.pNext = properties.pNext;\n"
                f"properties.pNext = &device.{self.prop2_struct_name};\n"
            ),
            src.generate_vk_version_structs_initialization(self.version_data, "Properties")
        )

    def test_handles_no_matching_struct(self):
        self.assertEqual(
            "",
            src.generate_vk_version_structs_initialization(self.version_data, "Custom")
        )
