
/**
 * `b64.h' - b64
 *
 * copyright (c) 2014 joseph werle
 */

#ifndef B64_H
#define B64_H 1
#ifdef __cplusplus
extern "C" {
#endif

#define encsize(X) (((4 * X / 3) + 3) & ~3)

/**
 * Base64 index table.
 */

static const char b64_table[] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', '0', '1', '2', '3',
  '4', '5', '6', '7', '8', '9', '+', '/'
};

/**
 * Encode `unsigned char *' source with `size_t' size.
 * Returns a `char *' base64 encoded string.
 */

int b64_encode (const unsigned char *src, unsigned char *dest, size_t inlen, size_t outlen);

/**
 * Dencode `char *' source with `size_t' size.
 * Returns a `unsigned char *' base64 decoded string.
 */
unsigned char *
b64_decode (const char *, size_t);
#ifdef __cplusplus
}
#endif

#endif
