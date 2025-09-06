; Generated LLVM IR
target triple = "x86_64-unknown-linux-gnu"

; Runtime function declarations
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)

define i32 @main() {
  %1 = mul i32 3, 2
  %2 = add i32 5, %1
  %3 = sub i32 %2, 1
  ret i32 %3
  }


