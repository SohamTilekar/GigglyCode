; ModuleID = 'main.gc..'
source_filename = "/mnt/soham/soham_code/GigglyCode/test/test6/src/main.gc"

%main.gc..Point.0 = type { i64, i64 }
%main.gc..Square.1 = type { ptr, ptr }

@0 = private unnamed_addr constant [13 x i8] c"Delta X: %d\0A\00", align 1
@1 = private unnamed_addr constant [13 x i8] c"Delta Y: %d\0A\00", align 1
@2 = private unnamed_addr constant [21 x i8] c"Delta X Squared: %d\0A\00", align 1
@3 = private unnamed_addr constant [21 x i8] c"Delta Y Squared: %d\0A\00", align 1
@4 = private unnamed_addr constant [20 x i8] c"Sum of Squares: %d\0A\00", align 1
@5 = private unnamed_addr constant [14 x i8] c"Distance: %f\0A\00", align 1
@6 = private unnamed_addr constant [42 x i8] c"Length of the side of the square is: %lf\0A\00", align 1

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

define void @__init__(ptr %self, i64 %X, i64 %Y) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %X2 = alloca i64, align 8
  store i64 %X, ptr %X2, align 4
  %Y3 = alloca i64, align 8
  store i64 %Y, ptr %Y3, align 4
  %0 = load ptr, ptr %self1, align 8
  %accesedX_from_Point = getelementptr inbounds %main.gc..Point.0, ptr %0, i32 0, i32 0
  %1 = load i64, ptr %X2, align 4
  store i64 %1, ptr %accesedX_from_Point, align 4
  %2 = load ptr, ptr %self1, align 8
  %accesedY_from_Point = getelementptr inbounds %main.gc..Point.0, ptr %2, i32 0, i32 1
  %3 = load i64, ptr %Y3, align 4
  store i64 %3, ptr %accesedY_from_Point, align 4
  ret void
}

define void @__init__.1(ptr %self) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  ret void
}

define double @__sub__(ptr %self, ptr %_other) {
entry:
  %0 = alloca double, align 8
  %1 = alloca i64, align 8
  %2 = alloca i64, align 8
  %3 = alloca i64, align 8
  %4 = alloca i64, align 8
  %5 = alloca i64, align 8
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %_other2 = alloca ptr, align 8
  store ptr %_other, ptr %_other2, align 8
  %6 = load ptr, ptr %self1, align 8
  %accesedX_from_Point = getelementptr inbounds %main.gc..Point.0, ptr %6, i32 0, i32 0
  %7 = load ptr, ptr %_other2, align 8
  %accesedX_from_Point3 = getelementptr inbounds %main.gc..Point.0, ptr %7, i32 0, i32 0
  %8 = load i64, ptr %accesedX_from_Point, align 4
  %9 = load i64, ptr %accesedX_from_Point3, align 4
  %10 = sub i64 %8, %9
  store i64 %10, ptr %5, align 4
  %11 = load ptr, ptr %self1, align 8
  %accesedY_from_Point = getelementptr inbounds %main.gc..Point.0, ptr %11, i32 0, i32 1
  %12 = load ptr, ptr %_other2, align 8
  %accesedY_from_Point4 = getelementptr inbounds %main.gc..Point.0, ptr %12, i32 0, i32 1
  %13 = load i64, ptr %accesedY_from_Point, align 4
  %14 = load i64, ptr %accesedY_from_Point4, align 4
  %15 = sub i64 %13, %14
  store i64 %15, ptr %4, align 4
  %16 = load i64, ptr %5, align 4
  %17 = call i64 (ptr, ...) @printf(ptr @0, i64 %16)
  %18 = load i64, ptr %4, align 4
  %19 = call i64 (ptr, ...) @printf(ptr @1, i64 %18)
  %20 = load i64, ptr %5, align 4
  %21 = load i64, ptr %5, align 4
  %22 = mul i64 %20, %21
  store i64 %22, ptr %3, align 4
  %23 = load i64, ptr %4, align 4
  %24 = load i64, ptr %4, align 4
  %25 = mul i64 %23, %24
  store i64 %25, ptr %2, align 4
  %26 = load i64, ptr %3, align 4
  %27 = call i64 (ptr, ...) @printf(ptr @2, i64 %26)
  %28 = load i64, ptr %2, align 4
  %29 = call i64 (ptr, ...) @printf(ptr @3, i64 %28)
  %30 = load i64, ptr %3, align 4
  %31 = load i64, ptr %2, align 4
  %32 = add i64 %30, %31
  store i64 %32, ptr %1, align 4
  %33 = load i64, ptr %1, align 4
  %34 = call i64 (ptr, ...) @printf(ptr @4, i64 %33)
  %35 = load i64, ptr %1, align 4
  %36 = sitofp i64 %35 to double
  %sqrt_result = call double @sqrt(double %36)
  store double %sqrt_result, ptr %0, align 8
  %37 = load double, ptr %0, align 8
  %38 = call i64 (ptr, ...) @printf(ptr @5, double %37)
  %39 = load double, ptr %0, align 8
  ret double %39
}

