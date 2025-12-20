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

import xml.etree.ElementTree as ET
from collections import defaultdict
import re
from datetime import datetime
from typing import Dict, Optional, List, Any, Tuple, Set, IO
from pathlib import Path
import pprint
import copy
import generator_common as gencom

# --- FILE PATHS & OUTPUT CONFIGURATION ---
SCRIPT_DIR = Path(__file__).resolve().parent
VULKAN_XML_FILE_PATH = SCRIPT_DIR / "../../../../external/vulkan-headers/registry/vk.xml"
VULKAN_XML_FILE_PATH = VULKAN_XML_FILE_PATH.resolve()

OUTPUT_VK_PY_PATH = Path("vk.py")

# Output file initial content
INIT_CONTENT = """\n\nfrom dataclasses import dataclass
from enum import Enum
import dataclasses
dataclass = dataclasses.dataclass
from typing import List
import ctypes
"""

INIT_CONSTANTS_CONTENT = """
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
"""
# TODO: b/415706521 (Refactor EXTRA_STRUCTURES_CONTENT Logic to Use XML Generation)
EXTRA_STRUCTURES_CONTENT = """
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
"""

# TODO: b/415706507 (Find a way to identify this structs from vk.xml API_1_0 tag)
VULKAN_API_1_0_STRUCTS_CONTENT = """# --- STRUCTS USED BY VULKAN_API_1_0 ---
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
    VkPhysicalDeviceFeatures]"""

ADDITIONAL_EXTENSION_INDEPENDENT_STRUCTS_CONTENT = """# --- ADDITIONAL EXTENSION INDEPENDENT STRUCTS ---
ADDITIONAL_EXTENSION_INDEPENDENT_STRUCTS = ['VkPhysicalDeviceProperties', 'VkPhysicalDeviceFeatures', 'VkPhysicalDeviceMemoryProperties']"""

# --- GENERAL CONSTANTS ---
PRIMITIVE_DATA_TYPE = [
    "float_t",
    "uint32_t",
    "uint8_t",
    "int32_t",
    "uint64_t",
    "size_t",
    "float",
    "VkBool32",
    "int64_t",
    "str",
    "VkDeviceSize",
    "uint16_t",
]
FLOAT_ARRAY_REPLACEMENT_TYPE = "float_t"

# --- VULKAN SPECIFIC CONSTANTS & KEYWORDS ---
PHYSICAL_DEVICE_PREFIX = "VkPhysicalDevice"
STANDARD_MEMBER_NAMES_TO_SKIP = ("sType", "pNext")
VALID_ALIAS_FLAGS = ("VkFlags", "VkFlags64")

VK_STRUCTURE_TYPE = "VkStructureType"
VK_API_CONSTANTS = "API Constants"

VK_API_FILTER = "vulkan,vulkansc"
VK_VULKAN_FILTER = "vulkan"
VK_VULKAN_SC_FILTER = "vulkansc"

# --- XML SPECIFIC CONSTANTS ---
# XML Element/Attribute Names
TAG_TYPE = "type"
TAG_MEMBER = "member"
TAG_ENUM = "enum"
TAG_ENUMS = "enums"
TAG_EXTENSION = "extension"
TAG_FEATURE = "feature"
TAG_REQUIRE = "require"
TAG_NAME = "name"

ATTR_NAME = "name"
ATTR_SUPPORTED = "supported"
ATTR_PLATFORM = "platform"
ATTR_CATEGORY = "category"
ATTR_DEFINE = "define"
ATTR_ALIAS = "alias"
ATTR_STRUCT_EXTENDS = "structextends"
ATTR_VALUES = "values"
ATTR_VALUE = "value"
ATTR_TYPE = "type"
ATTR_LEN = "len"
ATTR_API = "api"

# XML Categories
CAT_STRUCT = "struct"
CAT_HANDLE = "handle"

# --- REGULAR EXPRESSIONS ---
# Pointer Regex Helpers
CONST_POINTER_REGEX = re.compile(r"^const\s+([\w\s]+?)\s*\*\s*$")  # Matches 'const <type> *'
VOID_POINTER_REGEX = re.compile(r"\bvoid\s*\*")  # Matches 'void*' as whole word
CHAR_POINTER_REGEX = re.compile(r"\bchar\s*\*")  # Matches 'char *'
CONST_CHAR_POINTER_REGEX = re.compile(r"\bconst\s+char\s*\*")  # Matches 'const char *'
# Other Regex
CORE_VERSION_REGEX = re.compile(r"^(VkPhysicalDevice)(Vulkan)(\d+)(Properties|Features)$")
ARRAY_TAIL_REGEX = re.compile(r"\[\s*([^\]]+)\s*\]")  # Matches array notation like [VK_UUID_SIZE]

# --- GLOBAL DATA STRUCTURES (Parsed Data Storage) ---

VK_PHYSICAL_STRUCT_NAMES = []  # Stores names starting with PHYSICAL_DEVICE_PREFIX
ALL_STRUCT_NAMES = []
ALL_ENUM_NAMES = []  # Stores all unique enum type names found
ALL_ALIAS_NAMES = []  # Stores all unique alias names found
ALL_CONSTANTS = [  # Base types considered 'constant' or primitive-like
    "uint8_t",
    "uint32_t",
    "VkFlags",
    "int32_t",
    "uint64_t",
    "VkBool32",
    "VkDeviceSize",
    "size_t",
    "float_t",
    "int64_t",
    "uint16_t",
    "VkFlags64",
    "float",
    "str",
    "int",
    "double",
]

NECESSARY_ENUMS = []
CORE_MAPPING_STRUCT_LIST = []  # Physical device structs that match the CORE_VERSION_REGEX pattern
VK_PHYSICAL_DEVICE_DEPENDENT_STRUCTURES_LIST = []  # Structs that physical device structs depend on
REQUIRED_STRUCT_NAMES = []  # All structs deemed necessary (physical device + dependencies)
REQUIRED_DATA_MEMBERS = []

# Mappings derived from parsing
alias_map: Dict[str, str] = {}  # Maps alias name to original name
feature_api_map: Dict[str, List[Dict[str, str]]] = defaultdict(list)  # Maps feature name to list of {struct: type_enum}
disabled_structs = []
structs_with_valid_extends = []
enum_member_map = {}
additional_vk_format_values = {}
# --- GENERAL UTILITY FUNCTIONS ---


def write_py_file(file_handle: IO[str], content: str):
    """Writes the given content to the provided open file handle."""
    file_handle.write(content)


def get_element_text(element: Optional[ET.Element]) -> Optional[str]:
    """Safely retrieves and strips the text content from an XML element."""
    if element is not None and element.text:
        return element.text.strip()
    return None


def find_next_tag(parent: ET.Element, element: ET.Element) -> Optional[ET.Element]:
    """Finds the immediate next sibling XML element within the same parent."""
    children = list(parent)
    try:
        index = children.index(element)
        if index + 1 < len(children):
            return children[index + 1]
    except ValueError:
        pass
    return None


# --- XML LOADING FUNCTION ---


def load_xml_registry(xml_file_path: Path) -> Optional[ET.Element]:
    """Loads and parses the Vulkan XML registry file into an ElementTree root object."""
    if not xml_file_path.exists():
        return None
    try:
        tree = ET.parse(xml_file_path)
        return tree.getroot()
    except ET.ParseError as e:
        return None


# --- API CONSTANT & VERSION NUMBER PROCESSING ---


