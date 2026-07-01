// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 seclususs

#ifndef PASCAL_GOV_GOVERNOR_H
#define PASCAL_GOV_GOVERNOR_H

#include "daemon/state.h"

int pascal_gov_governor_init(pascal_gov_context *ctx);
void pascal_gov_governor_process_cpu(pascal_gov_context *ctx);

#endif // PASCAL_GOV_GOVERNOR_H
