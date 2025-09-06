; Generated LLVM IR
target triple = "x86_64-unknown-linux-gnu"

; Runtime function declarations
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)

define i32 @main() {
  %1 = call i32 @add(i32 5, i32 3)
  ret i32 %1
  ret i32 0
  }