def clean_api_constant_value(value: str, type_str: Optional[str]) -> str:
    """Cleans C-style suffixes and resolves specific Vulkan constants (like ~0) to their values."""
    original_value = value
    # Remove common C numeric suffixes
    cleaned_value = value.replace("F", "").replace("f", "").replace("U", "").replace("L", "")

    # Handle special Vulkan max value constants
    if cleaned_value == "(~0)":
        if type_str == "uint32_t":
            return "4294967295"  # 2^32 - 1
        elif type_str == "uint64_t":
            return "0xFFFFFFFFFFFFFFFF"  # 2^64 - 1
    elif cleaned_value == "(~1)":
        if type_str == "uint32_t":
            return "4294967294"  # 2^32 - 2
    elif cleaned_value == "(~2)":
        if type_str == "uint32_t":
            return "4294967293"  # 2^32 - 3
    elif cleaned_value.startswith("(~"):
        # Unhandled bitwise negation, return original to avoid incorrect interpretation
        return original_value
    # Attempt to validate/return as hex, float, or int
    try:
        if cleaned_value.startswith("0x") or cleaned_value.startswith("0X"):
            int(cleaned_value, 16)
            return cleaned_value
        else:
            try:
                float(cleaned_value)
                return cleaned_value
            except ValueError:
                int(cleaned_value)
                return cleaned_value
    except ValueError:
        return original_value


def write_constants(root: ET.Element, vk_py_file_handle: IO[str]):
    """Extracts and writes constants defined in the 'API Constants' section of the XML."""
    api_constants: Dict[str, str] = {}
    # Find the specific <enums> tag for API constants
    api_constants_element = root.find(f'.//enums[@{ATTR_NAME}="{VK_API_CONSTANTS}"]')

    if api_constants_element is None:
        return api_constants

    # Iterate through each <enum> defining a constant
    for enum_element in api_constants_element.findall(f"./{TAG_ENUM}"):
        name = enum_element.get(ATTR_NAME)
        value = enum_element.get(ATTR_VALUE)
        type_str = enum_element.get(ATTR_TYPE)  # Used by cleaner

        if name and value:
            # Clean the value (handle suffixes, special constants)
            cleaned_value = clean_api_constant_value(value, type_str)
            api_constants[name] = cleaned_value
        elif name and enum_element.get(ATTR_ALIAS):
            # Skip aliased constants here; they aren't assigned values directly
            pass

    content = "\n# --- API Constant values extracted from vk.xml ---\n"
    for name, value in api_constants.items():
        content += f"{name} = {value}\n"
    content += "\n"
    write_py_file(vk_py_file_handle, content)


def vk_make_api_version(variant: int, major: int, minor: int, patch: int) -> int:
    """Replicates the VK_MAKE_API_VERSION C macro logic to pack version numbers into an integer."""
    # Bitwise operations to pack variant, major, minor, patch into a 32-bit integer
    packed_version = (variant << 29) | (major << 22) | (minor << 12) | patch
    return packed_version


def extract_make_api_versions(root: ET.Element):
    """Extracts VK_API_VERSION_* defines and their corresponding (variant, major, minor, patch) tuples."""
    version_defines = {}  # {define_name: (variant, major, minor, patch)}
    # Find <type> tags categorized as 'define' that likely contain VK_MAKE_API_VERSION
    for type_tag in root.findall(f".//{TAG_TYPE}[@{ATTR_CATEGORY}='{ATTR_DEFINE}']"):
        name_elem = type_tag.find(ATTR_NAME)  # The #define name (e.g., VK_API_VERSION_1_0)
        macro_elem = type_tag.find(ATTR_TYPE)  # Should ideally contain 'VK_MAKE_API_VERSION'

        if name_elem is not None and macro_elem is not None:
            name = get_element_text(name_elem)
            macro_text = get_element_text(macro_elem)
            match_name = re.search(r"VK_API_VERSION(?:_\d+_\d+)?", name or "")
            if not match_name:
                continue

            full_text = "".join(type_tag.itertext())
            macro_pattern = re.escape(macro_text) if macro_text else r"VK_MAKE_API_VERSION"
            match_args = re.search(rf"{macro_pattern}\s*\(([^)]*)\)", full_text)

            if match_args:
                version_tuple = tuple(map(int, match_args.group(1).split(",")))
                if len(version_tuple) == 4:
                    version_defines[name] = version_tuple
    return version_defines


def write_api_constants(xml_root: ET.Element, vk_py_file_handle: IO[str]):
    """Extracts VK_API_VERSION defines, computes their integer values, and writes them to vk.py."""
    version_defines = extract_make_api_versions(xml_root)
    version_content = "\n# --- Computed VK_API_VERSION Constants ---\n"
    version_content += "VK_API_VERSION_MAP = {\n"
    version_defines_items = list(version_defines.items())
    if(len(version_defines_items)>1):
        for name, version in version_defines_items[1:]:
            packed_value = vk_make_api_version(version[0], version[1], version[2], version[3])
            version_content += f'    "{name}": {packed_value},\n'
    version_content += "}\n"
    write_py_file(vk_py_file_handle, version_content)


# --- ALIAS EXTRACTION & WRITING FUNCTIONS ---


def fetch_flags_aliases(root: ET.Element) -> dict[str, str]:
    """Extracts C typedefs specifically for VkFlags and VkFlags64 aliases."""
    vk_flags_aliases: dict[str, str] = {}
    # Find all <type> elements potentially containing typedefs
    for type_tag in root.findall(f".//{TAG_TYPE}"):
        # Check the element's text content for 'typedef' keyword
        if type_tag.text and "typedef" in type_tag.text:
            original_type_elem = type_tag.find(f"{TAG_TYPE}")  # The original type being aliased
            alias_elem = type_tag.find(f"{TAG_NAME}")  # The new alias name
            if original_type_elem is not None and alias_elem is not None:
                original_type = get_element_text(original_type_elem)
                alias = get_element_text(alias_elem)
                # Only capture aliases for types listed in VALID_ALIAS_FLAGS
                if original_type in VALID_ALIAS_FLAGS:
                    if alias not in ALL_ALIAS_NAMES:
                        ALL_ALIAS_NAMES.append(alias)
                    vk_flags_aliases[alias] = original_type  # Store {alias_name: original_type}

    return vk_flags_aliases


def write_aliases(
    vk_py_file_handle: IO[str],
    aliases: dict[str, str],
    comment: str,
    filter_required_data_members: bool = False,
) -> None:
    """Writes extracted type aliases (VkFlags or struct aliases) as Python assignments to a file."""
    content = f"\n{comment}\n"
    if filter_required_data_members:
        content += "\n".join(f"{alias} = {original}" for alias, original in aliases.items() if alias in REQUIRED_DATA_MEMBERS)
    else:
        content += "\n".join(f"{alias} = {original}" for alias, original in aliases.items() if is_valid_struct_name(original))
    content += "\n\n"
    write_py_file(vk_py_file_handle, content)


# --- ENUM EXTRACTION & WRITING FUNCTIONS ---


def fetch_enums_with_type_attribute(root: ET.Element):
    """Parses all <enums> blocks (except API constants) to extract enum members and values."""
    parsed_enums: Dict[str, Dict[str, Optional[str]]] = defaultdict(dict)
    processed_enums = set()  # Keep track of processed enum types to avoid duplicates

    for enums_tag in root.findall(f"{TAG_ENUMS}"):
        enum_type_name = enums_tag.get(ATTR_NAME)  # Name of the enum type (e.g., VkResult)
        # Skip if no name or if it's the special API constants block
        if not enum_type_name or enum_type_name == VK_API_CONSTANTS:
            continue

        if enum_type_name in processed_enums:
            continue

        # Add to global list if it's a newly encountered enum type
        if enum_type_name not in ALL_ENUM_NAMES:
            ALL_ENUM_NAMES.append(enum_type_name)

        enum_map = {}  # Stores {member_name: value/bitpos} for the current enum type
        # Iterate through individual <enum> members within the <enums> block
        for enum_tag in enums_tag.findall(f"{TAG_ENUM}"):
            name = enum_tag.get(ATTR_NAME)  # Enum member name (e.g., VK_SUCCESS)
            value = enum_tag.get(ATTR_VALUE)  # Explicit value
            bitPos = enum_tag.get("bitpos")  # Bit position (for bitmasks)

            if name is not None and (value is not None or bitPos is not None):
                # Prefer bitpos if value is missing (common for bitmasks)
                if not value and bitPos:
                    enum_map[name] = bitPos
                else:
                    enum_map[name] = value
        parsed_enums[enum_type_name] = enum_map
        processed_enums.add(enum_type_name)

    return parsed_enums


