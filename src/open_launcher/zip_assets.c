#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif
#define _XOPEN_SOURCE 700
#include "zip_assets.h"
#include "install.h"
#include <ftw.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

struct member {
    char *name;
    uint32_t offset, raw, compressed, crc;
    unsigned method;
};

struct ck2_zip {
    FILE *file;
    struct member *members;
    size_t count;
    char *cache;
};

static unsigned u16(const unsigned char *b)
{
    return b[0] | (unsigned)b[1] << 8;
}

static uint32_t u32(const unsigned char *b)
{
    return u16(b) | (uint32_t)u16(b + 2) << 16;
}

static bool safe_name(const char *name)
{
    if (!*name || *name == '/' || strchr(name, '\\') || strchr(name, ':'))
        return false;
    while (*name) {
        const size_t n = strcspn(name, "/");
        if (!n || (n == 1 && name[0] == '.') ||
            (n == 2 && name[0] == '.' && name[1] == '.'))
            return false;
        name += n;
        if (*name)
            ++name;
    }
    return true;
}

static int by_name(const void *a, const void *b)
{
    return strcmp(((const struct member *)a)->name, ((const struct member *)b)->name);
}

struct ck2_zip *ck2_zip_open(const char *path)
{
    struct ck2_zip *zip = calloc(1, sizeof *zip);
    if (!zip)
        return NULL;
    zip->file = fopen(path, "rb");
    if (!zip->file || fseek(zip->file, 0, SEEK_END))
        goto fail;
    const long size = ftell(zip->file);
    if (size < 22)
        goto fail;
    const size_t tail_size = size < 65557 ? (size_t)size : 65557;
    unsigned char *tail = malloc(tail_size);
    if (!tail)
        goto fail;
    size_t offset = tail_size - 22;
    bool ok = !fseek(zip->file, size - (long)tail_size, SEEK_SET) &&
        fread(tail, 1, tail_size, zip->file) == tail_size;
    while (ok && (u32(tail + offset) != 0x06054b50u ||
           offset + 22 + u16(tail + offset + 20) != tail_size)) {
        if (!offset) {
            ok = false;
            break;
        }
        --offset;
    }
    const unsigned count = ok ? u16(tail + offset + 10) : 0;
    const uint32_t central = ok ? u32(tail + offset + 16) : 0;
    ok = ok && !u16(tail + offset + 4) && !u16(tail + offset + 6) && count < 65535;
    free(tail);
    if (!ok || central >= (unsigned long)size || fseek(zip->file, central, SEEK_SET))
        goto fail;
    zip->members = calloc(count, sizeof *zip->members);
    if (!zip->members && count)
        goto fail;
    for (unsigned i = 0; i < count; ++i) {
        unsigned char header[46];
        if (fread(header, 1, sizeof header, zip->file) != sizeof header ||
            u32(header) != 0x02014b50u)
            goto fail;
        const unsigned len = u16(header + 28);
        char *name = malloc((size_t)len + 1);
        if (!name)
            goto fail;
        if (fread(name, 1, len, zip->file) != len ||
            fseek(zip->file, u16(header + 30) + u16(header + 32), SEEK_CUR)) {
            free(name);
            goto fail;
        }
        name[len] = 0;
        const unsigned method = u16(header + 10);
        if (!len || strlen(name) != len || name[len - 1] == '/' || !safe_name(name) ||
            (u16(header + 8) & 1) || (method != 0 && method != 8)) {
            free(name);
            continue;
        }
        zip->members[zip->count++] = (struct member){
            name, u32(header + 42), u32(header + 24), u32(header + 20),
            u32(header + 16), method
        };
    }
    if (zip->count > 1)
        qsort(zip->members, zip->count, sizeof *zip->members, by_name);
    return zip;

fail:
    ck2_zip_close(zip);
    return NULL;
}

static bool extract(struct ck2_zip *zip, const struct member *member, const char *path)
{
    unsigned char header[30];
    if (fseek(zip->file, member->offset, SEEK_SET) ||
        fread(header, 1, sizeof header, zip->file) != sizeof header ||
        u32(header) != 0x04034b50u ||
        fseek(zip->file, u16(header + 26) + u16(header + 28), SEEK_CUR))
        return false;
    FILE *out = fopen(path, "wb");
    if (!out)
        return false;
    unsigned char input[32768], output[32768];
    uint32_t left = member->compressed;
    uint64_t written = 0;
    uLong crc = crc32(0, NULL, 0);
    z_stream stream = {0};
    bool deflate = member->method == 8;
    bool ok = deflate ? inflateInit2(&stream, -MAX_WBITS) == Z_OK : member->raw == left;
    const bool initialized = deflate && ok;
    int code = Z_OK;
    while (ok && (left || (deflate && code == Z_OK))) {
        const size_t n = left < sizeof input ? left : sizeof input;
        if (fread(input, 1, n, zip->file) != n) {
            ok = false;
            break;
        }
        left -= (uint32_t)n;
        stream.next_in = input;
        stream.avail_in = (unsigned)n;
        do {
            const unsigned char *bytes = input;
            size_t amount = n;
            if (deflate) {
                stream.next_out = output;
                stream.avail_out = sizeof output;
                code = inflate(&stream, Z_NO_FLUSH);
                amount = sizeof output - stream.avail_out;
                bytes = output;
            }
            written += amount;
            ok = (code == Z_OK || code == Z_STREAM_END) && written <= member->raw &&
                fwrite(bytes, 1, amount, out) == amount;
            if (ok)
                crc = crc32(crc, bytes, (unsigned)amount);
        } while (ok && deflate && code == Z_OK && (stream.avail_in || stream.avail_out == 0));
        if (deflate && code == Z_STREAM_END)
            break;
    }
    if (initialized)
        inflateEnd(&stream);
    if (fclose(out))
        ok = false;
    return ok && !left && (!deflate || code == Z_STREAM_END) &&
        written == member->raw && crc == member->crc;
}

size_t ck2_zip_count(const struct ck2_zip *zip)
{
    return zip->count;
}

const char *ck2_zip_name(const struct ck2_zip *zip, size_t i)
{
    return zip->members[i].name;
}

char *ck2_zip_asset(struct ck2_zip *zip, const char *name)
{
    const struct member key = {.name = (char *)name};
    const struct member *member = zip->count
        ? bsearch(&key, zip->members, zip->count, sizeof key, by_name) : NULL;
    if (!member)
        return NULL;
    if (!zip->cache) {
        char *pattern = strdup("/tmp/openck2-dlc-XXXXXX");
        if (!pattern || !mkdtemp(pattern)) {
            free(pattern);
            return NULL;
        }
        zip->cache = pattern;
    }
    char *path = ck2_join_path(zip->cache, name);
    if (!path || !access(path, R_OK))
        return path;
    for (char *p = path + strlen(zip->cache) + 1; (p = strchr(p, '/')); ++p) {
        *p = 0;
        mkdir(path, 0700);
        *p = '/';
    }
    if (extract(zip, member, path))
        return path;
    unlink(path);
    free(path);
    return NULL;
}

static int remove_entry(const char *path, const struct stat *info, int type, struct FTW *walk)
{
    (void)info;
    (void)type;
    (void)walk;
    return remove(path);
}

void ck2_zip_close(struct ck2_zip *zip)
{
    if (!zip)
        return;
    if (zip->file)
        fclose(zip->file);
    if (zip->cache)
        nftw(zip->cache, remove_entry, 16, FTW_DEPTH | FTW_PHYS);
    free(zip->cache);
    for (size_t i = 0; i < zip->count; ++i)
        free(zip->members[i].name);
    free(zip->members);
    free(zip);
}
