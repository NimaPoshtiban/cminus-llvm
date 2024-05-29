; ModuleID = 'cminus'
source_filename = "cminus"

@version = common global ptr
@b = common global i64

declare i32 @printf(ptr, ...)

define i64 @main() {
entry:
  ret i64 121
}
