#ifndef _CTYPE_H
#define _CTYPE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

static inline int isdigit(int c)
{
    return c >= '0' && c <= '9';
}

static inline int isalpha(int c)
{
    return (c | 32) >= 'a' && (c | 32) <= 'z';
}

static inline int isalnum(int c)
{
    return isdigit(c) || isalpha(c);
}

static inline int isspace(int c)
{
    return c == ' ' || (c >= '\t' && c <= '\r');
}

static inline int isupper(int c)
{
    return c >= 'A' && c <= 'Z';
}

static inline int islower(int c)
{
    return c >= 'a' && c <= 'z';
}

static inline int isxdigit(int c)
{
    return isdigit(c) || ((c | 32) >= 'a' && (c | 32) <= 'f');
}

static inline int isprint(int c)
{
    return c >= 0x20 && c < 0x7f;
}

static inline int toupper(int c)
{
    return islower(c) ? c - 32 : c;
}

static inline int tolower(int c)
{
    return isupper(c) ? c + 32 : c;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _CTYPE_H */
