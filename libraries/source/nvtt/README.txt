This is NVTT 2.1.1 from https://github.com/castano/nvidia-texture-tools
plus some patches (see patches/):
  cmake.patch - disables some dependencies
  cmake-freebsd.patch (fixes build on FreeBSD)
  issue188.patch (fixes http://code.google.com/p/nvidia-texture-tools/issues/detail?id=188)
  issue261.patch (fixes https://github.com/castano/nvidia-texture-tools/issues/261)
  pr270.patch (from https://github.com/castano/nvidia-texture-tools/pull/270)
  rpath.patch (fixes .so file search paths for bundled copy)
  win-shared-build.patch (adapted from https://github.com/castano/nvidia-texture-tools/pull/285)
  musl-build.patch (fixes build on musl linux; contributed by voroskoi, with a part by leper, see https://code.wildfiregames.com/D2491)
  arm-build.patch (fixes build on non-aarch64 arm, includes a line from https://github.com/castano/nvidia-texture-tools/pull/309 by leper)
