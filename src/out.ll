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
  store double 0x40B3883F7851E000, ptr %salary, align 8
  %nima = alloca i32, align 4
  store i32 -112, ptr %nima, align 4
  %nima1 = alloca i32, align 4
  store i32 12, ptr %nima1, align 4
  %nima2 = load i32, ptr %nima1, align 4
  ret ptr %nima1
  %nima3 = load i32, ptr %nima1, align 4
  ret ptr %nima1
}

define i1 @hi(...) {
entry:
}
