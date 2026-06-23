// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 seclususs

#ifndef PASCAL_GOV_DAEMON_DETECT_H
#define PASCAL_GOV_DAEMON_DETECT_H

#include <stdbool.h>

bool pascal_gov_detect_cpu_psi(void);
int pascal_gov_detect_privilege(void);

#endif // PASCAL_GOV_DAEMON_DETECT_H
