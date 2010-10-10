#include "utils.h"
#include <string.h>
//#include <stdlib.h>
#ifdef _WIN32
#include <libc_interface.h>
#undef vsnprintf /* conflicts with msvc definition */
#endif
#include <util.h>

static char *s2_colors[10] = {
	"00","1C","38","54","70","8C","A8","C4","E0","FF"
	 };



GString* get_md5_string(gchar* input){
	PurpleCipher *cipher;
	PurpleCipherContext *context;
	unsigned char md5Hash[16];
	GString* res;
	int i;

	/* Create the MD5 hash by using Purple MD5 algorithm */
	cipher = purple_ciphers_find_cipher("md5");
	context = purple_cipher_context_new(cipher, NULL);

	purple_cipher_context_append(context, (guchar *)input, strlen(input));
	purple_cipher_context_digest(context, sizeof(md5Hash), md5Hash, NULL);
	purple_cipher_context_destroy(context);
	res = g_string_new(NULL);

	for (i = 0 ; i < sizeof(md5Hash);i++)
		g_string_append_printf(res,"%02x",md5Hash[i]);

	return res;
}
deserialized_element* deserialize_php(const gchar** pinput,int len){
	deserialized_element *res = g_new0(deserialized_element,1),*key,*val;
	const gchar* input = *pinput;
	gchar *ckey;
	const gchar* end = input + len;
	GString* buff;
	int stringlen,i;

	if (len < 1)
		return res;

	res->type = *(input++);

	switch(res->type){
		case PHP_NULL:
			break;
		case PHP_INT:
		case PHP_BOOL:
			input++;// :
			buff = g_string_new(0);
			while(*input != ';')
				buff = g_string_append_c(buff,*input++);
			res->u.int_val = atoi(buff->str);
			g_string_free(buff,TRUE);
			break;
		case PHP_STRING:
			input++;//:
			buff = g_string_new(0);
			while(*input != ':')
				buff = g_string_append_c(buff,*input++);
			input++; // :
			stringlen = atoi(buff->str);
			g_string_free(buff,TRUE);
			input++;// "
			res->u.string = g_string_new(NULL);
			for ( i = 0 ; i < stringlen ; i++)
				res->u.string = g_string_append_c(res->u.string,*input++);
			input++;// "
			break;
		case PHP_ARRAY:
			res->u.array = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,(GDestroyNotify)destroy_php_element);
			input++;//:
			
			buff = g_string_new(0);
			while(*input != ':')
				buff = g_string_append_c(buff,*input++);
			stringlen = atoi(buff->str);
			g_string_free(buff,TRUE);

			input++ ; // :
			input++ ; // {
			while (stringlen > 0)
			{
				key = deserialize_php(&input,end-input);
				switch (key->type)
				{
				case PHP_STRING:
					ckey = key->u.string->str;
					g_string_free(key->u.string,FALSE);
					g_free(key);
					break;
				case PHP_INT:
					buff = g_string_new(NULL);
					g_string_printf(buff,"%d",key->u.int_val);
					ckey = buff->str;
					g_string_free(buff,FALSE);
					destroy_php_element(key);
					break;
				default:
					ckey = "";
					destroy_php_element(key);
				}
				val = deserialize_php(&input,end-input);
				g_hash_table_replace(res->u.array,ckey,val);
				stringlen--;
			}
			//input++; // }
			break;
		default:
			res->type = PHP_ERROR;
			break;
	}
	input++; // ; }
	*pinput = input;
	return res;
}

void destroy_php_element(deserialized_element* element){
	if (!element)
		return;
	switch (element->type){
		case PHP_ARRAY:
			g_hash_table_destroy(element->u.array);
			break;
		case PHP_STRING:
			g_string_free(element->u.string,TRUE);
			break;
		
	}
	g_free(element);
}
static guint32 hexdump_g_string_append_line(
	GString             *str,
	char                *lpfx,
	guint8             *cp,
	guint32            lineoff,
	guint32            buflen)
{
	guint32            cwr = 0, twr = 0;

	/* stubbornly refuse to print nothing */
	if (!buflen) return 0;

	/* print line header */
	g_string_append_printf(str, "%s %04x:", lpfx, lineoff);

	/* print hex characters */
	for (twr = 0; twr < 16; twr++) {
		if (buflen) {
			g_string_append_printf(str, " %02hhx", cp[twr]);
			cwr++; buflen--;
		} else {
			g_string_append(str, "   ");
		}
	}

	/* print characters */
	g_string_append_c(str, ' ');
	for (twr = 0; twr < cwr; twr++) {
		if ((cp[twr] > 32 && cp[twr] < 128) || cp[twr] == 32) {
			g_string_append_c(str, cp[twr]);
		} else {
			g_string_append_c(str, '.');
		}
	}
	g_string_append_c(str, '\n');

	return cwr;
}

void hexdump_g_string_append(
								 GString             *str,
								 char                *lpfx,
								 guint8             *buf,
								 guint32            len)
{
	guint32            cwr = 0, lineoff = 0;

	do {
		cwr = hexdump_g_string_append_line(str, lpfx, buf, lineoff, len);
		buf += cwr; len -= cwr; lineoff += cwr;
	} while (cwr == 16);
}


