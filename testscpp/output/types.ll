; Generated LLVM IR
target triple = "arm64-apple-darwin"

; Runtime function declarations

define i32 @test_types(i32 %i, i32 %c, i32 %f, i32 %d) {
  %local_int = alloca i32
  %local_char = alloca i8
  %local_float = alloca float
  %local_double = alloca double
  %init_int = alloca i32
  store i32 10, i32* %init_int
  %init_char = alloca i8
  store i8 90, i8* %init_char
  %init_float = alloca float
  store float 1, float* %init_float
  %init_double = alloca double
  store double 9, double* %init_double
  %a = alloca i32
  store i32 1, i32* %a
  %b = alloca i32
  store i32 2, i32* %b
  %c = alloca i32
  store i32 3, i32* %c
  %mixed = alloca i32
  %1 = add i32 %i, %c
  store i32 %1, i32* %mixed
  %float_result = alloca float
  %2 = mul i32 %f, %i
  store float %2, float* %float_result
  %double_result = alloca double
  %3 = sdiv i32 %d, %f
  store double %3, double* %double_result
  %char_result = alloca i8
  %4 = add i32 %c, 1
  store i8 %4, i8* %char_result
  %5 = load i32, i32* %mixed
  %6 = load float, float* %float_result
  %7 = add i32 %5, %6
  %8 = load double, double* %double_result
  %9 = add i32 %7, %8
  %10 = load i8, i8* %char_result
  %11 = sext i8 %10 to i32
  %12 = add i32 %9, %11
  ret i32 %12
  }

define i32 @test_signed_unsigned() {
  %si = alloca i32
  %13 = sub i32 0, 100
  store i32 %13, i32* %si
  %ui = alloca i32
  store i32 200, i32* %ui
  %sc = alloca i32
  %14 = sub i32 0, 50
  store i32 %14, i32* %sc
  %uc = alloca i32
  store i32 250, i32* %uc
  %15 = load i32, i32* %si
  %16 = load i32, i32* %ui
  %17 = add i32 %15, %16
  %18 = load i32, i32* %sc
  %19 = add i32 %17, %18
  %20 = load i32, i32* %uc
  %21 = add i32 %19, %20
  ret i32 %21
  }

define void @void_function() {
  %temp = alloca i32
  store i32 42, i32* %temp
  %22 = load i32, i32* %temp
  %23 = add i32 %22, 1
  store i32 %23, i32* %temp
  ret void
  }

define i32 @test_void_return() {
  %24 = call i32 @void_function()
  ret i32 0
  }


; Global constants
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)
@global_int = global i32 42
@global_char = global i8 65
@global_float = global float 3
@global_double = global double 2
@static_var = global i32 100
@extern_var = global i32 0
