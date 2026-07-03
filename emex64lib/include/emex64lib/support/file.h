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

#ifndef EMEX64_FILE_H
#define EMEX64_FILE_H

#include <stdint.h>
#include <stdbool.h>
#include <emex64lib/support/virtual/vbitwalker.h>
#include <emex64lib/support/virtual/vfd.h>

typedef enum: uint8_t {
    kEmexFileTypeUnknown,
    kEmexFileTypeDirectory,
    kEmexFileTypeAssembly,
    kEmexFileTypeAssemblyIncludation,
    kEmexFileTypeC,
    kEmexFileTypeCHeader,
    kEmexFileTypeCXX,
    kEmexFileTypeCXXHeader,
    kEmexFileTypeObjC,
    kEmexFileTypeObjCXX,
    kEmexFileTypeObject
} kEmexFileType;

typedef enum: uint8_t {
    kEmexFilePolicyPermissionRead =     0b00000001,
    kEmexFilePolicyPermissionWrite =    0b00000010,
    kEmexFilePolicyPermissionExecute =  0b00000100,
} kEmexFilePolicyPermission;

typedef struct emex_file_policy {
    kEmexFilePolicyPermission needed_permission;    /* permissions them selves */
    bool must_exist;
    bool must_be_file;
    bool create_on_open;
} emex_file_policy_t;

extern emex_file_policy_t in_data_file_policy;
extern emex_file_policy_t out_data_file_policy;
extern emex_file_policy_t out_nocreate_file_policy;

typedef struct emex_file {
    char *path;
    char *content;          /* mapped file contents */
    size_t len;             /* lenght of the mapped file contents */
    vfd_t *d;               /* file descriptor that gets duped by emex_file_dup_fd */
    VpageObjRef vpageObjRef;   /* virtual page object */
    kEmexFileType type;
    emex_file_policy_t policy;
} emex_file_t;

emex_file_t *emex_file_alloc(const char *path, emex_file_policy_t policy);
emex_file_t *emex_file_alloc_vfd(const char *path, emex_file_policy_t policy, vfd_t *d);
emex_file_t *emex_file_alloc_unsaved(const char *path, emex_file_policy_t policy, const char *content);
void emex_file_dealloc(emex_file_t *f);

bool emex_file_open(emex_file_t *f);
void emex_file_close(emex_file_t *f);

vfd_t *emex_file_dup_vfd(emex_file_t *f);
vbitwalker_t *emex_file_dup_vbitwalker(emex_file_t *f, bw_endian_t endian);

bool emex_file_map(emex_file_t *f);
void emex_file_unmap(emex_file_t *f);

void emex_file_unlink(emex_file_t *f);

kEmexFileType emex_file_type_for_path(const char *path, bool must_exist);

#endif /* EMEX64_FILE_H */
