/*
 * compat.h
 *
 *  Created on: 15 de set de 2016
 *      Author: carlosfelipe.faruolo
 */

/// This header provides alternative implementations of POSIX-defined functions, for compatibilty purposes

#if !defined(_WIN32) && defined(__STRICT_ANSI__)
	#define off64_t _off64_t
#endif

#ifdef __cplusplus
	#include <cstdlib>
	#include <cstring>
	#include <cctype>
#else
	#include <stdlib.h>
	#include <string.h>
	#include <ctype.h>
#endif

// Dealing with specific compilers
#ifdef _MSC_VER
	#if _MSC_VER < 1900
		#define FORCE_CUSTOM_LRINT_ENABLED
		#define CUSTOM_STRCASECMP_DISABLED
		#define CUSTOM_STRDUP_DISABLED
		#define CUSTOM_SNPRINTF_ENABLED

		#define strcasecmp _stricmp
		#undef min
		#undef max
	#endif
#elif (defined(__GNUG__) || defined(__GNUC__)) && !defined(__STRICT_ANSI__)
	#define CUSTOM_LRINT_DISABLED
	#define CUSTOM_STRCASECMP_DISABLED
	#define CUSTOM_STRDUP_DISABLED
#endif

// If C++11 or C99, these POSIX functions are available
#if (defined(__cplusplus) && __cplusplus >= 201103L) || __STDC_VERSION__ >= 199901L
	#ifndef CUSTOM_LRINT_DISABLED
		#define CUSTOM_LRINT_DISABLED
	#endif
#endif

#if !defined(CUSTOM_LRINT_DISABLED) || defined(FORCE_CUSTOM_LRINT_ENABLED)
	#ifdef __cplusplus
		#include <cmath>
	#else
		#include <math.h>
	#endif

	long int lrint(double x)
	{
		//middle value point test
		if(ceil(x+0.5) == floor(x+0.5))
		{
			int a = (int) ceil(x+0.5);
			if(a%2 == 0)
				return ceil(x);
			else
				return floor(x);
		}
		else return floor(x+0.5);
	}
#endif

#ifndef CUSTOM_STRCASECMP_DISABLED
	#ifdef __cplusplus
		#include <cstring>
	#else
		#include <string.h>
	#endif

	int strcasecmp( const char * str1, const char * str2)
	{
		unsigned int str1len = strlen(str1);
		unsigned int str2len = strlen(str2);
		if(str1len < str2len) return -str2[str1len];
		if(str1len > str2len) return  str1[str2len];
		unsigned int i;
		for(i = 0; i < str1len; i++)
			if (tolower(str1[i]) != tolower(str2[i]))
				return tolower(str1[i]) - tolower(str2[i]);
		return 0;
	}
#endif

#ifndef CUSTOM_STRDUP_DISABLED
	char* strdup (const char *s)
	{
		char *d = (char*) malloc (strlen (s) + 1);  // Allocate memory (Space for length plus nul)
		if (d != NULL) strcpy (d,s);        		// Copy string if okay
		return d;                            		// Return the new string
	}
#endif

#ifdef FORCE_CUSTOM_SNPRINTF_DISABLED
	#undef CUSTOM_SNPRINTF_ENABLED
#endif

#ifdef CUSTOM_SNPRINTF_ENABLED
	#ifndef __cplusplus
		#include <stdarg.h>
		#include <stdio.h>
	#else
		#include <cstdarg>
		#include <cstdio>
	#endif

	#define vsnprintf c99_vsnprintf
	#define snprintf c99_snprintf

	int c99_vsnprintf(char *outBuf, size_t size, const char *format, va_list ap)
	{
		int count = -1;

		if (size != 0)
			count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
		if (count == -1)
			count = _vscprintf(format, ap);

		return count;
	}

	int c99_snprintf(char *outBuf, size_t size, const char *format, ...)
	{
		int count;
		va_list ap;

		va_start(ap, format);
		count = c99_vsnprintf(outBuf, size, format, ap);
		va_end(ap);

		return count;
	}
#endif

// Include directly because old SDL versions don't.
#include <SDL/SDL_thread.h>
