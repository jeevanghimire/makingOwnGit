#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
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
        // You can use print statements as follows for debugging, they'll be visible when running tests.
        fprintf(stderr, "Logs from your program will appear here!\n");

        // Uncomment this block to pass the first stage
        //
        if (mkdir(".git", 0755) == -1 ||
            mkdir(".git/objects", 0755) == -1 ||
            mkdir(".git/refs", 0755) == -1)
        {
            fprintf(stderr, "Failed to create directories: %s\n", strerror(errno));
            return 1;
        }

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
    else if (strcmp(command, "cat-file") == 0)
    {
        if (argc != 4 || strcmp(argv[2], '-p') == 0)
        {
            fprintf(stderr, "Usage: ./your_program.sh cat-file -p <object>\n");
            return 1;
        }
        const char *objectHash = argv[3];
        char objectPath[256];
        snprintf(objectPath, sizeof(objectPath), ".git/objects/%c%c/%s",
                 objectHash[0], objectHash[1], objectHash + 2);
        FILE *objectFile = fopen(objectPath, "rb");
        if (objectFile == NULL)
        {
            fprintf(stderr, "Object %s not found\n", objectHash);
            return 1;
        }
        // Get the file size
        fseek(objectFile, 0, SEEK_END);
        long size = ftell(objectFile);
        fseek(objectFile, 0, SEEK_SET);  // Reset the file pointer to the beginning
        char *buffer = malloc(size + 1); // Allocate memory for the file content
        if (buffer == NULL)
        {
            fprintf(stderr, "Memory allocation failed\n");
            fclose(objectFile);
            return 1;
        }
        // Reading the file content
        unsigned char *compressed_data = malloc(size);
        if ((fread(compressed_data, 1, size, objectFile)) != size)
        {
            fprintf(stderr, "Failed to read object file %s\n", objectHash);
            free(compressed_data);
            fclose(objectFile);
            return 1;
        }
        fclose(objectFile);
        // Decompress the data

        unsigned char decompressed_data[65536]; // Adjust size as needed
        z_stream stream = {
            .next_in = compressed_data,
            .avail_in = size,
            .next_out = decompressed_data,
            .avail_out = sizeof(decompressed_data),
            .zalloc = Z_NULL,
            .zfree = Z_NULL,
        }; // Initialize zlib stream 
        if(inflateInit(&stream)!= Z_OK){
            fprintf(stderr, "Failed to initialize zlib stream\n");
            free(compressed_data);
            return 1;
        }
        if (inflate(&stream, Z_FINISH) != Z_STREAM_END)
        {
            fprintf(stderr, "Failed to decompress data from object file %s\n", objectHash);
            inflateEnd(&stream);
            free(compressed_data);
            return 1;
        }
        inflateEnd(&stream);// Clean up zlib stream
        free(compressed_data);
        // Print the decompressed data
        decompressed_data[stream.total_out] = '\0';// Null-terminate the decompressed data
        if (!decompressed_data[0]) // Check if the decompressed data is empty
        {
            fprintf(stderr, "Object %s is empty\n", objectHash);
            free(buffer);
            return 1;
        }
        else
        {
            printf("%s", decompressed_data); // Print the content of the object
        }
        free(buffer); // Free the allocated memory
    }
    else
    {
        fprintf(stderr, "Unknown command %s\n", command);
        return 1;
    }
    return 0;
}