; ModuleID = 'main.gc..'
source_filename = "/mnt/soham/soham_code/GigglyCode/test/test1/src/main.gc"
target datalayout = "e-m:e-p:32:32-p10:8:8-p20:8:8-i64:64-n32:64-S128-ni:1:10:20"
target triple = "wasm32-unknown-wasi"

%main.gc..Rectangle.0 = type { i64, i64 }

@0 = private unnamed_addr constant [20 x i8] c"Rectangle area: %i\0A\00", align 1
@1 = private unnamed_addr constant [25 x i8] c"Rectangle perimeter: %i\0A\00", align 1

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

define void @__init__(ptr %self, i64 %width, i64 %height) {
entry:
  %self1 = alloca ptr, align 4
  store ptr %self, ptr %self1, align 4
  %width2 = alloca i64, align 8
  store i64 %width, ptr %width2, align 8
  %height3 = alloca i64, align 8
  store i64 %height, ptr %height3, align 8
  %0 = load ptr, ptr %self1, align 4
  %accesedwidth_from_Rectangle = getelementptr inbounds %main.gc..Rectangle.0, ptr %0, i32 0, i32 0
  %1 = load i64, ptr %width2, align 8
  store i64 %1, ptr %accesedwidth_from_Rectangle, align 8
  %2 = load ptr, ptr %self1, align 4
  %accesedheight_from_Rectangle = getelementptr inbounds %main.gc..Rectangle.0, ptr %2, i32 0, i32 1
  %3 = load i64, ptr %height3, align 8
  store i64 %3, ptr %accesedheight_from_Rectangle, align 8
  ret void
}

define i64 @area(ptr %self) {
entry:
  %self1 = alloca ptr, align 4
  store ptr %self, ptr %self1, align 4
  %0 = load ptr, ptr %self1, align 4
  %accesedwidth_from_Rectangle = getelementptr inbounds %main.gc..Rectangle.0, ptr %0, i32 0, i32 0
  %1 = load ptr, ptr %self1, align 4
  %accesedheight_from_Rectangle = getelementptr inbounds %main.gc..Rectangle.0, ptr %1, i32 0, i32 1
  %2 = load i64, ptr %accesedwidth_from_Rectangle, align 8
  %3 = load i64, ptr %accesedheight_from_Rectangle, align 8
  %4 = mul i64 %2, %3
  ret i64 %4
}

define i64 @perimeter(ptr %self) {
entry:
  %self1 = alloca ptr, align 4
  store ptr %self, ptr %self1, align 4
  %0 = load ptr, ptr %self1, align 4
  %accesedwidth_from_Rectangle = getelementptr inbounds %main.gc..Rectangle.0, ptr %0, i32 0, i32 0
  %1 = load ptr, ptr %self1, align 4
  %accesedheight_from_Rectangle = getelementptr inbounds %main.gc..Rectangle.0, ptr %1, i32 0, i32 1
  %2 = load i64, ptr %accesedwidth_from_Rectangle, align 8
  %3 = load i64, ptr %accesedheight_from_Rectangle, align 8
  %4 = add i64 %2, %3
  %5 = mul i64 2, %4
  ret i64 %5
}

define i64 @main() {
entry:
  %0 = alloca ptr, align 4
  %Rectangle = alloca %main.gc..Rectangle.0, align 8
  call void @__init__(ptr %Rectangle, i64 10, i64 20)
  store ptr %Rectangle, ptr %0, align 4
  %1 = load ptr, ptr %0, align 4
  %area_reuturn_value = call i64 @area(ptr %1)
  %2 = call i64 (ptr, ...) @printf(ptr @0, i64 %area_reuturn_value)
  %3 = load ptr, ptr %0, align 4
  %perimeter_reuturn_value = call i64 @perimeter(ptr %3)
  %4 = call i64 (ptr, ...) @printf(ptr @1, i64 %perimeter_reuturn_value)
  ret i64 0
}
