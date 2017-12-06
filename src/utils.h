#ifndef __FPRINTD_UTILS_H__
#define __FPRINTD_UTILS_H__

int mkdir_recursion(const char *dir);
char **read_dir_files(const char *dir);
char *read_file_contents(const char *file, long *length);

void free_strv(char **strv);

#endif
