; Generated LLVM IR
target triple = "x86_64-unknown-linux-gnu"

; Runtime function declarations
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)

define i32 @add(i32 %a, i32 %b) {
  %1 = add i32 %a, %b
  ret i32 %1
  }

