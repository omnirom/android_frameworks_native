/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//! This is a Native Serial Service that can be used to list, listen to and open serial ports.

use android_hardware_serial_flags::enable_wired_serial_api;
use android_hardware_serialservice::aidl::android::hardware::serialservice::ISerialManager::BnSerialManager;
use android_hardware_serialservice::binder;

use anyhow::{bail, Result};
use binder::BinderFeatures;
use binder_tokio::TokioRuntime;
use tokio::runtime::Handle;

use serialservice::serial_manager::SerialManager;

#[tokio::main]
async fn main() -> Result<()> {
    logger::init(
        logger::Config::default()
            .with_tag_on_device("serialservice")
            .with_max_level(log::LevelFilter::Debug),
    );

    if !enable_wired_serial_api() {
        log::debug!("Serial support isn't enabled. Skipping native serial service");
        bail!("Native Serial Service is disabled");
    }

    logger::init(
        logger::Config::default()
            .with_tag_on_device("serialservice")
            .with_max_level(log::LevelFilter::Debug),
    );

    binder::add_service(
        "native_serial",
        BnSerialManager::new_async_binder(
            SerialManager::new().await,
            TokioRuntime(Handle::current()),
            BinderFeatures::default(),
        )
        .as_binder(),
    )?;

    tokio::task::block_in_place(binder::ProcessState::join_thread_pool);
    Ok(())
}
