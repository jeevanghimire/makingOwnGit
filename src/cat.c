#include "blob.h"
int cat_file(char *path)
{
    const char *object_hash = path;
    char object_path[256];
    sprintf(object_path, ".git/objects/%c%c/%s", object_hash[0], object_hash[1],
            object_hash + 2);

    FILE *objectFile = fopen(object_path, "rb");
    if (!objectFile)
    {
        fprintf(stderr, "Failed to open object file: %s\n", strerror(errno));
        return 1;
    }

    fseek(objectFile, 0, SEEK_END);
    long size = ftell(objectFile);
    fseek(objectFile, 0, SEEK_SET);

    unsigned char *compressed_data = malloc(size);
    if (!compressed_data)
    {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(objectFile);
        return 1;
    }
    fclose(objectFile);
    // Read the compressed data from the object file
    unsigned char decompressed[65536];
    z_stream stream = {
        .next_in = compressed_data,
        .avail_in = size,
        .next_out = decompressed,
        .avail_out = sizeof(decompressed),
    };

    if (inflateInit(&stream) != Z_OK)
    {
        fprintf(stderr, "inflateInit failed\n");
        free(compressed_data);
        return 1;
    }
    if (inflate(&stream, Z_FINISH) != Z_STREAM_END)
    {
        fprintf(stderr, "inflate failed\n");
        inflateEnd(&stream);
        free(compressed_data);
        return 1;
    }
    inflateEnd(&stream);
    free(compressed_data);

    unsigned char *content = memchr(decompressed, 0, stream.total_out);
    if (!content)
    {
        fprintf(stderr, "Invalid object format\n");
        return 1;
    }
    content++;
    fwrite(content, 1, strlen((char *)content), stdout);
    return 0;
}