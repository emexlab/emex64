/*
 * MIT License
 *
 * Copyright (c) 2024 emexlab
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
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <limits.h>

#include <emex64lib/asm/code.h>
#include <emex64lib/asm/cmptok.h>
#include <emex64lib/asm/diag.h>

void assembler_code_parse(assembler_invocation_t *inv,
                          const char **filev,
                          int filec)
{
    /*
     * preparing compiler invocation to
     * map and parse files.
     */
    inv->file_cnt = filec;
    inv->file = calloc(filec, sizeof(compiler_file_t));

    /* mapping files */
    for(int i = 0; i < filec; i++)
    {
        /* initial open */
        int fd = open(filev[i], O_RDONLY);
        if(fd < 0)
        {
            diag_error(NULL, "couldnt open assembly file located at %s\n", inv->file[i].path);
        }

        /*
         * resolving the true paths is important
         * so errors can reveal the actual file
         * locations.
         */
        inv->file[i].path = malloc(PATH_MAX);
        if(realpath(filev[i], inv->file[i].path) == NULL)
        {
            diag_error(NULL, "couldnt get realpath of assembly file located at %s\n", filev[i]);
        }

        /* initially mapping assembly file */
        struct stat fdstat;
        if(fstat(fd, &fdstat) < 0)
        {
            diag_error(NULL, "couldnt get size of assembly file located at %s\n", inv->file[i].path);
        }

        inv->file[i].len = fdstat.st_size;
        inv->file[i].code = mmap(NULL, inv->file[i].len, PROT_READ, MAP_SHARED, fd, 0);
        if(inv->file[i].code == MAP_FAILED)
        {
            diag_error(NULL, "couldnt map assembly file located at %s\n", inv->file[i].path);
        }

        close(fd);
    }

    /*
     * iterating through code and look for newline characters as
     * indicator for a newline, all this to know how many lines
     * exist here so we can allocate a list of lines.
     * 
     * note: we might switch to linked list later with those
     *       things as it will be a lot faster than wasting
     *       cpu cycles on dry passes.
     */
    size_t total_lines = 0;
    for(size_t a = 0; a < inv->file_cnt; a++)
    {
        for(size_t i = 0; i < inv->file[a].len; i++)
        {
            if(inv->file[a].code[i] == '\n')
            {
                total_lines++;
            }
        }
        total_lines++;
    }

    /* copy each line */
    inv->line = calloc(total_lines, sizeof(compiler_line_t));
    inv->line_cnt = 0;
    for(size_t a = 0; a < inv->file_cnt; a++)
    {
        size_t file_line_cnt = 0;
        size_t start_off = 0;   /* this offset is used to determine the lenght of each line */
        for(size_t i = 0; i <= inv->file[a].len; i++)
        {
            /* checking for new line character*/
            if(inv->file[a].code[i] == '\n' || i == inv->file[a].len)
            {
                size_t end_off = i;

                /* calculating the total lenght of the string */
                size_t len = end_off - start_off;

                /* allocate and copy line string */
                inv->line[inv->line_cnt].str = malloc(len + 1);
                memcpy(inv->line[inv->line_cnt].str, &(inv->file[a].code[start_off]), len);
                inv->line[inv->line_cnt].str[len] = '\0';

                /* store diagnostic info */
                inv->line[inv->line_cnt].line_num = ++file_line_cnt;
                inv->line[inv->line_cnt].file_idx = a;
                inv->line[(inv->line_cnt)++].ci = inv;
                start_off = i + 1;
            }
        }
    }

    /* getting subtokens of each token */
    for(unsigned long i = 0; i < inv->line_cnt; i++)
    {
        /* using cmptok in first pass to get token count */
        for(cmptok_return_t token = cmptok(inv->line[i].str); token.token != NULL;)
        {
            /*
             * until this is not null i will not move
             * anywhere else than my safe space which
             * is this while loop :3
             */
            inv->line[i].token_cnt++;
            token = cmptok(NULL);
        }

        /* copy subtokens */
        inv->line[i].token = calloc(sizeof(compiler_token_t), inv->line[i].token_cnt);
        inv->line[i].token_cnt = 0;

        /*
         * again doing the same dance, over and over
         * and over again, is this a carousell or
         * why am I getting ill rn.
         */
        for(cmptok_return_t token = cmptok(inv->line[i].str); token.token != NULL;)
        {
            inv->line[i].token[inv->line[i].token_cnt].str = strdup(token.token);
            inv->line[i].token[inv->line[i].token_cnt].column_num = token.pos + 1;
            inv->line[i].token[inv->line[i].token_cnt++].cl = &(inv->line[i]);
            token = cmptok(NULL);
        }
    }

    /* token type evaluation */
    bool section_mode = false;
    for(unsigned long i = 0; i < inv->line_cnt; i++)
    {
        if(inv->line[i].token_cnt == 0)
        {
            /* probably a whitespace */
            continue;
        }
        else if(inv->line[i].token_cnt < 2)
        {
            /* getting size of subtoken */
            size_t size = strlen(inv->line[i].token[0].str);
            if(size == 0)
            {
                /* invalid size */
                continue;
            }

            /*
             * checking if last character of token is a ':',
             * because that means that its a label.
             */
            if(inv->line[i].token[0].str[size - 1] == ':')
            {
                section_mode = false;

                /*
                 * checking what type of label it is
                 *
                 * note: '_example' for example would be a global
                 *       label, which means it can be called by
                 *       any symbol in the same program, while
                 *       '.example' is a local label which can only
                 *       be called within the same global label's code. 
                 */
                switch(inv->line[i].token[0].str[0])
                {
                    case '_':
                        inv->line[i].type = ASSEMBLER_LINE_TYPE_GLOBAL_LABEL;
                        break;
                    case '.':
                        inv->line[i].type = ASSEMBLER_LINE_TYPE_LOCAL_LABEL;
                        break;
                    default:
                        diag_error(&(inv->line[i].token[0]), "illegal label definition \"%s\"\n", inv->line[i].token[0].str);
                        break;
                }

                continue;
            }
        }
        else if(inv->line[i].token_cnt < 3 && strcmp(inv->line[i].token[0].str, "section") == 0)
        {
            section_mode = true;
            inv->line[i].type = ASSEMBLER_LINE_TYPE_SECTION;
            continue;
        }
        else if(strcmp(inv->line[i].token[0].str, "%define%") == 0)
        {
            section_mode = false;
            inv->line[i].type = ASSEMBLER_LINE_TYPE_MACRODEF;
            continue;
        }

        /*
         * it is either part of a section or
         * assembly, this is a very important
         * differentiation. 
         */
        inv->line[i].type = section_mode ? ASSEMBLER_LINE_TYPE_SECTION_DATA : ASSEMBLER_LINE_TYPE_ASM;
    }

    /*
     * unmapping each code file, but this
     * shall not be the case bruh, we shall
     * use linked lists so we dont have to
     * store the code into the files
     * anyways.
     */
    for(int i = 0; i < filec; i++)
    {
        munmap(inv->file[i].code, inv->file[i].len);
        inv->file[i].code = NULL;
    }
}