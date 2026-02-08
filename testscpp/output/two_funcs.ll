; Generated LLVM IR
target triple = "arm64-apple-darwin"

; Runtime function declarations

define i32 @add(i32 %a, i32 %b) {
  %1 = add i32 %a, %b
  ret i32 %1
  }

define i32 @main() {
  ret i32 42
  }


; Global constants
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)
