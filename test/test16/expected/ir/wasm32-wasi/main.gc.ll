; ModuleID = 'main.gc..'
source_filename = "/mnt/soham/soham_code/GigglyCode/test/test16/src/main.gc"
target datalayout = "e-m:e-p:32:32-p10:8:8-p20:8:8-i64:64-n32:64-S128-ni:1:10:20"
target triple = "wasm32-unknown-wasi"

@0 = private unnamed_addr constant [5 x i8] c"INIT\00", align 1
@1 = private unnamed_addr constant [8 x i8] c"RUNNING\00", align 1
@2 = private unnamed_addr constant [8 x i8] c"STOPPED\00", align 1
@3 = private unnamed_addr constant [6 x i8] c"INIT\0A\00", align 1
@4 = private unnamed_addr constant [9 x i8] c"RUNNING\0A\00", align 1
@5 = private unnamed_addr constant [9 x i8] c"STOPPED\0A\00", align 1
@6 = private unnamed_addr constant [15 x i8] c"UNKNOWN STATE\0A\00", align 1
@7 = private unnamed_addr constant [6 x i8] c"hello\00", align 1
@8 = private unnamed_addr constant [7 x i8] c"giggly\00", align 1
@9 = private unnamed_addr constant [7 x i8] c"World\0A\00", align 1
@10 = private unnamed_addr constant [6 x i8] c"Code\0A\00", align 1
@11 = private unnamed_addr constant [16 x i8] c"default string\0A\00", align 1
@12 = private unnamed_addr constant [6 x i8] c"hello\00", align 1
@13 = private unnamed_addr constant [7 x i8] c"giggly\00", align 1
@14 = private unnamed_addr constant [6 x i8] c"other\00", align 1

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

define ptr @State..getName(i2 %0) {
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

define i64 @test_enum(i2 %s) {
entry:
  %s1 = alloca i2, align 1
  store i2 %s, ptr %s1, align 1
  %0 = load i2, ptr %s1, align 1
  switch i2 %0, label %default [
    i2 0, label %case
    i2 1, label %case2
    i2 -2, label %case3
  ]

default:                                          ; preds = %entry
  %1 = call i64 (ptr, ...) @printf(ptr @6)
  br label %end_switch

end_switch:                                       ; preds = %default, %case3, %case2, %case
  ret i64 0

case:                                             ; preds = %entry
  %2 = call i64 (ptr, ...) @printf(ptr @3)
  br label %end_switch

case2:                                            ; preds = %entry
  %3 = call i64 (ptr, ...) @printf(ptr @4)
  br label %end_switch

case3:                                            ; preds = %entry
  %4 = call i64 (ptr, ...) @printf(ptr @5)
  br label %end_switch
}

define i64 @test_string(ptr %s) {
entry:
  %s1 = alloca ptr, align 4
  store ptr %s, ptr %s1, align 4
  %0 = load ptr, ptr %s1, align 4
  %1 = call i32 @strcmp(ptr %0, ptr @7)
  %2 = icmp eq i32 %1, 0
  br i1 %2, label %case, label %switch_strcmp_check

default:                                          ; preds = %switch_strcmp_check
  %3 = call i64 (ptr, ...) @printf(ptr @11)
  br label %end_switch

end_switch:                                       ; preds = %default, %case2, %case
  ret i64 0

case:                                             ; preds = %entry
  %4 = call i64 (ptr, ...) @printf(ptr @9)
  br label %end_switch

case2:                                            ; preds = %switch_strcmp_check
  %5 = call i64 (ptr, ...) @printf(ptr @10)
  br label %end_switch

switch_strcmp_check:                              ; preds = %entry
  %6 = call i32 @strcmp(ptr %0, ptr @8)
  %7 = icmp eq i32 %6, 0
  br i1 %7, label %case2, label %default
}

declare i32 @strcmp(ptr, ptr)

define i64 @main() {
entry:
  %0 = call i64 @test_enum(i2 1)
  %1 = call i64 @test_enum(i2 -2)
  %2 = call i64 @test_string(ptr @12)
  %3 = call i64 @test_string(ptr @13)
  %4 = call i64 @test_string(ptr @14)
  ret i64 0
}
