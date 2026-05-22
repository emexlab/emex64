/*
 * MIT License
 *
 * Copyright (c) 2024 cr4zyengineer
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

#include <emex64asm/code.h>
#include <emex64asm/cmptok.h>
#include <emex64asm/diag.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <limits.h>

void code_tokengen(compiler_invocation_t *ci,
                   const char **filev,
                   int filec)
{
    /* setting file count for tokenizer */
    ci->file_cnt = filec;

    /* allocating code array */
    ci->file = calloc(filec, sizeof(compiler_file_t));

    /* calculating the total buffer size needed to store the code into */
    for(int i = 0; i < filec; i++)
    {
        /* opening file */
        int fd = open(filev[i], O_RDONLY);

        /* resolve real path */
        ci->file[i].path = malloc(PATH_MAX);
        if(realpath(filev[i], ci->file[i].path) == NULL)
        {
            diag_error(NULL, "couldnt get realpath of assembly file located at %s\n", filev[i]);
        }

        /* checking for succession */
        if(fd < 0)
        {
            diag_error(NULL, "couldnt open assembly file located at %s\n", ci->file[i].path);
        }

        /* getting stat */
        struct stat fdstat;
        if(fstat(fd, &fdstat) < 0)
        {
            diag_error(NULL, "couldnt get size of assembly file located at %s\n", ci->file[i].path);
        }

        /* map assembly file */
        ci->file[i].code = mmap(NULL, fdstat.st_size, PROT_READ, MAP_SHARED, fd, 0);

        if(ci->file[i].code == MAP_FAILED)
        {
            diag_error(NULL, "couldnt map assembly file located at %s\n", ci->file[i].path);
        }

        /* setting lenght of file */
        ci->file[i].len = fdstat.st_size;
    }

    /* iterating through code and look for newline characters as indicator for a newline, all this to know how many lines exist here */
    for(size_t a = 0; a < ci->file_cnt; a++)
    {
        for(size_t i = 0; i < ci->file[a].len; i++)
        {
            if(ci->file[a].code[i] == '\n')
            {
                (ci->line_cnt)++;
            }
        }
        (ci->line_cnt)++;
    }

    /* allocate array of lines */
    ci->line = calloc(ci->line_cnt, sizeof(compiler_line_t));
    ci->line_cnt = 0;

    /* reset line count and then begin to copy */
    for(size_t a = 0; a < ci->file_cnt; a++)
    {
        size_t file_line_cnt = 0;
        size_t start_off = 0;   /* this offset is used to determine the lenght of each line */
        for(size_t i = 0; i <= ci->file[a].len; i++)
        {
            /* checking for new line character*/
            if(ci->file[a].code[i] == '\n' || i == ci->file[a].len)
            {
                size_t end_off = i;

                /* calculating the total lenght of the string */
                size_t len = end_off - start_off;

                /* allocate and copy line string */
                ci->line[ci->line_cnt].str = malloc(len + 1);
                memcpy(ci->line[ci->line_cnt].str, &(ci->file[a].code[start_off]), len);
                ci->line[ci->line_cnt].str[len] = '\0';

                /* store diagnostic info */
                ci->line[ci->line_cnt].line_num = ++file_line_cnt;
                ci->line[ci->line_cnt].file_idx = a;
                ci->line[(ci->line_cnt)++].ci = ci;
                start_off = i + 1;
            }
        }
    }

    /* getting subtokens of each token */
    for(unsigned long i = 0; i < ci->line_cnt; i++)
    {
        /* using cmptok in first pass to get token count */
        for(cmptok_return_t token = cmptok(ci->line[i].str); token.token != NULL;)
        {
            /* until this is not null i will not move anywhere else than my safe space which is this while loop :3*/
            ci->line[i].token_cnt++;
            token = cmptok(NULL);
        }

        /* allocating memory for array of subtokens */
        ci->line[i].token = calloc(sizeof(compiler_token_t), ci->line[i].token_cnt);

        /* copy subtokens */
        ci->line[i].token_cnt = 0;

        /* again doing the same dance, over and over and over again, is this a carousell or why am I getting ill rn */
        for(cmptok_return_t token = cmptok(ci->line[i].str); token.token != NULL;)
        {
            ci->line[i].token[ci->line[i].token_cnt].str = strdup(token.token);
            ci->line[i].token[ci->line[i].token_cnt].column_num = token.pos + 1;
            ci->line[i].token[ci->line[i].token_cnt++].cl = &(ci->line[i]);
            token = cmptok(NULL);
        }
    }

    /* token type evaluation */
    unsigned char section_mode = 0b0;
    for(unsigned long i = 0; i < ci->line_cnt; i++)
    {
        /* checking if valid token in the first place */
        if(ci->line[i].token_cnt == 0)
        {
            continue;
        }

        /* lets go */
        if(ci->line[i].token_cnt < 2)
        {
            /* getting size of subtoken */
            size_t size = strlen(ci->line[i].token[0].str);

            /* anti wrap around check */
            if(size == 0)
            {
                continue;
            }

            /* checking if last character of token is a ':', because that means that its a label */
            if(ci->line[i].token[0].str[size - 1] == ':')
            {
                section_mode = 0b0;

                /* checking what type of label it is */
                if(ci->line[i].token[0].str[0] == '_')
                {
                    ci->line[i].type = COMPILER_LINE_TYPE_GLOBAL_LABEL;
                }
                else if(ci->line[i].token[0].str[0] == '.')
                {
                    ci->line[i].type = COMPILER_LINE_TYPE_LOCAL_LABEL;
                }
                else
                {
                    diag_error(&(ci->line[i].token[0]), "illegal label definition \"%s\"\n", ci->line[i].token[0].str);
                }

                continue;
            }
        }
        else if(ci->line[i].token_cnt < 3 &&
                strcmp(ci->line[i].token[0].str, "section") == 0)
        {
            section_mode = 0b1;
            ci->line[i].type = COMPILER_LINE_TYPE_SECTION;
            continue;
        }
        else if(strcmp(ci->line[i].token[0].str, "%define%") == 0)
        {
            section_mode = 0b0;
            ci->line[i].type = COMPILER_LINE_TYPE_MACRODEF;
            continue;
        }

        /* last if else dance */
        if(section_mode)
        {
            /* its part of a section definition */
            ci->line[i].type = COMPILER_LINE_TYPE_SECTION_DATA;
        }
        else
        {
            /* its probably assembly code */
            ci->line[i].type = COMPILER_LINE_TYPE_ASM;
        }
    }

    /* parsing completed and due to that we can straight up unmap the files */
    for(int i = 0; i < ci->file_cnt; i++)
    {
        munmap(ci->file[i].code, ci->file[i].len);
    }
}