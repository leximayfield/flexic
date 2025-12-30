FlexiC
======
A tiny, portable, zero-dependency FlexBuffer reader/writer for C/C++.

**WARNING:** This library is still a WIP and has not quite reached API
stability yet.  That said, it's still in a very usable state if you are
willing to put up with the odd usability paper-cut and API breakage.

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

Why FlexiC?
-----------
- A "cursor" API for navigating a FlexBuffer message by hand.
- A SAX-style "parser" API for unmarshalling an entire FlexBuffer at once.
- A stack-based "writer" API for writing a FlexBuffer message.
- A utility function for converting a FlexBuffer to JSON.
- The entire API surface is zero-allocation.
    - The stack implementation used by the stack-based writer is provided
      by the user of the library.
    - You can either provide a stack of fixed size, or a dynamic
      implementation which reallocates if necessary.
    - There is an opt-in use of `strdup`/`free` for keeping keys alive while
      writing maps.  Not only is this optional, but one can supply their own
      implementation of these functions.
- Hardened against stack overflows and DOS via maliciously crafted messages.
    - The default maximum recursion depth is 32, while the default maximum
      number of iterables per message is 2048.
    - Both values can be configured at compile-time.
- Implementation is contained in a single C source file and header pair.
    - Can be dropped directly into your source tree or linked as a static
      library.

### Why not FlexiC?
There are some trade-offs that this library makes over Google's implementation.

- The Reader and Writer APIs do not share a unified "Ref" abstraction.
  - This maps well to my own uses of serialization libraries, where data
    is converted to native structs and back quickly, and little time is spent
    in any intermediate representation.
  - If you want to mutate any arbitrary FlexBuffer in-place, this library
    will not work for you unless you write your own immediate representation.
- C is not a memory-safe language, and this library can be used without a
  memory allocator.
  - Much of the API can be used with values on the stack.  Care must be taken
    to avoid use-after-scope and use-after-stack bugs.
  - The writer API allows the optional use of `strdup`/`free` to keep keys
    alive until the values they're attached to have been inserted into a map.
    If this functionality is opted out of, the user must keep alive all keys
    used by mapped values until the map itself has been written.
    - If all of your keys are hard-coded, you don't have to worry about this.

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
automatic byte-swapping on BE platforms is a stretch goal if there is a
compelling use case for the feature.

Installation
------------
FlexiC is remarkably self-contained.  You can either copy `flexic.c` and
`flexic.h` into your C or C++ project and use them directly, or you can
statically link to FlexiC as a library using the included `CMakeLists.txt`.

This library is intended to be used by copying or static linking, but you
can also create a shared library if you want to FFI into the library from
a different language.

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
|               92.72 |       10,784,872.94 |    0.4% |      1.20 | `leximayfield/flexic`
|              248.90 |        4,017,674.85 |    0.3% |      1.22 | `google/flatbuffers`
|               85.13 |       11,746,560.90 |    0.7% |      1.17 | `nlohmann/json`
|              122.77 |        8,145,241.76 |    0.7% |      1.21 | `ibireme/yyjson.h`

|               ns/op |                op/s |    err% |     total | Parse and Seek value of root[map-50][key-50]
|--------------------:|--------------------:|--------:|----------:|:---------------------------------------------
|              101.92 |        9,811,239.15 |    0.4% |      1.21 | `leximayfield/flexic`
|              197.37 |        5,066,580.74 |    0.5% |      1.21 | `google/flatbuffers`
|        2,912,677.78 |              343.33 |    1.0% |      1.21 | `nlohmann/json`
|           95,051.71 |           10,520.59 |    0.5% |      1.20 | `ibireme/yyjson.h`

|               ns/op |                op/s |    err% |     total | Walk entire document (strings)
|--------------------:|--------------------:|--------:|----------:|:-------------------------------
|          126,022.04 |            7,935.12 |    0.6% |      1.21 | `leximayfield/flexic`
|          777,551.35 |            1,286.09 |    0.6% |      1.17 | `google/flatbuffers`
|          233,857.48 |            4,276.11 |    0.6% |      1.22 | `nlohmann/json (manual)`
|           23,921.22 |           41,803.89 |    0.9% |      1.20 | `ibireme/yyjson.h`

|               ns/op |                op/s |    err% |     total | Parse and Walk entire document (strings)
|--------------------:|--------------------:|--------:|----------:|:-----------------------------------------
|          124,231.30 |            8,049.50 |    0.9% |      1.22 | `leximayfield/flexic`
|          791,267.55 |            1,263.80 |    0.6% |      1.18 | `google/flatbuffers`
|          684,348.34 |            1,461.24 |    1.1% |      1.17 | `nlohmann/json (sax_parse)`
|          114,945.17 |            8,699.80 |    0.6% |      1.21 | `ibireme/yyjson.h`

|               ns/op |                op/s |    err% |     total | Walk entire document (vec3)
|--------------------:|--------------------:|--------:|----------:|:----------------------------
|           91,419.82 |           10,938.55 |    0.9% |      1.18 | `leximayfield/flexic`
|          582,539.78 |            1,716.62 |    1.5% |      1.22 | `google/flatbuffers`
|          787,313.79 |            1,270.14 |    0.6% |      1.17 | `nlohmann/json (manual)`
|          107,037.09 |            9,342.56 |    0.6% |      1.20 | `ibireme/yyjson.h`

|               ns/op |                op/s |    err% |     total | Parse and Walk entire document (vec3)
|--------------------:|--------------------:|--------:|----------:|:--------------------------------------
|           91,808.40 |           10,892.25 |    0.8% |      1.19 | `leximayfield/flexic`
|          580,568.82 |            1,722.45 |    0.5% |      1.21 | `google/flatbuffers`
|        6,347,117.65 |              157.55 |    0.6% |      1.20 | `nlohmann/json (sax_parse)`
|          632,526.79 |            1,580.96 |    0.5% |      1.22 | `ibireme/yyjson.h`

Compared to `google/flatbuffers`, FlexiC seems to be quicker across the board.

On the other hand, comparisons against JSON libraries is tricky.  There is
parsing overhead to consider, and there are certain data types that JSON lacks
completely, such as typed vectors and binary blobs.  However, JSON libraries
have, on the whole, had a lot of engineering time and effort put into them,
and in cases where there is overlap in the feature set, FlexBuffers are not
necessarily faster than the most hand-optimized JSON libraries like
`ibireme/yyjson.h`.

Hopefully these numbers are informative.  These were compiled and run with
Clang 21.1.5 `-O3` and run on a Ryzen 7 5800X running Windows 11 25H2.

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
