; Generated LLVM IR
target triple = "arm64-apple-darwin"

; Runtime function declarations

define i32 @main() {
  %2 = call i32 (i8*, ...) @printf(i8* @1, i32 42)
  ret i32 0
  }


; Global constants
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)
@1 = private unnamed_addr constant [10 x i8] c"val = %d\0A\00"
