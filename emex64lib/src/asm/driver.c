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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <spawn.h>
#include <sys/wait.h>

#include <emex64lib/support/diagnostic/legacy.h>

#include <emex64lib/asm/driver.h>
#include <emex64lib/asm/invocation.h>

#include <emex64lib/linker/driver.h>

extern char **environ;

assembler_job_t *assembler_job_alloc(assembler_job_t *prev,
                                     kAssemblerJobType type,
                                     const char *command,
                                     const char **argv,
                                     int argc)
{
    /* whitelisting job types */
    switch(type)
    {
        case kAssemblerJobTypeAssembler:
        case kAssemblerJobTypeLinker:
        case kAssemblerJobTypeDriver:
            break;
        default:
            /* illegal job type */
            return NULL;
    }

    /* allocating job (definetly AI generated btw xD I definetly didn't wrote this comments, cuz it is wayyyyy to generic right) */
    assembler_job_t *job = malloc(sizeof(assembler_job_t));
    if(job == NULL)
    {
        return NULL;
    }

    job->type = type;

    /* copy command */
    job->command = strdup(command);
    if(job->command == NULL)
    {
        free(job);
        return NULL;
    }

    /* copy arguments */
    job->argc = argc;
    job->argv = calloc(argc + 1, sizeof(char*));
    if(job->argv == NULL)
    {
        free(job->command);
        free(job);
        return NULL;
    }

    for(int i = 0; i < argc; i++)
    {
        job->argv[i] = strdup(argv[i]);
        if(job->argv[i] == NULL)
        {
            i--;
            while(i < 0)
            {
                free(job->argv[--i]);
            }

            free(job->command);
            free(job);
            return NULL;
        }
    }

    job->argv[argc] = NULL;

    job->prev = NULL;
    job->next = NULL;

    if(prev != NULL)
    {
        prev->next = job;
        job->prev = prev;
    }

    return job;
}

void assembler_job_dealloc(assembler_job_t *job)
{
    for(int i = 0; i < job->argc; i++)
    {
        free(job->argv[i]);
    }
    free(job->argv);

    if(job->prev != NULL)
    {
        job->prev->next = job->next;
    }

    if(job->next != NULL)
    {
        job->next->prev = job->prev;
    }

    free(job->command);
    free(job);
}

