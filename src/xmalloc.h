#ifndef __XMALLOC_H
#define __XMALLOC_H

#if __STDC__
# define VOID void
#else
# define VOID char
#endif
 
extern VOID *xmalloc (size_t n);
extern VOID *xcalloc (size_t n, size_t s);
extern VOID *xrealloc (VOID *p, size_t n);
extern char *xstrdup (char *str);

#define XCALLOC(type, num)                                  \
      ((type *) xcalloc ((num), sizeof(type)))
#define XMALLOC(type, num)                                  \
      ((type *) xmalloc ((num) * sizeof(type)))
#define XREALLOC(type, p, num)                              \
      ((type *) xrealloc ((p), (num) * sizeof(type)))
#define XFREE(stale)                            do {        \
      if (stale) { free (stale);  stale = 0; }            \
                                              } while (0)

#endif /* __XMALLOC_H */
