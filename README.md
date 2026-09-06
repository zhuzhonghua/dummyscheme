# dummyscheme

A portable, embeddable Scheme implementation based on a register-oriented bytecode vm

## Features

- Bytecode compiler with register-based instruction set
- tail-call optimization
- First-class continuations (`call/cc`) (currently it's unlimited, but delimited will be supported in the near future)
- Flattened upvalue design for closures, inspired by Lua
- Stack segment technique for continuation capture and chain walking, inspired by Chez Scheme
- Two-halves incremental generational GC (partially implemented)
- `syntax-rules` hygienic macros
- Bignum arithmetic support
- Line-level debug info with source location tracking
- Lua-like embeddability and portability: platform-independent, easy to integrate into C/C++ host applications

## Conformance

- **Current**: R5RS(Working in progress[176])
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
