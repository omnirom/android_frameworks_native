/*
 * Copyright 2024 The Android Open Source Project
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

//! Contains the DataStore, used to store input related data in a persistent way.

use crate::input::KeyboardType;
use log::{debug, error};
use serde::{Deserialize, Serialize};
use std::fs::File;
use std::io::{Read, Write};
use std::path::Path;
use std::sync::{Arc, RwLock};

/// Data store to be used to store information that persistent across device reboots.
pub struct DataStore {
    file_reader_writer: Box<dyn FileReaderWriter>,
    inner: Arc<RwLock<DataStoreInner>>,
}

#[derive(Default)]
struct DataStoreInner {
    is_loaded: bool,
    data: Data,
}

#[derive(Default, Serialize, Deserialize)]
struct Data {
    // Map storing data for keyboard classification for specific devices.
    #[serde(default)]
    keyboard_classifications: Vec<KeyboardClassification>,
    // NOTE: Important things to consider:
    // - Add any data that needs to be persisted here in this struct.
    // - Mark all new fields with "#[serde(default)]" for backward compatibility.
    // - Also, you can't modify the already added fields.
    // - Can add new nested fields to existing structs. e.g. Add another field to the struct
    //   KeyboardClassification and mark it "#[serde(default)]".
}

#[derive(Default, Serialize, Deserialize)]
struct KeyboardClassification {
    descriptor: String,
    keyboard_type: KeyboardType,
    is_finalized: bool,
}

impl DataStore {
    /// Creates a new instance of Data store
    pub fn new(file_reader_writer: Box<dyn FileReaderWriter>) -> Self {
        Self { file_reader_writer, inner: Default::default() }
    }

    fn load(&mut self) {
        if self.inner.read().unwrap().is_loaded {
            return;
        }
        self.load_internal();
    }

    fn load_internal(&mut self) {
        let s = self.file_reader_writer.read();
        let data: Data = if !s.is_empty() {
            let deserialize: Data = match serde_json::from_str(&s) {
                Ok(deserialize) => deserialize,
                Err(msg) => {
                    error!("Unable to deserialize JSON data into struct: {:?} -> {:?}", msg, s);
                    Default::default()
                }
            };
            deserialize
        } else {
            Default::default()
        };

        let mut inner = self.inner.write().unwrap();
        inner.data = data;
        inner.is_loaded = true;
    }

    fn save(&mut self) {
        let string_to_save;
        {
            let inner = self.inner.read().unwrap();
            string_to_save = serde_json::to_string(&inner.data).unwrap();
        }
        self.file_reader_writer.write(string_to_save);
    }

    /// Get keyboard type of the device (as stored in the data store)
    pub fn get_keyboard_type(&mut self, descriptor: &String) -> Option<(KeyboardType, bool)> {
        self.load();
        let data = &self.inner.read().unwrap().data;
        for keyboard_classification in data.keyboard_classifications.iter() {
            if keyboard_classification.descriptor == *descriptor {
                return Some((
                    keyboard_classification.keyboard_type,
                    keyboard_classification.is_finalized,
                ));
            }
        }
        None
    }

    /// Save keyboard type of the device in the data store
    pub fn set_keyboard_type(
        &mut self,
        descriptor: &String,
        keyboard_type: KeyboardType,
        is_finalized: bool,
    ) {
        {
            let data = &mut self.inner.write().unwrap().data;
            data.keyboard_classifications
                .retain(|classification| classification.descriptor != *descriptor);
            data.keyboard_classifications.push(KeyboardClassification {
                descriptor: descriptor.to_string(),
                keyboard_type,
                is_finalized,
            })
        }
        self.save();
    }
}

pub trait FileReaderWriter {
    fn read(&self) -> String;
    fn write(&self, to_write: String);
}

/// Default file reader writer implementation
pub struct DefaultFileReaderWriter {
    filepath: String,
}

impl DefaultFileReaderWriter {
    /// Creates a new instance of Default file reader writer that can read and write string to a
    /// particular file in the filesystem
    pub fn new(filepath: String) -> Self {
        Self { filepath }
    }
}

impl FileReaderWriter for DefaultFileReaderWriter {
    fn read(&self) -> String {
        let path = Path::new(&self.filepath);
        let mut fs_string = String::new();
        match File::open(path) {
            Err(e) => error!("couldn't open {:?}: {}", path, e),
            Ok(mut file) => match file.read_to_string(&mut fs_string) {
                Err(e) => error!("Couldn't read from {:?}: {}", path, e),
                Ok(_) => debug!("Successfully read from file {:?}", path),
            },
        };
        fs_string
    }

    fn write(&self, to_write: String) {
        let path = Path::new(&self.filepath);
        match File::create(path) {
            Err(e) => error!("couldn't create {:?}: {}", path, e),
            Ok(mut file) => match file.write_all(to_write.as_bytes()) {
                Err(e) => error!("Couldn't write to {:?}: {}", path, e),
                Ok(_) => debug!("Successfully saved to file {:?}", path),
            },
        };
    }
}

#[cfg(test)]
mod tests {
    use crate::data_store::{
        test_file_reader_writer::TestFileReaderWriter, DataStore, FileReaderWriter,
    };
    use crate::input::KeyboardType;

    #[test]
    fn test_backward_compatibility_version_1() {
        // This test tests JSON string that will be created by the first version of data store
        // This test SHOULD NOT be modified
        let test_reader_writer = TestFileReaderWriter::new();
        test_reader_writer.write(r#"{"keyboard_classifications":[{"descriptor":"descriptor","keyboard_type":{"type":"Alphabetic"},"is_finalized":true}]}"#.to_string());

        let mut data_store = DataStore::new(Box::new(test_reader_writer));
        let (keyboard_type, is_finalized) =
            data_store.get_keyboard_type(&"descriptor".to_string()).unwrap();
        assert_eq!(keyboard_type, KeyboardType::Alphabetic);
        assert!(is_finalized);
    }
}

#[cfg(test)]
pub mod test_file_reader_writer {

    use crate::data_store::FileReaderWriter;
    use std::sync::{Arc, RwLock};

    #[derive(Default)]
    struct TestFileReaderWriterInner {
        fs_string: String,
    }

    #[derive(Default, Clone)]
    pub struct TestFileReaderWriter(Arc<RwLock<TestFileReaderWriterInner>>);

    impl TestFileReaderWriter {
        pub fn new() -> Self {
            Default::default()
        }
    }

    impl FileReaderWriter for TestFileReaderWriter {
        fn read(&self) -> String {
            self.0.read().unwrap().fs_string.clone()
        }

        fn write(&self, fs_string: String) {
            self.0.write().unwrap().fs_string = fs_string;
        }
    }
}
