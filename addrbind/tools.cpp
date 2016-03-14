#include "tools.h"

/* Error reporting.  */

char *program_name;

/* Return the filename in a static buffer.  */

const char *bfd_get_archive_filename (const bfd *abfd)
{
  static size_t curr = 0;
  static char *buf;
  size_t needed;

  assert (abfd != NULL);

  if (!abfd->my_archive)
    return bfd_get_filename (abfd);

  needed = (strlen (bfd_get_filename (abfd->my_archive))
	    + strlen (bfd_get_filename (abfd)) + 3);
  if (needed > curr)
    {
      if (curr)
	free (buf);
      curr = needed + (needed >> 1);
      buf = (char *) bfd_malloc (curr);
      /* If we can't malloc, fail safe by returning just the file name.
	 This function is only used when building error messages.  */
      if (!buf)
	{
	  curr = 0;
	  return bfd_get_filename (abfd);
	}
    }
  sprintf (buf, "%s(%s)", bfd_get_filename (abfd->my_archive),
	   bfd_get_filename (abfd));
  return buf;
}

void bfd_nonfatal (const char *string)
{
    const char *errmsg;

    errmsg = bfd_errmsg (bfd_get_error ());
    fflush (stdout);
    if (string)
        fprintf (stderr, "%s: %s: %s\n", program_name, string, errmsg);
    else
        fprintf (stderr, "%s: %s\n", program_name, errmsg);
}

/* Issue a non fatal error message.  FILENAME, or if NULL then BFD,
   are used to indicate the problematic file.  SECTION, if non NULL,
   is used to provide a section name.  If FORMAT is non-null, then it
   is used to print additional information via vfprintf.  Finally the
   bfd error message is printed.  In summary, error messages are of
   one of the following forms:

   PROGRAM:file: bfd-error-message
   PROGRAM:file[section]: bfd-error-message
   PROGRAM:file: printf-message: bfd-error-message
   PROGRAM:file[section]: printf-message: bfd-error-message.  */

void bfd_nonfatal_message (const char *filename,
                           const bfd *abfd,
                           const asection *section,
                           const char *format, ...)
{
    const char *errmsg;
    const char *section_name;
    va_list args;

    errmsg = bfd_errmsg (bfd_get_error ());
    fflush (stdout);
    section_name = NULL;
    va_start (args, format);
    fprintf (stderr, "%s", program_name);

    if (abfd)
    {
        if (!filename)
            filename = bfd_get_archive_filename (abfd);
        if (section)
            section_name = bfd_get_section_name (abfd, section);
    }
    if (section_name)
        fprintf (stderr, ":%s[%s]", filename, section_name);
    else
        fprintf (stderr, ":%s", filename);

    if (format)
    {
        fprintf (stderr, ": ");
        vfprintf (stderr, format, args);
    }
    fprintf (stderr, ": %s\n", errmsg);
    va_end (args);
}

void bfd_fatal (const char *string)
{
    bfd_nonfatal (string);
    exit (1);
}

void report (const char * format, va_list args)
{
    fflush (stdout);
    fprintf (stderr, "%s: ", program_name);
    vfprintf (stderr, format, args);
    putc ('\n', stderr);
}

void fatal (const char *format, ...)
{
    va_list args;

    va_start (args, format);

    report (format, args);
    va_end (args);
    exit (1);
}

void non_fatal (const char *format, ...)
{
    va_list args;

    va_start (args, format);

    report (format, args);
    va_end (args);
}

/* Returns the size of the named file.  If the file does not
   exist, or if it is not a real file, then a suitable non-fatal
   error message is printed and (off_t) -1 is returned.  */

off_t get_file_size (const char * file_name)
{
    struct stat statbuf;

    if (stat (file_name, &statbuf) < 0)
    {
        if (errno == ENOENT)
            non_fatal ("'%s': No such file", file_name);
        else
            non_fatal ("Warning: could not locate '%s'.  reason: %s",
                       file_name, strerror (errno));
    }
    else if (! S_ISREG (statbuf.st_mode))
        non_fatal ("Warning: '%s' is not an ordinary file", file_name);
    else if (statbuf.st_size < 0)
        non_fatal ("Warning: '%s' has negative size, probably it is too large",
                   file_name);
    else
        return statbuf.st_size;

    return (off_t) -1;
}

/* After a FALSE return from bfd_check_format_matches with
   bfd_get_error () == bfd_error_file_ambiguously_recognized, print
   the possible matching targets.  */

void list_matching_formats (char **p)
{
    fflush (stdout);
    fprintf (stderr, "%s: Matching formats:", program_name);
    while (*p)
        fprintf (stderr, " %s", *p++);
    fputc ('\n', stderr);
}

/* Allocate memory using malloc.  */

void *bfd_malloc (bfd_size_type size)
{
  void *ptr;
  size_t sz = (size_t) size;

  if (size != sz
      /* This is to pacify memory checkers like valgrind.  */
      || ((signed long) sz) < 0)
    {
      bfd_set_error (bfd_error_no_memory);
      return NULL;
    }

  ptr = malloc (sz);
  if (ptr == NULL && sz != 0)
    bfd_set_error (bfd_error_no_memory);

  return ptr;
}
