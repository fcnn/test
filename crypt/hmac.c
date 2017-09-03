
// testcase:  https://tools.ietf.org/html/rfc2202
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/engine.h>

#define HEX_DIGITS "0123456789ABCDEF"

char *hmac_md5(const void *key, int key_len, const unsigned char *data, int data_len, char *res)
{
	char md[EVP_MAX_MD_SIZE];
	int md_len = 0;
	unsigned char *result = HMAC(EVP_md5(), key, key_len, data, data_len, md, &md_len);

	if (result == NULL)
		return NULL;
	int i = 0, n = 0;
	for ( ; i < md_len; ++i) {
		unsigned d = result[i];
		res[n++] = HEX_DIGITS[d>>4];
		res[n++] = HEX_DIGITS[d&0x0F];
	}
	res[n] = 0;
	return res;
}
