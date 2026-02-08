; Generated LLVM IR
target triple = "arm64-apple-darwin"

; Runtime function declarations

define i32 @main() {
  %x = alloca i32
  store i32 10, i32* %x
  %y = alloca i32
  store i32 20, i32* %y
  %1 = load i32, i32* %x
  %2 = load i32, i32* %y
  %3 = add i32 %1, %2
  ret i32 %3
  }


; Global constants
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)
