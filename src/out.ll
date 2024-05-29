; ModuleID = 'cminus'
source_filename = "cminus"

@version = common global ptr

declare i32 @printf(ptr, ...)

define i64 @main() {
entry:
  ret ptr @version
}

define i1 @hi(...) {
entry:
}
