/*
 * Copyright 2025 The Android Open Source Project
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

use anyhow::{anyhow, bail, Result};
use core::cell::RefCell;
use nix::libc;
use std::fs;
use std::ops::RangeInclusive;
use std::os::unix::fs::MetadataExt;
use std::path::{Path, PathBuf};

/// Contains device numbers supported by TTY drivers that allow to distinguish TTY devices
/// among all devices in /dev.

/// The file containing descriptions of all TTY drivers.
const DRIVERS_FILE_PATH: &str = "/proc/tty/drivers";

/// Finds type of a TTY driver corresponding to a given major and minor numbers of the device node.
#[mockall::automock]
pub trait DriverTypeFinder {
    /// Find driver type by the device node path /dev/name.
    fn find(&self, devnode_path: &Path) -> Result<String>;
}

/// Implements DriverTypeFinder
pub struct DriverTypeFinderImpl {
    /// The path to the file with drivers info, i.e. /proc/tty/drivers (except for tests).
    drivers_file_path: PathBuf,

    /// Cached content of /proc/tty/drivers, we re-read it when some driver is not found.
    drivers_cache: RefCell<Vec<DriverInfo>>,
}

#[cfg_attr(test, derive(Debug, PartialEq))]
struct DriverInfo {
    /// Major device number supported by the driver.
    pub major: u32,

    /// Range of minor numbers supported by the driver.
    pub minor_range: RangeInclusive<u32>,

    /// The type of the driver: "serial", "console", "system", "pty".
    pub driver_type: String,
}

impl DriverTypeFinder for DriverTypeFinderImpl {
    fn find(&self, devnode_path: &Path) -> Result<String> {
        let (major, minor) = Self::get_devnum(devnode_path)?;
        self.find_by_devnum(major, minor)
    }
}

impl DriverTypeFinderImpl {
    pub fn new() -> Self {
        Self::new_from_file(Path::new(DRIVERS_FILE_PATH).to_path_buf())
    }

    fn new_from_file(drivers_file_path: PathBuf) -> Self {
        Self { drivers_file_path, drivers_cache: RefCell::default() }
    }

    fn get_devnum(devnode_path: &Path) -> Result<(u32, u32)> {
        let devnum = fs::metadata(devnode_path)?.rdev() as libc::dev_t;
        let major = u32::try_from(libc::major(devnum))?;
        let minor = u32::try_from(libc::minor(devnum))?;
        Ok((major, minor))
    }

    fn find_by_devnum(&self, major: u32, minor: u32) -> Result<String> {
        let result = self.find_in_cache(major, minor);
        if result.is_ok() {
            return result;
        }

        // If not found, refresh the cache and retry.
        self.read_drivers_from_file()?;
        self.find_in_cache(major, minor)
    }

    fn find_in_cache(&self, major: u32, minor: u32) -> Result<String> {
        self.drivers_cache
            .borrow()
            .iter()
            .find(|d| d.major == major && d.minor_range.contains(&minor))
            .map(|d| d.driver_type.clone())
            .ok_or(anyhow!("TTY driver with numbers {}, {} not found", major, minor))
    }

    fn read_drivers_from_file(&self) -> Result<()> {
        let mut drivers = self.drivers_cache.borrow_mut();
        drivers.clear();
        for line in fs::read_to_string(&self.drivers_file_path)?.lines() {
            let parts: Vec<&str> = line.split_whitespace().collect();
            if parts.is_empty() {
                continue;
            }
            if parts.len() != 5 {
                bail!("Wrong number of fields in the line '{}'", line);
            }
            drivers.push(DriverInfo {
                driver_type: parts[4].split(":").next().unwrap().to_string(),
                major: parts[2].parse()?,
                minor_range: {
                    let mut minor = parts[3].split("-");
                    let from = minor.next().unwrap();
                    let to = minor.next().unwrap_or(from);
                    from.parse()?..=to.parse()?
                },
            });
        }
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use tempfile::{tempdir, TempDir};

    const REAL_DRIVERS_FILE_CONTENTS: &str = r"
/dev/tty             /dev/tty        5       0 system:/dev/tty
/dev/console         /dev/console    5       1 system:console
/dev/ptmx            /dev/ptmx       5       2 system
acm                  /dev/ttyACM   166 0-255 serial
g_serial             /dev/ttyGS    235       7 serial
ttynull              /dev/ttynull  240       0 console
serial               /dev/ttyS       4 64-95 serial
pty_slave            /dev/pts      136 0-1048575 pty:slave
pty_master           /dev/ptm      128 0-1048575 pty:master
";

    #[test]
    fn test_find_in_real_file_existing() -> Result<()> {
        let dir = tempdir()?;
        let filename = create_drivers_file(&dir, REAL_DRIVERS_FILE_CONTENTS)?;

        let result = DriverTypeFinderImpl::new_from_file(filename).find_by_devnum(4, 95)?;

        assert_eq!(result, "serial");
        Ok(())
    }

    #[test]
    fn test_find_in_real_file_missing() -> Result<()> {
        let dir = tempdir()?;
        let filename = create_drivers_file(&dir, REAL_DRIVERS_FILE_CONTENTS)?;

        let result = DriverTypeFinderImpl::new_from_file(filename).find_by_devnum(4, 96);

        assert!(result.is_err());
        Ok(())
    }

    #[test]
    fn test_read_with_wrong_of_fields() -> Result<()> {
        let dir = tempdir()?;
        let filename = create_drivers_file(&dir, "/dev/tty /dev/tty 5 0")?;

        let result = DriverTypeFinderImpl::new_from_file(filename).find_by_devnum(5, 0);

        assert!(result.is_err());
        Ok(())
    }

    #[test]
    fn test_read_with_wrong_format() -> Result<()> {
        let dir = tempdir()?;
        let filename = create_drivers_file(&dir, "acm /dev/ttyACM 166 00-FF serial")?;

        let result = DriverTypeFinderImpl::new_from_file(filename).find_by_devnum(166, 255);

        assert!(result.is_err());
        Ok(())
    }

    fn create_drivers_file(dir: &TempDir, content: &str) -> Result<PathBuf> {
        let filename = dir.path().join("drivers");
        std::fs::write(&filename, content)?;
        Ok(filename)
    }
}
