; Generated LLVM IR
target triple = "arm64-apple-darwin"

; Runtime function declarations

define i32 @modern_func(i32 %a, i32 %b, i32 %c) {
  %1 = add i32 %a, %b
  %2 = add i32 %1, %c
  ret i32 %2
  }

define i32 @old_style_func(i32 %x, i32 %y) {
  %3 = mul i32 %x, %y
  ret i32 %3
  }

define i32 @no_params_modern() {
  ret i32 100
  }

define i32 @no_params_old() {
  ret i32 200
  }

define i32 @caller() {
  %a = alloca i32
  %4 = call i32 @modern_func(i32 1, i32 2, i32 3)
  store i32 %4, i32* %a
  %b = alloca i32
  %5 = call i32 @old_style_func(i32 4, i32 5)
  store i32 %5, i32* %b
  %c = alloca i32
  %6 = call i32 @no_params_modern()
  store i32 %6, i32* %c
  %d = alloca i32
  %7 = call i32 @no_params_old()
  store i32 %7, i32* %d
  %8 = add i32 %a, %b
  %9 = add i32 %8, %c
  %10 = load i32, i32* %d
  %11 = add i32 %9, %10
  ret i32 %11
  }

define i32 @fibonacci(i32 %n) {
  ; if statement
  %12 = icmp sle i32 %n, 1
  %13 = zext i1 %12 to i32
  %14 = icmp ne i32 %13, 0
  br i1 %14, label %bb1, label %bb2
bb1:
  ret i32 %n
  br label %bb3
bb2:
  br label %bb3
bb3:
  %15 = sub i32 %n, 1
  %16 = call i32 @fibonacci(i32 %15)
  %17 = sub i32 %n, 2
  %18 = call i32 @fibonacci(i32 %17)
  %19 = add i32 %16, %18
  ret i32 %19
  }

define i32 @complex_math(i32 %x) {
  %result = alloca i32
  %20 = mul i32 %x, 2
  %21 = add i32 %20, 3
  %22 = sub i32 %x, 1
  %23 = mul i32 %21, %22
  store i32 %23, i32* %result
  %24 = add i32 %x, 1
  %25 = load i32, i32* %result
  %26 = sdiv i32 %25, %24
  store i32 %26, i32* %result
  %27 = load i32, i32* %result
  %28 = srem i32 %27, 100
  ret i32 %28
  }


; Global constants
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)