bool assembler_driver_predrive(assembler_driver_t *driver,
                               const char **argv,
                               int argc)
{
    driver->page_align = true;
    driver->warning_error = false;
    driver->warning_deprecated = true;
    driver->caret_diagnostics = true;

    driver->output_path = NULL;
    driver->input_path_count = 0;
    driver->input_path = calloc(argc, sizeof(char *));
    driver->input_path_type = calloc(argc, sizeof(kEmexFileType));

    driver->inc_dir_cnt = 0;
    driver->inc_dirs = NULL;

    driver->macro_cnt = 0;
    driver->macro = NULL;

    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            fprintf(stderr, "Usage: %s [options] file...\n", argv[0]);
            fprintf(stderr, "  -o [output path]     Sets the output file path, set to \"a.out\" if not set.\n");
            fprintf(stderr, "  -c                   Assemble the source files, but do not link.\n");
            fprintf(stderr, "  -r                   Relocatable object mode, meaning a ELF object will be emitted out of all assembly files.\n");
            fprintf(stderr, "  --in-process         All jobs are handled within the same process instead of executing subprocesses.\n");
            fprintf(stderr, "  -D macro[=value]     Defines an assembler macro, set to 1 when no value is given.\n");
            fprintf(stderr, "  -I dir               Adds a directory to the include search path.\n");
            fprintf(stderr, "  -Wl,<arg>,...        Pass the comma separated arguments to the linker.\n");
            fprintf(stderr, "\n");
            fprintf(stderr, "  -fpage-align         The assembler will align sections to a page boundary.\n");
            fprintf(stderr, "  -fcaret-diagnostics  The assembler will print diagnostics showing their caret positions.\n");
            fprintf(stderr, "                       Each feature flag can be reversed by prefixing it with a \"no\" (i.e -fno-page-align).\n");
            fprintf(stderr, "\n");
            fprintf(stderr, "  -Werror              The assembler will treat every warning as a error.\n");
            fprintf(stderr, "  -Wdeprecated         The assembler will print a warning on every as deprecated marked symbol or internal features.\n");
            fprintf(stderr, "                       Each warning flag can be reversed by prefixing it with a \"no\" (i.e -Wno-error).\n");
            return false;
        }
        else if(strcmp(argv[i], "-o") == 0 && i + 1 < argc)
        {
            driver->output_path = strdup(argv[++i]);
        }
        else if(strncmp(argv[i], "-f", 2) == 0)
        {
            const char *flag;
            if(argv[i][2] != '\0')
            {
                flag = argv[i] + 2;
            }
            else if(i + 1 < argc)
            {
                flag = argv[++i];
            }
            else
            {
                diag_error(NULL, "missing argument to '-f'\n");
                return false;
            }

            if(strcmp(flag, "page-align") == 0)
            {
                driver->page_align = true;
            }
            else if(strcmp(flag, "no-page-align") == 0)
            {
                driver->page_align = false;
            }
            else if(strcmp(flag, "caret-diagnostics") == 0)
            {
                driver->caret_diagnostics = true;
            }
            else if(strcmp(flag, "no-caret-diagnostics") == 0)
            {
                driver->caret_diagnostics = false;
            }
            else
            {
                diag_error(NULL, "unknown feature flag '%s'\n", flag);
                return false;
            }
        }
        else if(strncmp(argv[i], "-Wl,", 4) == 0)
        {
            const char *p = argv[i] + 4;
            if(*p == '\0')
            {
                diag_error(NULL, "missing argument to '-Wl,'\n");
                return false;
            }

            while(*p != '\0')
            {
                const char *comma = strchr(p, ',');
                size_t len = comma ? (size_t)(comma - p) : strlen(p);

                char *arg = malloc(len + 1);
                memcpy(arg, p, len);
                arg[len] = '\0';

                driver->linker_flags = realloc(driver->linker_flags, (driver->linker_flags_cnt + 1) * sizeof(char *));
                driver->linker_flags[driver->linker_flags_cnt++] = arg;

                p = comma ? comma + 1 : p + len;
            }
        }
        else if(strncmp(argv[i], "-W", 2) == 0)
        {
            const char *flag;
            if(argv[i][2] != '\0')
            {
                flag = argv[i] + 2;
            }
            else if(i + 1 < argc)
            {
                flag = argv[++i];
            }
            else
            {
                diag_error(NULL, "missing argument to '-W'\n");
                return false;
            }

            if(strcmp(flag, "error") == 0)
            {
                driver->warning_error = true;
            }
            else if(strcmp(flag, "no-error") == 0)
            {
                driver->warning_error = false;
            }
            else if(strcmp(flag, "deprecated") == 0)
            {
                driver->warning_deprecated = true;
            }
            else if(strcmp(flag, "no-deprecated") == 0)
            {
                driver->warning_deprecated = false;
            }
            else
            {
                diag_error(NULL, "unknown warning flag '%s'\n", flag);
                return false;
            }
        }
        else if(strncmp(argv[i], "-D", 2) == 0)
        {
            const char *flag;
            if(argv[i][2] != '\0')
            {
                flag = argv[i] + 2;
            }
            else if(i + 1 < argc)
            {
                flag = argv[++i];
            }
            else
            {
                diag_error(NULL, "missing argument to '-D'\n");
                return false;
            }

            const char *eq = strchr(flag, '=');
            char *match = NULL;
            char *value = NULL;
            if(!eq)
            {
                match = strdup(flag);
                size_t len = strlen(match);
                if(len > 0 && (match[len-1] == '"' || match[len-1] == '\''))
                {
                    match[len-1] = '\0';
                }
                value = strdup("1");
                goto early_macro_append;
            }

            size_t name_len = eq - flag;
            match = malloc(name_len + 1);
            memcpy(match, flag, name_len);
            match[name_len] = '\0';

            const char *v = eq + 1;
            char quote = 0;

            if(*v == '"' || *v == '\'')
            {
                quote = *v;
                v++;
            }

            const char *end = v;
            while (*end && *end != quote && *end != '\0') end++;

            size_t val_len = end - v;
            value = malloc(val_len + 1);
            memcpy(value, v, val_len);
            value[val_len] = '\0';

        early_macro_append:
            {
                uint64_t macro_slot = driver->macro_cnt++;
                if(driver->macro == NULL)
                {
                    driver->macro = calloc(driver->macro_cnt, sizeof(assembler_macro_definition_t));
                }
                else
                {
                    driver->macro = realloc(driver->macro, driver->macro_cnt * sizeof(assembler_macro_definition_t));
                }

                driver->macro[macro_slot].match = match;
                driver->macro[macro_slot].value = value;
            }
        }
        else if(strncmp(argv[i], "-I", 2) == 0)
        {
            const char *dir;
            if(argv[i][2] != '\0')
            {
                dir = argv[i] + 2;
            }
            else if(i + 1 < argc)
            {
                dir = argv[++i];
            }
            else
            {
                diag_error(NULL, "missing argument to '-I'\n");
                return false;
            }
            driver->inc_dirs = realloc(driver->inc_dirs, (driver->inc_dir_cnt + 1) * sizeof(char*));
            driver->inc_dirs[driver->inc_dir_cnt++] = strdup(dir);
        }
        else if(strncmp(argv[i], "-c", 2) == 0)
        {
            driver->emit_object = true;
        }
        else if(strncmp(argv[i], "-v", 2) == 0)
        {
            driver->verbose = true;
        }
        else if(strncmp(argv[i], "-r", 2) == 0)
        {
            driver->relocatable = true;
        }
        else if(strncmp(argv[i], "--in-process", 12) == 0)
        {
            driver->in_process = true;
        }
        else if(argv[i][0] != '-')
        {
            driver->input_path[driver->input_path_count] = strdup(argv[i]);
            driver->input_path_type[driver->input_path_count++] = emex_file_type_for_path(argv[i], true);
        }
        else
        {
            diag_error(NULL, "unknown option '%s'\n", argv[i]);
            return false;
        }
    }

    if(driver->input_path_count <= 0)
    {
        diag_error(NULL, "no input files\n");
        return false;
    }

    if(driver->output_path == NULL)
    {
        diag_warn(NULL, "no output path provided, falling back to 'a.out'\n");
        driver->output_path = strdup("a.out");
    }

    return true;
}

