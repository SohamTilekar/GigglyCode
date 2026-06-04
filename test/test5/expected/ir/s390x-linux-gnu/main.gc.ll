; ModuleID = 'main.gc..'
source_filename = "/mnt/soham/soham_code/GigglyCode/test/test5/src/main.gc"
target datalayout = "E-m:e-i1:8:16-i8:8:16-i64:64-f128:64-v128:64-a:8:16-n32:64"
target triple = "s390x-unknown-linux-gnu"

@0 = private unnamed_addr constant [23 x i8] c"Array element %i : %i\0A\00", align 1

declare ptr @malloc(i64)

declare void @free(ptr)

declare void @exit(i64)

declare i64 @printf(ptr, ...)

declare i64 @puts(ptr)

declare i64 @usleep(i64)

declare ptr @memset(ptr, i64, i64)

declare i32 @putchar(i64)

declare double @sin(double)

declare double @cos(double)

declare double @tan(double)

declare double @asin(double)

declare double @acos(double)

declare double @atan(double)

declare double @atan2(double, double)

declare double @sinh(double)

declare double @cosh(double)

declare double @tanh(double)

declare double @asinh(double)

declare double @acosh(double)

declare double @atanh(double)

declare double @exp(double)

declare double @exp2(double)

declare double @expm1(double)

declare double @log(double)

declare double @log10(double)

declare double @log2(double)

declare double @log1p(double)

declare double @sqrt(double)

declare double @cbrt(double)

declare double @hypot(double, double)

declare double @ceil(double)

declare double @floor(double)

declare double @round(double)

declare double @trunc(double)

declare double @fmod(double, double)

declare double @remainder(double, double)

declare double @remquo(double, double)

declare double @fma(double, double, double)

declare double @fdim(double, double)

declare double @fabs(double)

declare double @fmax(double, double)

declare double @fmin(double, double)

declare double @copysign(double, double)

declare double @nan(ptr)

declare double @nextafter(double, double)

declare double @nexttoward(double, double)

declare double @erf(double)

declare double @erfc(double)

declare double @tgamma(double)

declare double @lgamma(double)

define i64 @main() {
entry:
  %0 = alloca i64, align 8
  %1 = alloca ptr, align 8
  %2 = alloca i64, i64 5, align 8
  store ptr %2, ptr %1, align 8
  store i64 0, ptr %0, align 8
  br label %cond

cond:                                             ; preds = %body, %entry
  %3 = load i64, ptr %0, align 8
  %4 = icmp slt i64 %3, 5
  br i1 %4, label %body, label %cont

body:                                             ; preds = %cond
  %5 = load i64, ptr %0, align 8
  %6 = mul i64 %5, 10
  %7 = load i64, ptr %0, align 8
  %8 = load ptr, ptr %1, align 8
  %element = getelementptr i64, ptr %8, i64 %7
  %9 = load i64, ptr %element, align 8
  store i64 %6, ptr %element, align 8
  %10 = load i64, ptr %0, align 8
  %11 = add i64 %10, 1
  store i64 %11, ptr %0, align 8
  br label %cond

cont:                                             ; preds = %cond
  store i64 0, ptr %0, align 8
  br label %cond1

cond1:                                            ; preds = %body2, %cont
  %12 = load i64, ptr %0, align 8
  %13 = icmp slt i64 %12, 5
  br i1 %13, label %body2, label %cont3

body2:                                            ; preds = %cond1
  %14 = load i64, ptr %0, align 8
  %15 = load i64, ptr %0, align 8
  %16 = load ptr, ptr %1, align 8
  %element4 = getelementptr i64, ptr %16, i64 %15
  %17 = load i64, ptr %element4, align 8
  %18 = call i64 (ptr, ...) @printf(ptr @0, i64 %14, i64 %17)
  %19 = load i64, ptr %0, align 8
  %20 = add i64 %19, 1
  store i64 %20, ptr %0, align 8
  br label %cond1

cont3:                                            ; preds = %cond1
  ret i64 0
}
