# dummyscheme

[English](README.md)

一个基于寄存器式字节码虚拟机的、可移植、可嵌入的 Scheme 实现。

[在线体验](https://zhuzhonghua.github.io/dummyscheme/repl.html)

想测试 Scheme 代码，可以参考项目里的 r4rstest.scm、r5rs-tests.scm、aftertest.scm。

# 起因

我在游戏行业特别是后端做了很多年。Lua 在游戏行业非常流行，在很多其他领域也很流行。
我很喜欢 SICP，也读过 [The Roots Of Lisp](https://paulgraham.com/rootsoflisp.html)
和《Hackers and Painters》这两本书。Lisp 一直备受推崇。

我想做一个像 Lua 一样的 Scheme 实现：虚拟机 + 寄存器 + 字节码，带行级调试信息。

同时它要能方便地嵌入各种宿主程序 —— 只需要把几个 .h/.cpp 文件拷到你的工程里一起编译，再写几个 stub 就行。

## 特性

- 字节码编译器，指令集基于寄存器
- 尾调用优化
- 一等公民延续（`call/cc`），multi-short、不限长度，用 callframe 的 copy-on-write 实现
  （界定延续 delimited continuation 计划在不久的将来支持）
- 扁平的 box value（受 Lua 启发），没有链条；被内层 lambda 捕获时把栈上的值装箱
- 用 stack segment 技术做延续捕获和链遍历，思路来自 Chez Scheme
- 增量分代 GC，两个半区（部分实现，目前未启用）
- `syntax-rules` 卫生宏，也支持自定义省略号
- 数值塔，支持 bignum
- 行级调试信息，带源码位置跟踪
- 类似 Lua 的可嵌入性和可移植性：与平台无关，容易集成进 C/C++ 宿主程序
- r4rstest（加 `SCHEME_STD_R4RS=1` 编译）和 r5rs-tests 全部测试通过
- 支持 dynamic-wind，与 call/cc 嵌套，可以从 dynamic-wind 的 thunk/body 里跳出去；
  允许跳出 before 和 after，但跳进 before 和 after 并不禁止
- values + call-with-values

## 额外支持

- 没有 transcript-on / transcript-off
- 没有 eval，但有 `vm->evalstr`
- hash-table（受 Lua 启发）
```
(make-hash-table)
(hash-table-ref table key)
(hash-table-set! table key val)
(hash-table-for-each (lambda (key val) xxx) table)
```

## 怎么用

0. 把 *.cpp/h 拷到你的工程里，除了 main.cpp
1. 引入头文件
```
#include "vm.h"
using namespace Scheme;
```
2. 初始化 vm

用默认的 malloc/free
```
VM vm;
```
或者用自定义的 malloc/free
```
static void * myalloc(void *ptr, size_t nsize) {
  if (nsize == 0) {
    myfree(ptr);
    return NULL;
  }
  else
    return myrealloc(ptr, nsize);
}
VM vm(myalloc);
```
3. 加载文件
```
vm.loadfile("filename.scm");
```
4. 写 stub
```
// (lambda (a b) xxx)
static ValueT scm_stub_my_add(VM* vm, ValueT* a, ValueT* b)
{
  return scm_make_int(vm, scm_get_int(vm, a) + scm_get_int(vm, b));
}
// (lambda rest xxx)
static ValueT scm_stub_my_sum(VM* vm, ValueT* rest)
{
  scm_int t = 0;
  ValueT* p = rest;
  while (!scm_is_null(vm, p))
  {
    t += scm_get_int(vm, scm_car(vm, p));
    p = scm_cdr(vm, p);
  }
  return scm_make_int(vm, t);
}
const RegCProc myext[] = {
  RegCProc("my-add", scm_stub_my_add), // (lambda (a) xxx)
  RegCProc("my-sum", scm_stub_my_sum, true), // (lambda (a . rest) xxx)
  RegCProc(NULL, -1)
};
regcfunc(vm, myext);

```
5. 在 Scheme 里调用这些 stub
```
(my-add 2 3)
(my-sum 1 2 3 4)
```
6. 从 C/C++ 里求值 Scheme 代码
```
ValueT val = vm->evalstr("(+ 1 2)")
int iv = scm_get_int(vm, &val);
// iv = 3
```
7. 从 C/C++ 调用 lambda
```
ValueT* argv[] = {xxx};
ValueT val = vm->call("f", argv, n)
```

## 在线体验（wasm）

同一套虚拟机也能编译成 WebAssembly 在浏览器里跑。

```
sh web/build.sh          # 生成 web/dist/repl.html
```

需要先装好 emscripten。产物是一个不依赖外部文件的单文件 HTML（wasm 以 base64 内嵌），
所以本地双击打开或者用 HTTP 访问都可以。推送到 `main` 分支时，
GitHub Actions 会自动重新构建并发布到 GitHub Pages，也就是上面那个在线体验的链接。

所有跟浏览器相关的东西都放在 `web/` 目录下，原生的 CMake 构建完全看不到它们：

- `web/scmwasm.cpp` — `EMSCRIPTEN_KEEPALIVE` 桥接层（`scm_wasm_init`、`scm_wasm_eval`、
  `scm_wasm_failed`、`scm_wasm_free`、`scm_wasm_version`）。它把 stderr 重定向到临时文件，
  这样求值结果和错误信息就能作为字符串回传给 JS；因为抛出的值只是失败断言的条件文本，
  没法直接显示，所以用 `scm_wasm_failed` 单独报告上一次求值有没有抛异常。
- `web/shell.html` — REPL 界面。单根滚动转录流，shell 的样子：输入、返回值和活的 `>`
  提示符混在一起流动；输入会一直缓冲到括号配平，所以多行 form 直接按回车就能写完。
- `web/build.sh` — emcc 调用。CI 跑的就是这个脚本。

wasm 版跑的是同一套测试：r5rs-tests 189/189 全过，r4rstest.scm + r4rstestafter.scm
的输出和原生 `vmr4` 逐字节一致。

两个和平台有关的细节：`long` 在 wasm32 上只有 32 位，所以那里 `scm_int`/`scm_uint`
换成 `long long`，以保住 bignum 的 64 位 limb；文件 I/O（`open-input-file`、`load`）
走的是 Emscripten 的 MEMFS，只能看到构建时预置的文件，REPL 本身用不到。

## 标准符合度

- R5RS，不需要任何额外设置（默认）
- R4RS，编译时加宏 `SCHEME_STD_R4RS`（主要影响 SYMBOL 是否转大写）

## 未来

优化和特性都数不尽。

## 许可

MIT
