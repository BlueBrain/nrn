#include <../../nrnconf.h>
#include "nrnmpiuse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#if NRNMPI_DYNAMICLOAD /* to end of file */

#ifdef MINGW
#define RTLD_NOW 0
#define RTLD_GLOBAL 0
#define RTLD_NOLOAD 0
extern void* dlopen_noerr(const char* name, int mode);
#define dlopen dlopen_noerr
extern void* dlsym(void* handle, const char* name);
extern int dlclose(void* handle);
extern char* dlerror();
#else
#include <dlfcn.h>
#endif

#include "nrnmpi.h"

extern char* cxx_char_alloc(int);

#include "mpispike.h"
#include "nrnmpi_def_cinc" /* nrnmpi global variables */
#include "nrnmpi_dynam_cinc" /* autogenerated file */
#include "nrnmpi_dynam_wrappers.inc" /* autogenerated file */
#include "nrnmpi_dynam_stubs.c"

static void* load_mpi(const char* name, char* mes) {
	int flag = RTLD_NOW | RTLD_GLOBAL;
	void* handle = dlopen(name, flag);
	if (!handle) {
		sprintf(mes, "load_mpi: %s\n", dlerror());
	}else{
		sprintf(mes, "load_mpi: %s successful\n", name);
	}
	return handle;
}

static void* load_nrnmpi(const char* name, char* mes) {
	int i;
	int flag = RTLD_NOW | RTLD_GLOBAL;
	void* handle = dlopen(name, flag);
	if (!handle) {
		sprintf(mes, "load_nrnmpi: %s\n", dlerror());
		return 0;
	}	
	sprintf(mes, "load_nrnmpi: %s successful\n", name);
	for (i = 0; ftable[i].name; ++i) {
		void* p = dlsym(handle, ftable[i].name);
		if (!p) {
			sprintf(mes+strlen(mes), "load_nrnmpi: %s\n", dlerror());
			return 0;
		}	
		*ftable[i].ppf = p;
	}
	{
		char* (**p)(int) = (char* (**)(int))dlsym(handle, "p_cxx_char_alloc");
		if (!p) {
			sprintf(mes+strlen(mes), "load_nrnmpi: %s\n", dlerror());
			return 0;
		}
		*p = cxx_char_alloc;
	}
	return handle;
}

char* nrnmpi_load(int is_python) {
	int ismes=0;
	char* pmes;
	pmes = (char*)malloc(1024);
	pmes[0]='\0';
#if DARWIN
	sprintf(pmes, "Try loading openmpi\n");
	void* handle = load_mpi("libmpi.dylib", pmes+strlen(pmes));
	if (handle) {
		/* see man dyld */
		if (!load_nrnmpi("@loader_path/libnrnmpi.dylib", pmes+strlen(pmes))) {
			return pmes;
		}
	}else{
		ismes = 1;
sprintf(pmes+strlen(pmes), "Is openmpi installed? If not in default location, need a LD_LIBRARY_PATH.\n");
	}
#else /*not DARWIN*/
#if defined(MINGW)
	sprintf(pmes, "Try loading msmpi\n");
	void* handle = load_mpi("msmpi.dll", pmes+strlen(pmes));
	if (handle) {
		if (!load_nrnmpi("libnrnmpi.dll", pmes+strlen(pmes))){
			return pmes;
		}
	}else{
		ismes = 1;
		return pmes;
	}
#else /*not MINGW*/
	sprintf(pmes, "Try loading openmpi\n");
	void* handle = load_mpi("libmpi.so", pmes+strlen(pmes));
	if (handle) {
		if (!load_nrnmpi(NRN_LIBDIR"/libnrnmpi.so", pmes+strlen(pmes))){
			return pmes;
		}
	}else{
		sprintf(pmes+strlen(pmes), "Try loading mpich2\n");
		handle = load_mpi("libmpl.so", pmes+strlen(pmes));
		handle = load_mpi("libmpich.so", pmes+strlen(pmes));
#if 0
/* Not needed because the issue of Python launch on LINUX not resolving
   variables from already loaded shared libraries (due to loading them with
   RTLD_LOCAL) was solved at the src/nrnmpi/Makefile.am level via a change
   to libnrnmpi_la_LIBADD
*/
if (!dlopen("liboc.so", RTLD_NOW | RTLD_NOLOAD | RTLD_GLOBAL)) {
	fprintf(stderr, "Did not promote liboc.so to RTLD_GLOBAL: %s\n", dlerror());
}
if (!dlopen("libnrniv.so", RTLD_NOW | RTLD_NOLOAD | RTLD_GLOBAL)) {
	fprintf(stderr, "Did not promote libnrniv.so to RTLD_GLOBAL: %s\n", dlerror());
}
#endif
		if(!load_nrnmpi("libnrnmpi.so", pmes+strlen(pmes))){
			return pmes;
		}
	}
#endif /*not MINGW*/
#endif /* not DARWIN */
	if (!handle) {
		sprintf(pmes+strlen(pmes), "could not dynamically load libmpi.so or libmpich2.so\n");
		return pmes;
	}	
	free(pmes);
	return 0;
}
#endif
