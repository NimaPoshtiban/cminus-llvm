; ModuleID = 'cminus'
source_filename = "cminus"

@version = common global i32 1

declare i32 @printf(ptr, ...)

define i64 @main() {
entry:
  %version = load i32, ptr @version, align 4
  ret i32 %version
  %b = alloca i32, align 4
  store i32 121, ptr %b, align 4
  %salary = alloca double, align 8
  store double add (double 0x40B38820C0000000, double 0x3FBEB851E0000000), ptr %salary, align 8
  %nima = alloca i32, align 4
  store i32 -112, ptr %nima, align 4
  %nima1 = alloca i32, align 4
  store i32 12, ptr %nima1, align 4
  %nima2 = load i32, ptr %nima1, align 4
  ret ptr %nima1
  %b3 = load i32, ptr %b, align 4
  ret ptr %b
  %0 = fcmp oeq void <badref>, i32 121
  br i1 %0, label %consequence, label %else4
  %result = alloca i1, align 1
  %result4 = alloca i1, align 1

consequence:                                      ; preds = %entry
  store i1 true, ptr %result, align 1
  br label %end

else:                                             ; preds = %entry
  store i1 false, ptr %result4, align 1
  br label %end

end:                                              ; preds = %else, %consequence
  %tmpif = phi i1 [ true, %consequence ], [ false, %else ]
}

define i1 @hi(i32 %age, i64 %salary, ...) {
entry:
}
