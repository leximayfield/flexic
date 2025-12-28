FlexiC
======
A tiny, portable, zero-dependency FlexBuffer reader/writer for C/C++.

**WARNING:** This library is still a WIP and has not quite reached API
stability yet.  That said, it's still in a very usable state if you are
willing to put up with the odd usability papercut and API breakage.

Features
--------
- A "cursor" API for navigating a FlexBuffer message by hand.
- A SAX-style "parser" API for unmarshalling an entire FlexBuffer at once.
- A stack-based "writer" API for writing a FlexBuffer message.
- The entire API surface is zero-allocation.
    - The stack implementation used by the stack-based writer is provided
      by the user of the library.
    - You can either provide a stack of fixed size, or a dynamic
      implementation which reallocates if necessary.
- Hardened against stack overflows and DOS via maliciously crafted messages.
    - The default maximum recursion depth is 32, while the default maximum
      number of iterables per message is 2048.
    - Both values can be configured at compile-time.
- Implementation is contained in a single C source file and header pair.
    - Can be dropped directly into your source tree or linked as a static
      library.

What is a FlexBuffer?
---------------------
FlexBuffer is a self-contained binary serialization format created by Google,
similar to MessagePack or CBOR.  It can also be likened to a binary version
of JSON.  If you're curious, you can get a description of the format internals
[here][2].

FlexBuffers are designed to be fast to read and write.  The format keeps
data in formats that are designed to require the least amount of unmarshalling
overhead in the widest number of circumstances.  You can read or write a
FlexBuffer with no memory allocation.

FlexBuffers are not so well suited for streaming.  Values are not
self-contained, and reading a FlexBuffer requires resolving the root type,
which is written last.

FlexBuffers are also not designed for use cases where space is at a premium.
There is a lot of empty space in the wire format, in order to encourage aligned
loads and stores.  That said, empty space does tend to compress well with
compression schemes like zlib or zstd.

Requirements
------------
This library does not require dynamic memory allocation of any kind.  However,
your C compiler and standard library must support the following features:

- `<string.h>`, specifically `memcpy`, `strlen` and `strcmp`.
- `<stdint.h>` fixed-width int types.
- `<stdbool.h>` booleans.
- `//` line-based comments.

Any modern compiler can support these trivially.  Older compilers will probably
work with a `stdint.h` and/or `stdbool.h` shim.

The test suite uses Google Test, and requires a C++17 compiler.

There is currently no support for Big Endian architectures.  Support for
automatic byteswapping on BE platforms is a stretch goal if there is a
compelling use case for the feature.

Installation
------------
FlexiC is remarkably self-contained.  You can either copy `flexic.c` and
`flexic.h` into your C or C++ project and use them directly, or you can
statically link to flexic as a library using the included `CMakeLists.txt`.

I can't imagine why you would want to turn this library into a shared library,
but if you have a use case, let me know in an issue.  FWIW, I do not consider
packaging for a Linux distro to be a compelling use case.

How Fast Is FlexiC?
-------------------
If speed is a serious concern of yours, you should probably construct a
benchmark that mirrors your own expected usage of the library.

That being said, I decided to construct a few benchmarks of my own, just
to get a ballpark idea of how fast this library compares to the official
Google library, as well as two different JSON libraries.  These benchmarks
can be found in the `flexic_bench` target.

|               ns/op |                op/s |    err% |     total | Seek value of root[map-50][key-50]
|--------------------:|--------------------:|--------:|----------:|:-----------------------------------
|               90.01 |       11,110,411.58 |    0.7% |      1.20 | `leximayfield/flexic`
|              247.50 |        4,040,332.27 |    0.2% |      1.21 | `google/flatbuffers`
|               85.35 |       11,717,125.29 |    0.5% |      1.18 | `nlohmann/json`
|              104.70 |        9,551,470.54 |    0.1% |      1.21 | `ibireme/yyjson.h`

|               ns/op |                op/s |    err% |     total | Parse and Seek value of root[map-50][key-50]
|--------------------:|--------------------:|--------:|----------:|:---------------------------------------------
|               99.08 |       10,092,450.76 |    0.5% |      1.19 | `leximayfield/flexic`
|              205.64 |        4,862,818.42 |    0.6% |      1.21 | `google/flatbuffers`
|        2,889,516.22 |              346.08 |    0.8% |      1.21 | `nlohmann/json`
|           97,537.09 |           10,252.51 |    0.7% |      1.19 | `ibireme/yyjson.h`

|               ns/op |                op/s |    err% |     total | Walk entire document
|--------------------:|--------------------:|--------:|----------:|:---------------------
|          128,234.38 |            7,798.22 |    0.7% |      1.21 | `leximayfield/flexic`
|          767,220.00 |            1,303.41 |    0.5% |      1.17 | `google/flatbuffers`
|          230,910.04 |            4,330.69 |    0.6% |      1.21 | `nlohmann/json (manual)`
|           13,615.23 |           73,447.15 |    0.5% |      1.21 | `ibireme/yyjson.h`

|               ns/op |                op/s |    err% |     total | Parse and Walk entire document
|--------------------:|--------------------:|--------:|----------:|:-------------------------------
|          127,260.47 |            7,857.90 |    0.6% |      1.22 | `leximayfield/flexic`
|          774,352.70 |            1,291.40 |    0.5% |      1.17 | `google/flatbuffers`
|          698,647.59 |            1,431.34 |    0.7% |      1.16 | `nlohmann/json (sax_parse)`
|          111,539.57 |            8,965.43 |    0.4% |      1.21 | `ibireme/yyjson.h`

Compared to `google/flatbuffers`, FlexiC seems to be quicker across the board.
Comparisons against JSON libraries is tricky, as `nlohmann/json` often runs
into overhead from parsing, and `ibireme/yyjson.h` is a spectacularly optimized
library.  Nevertheless, hopefully these numbers give you some idea about how
fast FlexiC can go.

Compiled with Clang 21.1.5 `-O3` and run on a Ryzen 7 5800X running Windows
11 25H2.

License
-------
FlexiC is licensed under the [zlib][3] license.

This means that yes, you can use this library in your project, just don't
remove the license from the source code.  If you already itemize the libraries
and licenses in your project, I'd appreciate a shout-out, but unlike the MIT
or BSD licenses I'm not going to legally compel you to do so.

[1]: https://flatbuffers.dev/flexbuffers/
[2]: https://flatbuffers.dev/internals/#flexbuffers
[3]: https://choosealicense.com/licenses/zlib/
[4]: https://github.com/google/flatbuffers
[5]: https://github.com/kanosaki/flexbuffers
