/*
 * MIT License
 *
 * Copyright (c) 2026 emexlab
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef EMEX64_FILE_H
#define EMEX64_FILE_H

#include <stdint.h>
#include <stdbool.h>

#include <emex64lib/support/fdwalker.h>

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
    kEmexFileInstanceTypeSaved,
    kEmexFileInstanceTypeUnsaved,
} kEmexFileInstanceType;

typedef enum: uint8_t {
    kEmexFilePolicyPermissionRead =     0b00000001,
    kEmexFilePolicyPermissionWrite =    0b00000010,
    kEmexFilePolicyPermissionExecute =  0b00000100,
} kEmexFilePolicyPermission;

typedef struct emex_file_policy {
    kEmexFilePolicyPermission needed_permission;    /* permissions them selves */
    bool must_exist;
    bool must_be_file;
} emex_file_policy_t;

typedef struct emex_file {
    const char *path;
    const char *content;    /* mapped file contents */
    int fd;                 /* file descriptor that gets duped by emex_file_dup_fd */
    size_t len;
    kEmexFileType type;
    kEmexFileInstanceType instance_type;
    emex_file_policy_t policy;
} emex_file_t;

emex_file_t *emex_file_alloc(const char *path, emex_file_policy_t policy);
emex_file_t *emex_file_alloc_unsaved(const char *path, emex_file_policy_t policy, const char *content);
void emex_file_dealloc(emex_file_t *f);

bool emex_file_open(emex_file_t *f);
void emex_file_close(emex_file_t *f);

bool emex_file_map(emex_file_t *f);
void emex_file_unmap(emex_file_t *f);

kEmexFileType emex_file_type_for_path(const char *path, bool must_exist);

#endif /* EMEX64_FILE_H */
