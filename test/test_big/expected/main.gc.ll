; ModuleID = 'main.gc..'
source_filename = "/mnt/soham/soham_code/GigglyCode/test/test_big/src/main.gc"

%main.gc..Point.0 = type { i64, i64 }
%main.gc..Box = type { i64 }

@0 = private unnamed_addr constant [8 x i8] c"SUCCESS\00", align 1
@1 = private unnamed_addr constant [7 x i8] c"FAILED\00", align 1
@2 = private unnamed_addr constant [8 x i8] c"PENDING\00", align 1
@3 = private unnamed_addr constant [18 x i8] c"Overload Int: %i\0A\00", align 1
@4 = private unnamed_addr constant [20 x i8] c"Overload Float: %f\0A\00", align 1
@5 = private unnamed_addr constant [12 x i8] c"Status: %s\0A\00", align 1
@6 = private unnamed_addr constant [15 x i8] c"Box Value: %i\0A\00", align 1
@7 = private unnamed_addr constant [21 x i8] c"Sum from helper: %i\0A\00", align 1
@8 = private unnamed_addr constant [28 x i8] c"Point sub result: (%i, %i)\0A\00", align 1
@9 = private unnamed_addr constant [14 x i8] c"arr[%i] = %i\0A\00", align 1
@10 = private unnamed_addr constant [8 x i8] c"Case 1\0A\00", align 1
@11 = private unnamed_addr constant [8 x i8] c"Case 2\0A\00", align 1
@12 = private unnamed_addr constant [12 x i8] c"Other Case\0A\00", align 1
@13 = private unnamed_addr constant [16 x i8] c"Loop count: %i\0A\00", align 1
@14 = private unnamed_addr constant [19 x i8] c"Loop broke early!\0A\00", align 1
@15 = private unnamed_addr constant [31 x i8] c"Loop completed without break!\0A\00", align 1

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

declare i64 @modules..helper.gc..compute_sum(i64, i64)

define ptr @Status..getName(i2 %0) {
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

define void @__init__(ptr %self, i64 %x, i64 %y) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %x2 = alloca i64, align 8
  store i64 %x, ptr %x2, align 4
  %y3 = alloca i64, align 8
  store i64 %y, ptr %y3, align 4
  %0 = load ptr, ptr %self1, align 8
  %accesedx_from_Point = getelementptr inbounds %main.gc..Point.0, ptr %0, i32 0, i32 0
  %1 = load i64, ptr %x2, align 4
  store i64 %1, ptr %accesedx_from_Point, align 4
  %2 = load ptr, ptr %self1, align 8
  %accesedy_from_Point = getelementptr inbounds %main.gc..Point.0, ptr %2, i32 0, i32 1
  %3 = load i64, ptr %y3, align 4
  store i64 %3, ptr %accesedy_from_Point, align 4
  ret void
}

define ptr @__sub__(ptr %self, ptr %other_) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %other_2 = alloca ptr, align 8
  store ptr %other_, ptr %other_2, align 8
  %0 = load ptr, ptr %self1, align 8
  %accesedx_from_Point = getelementptr inbounds %main.gc..Point.0, ptr %0, i32 0, i32 0
  %1 = load ptr, ptr %other_2, align 8
  %accesedx_from_Point3 = getelementptr inbounds %main.gc..Point.0, ptr %1, i32 0, i32 0
  %2 = load i64, ptr %accesedx_from_Point, align 4
  %3 = load i64, ptr %accesedx_from_Point3, align 4
  %4 = sub i64 %2, %3
  %5 = load ptr, ptr %self1, align 8
  %accesedy_from_Point = getelementptr inbounds %main.gc..Point.0, ptr %5, i32 0, i32 1
  %6 = load ptr, ptr %other_2, align 8
  %accesedy_from_Point4 = getelementptr inbounds %main.gc..Point.0, ptr %6, i32 0, i32 1
  %7 = load i64, ptr %accesedy_from_Point, align 4
  %8 = load i64, ptr %accesedy_from_Point4, align 4
  %9 = sub i64 %7, %8
  %Point = alloca %main.gc..Point.0, align 8
  call void @__init__(ptr %Point, i64 %4, i64 %9)
  ret ptr %Point
}

