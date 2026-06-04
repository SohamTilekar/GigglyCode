; ModuleID = 'main.gc..'
source_filename = "/mnt/soham/soham_code/GigglyCode/test/test7/src/main.gc"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@0 = private unnamed_addr constant [17 x i8] c"Loop normal: %i\0A\00", align 1
@1 = private unnamed_addr constant [67 x i8] c"Normal completion: ifbreak block executed (ERROR: should not run)\0A\00", align 1
@2 = private unnamed_addr constant [44 x i8] c"Normal completion: notbreak block executed\0A\00", align 1
@3 = private unnamed_addr constant [16 x i8] c"Loop break: %i\0A\00", align 1
@4 = private unnamed_addr constant [42 x i8] c"Break completion: ifbreak block executed\0A\00", align 1
@5 = private unnamed_addr constant [67 x i8] c"Break completion: notbreak block executed (ERROR: should not run)\0A\00", align 1

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
  store i64 0, ptr %1, align 8
  br label %cond

cond:                                             ; preds = %body, %entry
  %2 = load i64, ptr %1, align 8
  %3 = icmp slt i64 %2, 3
  br i1 %3, label %body, label %notbreak

body:                                             ; preds = %cond
  %4 = load i64, ptr %1, align 8
  %5 = call i64 (ptr, ...) @printf(ptr @0, i64 %4)
  %6 = load i64, ptr %1, align 8
  %7 = add i64 %6, 1
  store i64 %7, ptr %1, align 8
  br label %cond

cont:                                             ; preds = %notbreak, %ifbreak
  store i64 0, ptr %0, align 8
  br label %cond1

ifbreak:                                          ; No predecessors!
  %8 = call i64 (ptr, ...) @printf(ptr @1)
  br label %cont

notbreak:                                         ; preds = %cond
  %9 = call i64 (ptr, ...) @printf(ptr @2)
  br label %cont

cond1:                                            ; preds = %cont6, %cont
  %10 = load i64, ptr %0, align 8
  %11 = icmp slt i64 %10, 3
  br i1 %11, label %body2, label %notbreak5

body2:                                            ; preds = %cond1
  %12 = load i64, ptr %0, align 8
  %13 = call i64 (ptr, ...) @printf(ptr @3, i64 %12)
  %14 = load i64, ptr %0, align 8
  %15 = icmp eq i64 %14, 1
  br i1 %15, label %then, label %cont6

cont3:                                            ; preds = %notbreak5, %ifbreak4
  ret i64 0

ifbreak4:                                         ; preds = %then
  %16 = call i64 (ptr, ...) @printf(ptr @4)
  br label %cont3

notbreak5:                                        ; preds = %cond1
  %17 = call i64 (ptr, ...) @printf(ptr @5)
  br label %cont3

then:                                             ; preds = %body2
  br label %ifbreak4

cont6:                                            ; preds = %body2
  %18 = load i64, ptr %0, align 8
  %19 = add i64 %18, 1
  store i64 %19, ptr %0, align 8
  br label %cond1
}
