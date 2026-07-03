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

#ifndef EMEX64_PARSER_H
#define EMEX64_PARSER_H

#include <stdint.h>

typedef enum emexParserTypeValue: uint8_t {
    emexParserValueTypeString,
    emexParserValueTypeNumber,
    emexParserValueTypeBuffer,
    emexParserValueTypeOverflow
} parser_value_type_t;

typedef struct parsed_type_return {
    parser_value_type_t type;
    uint64_t value;
    uint64_t len;
} parser_return_t;

parser_return_t parse_value_from_string(const char *str);

#endif /* EMEX64_PARSER_H */
