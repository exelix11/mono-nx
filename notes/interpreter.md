# Environment

Cross compilation requires the rootfs dir. We simulate that with the devkitpro path

```
export ROOTFS_DIR=$DEVKITPRO
```

# Building

## Interpreter

This is more or less the order i used during the port development.

First i ported the actual mono runtime
```
./build.sh --subset Mono.Runtime --cross -a arm64 --os libnx
```

Then the corelib, this is enough to get the runtime to load something but it will fail without the native libs
```
./build.sh --subset mono.corelib --cross -a arm64 --os libnx
```

With the native libraries in place it's possible to actually run simple apps
```
./build.sh --subset libs.native --cross -a arm64 --os libnx
```

At this point building the rest of the framework should work fine
```
./build.sh --subset libs.sfx --cross -a arm64 --os libnx
```