def write_enums(vk_py_file_handle: IO[str], enums: dict[str, dict[str, Optional[str]]]) -> None:
    """Writes necessary enum definitions as Python Enum classes and generates C++ traits string."""
    content = "\n# --- Enum Definitions ---\n"
    # Iterate through all parsed enum types
    for enum_name, members in enums.items():
        # Only generate code for enums that were deemed necessary (used by structs)
        if enum_name not in NECESSARY_ENUMS:
            continue

        enum_member_map[enum_name] = members
        # --- Python Enum class generation ---
        content += f"class {enum_name}(Enum):\n"
        if members:
            for member, value in members.items():
                # Ensure both member name and value/bitpos exist
                if value is not None and member:
                    content += f"    {member} = {value}\n"
            if enum_name == "VkFormat":
                enum_member_map[enum_name].update(additional_vk_format_values)
                for member, value in additional_vk_format_values.items():
                    # Ensure both member name and value/bitpos exist
                    if value is not None and member:
                        content += f"    {member} = {int(value)}\n"
        else:
            content += "    pass\n"
        content += "\n"
    write_py_file(vk_py_file_handle, content)


# --- HANDLE EXTRACTION & WRITING FUNCTIONS ---


def fetch_struct_handles(root: ET.Element):
    """Extracts a list of names for types categorized as 'handle' in the XML."""
    created_class_names = []
    # Find all <type> tags with category='handle'
    for type_tag in root.findall(f".//{TAG_TYPE}[@{ATTR_CATEGORY}='{CAT_HANDLE}']"):
        name_tag = type_tag.find("name")  # Find the <name> child element
        if name_tag is not None and name_tag.text:
            # Basic sanitization: remove non-alphanumeric characters (except underscore)
            class_name = re.sub(r"[^a-zA-Z0-9_]", "", name_tag.text.strip())
            if class_name and class_name not in created_class_names:
                created_class_names.append(class_name)
                # Add handle names to the global list of all struct-like types
                if class_name not in ALL_STRUCT_NAMES:
                    ALL_STRUCT_NAMES.append(class_name)
    return created_class_names


def write_empty_dataclasses(dataclasses_list: List[str], vk_py_file_handle: IO[str]):
    """Generates empty Python dataclasses for Vulkan handle types and adds predefined common structs."""
    created_class_names = set()  # Track generated classes to avoid duplicates
    content_to_append = "\n# --- Empty Handle Dataclasses ---\n"

    # Generate empty dataclasses only for handles that are dependencies of physical device structs
    for class_name in dataclasses_list:
        if class_name not in VK_PHYSICAL_DEVICE_DEPENDENT_STRUCTURES_LIST:
            continue
        if class_name not in created_class_names:
            content_to_append += f"@dataclass\n"
            content_to_append += f"class {class_name}:\n"
            content_to_append += f"    pass\n\n"
            created_class_names.add(class_name)

    # Add manually defined common Vulkan struct definitions
    content_to_append += "# --- Pre-defined Struct Definitions ---\n"
    content_to_append += EXTRA_STRUCTURES_CONTENT
    content_to_append += "\n"
    write_py_file(vk_py_file_handle, content_to_append)


# --- STRUCT DEFINITION PROCESSING (Parsing & Writing vk_definitions.py content) ---


# --- Struct Validation Helpers ---
def is_valid_struct_name(struct_name):
    """Checks if a struct name starts with the physical device prefix and is not invalid."""
    return struct_name.startswith(PHYSICAL_DEVICE_PREFIX)


def check_valid_struct_extends(struct_extends):
    """Validates if a struct extends one of the base physical device property/feature structs."""
    return "VkPhysicalDeviceProperties2" in struct_extends or "VkPhysicalDeviceFeatures2" in struct_extends


def check_if_valid_struct(struct_name, alias_name):
    """Determines if a struct or its alias is considered valid for processing (not None or disabled)."""
    if not struct_name:
        return False
    # Check both original and alias name against the disabled list
    if struct_name in disabled_structs or alias_name in disabled_structs:
        return False
    return True


# --- Type Formatting for Struct Members ---
def format_data_types(c_type: str, array_size_str: Optional[str], struct_name_context: str) -> str:
    """Converts C type strings (including pointers, arrays) into Python type hints."""
    py_type_hint = c_type  # Default to original C type if no specific rule matches
    is_list = False  # Flag indicating if the type should be List[...]
    shouldSkipCheck = False  # Flag to bypass the unknown type check

    # 1. Handle specific pointer types first
    if CONST_CHAR_POINTER_REGEX.search(c_type) or CHAR_POINTER_REGEX.search(c_type):
        py_type_hint = "str"  # 'const char*' or 'char*' maps to Python 'str'
    elif VOID_POINTER_REGEX.search(c_type):
        py_type_hint = "object"  # 'void*' maps to Python 'object' (or could use 'Any')

    # 2. Handle general 'const <type> *' (pointer to const) -> 'List[<type>]'
    elif const_ptr_match := CONST_POINTER_REGEX.match(c_type):
        base_type = const_ptr_match.group(1).strip()  # Extract the base type
        py_type_hint = base_type
        is_list = True  # Pointers generally map to lists

    # 3. Handle general non-const '<type> *' -> 'List[<type>]'
    elif c_type.strip().endswith("*") and not py_type_hint == "object":
        base_type = c_type.strip().replace("*", "").strip()
        py_type_hint = base_type
        is_list = True  # Pointers map to lists

    # 4. Handle C-style arrays '[SIZE]' if 'array_size_str' is provided
    elif array_size_str:
        base_type = c_type  # Type before the brackets
        if base_type == "float":
            # Represent float arrays using a specific type hint format if needed
            py_type_hint = f"{FLOAT_ARRAY_REPLACEMENT_TYPE} * {array_size_str}"
            shouldSkipCheck = True  # Skip check for this specific format
        elif base_type == "char":
            # Fixed-size char arrays in C often represent strings
            py_type_hint = "str"
        elif base_type in PRIMITIVE_DATA_TYPE:
            # Represent primitive arrays using a specific format if needed (e.g., for ctypes)
            py_type_hint = f"{base_type} * {array_size_str}"
            shouldSkipCheck = True
        else:
            # Assume arrays of non-primitives (structs, enums) map to List
            py_type_hint = base_type
            is_list = True

    # 5. If none of the above, it's a simple type name (e.g., uint32_t, VkResult)
    else:
        py_type_hint = c_type  # Use the C type name directly

    # --- Clean up and Final Check ---
    py_type_hint = py_type_hint.replace("const ", "").strip()  # Remove 'const ' prefix
    core_type_for_check = py_type_hint  # The base type after initial conversion

    # Verify the resulting base Python type hint is known (primitive, struct, enum, alias)
    if (
        not shouldSkipCheck
        and core_type_for_check not in ALL_STRUCT_NAMES
        and core_type_for_check not in ALL_ENUM_NAMES
        and core_type_for_check not in ALL_ALIAS_NAMES
        and core_type_for_check not in ALL_CONSTANTS
    ):
        raise Exception(f"Warning: Unknown base type '{core_type_for_check}' derived from C type '{c_type}' " f"in struct '{struct_name_context}'")

    if is_list:
        py_type_hint = f"List[{py_type_hint}]"

    return py_type_hint


# --- Core Struct Parsing ---


