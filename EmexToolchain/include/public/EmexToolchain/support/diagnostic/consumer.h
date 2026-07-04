/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Copyright (C) 2026 emexlab
 *
 * This file is part of emex64.
 *
 * emex64 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * emex64 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with emex64. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef EMEX64_DIAGNOSTIC_CONSUMER_H
#define EMEX64_DIAGNOSTIC_CONSUMER_H

#include <stdint.h>
#include <EmexToolchain/support/diagnostic/diagnostic.h>

#define C_BOLD "\x1b[1m"
#define C_CARET "\x1b[0;1;32m"
#define C_FIXIT "\x1b[0;1;32m"
#define C_NOTE "\x1b[0;1;36m"
#define C_REMARK "\x1b[0;1;34m"
#define C_WARN "\x1b[0;1;35m"
#define C_ERROR "\x1b[0;1;31m"
#define C_FATAL "\x1b[0;1;31m"
#define C_RESET "\x1b[0m"

typedef struct diagnostic_consumer diagnostic_consumer_t;

typedef void (*diagnostic_consumer_consume_diagnostic_handler)(diagnostic_consumer_t*,diagnostic_t*);

/* mostly used as a header */
typedef struct diagnostic_consumer {
    diagnostic_consumer_consume_diagnostic_handler consume_handler;
    void *ctx;
} diagnostic_consumer_t;

void diagnostic_report(diagnostic_consumer_t *consumer, kDiagnosticSeverity severity, diagnostic_location_t *location, char *str, ...);

#endif /* EMEX64_DIAGNOSTIC_CONSUMER_H */
