# dummyscheme

[English](README.md) · [中文](README_cn.md)

レジスタ指向のバイトコード VM に基づく、ポータブルで組み込み可能な Scheme 実装

[オンラインで試す](https://zhuzhonghua.github.io/dummyscheme/repl.html)

Scheme コードのテストは、プロジェクト内の r4rstest.scm、r5rs-tests.scm、aftertest.scm を参照してください。
aftertest.scm（449 項目のチェック）はさらに pcall/xpcall/error、コルーチンの多値、および下記の言語層拡張をカバーします。

# 背景

私はゲームプログラミング、特にバックエンドで長年の実務経験があります。
Lua はゲーム業界や他の多くの分野で非常に人気があります。
SICP が大好きで、「The Roots Of Lisp」(https://paulgraham.com/rootsoflisp.html) と「ハッカーと画家」を読みました。
Lisp は高く評価されています。

Lua のような Scheme 実装が欲しいと考えました。行レベルのデバッグ情報を持つ、レジスタ + バイトコード方式の VM です。

いくつかの .h/.cpp ファイルをコピーして一緒にコンパイルし、stub を書くだけで、多くのホストプログラムに簡単に組み込めます。

# 機能

- レジスタベース命令セットのバイトコードコンパイラ
- 末尾呼び出し最適化
- 第一級継続（`call/cc`）、マルチショット、無制限。callframe のコピーオンライト方式を使用。区切り継続も近い将来サポート予定
- フラットな box value（チェーンなし）：内側の lambda にキャプチャされた場合、スタック上の値をボックス化
- 継続キャプチャとチェーン走査のためのスタックセグメント技法（Chez Scheme に触発）
- 2 段階のインクリメンタル世代別 GC（部分的に実装、現在は未使用）
- `syntax-rules` 衛生的マクロ、カスタム省略記号もサポート
- 数タワー、bignum もサポート
- ソース位置追跡付きの行レベルデバッグ情報
- Lua ライクな Scheme 層デバッグ API（debug ライブラリ）：
```
debug-getinfo <level-or-proc>       → alist（what/source/short_src/currentline/
                                        linedefined/lastlinedefined/nups/nparams/
                                        isvararg/istailcall/name/namewhat/
                                        func/activelines）
debug-getlocal <level> <index>      → 名前 + 値（多値返却）または #f
debug-setlocal <level> <index> <v>  → 名前または #f
debug-getoutervalue <proc> <idx>    → 名前 + 値（outer は 1 始まり）または #f
debug-setoutervalue <proc> <idx> <v>→ 名前または #f
debug-outervalueid <proc> <idx>     → 不透明な id；共有 box は必ず同 id
debug-outervaluejoin <p1> <i1> <p2> <i2>  → #t；join 後は双方向 write-through
debug-sethook <spec> <count> <proc> → hook 設定；spec = 'call/'return/'line/'count、
                                        count は N 命令ごとに触发
debug-gethook                       → (spec count proc) または (#f ())
debug-getregistry                   → GC をまたいで生存する共有 hash-table
debug-traceback [<msg>]             → 複数行の traceback 文字列
```
イベントマスクは Lua と同じ（`call`/`return`/`line`/`count`）、hook クロージャは
1 スロットのみ。C 層のブレークポイントテーブルはなく、ブレークポイントは
Scheme 側で `debug-sethook` を使って実装する。
- 組み込み性と可搬性：プラットフォーム非依存、C/C++ ホストアプリケーションへの統合が容易
- r4rstest（SCHEME_STD_R4RS=1）と r5rs-tests で全テスト合格
- dynamic-wind サポート、call/cc とネストし、dynamic-wind の thunk/body の内外へのジャンプを許可
- values + call-with-values

# 追加サポート

- transcript-on / transcript-off なし
- eval はないが vm->evalstr あり
- hash-table
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
各コルーチンは専用の VM を所有します（独立したフレームチェーン + dynamic-wind チェーン + StkVar）。
resume/yield/終了で VM の現在の実行コンテキストを切り替えます（1 コルーチンにつき 1 VM）。
`yield`/`resume` は多値をサポート：resume は複数の値をコルーチンに渡せ（yield が多値として受信）、
yield は複数の値を生成でき（resume が返す）、`call-with-values` で消費します。

- error / pcall / xpcall
```
`(error msg irritant...)`     -- VM エラーを発生させる。メッセージはテキスト：文字列はそのまま、
                                 他のオブジェクトは write セマンティクスで表示され、スペースで結合
`(pcall thunk)`               -- 0 引数クロージャの保護付き呼び出し。(ok . result) または
                                 (false . err-string) を返す。エラーは決して外に漏れない
`(xpcall thunk handler)`      -- 保護付き呼び出し：成功時は thunk の値を返す。失敗時は
                                 (dynamic-wind の after thunk を実行してから) 巻き戻し、エラー
                                 文字列を handler に渡してその値を返す。handler 内のエラーは伝播する
```

- 言語層拡張：R7RS（bytevector / bitwise / random / format）+ Lua（string-format / string-pack）
```
bytevector（R7RS-small 組み込み、バイナリデータ）:
`bytevector?` / `make-bytevector` / `bytevector` / `bytevector-length` /
`bytevector-u8-ref` / `bytevector-u8-set!` / `bytevector=?` / `bytevector-copy` /
`bytevector-copy!` / `bytevector-append` / `utf8->string` / `string->utf8`

bitwise（SRFI-151 / R7RS-large (scheme bitwise)、64 ビット整数）:
`bitwise-not` / `bitwise-and` / `bitwise-ior` / `bitwise-xor` (and/ior/xor は可変長)
`bitwise-arithmetic-shift` と別名 `ash`（算術シフト、右シフトは符号維持）
`bitwise-arithmetic-shift-left` / `bitwise-arithmetic-shift-right`
`bit-count` / `bit-length` / `bit-set?` / `bit-field` / `first-set-bit`
SRFI-60 別名: lognot / logand / logior / logxor / logcount / integer-length

random（SRFI-27 の名前）:
`(random-integer n)` -- [0, n) の整数; `(random-real)` -- [0, 1] の浮動小数;
`(random-seed n)` -- シード設定; 同じシードで同じ数列を再現

format（R7RS/SRFI-28 構文 + SRFI-159 数値パラメータ）:
`~a` display  `~s` write  `~d` 10 進  `~x` 16 進  `~o` 8 進  `~b` 2 進
`~c` char  `~f` 固定小数  `~e` 指数  `~g` 一般  `~%` 改行  `~~` チルダ
数値ディレクティブはパラメータをサポート: `~08x`（ゼロ埋め幅）、`~,2f`（精度）

string-format（Lua string.format、printf スタイル）:
`%s %d %i %u %o %x %X %c %f %e %E %g %G %%`、フラグ `- + # 0`、
幅/精度（`%08x %03d %.2f %8.2f %-35s`）、`*` なし（Lua も拒否）

string-pack / string-unpack / string-packsize（Lua 構文、strobj ベース、バイナリセーフ）:
エンディアン `<` リトル / `>` ビッグ / `=` ネイティブ; 整数 `b B h H l L j J T i[n] I[n]`;
浮動 `f` 4 バイト / `d` `n` 8 バイト（Lua 'f'/'d'/'n'）; `s[n]` 長さプレフィックス付き文字列（プレフィックスは常にビッグエンディアン）;
`z` NUL 終端文字列; `c[n]` 固定長文字列; `x` パディングバイト; `Xop` アラインメント
`(string-pack fmt . args)`          -- バイナリ文字列
`(string-unpack fmt s [pos])`       -- 多値 (vals... nextpos); pos は負値可
`(string-packsize fmt)`             -- 固定サイズ（s/z/c はエラー）
`(string-char . bytes)`             -- Lua string.char
`(string-byte s [i [j]])`           -- Lua string.byte（複数のバイト値）
`(type x)`                          -- Lua type; 型シンボルを返す (number/string/boolean/
                                        char/symbol/pair/null/vector/hash-table/bytevector/
                                        closure/values/void)
インデックス/位置は 0 ベース（負値は末尾から数える）
```

# 使い方

0. プロジェクトに *.cpp/h をコピー（main.cpp は除く）
1. ヘッダをインクルード（vm.h の末尾で scmapi.h もインクルードされる）
```
#include "vm.h"
using namespace Scheme;
```
2. VM の作成 / 破棄 — マルチインスタンス。呼び出しごとに独立した環境
（それぞれのグローバル環境、intern テーブル、GC、registry）を作成します。
値は VM 間で共有してはいけません
```
VM* vm = scm_create_vm();   // デフォルトアロケータ（malloc/realloc）、ud = NULL
// ... 作業 ...
scm_destroy_vm(vm);         // すべての実行時オブジェクトと VM 自体を解放
```
カスタムアロケータ — realloc セマンティクス + 不透明な ud コンテキスト
（例: VM 単位のメモリ計上やクォータ制限）
```
static void * myalloc(void *ud, void *ptr, size_t osize, size_t nsize) {
  (void)ud;
  if (nsize == 0) {
    myfree(ptr);
    return NULL;
  }
  return myrealloc(ptr, nsize);  // osize = 旧サイズ、計上用
}
VM* vm = scm_create_vm(myalloc, NULL);
```
3. ファイルのロード / 評価
```
vm->loadfile("filename.scm");
ValueT val = vm->evalstr("(+ 1 2)");
int iv = scm_get_int(vm, &val);   // iv = 3
```
4. stub を書く
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
5. Scheme 側から stub を呼ぶ
```
(my-add 2 3)
(my-sum 1 2 3 4)
```
6. VM 単位のグローバル registry（GC をまたいで生存。C バインディングが値を保持できる）
```
scm_registry_set(vm, "config", scm_make_string(vm, "v1"));
ValueT got = scm_registry_get(vm, "config");   // キーなし -> null
scm_registry_unset(vm, "config");
```
7. 値 → 文字列（write セマンティクス。ポインタは次の scm_tostring 呼び出しまで有効）
```
int len;
const char* s = scm_tostring(vm, &val, &len);  // 例: "42" / "\"hi\""
```
8. 型名（静的文字列、GC されない）
```
scm_typename(vm, &val);   // "number" / "string" / "pair" / "hash-table" / ...
```
9. C/C++ から Scheme の lambda を呼ぶ
```
ValueT* argv[] = {xxx};
ValueT val = vm->call("f", argv, n);       // 非保護呼び出し
bool ok = vm->pcall(&fn, argv, n, &out);   // 保護付き呼び出し: 失敗で false + エラー値
```

# 標準準拠

- デフォルトで R5RS（追加設定なし）
- マクロ SCHEME_STD_R4RS で R4RS（主に SYMBOL の大文字変換）

# 未来

最適化と機能は無限にあります

# ライセンス

MIT
