#include "blob.h"

void get_file_path(char *file_path, char *object_hash)
{
    // Generate path like: .mygit/objects/ab/cdef... (Git-like)
    snprintf(file_path, PATH_MAX, "%s/%.2s/%.38s", OBJ_DIR, object_hash, object_hash + 2);
}

int hash_object(char *filename, int write_flag)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        fprintf(stderr, "Failed to open file %s\n", filename);
        return -1;
    }

    // Read file contents
    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char *file_contents = malloc(file_size);
    if (!file_contents)
    {
        fprintf(stderr, "Failed to allocate memory\n");
        fclose(f);
        return -1;
    }

    fread(file_contents, 1, file_size, f);
    fclose(f);

    // Create blob content: "blob <size>\0<file_contents>"
    char header[64];
    int header_len = snprintf(header, sizeof(header), "blob %zu", file_size);
    size_t blob_size = header_len + 1 + file_size;

    unsigned char *blob = malloc(blob_size);
    if (!blob)
    {
        fprintf(stderr, "Failed to allocate blob memory\n");
        free(file_contents);
        return -1;
    }

    memcpy(blob, header, header_len);
    blob[header_len] = '\0';
    memcpy(blob + header_len + 1, file_contents, file_size);
    free(file_contents);

    // Hash the blob using SHA-1 (from CommonCrypto)
    unsigned char hash[SHA_LEN];
    CC_SHA1(blob, (CC_LONG)blob_size, hash);

    // Convert hash to hex string
    char hash_str[SHA_LEN * 2 + 1];
    for (int i = 0; i < SHA_LEN; i++)
    {
        snprintf(hash_str + i * 2, 3, "%02x", hash[i]);
    }
    hash_str[SHA_LEN * 2] = '\0';

    // Print hash
    printf("%s\n", hash_str);

    if (write_flag)
    {
        // Create object subdirectory .mygit/objects/ab
        char subdir_path[PATH_MAX];
        snprintf(subdir_path, sizeof(subdir_path), "%s/%.2s", OBJ_DIR, hash_str);
        mkdir(OBJ_DIR, 0755);               // Ignore if already exists
        mkdir(subdir_path, 0755);           // Create subdir like .mygit/objects/ab

        // Get full object path like .mygit/objects/ab/cdef...
        char object_path[PATH_MAX];
        get_file_path(object_path, hash_str);

        // Compress the blob
        unsigned char compressed_blob[CHUNK];
        z_stream stream = {0};
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;

        if (deflateInit(&stream, Z_BEST_COMPRESSION) != Z_OK)
        {
            fprintf(stderr, "deflateInit failed\n");
            free(blob);
            return -1;
        }

        stream.avail_in = blob_size;
        stream.next_in = blob;
        stream.next_out = compressed_blob;
        stream.avail_out = sizeof(compressed_blob);

        int status = deflate(&stream, Z_FINISH);
        if (status != Z_STREAM_END)
        {
            fprintf(stderr, "Blob compression failed\n");
            deflateEnd(&stream);
            free(blob);
            return -1;
        }

        deflateEnd(&stream);

        // Write compressed data to file
        FILE *out = fopen(object_path, "wb");
        if (!out)
        {
            fprintf(stderr, "Failed to write to object file %s\n", object_path);
            free(blob);
            return -1;
        }

        fwrite(compressed_blob, 1, stream.total_out, out);
        fclose(out);
    }

    free(blob);
    return 0;
}