def fetch_all_structs_and_aliases(
    struct_elements: List[ET.Element],
) -> Tuple[Dict[str, List[Tuple[str, str, Optional[str]]]], Dict[str, str]]:
    """
    Parses VkPhysicalDevice struct definitions and aliases from XML.
    Extracts member details (name, C type, array size) for each struct,
    handles aliases, and populates global lists for dependency tracking
    (e.g., `VK_PHYSICAL_STRUCT_NAMES`, `REQUIRED_STRUCT_NAMES`, `NECESSARY_ENUMS`).
    """
    parsed_structs: Dict[str, List[Tuple[str, str, Optional[str]]]] = {}  # {struct_name: [(member_name, c_type, array_size)]}
    struct_alias_data = {}  # Stores {alias_name: original_struct_name}

    for struct_elem in struct_elements:
        struct_name = struct_elem.get(ATTR_NAME)
        alias_name = struct_elem.get(ATTR_ALIAS)
        is_struct_of_interest = is_valid_struct_name(struct_name)

        # Skip if the struct (or its alias) is invalid or disabled
        if not check_if_valid_struct(struct_name, alias_name):
            continue

        # Add if structs has valid structextends ('VkPhysicalDeviceProperties2' or 'VkPhysicalDeviceFeatures2')
        if struct_name in structs_with_valid_extends:
            if struct_name not in VK_PHYSICAL_STRUCT_NAMES:
                VK_PHYSICAL_STRUCT_NAMES.append(struct_name)

        # --- Handle Aliases ---
        if alias_name:
            struct_alias_data[struct_name] = alias_name
            continue  # Don't process members for the alias tag itself

        # --- Process Actual Struct Definitions ---

        if is_struct_of_interest:
            if struct_name not in REQUIRED_STRUCT_NAMES:
                REQUIRED_STRUCT_NAMES.append(struct_name)

        struct_members: List[Tuple[str, str, Optional[str]]] = []
        # Iterate through <member> tags within the struct definition
        for member_elem in struct_elem.findall(TAG_MEMBER):
            member_type_tag = member_elem.find(TAG_TYPE)
            member_name_tag = member_elem.find(TAG_NAME)

            member_type = get_element_text(member_type_tag)  # C type (e.g., 'uint32_t', 'const char*')
            member_name = get_element_text(member_name_tag)  # Member variable name
            member_array_size: Optional[str] = None  # Extracted array size (like 'VK_UUID_SIZE')
            member_type_c = member_type  # Keep the original C type string

            # Skip standard Vulkan struct members (sType, pNext)
            if member_name in STANDARD_MEMBER_NAMES_TO_SKIP:
                continue

            # Determine base type for dependency checking (strip const/pointer)
            base_type_for_necessity_check = member_type_c.replace("const", "").replace("*", "").strip()

            # --- Determine Array Size (from 'len' attribute or '[size]' notation) ---
            len_attr = member_elem.get(ATTR_LEN)
            if len_attr and len_attr.strip().lower() != "null-terminated":
                member_array_size = len_attr.strip()
            elif member_name_tag is not None:  # Check for '[size]' notation after <name> tag
                tail_text = member_name_tag.tail.strip() if member_name_tag.tail else ""
                next_tag = find_next_tag(member_elem, member_name_tag)
                # Case 2a: Size defined by an adjacent <enum> tag: <name>type_name</name>[<enum>SIZE</enum>]
                if tail_text.startswith("[") and next_tag is not None and next_tag.tag == TAG_ENUM:
                    enum_text = get_element_text(next_tag)
                    enum_tail = next_tag.tail.strip() if next_tag.tail else ""
                    if enum_text and enum_tail.endswith("]"):
                        member_array_size = enum_text
                # Case 2b: Size defined in plain text in tail: <name>type_name</name>[32]
                elif not member_array_size:
                    match = ARRAY_TAIL_REGEX.search(tail_text)
                    if match:
                        member_array_size = match.group(1).strip()

            # --- Dependency Tracking ---
            # If parsing a physical device struct and a member is a non-primitive type,
            # mark that type as a required dependency.
            if is_struct_of_interest and base_type_for_necessity_check not in PRIMITIVE_DATA_TYPE:
                if (
                    base_type_for_necessity_check and base_type_for_necessity_check != "void" and base_type_for_necessity_check not in REQUIRED_STRUCT_NAMES
                ):  # Check if the type is valid, not void, and not already required
                    REQUIRED_STRUCT_NAMES.append(base_type_for_necessity_check)
                    # Also track as a direct dependency of physical device structs
                    if base_type_for_necessity_check not in VK_PHYSICAL_DEVICE_DEPENDENT_STRUCTURES_LIST:
                        VK_PHYSICAL_DEVICE_DEPENDENT_STRUCTURES_LIST.append(base_type_for_necessity_check)

            if member_type and member_name:
                if struct_name in REQUIRED_STRUCT_NAMES:
                    # Adding all required ENUMs
                    if base_type_for_necessity_check in ALL_ENUM_NAMES and base_type_for_necessity_check not in NECESSARY_ENUMS:
                        NECESSARY_ENUMS.append(base_type_for_necessity_check)
                if base_type_for_necessity_check not in REQUIRED_DATA_MEMBERS:
                    REQUIRED_DATA_MEMBERS.append(base_type_for_necessity_check)
                struct_members.append((member_name, member_type, member_array_size))

        if struct_name:
            parsed_structs[struct_name] = struct_members

    return parsed_structs, struct_alias_data


# --- Writing Struct Dataclasses ---
def write_structs(vk_py_file_handle: IO[str], structs: Dict[str, List[Tuple[str, str, Optional[str]]]]) -> None:
    """Writes Python dataclass definitions for parsed structs, ensuring dependencies are written first."""
    content = "\n# --- Vulkan Struct Definitions (Dependencies first, then PhysicalDevice structs) ---\n"
    local_content = ""

    # Helper function to generate the @dataclass string for a single struct
    def process_struct(class_name: str, member_list: List[Tuple[str, str, Optional[str]]]):
        nonlocal local_content
        local_content += f"@dataclass\nclass {class_name}:\n"
        if member_list:
            for member_name, member_type_c, size_val in member_list:
                # Convert the C type to a Python type hint
                python_type_hint = format_data_types(member_type_c, size_val, class_name)
                local_content += f"    {member_name}: {python_type_hint}\n"  # Add member line
        else:
            # Handle structs with no members (e.g., placeholder structs)
            local_content += "    pass\n"
        local_content += "\n"  # Add blank line after class definition

    # --- Write Structs in Dependency Order ---
    # First pass: Write structs that are dependencies of physical device structs
    for class_name, member_list in structs.items():
        if class_name in VK_PHYSICAL_DEVICE_DEPENDENT_STRUCTURES_LIST:
            process_struct(class_name, member_list)

    # Second pass: Write the required physical device structs themselves (if not already written as dependencies)
    for class_name, member_list in structs.items():
        if class_name in REQUIRED_STRUCT_NAMES and class_name not in VK_PHYSICAL_DEVICE_DEPENDENT_STRUCTURES_LIST:
            process_struct(class_name, member_list)

    # Combine the initial comment with the generated struct definitions
    content += local_content
    write_py_file(vk_py_file_handle, content)


# --- STRUCT & EXTENSION MAPPING LOGIC (Processing for vk.py content) ---


# --- Initial Filtering based on XML attributes/API types ---
def create_disabled_struct_list(struct_elements: List[ET.Element]) -> None:
    """Identifies structs to disable based on invalid suffixes or if they don't extend expected base structs."""
    for type_element in struct_elements:
        struct_name = type_element.get(ATTR_NAME)
        structextends = type_element.get(ATTR_STRUCT_EXTENDS)  # Check the 'structextends' attribute

        # Condition for disabling a struct:
        # 1. It has a 'structextends' attribute, but it doesn't reference the valid base structs ('VkPhysicalDeviceProperties2' or 'VkPhysicalDeviceFeatures2')
        is_invalid_extends = structextends and not check_valid_struct_extends(struct_extends=structextends)

        if is_invalid_extends:
            if struct_name and struct_name not in disabled_structs:
                disabled_structs.append(struct_name)


def extract_vulkan_sc_features(root: ET.Element):
    """Identifies physical device structs required only by Vulkan SC API and adds them to the disabled list."""
    # Find <feature> tags specific to the 'vulkansc' API
    feature_tags = root.findall(f'.//{TAG_FEATURE}[@{ATTR_API}="{VK_VULKAN_SC_FILTER}"]')
    for feature in feature_tags:
        feature_name = feature.get(ATTR_NAME)
        if not feature_name:
            continue
        # Find types required by this Vulkan SC feature
        for require_tag in feature.findall(f"./{TAG_REQUIRE}"):
            for type_tag in require_tag.findall(f"./{TAG_TYPE}"):
                type_name = type_tag.get(ATTR_NAME)
                # If it's a physical device struct, add it to the disabled list
                if type_name and type_name.startswith(PHYSICAL_DEVICE_PREFIX):
                    if type_name not in disabled_structs:
                        disabled_structs.append(type_name)


