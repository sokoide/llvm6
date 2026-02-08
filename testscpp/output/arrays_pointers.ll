; Generated LLVM IR
target triple = "arm64-apple-darwin"

; Runtime function declarations

define i32 @test_arrays() {
  %val0 = alloca i32
  store i32 100, i32* %val0
  %val1 = alloca i32
  store i32 200, i32* %val1
  %val2 = alloca i32
  store i32 300, i32* %val2
  %val3 = alloca i32
  store i32 400, i32* %val3
  %sum = alloca i32
  %1 = load i32, i32* %val0
  %2 = load i32, i32* %val1
  %3 = add i32 %1, %2
  store i32 %3, i32* %sum
  %product = alloca i32
  %4 = load i32, i32* %val2
  %5 = load i32, i32* %val3
  %6 = mul i32 %4, %5
  store i32 %6, i32* %product
  %complex = alloca i32
  %7 = load i32, i32* %val0
  %8 = srem i32 %7, 5
  %9 = load i32, i32* %val1
  %10 = srem i32 %9, 5
  %11 = add i32 %8, %10
  store i32 %11, i32* %complex
  %12 = load i32, i32* %sum
  %13 = load i32, i32* %product
  %14 = add i32 %12, %13
  %15 = load i32, i32* %complex
  %16 = add i32 %14, %15
  ret i32 %16
  }

define i32 @test_sizeof() {
  %x = alloca i32
  store i32 4, i32* %x
  %y = alloca i32
  store i32 1, i32* %y
  %z = alloca i32
  store i32 4, i32* %z
  %w = alloca i32
  store i32 8, i32* %w
  %a = alloca i32
  store i32 42, i32* %a
  %size_of_expr = alloca i32
  store i32 4, i32* %size_of_expr
  %size_of_complex = alloca i32
  store i32 4, i32* %size_of_complex
  %17 = load i32, i32* %x
  %18 = load i32, i32* %y
  %19 = add i32 %17, %18
  %20 = load i32, i32* %z
  %21 = add i32 %19, %20
  %22 = load i32, i32* %w
  %23 = add i32 %21, %22
  %24 = load i32, i32* %size_of_expr
  %25 = add i32 %23, %24
  %26 = load i32, i32* %size_of_complex
  %27 = add i32 %25, %26
  ret i32 %27
  }

define i32 @test_address_deref() {
  %x = alloca i32
  store i32 100, i32* %x
  %value = alloca i32
  store i32 %x, i32* %value
  ret i32 %value
  }


; Global constants
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)
