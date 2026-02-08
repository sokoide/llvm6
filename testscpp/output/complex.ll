; Generated LLVM IR
target triple = "arm64-apple-darwin"

; Runtime function declarations

define i32 @add(i32 %a, i32 %b) {
  %1 = add i32 %a, %b
  ret i32 %1
  }

define i32 @multiply(i32 %a, i32 %b) {
  %2 = mul i32 %a, %b
  ret i32 %2
  }

define i32 @nested_calls() {
  %3 = call i32 @multiply(i32 2, i32 3)
  %4 = call i32 @add(i32 4, i32 5)
  %5 = call i32 @add(i32 %3, i32 %4)
  ret i32 %5
  }

define i32 @complex_expressions(i32 %x, i32 %y) {
  %result1 = alloca i32
  %6 = add i32 %x, %y
  %7 = sub i32 %x, %y
  %8 = mul i32 %6, %7
  %9 = mul i32 %x, 2
  %10 = sdiv i32 %y, 2
  %11 = add i32 %9, %10
  %12 = sdiv i32 %8, %11
  store i32 %12, i32* %result1
  %result2 = alloca i32
  %13 = mul i32 %y, 2
  %14 = add i32 %x, %13
  %15 = sdiv i32 %x, 3
  %16 = sub i32 %14, %15
  %17 = srem i32 %y, 4
  %18 = add i32 %16, %17
  store i32 %18, i32* %result2
  %result3 = alloca i32
  %19 = icmp sgt i32 %x, %y
  %20 = zext i1 %19 to i32
  store i32 %20, i32* %result3
  %result4 = alloca i32
  store i32 0, i32* %result4
  ; if statement
  %21 = icmp sgt i32 %x, %y
  %22 = zext i1 %21 to i32
  %23 = icmp ne i32 %22, 0
  br i1 %23, label %bb1, label %bb2
bb1:
  %24 = mul i32 %x, 2
  store i32 %24, i32* %result4
  br label %bb3
bb2:
  %25 = add i32 %y, 10
  store i32 %25, i32* %result4
  br label %bb3
bb3:
  %26 = load i32, i32* %result1
  %27 = load i32, i32* %result2
  %28 = add i32 %26, %27
  %29 = load i32, i32* %result3
  %30 = add i32 %28, %29
  %31 = load i32, i32* %result4
  %32 = add i32 %30, %31
  ret i32 %32
  }

define i32 @test_scoping() {
  %x = alloca i32
  store i32 10, i32* %x
  %y = alloca i32
  store i32 20, i32* %y
  %x = alloca i32
  store i32 30, i32* %x
  %z = alloca i32
  store i32 40, i32* %z
  %result = alloca i32
  %33 = add i32 %x, %y
  %34 = load i32, i32* %z
  %35 = add i32 %33, %34
  store i32 %35, i32* %result
  ; if statement
  %36 = load i32, i32* %result
  %37 = icmp sgt i32 %36, 80
  %38 = zext i1 %37 to i32
  %39 = icmp ne i32 %38, 0
  br i1 %39, label %bb4, label %bb5
bb4:
  %w = alloca i32
  store i32 50, i32* %w
  %40 = load i32, i32* %result
  %41 = load i32, i32* %w
  %42 = add i32 %40, %41
  store i32 %42, i32* %result
  br label %bb6
bb5:
  br label %bb6
bb6:
  ret i32 %result
  }

define i32 @test_increment_decrement() {
  %x = alloca i32
  store i32 10, i32* %x
  %y = alloca i32
  store i32 20, i32* %y
  %a = alloca i32
  %b = alloca i32
  %result = alloca i32
  %43 = add i32 %x, 1
  store i32 %43, i32* %x
  %44 = sub i32 %y, 1
  store i32 %44, i32* %y
  store i32 %x, i32* %a
  store i32 %y, i32* %b
  %45 = add i32 %x, %y
  store i32 %45, i32* %result
  %46 = add i32 %a, %b
  %47 = load i32, i32* %result
  %48 = add i32 %46, %47
  ret i32 %48
  }

define i32 @test_strings() {
  %result = alloca i32
  store i32 42, i32* %result
  ret i32 %result
  }

define i32 @test_empty() {
  %i = alloca i32
  ; if statement
  %49 = icmp ne i32 1, 0
  br i1 %49, label %bb7, label %bb8
bb7:
  br label %bb9
bb8:
  br label %bb9
bb9:
  ; while statement
  br label %bb10
bb10:
  %50 = icmp ne i32 0, 0
  br i1 %50, label %bb11, label %bb12
bb11:
  br label %bb10
  br label %bb10
bb12:
  store i32 0, i32* %i
  ; while statement
  br label %bb13
bb13:
  %51 = load i32, i32* %i
  %52 = icmp slt i32 %51, 1
  %53 = zext i1 %52 to i32
  %54 = icmp ne i32 %53, 0
  br i1 %54, label %bb14, label %bb15
bb14:
  %55 = load i32, i32* %i
  %56 = add i32 %55, 1
  store i32 %56, i32* %i
  br label %bb13
  br label %bb13
bb15:
  ret i32 0
  }

define i32 @main() {
  %result = alloca i32
  store i32 0, i32* %result
  %57 = call i32 @nested_calls()
  %58 = load i32, i32* %result
  %59 = add i32 %58, %57
  store i32 %59, i32* %result
  %60 = call i32 @complex_expressions(i32 10, i32 5)
  %61 = load i32, i32* %result
  %62 = add i32 %61, %60
  store i32 %62, i32* %result
  %63 = call i32 @test_scoping()
  %64 = load i32, i32* %result
  %65 = add i32 %64, %63
  store i32 %65, i32* %result
  %66 = call i32 @test_increment_decrement()
  %67 = load i32, i32* %result
  %68 = add i32 %67, %66
  store i32 %68, i32* %result
  %69 = call i32 @test_strings()
  %70 = load i32, i32* %result
  %71 = add i32 %70, %69
  store i32 %71, i32* %result
  %72 = call i32 @test_empty()
  %73 = load i32, i32* %result
  %74 = add i32 %73, %72
  store i32 %74, i32* %result
  ret i32 %result
  }


; Global constants
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)
