# refsi_huca

## How to build

```bash
git submodule update --init

ln -s $PWD/CMakePresets.json $PWD/oneapi-construction-kit/CMakePresets.json

# optional
ln -s $PWD/CMakeUserPresets.json $PWD/oneapi-construction-kit/CMakeUserPresets.json

# optional
pushd oneapi-construction-kit
git sparse-checkout set --no-cone '/*' '!/examples/refsi'
popd
```

```bash
cmake --preset linux -Soneapi-construction-kit -Bbuild
# cmake --preset osx -Soneapi-construction-kit -Bbuild

cmake --build build -t all
```

test

```bash
./build/bin/muxc --list-devices

# ComputeAorta x86_64
# ComputeAorta riscv64
# RefSi M1
# RefSi M1
```

test OpenCL example: clVectorAddition

```bash
CA_HAL_DEBUG=1 OCL_ICD_FILENAMES=build/lib/libCL.so build/bin/clVectorAddition

# Example ran successfully, exiting
```