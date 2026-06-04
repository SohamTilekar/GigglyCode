; ModuleID = 'main.gc..'
source_filename = "/mnt/soham/soham_code/GigglyCode/test/test14/src/main.gc"
target datalayout = "e-m:e-p:32:32-p10:8:8-p20:8:8-i64:64-n32:64-S128-ni:1:10:20"
target triple = "wasm32-unknown-wasi"

%main.gc..Point.0 = type { i64, i64 }
%main.gc..Circle.1 = type { ptr, i64 }

@0 = private unnamed_addr constant [29 x i8] c"Circle approximate area: %i\0A\00", align 1
@1 = private unnamed_addr constant [28 x i8] c"Center coordinates sum: %i\0A\00", align 1

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

define void @__init__(ptr %self, i64 %x, i64 %y) {
entry:
  %self1 = alloca ptr, align 4
  store ptr %self, ptr %self1, align 4
  %x2 = alloca i64, align 8
  store i64 %x, ptr %x2, align 8
  %y3 = alloca i64, align 8
  store i64 %y, ptr %y3, align 8
  %0 = load ptr, ptr %self1, align 4
  %accesedx_from_Point = getelementptr inbounds %main.gc..Point.0, ptr %0, i32 0, i32 0
  %1 = load i64, ptr %x2, align 8
  store i64 %1, ptr %accesedx_from_Point, align 8
  %2 = load ptr, ptr %self1, align 4
  %accesedy_from_Point = getelementptr inbounds %main.gc..Point.0, ptr %2, i32 0, i32 1
  %3 = load i64, ptr %y3, align 8
  store i64 %3, ptr %accesedy_from_Point, align 8
  ret void
}

define i64 @get_sum(ptr %self) {
entry:
  %self1 = alloca ptr, align 4
  store ptr %self, ptr %self1, align 4
  %0 = load ptr, ptr %self1, align 4
  %accesedx_from_Point = getelementptr inbounds %main.gc..Point.0, ptr %0, i32 0, i32 0
  %1 = load ptr, ptr %self1, align 4
  %accesedy_from_Point = getelementptr inbounds %main.gc..Point.0, ptr %1, i32 0, i32 1
  %2 = load i64, ptr %accesedx_from_Point, align 8
  %3 = load i64, ptr %accesedy_from_Point, align 8
  %4 = add i64 %2, %3
  ret i64 %4
}

define void @__init__.1(ptr %self, i64 %cx, i64 %cy, i64 %r) {
entry:
  %self1 = alloca ptr, align 4
  store ptr %self, ptr %self1, align 4
  %cx2 = alloca i64, align 8
  store i64 %cx, ptr %cx2, align 8
  %cy3 = alloca i64, align 8
  store i64 %cy, ptr %cy3, align 8
  %r4 = alloca i64, align 8
  store i64 %r, ptr %r4, align 8
  %0 = load i64, ptr %cx2, align 8
  %1 = load i64, ptr %cy3, align 8
  %Point = alloca %main.gc..Point.0, align 8
  call void @__init__(ptr %Point, i64 %0, i64 %1)
  %2 = load ptr, ptr %self1, align 4
  %accesedcenter_from_Circle = getelementptr inbounds %main.gc..Circle.1, ptr %2, i32 0, i32 0
  store ptr %Point, ptr %accesedcenter_from_Circle, align 4
  %3 = load ptr, ptr %self1, align 4
  %accesedradius_from_Circle = getelementptr inbounds %main.gc..Circle.1, ptr %3, i32 0, i32 1
  %4 = load i64, ptr %r4, align 8
  store i64 %4, ptr %accesedradius_from_Circle, align 8
  ret void
}

define i64 @area_approx(ptr %self) {
entry:
  %self1 = alloca ptr, align 4
  store ptr %self, ptr %self1, align 4
  %0 = load ptr, ptr %self1, align 4
  %accesedradius_from_Circle = getelementptr inbounds %main.gc..Circle.1, ptr %0, i32 0, i32 1
  %1 = load i64, ptr %accesedradius_from_Circle, align 8
  %2 = mul i64 3, %1
  %3 = load ptr, ptr %self1, align 4
  %accesedradius_from_Circle2 = getelementptr inbounds %main.gc..Circle.1, ptr %3, i32 0, i32 1
  %4 = load i64, ptr %accesedradius_from_Circle2, align 8
  %5 = mul i64 %2, %4
  ret i64 %5
}

define i64 @sum_coordinates(ptr %self) {
entry:
  %self1 = alloca ptr, align 4
  store ptr %self, ptr %self1, align 4
  %0 = load ptr, ptr %self1, align 4
  %accesedcenter_from_Circle = getelementptr inbounds %main.gc..Circle.1, ptr %0, i32 0, i32 0
  %1 = load ptr, ptr %self1, align 4
  %accesedcenter_from_Circle2 = getelementptr inbounds %main.gc..Circle.1, ptr %1, i32 0, i32 0
  %2 = load ptr, ptr %accesedcenter_from_Circle2, align 4
  %get_sum_reuturn_value = call i64 @get_sum(ptr %2)
  ret i64 %get_sum_reuturn_value
}

define i64 @main() {
entry:
  %0 = alloca ptr, align 4
  %Circle = alloca %main.gc..Circle.1, align 8
  call void @__init__.1(ptr %Circle, i64 10, i64 20, i64 5)
  store ptr %Circle, ptr %0, align 4
  %1 = load ptr, ptr %0, align 4
  %area_approx_reuturn_value = call i64 @area_approx(ptr %1)
  %2 = call i64 (ptr, ...) @printf(ptr @0, i64 %area_approx_reuturn_value)
  %3 = load ptr, ptr %0, align 4
  %sum_coordinates_reuturn_value = call i64 @sum_coordinates(ptr %3)
  %4 = call i64 (ptr, ...) @printf(ptr @1, i64 %sum_coordinates_reuturn_value)
  ret i64 0
}
