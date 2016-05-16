#ifndef TOOLS_H
#define TOOLS_H

#include <string>
#include <cstdarg>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <bfd.h>
#include <dis-asm.h>

/* Pseudo FILE object for strings.  */
typedef struct
{
  char *buffer;
  size_t pos;
  size_t alloc;
} SFILE;

struct print_file_list
{
  struct print_file_list *next;
  const char *filename;
  const char *modname;
  const char *map;
  size_t mapsize;
  const char **linemap;
  unsigned maxline;
  unsigned last_line;
  int first;
};

/* Return the filename in a static buffer.  */
const char *bfd_get_archive_filename(const bfd *);

void bfd_nonfatal(const char *);

void bfd_nonfatal_message(const char *, const bfd *, const asection *,
                          const char *, ...);

void bfd_fatal(const char *) ATTRIBUTE_NORETURN;

void report(const char *, va_list) ATTRIBUTE_PRINTF(1,0);

void fatal(const char *, ...) ATTRIBUTE_PRINTF_1 ATTRIBUTE_NORETURN;

void non_fatal(const char *, ...) ATTRIBUTE_PRINTF_1;

void list_matching_formats(char **);

off_t get_file_size(const char *);

extern void *bfd_malloc(bfd_size_type);

int compare_symbols(const void *, const void *);

#endif // TOOLS_H