static char *assembler_driver_tmppath(assembler_driver_t *driver,
                                      const char *input_path)
{
    const char *base = strrchr(input_path, '/');
    base = base ? base + 1 : input_path;
    const char *dot = strrchr(base, '.');
    size_t stem_len = dot ? (size_t)(dot - base) : strlen(base);

    const char *tmpdir = getenv("TMPDIR");
    if(tmpdir == NULL || tmpdir[0] == '\0')
    {
        tmpdir = "/tmp";
    }

    size_t len = strlen(tmpdir) + 1 + 7 + stem_len + 1 + 6 + 2 + 1;
    char *path = malloc(len);
    if(path == NULL)
    {
        return NULL;
    }

    snprintf(path, len, "%s/emex64-%.*s-XXXXXX.o", tmpdir, (int)stem_len, base);

    int fd = mkstemps(path, 2);
    if(fd < 0)
    {
        free(path);
        return NULL;
    }
    close(fd);

    driver->tmp_paths = realloc(driver->tmp_paths, (driver->tmp_path_cnt + 1) * sizeof(char *));
    driver->tmp_paths[driver->tmp_path_cnt++] = path;

    return path;
}

static void assembler_driver_append_additional_linker_flag(assembler_driver_t *driver,
                                                           char *flag)
{
    driver->linker_flags = realloc(driver->linker_flags, (driver->linker_flags_cnt + 1) * sizeof(char *));
    driver->linker_flags[driver->linker_flags_cnt++] = strdup(flag);
}

