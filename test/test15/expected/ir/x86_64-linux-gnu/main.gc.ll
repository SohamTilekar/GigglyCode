; ModuleID = 'main.gc..'
source_filename = "/mnt/soham/soham_code/GigglyCode/test/test15/src/main.gc"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%main.gc..MyException = type { i64 }

@_ZTS11MyException = private unnamed_addr constant [18 x i8] c"_ZTS11MyException\00", align 1
@_ZTVN10__cxxabiv117__class_type_infoE = external constant ptr
@_ZTI11MyException = linkonce_odr dso_local constant { ptr, ptr } { ptr getelementptr (i8, ptr @_ZTVN10__cxxabiv117__class_type_infoE, i64 16), ptr @_ZTS11MyException }
@0 = private unnamed_addr constant [21 x i8] c"Trying positive: %i\0A\00", align 1
@1 = private unnamed_addr constant [20 x i8] c"Trying negative...\0A\00", align 1
@2 = private unnamed_addr constant [29 x i8] c"This should not be printed!\0A\00", align 1
@3 = private unnamed_addr constant [32 x i8] c"Caught exception with code: %i\0A\00", align 1

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

define void @__init__(ptr %self, i64 %code) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %code2 = alloca i64, align 8
  store i64 %code, ptr %code2, align 8
  %0 = load ptr, ptr %self1, align 8
  %accesedcode_from_MyException = getelementptr inbounds %main.gc..MyException, ptr %0, i32 0, i32 0
  %1 = load i64, ptr %code2, align 8
  store i64 %1, ptr %accesedcode_from_MyException, align 8
  ret void
}

define i64 @throw_func(i64 %val) {
entry:
  %val1 = alloca i64, align 8
  store i64 %val, ptr %val1, align 8
  %0 = load i64, ptr %val1, align 8
  %1 = icmp slt i64 %0, 0
  br i1 %1, label %then, label %cont

then:                                             ; preds = %entry
  %2 = load i64, ptr %val1, align 8
  %3 = call ptr @malloc(i64 ptrtoint (ptr getelementptr (%main.gc..MyException, ptr null, i64 1) to i64))
  call void @__init__(ptr %3, i64 %2)
  %4 = call ptr @__cxa_allocate_exception(i64 ptrtoint (ptr getelementptr (%main.gc..MyException, ptr null, i32 1) to i64))
  call void @llvm.memcpy.p0.p0.i64(ptr %4, ptr %3, i64 ptrtoint (ptr getelementptr (%main.gc..MyException, ptr null, i32 1) to i64), i1 false)
  call void @__cxa_throw(ptr %4, ptr @_ZTI11MyException, ptr null)
  unreachable

cont:                                             ; preds = %entry
  %5 = load i64, ptr %val1, align 8
  %6 = mul i64 %5, 2
  ret i64 %6
}

declare ptr @__cxa_allocate_exception(i64)

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias nocapture writeonly, ptr noalias nocapture readonly, i64, i1 immarg) #0

declare void @__cxa_throw(ptr, ptr, ptr)

define i64 @main() personality ptr @__gxx_personality_v0 {
entry:
  %0 = invoke i64 @throw_func(i64 5)
          to label %invoke_cont unwind label %landing_pad

landing_pad:                                      ; preds = %invoke_cont3, %invoke_cont2, %invoke_cont1, %invoke_cont, %entry
  %1 = landingpad { ptr, i32 }
          catch ptr @_ZTI11MyException
  %ex_obj = extractvalue { ptr, i32 } %1, 0
  %sel_id = extractvalue { ptr, i32 } %1, 1
  br label %catch_dispatch

catch_dispatch:                                   ; preds = %landing_pad
  %2 = call i32 @llvm.eh.typeid.for(ptr @_ZTI11MyException)
  %3 = icmp eq i32 %sel_id, %2
  br i1 %3, label %catch_body, label %unwind_resume

try_cont:                                         ; preds = %catch_body, %invoke_cont4
  ret i64 0

invoke_cont:                                      ; preds = %entry
  %4 = invoke i64 (ptr, ...) @printf(ptr @0, i64 %0)
          to label %invoke_cont1 unwind label %landing_pad

invoke_cont1:                                     ; preds = %invoke_cont
  %5 = invoke i64 (ptr, ...) @printf(ptr @1)
          to label %invoke_cont2 unwind label %landing_pad

invoke_cont2:                                     ; preds = %invoke_cont1
  %6 = invoke i64 @throw_func(i64 -100)
          to label %invoke_cont3 unwind label %landing_pad

invoke_cont3:                                     ; preds = %invoke_cont2
  %7 = invoke i64 (ptr, ...) @printf(ptr @2)
          to label %invoke_cont4 unwind label %landing_pad

invoke_cont4:                                     ; preds = %invoke_cont3
  br label %try_cont

unwind_resume:                                    ; preds = %catch_dispatch
  resume { ptr, i32 } %1

catch_body:                                       ; preds = %catch_dispatch
  %8 = call ptr @__cxa_begin_catch(ptr %ex_obj)
  %e = alloca %main.gc..MyException, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr %e, ptr %8, i64 ptrtoint (ptr getelementptr (%main.gc..MyException, ptr null, i32 1) to i64), i1 false)
  %e_ptr = alloca ptr, align 8
  store ptr %e, ptr %e_ptr, align 8
  %9 = load ptr, ptr %e_ptr, align 8
  %accesedcode_from_MyException = getelementptr inbounds %main.gc..MyException, ptr %9, i32 0, i32 0
  %10 = load i64, ptr %accesedcode_from_MyException, align 8
  %11 = call i64 (ptr, ...) @printf(ptr @3, i64 %10)
  call void @__cxa_end_catch()
  br label %try_cont
}

declare i32 @__gxx_personality_v0(...)

; Function Attrs: nounwind memory(none)
declare i32 @llvm.eh.typeid.for(ptr) #1

declare ptr @__cxa_begin_catch(ptr)

declare void @__cxa_end_catch()

attributes #0 = { nocallback nofree nounwind willreturn memory(argmem: readwrite) }
attributes #1 = { nounwind memory(none) }