# --- Core Mappings for Structs ---
def extract_struct_to_type_mapping(struct_elements: List[ET.Element]) -> Dict[str, str]:
    """
    Maps Vulkan struct names (including aliases) to their VkStructureType enum strings.
    Identifies core version structs for `CORE_MAPPING_STRUCT_LIST` and structs
    with valid 'structextends' attributes for `structs_with_valid_extends`.
    Resolves aliases to find their sType values.
    """
    struct_to_type_map: Dict[str, str] = {}  # {struct_name: sType_enum_value}

    # --- First Pass: Extract Direct sType Values ---
    for type_elem in struct_elements:
        struct_name = type_elem.get(ATTR_NAME)
        alias_name = type_elem.get(ATTR_ALIAS)
        struct_extends = type_elem.get(ATTR_STRUCT_EXTENDS)

        if struct_name not in ALL_STRUCT_NAMES:
            ALL_STRUCT_NAMES.append(struct_name)

        # Skip invalid, disabled, or non-physical device structs
        if not check_if_valid_struct(struct_name, alias_name) or not is_valid_struct_name(struct_name):
            continue

        # Handle Aliases: Store alias mapping and skip member processing for this tag
        if alias_name:
            alias_map[struct_name] = alias_name  # Store {alias: original}
            if alias_name in structs_with_valid_extends:
                if struct_name not in structs_with_valid_extends:
                    structs_with_valid_extends.append(struct_name)
            continue

        # Track structs that don't extend the expected base types
        if struct_extends and check_valid_struct_extends(struct_extends=struct_extends):
            if struct_name not in structs_with_valid_extends:
                structs_with_valid_extends.append(struct_name)

        # Track core physical device structs (e.g., VkPhysicalDeviceVulkan11Properties) separately
        if struct_name not in CORE_MAPPING_STRUCT_LIST:
            match = CORE_VERSION_REGEX.match(struct_name)
            if match:
                CORE_MAPPING_STRUCT_LIST.append(struct_name)

        # Find the 'sType' member to get the structure type enum value
        for member_elem in type_elem.findall(TAG_MEMBER):
            name_elem = member_elem.find(TAG_NAME)
            if get_element_text(name_elem) == "sType":
                values = member_elem.get(ATTR_VALUES)
                if values:
                    struct_to_type_map[struct_name] = values  # Store {struct_name: sType_value}
                    break

    # --- Second Pass: Resolve Aliases ---
    # Helper function to recursively find the sType value by following alias chains
    def resolve_alias_chain(alias: str, visited=None) -> Optional[str]:
        if visited is None:
            visited = set()
        if alias in visited:
            return None

        visited.add(alias)
        target = alias_map.get(alias)  # Get the name the alias points to
        if not target:
            return None

        if target in struct_to_type_map:
            return struct_to_type_map[target]
        # Otherwise, if the target is itself an alias, resolve it recursively
        elif target in alias_map:
            return resolve_alias_chain(target, visited)
        else:
            return None

    # Resolve sType values for all found struct aliases and add them to the map
    resolved_aliases = {}
    for alias, original in alias_map.items():
        # Only resolve if the original struct is a relevant physical device struct
        if is_valid_struct_name(original):
            resolved_value = resolve_alias_chain(alias)
            if resolved_value:
                resolved_aliases[alias] = resolved_value

    struct_to_type_map.update(resolved_aliases)
    return struct_to_type_map


def extract_struct_extends_mapping(struct_elements: List[ET.Element]) -> Dict[str, str]:
    alias_struct_name_to_target_name_map: Dict[str, str] = {}  # {alias_name: name_it_points_to}
    # Stores structextends for non-alias structs that define it
    defined_struct_name_to_struct_extends_map: Dict[str, str] = {}
    all_declared_struct_names: Set[str] = set()  # Stores all names from type_elem.get(ATTR_NAME)

    # --- First Pass: Collect direct structextends and alias definitions ---
    for type_elem in struct_elements:
        struct_name = type_elem.get(ATTR_NAME)

        if not check_if_valid_struct(struct_name, None):
            continue

        all_declared_struct_names.add(struct_name)
        alias_target = type_elem.get(ATTR_ALIAS)
        struct_extends_value = type_elem.get(ATTR_STRUCT_EXTENDS)

        if struct_extends_value and not check_valid_struct_extends(struct_extends_value):
            continue

        if alias_target:
            # This struct_name is an alias for alias_target
            alias_struct_name_to_target_name_map[struct_name] = alias_target
        elif struct_extends_value:
            # This struct is not an alias and has a structextends attribute directly
            defined_struct_name_to_struct_extends_map[struct_name] = struct_extends_value

    # --- Second Pass: Resolve structextends for all struct names (including aliases) ---
    final_struct_name_to_resolved_struct_extends_map: Dict[str, str] = {}

    def get_resolved_struct_extends(name_to_resolve: str, visited_in_chain: Set[str]) -> Optional[str]:
        """
        Recursively resolves the structextends value for a given name.
        - If name_to_resolve has a direct structextends, returns it.
        - If name_to_resolve is an alias, follows the alias chain.
        - Returns None if no structextends is found or a cycle is detected.
        """
        if name_to_resolve in visited_in_chain:
            # Cycle detected in alias chain
            return None
        visited_in_chain.add(name_to_resolve)

        # Case 1: The name_to_resolve is a non-alias struct that directly defines structextends
        if name_to_resolve in defined_struct_name_to_struct_extends_map:
            return defined_struct_name_to_struct_extends_map[name_to_resolve]

        # Case 2: The name_to_resolve is an alias. Follow the chain.
        if name_to_resolve in alias_struct_name_to_target_name_map:
            target_name = alias_struct_name_to_target_name_map[name_to_resolve]
            return get_resolved_struct_extends(target_name, visited_in_chain)

        # Case 3: The name is not an alias we know of, and not in the direct map.
        # This means it's an original struct without 'structextends', or an unresolvable name.
        return None

    # Iterate through all unique struct names found in the XML
    for name in all_declared_struct_names:
        resolved_extends_value = get_resolved_struct_extends(name, set())

        if resolved_extends_value:
            final_struct_name_to_resolved_struct_extends_map[name] = resolved_extends_value

    return final_struct_name_to_resolved_struct_extends_map


def write_structs_extends_mapping(struct_elements: List[ET.Element], vk_py_file_handle: IO[str]):
    struct_extends_mapping_dict = extract_struct_extends_mapping(struct_elements)
    struct_extends_mapping_str = pprint.pformat(struct_extends_mapping_dict, indent=4, width=100)
    content = f"\n# --- STRUCT EXTENDS MAPPINGS ---\nSTRUCT_EXTENDS_MAPPING = {struct_extends_mapping_str}\n\n"
    write_py_file(vk_py_file_handle, content)


