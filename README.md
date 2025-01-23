# nupu_refsi

This project builds an OpenCL implementation with RISC-V backend based on oneAPI Construction Kit. Key features include:

- Support for RISC-V 64-bit instruction set
- RefSi M1 device emulator support

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
# Linux
cmake --preset linux -Soneapi-construction-kit -Bbuild
# macOS
# cmake --preset osx -Soneapi-construction-kit -Bbuild

cmake --build build -t all
```

Build with customized platform name

```bash
cmake --preset linux_nu -DCA_CL_PLATFORM_NAME=NupuOpenCL -Soneapi-construction-kit -Bbuild
cmake --build build -t all
```

test

```bash
build/bin/muxc --list-devices

# ComputeAorta x86_64
# ComputeAorta riscv64
# RefSi M1
# RefSi M1
```

test OpenCL example: clVectorAddition

```bash
CA_HAL_DEBUG=1 OCL_ICD_VENDORS=build/share/OpenCL/vendors build/bin/clVectorAddition
# Example ran successfully, exiting
```

example to run with multiple devices:

```bash
NUM_NUPU_GPUS=8 OCL_ICD_VENDORS=build/share/OpenCL/vendors build/bin/cl_nupu_multi_devices
```