gchar* hon2html(const gchar* input){
	GString* decoded = g_string_new(NULL);
	gboolean opened = 0;
	gchar* buffer = purple_markup_escape_text(input, -1);
	gchar* buf_temp = buffer;
	const char* end = buffer + strlen(buffer);
	while(buffer < end){
		while((*buffer != '^') && (buffer < end))
			decoded = g_string_append_c(decoded,*buffer++);
		if (buffer < end)
		{
			opened |= 2;
			if(buffer[1] == 'w' || buffer[1] == 'W')
			{
#if 0
				if (opened == 3)decoded = g_string_append(decoded,"</FONT>");
				decoded = g_string_append(decoded,"<FONT COLOR=\"#FFFFFF\" BACK=\"black\">");
				buffer += 2;
#else
				/* let's just clear color */
				g_string_append(decoded,"</FONT>");
				buffer += 2;
				opened = 0;
#endif
			}
			else if (buffer[1] == 'r' || buffer[1] == 'R' )
			{
				if (opened == 3)decoded = g_string_append(decoded,"</FONT>");
				decoded = g_string_append(decoded,"<FONT COLOR=\"#FF4C4C\">");
				buffer += 2;
			}
			else if (buffer[1] == 'y' || buffer[1] == 'Y')
			{
				if (opened == 3)decoded = g_string_append(decoded,"</FONT>");
				decoded = g_string_append(decoded,"<FONT COLOR=\"#FFFF19\">");
				buffer += 2;
			}
			else if (buffer[1] == 'g' || buffer[1] == 'G')
			{
				if (opened == 3)decoded = g_string_append(decoded,"</FONT>");
				decoded = g_string_append(decoded,"<FONT COLOR=\"#008000\">");
				buffer += 2;
			}
			else if (buffer[1] == 'k' || buffer[1] == 'K')
			{
				if (opened == 3)decoded = g_string_append(decoded,"</FONT>");
				decoded = g_string_append(decoded,"<FONT COLOR=\"#000000\">");
				buffer += 2;
			}
			else if (buffer[1] == 'c' || buffer[1] == 'C')
			{
				if (opened == 3)decoded = g_string_append(decoded,"</FONT>");
				decoded = g_string_append(decoded,"<FONT COLOR=\"#00FFFF\">");
				buffer += 2;
			}
			else if (buffer[1] == 'b' || buffer[1] == 'B')
			{
				if (opened == 3)decoded = g_string_append(decoded,"</FONT>");
				decoded = g_string_append(decoded,"<FONT COLOR=\"#4C4CFF\">");
				buffer += 2;
			}
			else if (buffer[1] == 'm' || buffer[1] == 'M')
			{
				if (opened == 3)decoded = g_string_append(decoded,"</FONT>");
				decoded = g_string_append(decoded,"<FONT COLOR=\"#FF00FF\">");
				buffer += 2;
			}
			else if (buffer < end - 3
				&& g_ascii_isdigit(buffer[1])
				&& g_ascii_isdigit(buffer[2])
				&& g_ascii_isdigit(buffer[3])
				)
			{
				if (opened == 3)decoded = g_string_append(decoded,"</FONT>");
				g_string_append_printf(decoded,"<FONT COLOR=\"#%s%s%s\">",
					s2_colors[buffer[1] - '0'],
					s2_colors[buffer[2] - '0'],
					s2_colors[buffer[3] - '0']);
				buffer += 4;
			}
			else
			{
				decoded = g_string_append_c(decoded,*buffer++);
				opened &= !2;
			}
			
			if (opened & 2)
			{
				opened = 1;
			}
		}
	}
	if (opened){
		decoded = g_string_append(decoded,"</FONT>");
		opened = 0;
	}

	g_free(buf_temp);
	return g_string_free(decoded,FALSE);
}
gchar* hon_strip(const gchar* input,gboolean escape){
	GString* decoded = g_string_new(NULL);
	gchar* buffer;
	gchar* buf_temp;
	const char* end;
	if (escape)
	{
		buffer = purple_markup_escape_text(input, -1);
	}
	else
		buffer = g_strdup(input);
	buf_temp = buffer;
	end = buffer + strlen(buffer);
	while(buffer < end){
		while((*buffer != '^') && (buffer < end))
			decoded = g_string_append_c(decoded,*buffer++);
		if (buffer < end)
		{
			if (
			   buffer[1] == 'w' || buffer[1] == 'W' 
			|| buffer[1] == 'r' || buffer[1] == 'R'
			|| buffer[1] == 'y' || buffer[1] == 'Y'
			|| buffer[1] == 'g' || buffer[1] == 'G'
			|| buffer[1] == 'k' || buffer[1] == 'K'
			|| buffer[1] == 'c' || buffer[1] == 'C'
			|| buffer[1] == 'b' || buffer[1] == 'B'
			|| buffer[1] == 'm' || buffer[1] == 'M'
			)
			{
				buffer += 2;
			}
			else if (buffer < end - 3
				&& g_ascii_isdigit(buffer[1])
				&& g_ascii_isdigit(buffer[2])
				&& g_ascii_isdigit(buffer[3])
				)
			{
				buffer += 4;
			}
			else
			{
				decoded = g_string_append_c(decoded,*buffer++);
			}
		}
	}

	g_free(buf_temp);
	return g_string_free(decoded,FALSE);
}

