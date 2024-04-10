# Marcai
A physical music box that calls a API connected to an AI music generator that gets fed different parameters from the box.

# Build

```
exoprt PICO_SDK_PATH=path/to/pico-sdk
mkdir build
cmake -S . -B build
cmake --build build
```

After this, there is a .uf2 file in the build directory that you copy
to the Raspberry Pi Pico.
