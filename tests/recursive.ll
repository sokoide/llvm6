; Generated LLVM IR
target triple = "x86_64-unknown-linux-gnu"

; Runtime function declarations
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)

define i32 @factorial(i32 %n) {
  ; if statement
  %2 = icmp sle i32 %n, 1
  %1 = zext i1 %2 to i32
  br i1 %1, label %bb1, label %bb2
  bb1:
  ret i32 1
  br label %bb3
  bb2:
  br label %bb3
  bb3:
  %3 = sub i32 %n, 1
