; Generated LLVM IR
target triple = "x86_64-unknown-linux-gnu"

; Runtime function declarations
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)

define i32 @square(i32 %n) {
  %1 = mul i32 %n, %n
