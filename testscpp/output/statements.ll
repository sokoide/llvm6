; Generated LLVM IR
target triple = "arm64-apple-darwin"

; Runtime function declarations

define i32 @test_statements(i32 %n) {
  %1 = add i32 %n, 1
  %local = alloca i32
  store i32 5, i32* %local
  %2 = load i32, i32* %local
  %4 = add i32 %2, 1
  store i32 %4, i32* %local
  ; if statement
  %5 = icmp sgt i32 %n, 0
  %6 = zext i1 %5 to i32
  %7 = icmp ne i32 %6, 0
  br i1 %7, label %bb1, label %bb2
bb1:
  %8 = add i32 %n, 1
  store i32 %8, i32* %n
  br label %bb3
bb2:
  br label %bb3
bb3:
  ; if statement
  %9 = icmp slt i32 %n, 10
  %10 = zext i1 %9 to i32
  %11 = icmp ne i32 %10, 0
  br i1 %11, label %bb4, label %bb5
bb4:
  %12 = mul i32 %n, 2
  store i32 %12, i32* %n
  br label %bb6
bb5:
  %13 = sdiv i32 %n, 2
  store i32 %13, i32* %n
  br label %bb6
bb6:
  ; while statement
  br label %bb7
bb7:
  %14 = icmp sgt i32 %n, 0
  %15 = zext i1 %14 to i32
  %16 = icmp ne i32 %15, 0
  br i1 %16, label %bb8, label %bb9
bb8:
  %17 = sub i32 %n, 1
  store i32 %17, i32* %n
  ; if statement
  %18 = icmp eq i32 %n, 5
  %19 = zext i1 %18 to i32
  %20 = icmp ne i32 %19, 0
  br i1 %20, label %bb10, label %bb11
bb10:
  br label %bb7
  br label %bb12
bb11:
  br label %bb12
bb12:
  ; if statement
  %21 = icmp eq i32 %n, 1
  %22 = zext i1 %21 to i32
  %23 = icmp ne i32 %22, 0
  br i1 %23, label %bb13, label %bb14
bb13:
  br label %bb9
  br label %bb15
bb14:
  br label %bb15
bb15:
  br label %bb7
  br label %bb7
bb9:
  ; for statement
  store i32 0, i32* %n
  br label %bb16
bb16:
  %24 = icmp slt i32 %n, 5
  %25 = zext i1 %24 to i32
  %26 = icmp ne i32 %25, 0
  br i1 %26, label %bb17, label %bb19
bb17:
  ; if statement
  %27 = icmp eq i32 %n, 3
  %28 = zext i1 %27 to i32
  %29 = icmp ne i32 %28, 0
  br i1 %29, label %bb20, label %bb21
bb20:
  br label %bb19
  br label %bb22
bb21:
  br label %bb22
bb22:
  br label %bb18
bb18:
  %30 = add i32 %n, 1
  store i32 %30, i32* %n
  br label %bb16
bb19:
  ; for statement
  br label %bb23
bb23:
  %31 = icmp slt i32 %n, 10
  %32 = zext i1 %31 to i32
  %33 = icmp ne i32 %32, 0
  br i1 %33, label %bb24, label %bb26
bb24:
  %34 = add i32 %n, 1
  store i32 %34, i32* %n
  br label %bb25
bb25:
  br label %bb23
bb26:
  ; if statement
  %35 = icmp eq i32 %n, 0
  %36 = zext i1 %35 to i32
  %37 = icmp ne i32 %36, 0
  br i1 %37, label %bb27, label %bb28
bb27:
  ret i32 42
  br label %bb29
bb28:
  br label %bb29
bb29:
  ret i32 %n
  }

define i32 @test_no_params() {
  ret i32 123
  }


; Global constants
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)
