; Variable Decl: fp, p_level=1
; Result type: base=15, p=1
; Generated LLVM IR
target triple = "arm64-apple-darwin"


declare i32 @printf(i8*, ...)

; Type to string: base=4, p=0
define i32 @add(; Type to string: base=4, p=0
i32 %p0, ; Type to string: base=4, p=0
i32 %p1) {
; Type to string: base=4, p=0
  %a = alloca i32
  store i32 %p0, i32* %a
; Type to string: base=4, p=0
  %b = alloca i32
  store i32 %p1, i32* %b
; Type to string: base=4, p=0
  %r0 = load i32, i32* %a
; Type to string: base=4, p=0
  %r1 = load i32, i32* %b
; Type to string: base=4, p=0
  %r2 = add i32 %r0, %r1
; Type to string: base=4, p=0
  ret i32 %r2
}

; Type to string: base=4, p=0
define i32 @main() {
; Type to string: base=15, p=1
; Type to string: base=4, p=0
; Type to string: base=4, p=0
; Type to string: base=4, p=0
  %fp = alloca i32 (i32, i32)*
  store i32 (i32, i32)* @add, i32 (i32, i32)** %fp
; Type to string: base=15, p=1
; Type to string: base=4, p=0
; Type to string: base=4, p=0
; Type to string: base=4, p=0
  %r3 = load i32 (i32, i32)*, i32 (i32, i32)** %fp
; Type to string: base=4, p=0
  %r4 = call i32 %r3(; Type to string: base=4, p=0
i32 1, ; Type to string: base=4, p=0
i32 2)
; Type to string: base=4, p=0
  ret i32 %r4
}

