; ModuleID = 'main.gc..'
source_filename = "/mnt/soham/soham_code/GigglyCode/test/test13/src/main.gc"
target datalayout = "E-m:e-i1:8:16-i8:8:16-i64:64-f128:64-v128:64-a:8:16-n32:64"
target triple = "s390x-unknown-linux-gnu"

@0 = private unnamed_addr constant [14 x i8] c"const x = %i\0A\00", align 1
@1 = private unnamed_addr constant [17 x i8] c"volatile y = %i\0A\00", align 1
@2 = private unnamed_addr constant [26 x i8] c"volatile y modified = %i\0A\00", align 1

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
  %1 = alloca i64, align 8
  store i64 42, ptr %1, align 8
  store volatile i64 100, ptr %0, align 8
  %2 = load i64, ptr %1, align 8
  %3 = call i64 (ptr, ...) @printf(ptr @0, i64 %2)
  %4 = load i64, ptr %0, align 8
  %5 = call i64 (ptr, ...) @printf(ptr @1, i64 %4)
  %6 = load i64, ptr %0, align 8
  %7 = add i64 %6, 1
  store i64 %7, ptr %0, align 8
  %8 = load i64, ptr %0, align 8
  %9 = call i64 (ptr, ...) @printf(ptr @2, i64 %8)
  ret i64 0
}
