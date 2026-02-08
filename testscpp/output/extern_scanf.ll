; Generated LLVM IR
target triple = "arm64-apple-darwin"

; Runtime function declarations

define i32 @main() {
  %x = alloca i32
  %r = alloca i32
  store i32 0, i32* %x
  %3 = call i32 (i8*, ...) @scanf(i8* @1, i32* %x)
  store i32 %3, i32* %r
  %5 = load i32, i32* %r
  %6 = load i32, i32* %x
  %7 = call i32 (i8*, ...) @printf(i8* @4, i32 %5, i32 %6)
  ret i32 0
  }


; Global constants
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)
@1 = private unnamed_addr constant [3 x i8] c"%d\00"
@4 = private unnamed_addr constant [16 x i8] c"r = %d, x = %d\0A\00"
