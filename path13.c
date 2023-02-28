/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)scandir.c	5.10 (Berkeley) 2/23/91";
#endif /* LIBC_SCCS and not lint */

/*
 * Scan the directory dirname calling select to make a list of selected
 * directory entries then sort using qsort and compare routine dcomp.
 * Returns the number of entries and a pointer to a list of pointers to
 * struct dirent (through namelist). Returns -1 if there were any errors.
 */

#include <sys/types.h>
#include <sys/stat.h>
//#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#ifdef linux
#include <unistd.h>
#else

#endif

#include "include/path13.h"
#include "include/debug13.h"

#define _deb1(f, a...)
#define _deb2(f, a...) //used to debug merging
#define _deb3   _NullMsg //debugging get_unix_path and join_path on linux

#ifdef _WIN32

#define MAXNAMLEN FILENAME_MAX

#define ARRAY_CHUNK	10

/*
 * The DIRSIZ macro gives the minimum record length which will hold
 * the directory entry.  This requires the amount of space in struct dirent
 * without the d_name field, plus enough space for the name with a terminating
 * null byte (dp->d_namlen+1), rounded up to a 4 byte boundary.
 */
#undef DIRSIZ
#define DIRSIZ(dp) \
    ((sizeof (struct dirent) - (MAXNAMLEN+1)) + (((dp)->d_namlen+1 + 3) &~ 3))

#ifndef __P
#define __P(args) ()
#endif

int
scandir(dirname, namelist, select, dcomp)
    const char *dirname;
    struct dirent ***namelist;
    int (*select) __P((struct dirent *));
    int (*dcomp) __P((const void *, const void *));
{
    register struct dirent *d, *p, **names;
    register size_t nitems;
    //struct stat stb;
    long arraysz;
    DIR *dirp;

    //char this_dir[FILENAME_MAX];

    if ((dirp = opendir(dirname)) == NULL)
        return(-1);

    //if (fstat(dirp->dd_fd, &stb) < 0)
    //	return(-1);

    //printf("stating %s ...", dirname);

    //if (stat(dirname, &stb) < 0)
    //	return(-1);

    ////printf("done, stb.st_size = %u.\n", stb.st_size);

    /*
     * estimate the array size by taking the size of the directory file
     * and dividing it by a multiple of the minimum size entry.
     */

    //arraysz = (stb.st_size / 24);

    arraysz = ARRAY_CHUNK;
    names = (struct dirent **)malloc(arraysz * sizeof(struct dirent *));
    if (names == NULL)
        return(-1);

    nitems = 0;
    while ((d = readdir(dirp)) != NULL) {

        //printf("entered while, selecting.\n");

        if (select != NULL && !(*select)(d))
            continue;	/* just selected names */
        /*
         * Make a minimum size copy of the data
         */

        p = (struct dirent *)malloc(DIRSIZ(d));

        //printf("getting p...");

        if (p == NULL)
            return(-1);

        //printf("done, d->d_name = %s\n", d->d_name);

        p->d_ino = d->d_ino;
        p->d_reclen = d->d_reclen;
        p->d_namlen = d->d_namlen;
        //bcopy(d->d_name, p->d_name, p->d_namlen + 1);
        memcpy(p->d_name, d->d_name, p->d_namlen + 1);
        /*
         * Check to make sure the array has space left and
         * realloc the maximum size.
         */
        if (++nitems >= arraysz) {
            //if (fstat(dirp->dd_fd, &stb) < 0)
            //	return(-1);	/* just might have grown */

            //if (stat(dirname, &stb) < 0)
            //	return(-1);	/* just might have grown */

            //printf("here, nitems = %i >= arraysz = %i\n", nitems, arraysz);

            //arraysz += stb.st_size / 12;
            arraysz += ARRAY_CHUNK;
            names = (struct dirent **)realloc((char *)names,
                arraysz * sizeof(struct dirent *));

            //printf("checking realloc\n");
            if (names == NULL)
                return(-1);
            //printf("OK\n");
        }
        names[nitems-1] = p;
        //printf("p->d_name = %s, names: %s\n", p->d_name, names[nitems - 1]->d_name);
    }
    closedir(dirp);
    if (nitems && dcomp != NULL)
        qsort(names, nitems, sizeof(struct dirent *), dcomp);
    *namelist = names;

    //printf("nitems = %i", nitems);

    return(nitems);
}

/*
 * Alphabetic order comparison routine for those who want it.
 */
int
alphasort(d1, d2)
    const void *d1;
    const void *d2;
{
    return(	strcmp((*(struct dirent **)d1)->d_name,
		(*(struct dirent **)d2)->d_name));
}
#endif

#ifdef __WIN32
char* p13_get_unix_path(char* abs_path){

    //assert(abs_path);

    char* pos;
    if((pos = strstr(abs_path, DRIVE_SEP))){
        return pos+strlen(DRIVE_SEP);
    }
    return abs_path;

}
#else
char* p13_get_unix_path(char* abs_path){
	return abs_path;
}
#endif

