# Minimal Perfetto Setup & Commit Guide for updating protos stored in external/perfetto

_As of 2025-08-19_

This is a subset of go/perfetto-github-instructions

Below is the minimal set of commands to clone, build, modify, and contribute to the Perfetto repository.


```bash
git clone https://github.com/google/perfetto
cd perfetto
tools/install-build-deps
tools/setup_all_configs.py
tools/ninja -C out/linux_clang_debug

# Update proto files
#     protos/perfetto/trace/android/surfaceflinger_common.proto
#     protos/perfetto/trace/android/surfaceflinger_transactions.proto
#     protos/perfetto/trace/android/surfaceflinger_layers.proto


# Generate stuff
tools/gen_all out/linux_clang_debug

# Make commit with generated files
git remote set-url origin git@github.com:google/perfetto.git
git push origin HEAD:dev/$USER/branchname
```

You can mirror these changes in your android repo like a caveman as follows
```bash
# From the standalone perfetto repo
git diff HEAD^ HEAD > ~/foo.patch

# From the Android perfetto repo
git apply --reject ~/foo.patch

# Build the perfetto repo
mmm
```