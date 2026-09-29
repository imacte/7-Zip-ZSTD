/* Exercise stored and compressed LZ5 frames, including the ARM64 CI payload. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lz5frame.h"
#include "xxhash.h"

static int check(const unsigned char *input, size_t size, int level, int checksum)
{
    LZ5F_preferences_t prefs = {0};
    LZ5F_decompressionContext_t ctx;
    unsigned char *frame, *output;
    size_t capacity, compressed, consumed, produced, result;
    int failed;
    prefs.compressionLevel = level;
    prefs.frameInfo.contentSize = size;
    prefs.frameInfo.contentChecksumFlag = checksum ? LZ5F_contentChecksumEnabled : LZ5F_noContentChecksum;
    capacity = LZ5F_compressFrameBound(size, &prefs);
    frame = (unsigned char *)malloc(capacity);
    output = (unsigned char *)malloc(size + 1);
    if (!frame || !output) exit(2);
    compressed = LZ5F_compressFrame(frame, capacity, input, size, &prefs);
    if (LZ5F_isError(compressed)) {
        printf("::error::compress size=%u level=%d: %s\n", (unsigned)size, level, LZ5F_getErrorName(compressed));
        exit(2);
    }
    result = LZ5F_createDecompressionContext(&ctx, LZ5F_VERSION);
    if (LZ5F_isError(result)) exit(2);
    consumed = compressed;
    produced = size + 1;
    result = LZ5F_decompress(ctx, output, &produced, frame, &consumed, NULL);
    failed = result != 0 || consumed != compressed || produced != size || memcmp(input, output, size) != 0;
    if (failed) {
        size_t i;
        printf("::error::size=%u level=%d checksum=%d frame=%u consumed=%u produced=%u result=%s (%u) inputXXH=%08x\n",
            (unsigned)size, level, checksum, (unsigned)compressed, (unsigned)consumed,
            (unsigned)produced, LZ5F_getErrorName(result), (unsigned)result, XXH32(input, size, 0));
        if (size < 128) {
            printf("::notice::frame=");
            for (i = 0; i < compressed; ++i) printf("%02x", frame[i]);
            printf("\n::notice::output=");
            for (i = 0; i < size; ++i) printf("%02x", output[i]);
            printf("\n");
        }
    }
    LZ5F_freeDecompressionContext(ctx);
    free(output);
    free(frame);
    return failed;
}

int main(void)
{
    const unsigned char small[] = "lz5: This is a test string passed to stdin\r\n";
    unsigned char data[4096];
    size_t i;
    int level, checksum, failures = 0;
    for (i = 0; i < sizeof(data); ++i) data[i] = (unsigned char)(i * 7 + (i >> 5));
    for (level = 1; level <= 3; level += 2) {
        for (checksum = 0; checksum <= 1; ++checksum) {
            failures += check(small, sizeof(small) - 1, level, checksum);
            for (i = 0; i < 80; ++i) failures += check(data, i, level, checksum);
            failures += check(data, sizeof(data), level, checksum);
        }
    }
    printf("LZ5 frame tests: %d failures\n", failures);
    return failures ? 1 : 0;
}
