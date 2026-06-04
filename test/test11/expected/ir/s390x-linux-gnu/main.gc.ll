; ModuleID = 'main.gc..'
source_filename = "/mnt/soham/soham_code/GigglyCode/test/test11/src/main.gc"
target datalayout = "E-m:e-i1:8:16-i8:8:16-i64:64-f128:64-v128:64-a:8:16-n32:64"
target triple = "s390x-unknown-linux-gnu"

@0 = private unnamed_addr constant [4 x i8] c"RED\00", align 1
@1 = private unnamed_addr constant [6 x i8] c"GREEN\00", align 1
@2 = private unnamed_addr constant [5 x i8] c"BLUE\00", align 1
@3 = private unnamed_addr constant [21 x i8] c"Enum value name: %s\0A\00", align 1

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

define ptr @Color..getName(i2 %0) {
entry:
  switch i2 %0, label %dump [
    i2 0, label %case
    i2 1, label %case1
    i2 -2, label %case2
  ]

dump:                                             ; preds = %entry
  unreachable

case:                                             ; preds = %entry
  ret ptr @0

case1:                                            ; preds = %entry
  ret ptr @1

case2:                                            ; preds = %entry
  ret ptr @2
}

define i64 @main() {
entry:
  %0 = alloca i2, align 2
  store i2 1, ptr %0, align 1
  %1 = load i2, ptr %0, align 1
  %2 = call ptr @Color..getName(i2 %1)
  %3 = call i64 (ptr, ...) @printf(ptr @3, ptr %2)
  ret i64 0
}