bool assembler_driver_jobgen(assembler_driver_t *driver)
{
    if(driver->emit_object && driver->input_path_count > 1)
    {
        diag_error(NULL, "multiple input files were passed in object emit mode\n");
        return false;
    }

    for(int i = 0; i < driver->input_path_count; i++)
    {
        switch(driver->input_path_type[i])
        {
            case kEmexFileTypeAssembly:
            case kEmexFileTypeAssemblyIncludation:
            {
                int argc = 0;
                char **argv = calloc(1024, sizeof(char*));
                if(argv == NULL)
                {
                    return false;
                }

                argv[argc++] = strdup("emex64asm");
                if(driver->verbose)
                {
                    argv[argc++] = strdup("-v");
                }
                argv[argc++] = strdup("-c");
                argv[argc++] = strdup("-o");
                argv[argc++] = strdup(assembler_driver_tmppath(driver, driver->input_path[i]));
                argv[argc++] = strdup(driver->input_path[i]);
                argv[argc++] = driver->page_align ? strdup("-fpage-align") : strdup("-fno-page-align");
                argv[argc++] = driver->warning_error ? strdup("-Werror") : strdup("-Wno-error");
                argv[argc++] = driver->warning_deprecated ? strdup("-Wdeprecated") : strdup("-Wno-deprecated");
                argv[argc++] = driver->caret_diagnostics ? strdup("-fcaret-diagnostics") : strdup("-fno-caret-diagnostics");
                for(size_t j = 0; j < driver->inc_dir_cnt; j++)
                {
                    size_t ilen = strlen(driver->inc_dirs[j]);
                    char *new_buf = malloc(ilen + 3);
                    snprintf(new_buf, ilen + 3, "-I%s", driver->inc_dirs[j]);
                    argv[argc++] = new_buf;
                }
                for(uint64_t j = 0; j < driver->macro_cnt; j++)
                {
                    const char *m = driver->macro[j].match;
                    const char *v = driver->macro[j].value;

                    size_t blen = 2 + strlen(m) + 1 + strlen(v) + 1;
                    char *buf = malloc(blen);
                    if(buf == NULL)
                    {
                        return false;
                    }
                    snprintf(buf, blen, "-D%s=%s", m, v);
                    argv[argc++] = buf;
                }

                driver->job = assembler_job_alloc(driver->job, (driver->emit_object) ? kAssemblerJobTypeAssembler : kAssemblerJobTypeDriver, "emex64asm", (const char**)argv, argc);

                for(int j = 0; j < argc; j++)
                {
                    free(argv[j]);
                }
                free(argv);
                break;
            }
            case kEmexFileTypeObject:
                assembler_driver_append_additional_linker_flag(driver, driver->input_path[i]);
                break;
            default:
                diag_error(NULL, "unknown or non existing input file '%s'\n", driver->input_path[i]);
                return false;
        }
    }

    if(!driver->emit_object)
    {
        int argc = 0;
        char **argv = calloc(1024, sizeof(char*));
        if(argv == NULL)
        {
            return false;
        }

        argv[argc++] = strdup("emex64ld");
        if(driver->relocatable)
        {
            argv[argc++] = strdup("-r");
        }
        if(driver->verbose)
        {
            argv[argc++] = strdup("-v");
        }
        argv[argc++] = strdup("-o");
        argv[argc++] = strdup(driver->output_path);
        for(size_t i = 0; i < driver->tmp_path_cnt; i++)
        {
            argv[argc++] = strdup(driver->tmp_paths[i]);
        }
        for(int i = 0; i < driver->linker_flags_cnt; i++)
        {
            argv[argc++] = strdup(driver->linker_flags[i]);
        }

        driver->job = assembler_job_alloc(driver->job, kAssemblerJobTypeLinker, "emex64ld", (const char**)argv, argc);

        for(int j = 0; j < argc; j++)
        {
            free(argv[j]);
        }
        free(argv);
    }

    assembler_job_t *job = driver->job;
    while(job != NULL)
    {
        driver->job = job;
        job = job->prev;
    }

    return true;
}

const char *assembler_job_string_for_type(kAssemblerJobType type)
{
    switch(type)
    {
        case kAssemblerJobTypeAssembler:
            return "assembler";
        case kAssemblerJobTypeLinker:
            return "linker";
        case kAssemblerJobTypeDriver:
            return "driver";
        default:
            return "unknown";
    }
}

