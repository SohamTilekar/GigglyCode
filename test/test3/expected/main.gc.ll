; ModuleID = 'main.gc..'
source_filename = "/mnt/soham/soham_code/GigglyCode/test/test3/src/main.gc"

@0 = private unnamed_addr constant [12 x i8] c"Int: %i:%i\0A\00", align 1
@1 = private unnamed_addr constant [14 x i8] c"Float: %f:%f\0A\00", align 1
@2 = private unnamed_addr constant [23 x i8] c"Subtract integers: %i\0A\00", align 1
@3 = private unnamed_addr constant [21 x i8] c"Subtract floats: %f\0A\00", align 1

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

define i64 @subtract(i64 %a, i64 %b) {
entry:
  %a1 = alloca i64, align 8
  store i64 %a, ptr %a1, align 4
  %b2 = alloca i64, align 8
  store i64 %b, ptr %b2, align 4
  %0 = load i64, ptr %a1, align 4
  %1 = load i64, ptr %b2, align 4
  %2 = call i64 (ptr, ...) @printf(ptr @0, i64 %0, i64 %1)
  %3 = load i64, ptr %a1, align 4
  %4 = load i64, ptr %b2, align 4
  %5 = sub i64 %3, %4
  ret i64 %5
}

define double @subtract.1(double %a, double %b) {
entry:
  %a1 = alloca double, align 8
  store double %a, ptr %a1, align 8
  %b2 = alloca double, align 8
  store double %b, ptr %b2, align 8
  %0 = load double, ptr %a1, align 8
  %1 = load double, ptr %b2, align 8
  %2 = call i64 (ptr, ...) @printf(ptr @1, double %0, double %1)
  %3 = load double, ptr %a1, align 8
  %4 = load double, ptr %b2, align 8
  %5 = fsub double %3, %4
  ret double %5
}

define i64 @main() {
entry:
  %0 = call i64 @subtract(i64 10, i64 5)
  %1 = call i64 (ptr, ...) @printf(ptr @2, i64 %0)
  %2 = call double @subtract.1(double 1.050000e+01, double 5.500000e+00)
  %3 = call i64 (ptr, ...) @printf(ptr @3, double %2)
  ret i64 0
}
