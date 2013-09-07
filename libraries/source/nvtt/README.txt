This is NVTT 2.0.8-1 from http://code.google.com/p/nvidia-texture-tools/
plus some patches (see patches/):
  r1156.patch (from NVTT SVN r1156 - fixes build with libtiff 4.0)
  r1157.patch (from NVTT SVN r1157 - fixes build with CUDA 3.0)
  r1172.patch (from NVTT SVN r1172 - fixes memory allocator interaction with Valgrind)
  r907.patch and r1025.patch (from NVTT SVN - fixes build on FreeBSD)
  rpath.patch (fixes .so file search paths for bundled copy)
  issue139.patch (fixes http://code.google.com/p/nvidia-texture-tools/issues/detail?id=139)
  issue176.patch (partially from http:/code.google.com/p/nvidia-texture-tools/issues/detail?id=176 - fixes build on OpenBSD)
  png-api.patch (partially from NVTT SVN r1248 - fixes build with libpng 1.5)
  cmake-freebsd.patch (fixes build on FreeBSD)
  gcc47-unistd.patch (fixes build on GCC 4.7)
  cmake-devflags.patch (from https://407191.bugs.gentoo.org/attachment.cgi?id=308589 - allows disabling various dependencies)
  cmake-devflags2.patch - allows disabling more dependencies
  issue182.patch (fixes http://code.google.com/p/nvidia-texture-tools/issues/detail?id=182)
  cmake-noqt4.patch (removes unused dependency on Qt4, fixes build on systems without Qt)
  arm-fix.patch (from NVTT SVN r1173 - fixes ARM build)
  issue188.patch (fixes http://code.google.com/p/nvidia-texture-tools/issues/detail?id=188)
