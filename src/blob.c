#include "blob.h"
#define SHA1_RAW_LEN 20
void get_file_path(char *file_path, char *object_hash)
{
    // Generate path like: .mygit/objects/ab/cdef... (Git-like)
    snprintf(file_path, PATH_MAX, "%s/%.2s/%.38s", OBJ_DIR, object_hash, object_hash + 2);
}

int decompress_blob(FILE *file, unsigned char **blob_data, size_t *blob_size){
    if (!file || !blob_data || !blob_size)
    {
        fprintf(stderr, "Invalid arguments to decompress_blob\n");
        return -1;
    }

    // Read the compressed data from the file
    fseek(file, 0, SEEK_END);
    size_t compressed_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    unsigned char *compressed_data = malloc(compressed_size);
    if (!compressed_data)
    {
        fprintf(stderr, "Failed to allocate memory for compressed data\n");
        return -1;
    }

    fread(compressed_data, 1, compressed_size, file);

    // Prepare for decompression
    z_stream stream = {0};
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    if (inflateInit(&stream) != Z_OK)
    {
        fprintf(stderr, "inflateInit failed\n");
        free(compressed_data);
        return -1;
    }

    stream.avail_in = compressed_size;
    stream.next_in = compressed_data;

    // Allocate buffer for decompressed data
    size_t buffer_size = CHUNK;
    unsigned char *buffer = malloc(buffer_size);
    if (!buffer)
    {
        fprintf(stderr, "Failed to allocate memory for decompression buffer\n");
        inflateEnd(&stream);
        free(compressed_data);
        return -1;
    }

    stream.avail_out = buffer_size;
    stream.next_out = buffer;

    // Decompress the data
    int status = inflate(&stream, Z_FINISH);
    
    if (status != Z_STREAM_END)
    {
        fprintf(stderr, "Decompression failed: %d\n", status);
        inflateEnd(&stream);
        free(buffer);
        free(compressed_data);
        return -1;
    }

    // Set output parameters
    *blob_data = buffer;
    *blob_size = stream.total_out;

    // Clean up
    inflateEnd(&stream);
    free(compressed_data);

    return 0;
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



void ls_tree(const char *tree_file, int name_only)
{
    size_t size;
    unsigned char *data = decompress_file(tree_file, &size);
    if (!data)
        return;

    // Skip header "tree <size>\0"
    unsigned char *p = data;
    while (*p != '\0' && (p - data) < size)
        p++;
    p++; // skip null byte

    // Parse entries
    while ((p - data) < (ptrdiff_t)size)
    {
        // Read mode string until space
        char mode[16];
        int i = 0;
        while (*p != ' ' && (p - data) < (ptrdiff_t)size && i < 15)
            mode[i++] = *p++;
        mode[i] = '\0';

        if (*p != ' ')
        {
            fprintf(stderr, "Malformed tree entry: no space after mode\n");
            break;
        }
        p++; // skip space

        // Read filename until null byte
        char name[256];
        i = 0;
        while (*p != '\0' && (p - data) < (ptrdiff_t)size && i < 255)
            name[i++] = *p++;
        name[i] = '\0';

        if (*p != '\0')
        {
            fprintf(stderr, "Malformed tree entry: no null byte after name\n");
            break;
        }
        p++; // skip null byte

        // Read 20-byte raw SHA
        if ((p - data) + SHA1_RAW_LEN > size)
        {
            fprintf(stderr, "Malformed tree entry: not enough bytes for SHA\n");
            break;
        }
        unsigned char sha_raw[SHA1_RAW_LEN];
        memcpy(sha_raw, p, SHA1_RAW_LEN);
        p += SHA1_RAW_LEN;

        if (name_only)
        {
            printf("%s\n", name);
        }
        else
        {
            char sha_hex[SHA1_RAW_LEN * 2 + 1];
            sha1_to_hex(sha_raw, sha_hex);

            // Determine type from mode: "040000" means tree, else blob
            const char *type = (strcmp(mode, "40000") == 0 || strcmp(mode, "040000") == 0) ? "tree" : "blob";

            printf("%s %s %s\t%s\n", mode, type, sha_hex, name);
        }
    }

    free(data);
}