# ARM64/Decky layer port

The Decky ARM64 layer is built from the same v2 source baseline as the owner
fork's x86 release, rather than from the old Android fork's v1 code:

- common v2 baseline: `PancakeTAS/lsfg-vk` `8b0da2661c6f3473a7fccc8ba643880050e71642`
- Decky release/build baseline: `xXJSONDeruloXx/lsfg-vk` `v2.0.0-decky.2`
- ARM reference implementation: `FrankBarretta/lsfg-vk-android` `3e89e5439a98f55d5acb003d20039426ab24e69c`

The ARM port carries the parts of the reference work that apply to the v2
glibc layer:

- explicit initialization of `firstIter` and `firstIterS` in the v2 constant buffer;
- runtime probing and enabling of `VK_EXT_robustness2` null image descriptors;
- a 1×1 sampled/storage fallback image for optional descriptors on drivers that
  do not support null descriptors.

FP16 device-feature support is already implemented in the v2 baseline. The
reference fork's Android AHardwareBuffer path is intentionally not substituted
for the Armada glibc layer: it targets Android/bionic and is a different Vulkan
image-sharing ABI. Armada packages therefore use the glibc-aarch64 target and
the same static `libstdc++`/`libgcc` runtime policy as the x86 Decky bundle.
