#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
int halfsiphash(const uint8_t *in, const size_t inlen, const uint8_t *k,
                uint8_t *out, const size_t outlen);

int siphash(const uint8_t *in, const size_t inlen, const uint8_t *k,
            uint8_t *out, const size_t outlen);

void getmac(char* plaintext, int sp, char* nonce, char* key, int sk, uint8_t* mac);
#ifdef __cplusplus
}
#endif
