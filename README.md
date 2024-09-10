# refsi_huca

## How to build

```bash
git submodule update --init

ln -s $PWD/CMakePresets.json $PWD/oneapi-construction-kit/CMakePresets.json
ln -s $PWD/CMakeUserPresets.json $PWD/oneapi-construction-kit/CMakeUserPresets.json
```

```bash
cmake --preset linux -Soneapi-construction-kit -Bbuild
# cmake --preset osx -Soneapi-construction-kit -Bbuild

cmake --build build -t all
```

test

```bash
muxc --list-devices
```
