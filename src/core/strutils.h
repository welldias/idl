#ifndef IDL_STRUTILS_H
#define IDL_STRUTILS_H

#include "commons.h"
#include "list.h"

char *strutils_strndup(const char *str, int size);
/* Como sprintf, mas aloca o resultado (liberar com free). */
char *strutils_format(const char *fmt, ...);
bool strutils_lshift(char *dst, const char *src, int size);
int strutils_cmp(const char *a, word size_a, const char *b, word size_b);

/**
 * \brief	Removes any space from beginning and end of string
 * \param	str 	The string that will be trimmed
 * \return	true if OK or false if param is null.
 */
bool strutils_trim(char *str);

/**
 * \brief	Replace multiple spaces with a single space of string
 * \param	str 	The string that will be replaced
 * \return	true if OK or false if param is null.
 */
bool strutils_one_space(char *str);

/**
 * \brief	Remove all space of string
 * \param	str 	The string that will be removed
 * \return	true if OK or false if param is null.
 */
bool strutils_no_space(char *str);

/**
 * \brief 	Convert string to string.
 * \param	str		String that will be converted
 * \param	value	Value converted
 * \return	true if Ok or false if param is null.
 */
bool strutils_str_tostr(const char *str, char **value);
/**
 * \brief	Convert string to int
 * \param	str		String that will be converted
 * \param	value	Value converted
 * \return	true if Ok or false if param is null.
 */
bool strutils_str_toint(const char *str, int *value);
/**
 * \brief	Convert string to float
 * \param	str		String that will be converted
 * \param	value	Value converted
 * \return	true if Ok or false if param is null.
 */
bool strutils_str_tofloat(const char *str, float *value);
/**
 * \brief	Convert string to bool
 * \param	str		String that will be converted
 * \param	value	Value converted
 * \return	true if Ok or false if param is null.
 */
bool strutils_str_tobool(const char *str, bool *value);

/*
 * \brief	Count the occurrences of a char in a string
 * \param	s		Pointer to char, the string
 * \param	c		char with will be sought
 * \return	0 if OK or -1 if param is null.
 */
int strutils_count_matches(char *s, char c);

/*
 * \brief It splits "string" into pieces at "token", and returns the number of pieces in "count", and the pieces themselves in "array".
 * \return	true if OK or false on ERROR
 */
bool strutils_split(const char *string, char token, char ***array, word *count);

bool strutils_str_to_list(const char *buffer, word size, char separator, list_t *list);

#endif /* IDL_STRUTILS_H */
