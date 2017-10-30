This premake module adds supports for pkgconfig.

It allows one to use the `pkg-config` command (or an alternative one like
`sdl2-config` to determine the names of libraries to be passed to the linker.

The solution of putting directly the output (like "-Lxml2") into the linker
options creates an inconsistency between libraries using pkgconfig and the
libraries not using it.

We should always use premake's linkoptions to specify global options and
links to specify libraries, in order to avoid ordering problems in the
list of libraries statically linked.
