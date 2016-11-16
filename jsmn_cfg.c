#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "jsmn.h"

// 见配置文件适配成json格式，
//   1。将"="号转成":", 
//   2. 在行末加上","
//   3. 将整篇用{}括起来
//   4. 转义符“\”的处理
static int readall(char *js, int jslen)
{
	char *buf = (char *)malloc(jslen);
	int data_len = fread(buf, 1, jslen, stdin);
	int len = 0;
	int i = 0;
	for ( ; i < data_len && (buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\r' || buf[i] == '\n'); )
		++i;
	if (i < data_len && buf[i] != '{')
		js[len++] = '{';

	int new_key = 1;
	for ( ; i < data_len; ++i) {
		if (buf[i] == '=' && new_key) {
			js[len++] = ':';
			new_key = 0;
		} else if (buf[i] == ':') {
			js[len++] = '\\';
			js[len++] = ':';
		} else if (buf[i] == ',' || buf[i] == '\r' || buf[i] == '\n') {
			i++;
			for ( ; i < data_len && (buf[i] == '\r' || buf[i] == '\n' || buf[i] == ' ' || buf[i] == '\t'); )
				++i;
			if (i < data_len) {
				js[len++] = ',';
				new_key = 1;
				--i;
			}
		}
		else if (buf[i] == '"') {
			js[len++] = '"';
			++i;
			for ( ; i < data_len; ++i) {
				js[len++] = buf[i];
				if (buf[i] == '"' && buf[i-1] != '\\')
					break;
			}
		} else if (buf[i] == '\\') {
			if (i + 1 < data_len) {
				++i;
				if (buf[i] != '=')
					js[len++] = '\\';
				js[len++] = buf[i];
			} else {
				js[len++] = '\\';
			}
		}
		else {
			js[len++] = buf[i];
		}
		
	}
	free(buf);
	i = len - 1;
	for ( ; i >= 0 && (js[i] == '\r' || js[i] == '\n' || js[i] == ' ' || js[i] == '\t'); )
		--i;
	if (i >= 0 && js[i] != '}')
		js[++i] = '}';
	len = i + 1;
	js[len] = 0;
	printf("%s\n", js);
	return len;
}

/*
 * An example of reading JSON from stdin and printing its content to stdout.
 * The output looks like YAML, but I'm not sure if it's really compatible.
 */

static int dump(const char *js, jsmntok_t *t, size_t count, int indent) {
	int i, j, k;
	if (count == 0) {
		return 0;
	}
	if (t->type == JSMN_PRIMITIVE) {
		printf("%.*s", t->end - t->start, js+t->start);
		return 1;
	} else if (t->type == JSMN_STRING) {
		printf("'%.*s'", t->end - t->start, js+t->start);
		return 1;
	} else if (t->type == JSMN_OBJECT) {
		printf("\n");
		j = 0;
		for (i = 0; i < t->size; i++) {
			for (k = 0; k < indent; k++) printf("  ");
			j += dump(js, t+1+j, count-j, indent+1);
			printf(": ");
			j += dump(js, t+1+j, count-j, indent+1);
			printf("\n");
		}
		return j+1;
	} else if (t->type == JSMN_ARRAY) {
		j = 0;
		printf("\n");
		for (i = 0; i < t->size; i++) {
			for (k = 0; k < indent-1; k++) printf("  ");
			printf("   - ");
			j += dump(js, t+1+j, count-j, indent+1);
			printf("\n");
		}
		return j+1;
	}
	return 0;
}

int main() {
	size_t jslen = 0;
	char *js = (char*)malloc(1<<13);

	jsmn_parser p;
	jsmntok_t *tok;
	size_t tokcount = 10;

	/* Prepare parser */
	jsmn_init(&p);

	/* Allocate some tokens as a start */
	tok = malloc(sizeof(*tok) * tokcount);
	if (tok == NULL) {
		fprintf(stderr, "malloc(): errno=%d\n", errno);
		return 3;
	}

		/* Read another chunk */
	jslen = readall(js, (1<<13)-3);
	if (jslen < 0) {
		fprintf(stderr, "fread(): %d, errno=%d\n", jslen, errno);
		return 1;
	}
	if (jslen == 0) {
		fprintf(stderr, "fread(): unexpected EOF\n");
		return 2;
	}

	if (js == NULL) {
		return 3;
	}

	int r = jsmn_parse(&p, js, jslen, tok, tokcount);
	if (r < 0) {
		return 3;
	}

	dump(js, tok, p.toknext, 0);

	return EXIT_SUCCESS;
}
