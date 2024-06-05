; ModuleID = 'cminus'
source_filename = "cminus"

@version = common global i32 1

declare i32 @printf(ptr, ...)

define i64 @main() {
entry:
  %version = load i32, ptr @version, align 4
  %b = alloca i32, align 4
  store i32 121, ptr %b, align 4
  %salary = alloca double, align 8
  store double 0x40B3883F7851E000, ptr %salary, align 8
  %nima = alloca i32, align 4
  store i32 -112, ptr %nima, align 4
  %salary1 = load double, ptr %salary, align 8
  %0 = fadd double %salary1, 0x4034333340000000
  store double %0, ptr %salary, align 8
  %nima2 = alloca i32, align 4
  store i32 12, ptr %nima2, align 4
  %arrayAlloc = alloca [4 x i32], align 4
  %1 = getelementptr [4 x i32], ptr %arrayAlloc, i32 0, i32 0
  store i32 1, ptr %1, align 4
  %2 = getelementptr [4 x i32], ptr %arrayAlloc, i32 0, i32 1
  store i32 12, ptr %2, align 4
  %3 = getelementptr [4 x i32], ptr %arrayAlloc, i32 0, i32 2
  store i32 23, ptr %3, align 4
  %4 = getelementptr [4 x i32], ptr %arrayAlloc, i32 0, i32 3
  store i32 43, ptr %4, align 4
  %5 = getelementptr [4 x i32], ptr %arrayAlloc, i32 0, i32 0
  %numbers = alloca ptr, align 8
  store ptr %5, ptr %numbers, align 8
  %b3 = load i32, ptr %b, align 4
  %6 = icmp eq i32 %b3, 121
  br i1 %6, label %consequence, label %else4
  %result = alloca i1, align 1
  %result5 = alloca i1, align 1
  %age = alloca i32, align 4
  %salary7 = alloca i64, align 8

consequence:                                      ; preds = %entry
  store i1 true, ptr %result, align 1
  br label %end6

else4:                                            ; preds = %entry
  store i1 false, ptr %result5, align 1
  br label %end6

end6:                                             ; preds = %else4, %consequence
  %tmpif = phi i1 [ true, %consequence ], [ false, %else4 ]
  %7 = call i1 (i32, i64, ...) @hi(i32 113, i32 21)
  store i1 %7, ptr %b, align 1
}

define i1 @hi(i32 %age, i64 %salary, ...) {
entry:
  store i32 %age, ptr %age, align 4
  store i64 %salary, ptr %salary7, align 4
  %age1 = load i32, ptr %age, align 4
  %0 = add i32 %age1, 2
  store i32 %0, ptr %age, align 4
  ret i1 false
}