# --- Feature/Extension/Core Version Based Mappings ---
def extract_feature_struct_mapping(root: ET.Element, struct_to_type_map: Dict[str, str], api_filter: str):
    """
    Maps Vulkan API features (e.g., VK_API_VERSION_1_1) to PhysicalDevice structs they introduce.
    Populates the global `feature_api_map`. Filters structs based on:
    - Validity (PhysicalDevice, not disabled).
    - Having valid 'structextends' (in `structs_with_valid_extends`).
    - Not being a core version struct (not in `CORE_MAPPING_STRUCT_LIST`).
    """
    # Find <feature> tags matching the specified API filter (e.g., 'vulkan')
    feature_tags = root.findall(f'.//{TAG_FEATURE}[@{ATTR_API}="{api_filter}"]')

    for feature in feature_tags:
        feature_name = feature.get(ATTR_NAME)  # e.g., "VK_API_VERSION_1_1"
        if not feature_name:
            continue

        if feature_name not in feature_api_map:
            feature_api_map[feature_name] = []

        # Find <require> blocks within the feature
        for require_tag in feature.findall(f"./{TAG_REQUIRE}"):
            # Find <type> tags specifying required types (structs, enums, etc.)
            for type_tag in require_tag.findall(f"./{TAG_TYPE}"):
                type_name = type_tag.get(ATTR_NAME)
                is_struct_of_interest = is_valid_struct_name(type_name)

                # --- Skip if not relevant ---
                if type_name in disabled_structs:
                    continue

                if type_name not in structs_with_valid_extends:
                    continue

                if type_name in CORE_MAPPING_STRUCT_LIST:
                    continue  # Core structs handled separately
                # --- Process relevant structs ---
                if type_name and is_struct_of_interest:
                    structure_type = struct_to_type_map.get(type_name)
                    if not structure_type:  # Skip if sType couldn't be found (shouldn't happen for valid structs)
                        continue
                    # Create the mapping {struct_name: sType_value}
                    struct_map = {type_name: structure_type}
                    if struct_map not in feature_api_map[feature_name]:
                        feature_api_map[feature_name].append(struct_map)


def extract_extension_struct_mapping(root: ET.Element, struct_to_type_map: Dict[str, str]) -> Tuple[Dict[str, List[Dict[str, str]]], Set[str]]:
    """
    Maps enabled Vulkan extensions to their required PhysicalDevice structs.
    Filters extensions (e.g., not 'disabled', 'android' platform). Structs from
    skipped extensions are added to `disabled_structs`.
    For included extensions, structs must be:
    - Valid PhysicalDevice structs (not disabled).
    - Have valid 'structextends' (in `structs_with_valid_extends`).
    """
    extension_map: Dict[str, List[Dict[str, str]]] = defaultdict(list)  # {ext_name: [{struct: sType}]}
    extensions_found = root.findall(f".//{TAG_EXTENSION}")  # Find all <extension> tags
    processed_structs: Set[str] = set()

    for extension in extensions_found:
        ext_name = extension.get(ATTR_NAME)  # Extension name (e.g., "VK_KHR_surface")
        supported = extension.get(ATTR_SUPPORTED)  # Usually "vulkan", "disabled", etc.
        platform = extension.get(ATTR_PLATFORM)  # e.g., "android", "win32"
        if not ext_name:
            continue

        # --- Filter out disabled, non-standard, or unsupported platform extensions ---
        is_disabled_or_unsupported = supported == "disabled" or (platform and platform != "android")

        if is_disabled_or_unsupported:
            # If extension is skipped, add its required types to the global disabled list
            for require_tag in extension.findall(f"./{TAG_REQUIRE}"):
                type_names_list = [type_tag.get(ATTR_NAME) for type_tag in require_tag.findall(f"./{TAG_TYPE}") if type_tag.get(ATTR_NAME)]
                for type_name in type_names_list:
                    if type_name not in disabled_structs:
                        disabled_structs.append(type_name)
            continue  # Skip processing this extension further

        # --- Process enabled/supported extensions ---
        current_ext_structs: List[Dict[str, str]] = []  # Structs required by *this* specific extension
        for require_tag in extension.findall(f"./{TAG_REQUIRE}"):
            # Get names of all types required by this section of the extension
            type_names_list = [type_tag.get(ATTR_NAME) for type_tag in require_tag.findall(f"./{TAG_TYPE}") if type_tag.get(ATTR_NAME)]
            for type_name in type_names_list:

                # Skip disabled structs
                if type_name in disabled_structs:
                    continue

                # Process only relevant physical device structs not already added to the map
                if type_name in ALL_STRUCT_NAMES:
                    if "Properties" not in type_name and "Features" not in type_name:
                        if type_name not in disabled_structs:
                            disabled_structs.append(type_name)
                        continue
                    if type_name not in structs_with_valid_extends:
                        continue
                    # Look up the sType value from the precomputed map
                    structure_type = struct_to_type_map.get(type_name)
                    if structure_type:
                        struct_map = {type_name: structure_type}
                        # Add to this extension's list if not already present
                        if struct_map not in current_ext_structs:
                            current_ext_structs.append(struct_map)
                            processed_structs.add(type_name)  # Mark struct as processed globally

        if current_ext_structs:
            extension_map[ext_name] = current_ext_structs

    return dict(extension_map), processed_structs


def generate_core_struct_mapping(struct_names: list[str], vk_py_file_handle: IO[str]) -> dict:
    """
    Generates and writes the VULKAN_CORES_AND_STRUCTS_MAPPING dictionary.
    This map links core Vulkan versions (e.g., "Core11") to their specific
    PhysicalDevice Properties and Features structs (e.g., VkPhysicalDeviceVulkan11Features).
    It processes struct names from the `CORE_MAPPING_STRUCT_LIST`.
    """
    STRUCTURE_TYPE_PREFIX_PART = "PHYSICAL_DEVICE"
    versions_map = defaultdict(list)  # { "Core11": [{struct: sType}], ... }

    # Process struct names matching the core version pattern (e.g., VkPhysicalDeviceVulkan11Properties)
    for struct_name in struct_names:
        match = CORE_VERSION_REGEX.match(struct_name)
        if match:
            # Extract version digits (e.g., "11") and suffix (e.g., "Properties")
            _, _, version_digits, suffix = match.groups()
            core_key = f"Core{version_digits}"  # Create key like "Core11", "Core12"

            # Format version for the enum name (e.g., "11" -> "1_1")
            if len(version_digits) == 2:
                version_snake = f"{version_digits[0]}_{version_digits[1]}"
            else:  # Unlikely based on current vk.xml naming
                version_snake = version_digits

            # Construct the expected VkStructureType enum value name based on convention
            suffix_upper = suffix.upper()  # "Properties" -> "PROPERTIES"
            structure_type_value = f"VK_STRUCTURE_TYPE_{STRUCTURE_TYPE_PREFIX_PART}_VULKAN_{version_snake}_{suffix_upper}"

            inner_map = {struct_name: structure_type_value}
            versions_map[core_key].append(inner_map)

    sorted_versions_map = dict(sorted(versions_map.items(), key=lambda item: int(item[0][4:])))

    core_comment = """\
# VULKAN_CORES_AND_STRUCTS_MAPPING: Maps core Vulkan versions (e.g., "Core11") to their
# specific PhysicalDevice Properties and Features structs.
# Struct Filters:
# - Name matches "VkPhysicalDeviceVulkan<Version><Properties|Features>" (from CORE_MAPPING_STRUCT_LIST).
# - sType is programmatically derived.
# Format: {"versions": {core_version_key: [{struct_name: sType_enum_value}, ...]}}"""

    allStructInfo = "\n# --- Vulkan Core Version to Struct Mappings ---\n"
    allStructInfo += core_comment + "\n"  # Add the comment
    allStructInfo += """VULKAN_CORES_AND_STRUCTS_MAPPING = {"versions": """
    allStructInfo += f"{pprint.pformat(sorted_versions_map, indent=4, width=100)}"
    allStructInfo += "}\n\n"
    write_py_file(vk_py_file_handle, allStructInfo)

    return {"versions": sorted_versions_map}


