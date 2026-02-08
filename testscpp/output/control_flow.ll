; Generated LLVM IR
target triple = "arm64-apple-darwin"

; Runtime function declarations

define i32 @test(i32 %x) {
  ; if statement
  %1 = icmp sgt i32 %x, 0
  %2 = zext i1 %1 to i32
  %3 = icmp ne i32 %2, 0
  br i1 %3, label %bb1, label %bb2
bb1:
  ret i32 1
  br label %bb3
bb2:
  br label %bb3
bb3:
  ret i32 0
  }


; Global constants
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)
