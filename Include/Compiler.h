#ifndef TACHYON_COMPILER_H
#define TACHYON_COMPILER_H

#define __hot __attribute((hot))
#define __unreachable __builtin_unreachable()
#define likely(expr) __builtin_expect(!!(expr), 1)
#define unlikely(expr) __builtin_expect(!!(expr), 0)

#endif