assembler_driver_t *assembler_driver_alloc(const char **argv,
                                           int argc)
{
    assembler_driver_t *driver = calloc(1, sizeof(assembler_driver_t));
    if(driver == NULL)
    {
        return NULL;
    }

    if(!assembler_driver_predrive(driver, argv, argc) ||
       !assembler_driver_jobgen(driver))
    {
        assembler_driver_dealloc(driver);
        return NULL;
    }

    if(driver->verbose)
    {
        fprintf(stderr, "---- driver ----\n");
        fprintf(stderr, "page_align: %d\n", driver->page_align);
        fprintf(stderr, "warning_error: %d\n", driver->warning_error);
        fprintf(stderr, "warning_deprecated: %d\n", driver->warning_deprecated);
        fprintf(stderr, "emit_object: %d\n", driver->emit_object);
        fprintf(stderr, "verbose: %d\n", driver->verbose);
        fprintf(stderr, "in_process: %d\n", driver->in_process || driver->emit_object);
        fprintf(stderr, "output_path: %s\n", driver->output_path);

        fprintf(stderr, "input_path[%d]: { ", driver->input_path_count);
        for(int i = 0; i < driver->input_path_count; i++)
        {
            if(i != 0)
            {
                fprintf(stderr, ", ");
            }
            fprintf(stderr, "%s", driver->input_path[i]);
        }
        fprintf(stderr, " }\n");

        fprintf(stderr, "inc_dirs[%zu]: { ", driver->inc_dir_cnt);
        for(size_t i = 0; i < driver->inc_dir_cnt; i++)
        {
            if(i != 0)
            {
                fprintf(stderr, ", ");
            }
            fprintf(stderr, "%s", driver->inc_dirs[i]);
        }
        fprintf(stderr, " }\n");

        fprintf(stderr, "macro[%llu]: { ", (unsigned long long)driver->macro_cnt);
        for(uint64_t i = 0; i < driver->macro_cnt; i++)
        {
            if(i != 0)
            {
                fprintf(stderr, ", ");
            }
            fprintf(stderr, "(match='%s' | replacement='%s')", driver->macro[i].match, driver->macro[i].value);
        }
        fprintf(stderr, " }\n");

        fprintf(stderr, "linker_flags[%d]: { ", driver->linker_flags_cnt);
        for(int i = 0; i < driver->linker_flags_cnt; i++)
        {
            if(i != 0)
            {
                fprintf(stderr, ", ");
            }
            fprintf(stderr, "%s", driver->linker_flags[i]);
        }
        fprintf(stderr, " }\n");

        fprintf(stderr, "jobs: {");
        assembler_job_t *job = driver->job;
        if(job != NULL)
        {
            fprintf(stderr, "\n");
        }
        while(job != NULL)
        {
            fprintf(stderr, "\t{\n");
            fprintf(stderr, "\t\ttype: %s\n", assembler_job_string_for_type(job->type));
            fprintf(stderr, "\t\tcommand: %s\n", job->command);
            fprintf(stderr, "\t\targv[%d]: { ", job->argc);
            for(int i = 0; i < job->argc; i++)
            {
                if(i != 0)
                {
                    fprintf(stderr, ", ");
                }
                fprintf(stderr, "%s", job->argv[i]);
            }
            fprintf(stderr, " }\n");
            fprintf(stderr, "\t\tprev: %p\n ", (void*)job->prev);
            fprintf(stderr, "\t\tnext: %p\n ", (void*)job->next);
            fprintf(stderr, "\t}");
            fprintf(stderr, "\n");

            job = job->next;
        }
        fprintf(stderr, "}\n");
    }

    return driver;
}