define void @printVal(i64 %x) {
entry:
  %x1 = alloca i64, align 8
  store i64 %x, ptr %x1, align 4
  %0 = load i64, ptr %x1, align 4
  %1 = call i64 (ptr, ...) @printf(ptr @3, i64 %0)
  ret void
}

define void @printVal.1(double %x) {
entry:
  %x1 = alloca double, align 8
  store double %x, ptr %x1, align 8
  %0 = load double, ptr %x1, align 8
  %1 = call i64 (ptr, ...) @printf(ptr @4, double %0)
  ret void
}

define i64 @main() {
entry:
  %0 = alloca i64, align 8
  %1 = alloca i64, align 8
  %2 = alloca ptr, align 8
  %3 = alloca ptr, align 8
  %4 = alloca ptr, align 8
  %5 = alloca ptr, align 8
  %6 = alloca ptr, align 8
  %7 = alloca i2, align 1
  %8 = alloca double, align 8
  %9 = alloca i64, align 8
  %10 = alloca i64, align 8
  %11 = alloca i64, align 8
  store i64 10, ptr %11, align 4
  store volatile i64 0, ptr %10, align 4
  store i64 42, ptr %9, align 4
  store double 3.140000e+00, ptr %8, align 8
  store i2 0, ptr %7, align 1
  %12 = load i2, ptr %7, align 1
  %13 = call ptr @Status..getName(i2 %12)
  %14 = call i64 (ptr, ...) @printf(ptr @5, ptr %13)
  %15 = load i64, ptr %9, align 4
  %Box = alloca %main.gc..Box, align 8
  call void @__init__.2(ptr %Box, i64 %15)
  store ptr %Box, ptr %6, align 8
  %16 = load ptr, ptr %6, align 8
  %getValue_reuturn_value = call i64 @getValue(ptr %16)
  %17 = call i64 (ptr, ...) @printf(ptr @6, i64 %getValue_reuturn_value)
  %compute_sum_result = call i64 @modules..helper.gc..compute_sum(i64 100, i64 200)
  %18 = call i64 (ptr, ...) @printf(ptr @7, i64 %compute_sum_result)
  %19 = load i64, ptr %9, align 4
  call void @printVal(i64 %19)
  %20 = load double, ptr %8, align 8
  call void @printVal.1(double %20)
  %Point = alloca %main.gc..Point.0, align 8
  call void @__init__(ptr %Point, i64 10, i64 20)
  store ptr %Point, ptr %5, align 8
  %Point1 = alloca %main.gc..Point.0, align 8
  call void @__init__(ptr %Point1, i64 3, i64 4)
  store ptr %Point1, ptr %4, align 8
  %21 = load ptr, ptr %5, align 8
  %22 = load ptr, ptr %4, align 8
  %23 = call ptr @__sub__(ptr %21, ptr %22)
  store ptr %23, ptr %3, align 8
  %24 = load ptr, ptr %3, align 8
  %accesedx_from_Point = getelementptr inbounds %main.gc..Point.0, ptr %24, i32 0, i32 0
  %25 = load i64, ptr %accesedx_from_Point, align 4
  %26 = load ptr, ptr %3, align 8
  %accesedy_from_Point = getelementptr inbounds %main.gc..Point.0, ptr %26, i32 0, i32 1
  %27 = load i64, ptr %accesedy_from_Point, align 4
  %28 = call i64 (ptr, ...) @printf(ptr @8, i64 %25, i64 %27)
  %29 = alloca i64, i64 3, align 8
  store ptr %29, ptr %2, align 8
  %30 = load ptr, ptr %2, align 8
  %element = getelementptr i64, ptr %30, i64 0
  %31 = load i64, ptr %element, align 4
  store i64 10, ptr %element, align 4
  %32 = load ptr, ptr %2, align 8
  %element2 = getelementptr i64, ptr %32, i64 1
  %33 = load i64, ptr %element2, align 4
  store i64 20, ptr %element2, align 4
  %34 = load ptr, ptr %2, align 8
  %element3 = getelementptr i64, ptr %34, i64 2
  %35 = load i64, ptr %element3, align 4
  store i64 30, ptr %element3, align 4
  store i64 0, ptr %1, align 4
  br label %cond

cond:                                             ; preds = %body, %entry
  %36 = load i64, ptr %1, align 4
  %37 = icmp slt i64 %36, 3
  br i1 %37, label %body, label %cont

body:                                             ; preds = %cond
  %38 = load i64, ptr %1, align 4
  %39 = load i64, ptr %1, align 4
  %40 = load ptr, ptr %2, align 8
  %element4 = getelementptr i64, ptr %40, i64 %39
  %41 = load i64, ptr %element4, align 4
  %42 = call i64 (ptr, ...) @printf(ptr @9, i64 %38, i64 %41)
  %43 = load i64, ptr %1, align 4
  %44 = add i64 %43, 1
  store i64 %44, ptr %1, align 4
  br label %cond

cont:                                             ; preds = %cond
  store i64 2, ptr %0, align 4
  %45 = load i64, ptr %0, align 4
  switch i64 %45, label %default [
    i64 1, label %case
    i64 2, label %case5
  ]

default:                                          ; preds = %cont
  %46 = call i64 (ptr, ...) @printf(ptr @12)
  br label %end_switch

end_switch:                                       ; preds = %default, %case5, %case
  br label %cond6

case:                                             ; preds = %cont
  %47 = call i64 (ptr, ...) @printf(ptr @10)
  br label %end_switch

case5:                                            ; preds = %cont
  %48 = call i64 (ptr, ...) @printf(ptr @11)
  br label %end_switch

cond6:                                            ; preds = %cont9, %end_switch
  %49 = load i64, ptr %10, align 4
  %50 = icmp slt i64 %49, 3
  br i1 %50, label %body7, label %notbreak

body7:                                            ; preds = %cond6
  %51 = load i64, ptr %10, align 4
  %52 = call i64 (ptr, ...) @printf(ptr @13, i64 %51)
  %53 = load i64, ptr %10, align 4
  %54 = add i64 %53, 1
  store i64 %54, ptr %10, align 4
  %55 = load i64, ptr %10, align 4
  %56 = icmp eq i64 %55, 5
  br i1 %56, label %then, label %cont9

cont8:                                            ; preds = %notbreak, %ifbreak
  ret i64 0

ifbreak:                                          ; preds = %then
  %57 = call i64 (ptr, ...) @printf(ptr @14)
  br label %cont8

notbreak:                                         ; preds = %cond6
  %58 = call i64 (ptr, ...) @printf(ptr @15)
  br label %cont8

then:                                             ; preds = %body7
  br label %ifbreak

cont9:                                            ; preds = %body7
  br label %cond6
}

define void @__init__.2(ptr %self, i64 %val) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %val2 = alloca i64, align 8
  store i64 %val, ptr %val2, align 4
  %0 = load ptr, ptr %self1, align 8
  %accesedvalue_from_Box = getelementptr inbounds %main.gc..Box, ptr %0, i32 0, i32 0
  %1 = load i64, ptr %val2, align 4
  store i64 %1, ptr %accesedvalue_from_Box, align 4
  ret void
}

define i64 @getValue(ptr %self) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %0 = load ptr, ptr %self1, align 8
  %accesedvalue_from_Box = getelementptr inbounds %main.gc..Box, ptr %0, i32 0, i32 0
  %1 = load i64, ptr %accesedvalue_from_Box, align 4
  ret i64 %1
}