char* p13_join_path(char* path1, char* path2, char* buf){

    assert(path1);
    assert(path2);
    assert(buf);

    char* unix_path2 = p13_get_unix_path(path2);

    _deb3("path1: %s, unix_path: %s, path2: %s", path1, unix_path2, path2);

    if(
        !memcmp(path1 + strlen(path1) - strlen(DIR_SEP), DIR_SEP, strlen(DIR_SEP)) ||
        !memcmp(unix_path2, DIR_SEP, strlen(DIR_SEP))
            ){
	#ifndef linux
        snprintf(buf, MAXPATHNAME, "%s%s", path1, unix_path2);
        #else
        s13_strcpy(buf, path1, MAXPATHNAME);
        s13_strcat(buf, unix_path2, MAXPATHNAME);
	#endif
    } else {
    	#ifndef linux
        snprintf(buf, MAXPATHNAME, "%s%s%s", path1, DIR_SEP, unix_path2);
        #else
        s13_strcpy(buf, path1, MAXPATHNAME);
        s13_strcat(buf, DIR_SEP, MAXPATHNAME);
        s13_strcat(buf, unix_path2, MAXPATHNAME);
        #endif
    }

    _deb3("buf: %s", buf);

    return buf;
}

char* p13_get_abs_path(char* root, char* abs_path){

#ifdef _WIN32
    char* toor = root;
#endif

    if(!strcmp(root, THIS_DIR)){//return the cwd

        if(!getcwd(abs_path, MAXPATHNAME)){
            return NULL;
        }

    } else if(!strcmp(root, PREV_DIR)){

        if(!getcwd(abs_path, MAXPATHNAME)){
            return NULL;
        }

    } else if( !memcmp(root, DIR_SEP, strlen(DIR_SEP))
#ifdef _WIN32
               || strstr(toor, DRIVE_SEP)
#endif
              ){//this is abs path
	#ifdef _WIN32
        snprintf(abs_path, MAXPATHNAME, "%s", root);
        #else
        s13_strcpy(abs_path, root, MAXPATHNAME);
        #endif
    } else {//add abs path of cwd to the relative path
    	#ifdef _WIN32
        snprintf(abs_path, MAXPATHNAME, "%s%s%s", getcwd(NULL, 0), DIR_SEP, root);
        #else
        s13_strcpy(abs_path, getcwd(NULL, 0), MAXPATHNAME);
        s13_strcat(abs_path, DIR_SEP, MAXPATHNAME);
        s13_strcat(abs_path, root, MAXPATHNAME);
        #endif
    }

#ifdef _WIN32
    strlwr(abs_path);
    _deb1("lower: %s", abs_path);
#endif

    return abs_path;
}

char* p13_merge_path(char* path1, char* path2, char* buf){

    char* unix_path1, *unix_path2, *end;
    size_t cmp_len, l1, l2;

    _deb2("merging \'%s\' & \'%s\'", path1, path2);

    unix_path1 = p13_get_unix_path(path1);
    unix_path2 = p13_get_unix_path(path2);

    l1 = strlen(unix_path1);
    l2 = strlen(unix_path2);

    if(l1 < l2) cmp_len = l1;
    else cmp_len = l2;

#ifdef _WIN32
    strlwr(unix_path1);
    strlwr(unix_path2);
#endif

    if(!memcmp(unix_path1, unix_path2, cmp_len)){

        //found similarity, needs merging
        p13_join_path(path1, unix_path2 + cmp_len , buf);

    } else {

        //do not need merging, join it!
        p13_join_path(path1, unix_path2, buf);

    }

    //remove trailing slashes
    cmp_len = strlen(DIR_SEP);
    end = buf + strlen(buf);
    while(!memcmp(end - cmp_len, DIR_SEP, cmp_len) && strlen(buf))
        {end -= cmp_len; *end = '\0';}

    return buf;
}

int p13_get_path_depth(char* path){
    register int level = 0;
    register size_t i;
    for(i = 1; i < strlen(path)-1; i++){
        if(path[i]==DIR_SEP[0]) level++;
    }

    return level;
}

int p13_is_abs_path(char* path){

#ifdef _WIN32

    if(strstr(path, DRIVE_SEP)) return 1;

#endif

    if(!memcmp(path, DIR_SEP, strlen(DIR_SEP))) return 1;

    return 0;

}

char* p13_get_filename(char* path){
    size_t len = strlen(path);
    while(path[len] != DIR_SEP[0] && len)
        len--;

    return path + ++len;
}

char* p13_get_ext(char* path){
    size_t len = strlen(path);
    while(path[len] != EXT_MARK && len)
        len--;

    return path + ++len;
}

int p13_create_dir_struct(char* abs_path){

    char* eod = p13_get_filename(abs_path);//end-of-dir

    *eod = '\0';

    eod = abs_path;

    while( (eod = strstr(eod, DIR_SEP)) ){
        *eod = '\0';

        if(access(abs_path, X_OK)){

#ifdef _WIN32
            if(mkdir(abs_path) == -1){
#else
	    if(mkdir(abs_path, S_IRWXU|S_IRWXG|S_IRWXO) == -1){
#endif
                return -1;
            }
        }

        *eod = DIR_SEP[0];
        eod++;
    }
    return 0;
}

char* p13_cut_from_start(char* path, char* tocut){
	if(memcmp(path, tocut, strlen(tocut))) return NULL;
	return path + strlen(tocut);
}

char* p13_get_home(){
#ifndef WIN32
    return UNIX_HOME_DIR;
#else
    char* home;
    home = getenv(WIN_HOME_VAR);
    _deb3("home1 is %s", home);
    if(!home) home = getenv(WIN_HOME_VAR2);
    _deb3("home2 is %s", home);
    if(p13_is_abs_path(home)){
		home = getenv(WIN_USER_PROFILE);
		_deb3("user profile is %s", home);
    }
    return home?home:".";
#endif
}

int p13_get_type_id(char* path){

	struct stat st;
	if(stat(path, &st) < 0){
        return P13_TYPE_ID_ERR;
	}

	return p13_type_id(&st);

}


