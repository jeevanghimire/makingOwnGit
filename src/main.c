#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <zlib.h>


int main(int argc, char *argv[])
{
    // Disable output buffering
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    if (argc < 2)
    {
        fprintf(stderr, "Usage: ./your_program.sh <command> [<args>]\n");
        return 1;
    }

    const char *command = argv[1];

    if (strcmp(command, "init") == 0)
    {
        // You can use print statements as follows for debugging, they'll be visible
        // when running tests.
        fprintf(stderr, "Logs from your program will appear here!\n");

        // Uncomment this block to pass the first stage
        //
        if (mkdir(".git", 0755) == -1 || mkdir(".git/objects", 0755) == -1 ||
            mkdir(".git/refs", 0755) == -1)
        {
            fprintf(stderr, "Failed to create directories: %s\n", strerror(errno));
            return 1;
        }
        //
        FILE *headFile = fopen(".git/HEAD", "w");
        if (headFile == NULL)
        {
            fprintf(stderr, "Failed to create .git/HEAD file: %s\n", strerror(errno));
            return 1;
        }
        fprintf(headFile, "ref: refs/heads/main\n");
        fclose(headFile);

        printf("Initialized git directory\n");
    }
    else if (!strcmp(command, "cat-file"))
    {
        if (argc != 4 || strcmp(argv[2], "-p") != 0)
        {
            fprintf(stderr, "Usage: ./your_program.sh cat-file -p <hash>\n");
            return 1;
        }
        const char *object_hash = argv[3];
        char object_path[256];
        sprintf(object_path, ".git/objects/%c%c/%s", object_hash[0], object_hash[1],
                object_hash + 2);

        FILE *objectFile;
        if ((objectFile = fopen(object_path, "rb")) == NULL)
        {
            fprintf(stderr, "Failed to open object file: %s\n", strerror(errno));
            return 1;
        }

        fseek(objectFile, 0, SEEK_END);
        long size = ftell(objectFile);
        fseek(objectFile, 0, SEEK_SET);

        unsigned char *compressed_data = malloc(size);
        if ((fread(compressed_data, 1, size, objectFile)) != size)
        {
            fprintf(stderr, "Failed to read object file: %s\n", strerror(errno));
            fclose(objectFile);
            free(compressed_data);
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
        fwrite(content, 1, strlen(content), stdout);
    }
    else
    {
        fprintf(stderr, "Unknown command %s\n", command);
        return 1;
    }

    return 0;
}