# dummyscheme

A portable, embeddable Scheme implementation based on a register-oriented bytecode vm

# Thoughts

I want to have a scheme implementation that's like lua, a vm register and bytecode-based with line level debug info

Easily could be embed to many host programs

To embed dummyscheme to a host program, just copy source files under src, and write some plugin methods to register them to dummyscheme'vm easily as lua

## Features

- Bytecode compiler with register-based instruction set
- tail-call optimization
- First-class continuations (`call/cc`)
(multi-short, unlimited, but delimited ones will be supported in the near future too)
- Flattened upvalue design for closures, inspired by Lua
- Stack segment technique for continuation capture and chain walking, inspired by Chez Scheme
- Two-halves incremental generational GC (partially implemented)
- `syntax-rules` hygienic macros
- Bignum arithmetic support
- Line-level debug info with source location tracking
- Lua-like embeddability and portability: platform-independent, easy to integrate into C/C++ host applications
(windows, linux, mac, ios, android, any platform where there is a c99/c++98 compiliance compiler)
- Full tests passed with r4rstest.scm
If you have r4rstest.scm, then open the macro SCHEME_STD_R4RS=1, compile the program, then run

```
./build/vmr4 init.scm r4rstest.scm
```

Even the optional

```
(test-cont) (test-sc4) (test-delay)
```

is supported too

## Conformance

- **Current**: R5RS(Working in progress[186])
- **Planned**: R6RS

## Building

```bash
cmake -B build
cmake --build build
```

The executable `vm` will be placed in the `build/` directory.

## Usage

Run Scheme files by passing them as arguments:

```bash
./build/vm init.scm your_script.scm
```

`init.scm` loads the standard macro definitions and is required before running most Scheme code.

`init.scm`'s content could be put into vm.cpp as a const char xxx[] like many other scheme implementations(s7)

## License

MIT
