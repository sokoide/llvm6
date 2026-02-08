; Generated LLVM IR
target triple = "arm64-apple-darwin"

; Runtime function declarations

define i32 @main() {
  %x = alloca i32
  store i32 42, i32* %x
  %y = alloca i32
  store i32 10, i32* %y
  %result = alloca i32
  %1 = load i32, i32* %x
  %2 = load i32, i32* %y
  %3 = add i32 %1, %2
  store i32 %3, i32* %result
  ret i32 %result
  }


; Global constants
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)