# --- Ancillary Mappings & Lists for vk.py ---
def extract_list_size_mapping(struct_elements: List[ET.Element]) :
    """Extracts mappings from list members to their corresponding size-indicating members within structs."""
    member_map = {}  # Stores {list_member_name: size_member_name}
    struct_with_dynamic_size_list_variables = {} # Stores {struct_name: list of list variables}
    for type_element in struct_elements:
        struct_name = type_element.get(ATTR_NAME)
        # Skip invalid or non-physical device structs
        if not check_if_valid_struct(struct_name, None) or not is_valid_struct_name(struct_name):
            continue

        # Find members within the struct that have a 'len' attribute
        for member_element in type_element.findall(f"./{TAG_MEMBER}[@{ATTR_LEN}]"):
            len_attribute_value = member_element.get(ATTR_LEN)  # Name of the member holding the size
            name_tag = member_element.find(TAG_NAME)
            name_value = get_element_text(name_tag)  # Name of the list/array member
            member_type_element = member_element.find(ATTR_TYPE)
            is_pointer = '*' in member_type_element.tail if member_type_element.tail else False

            # Store the mapping if valid (names exist, len is not "null-terminated")
            if name_value and len_attribute_value and len_attribute_value.strip().lower() != "null-terminated":
                member_map[name_value] = len_attribute_value.strip()
                if is_pointer:
                    struct_with_dynamic_size_list_variables.setdefault(struct_name, []).append(name_value)
    return member_map, struct_with_dynamic_size_list_variables


def write_list_size_mapping(struct_elements: List[ET.Element], vk_py_file_handle: IO[str]):

    def sort_dict_by_key_and_content(input_dict):
        """
        Sorts a dictionary first by its keys alphabetically,
        and then sorts the elements within the sets associated with each key.
        """
        sorted_items_by_key = sorted(input_dict.items())
        sorted_dict = {}
        for key, value_set in sorted_items_by_key:
            sorted_dict[key] = sorted(list(value_set))

        return sorted_dict

    def write_formatted_code(list_variable_name, list_data):
        formatted_list_code = pprint.pformat(list_data, indent=4, width=100)
        formatted_content = f"""{list_variable_name} = {formatted_list_code}\n\n"""
        write_py_file(vk_py_file_handle, formatted_content)

    """Extracts and writes the list-member-to-size-member mapping to a Python file."""
    content = "# --- List Size Mappings (Field name to size field name) ---\n"
    write_py_file(vk_py_file_handle, content)

    list_size_map, dynamic_list_variables = extract_list_size_mapping(struct_elements)
    write_formatted_code("LIST_TYPE_FIELD_AND_SIZE_MAPPING", list_size_map)

    sorted_dynamic_list_variables = sort_dict_by_key_and_content(dynamic_list_variables)
    write_formatted_code("STRUCT_WITH_DYNAMIC_SIZE_LIST_MAPPING", sorted_dynamic_list_variables)


def write_all_structs(vk_py_file_handle: IO[str]) -> None:
    """Writes a Python list of all processed PhysicalDevice struct names."""
    VK_PHYSICAL_STRUCT_NAMES.sort()
    comment = (
        "\n# --- List of All Processed Physical Device Structs ---\n"
        "# Includes structs that:\n"
        "# 1. Are not in 'disabled_structs' (implicitly, via VK_PHYSICAL_STRUCT_NAMES population).\n"
        '# 2. Extend "VkPhysicalDeviceProperties2" or "VkPhysicalDeviceFeatures2"\n'
        "#    (i.e., are in 'structs_with_valid_extends')."
    )
    allStructInfo = comment + "\n"
    allStructInfo += "ALL_STRUCTS_EXTENDING_FEATURES_OR_PROPERTIES = [\n"
    # Add each physical device struct name as a string element in the list
    for class_name in VK_PHYSICAL_STRUCT_NAMES:
        allStructInfo += f"    {class_name},\n"
    allStructInfo += "]\n\n"
    write_py_file(vk_py_file_handle, allStructInfo)


def write_extension_independent_structs(structs_in_extensions: Set[str], vk_py_file_handle: IO[str]):
    """Writes a list of PhysicalDevice structs not tied to specific features or enabled extensions."""
    all_physical_device_structs = VK_PHYSICAL_STRUCT_NAMES
    # Find structs that are not disabled and not explicitly required by features/extensions
    independent_structs = [
        struct
        for struct in all_physical_device_structs
        if struct not in disabled_structs and struct in structs_with_valid_extends and struct not in CORE_MAPPING_STRUCT_LIST and struct not in structs_in_extensions
    ]

    independent_structs.sort()
    comment = (
        "\n# --- Extension Independent Structs ---\n"
        "# These are PhysicalDevice structs that meet the following criteria:\n"
        "# 1. Sourced from VK_PHYSICAL_STRUCT_NAMES.\n"
        "# 2. Not in the global 'disabled_structs' list.\n"
        '# 3. Extend "VkPhysicalDeviceProperties2" or "VkPhysicalDeviceFeatures2" (in \'structs_with_valid_extends\').\n'
        "# 4. Not core version-specific (not in CORE_MAPPING_STRUCT_LIST).\n"
        "# 5. Not required by any enabled Vulkan extension (not in 'structs_in_extensions')."
    )
    content = comment + "\n"
    independent_structs_str = pprint.pformat(independent_structs, indent=4, width=100)
    content += f"EXTENSION_INDEPENDENT_STRUCTS = {independent_structs_str}\n\n"
    write_py_file(vk_py_file_handle, content)


# --- Orchestration & Content Generation for Mappings ---
def parse_structs_and_extensions(xml_root: ET.Element, struct_elements: List[ET.Element]):
    """
    Helps in extraction of key mappings for Vulkan structs, features, and extensions.
    Calls helpers to:
    1. Map structs to sTypes (`extract_struct_to_type_mapping`).
    2. Map features to their structs (`extract_feature_struct_mapping`).
    3. Map extensions to their structs (`extract_extension_struct_mapping`).
    Populates global lists/dicts as a side effect.
    """
    # 1. Create the mapping from struct names (including aliases) to sType values
    struct_to_type_map = extract_struct_to_type_mapping(struct_elements)

    # 2. Extract feature-to-struct using api filters
    extract_feature_struct_mapping(xml_root, struct_to_type_map, VK_API_FILTER)
    extract_feature_struct_mapping(xml_root, struct_to_type_map, VK_VULKAN_FILTER)

    # 3. Extract extension-to-struct mappings for enabled extensions, also getting the set of structs used in extensions
    extension_map_data, structs_in_extensions = extract_extension_struct_mapping(xml_root, struct_to_type_map)
    return structs_in_extensions, dict(feature_api_map), extension_map_data


def generate_vk_py_content(
    feature_map: Dict[str, List[Dict[str, str]]],
    extension_map: Dict[str, List[Dict[str, str]]],
) -> str:
    """
    Formats feature and extension struct mappings for vk.py.
    Generates Python dictionary string definitions for:
    - VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING
    - VULKAN_VERSIONS_AND_STRUCTS_MAPPING
    Includes comments detailing their generation filters.
    """
    # --- Vulkan Extension to Struct Mappings ---
    extension_comment = """\
# VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING: Maps enabled extension names to their PhysicalDevice structs.
# Extension Filters:
# - 'supported' is not "disabled".
# - 'platform' (if present) is "android".
# Struct Filters (per extension):
# - Not in global 'disabled_structs'.
# - Extends "VkPhysicalDeviceProperties2" or "VkPhysicalDeviceFeatures2".
# Format: {ext_name: [{struct_name: sType_enum_value}, ...]}"""
    content = "\n# --- Vulkan Extension to Struct Mappings ---\n"
    content += extension_comment + "\n"
    sorted_data = dict(sorted(extension_map.items()))
    extension_map_str = pprint.pformat(sorted_data, indent=4, width=100)
    content += """VULKAN_EXTENSIONS_AND_STRUCTS_MAPPING = {"extensions":\n"""
    content += f"{extension_map_str}"
    content += "}\n\n"

    # --- Vulkan Feature to Struct Mappings ---
    feature_comment = """\
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
# Format: {feature_name: [{struct_name: sType_enum_value}, ...]}"""
    content += "# --- Vulkan Feature to Struct Mappings ---\n"
    content += feature_comment + "\n"
    feature_map_str = pprint.pformat(feature_map, indent=4, width=100)
    content += f"VULKAN_VERSIONS_AND_STRUCTS_MAPPING = {feature_map_str}\n\n"
    return content


