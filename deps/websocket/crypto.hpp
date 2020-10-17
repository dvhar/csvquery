#ifndef SIMPLE_WEB_CRYPTO_HPP
#define SIMPLE_WEB_CRYPTO_HPP

#include <cmath>
#include <iomanip>
#include <istream>
#include <sstream>
#include <string>
#include <vector>

#include "../crypto/base64.h"
#include "../crypto/sha1.h"

namespace SimpleWeb {

  class Crypto {
    const static std::size_t buffer_size = 131072;

  public:
    class Base64 {
    public:
      /// Returns Base64 encoded string from input string.
      static std::string encode(const std::string &input) noexcept {
		int newsize = encsize(input.size());
		auto btemp = (BYTE*) alloca(encsize(newsize+1));
		base64_encode((BYTE*)input.data(), btemp, input.size(), 0);
		btemp[newsize] = 0;
        return std::string((char*)btemp);
      }

      /// Returns Base64 decoded string from base64 input.
      static std::string decode(const std::string &base64) noexcept {
		auto btemp = (BYTE*) alloca(base64.size());
		auto sz = base64_decode((BYTE*)base64.data(), btemp, base64.size());
		btemp[sz] = 0;
        return std::string((char*)btemp);;
      }
    };

    /// Returns hex string from bytes in input string.
    static std::string to_hex_string(const std::string &input) noexcept {
      std::stringstream hex_stream;
      hex_stream << std::hex << std::internal << std::setfill('0');
      for(auto &byte : input)
        hex_stream << std::setw(2) << static_cast<int>(static_cast<unsigned char>(byte));
      return hex_stream.str();
    }

    /// Returns sha1 hash value from input string.
    static std::string sha1(const std::string &input, std::size_t iterations = 1) noexcept {
		std::string hash;
		hash.resize(20);
		SHA1_CTX ctx;
		sha1_init(&ctx);
		sha1_update(&ctx, (BYTE*)input.data(), input.size());
		sha1_final(&ctx, (BYTE*)hash.data());
		return hash;
    }


    /// Returns PBKDF2 hash value from the given password
    /// Input parameter key_size  number of bytes of the returned key.

    /**
     * Returns PBKDF2 derived key from the given password.
     *
     * @param password   The password to derive key from.
     * @param salt       The salt to be used in the algorithm.
     * @param iterations Number of iterations to be used in the algorithm.
     * @param key_size   Number of bytes of the returned key.
     *
     * @return The PBKDF2 derived key.
     */
  };
} // namespace SimpleWeb
#endif /* SIMPLE_WEB_CRYPTO_HPP */
