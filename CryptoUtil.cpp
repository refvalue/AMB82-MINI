#include "CryptoUtil.hpp"

#include <Base64.h>
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>

namespace CryptoUtil {
    namespace {
        String hexEncode(const uint8_t* buffer, size_t size) {
            static constexpr char hexTable[] = "0123456789ABCDEF";
            String result;

            for (size_t i = 0; i < size; i++) {
                result += hexTable[buffer[i] >> 8];
                result += hexTable[buffer[i] & 0xF];
            }

            return result;
        }

        String base64Encode(const uint8_t* buffer, size_t size) {
            static constexpr size_t blockSize = 128;
            const auto blocks                 = size / blockSize;
            const auto remainingSize          = size % blockSize;
            char intermediate[base64_enc_len(blockSize) + 1]{};

            auto ptr = buffer;
            String result;

            auto encodeBlock = [&](size_t size) mutable {
                const auto actualSize =
                    base64_encode(intermediate, reinterpret_cast<char*>(const_cast<uint8_t*>(ptr)), size);

                intermediate[actualSize] = '\0';
                result += intermediate;
            };

            for (auto ptrEnd = buffer + blocks * blockSize; ptr < ptrEnd; ptr += blockSize) {
                encodeBlock(blockSize);
            }

            if (remainingSize != 0) {
                encodeBlock(remainingSize);
            }

            return result;
        }
    } // namespace

    String sha1(const uint8_t* buffer, size_t size, bool base64) {
        mbedtls_sha1_context context;
        uint8_t hash[20]{};

        mbedtls_sha1_init(&context);
        mbedtls_sha1_starts_ret(&context);
        mbedtls_sha1_update_ret(&context, buffer, size);
        mbedtls_sha1_finish_ret(&context, hash);

        if (base64) {
            return base64Encode(hash, sizeof(hash));
        } else {
            return hexEncode(hash, sizeof(hash));
        }
    }

    String sha1(const String& str, bool base64) {
        return sha1(reinterpret_cast<const uint8_t*>(str.c_str()), str.length(), base64);
    }

    String sha256(const uint8_t* buffer, size_t size, bool base64) {
        mbedtls_sha256_context context;
        uint8_t hash[32]{};

        mbedtls_sha256_init(&context);
        mbedtls_sha256_starts_ret(&context, 0);
        mbedtls_sha256_update_ret(&context, buffer, size);
        mbedtls_sha256_finish_ret(&context, hash);

        if (base64) {
            return base64Encode(hash, sizeof(hash));
        } else {
            return hexEncode(hash, sizeof(hash));
        }
    }

    String sha256(const String& str, bool base64) {
        return sha256(reinterpret_cast<const uint8_t*>(str.c_str()), str.length(), base64);
    }
} // namespace CryptoUtil
