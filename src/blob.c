#include "blob.h"


void get_file_path(char *file_path, char *object_hash) {
    // Assuming you store git-like objects as .git/objects/ab/cd... (just an example)
    snprintf(file_path, 100, ".mygit/objects/%.2s/%.38s", object_hash, object_hash + 2);
}

int hash_object(char *filename, int write_flag)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        fprintf(stderr, "Failed to open file %s\n", filename);
        return -1;
    }

    // Read the file contents
    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *file_contents = malloc(file_size);
    if (!file_contents)
    {
        fprintf(stderr, "Failed to allocate memory for file contents\n");
        fclose(f);
        return -1;
    }
    fread(file_contents, 1, file_size, f);
    fclose(f);

    // Create blob content: "blob <size>\0<file_contents>"
    char header[64];
    int header_len = snprintf(header, sizeof(header), "blob %zu", file_size);
    header[header_len] = '\0';
    // int header_len = snprintf(header, sizeof(header), "blob %zu\0", file_size);
    size_t blob_size = header_len + 1 + file_size;

    unsigned char *blob = malloc(blob_size);
    if (!blob)
    {
        fprintf(stderr, "Failed to allocate memory for blob\n");
        free(file_contents);
        return -1;
    }
    memcpy(blob, header, header_len);
    blob[header_len] = '\0';
    memcpy(blob + header_len + 1, file_contents, file_size);
    free(file_contents);

    // Compute the SHA-1 hash of the blob using CommonCrypto
    unsigned char hash[SHA_LEN];
    CC_SHA1(blob, (CC_LONG)blob_size, hash);

    // Convert hash to hex string
    char hash_str[SHA_LEN * 2 + 1];
    for (int i = 0; i < SHA_LEN; i++)
    {
        snprintf(hash_str + i * 2, 3, "%02x", hash[i]);
    }
    hash_str[SHA_LEN * 2] = '\0';
    printf("%s\n", hash_str);

    // Write the blob to the object store
    if (write_flag)
    {
        // Compress the blob data
        unsigned char compressed_blob[CHUNK];
        z_stream stream = {0};
        deflateInit(&stream, Z_BEST_COMPRESSION);

        stream.avail_in = blob_size;
        stream.next_in = blob;
        stream.next_out = compressed_blob;
        stream.avail_out = sizeof(compressed_blob);

        int status = deflate(&stream, Z_FINISH);
        if (status != Z_STREAM_END)
        {
            fprintf(stderr, "Failed to compress blob data\n");
            free(blob);
            return -1;
        }

        // Write the compressed blob data to the object store .git/objects<dir>/<file>
        char object_dir[256];
        snprintf(object_dir, sizeof(object_dir), "%s/%.2s", OBJ_DIR, hash_str);
        mkdir(object_dir, 0755);

        char object_path[256];
        snprintf(object_path, sizeof(object_path), "%s/%s", object_dir, hash_str + 2);
        FILE *object_file = fopen(object_path, "wb");
        if (!object_file)
        {
            fprintf(stderr, "Failed to open object file %s\n", object_path);
            free(blob);
            return -1;
        }
        fwrite(compressed_blob, 1, stream.total_out, object_file);
        fclose(object_file);
    }
    free(blob);
    return 0;
}