define void @__init__.2(ptr %self, ptr %bottomLeft, ptr %topLeft) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %bottomLeft2 = alloca ptr, align 8
  store ptr %bottomLeft, ptr %bottomLeft2, align 8
  %topLeft3 = alloca ptr, align 8
  store ptr %topLeft, ptr %topLeft3, align 8
  %0 = load ptr, ptr %self1, align 8
  %accesedbottomLeft_from_Square = getelementptr inbounds %main.gc..Square.1, ptr %0, i32 0, i32 0
  %1 = load ptr, ptr %bottomLeft2, align 8
  store ptr %1, ptr %accesedbottomLeft_from_Square, align 8
  %2 = load ptr, ptr %self1, align 8
  %accesedtopLeft_from_Square = getelementptr inbounds %main.gc..Square.1, ptr %2, i32 0, i32 1
  %3 = load ptr, ptr %topLeft3, align 8
  store ptr %3, ptr %accesedtopLeft_from_Square, align 8
  ret void
}

define void @__init__.3(ptr %self) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  ret void
}

define double @getLen(ptr %self) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %0 = load ptr, ptr %self1, align 8
  %accesedbottomLeft_from_Square = getelementptr inbounds %main.gc..Square.1, ptr %0, i32 0, i32 0
  %1 = load ptr, ptr %self1, align 8
  %accesedtopLeft_from_Square = getelementptr inbounds %main.gc..Square.1, ptr %1, i32 0, i32 1
  %2 = load ptr, ptr %accesedbottomLeft_from_Square, align 8
  %3 = load ptr, ptr %accesedtopLeft_from_Square, align 8
  %4 = call double @__sub__(ptr %2, ptr %3)
  ret double %4
}

define i64 @main() {
entry:
  %0 = alloca ptr, align 8
  %Point = alloca %main.gc..Point.0, align 8
  call void @__init__(ptr %Point, i64 0, i64 0)
  %Point1 = alloca %main.gc..Point.0, align 8
  call void @__init__(ptr %Point1, i64 10, i64 10)
  %Square = alloca %main.gc..Square.1, align 8
  call void @__init__.2(ptr %Square, ptr %Point, ptr %Point1)
  store ptr %Square, ptr %0, align 8
  %1 = load ptr, ptr %0, align 8
  %getLen_reuturn_value = call double @getLen(ptr %1)
  %2 = call i64 (ptr, ...) @printf(ptr @6, double %getLen_reuturn_value)
  %3 = load ptr, ptr %0, align 8
  %getLen_reuturn_value2 = call double @getLen(ptr %3)
  %4 = fsub double %getLen_reuturn_value2, 1.400000e+01
  %5 = fptosi double %4 to i64
  ret i64 %5
}
