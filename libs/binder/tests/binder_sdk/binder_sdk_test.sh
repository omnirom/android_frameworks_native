#!/bin/bash

#
# Copyright (C) 2024 The Android Open Source Project
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
#

set -ex

RED='\033[1;31m'
NO_COLOR='\033[0m'

if [ ! -f "binder_sdk.zip" ]; then
    echo -e "${RED}binder_sdk.zip doesn't exist. Are you running this test through 'atest binder_sdk_test'?${NO_COLOR}"
    exit 1
fi

mkdir -p bin
cp `pwd`/cmake bin/cmake
cp `pwd`/ctest bin/ctest
export PATH="`pwd`/bin:$PATH"

WORKDIR=workdir_$RANDOM$RANDOM$RANDOM
unzip -q -d $WORKDIR binder_sdk.zip
cd $WORKDIR

cmake .
make -j
make test ARGS="--parallel 32 --output-on-failure"
