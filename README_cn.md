# dummyscheme

[English](README.md) · [日本語](README_ja.md)

一个基于寄存器式字节码虚拟机的、可移植、可嵌入的 Scheme 实现。

[在线体验](https://zhuzhonghua.github.io/dummyscheme/repl.html)

想测试 Scheme 代码，可以参考项目里的 r4rstest.scm、r5rs-tests.scm、aftertest.scm。
aftertest.scm（449 项检查）另覆盖 pcall/xpcall/error、协程多值、以及下文语言层扩展。

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
- 类 Lua 的 Scheme 层调试接口（debug 库）：
```
debug-getinfo <level-or-proc>       → alist（what/source/short_src/currentline/
                                        linedefined/lastlinedefined/nups/nparams/
                                        isvararg/istailcall/name/namewhat/
                                        func/activelines）
debug-getlocal <level> <index>      → 名字 + 值（多值返回）或 #f
debug-setlocal <level> <index> <v>  → 名字或 #f
debug-getoutervalue <proc> <idx>    → 名字 + 值（outer 从 1 开始）或 #f
debug-setoutervalue <proc> <idx> <v>→ 名字或 #f
debug-outervalueid <proc> <idx>     → 不透明 id；共享 box 一定同 id
debug-outervaluejoin <p1> <i1> <p2> <i2>  → #t；join 后两侧写穿
debug-sethook <spec> <count> <proc> → 设 hook；spec = 'call/'return/'line/'count，
                                        count 每 N 条指令触发一次
debug-gethook                       → (spec count proc) 或 (#f ())
debug-getregistry                   → 跨 GC 存活的共享 hash-table
debug-traceback [<msg>]             → 多行 traceback 字符串
```
事件触发位与 Lua 对齐（`call`/`return`/`line`/`count`），单槽位 hook
闭包——没有 C 层断点表；断点在 Scheme 层基于 `debug-sethook` 自行实现。
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

- coroutine
```
`coroutine-create` / `coroutine-resume` / `coroutine-yield` / `coroutine-status` / `coroutine-running`
```
每个协程一个私有 VM（独立帧链 + dynamic-wind 风链 + StkVar），
resume/yield/结束 = 切换 VM 当前执行上下文（一协程一 VM，对应 Lua 的 lua_State）。
`yield`/`resume` 支持多值：resume 可向协程传入多个值（yield 以多值接收），
yield 可产出多个值（resume 以多值返回），用 `call-with-values` 消费。

- error / pcall / xpcall（受 Lua 启发）
```
`(error msg irritant...)`      -- 抛 VM 错误，消息为文本：字符串直接用，其余按 write 语义转文本，空格连接
`(pcall thunk)`                -- 受保护调用 0 参闭包，返回 (ok . result) 或 (false . err-string)，错误永不逃逸
`(xpcall thunk handler)`       -- 受保护调用：成功返回 thunk 的值；失败先 unwind（含 dynamic-wind after），
                                  再把错误串交给 handler 并返回其值；handler 自身出错向上传播
```

- 语言层扩展：R7RS（bytevector/位运算/random/format）+ Lua（string-format/string-pack）
```
bytevector（R7RS-small 内建，二进制数据）：
`bytevector?` / `make-bytevector` / `bytevector` / `bytevector-length` /
`bytevector-u8-ref` / `bytevector-u8-set!` / `bytevector=?` / `bytevector-copy` /
`bytevector-copy!` / `bytevector-append` / `utf8->string` / `string->utf8`

位运算（SRFI-151 / R7RS-large (scheme bitwise)，64 位整数）：
`bitwise-not` / `bitwise-and` / `bitwise-ior` / `bitwise-xor`（and/ior/xor 多参）
`bitwise-arithmetic-shift` 与别名 `ash`（算术移位，右移保留符号）
`bitwise-arithmetic-shift-left` / `bitwise-arithmetic-shift-right`
`bit-count` / `bit-length` / `bit-set?` / `bit-field` / `first-set-bit`
SRFI-60 别名：lognot / logand / logior / logxor / logcount / integer-length

random（SRFI-27 命名）：
`(random-integer n)` -- [0, n) 的整数；`(random-real)` -- [0, 1) 的浮点；
`(random-seed n)` -- 设置种子，同种子序列可复现

format（R7RS/SRFI-28 语法 + SRFI-159 数值参数）：
`~a` display 语义  `~s` write 语义  `~d` 十进制  `~x` 十六进制  `~o` 八进制
`~b` 二进制  `~c` 字符  `~f` 定点  `~e` 指数  `~g` 通用  `~%` 换行  `~~` 波浪号
数值指令支持参数：`~08x`（零填充宽度）、`~,2f`（小数位数）

string-format（Lua string.format，printf 风格）：
`%s %d %i %u %o %x %X %c %f %e %E %g %G %%`，支持标志 `- + # 0`、
宽度/精度（`%08x %03d %.2f %8.2f %-35s`）、不支持 `*`（Lua 同样拒绝）

string-pack / string-unpack / string-packsize（Lua 语法，底层 strobj、二进制安全）：
端序 `<` 小端 / `>` 大端 / `=` 主机序；整数 `b B h H l L j J T i[n] I[n]`；
浮点 `f` 4 字节单精度 / `d` `n` 8 字节双精度（对齐 Lua 'f'/'d'/'n'）；`s[n]` 长度前缀串（前缀恒大端）；`z` NUL 终止串；`c[n]` 定长串；
`x` 填充字节；`Xop` 对齐
`(string-pack fmt . args)`          -- 二进制字符串
`(string-unpack fmt s [pos])`       -- 多值 (值... 下一位)；pos 可为负（从尾部数）
`(string-packsize fmt)`             -- 固定部分大小（s/z/c 报错）
`(string-char . 字节)`              -- 对应 Lua string.char
`(string-byte s [i [j]])`           -- 对应 Lua string.byte（多值字节）
`(type x)`                          -- 对应 Lua type；返回类型 symbol（number/string/boolean/
                                        char/symbol/pair/null/vector/hash-table/bytevector/
                                        closure/values/void）
下标/位置均为 0 起始（负值从尾部数）
```

## 怎么用

0. 把 *.cpp/h 拷到你的工程里，除了 main.cpp
1. 引入头文件（vm.h 末尾已包含 scmapi.h 声明）
```
#include "vm.h"
using namespace Scheme;
```
2. 创建/销毁 VM — 多实例，每次调用创建独立环境
（各自的全局环境、intern 表、GC、registry）；值不得跨 VM 共享
```
VM* vm = scm_create_vm();   // 默认分配器（malloc/realloc），ud = NULL
// ... 干活 ...
scm_destroy_vm(vm);         // 释放全部运行期对象和 VM 本身
```
自定义分配器 — realloc 语义 + 不透明 ud 上下文（例如用于
按 VM 内存记账或配额限制）
```
static void * myalloc(void *ud, void *ptr, size_t osize, size_t nsize) {
  (void)ud;
  if (nsize == 0) {
    myfree(ptr);
    return NULL;
  }
  return myrealloc(ptr, nsize);  // osize = 旧大小，记账用
}
VM* vm = scm_create_vm(myalloc, NULL);
```
3. 加载文件 / 求值
```
vm->loadfile("filename.scm");
ValueT val = vm->evalstr("(+ 1 2)");
int iv = scm_get_int(vm, &val);   // iv = 3
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
6. 每-VM 全局注册表（跨 GC 存活，C 绑定可借此让值保持存活）
```
scm_registry_set(vm, "config", scm_make_string(vm, "v1"));
ValueT got = scm_registry_get(vm, "config");   // 缺键 -> null
scm_registry_unset(vm, "config");
```
7. 值转字符串（write 语义；指针只在下一次 scm_tostring 调用前有效）
```
int len;
const char* s = scm_tostring(vm, &val, &len);  // 如 "42" / "\"hi\""
```
8. 类型名（静态字符串，永不被 GC）
```
scm_typename(vm, &val);   // "number" / "string" / "pair" / "hash-table" / ...
```
9. 从 C/C++ 调用 lambda
```
ValueT* argv[] = {xxx};
ValueT val = vm->call("f", argv, n);       // 非受保护调用
bool ok = vm->pcall(&fn, argv, n, &out);   // 受保护调用：失败返回 false + 错误值
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
