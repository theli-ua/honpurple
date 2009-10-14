#ifndef _UTILS_H
#define _UTILS_H
#include "cipher.h"
#include <glib.h>

typedef struct {
	char type;
	union {
		guint int_val;
//		gpointer pointer;
		GString* string;
		GHashTable* array;
	};
} deserialized_element;

GString* get_md5_string(gchar* input);
deserialized_element* deserialize_php(const gchar** pinput,int len);
void destroy_php_element(deserialized_element* element);

void hexdump_g_string_append(
								 GString             *str,
								 char                *lpfx,
								 guint8             *buf,
								 guint32            len);

gchar* hon2html(const gchar* buffer);
gchar* hon_strip(const gchar* input,gboolean escape);

#define PHP_INT		'i'
#define PHP_BOOL	'b'
#define PHP_ERROR	0x00
#define PHP_NULL	'N'
#define PHP_ARRAY	'a'
#define PHP_STRING	's'


#define read_guint32(x) *((guint32*)x) ; x+=4
#define read_string(x) x ; x+=strlen(x) + 1


#endif