def write_initial_content(vk_py_file_handle: IO[str]):
    copyright_header = (
        gencom.copyright_and_warning(datetime.now().year).replace("*/", "").replace("/*", "").replace(" *", "#").replace("// WARNING: This file is generated. See ../README.md for instructions.", "").strip()
    )
    write_py_file(vk_py_file_handle, copyright_header)
    write_py_file(vk_py_file_handle, INIT_CONTENT + INIT_CONSTANTS_CONTENT)


def extract_vkformat_enums(xml_root):
    """
    Parses an XML root element to find <feature> and <extension> tags,
    and extracts names from specific <enum extends="VkFormat"> tags within them.
    """
    result_map = {}
    format_offset_mapping = {}
    # Iterate over <feature> tags
    for feature_tag in xml_root.findall(f".//{TAG_FEATURE}"):
        feature_name = feature_tag.get(f"{ATTR_NAME}")
        if not feature_name:
            continue  # Skip if feature tag has no name

        enum_names = []
        for require_tag in feature_tag.findall(f".//{TAG_REQUIRE}"):
            for enum_tag in require_tag.findall(f'.//enum[@extends="VkFormat"]'):
                enum_name = enum_tag.get(f"{ATTR_NAME}")
                ext_number = enum_tag.get("extnumber")
                ext_offset = enum_tag.get("offset")
                calculated_extension = 0
                if not ext_number or not ext_offset:
                    raise Exception(f"Invalid Extension Found: {enum_name}")
                else:
                    calculated_extension = 1000000000 + ((int(ext_number) - 1) * 1000) + int(ext_offset)
                format_offset_mapping[enum_name] = calculated_extension
                additional_vk_format_values[enum_name] = str(calculated_extension)
                if enum_name:
                    enum_names.append((enum_name, calculated_extension))

        if enum_names:  # Only add if there are relevant enums
            if feature_name in result_map:
                result_map[feature_name].extend(enum_names)
            else:
                result_map[feature_name] = enum_names

    # Iterate over <extension> tags
    for extension_tag in xml_root.findall(f".//{TAG_EXTENSION}"):
        extension_name = extension_tag.get(f"{ATTR_NAME}")
        supported = extension_tag.get(ATTR_SUPPORTED)  # Usually "vulkan", "disabled", etc.
        if supported == "disabled":
            continue
        ext_number = extension_tag.get("number")
        calculated_extension = 0
        if not extension_name:
            continue  # Skip if extension tag has no name

        enum_names = []
        for require_tag in extension_tag.findall(f".//{TAG_REQUIRE}"):
            for enum_tag in require_tag.findall('.//enum[@extends="VkFormat"]'):
                enum_name = enum_tag.get(f"{ATTR_NAME}")
                ext_offset = enum_tag.get("offset")
                ext_alias = enum_tag.get("alias")
                if not ext_number:
                    raise Exception(f"Invalid Extension Found: {enum_name}")
                else:
                    if not ext_offset:
                        if format_offset_mapping.get(ext_alias):
                            calculated_extension = format_offset_mapping[ext_alias]
                    else:
                        calculated_extension = 1000000000 + ((int(ext_number) - 1) * 1000) + int(ext_offset)

                if not ext_alias:
                    additional_vk_format_values[enum_name] = str(calculated_extension)

                if enum_name:
                    enum_names.append((enum_name, calculated_extension))

        if enum_names:  # Only add if there are relevant enums
            if extension_name in result_map:
                result_map[extension_name].extend(enum_names)
            else:
                result_map[extension_name] = enum_names

    # Remove duplicates from lists if any were introduced by multiple require tags
    for key in result_map:
        result_map[key] = list(set(result_map[key]))
        result_map[key] = sorted(result_map[key], key=lambda x: x[0], reverse=True)
        result_map[key] = sorted(result_map[key], key=lambda x: x[1])

    return result_map


def copy_vulkan_1_0_enums(enums_data, vk_format_map):
    copy_vulkan_1_0_vkformats = copy.copy(enums_data["VkFormat"])
    for name, value in copy_vulkan_1_0_vkformats.items():
        if not vk_format_map.get("VK_VERSION_1_0"):
            vk_format_map["VK_VERSION_1_0"] = []
        if int(value) == 0:
            continue
        vk_format_map["VK_VERSION_1_0"].append((name, int(value)))


# --- CODEGEN EXECUTION ---
def gen_vk(xml_path = None, output_path = None):
    """
    Orchestrates parsing of Vulkan XML registry and generation of vk.py.

    Loads vk.xml, then calls a sequence of functions to:
    1. Filter and identify structs (e.g., disable SC-specific ones).
    2. Extract mappings: struct-to-sType, feature-to-structs, extension-to-structs.
    3. Fetch detailed definitions for enums, aliases, structs, and handles.
    4. Write all parsed and processed data into vk.py, including dataclasses,
       constants, aliases, and various mapping dictionaries.
    """
    OUTPUT_VK_PY_PATH = output_path if output_path else SCRIPT_DIR / "vk.py"
    # Ensure output directory exists
    OUTPUT_VK_PY_PATH.parent.mkdir(parents=True, exist_ok=True)

    with open(OUTPUT_VK_PY_PATH, "w", encoding="utf-8") as vk_py_file_handle:
        write_initial_content(vk_py_file_handle)
        xml_root = None
        if xml_path is None:
            xml_root = load_xml_registry(VULKAN_XML_FILE_PATH)
        else:
            xml_root = load_xml_registry(xml_path)

        if xml_root is None:
            return

        all_struct_type_elements = xml_root.findall(f".//{TAG_TYPE}[@{ATTR_CATEGORY}='{CAT_STRUCT}']")
        create_disabled_struct_list(all_struct_type_elements)
        extract_vulkan_sc_features(xml_root)
        empty_dataclass_names = fetch_struct_handles(xml_root)
        structs_in_extensions, feature_map_data, extension_map_data = parse_structs_and_extensions(xml_root, all_struct_type_elements)
        vk_format_map = extract_vkformat_enums(xml_root)
        enums_data = fetch_enums_with_type_attribute(xml_root)
        aliases_vk_flags_data = fetch_flags_aliases(xml_root)
        all_structs_data, struct_alias_data = fetch_all_structs_and_aliases(all_struct_type_elements)
        copy_vulkan_1_0_enums(enums_data, vk_format_map)

        write_enums(vk_py_file_handle, enums_data)
        write_constants(xml_root, vk_py_file_handle)
        write_api_constants(xml_root, vk_py_file_handle)
        write_aliases(vk_py_file_handle, aliases_vk_flags_data, "# --- VkFlags Type Aliases ---", filter_required_data_members=True)
        write_empty_dataclasses(empty_dataclass_names, vk_py_file_handle)
        write_structs(vk_py_file_handle, all_structs_data)
        write_aliases(vk_py_file_handle, struct_alias_data, "# --- Physical Device Struct Aliases ---")
        write_all_structs(vk_py_file_handle)
        write_py_file(vk_py_file_handle, generate_vk_py_content(feature_map=feature_map_data, extension_map=extension_map_data))
        write_extension_independent_structs(structs_in_extensions, vk_py_file_handle)
        generate_core_struct_mapping(CORE_MAPPING_STRUCT_LIST, vk_py_file_handle)
        write_list_size_mapping(all_struct_type_elements, vk_py_file_handle)
        write_py_file(vk_py_file_handle, VULKAN_API_1_0_STRUCTS_CONTENT + "\n\n")
        write_py_file(vk_py_file_handle, ADDITIONAL_EXTENSION_INDEPENDENT_STRUCTS_CONTENT + "\n")
        write_structs_extends_mapping(all_struct_type_elements, vk_py_file_handle)
        enum_traits_content = f"\n# --- Enum Traits Mapping ---\nENUM_TRAITS_MAPPING = {pprint.pformat(enum_member_map, indent=4, width=100, sort_dicts=False)}\n\n"
        write_py_file(vk_py_file_handle, enum_traits_content)
        vk_formats_content = f"\n# --- VK Format Mapping ---\nVK_FORMAT_MAPPING = {pprint.pformat(vk_format_map, indent=4, width=100, sort_dicts=False)}"
        write_py_file(vk_py_file_handle, vk_formats_content)
