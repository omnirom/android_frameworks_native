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

TEST_NAME="$(basename "$0")"
DOCKER_TAG="${TEST_NAME}-${RANDOM}${RANDOM}"
DOCKER_FILE=*.Dockerfile
DOCKER_RUN_FLAGS=

# Guess if we're running as an Android test or directly
if [ "$(ls -1 ${DOCKER_FILE} | wc -l)" == "1" ]; then
    # likely running as `atest binder_sdk_docker_test_XYZ`
    DOCKER_PATH="$(dirname $(readlink --canonicalize --no-newline binder_sdk.zip))"
else
    # likely running directly as `./binder_sdk_docker_test.sh` - provide mode for easy testing
    RED='\033[1;31m'
    NO_COLOR='\033[0m'

    if ! modinfo vsock_loopback &>/dev/null ; then
        echo -e "${RED}Module vsock_loopback is not installed.${NO_COLOR}"
        exit 1
    fi
    if modprobe --dry-run --first-time vsock_loopback &>/dev/null ; then
        echo "Module vsock_loopback is not loaded. Attempting to load..."
        if ! sudo modprobe vsock_loopback ; then
            echo -e "${RED}Module vsock_loopback is not loaded and attempt to load failed.${NO_COLOR}"
            exit 1
        fi
    fi

    DOCKER_RUN_FLAGS="--interactive --tty"

    DOCKER_FILE="$1"
    if [ ! -f "${DOCKER_FILE}" ]; then
        echo -e "${RED}Docker file '${DOCKER_FILE}' doesn't exist. Please provide one as an argument.${NO_COLOR}"
        exit 1
    fi

    if [ ! -d "${ANDROID_BUILD_TOP}" ]; then
        echo -e "${RED}ANDROID_BUILD_TOP doesn't exist. Please lunch some target.${NO_COLOR}"
        exit 1
    fi
    ${ANDROID_BUILD_TOP}/build/soong/soong_ui.bash --make-mode binder_sdk
    BINDER_SDK_ZIP="${ANDROID_BUILD_TOP}/out/soong/.intermediates/frameworks/native/libs/binder/binder_sdk/linux_glibc_x86_64/binder_sdk.zip"
    DOCKER_PATH="$(dirname $(ls -1 ${BINDER_SDK_ZIP} | head --lines=1))"
fi

function cleanup {
    docker rmi --force "${DOCKER_TAG}" 2>/dev/null || true
}
trap cleanup EXIT

docker build --force-rm --tag "${DOCKER_TAG}" --file ${DOCKER_FILE} ${DOCKER_PATH}
docker run ${DOCKER_RUN_FLAGS} --rm "${DOCKER_TAG}"
