// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 seclususs

#ifndef PASCAL_GOV_COMPILER_H
#define PASCAL_GOV_COMPILER_H

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef PASCAL_GOV_RESTRICT
#if defined(__cplusplus) || defined(_MSC_VER)
#define PASCAL_GOV_RESTRICT __restrict
#else
#define PASCAL_GOV_RESTRICT restrict
#endif
#endif

#ifndef PASCAL_GOV_LIKELY
#define PASCAL_GOV_LIKELY(x) __builtin_expect(!!(x), 1)
#define PASCAL_GOV_UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif

#endif // PASCAL_GOV_COMPILER_H
