#ifndef PATH13_H
#define PATH13_H

#include <dirent.h>
#include <sys/stat.h>

#ifndef __P
#define __P(args) ()
#endif

#define UNIX_HOME_DIR   "~"
#define WIN_HOME_VAR    "HOME"
#define WIN_HOME_VAR2   "HOMEPATH"
#define WIN_USER_PROFILE	"USERPROFILE"
#define PREV_DIR    ".."
#define THIS_DIR    "."

#ifdef __WIN32
#define DIR_SEP "\\"
#define DRIVE_SEP  ":"
#else
#define DIR_SEP "/"
#endif

#define EXT_MARK    '.'

#define P13_TYPE_STR_DIR "dir"
#define P13_TYPE_STR_FILE "file"
#define P13_TYPE_STR_UNK "unknown"

#define P13_TYPE_ID_ERR		-1
#define P13_TYPE_ID_UNK		0
#define P13_TYPE_ID_DIR		1
#define P13_TYPE_ID_FILE	2

#ifndef MAXPATHNAME
#define MAXPATHNAME 1024
#endif

struct path13_entry {

    char* name;
    char* abs_path;

    struct stat st;

    struct path13_entry* next;
};

#ifdef __cplusplus
    extern "C" {
#endif

#define p13_type_str(st) ((((st)->st_mode & S_IFMT)==S_IFDIR)?P13_TYPE_STR_DIR:(((st)->st_mode & S_IFMT)==S_IFREG)?P13_TYPE_STR_FILE:P13_TYPE_STR_UNK)
#define p13_type_id(st) ((((st)->st_mode & S_IFMT)==S_IFDIR)?P13_TYPE_ID_DIR:(((st)->st_mode & S_IFMT)==S_IFREG)?P13_TYPE_ID_FILE:P13_TYPE_ID_UNK)

int p13_create_dir_struct(char* abs_path);
char* p13_get_unix_path(char* abs_path);
char* p13_get_abs_path(char* root, char* abs_path);
char* p13_join_path(char* path1, char* path2, char* buf);
char* p13_merge_path(char* path1, char* path2, char* buf);
int p13_get_path_depth(char* path);
int p13_is_abs_path(char* path);
char* p13_get_filename(char* path);
char* p13_cut_from_start(char* path, char* tocut);
char* p13_get_ext(char* path);

char* p13_get_home();

//returns path type, dir, file, etc...
int p13_get_type_id(char* path);

#define p13_merge_path2(source_base, source, target_base, buf) p13_merge_path(target_base, p13_cut_from_start(source, source_base), buf);

#ifdef WIN32
int alphasort(const void* d1, const void* d2);
int scandir(const char* dirname, struct dirent ***namelist, int (*select)__P((struct dirent *)), int (*dcomp)__P((const void *, const void *)));
#endif

#ifdef __cplusplus
    }
#endif

#endif // PATH13_H
