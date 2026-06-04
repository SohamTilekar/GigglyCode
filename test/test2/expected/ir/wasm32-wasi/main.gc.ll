; ModuleID = 'main.gc..'
source_filename = "/mnt/soham/soham_code/GigglyCode/test/test2/src/main.gc"
target datalayout = "e-m:e-p:32:32-p10:8:8-p20:8:8-i64:64-n32:64-S128-ni:1:10:20"
target triple = "wasm32-unknown-wasi"

%main.gc..LinkedList = type { ptr }
%main.gc..ListNode.0 = type { i64, ptr }

@0 = private unnamed_addr constant [23 x i8] c"List node value: %lld\0A\00", align 1

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
  %0 = alloca ptr, align 4
  %1 = alloca ptr, align 4
  %LinkedList = alloca %main.gc..LinkedList, align 8
  call void @__init__.1(ptr %LinkedList)
  %LinkedList1 = alloca %main.gc..LinkedList, align 8
  call void @__init__.1(ptr %LinkedList1)
  store ptr %LinkedList1, ptr %1, align 4
  %2 = load ptr, ptr %1, align 4
  call void @add(ptr %2, i64 10)
  %3 = load ptr, ptr %1, align 4
  call void @add(ptr %3, i64 20)
  %4 = load ptr, ptr %1, align 4
  call void @add(ptr %4, i64 30)
  %5 = load ptr, ptr %1, align 4
  %accesedhead_from_LinkedList = getelementptr inbounds %main.gc..LinkedList, ptr %5, i32 0, i32 0
  %6 = load ptr, ptr %accesedhead_from_LinkedList, align 4
  store ptr %6, ptr %0, align 4
  br label %cond

cond:                                             ; preds = %body, %entry
  %7 = load ptr, ptr %0, align 4
  %8 = icmp ne ptr %7, null
  br i1 %8, label %body, label %cont

body:                                             ; preds = %cond
  %9 = load ptr, ptr %0, align 4
  %accesedvalue_from_ListNode = getelementptr inbounds %main.gc..ListNode.0, ptr %9, i32 0, i32 0
  %10 = load i64, ptr %accesedvalue_from_ListNode, align 8
  %11 = call i64 (ptr, ...) @printf(ptr @0, i64 %10)
  %12 = load ptr, ptr %0, align 4
  %accesednext_from_ListNode = getelementptr inbounds %main.gc..ListNode.0, ptr %12, i32 0, i32 1
  %13 = load ptr, ptr %accesednext_from_ListNode, align 4
  store ptr %13, ptr %0, align 4
  br label %cond

cont:                                             ; preds = %cond
  ret i64 0
}

define void @__init__(ptr %self, i64 %value, ptr %next) {
entry:
  %self1 = alloca ptr, align 4
  store ptr %self, ptr %self1, align 4
  %value2 = alloca i64, align 8
  store i64 %value, ptr %value2, align 8
  %next3 = alloca ptr, align 4
  store ptr %next, ptr %next3, align 4
  %0 = load ptr, ptr %self1, align 4
  %accesedvalue_from_ListNode = getelementptr inbounds %main.gc..ListNode.0, ptr %0, i32 0, i32 0
  %1 = load i64, ptr %value2, align 8
  store i64 %1, ptr %accesedvalue_from_ListNode, align 8
  %2 = load ptr, ptr %self1, align 4
  %accesednext_from_ListNode = getelementptr inbounds %main.gc..ListNode.0, ptr %2, i32 0, i32 1
  %3 = load ptr, ptr %next3, align 4
  store ptr %3, ptr %accesednext_from_ListNode, align 4
  ret void
}

define void @__init__.1(ptr %self) {
entry:
  %self1 = alloca ptr, align 4
  store ptr %self, ptr %self1, align 4
  %0 = load ptr, ptr %self1, align 4
  %accesedhead_from_LinkedList = getelementptr inbounds %main.gc..LinkedList, ptr %0, i32 0, i32 0
  store ptr null, ptr %accesedhead_from_LinkedList, align 4
  ret void
}

define void @add(ptr %self, i64 %value) {
entry:
  %0 = alloca ptr, align 4
  %1 = alloca ptr, align 4
  %self1 = alloca ptr, align 4
  store ptr %self, ptr %self1, align 4
  %value2 = alloca i64, align 8
  store i64 %value, ptr %value2, align 8
  %2 = load i64, ptr %value2, align 8
  %3 = call ptr @malloc(i64 ptrtoint (ptr getelementptr (%main.gc..ListNode.0, ptr null, i64 1) to i64))
  call void @__init__(ptr %3, i64 %2, ptr null)
  store ptr %3, ptr %1, align 4
  %4 = load ptr, ptr %self1, align 4
  %accesedhead_from_LinkedList = getelementptr inbounds %main.gc..LinkedList, ptr %4, i32 0, i32 0
  %5 = load ptr, ptr %accesedhead_from_LinkedList, align 4
  %6 = icmp eq ptr %5, null
  br i1 %6, label %then, label %else

then:                                             ; preds = %entry
  %7 = load ptr, ptr %self1, align 4
  %accesedhead_from_LinkedList3 = getelementptr inbounds %main.gc..LinkedList, ptr %7, i32 0, i32 0
  %8 = load ptr, ptr %1, align 4
  store ptr %8, ptr %accesedhead_from_LinkedList3, align 4
  br label %cont

cont:                                             ; preds = %cont5, %then
  ret void

else:                                             ; preds = %entry
  %9 = load ptr, ptr %self1, align 4
  %accesedhead_from_LinkedList4 = getelementptr inbounds %main.gc..LinkedList, ptr %9, i32 0, i32 0
  %10 = load ptr, ptr %accesedhead_from_LinkedList4, align 4
  store ptr %10, ptr %0, align 4
  br label %cond

cond:                                             ; preds = %body, %else
  %11 = load ptr, ptr %0, align 4
  %accesednext_from_ListNode = getelementptr inbounds %main.gc..ListNode.0, ptr %11, i32 0, i32 1
  %12 = load ptr, ptr %accesednext_from_ListNode, align 4
  %13 = icmp ne ptr %12, null
  br i1 %13, label %body, label %cont5

body:                                             ; preds = %cond
  %14 = load ptr, ptr %0, align 4
  %accesednext_from_ListNode6 = getelementptr inbounds %main.gc..ListNode.0, ptr %14, i32 0, i32 1
  %15 = load ptr, ptr %accesednext_from_ListNode6, align 4
  store ptr %15, ptr %0, align 4
  br label %cond

cont5:                                            ; preds = %cond
  %16 = load ptr, ptr %0, align 4
  %accesednext_from_ListNode7 = getelementptr inbounds %main.gc..ListNode.0, ptr %16, i32 0, i32 1
  %17 = load ptr, ptr %1, align 4
  store ptr %17, ptr %accesednext_from_ListNode7, align 4
  br label %cont
}
