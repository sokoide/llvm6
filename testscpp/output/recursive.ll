; Generated LLVM IR
target triple = "arm64-apple-darwin"

; Runtime function declarations

define i32 @factorial(i32 %n) {
  ; if statement
  %1 = icmp sle i32 %n, 1
  %2 = zext i1 %1 to i32
  %3 = icmp ne i32 %2, 0
  br i1 %3, label %bb1, label %bb2
bb1:
  ret i32 1
  br label %bb3
bb2:
  br label %bb3
bb3:
  %4 = sub i32 %n, 1
  %5 = call i32 @factorial(i32 %4)
  %6 = mul i32 %n, %5
  ret i32 %6
  }

define i32 @main() {
  %7 = call i32 @factorial(i32 5)
  ret i32 %7
  }


; Global constants
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)
