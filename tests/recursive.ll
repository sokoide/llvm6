; Generated LLVM IR
target triple = "x86_64-unknown-linux-gnu"

; Runtime function declarations
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)

define i32 @factorial(i32 %n) {
  ; if statement
  %1 = icmp sle i32 %n, 1
  %2 = zext i1 %1 to i32
  br i1 %2, label %bb1, label %bb2
  bb1:
  ret i32 1
  br label %bb3
  bb2:
  br label %bb3
  bb3:
  %3 = sub i32 %n, 1
  %4 = call i32 @factorial(i32 %3)
  %5 = mul i32 %n, %4
  ret i32 %5
  }

define i32 @main() {
  %6 = call i32 @factorial(i32 5)
  ret i32 %6
  }


