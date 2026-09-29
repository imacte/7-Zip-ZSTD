/* Regression for stored-block decoding with MSVC 2026 /O1 (notably ARM64). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lz5frame_static.h"

static int decode(const unsigned char *frame, size_t frameSize,
    const unsigned char *input, size_t size, size_t inputChunk,
    size_t outputChunk, int corrupt)
{
    LZ5F_decompressionContext_t ctx;
    unsigned char *output = (unsigned char *)calloc(size + 1, 1);
    size_t read = 0, written = 0, result;
    int failed;
    if (!output) exit(2);
    result = LZ5F_createDecompressionContext(&ctx, LZ5F_VERSION);
    if (LZ5F_isError(result)) exit(2);
    do {
        size_t consumed = frameSize - read;
        size_t produced = size + 1 - written;
        if (consumed > inputChunk) consumed = inputChunk;
        if (produced > outputChunk) produced = outputChunk;
        result = LZ5F_decompress(ctx, output + written, &produced,
            frame + read, &consumed, NULL);
        read += consumed;
        written += produced;
        if (!consumed && !produced) break;
    } while (result != 0 && !LZ5F_isError(result));
    if (corrupt)
        failed = result != (size_t)-LZ5F_ERROR_contentChecksum_invalid;
    else
        failed = result != 0 || read != frameSize || written != size || memcmp(input, output, size) != 0;
    if (failed)
        printf("::error::LZ5 size=%u inputChunk=%u outputChunk=%u corrupt=%d: %s, read=%u written=%u\n",
            (unsigned)size, (unsigned)inputChunk, (unsigned)outputChunk, corrupt,
            LZ5F_getErrorName(result), (unsigned)read, (unsigned)written);
    LZ5F_freeDecompressionContext(ctx);
    free(output);
    return failed;
}

static int check(const unsigned char *input, size_t size, int level, int checksum)
{
    LZ5F_preferences_t prefs = {0};
    unsigned char *frame;
    size_t capacity, compressed;
    int failures = 0;
    prefs.compressionLevel = level;
    prefs.frameInfo.contentSize = size;
    prefs.frameInfo.contentChecksumFlag = checksum ? LZ5F_contentChecksumEnabled : LZ5F_noContentChecksum;
    capacity = LZ5F_compressFrameBound(size, &prefs);
    frame = (unsigned char *)malloc(capacity);
    if (!frame) exit(2);
    compressed = LZ5F_compressFrame(frame, capacity, input, size, &prefs);
    if (LZ5F_isError(compressed)) {
        printf("::error::LZ5 compress size=%u level=%d: %s\n", (unsigned)size, level, LZ5F_getErrorName(compressed));
        exit(2);
    }
    /* Whole frame, split headers/blocks/suffix, and limited output space. */
    failures += decode(frame, compressed, input, size, compressed, size + 1, 0);
    failures += decode(frame, compressed, input, size, 1, size + 1, 0);
    failures += decode(frame, compressed, input, size, compressed, 7, 0);
    if (checksum) {
        frame[compressed - 1] ^= 1;
        failures += decode(frame, compressed, input, size, compressed, size + 1, 1);
        failures += decode(frame, compressed, input, size, 1, 7, 1);
    }
    free(frame);
    return failures;
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
            /* Small inputs exercise stored blocks; the larger one compresses. */
            for (i = 0; i < 80; ++i) failures += check(data, i, level, checksum);
            failures += check(data, sizeof(data), level, checksum);
        }
    }
    printf("LZ5 frame tests: %d failures\n", failures);
    return failures ? 1 : 0;
}
