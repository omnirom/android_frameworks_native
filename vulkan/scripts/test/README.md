# Fixing vk.py Sync Failures
----------------------------------------------------
A failure in the `test_vkparser_output` test means `vk.py` is out of sync with the official Vulkan specification (`vk.xml`).

Follow the two-step process below: first, fix the failing test, and second, file a bug to ensure the rest of the data pipeline is updated.

----------------------------------------------------
## 1. How to Fix the Test
----------------------------------------------------
This step regenerates `vk.py` to match the latest Vulkan specification, which will fix the failing presubmit test.

**Command**: Run the following from your Android project root (`$ANDROID_BUILD_TOP`):
```
    python frameworks/native/vulkan/scripts/vkjson_codegen.py
```

**Reviewer**: `agdq-eng@google.com`
**Commit Message**: Use the following template for your change:
```
[vkjson] Update vk.py to the latest Vulkan spec

Regenerates vk.py to match the updated vk.xml.
This change fixes the outdated vk.py and related vkjson_*.cc files.

Bug: <bug_id>
Test: Presubmit passes for test_vkparser_output.
```

----------------------------------------------------
## 2. How to Create a Bug for Pipeline Updates
----------------------------------------------------
After your CL from Part 1 is submitted, you must **file a bug** to get the downstream components updated. This is critical for updating the live EDI pipeline.

Create a new bug and use the template below.

**Create the bug to**: `Android > Android OS & Apps > graphics > vulkan` component.
**Assign the bug to**: `agdq-eng@google.com`

### Bug Template

**Title**:
[Vulkan Pipeline] Update Collector and Proto to match latest vk.py
**Description**:
The vk.py file was updated in CL [LINK TO YOUR CL HERE] to sync with the latest vk.xml. The downstream Vulkan collector and Protobuf schema must now be updated to maintain pipeline integrity.
Steps to fix the pipeline are outlined in:
`### Vulkan EDI Pipeline Update Guide` located at (Android Repository):
`$ANDROID_BUILD_TOP/frameworks/native/vulkan/README.md`