"""
This script acts as a linter to ensure that all flags defined in
frameworks/native/services/surfaceflinger/common/FlagManager.cpp are also dumped
for debugging purposes.

It is run as a presubmit test on Gerrit.
"""

import re
import sys
import os
import unittest

class SurfaceFlingerFlagManagerTest(unittest.TestCase):
    """
    A unittest class to check for missing flag dumps in FlagManager.cpp.
    """
    def testSurfaceFlingerFlagManagerFlagsAreDumped(self):
        """
        Tests that every flag defined with a FLAG_MANAGER_* macro has a
        corresponding DUMP_* macro.
        """
        file_path = "FlagManager.cpp"
        self.assertTrue(os.path.exists(file_path), f"FlagManager.cpp not found at {file_path}")

        with open(file_path, 'r') as f:
            content = f.read()

        # Find all flags defined with FLAG_MANAGER_* macros
        defined_flags = re.findall(
            r'FLAG_MANAGER_(?:SYSPROP_FLAG|LEGACY_SERVER_FLAG|ACONFIG_FLAG|'
            r'ACONFIG_FLAG_IMPORTED)\s*\(\s*([a-zA-Z0-9_]+)',
            content
        )

        # Find all flags dumped with DUMP_* macros
        dumped_flags = re.findall(
            r'DUMP_(?:SYSPROP_FLAG|LEGACY_SERVER_FLAG|ACONFIG_FLAG)\s*\(\s*([a-zA-Z0-9_]+)',
            content
        )

        missing_flags = []
        for flag in defined_flags:
            if flag not in dumped_flags:
                missing_flags.append(flag)

        if missing_flags:
            self.fail(
                f"The following flags are defined but not dumped:\n  - "
                f"{'.\n  - '.join(missing_flags)}"
            )

if __name__ == "__main__":
    unittest.main(verbosity=3)
