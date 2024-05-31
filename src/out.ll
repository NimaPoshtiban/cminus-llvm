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
  store double add (double 0x40B38820C0000000, double 0x3FBEB851E0000000), ptr %salary, align 8
  %nima = alloca i32, align 4
  store i32 -112, ptr %nima, align 4
  %salary1 = load double, ptr %salary, align 8
  %nima2 = alloca i32, align 4
  store i32 12, ptr %nima2, align 4
  %b3 = load i32, ptr %b, align 4
  %0 = icmp eq i32 %b3, 121
  br i1 %0, label %consequence, label %else4
  %result = alloca i1, align 1
  %result5 = alloca i1, align 1

consequence:                                      ; preds = %entry
  store i1 true, ptr %result, align 1
  br label %end6

else4:                                            ; preds = %entry
  store i1 false, ptr %result5, align 1
  br label %end6

end6:                                             ; preds = %else4, %consequence
  %tmpif = phi i1 [ true, %consequence ], [ false, %else4 ]
  br label %condition

condition:                                        ; preds = %body8, %end6
  %b7 = load i32, ptr %b, align 4
  %1 = icmp sgt i32 %b7, 3
  br i1 %1, label %body8, label %end10

body8:                                            ; preds = %condition
  %b9 = load i32, ptr %b, align 4
  %2 = sub i32 %b9, 1
  store i32 %2, ptr %b, align 4
  br label %condition

end10:                                            ; preds = %condition
}

define i1 @hi(i32 %age, i64 %salary, ...) {
entry:
}
