#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <zlib.h>
    


#define MAX_INPUT 1024
#define CHUNK_SIZE 4096

// function for different commands
int cat_file(const char *hash)
{
    const char *object_hash = hash;
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

int hash_object(char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        fprintf(stderr, "Failed to open file: %s\n", strerror(errno));
        return 1;
    }
    // Read file content

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *data = malloc(size);
    if (!data)
    {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(f);
        return 1;
    }
    fread(data, 1, size, f);
    unsigned char hash[20];
        for (int i = 0; i < 20; i++) {
            hash[i] = i;
        }
        for (int i = 0; i < 20; i++) {
            printf("%02x", hash[i]);
        }
        printf("\n");
    fclose(f);
return 0;
}

int main()
{
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    char input[MAX_INPUT];
    printf("Enter command: ");
    if (!fgets(input, sizeof(input), stdin))
    {
        fprintf(stderr, "Failed to read input.\n");
        return 1;
    }

    // Remove newline at end of input
    input[strcspn(input, "\n")] = '\0';

    // Tokenize the input into arguments
    char *argv[10];
    int argc = 0;
    char *token = strtok(input, " ");
    while (token && argc < 10)
    {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }

    if (argc == 0)
    {
        fprintf(stderr, "No command entered.\n");
        return 1;
    }

    const char *command = argv[0];

    if (strcmp(command, "init") == 0)
    {
        if (mkdir(".git", 0755) == -1 || mkdir(".git/objects", 0755) == -1 ||
            mkdir(".git/refs", 0755) == -1)
        {
            fprintf(stderr, "Failed to create directories: %s\n", strerror(errno));
            return 1;
        }

        FILE *headFile = fopen(".git/HEAD", "w");
        if (!headFile)
        {
            fprintf(stderr, "Failed to create .git/HEAD file: %s\n", strerror(errno));
            return 1;
        }
        fprintf(headFile, "ref: refs/heads/main\n");
        fclose(headFile);

        printf("Initialized git directory\n");
    }
    else if (strcmp(command, "cat-file") == 0)
    {
        if (argc != 3 || strcmp(argv[1], "-p") != 0)
        {
            fprintf(stderr, "Usage: cat-file -p <hash>\n");
            return 1;
        }
        const char *hash = argv[2];
        // call the cat_file function
        if (strlen(hash) != 40)
        {
            fprintf(stderr, "Invalid hash length. Expected 40 characters.\n");
            return 1;
        }
        if (cat_file(hash) != 0)
        {
            fprintf(stderr, "Failed to display file content for hash: %s\n", hash);
            return 1;
        }
    }
    else if (strcmp(command, "hash-object") == 0)
    {
        if (argc != 2)
        {
            fprintf(stderr, "Usage: hash-object <file>\n");
            return 1;
        }
        const char *file_path = argv[1];
        FILE *file = fopen(file_path, "rb");
        if (!file)
        {
            fprintf(stderr, "Failed to open file: %s\n", strerror(errno));
            return 1;
        }
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        fseek(file, 0, SEEK_SET);
        unsigned char *data = malloc(size);
        if (!data || fread(data, 1, size, file) != (size_t)size)
        {
            fprintf(stderr, "Failed to read file: %s\n", strerror(errno));
            fclose(file);
            free(data);
            return 1;
        }
        fclose(file);


        // implement the hash here using SHA-1
    }
    else
    {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}
