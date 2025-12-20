import os
import base_test_helper as helper
import vk_parser
from pathlib import Path


error_message = """\n
============================================================
ERROR: Outdated vk.py file detected.
------------------------------------------------------------
vk.py (Generated from vk.xml) is outdated.

To fix this, run the following command from your Android Git repository's root directory: (e.g.: main/)
and raise a CL (with all the updated files) adding `agdq-eng@google.com` as the reviewer:
  python frameworks/native/vulkan/scripts/vkjson_codegen.py
============================================================\
"""

# The test `test_vkparser_output` compares the output of the vk_parser against the
# last known-good version of vk.py. This genrule copies the pre-generated file (`vk.py` from
# the scripts directory) into the test environment as `vk_py_baseline.txt`.
class TestVKParserOutput(helper.BaseCodeAssertTest):
    def setUp(self):
        original_file = "vk_py_baseline.txt"
        self.generated_file = Path("vk_py_generated.txt")
        self.sample_input = Path("vk_xml_test_copy.xml")

        if not os.path.exists(original_file):
            self.fail(f"Reference file not found at {original_file}. Cannot run comparison test.")

        with open(original_file, "r", encoding="utf-8") as f:
            self.original_content = f.read()

    def test_output_not_changed(self):
        vk_parser.gen_vk(self.sample_input, self.generated_file)

        new_content = self.generated_file.read_text(encoding="utf-8")

        self.assertCodeEqual(self.original_content, new_content, error_message)

    def tearDown(self):
        try:
            self.generated_file.unlink()
        except FileNotFoundError:
            pass
