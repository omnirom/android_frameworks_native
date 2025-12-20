/*
 * Copyright (C) 2025, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#![no_main]
//! Fuzzer for the ISerialManager service implementation

use android_hardware_serialservice::aidl::android::hardware::serialservice::ISerialManager::BnSerialManager;
use binder::BinderFeatures;
use binder_random_parcel_rs::fuzz_service;
use binder_tokio::TokioRuntime;
use libfuzzer_sys::fuzz_target;
use tokio::runtime::Builder;

use serialservice::serial_manager::SerialManager;

fuzz_target!(|data: &[u8]| {
    let runtime = Builder::new_current_thread().enable_all().build().unwrap();
    let service = BnSerialManager::new_async_binder(
        runtime.block_on(async move { SerialManager::new().await }),
        TokioRuntime(runtime),
        BinderFeatures::default(),
    );
    fuzz_service(&mut service.as_binder(), data);
});
