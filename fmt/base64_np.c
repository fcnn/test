// base64 - no padding
#include <stdio.h>
#include <string.h>

#define TABLE "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
int encode(unsigned char *data, int len, unsigned char *out) {
	for (int i =0; i < len; ++i) {
		int ch = data[i];
		*out++ = TABLE[ch>>2];
		ch = (ch&0x03)<<4;
		if (++i == len) {
			*out++ = TABLE[ch];
			break;
		}
		ch |= data[i]>>4;
		*out++ = TABLE[ch];
		ch = (data[i]&0x0f)<<2;
		if (++i == len) {
			*out++ = TABLE[ch];
			break;
		}
		ch |= (data[i]>>6);
		*out++ = TABLE[ch];
		*out++ = TABLE[data[i]&0x3f];
	}

	*out = 0;

	return 0;
}

#define INDEX(c) c>='A'&&c<='Z'?c-'A':(c>='a'&&c<='z'?c-'a'+26:(c>='0'&&c<='9'?c-'0'+52:(c=='+'?62:(c=='/'?63:0))))
int decode(unsigned char *data) {
	int j = 0;
	for (int i = 0; data[i] != 0; ++i) {
		int ch = data[i];
		ch = INDEX(ch);
		int b = ch<<2;
		
		if (data[++i] == 0) {
			break;
		}

		ch = data[i];
		ch = INDEX(ch);
		b |= (ch>>4);
		data[j++] = b;

		b = (ch<<4);
		if (data[++i] == 0) {
			break;
		}

		ch = data[i];
		ch = INDEX(ch);
		b |= (ch>>2);
		data[j++] = b;

		b = (ch<<6);
		if (data[++i] == 0) {
			break;
		}

		ch = data[i];
		ch = INDEX(ch);
		b |= ch;
		data[j++] = b;
	}

	return j;
}

int main(int argc, char *argv[]) {
	unsigned char msg[] = "hello world!-01";
	unsigned char buf[1024];
	encode(msg, strlen((char*)msg)+1, buf);
	printf("%s\n", buf);
	decode(buf);
	printf("[%s]\n", buf);
}
