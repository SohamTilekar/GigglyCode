; ModuleID = 'main.gc..'
source_filename = "/mnt/soham/soham_code/GigglyCode/test/test4/src/main.gc"

@0 = private unnamed_addr constant [13 x i8] c"Value is 1.\0A\00", align 1
@1 = private unnamed_addr constant [13 x i8] c"Value is 2.\0A\00", align 1
@2 = private unnamed_addr constant [13 x i8] c"Value is 3.\0A\00", align 1
@3 = private unnamed_addr constant [19 x i8] c"Value is unknown.\0A\00", align 1
@4 = private unnamed_addr constant [13 x i8] c"Counter: %i\0A\00", align 1

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
  store i64 2, ptr %1, align 4
  %2 = load i64, ptr %1, align 4
  switch i64 %2, label %default [
    i64 1, label %case
    i64 2, label %case1
    i64 3, label %case2
  ]

default:                                          ; preds = %entry
  %3 = call i64 (ptr, ...) @printf(ptr @3)
  br label %end_switch

end_switch:                                       ; preds = %default, %case2, %case1, %case
  store i64 0, ptr %0, align 4
  br label %cond

case:                                             ; preds = %entry
  %4 = call i64 (ptr, ...) @printf(ptr @0)
  br label %end_switch

case1:                                            ; preds = %entry
  %5 = call i64 (ptr, ...) @printf(ptr @1)
  br label %end_switch

case2:                                            ; preds = %entry
  %6 = call i64 (ptr, ...) @printf(ptr @2)
  br label %end_switch

cond:                                             ; preds = %body, %end_switch
  %7 = load i64, ptr %0, align 4
  %8 = icmp slt i64 %7, 5
  br i1 %8, label %body, label %cont

body:                                             ; preds = %cond
  %9 = load i64, ptr %0, align 4
  %10 = call i64 (ptr, ...) @printf(ptr @4, i64 %9)
  %11 = load i64, ptr %0, align 4
  %12 = add i64 %11, 1
  store i64 %12, ptr %0, align 4
  br label %cond

cont:                                             ; preds = %cond
  ret i64 0
}
