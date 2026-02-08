; Generated LLVM IR
target triple = "arm64-apple-darwin"

; Runtime function declarations

define i32 @test_expressions() {
  %x = alloca i32
  store i32 42, i32* %x
  %y = alloca i32
  store i32 10, i32* %y
  %z = alloca i32
  %1 = load i32, i32* %x
  %2 = load i32, i32* %y
  %3 = add i32 %1, %2
  store i32 %3, i32* %z
  %add = alloca i32
  %4 = load i32, i32* %x
  %5 = load i32, i32* %y
  %6 = add i32 %4, %5
  store i32 %6, i32* %add
  %sub = alloca i32
  %7 = load i32, i32* %x
  %8 = load i32, i32* %y
  %9 = sub i32 %7, %8
  store i32 %9, i32* %sub
  %mul = alloca i32
  %10 = load i32, i32* %x
  %11 = load i32, i32* %y
  %12 = mul i32 %10, %11
  store i32 %12, i32* %mul
  %div = alloca i32
  %13 = load i32, i32* %x
  %14 = load i32, i32* %y
  %15 = sdiv i32 %13, %14
  store i32 %15, i32* %div
  %mod = alloca i32
  %16 = load i32, i32* %x
  %17 = load i32, i32* %y
  %18 = srem i32 %16, %17
  store i32 %18, i32* %mod
  %lt = alloca i32
  %19 = load i32, i32* %x
  %20 = load i32, i32* %y
  %21 = icmp slt i32 %19, %20
  %22 = zext i1 %21 to i32
  store i32 %22, i32* %lt
  %gt = alloca i32
  %23 = load i32, i32* %x
  %24 = load i32, i32* %y
  %25 = icmp sgt i32 %23, %24
  %26 = zext i1 %25 to i32
  store i32 %26, i32* %gt
  %le = alloca i32
  %27 = load i32, i32* %x
  %28 = load i32, i32* %y
  %29 = icmp sle i32 %27, %28
  %30 = zext i1 %29 to i32
  store i32 %30, i32* %le
  %ge = alloca i32
  %31 = load i32, i32* %x
  %32 = load i32, i32* %y
  %33 = icmp sge i32 %31, %32
  %34 = zext i1 %33 to i32
  store i32 %34, i32* %ge
  %eq = alloca i32
  %35 = load i32, i32* %x
  %36 = load i32, i32* %y
  %37 = icmp eq i32 %35, %36
  %38 = zext i1 %37 to i32
  store i32 %38, i32* %eq
  %ne = alloca i32
  %39 = load i32, i32* %x
  %40 = load i32, i32* %y
  %41 = icmp ne i32 %39, %40
  %42 = zext i1 %41 to i32
  store i32 %42, i32* %ne
  store i32 100, i32* %x
  store i32 200, i32* %y
  %43 = load i32, i32* %x
  %44 = load i32, i32* %y
  %45 = add i32 %43, %44
  %46 = load i32, i32* %z
  %47 = add i32 %45, %46
  ret i32 %47
  }

define i32 @main() {
  %48 = call i32 @test_expressions()
  ret i32 %48
  }


; Global constants
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)
