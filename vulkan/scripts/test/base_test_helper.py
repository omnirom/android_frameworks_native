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

import difflib
import re
import unittest
from unittest.mock import Mock


class BaseCodeAssertTest(unittest.TestCase):

    @staticmethod
    def normalise_for_code_compare(code: str):
        return re.sub(r'\s+', '', code)

    @staticmethod
    def normalise_for_diff_compare(code: str):
        return [
            line.strip()
            for line in code.splitlines()
        ]

    def assertCodeEqual(self, expected_code_str: str, actual_code_str: str, msg=None):
        """
        This code comparator lacks semantic awareness and performs a naive normalization by
        stripping all whitespace characters, without distinguishing between executable code,
        string literals, or comments.

        Eg: It would treat "Hello world" and "Helloworld" as identical.
        """

        expected_normalised = self.normalise_for_code_compare(expected_code_str)
        actual_normalised = self.normalise_for_code_compare(actual_code_str)

        if expected_normalised == actual_normalised:
            pass
        else:
            expected_for_diff = self.normalise_for_diff_compare(expected_code_str)
            actual_for_diff = self.normalise_for_diff_compare(actual_code_str)

            diff_generator = difflib.unified_diff(
                expected_for_diff,
                actual_for_diff,
                fromfile="Expected Code",
                tofile="Actual Code",
                lineterm="",
                n=1
            )

            diff_output = "\n".join(diff_generator)
            standard_message = (
                f"Test failed: Non-whitespace code differences detected."
                f"---------------------------------------------------\n"
                f"{diff_output}\n"
                f"---------------------------------------------------"
            )

            failure_message = self._formatMessage(msg, standard_message)
            self.fail(failure_message)


class BaseMockCodeFileTest(BaseCodeAssertTest):

    def setUp(self):
        self.mock_file = Mock()

    def assertCodeFileWrite(self, expected_code_str: str):
        actual_code_str = "".join([c.args[0] for c in self.mock_file.write.call_args_list])
        self.assertCodeEqual(expected_code_str, actual_code_str)
