/*
 ** Copyright 2023, The Android Open Source Project
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#include "MultifileBlobCache.h"

#include <android-base/properties.h>
#include <android-base/test_utils.h>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <utils/JenkinsHash.h>

#include <fstream>
#include <memory>

#include <com_android_graphics_egl_flags.h>

using namespace com::android::graphics::egl;

using namespace std::literals;

namespace android {

template <typename T>
using sp = std::shared_ptr<T>;

constexpr size_t kMaxKeySize = 2 * 1024;
constexpr size_t kMaxValueSize = 6 * 1024;
constexpr size_t kMaxTotalSize = 32 * 1024;
constexpr size_t kMaxTotalEntries = 64;

class MultifileBlobCacheTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        clearProperties();
        mTempFile.reset(new TemporaryFile());
        mMBC.reset(new MultifileBlobCache(kMaxKeySize, kMaxValueSize, kMaxTotalSize,
                                          kMaxTotalEntries, &mTempFile->path[0]));
    }

    virtual void TearDown() {
        clearProperties();
        mMBC.reset();
    }

    int getFileDescriptorCount();
    std::vector<std::string> getCacheEntries();

    void clearProperties();
    bool clearCache();

    std::unique_ptr<TemporaryFile> mTempFile;
    std::unique_ptr<MultifileBlobCache> mMBC;
};

void MultifileBlobCacheTest::clearProperties() {
    // Clear any debug properties used in the tests
    base::SetProperty("debug.egl.blobcache.cache_version", "");
    base::WaitForProperty("debug.egl.blobcache.cache_version", "");

    base::SetProperty("debug.egl.blobcache.build_id", "");
    base::WaitForProperty("debug.egl.blobcache.build_id", "");
}

TEST_F(MultifileBlobCacheTest, CacheSingleValueSucceeds) {
    unsigned char buf[4] = {0xee, 0xee, 0xee, 0xee};
    mMBC->set("abcd", 4, "efgh", 4);
    ASSERT_EQ(size_t(4), mMBC->get("abcd", 4, buf, 4));
    ASSERT_EQ('e', buf[0]);
    ASSERT_EQ('f', buf[1]);
    ASSERT_EQ('g', buf[2]);
    ASSERT_EQ('h', buf[3]);
}

TEST_F(MultifileBlobCacheTest, CacheTwoValuesSucceeds) {
    unsigned char buf[2] = {0xee, 0xee};
    mMBC->set("ab", 2, "cd", 2);
    mMBC->set("ef", 2, "gh", 2);
    ASSERT_EQ(size_t(2), mMBC->get("ab", 2, buf, 2));
    ASSERT_EQ('c', buf[0]);
    ASSERT_EQ('d', buf[1]);
    ASSERT_EQ(size_t(2), mMBC->get("ef", 2, buf, 2));
    ASSERT_EQ('g', buf[0]);
    ASSERT_EQ('h', buf[1]);
}

TEST_F(MultifileBlobCacheTest, GetSetTwiceSucceeds) {
    unsigned char buf[2] = {0xee, 0xee};
    mMBC->set("ab", 2, "cd", 2);
    ASSERT_EQ(size_t(2), mMBC->get("ab", 2, buf, 2));
    ASSERT_EQ('c', buf[0]);
    ASSERT_EQ('d', buf[1]);
    // Use the same key, but different value
    mMBC->set("ab", 2, "ef", 2);
    ASSERT_EQ(size_t(2), mMBC->get("ab", 2, buf, 2));
    ASSERT_EQ('e', buf[0]);
    ASSERT_EQ('f', buf[1]);
}

TEST_F(MultifileBlobCacheTest, GetOnlyWritesInsideBounds) {
    unsigned char buf[6] = {0xee, 0xee, 0xee, 0xee, 0xee, 0xee};
    mMBC->set("abcd", 4, "efgh", 4);
    ASSERT_EQ(size_t(4), mMBC->get("abcd", 4, buf + 1, 4));
    ASSERT_EQ(0xee, buf[0]);
    ASSERT_EQ('e', buf[1]);
    ASSERT_EQ('f', buf[2]);
    ASSERT_EQ('g', buf[3]);
    ASSERT_EQ('h', buf[4]);
    ASSERT_EQ(0xee, buf[5]);
}

TEST_F(MultifileBlobCacheTest, GetOnlyWritesIfBufferIsLargeEnough) {
    unsigned char buf[3] = {0xee, 0xee, 0xee};
    mMBC->set("abcd", 4, "efgh", 4);
    ASSERT_EQ(size_t(4), mMBC->get("abcd", 4, buf, 3));
    ASSERT_EQ(0xee, buf[0]);
    ASSERT_EQ(0xee, buf[1]);
    ASSERT_EQ(0xee, buf[2]);
}

TEST_F(MultifileBlobCacheTest, GetDoesntAccessNullBuffer) {
    mMBC->set("abcd", 4, "efgh", 4);
    ASSERT_EQ(size_t(4), mMBC->get("abcd", 4, nullptr, 0));
}

TEST_F(MultifileBlobCacheTest, MultipleSetsCacheLatestValue) {
    unsigned char buf[4] = {0xee, 0xee, 0xee, 0xee};
    mMBC->set("abcd", 4, "efgh", 4);
    mMBC->set("abcd", 4, "ijkl", 4);
    ASSERT_EQ(size_t(4), mMBC->get("abcd", 4, buf, 4));
    ASSERT_EQ('i', buf[0]);
    ASSERT_EQ('j', buf[1]);
    ASSERT_EQ('k', buf[2]);
    ASSERT_EQ('l', buf[3]);
}

TEST_F(MultifileBlobCacheTest, SecondSetKeepsFirstValueIfTooLarge) {
    unsigned char buf[kMaxValueSize + 1] = {0xee, 0xee, 0xee, 0xee};
    mMBC->set("abcd", 4, "efgh", 4);
    mMBC->set("abcd", 4, buf, kMaxValueSize + 1);
    ASSERT_EQ(size_t(4), mMBC->get("abcd", 4, buf, 4));
    ASSERT_EQ('e', buf[0]);
    ASSERT_EQ('f', buf[1]);
    ASSERT_EQ('g', buf[2]);
    ASSERT_EQ('h', buf[3]);
}

TEST_F(MultifileBlobCacheTest, DoesntCacheIfKeyIsTooBig) {
    char key[kMaxKeySize + 1];
    unsigned char buf[4] = {0xee, 0xee, 0xee, 0xee};
    for (int i = 0; i < kMaxKeySize + 1; i++) {
        key[i] = 'a';
    }
    mMBC->set(key, kMaxKeySize + 1, "bbbb", 4);
    ASSERT_EQ(size_t(0), mMBC->get(key, kMaxKeySize + 1, buf, 4));
    ASSERT_EQ(0xee, buf[0]);
    ASSERT_EQ(0xee, buf[1]);
    ASSERT_EQ(0xee, buf[2]);
    ASSERT_EQ(0xee, buf[3]);
}

TEST_F(MultifileBlobCacheTest, DoesntCacheIfValueIsTooBig) {
    char buf[kMaxValueSize + 1];
    for (int i = 0; i < kMaxValueSize + 1; i++) {
        buf[i] = 'b';
    }
    mMBC->set("abcd", 4, buf, kMaxValueSize + 1);
    for (int i = 0; i < kMaxValueSize + 1; i++) {
        buf[i] = 0xee;
    }
    ASSERT_EQ(size_t(0), mMBC->get("abcd", 4, buf, kMaxValueSize + 1));
    for (int i = 0; i < kMaxValueSize + 1; i++) {
        SCOPED_TRACE(i);
        ASSERT_EQ(0xee, buf[i]);
    }
}

TEST_F(MultifileBlobCacheTest, CacheMaxKeySizeSucceeds) {
    char key[kMaxKeySize];
    unsigned char buf[4] = {0xee, 0xee, 0xee, 0xee};
    for (int i = 0; i < kMaxKeySize; i++) {
        key[i] = 'a';
    }
    mMBC->set(key, kMaxKeySize, "wxyz", 4);
    ASSERT_EQ(size_t(4), mMBC->get(key, kMaxKeySize, buf, 4));
    ASSERT_EQ('w', buf[0]);
    ASSERT_EQ('x', buf[1]);
    ASSERT_EQ('y', buf[2]);
    ASSERT_EQ('z', buf[3]);
}

TEST_F(MultifileBlobCacheTest, CacheMaxValueSizeSucceeds) {
    char buf[kMaxValueSize];
    for (int i = 0; i < kMaxValueSize; i++) {
        buf[i] = 'b';
    }
    mMBC->set("abcd", 4, buf, kMaxValueSize);
    for (int i = 0; i < kMaxValueSize; i++) {
        buf[i] = 0xee;
    }
    mMBC->get("abcd", 4, buf, kMaxValueSize);
    for (int i = 0; i < kMaxValueSize; i++) {
        SCOPED_TRACE(i);
        ASSERT_EQ('b', buf[i]);
    }
}

TEST_F(MultifileBlobCacheTest, CacheMaxKeyAndValueSizeSucceeds) {
    char key[kMaxKeySize];
    for (int i = 0; i < kMaxKeySize; i++) {
        key[i] = 'a';
    }
    char buf[kMaxValueSize];
    for (int i = 0; i < kMaxValueSize; i++) {
        buf[i] = 'b';
    }
    mMBC->set(key, kMaxKeySize, buf, kMaxValueSize);
    for (int i = 0; i < kMaxValueSize; i++) {
        buf[i] = 0xee;
    }
    mMBC->get(key, kMaxKeySize, buf, kMaxValueSize);
    for (int i = 0; i < kMaxValueSize; i++) {
        SCOPED_TRACE(i);
        ASSERT_EQ('b', buf[i]);
    }
}

TEST_F(MultifileBlobCacheTest, CacheMaxEntrySucceeds) {
    // Fill the cache with max entries
    int i = 0;
    for (i = 0; i < kMaxTotalEntries; i++) {
        mMBC->set(std::to_string(i).c_str(), sizeof(i), std::to_string(i).c_str(), sizeof(i));
    }

    // Ensure it is full
    ASSERT_EQ(mMBC->getTotalEntries(), kMaxTotalEntries);

    // Add another entry
    mMBC->set(std::to_string(i).c_str(), sizeof(i), std::to_string(i).c_str(), sizeof(i));

    // Ensure total entries is cut in half + 1
    ASSERT_EQ(mMBC->getTotalEntries(), kMaxTotalEntries / 2 + 1);
}

TEST_F(MultifileBlobCacheTest, CacheMinKeyAndValueSizeSucceeds) {
    unsigned char buf[1] = {0xee};
    mMBC->set("x", 1, "y", 1);
    ASSERT_EQ(size_t(1), mMBC->get("x", 1, buf, 1));
    ASSERT_EQ('y', buf[0]);
}

int MultifileBlobCacheTest::getFileDescriptorCount() {
    DIR* directory = opendir("/proc/self/fd");

    int fileCount = 0;
    struct dirent* entry;
    while ((entry = readdir(directory)) != NULL) {
        fileCount++;
        // printf("File: %s\n", entry->d_name);
    }

    closedir(directory);
    return fileCount;
}

TEST_F(MultifileBlobCacheTest, EnsureFileDescriptorsClosed) {
    // Populate the cache with a bunch of entries
    for (int i = 0; i < kMaxTotalEntries; i++) {
        // printf("Caching: %i", i);

        // Use the index as the key and value
        mMBC->set(&i, sizeof(i), &i, sizeof(i));

        int result = 0;
        ASSERT_EQ(sizeof(i), mMBC->get(&i, sizeof(i), &result, sizeof(result)));
        ASSERT_EQ(i, result);
    }

    // Ensure we don't have a bunch of open fds
    ASSERT_LT(getFileDescriptorCount(), kMaxTotalEntries / 2);

    // Close the cache so everything writes out
    mMBC->finish();
    mMBC.reset();

    // Now open it again and ensure we still don't have a bunch of open fds
    mMBC.reset(new MultifileBlobCache(kMaxKeySize, kMaxValueSize, kMaxTotalSize, kMaxTotalEntries,
                                      &mTempFile->path[0]));

    // Check after initialization
    ASSERT_LT(getFileDescriptorCount(), kMaxTotalEntries / 2);

    for (int i = 0; i < kMaxTotalEntries; i++) {
        int result = 0;
        ASSERT_EQ(sizeof(i), mMBC->get(&i, sizeof(i), &result, sizeof(result)));
        ASSERT_EQ(i, result);
    }

    // And again after we've actually used it
    ASSERT_LT(getFileDescriptorCount(), kMaxTotalEntries / 2);
}

std::vector<std::string> MultifileBlobCacheTest::getCacheEntries() {
    std::string cachePath = &mTempFile->path[0];
    std::string multifileDirName = cachePath + ".multifile";
    std::vector<std::string> cacheEntries;

    struct stat info;
    if (stat(multifileDirName.c_str(), &info) == 0) {
        // We have a multifile dir. Skip the status file and return the entries.
        DIR* dir;
        struct dirent* entry;
        if ((dir = opendir(multifileDirName.c_str())) != nullptr) {
            while ((entry = readdir(dir)) != nullptr) {
                if (entry->d_name == "."s || entry->d_name == ".."s) {
                    continue;
                }
                if (strcmp(entry->d_name, kMultifileBlobCacheStatusFile) == 0) {
                    continue;
                }
                // printf("Found entry: %s\n", entry->d_name);
                cacheEntries.push_back(multifileDirName + "/" + entry->d_name);
            }
        } else {
            printf("Unable to open %s, error: %s\n", multifileDirName.c_str(),
                   std::strerror(errno));
        }
    } else {
        printf("Unable to stat %s, error: %s\n", multifileDirName.c_str(), std::strerror(errno));
    }

    return cacheEntries;
}

TEST_F(MultifileBlobCacheTest, CacheContainsStatus) {
    struct stat info;
    std::stringstream statusFile;
    statusFile << &mTempFile->path[0] << ".multifile/" << kMultifileBlobCacheStatusFile;

    // After INIT, cache should have a status
    ASSERT_TRUE(stat(statusFile.str().c_str(), &info) == 0);

    // Set one entry
    mMBC->set("abcd", 4, "efgh", 4);

    // Close the cache so everything writes out
    mMBC->finish();
    mMBC.reset();

    // Ensure status lives after closing the cache
    ASSERT_TRUE(stat(statusFile.str().c_str(), &info) == 0);

    // Open the cache again
    mMBC.reset(new MultifileBlobCache(kMaxKeySize, kMaxValueSize, kMaxTotalSize, kMaxTotalEntries,
                                      &mTempFile->path[0]));

    // Ensure we still have a status
    ASSERT_TRUE(stat(statusFile.str().c_str(), &info) == 0);
}

// Verify missing cache status file causes cache the be cleared
TEST_F(MultifileBlobCacheTest, MissingCacheStatusClears) {
    // Set one entry
    mMBC->set("abcd", 4, "efgh", 4);

    // Close the cache so everything writes out
    mMBC->finish();
    mMBC.reset();

    // Ensure there is one cache entry
    ASSERT_EQ(getCacheEntries().size(), 1);

    // Delete the status file
    std::stringstream statusFile;
    statusFile << &mTempFile->path[0] << ".multifile/" << kMultifileBlobCacheStatusFile;
    remove(statusFile.str().c_str());

    // Open the cache again and ensure no cache hits
    mMBC.reset(new MultifileBlobCache(kMaxKeySize, kMaxValueSize, kMaxTotalSize, kMaxTotalEntries,
                                      &mTempFile->path[0]));

    // Ensure we have no entries
    ASSERT_EQ(getCacheEntries().size(), 0);
}

// Verify modified cache status file BEGIN causes cache to be cleared
TEST_F(MultifileBlobCacheTest, ModifiedCacheStatusBeginClears) {
    // Set one entry
    mMBC->set("abcd", 4, "efgh", 4);

    // Close the cache so everything writes out
    mMBC->finish();
    mMBC.reset();

    // Ensure there is one cache entry
    ASSERT_EQ(getCacheEntries().size(), 1);

    // Modify the status file
    std::stringstream statusFile;
    statusFile << &mTempFile->path[0] << ".multifile/" << kMultifileBlobCacheStatusFile;

    // Stomp on the beginning of the cache file
    const char* stomp = "BADF00D";
    std::fstream fs(statusFile.str());
    fs.seekp(0, std::ios_base::beg);
    fs.write(stomp, strlen(stomp));
    fs.flush();
    fs.close();

    // Open the cache again and ensure no cache hits
    mMBC.reset(new MultifileBlobCache(kMaxKeySize, kMaxValueSize, kMaxTotalSize, kMaxTotalEntries,
                                      &mTempFile->path[0]));

    // Ensure we have no entries
    ASSERT_EQ(getCacheEntries().size(), 0);
}

// Verify modified cache status file END causes cache to be cleared
TEST_F(MultifileBlobCacheTest, ModifiedCacheStatusEndClears) {
    // Set one entry
    mMBC->set("abcd", 4, "efgh", 4);

    // Close the cache so everything writes out
    mMBC->finish();
    mMBC.reset();

    // Ensure there is one cache entry
    ASSERT_EQ(getCacheEntries().size(), 1);

    // Modify the status file
    std::stringstream statusFile;
    statusFile << &mTempFile->path[0] << ".multifile/" << kMultifileBlobCacheStatusFile;

    // Stomp on the END of the cache status file, modifying its contents
    const char* stomp = "BADF00D";
    std::fstream fs(statusFile.str());
    fs.seekp(-strlen(stomp), std::ios_base::end);
    fs.write(stomp, strlen(stomp));
    fs.flush();
    fs.close();

    // Open the cache again and ensure no cache hits
    mMBC.reset(new MultifileBlobCache(kMaxKeySize, kMaxValueSize, kMaxTotalSize, kMaxTotalEntries,
                                      &mTempFile->path[0]));

    // Ensure we have no entries
    ASSERT_EQ(getCacheEntries().size(), 0);
}

// Verify mismatched cacheVersion causes cache to be cleared
TEST_F(MultifileBlobCacheTest, MismatchedCacheVersionClears) {
    // Set one entry
    mMBC->set("abcd", 4, "efgh", 4);

    uint32_t initialCacheVersion = mMBC->getCurrentCacheVersion();

    // Close the cache so everything writes out
    mMBC->finish();
    mMBC.reset();

    // Ensure there is one cache entry
    ASSERT_EQ(getCacheEntries().size(), 1);

    // Set a debug cacheVersion
    std::string newCacheVersion = std::to_string(initialCacheVersion + 1);
    ASSERT_TRUE(base::SetProperty("debug.egl.blobcache.cache_version", newCacheVersion.c_str()));
    ASSERT_TRUE(
            base::WaitForProperty("debug.egl.blobcache.cache_version", newCacheVersion.c_str()));

    // Open the cache again and ensure no cache hits
    mMBC.reset(new MultifileBlobCache(kMaxKeySize, kMaxValueSize, kMaxTotalSize, kMaxTotalEntries,
                                      &mTempFile->path[0]));

    // Ensure we have no entries
    ASSERT_EQ(getCacheEntries().size(), 0);
}

// Verify mismatched buildId causes cache to be cleared
TEST_F(MultifileBlobCacheTest, MismatchedBuildIdClears) {
    // Set one entry
    mMBC->set("abcd", 4, "efgh", 4);

    // Close the cache so everything writes out
    mMBC->finish();
    mMBC.reset();

    // Ensure there is one cache entry
    ASSERT_EQ(getCacheEntries().size(), 1);

    // Set a debug buildId
    base::SetProperty("debug.egl.blobcache.build_id", "foo");
    base::WaitForProperty("debug.egl.blobcache.build_id", "foo");

    // Open the cache again and ensure no cache hits
    mMBC.reset(new MultifileBlobCache(kMaxKeySize, kMaxValueSize, kMaxTotalSize, kMaxTotalEntries,
                                      &mTempFile->path[0]));

    // Ensure we have no entries
    ASSERT_EQ(getCacheEntries().size(), 0);
}

// Ensure cache is correct when a key is reused
TEST_F(MultifileBlobCacheTest, SameKeyDifferentValues) {
    if (!flags::multifile_blobcache_advanced_usage()) {
        GTEST_SKIP() << "Skipping test that requires multifile_blobcache_advanced_usage flag";
    }

    unsigned char buf[4] = {0xee, 0xee, 0xee, 0xee};

    size_t startingSize = mMBC->getTotalSize();

    // New cache should be empty
    ASSERT_EQ(startingSize, 0);

    // Set an initial value
    mMBC->set("ab", 2, "cdef", 4);

    // Grab the new size
    size_t firstSize = mMBC->getTotalSize();

    // Ensure the size went up
    // Note: Checking for an exact size is challenging, as the
    // file size can differ between platforms.
    ASSERT_GT(firstSize, startingSize);

    // Verify the cache is correct
    ASSERT_EQ(size_t(4), mMBC->get("ab", 2, buf, 4));
    ASSERT_EQ('c', buf[0]);
    ASSERT_EQ('d', buf[1]);
    ASSERT_EQ('e', buf[2]);
    ASSERT_EQ('f', buf[3]);

    // Now reuse the key with a smaller value
    mMBC->set("ab", 2, "gh", 2);

    // Grab the new size
    size_t secondSize = mMBC->getTotalSize();

    // Ensure it decreased in size
    ASSERT_LT(secondSize, firstSize);

    // Verify the cache is correct
    ASSERT_EQ(size_t(2), mMBC->get("ab", 2, buf, 2));
    ASSERT_EQ('g', buf[0]);
    ASSERT_EQ('h', buf[1]);

    // Now put back the original value
    mMBC->set("ab", 2, "cdef", 4);

    // And we should get back a stable size
    size_t finalSize = mMBC->getTotalSize();
    ASSERT_EQ(firstSize, finalSize);
}

// Ensure cache is correct when a key is reused with large value size
TEST_F(MultifileBlobCacheTest, SameKeyLargeValues) {
    if (!flags::multifile_blobcache_advanced_usage()) {
        GTEST_SKIP() << "Skipping test that requires multifile_blobcache_advanced_usage flag";
    }

    // Create the cache with larger limits to stress test reuse
    constexpr uint32_t kLocalMaxKeySize = 1 * 1024 * 1024;
    constexpr uint32_t kLocalMaxValueSize = 4 * 1024 * 1024;
    constexpr uint32_t kLocalMaxTotalSize = 32 * 1024 * 1024;
    mMBC.reset(new MultifileBlobCache(kLocalMaxKeySize, kLocalMaxValueSize, kLocalMaxTotalSize,
                                      kMaxTotalEntries, &mTempFile->path[0]));

    constexpr uint32_t kLargeValueCount = 8;
    constexpr uint32_t kLargeValueSize = 64 * 1024;

    // Create a several really large values
    unsigned char largeValue[kLargeValueCount][kLargeValueSize];
    for (int i = 0; i < kLargeValueCount; i++) {
        for (int j = 0; j < kLargeValueSize; j++) {
            // Fill the value with the index for uniqueness
            largeValue[i][j] = i;
        }
    }

    size_t startingSize = mMBC->getTotalSize();

    // New cache should be empty
    ASSERT_EQ(startingSize, 0);

    // Cycle through the values and set them all in sequence
    for (int i = 0; i < kLargeValueCount; i++) {
        mMBC->set("abcd", 4, largeValue[i], kLargeValueSize);
    }

    // Ensure we get the last one back
    unsigned char outBuf[kLargeValueSize];
    mMBC->get("abcd", 4, outBuf, kLargeValueSize);

    for (int i = 0; i < kLargeValueSize; i++) {
        // Buffer should contain highest index value
        ASSERT_EQ(kLargeValueCount - 1, outBuf[i]);
    }
}

// Ensure cache eviction is LRU
TEST_F(MultifileBlobCacheTest, CacheEvictionIsLRU) {
    if (!flags::multifile_blobcache_advanced_usage()) {
        GTEST_SKIP() << "Skipping test that requires multifile_blobcache_advanced_usage flag";
    }

    // Fill the cache with exactly how much it can hold
    int entry = 0;
    for (entry = 0; entry < kMaxTotalEntries; entry++) {
        // Use the index as the key and value
        mMBC->set(&entry, sizeof(entry), &entry, sizeof(entry));

        int result = 0;
        ASSERT_EQ(sizeof(entry), mMBC->get(&entry, sizeof(entry), &result, sizeof(result)));
        ASSERT_EQ(entry, result);
    }

    // Ensure the cache is full
    ASSERT_EQ(mMBC->getTotalEntries(), kMaxTotalEntries);

    // Add one more entry to trigger eviction
    size_t overflowEntry = kMaxTotalEntries;
    mMBC->set(&overflowEntry, sizeof(overflowEntry), &overflowEntry, sizeof(overflowEntry));

    // Verify it contains the right amount, which will be one more than reduced size
    // because we evict the cache before adding a new entry
    size_t evictionLimit = kMaxTotalEntries / mMBC->getTotalCacheSizeDivisor();
    ASSERT_EQ(mMBC->getTotalEntries(), evictionLimit + 1);

    // Ensure cache is as expected, with old entries removed, newer entries remaining
    for (entry = 0; entry < kMaxTotalEntries; entry++) {
        int result = 0;
        mMBC->get(&entry, sizeof(entry), &result, sizeof(result));

        if (entry < evictionLimit) {
            // We should get no hits on evicted entries, i.e. the first added
            ASSERT_EQ(result, 0);
        } else {
            // Above the limit should still be present
            ASSERT_EQ(result, entry);
        }
    }
}

// Ensure calling GET on an entry updates its access time, even if already in hotcache
TEST_F(MultifileBlobCacheTest, GetUpdatesAccessTime) {
    if (!flags::multifile_blobcache_advanced_usage()) {
        GTEST_SKIP() << "Skipping test that requires multifile_blobcache_advanced_usage flag";
    }

    // Fill the cache with exactly how much it can hold
    int entry = 0;
    int result = 0;
    for (entry = 0; entry < kMaxTotalEntries; entry++) {
        // Use the index as the key and value
        mMBC->set(&entry, sizeof(entry), &entry, sizeof(entry));
        ASSERT_EQ(sizeof(entry), mMBC->get(&entry, sizeof(entry), &result, sizeof(result)));
        ASSERT_EQ(entry, result);
    }

    // Ensure the cache is full
    ASSERT_EQ(mMBC->getTotalEntries(), kMaxTotalEntries);

    // GET the first few entries to update their access time
    std::vector<int> accessedEntries = {1, 2, 3};
    for (int i = 0; i < accessedEntries.size(); i++) {
        entry = accessedEntries[i];
        ASSERT_EQ(sizeof(entry), mMBC->get(&entry, sizeof(entry), &result, sizeof(result)));
    }

    // Add one more entry to trigger eviction
    size_t overflowEntry = kMaxTotalEntries;
    mMBC->set(&overflowEntry, sizeof(overflowEntry), &overflowEntry, sizeof(overflowEntry));

    size_t evictionLimit = kMaxTotalEntries / mMBC->getTotalCacheSizeDivisor();

    // Ensure cache is as expected, with old entries removed, newer entries remaining
    for (entry = 0; entry < kMaxTotalEntries; entry++) {
        int result = 0;
        mMBC->get(&entry, sizeof(entry), &result, sizeof(result));

        if (std::find(accessedEntries.begin(), accessedEntries.end(), entry) !=
            accessedEntries.end()) {
            // If this is one of the handful we accessed after filling the cache,
            // they should still be in the cache because LRU
            ASSERT_EQ(result, entry);
        } else if (entry >= (evictionLimit + accessedEntries.size())) {
            // If they were above the eviction limit (plus three for our updated entries),
            // they should still be present
            ASSERT_EQ(result, entry);
        } else {
            // Otherwise, they shold be evicted and no longer present
            ASSERT_EQ(result, 0);
        }
    }

    // Close the cache so everything writes out
    mMBC->finish();
    mMBC.reset();

    // Open the cache again
    mMBC.reset(new MultifileBlobCache(kMaxKeySize, kMaxValueSize, kMaxTotalSize, kMaxTotalEntries,
                                      &mTempFile->path[0]));

    // Check the cache again, ensuring the updated access time made it to disk
    for (entry = 0; entry < kMaxTotalEntries; entry++) {
        int result = 0;
        mMBC->get(&entry, sizeof(entry), &result, sizeof(result));
        if (std::find(accessedEntries.begin(), accessedEntries.end(), entry) !=
            accessedEntries.end()) {
            ASSERT_EQ(result, entry);
        } else if (entry >= (evictionLimit + accessedEntries.size())) {
            ASSERT_EQ(result, entry);
        } else {
            ASSERT_EQ(result, 0);
        }
    }
}

bool MultifileBlobCacheTest::clearCache() {
    std::string cachePath = &mTempFile->path[0];
    std::string multifileDirName = cachePath + ".multifile";

    DIR* dir = opendir(multifileDirName.c_str());
    if (dir == nullptr) {
        printf("Error opening directory: %s\n", multifileDirName.c_str());
        return false;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // Skip "." and ".." entries
        if (std::string(entry->d_name) == "." || std::string(entry->d_name) == "..") {
            continue;
        }

        std::string entryPath = multifileDirName + "/" + entry->d_name;

        // Delete the entry (we assert it's a file, nothing nested here)
        if (unlink(entryPath.c_str()) != 0) {
            printf("Error deleting file: %s\n", entryPath.c_str());
            closedir(dir);
            return false;
        }
    }

    closedir(dir);

    // Delete the empty directory itself
    if (rmdir(multifileDirName.c_str()) != 0) {
        printf("Error deleting directory %s, error %s\n", multifileDirName.c_str(),
               std::strerror(errno));
        return false;
    }

    return true;
}

// Recover from lost cache in the case of app clearing it
TEST_F(MultifileBlobCacheTest, RecoverFromLostCache) {
    if (!flags::multifile_blobcache_advanced_usage()) {
        GTEST_SKIP() << "Skipping test that requires multifile_blobcache_advanced_usage flag";
    }

    int entry = 0;
    int result = 0;

    uint32_t kEntryCount = 10;

    // Add some entries
    for (entry = 0; entry < kEntryCount; entry++) {
        mMBC->set(&entry, sizeof(entry), &entry, sizeof(entry));
        ASSERT_EQ(sizeof(entry), mMBC->get(&entry, sizeof(entry), &result, sizeof(result)));
        ASSERT_EQ(entry, result);
    }

    // For testing, wait until the entries have completed writing
    mMBC->finish();

    // Manually delete the cache!
    ASSERT_TRUE(clearCache());

    // Cache should not contain any entries
    for (entry = 0; entry < kEntryCount; entry++) {
        ASSERT_EQ(size_t(0), mMBC->get(&entry, sizeof(entry), &result, sizeof(result)));
    }

    // Ensure we can still add new ones
    for (entry = kEntryCount; entry < kEntryCount * 2; entry++) {
        mMBC->set(&entry, sizeof(entry), &entry, sizeof(entry));
        ASSERT_EQ(sizeof(entry), mMBC->get(&entry, sizeof(entry), &result, sizeof(result)));
        ASSERT_EQ(entry, result);
    }

    // Close the cache so everything writes out
    mMBC->finish();
    mMBC.reset();

    // Open the cache again
    mMBC.reset(new MultifileBlobCache(kMaxKeySize, kMaxValueSize, kMaxTotalSize, kMaxTotalEntries,
                                      &mTempFile->path[0]));

    // Before fixes, writing the second entries to disk should have failed due to missing
    // cache dir.  But now they should have survived our shutdown above.
    for (entry = kEntryCount; entry < kEntryCount * 2; entry++) {
        ASSERT_EQ(sizeof(entry), mMBC->get(&entry, sizeof(entry), &result, sizeof(result)));
        ASSERT_EQ(entry, result);
    }
}

// Ensure cache eviction succeeds if the cache is deleted
TEST_F(MultifileBlobCacheTest, EvictAfterLostCache) {
    if (!flags::multifile_blobcache_advanced_usage()) {
        GTEST_SKIP() << "Skipping test that requires multifile_blobcache_advanced_usage flag";
    }

    int entry = 0;
    int result = 0;

    uint32_t kEntryCount = 10;

    // Add some entries
    for (entry = 0; entry < kEntryCount; entry++) {
        mMBC->set(&entry, sizeof(entry), &entry, sizeof(entry));
        ASSERT_EQ(sizeof(entry), mMBC->get(&entry, sizeof(entry), &result, sizeof(result)));
        ASSERT_EQ(entry, result);
    }

    // For testing, wait until the entries have completed writing
    mMBC->finish();

    // Manually delete the cache!
    ASSERT_TRUE(clearCache());

    // Now start adding entries to trigger eviction, cache should survive
    for (entry = kEntryCount; entry < 2 * kMaxTotalEntries; entry++) {
        mMBC->set(&entry, sizeof(entry), &entry, sizeof(entry));
        ASSERT_EQ(sizeof(entry), mMBC->get(&entry, sizeof(entry), &result, sizeof(result)));
        ASSERT_EQ(entry, result);
    }

    // We should have triggered multiple evictions above and remain at or below the
    // max amount of entries
    ASSERT_LE(getCacheEntries().size(), kMaxTotalEntries);
}

// Remove from cache when size is zero
TEST_F(MultifileBlobCacheTest, ZeroSizeRemovesEntry) {
    if (!flags::multifile_blobcache_advanced_usage()) {
        GTEST_SKIP() << "Skipping test that requires multifile_blobcache_advanced_usage flag";
    }

    // Put some entries in
    int entry = 0;
    int result = 0;

    uint32_t kEntryCount = 20;

    // Add some entries
    for (entry = 0; entry < kEntryCount; entry++) {
        mMBC->set(&entry, sizeof(entry), &entry, sizeof(entry));
        ASSERT_EQ(sizeof(entry), mMBC->get(&entry, sizeof(entry), &result, sizeof(result)));
        ASSERT_EQ(entry, result);
    }

    // Send some of them again with size zero
    std::vector<int> removedEntries = {5, 10, 18};
    for (int i = 0; i < removedEntries.size(); i++) {
        entry = removedEntries[i];
        mMBC->set(&entry, sizeof(entry), nullptr, 0);
    }

    // Ensure they do not get a hit
    for (int i = 0; i < removedEntries.size(); i++) {
        entry = removedEntries[i];
        ASSERT_EQ(size_t(0), mMBC->get(&entry, sizeof(entry), &result, sizeof(result)));
    }

    // And have been removed from disk
    std::vector<std::string> diskEntries = getCacheEntries();
    ASSERT_EQ(diskEntries.size(), kEntryCount - removedEntries.size());
    for (int i = 0; i < removedEntries.size(); i++) {
        entry = removedEntries[i];
        // Generate a hash for our removed entries and ensure they are not contained
        // Note our entry and key and the same here, so we're hashing the key just like
        // the multifile blobcache does.
        uint32_t entryHash =
                android::JenkinsHashMixBytes(0, reinterpret_cast<uint8_t*>(&entry), sizeof(entry));
        ASSERT_EQ(std::find(diskEntries.begin(), diskEntries.end(), std::to_string(entryHash)),
                  diskEntries.end());
    }

    // Ensure the others are still present
    for (entry = 0; entry < kEntryCount; entry++) {
        if (std::find(removedEntries.begin(), removedEntries.end(), entry) ==
            removedEntries.end()) {
            ASSERT_EQ(sizeof(entry), mMBC->get(&entry, sizeof(entry), &result, sizeof(result)));
            ASSERT_EQ(result, entry);
        }
    }
}

} // namespace android