void assembler_driver_dealloc(assembler_driver_t *driver)
{
    free(driver->output_path);

    for(int i = 0; i < driver->input_path_count; i++)
    {
        free(driver->input_path[i]);
    }
    free(driver->input_path);
    free(driver->input_path_type);

    for(size_t i = 0; i < driver->inc_dir_cnt; i++)
    {
        free(driver->inc_dirs[i]);
    }
    free(driver->inc_dirs);

    for(uint64_t i = 0; i < driver->tmp_path_cnt; i++)
    {
        unlink(driver->tmp_paths[i]);
        free(driver->tmp_paths[i]);
    }
    free(driver->tmp_paths);

    for(uint64_t i = 0; i < driver->macro_cnt; i++)
    {
        free(driver->macro[i].match);
        free(driver->macro[i].value);
    }
    free(driver->macro);

    for(int i = 0; i < driver->linker_flags_cnt; i++)
    {
        free(driver->linker_flags[i]);
    }
    free(driver->linker_flags);

    assembler_job_t *job = driver->job;
    while(job != NULL)
    {
        assembler_job_t *next = job->next;
        assembler_job_dealloc(job);
        job = next;
    }

    free(driver);
}

bool assembler_driver_drive_the_fucking_car(assembler_driver_t *driver)
{
    if(driver->emit_object)
    {
        assembler_options_t options = assembler_options_default;
        options.page_align = driver->page_align;
        options.warning_error = driver->warning_error;
        options.warning_deprecated = driver->warning_deprecated;
        options.caret_diagnostics = driver->caret_diagnostics;

        assembler_invocation_t *inv = assembler_invocation_alloc(options);
        if(inv == NULL)
        {
            return false;
        }

        inv->definition_cnt = driver->macro_cnt;
        inv->definition = driver->macro;
        inv->include_dir_cnt = driver->inc_dir_cnt;
        inv->include_dirs = driver->inc_dirs;

        emex_file_t *input = emex_file_alloc(driver->input_path[0], assembly_file_policy);
        if(input == NULL)
        {
            assembler_invocation_dealloc(inv);
            return false;
        }

        emex_file_t *output = emex_file_alloc(driver->output_path, object_file_out_policy);
        if(output == NULL)
        {
            emex_file_dealloc(output);
            assembler_invocation_dealloc(inv);
            return false;
        }

        bool success = assembler_invocation_emit(inv, input, output);

        emex_file_dealloc(output);
        emex_file_dealloc(input);
        assembler_invocation_dealloc(inv);

        return success;
    }
    else
    {
        assembler_job_t *job = driver->job;
        while(job != NULL)
        {
            if(job->type == kAssemblerJobTypeDriver && driver->in_process)
            {
                if(driver->verbose)
                {
                    printf("\n");
                }

                assembler_driver_t *subdriver = assembler_driver_alloc((const char**)job->argv, job->argc);
                if(subdriver == NULL)
                {
                    return false;
                }

                bool success = assembler_driver_drive_the_fucking_car(subdriver);
                assembler_driver_dealloc(subdriver);
                if(!success)
                {
                    return false;
                }
            }
            else if(job->type == kAssemblerJobTypeLinker && driver->in_process)
            {
                if(driver->verbose)
                {
                    printf("\n");
                }

                linker_driver_t *subdriver = linker_driver_alloc((const char**)job->argv, job->argc);
                if(subdriver == NULL)
                {
                    return false;
                }

                bool success = linker_driver_drive_the_fucking_car(subdriver);
                linker_driver_dealloc(subdriver);
                if(!success)
                {
                    return false;
                }
            }
            else
            {
                pid_t pid = 0;
                if(posix_spawnp(&pid, job->command, NULL, NULL, job->argv, environ) != 0)
                {
                    diag_error(NULL, "failed to spawn it!\n");
                    return false;
                }

                if(driver->verbose)
                {
                    printf("\nspawned job (command='%s' | pid=%d)\n", job->command, pid);
                }

                int rstatus = 0;
                if(waitpid(pid, &rstatus, 0) != pid)
                {
                    return false;
                }

                if(WIFEXITED(rstatus))
                {
                    if(WEXITSTATUS(rstatus) != 0)
                    {
                        return false;
                    }
                }
                else if(WIFSIGNALED(rstatus))
                {
                    diag_error(NULL, "job (command='%s' | pid=%d) terminated by signal %d\n", job->command, pid, WTERMSIG(rstatus));
                    return false;
                }
            }

            job = job->next;
        }

        return true;
    }

    return false;
}
