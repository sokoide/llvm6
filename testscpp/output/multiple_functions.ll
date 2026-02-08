; Generated LLVM IR
target triple = "arm64-apple-darwin"

; Runtime function declarations

define i32 @square(i32 %n) {
  %1 = mul i32 %n, %n
  ret i32 %1
  }

define i32 @cube(i32 %n) {
  %2 = mul i32 %n, %n
  %3 = mul i32 %2, %n
  ret i32 %3
  }

define i32 @main() {
  %4 = call i32 @square(i32 3)
  %5 = call i32 @cube(i32 2)
  %6 = add i32 %4, %5
  ret i32 %6
  }


; Global constants
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)
