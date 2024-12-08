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

FROM gcc:9

RUN echo 'deb http://deb.debian.org/debian bullseye-backports main' >> /etc/apt/sources.list && \
    apt-get update -y && \
    apt-get install -y cmake ninja-build

ADD binder_sdk.zip /
RUN unzip -q -d binder_sdk binder_sdk.zip

WORKDIR /binder_sdk
RUN CC=gcc CXX=g++ cmake -G Ninja -B build .
RUN cmake --build build

WORKDIR /binder_sdk/build
# Alternatively: `ninja test`, but it won't pass parallel argument
ENTRYPOINT [ "ctest", "--parallel", "32", "--output-on-failure" ]
