#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <zlib.h>
#include "blob.h"

#define MAX_INPUT 1024
#define CHUNK_SIZE 4096

// function for different commands
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
        // Assuming the second argument is a file stream like stdout
        char *path = malloc(sizeof(char) * (SHA_LEN + 2 + strlen(OBJ_DIR)));
        get_file_path(path, argv[2]);
        FILE *blob_file = NULL;

        get_file_path(path, argv[2]);
        blob_file = fopen(path, "rb");
        if (blob_file == NULL)
        {
            fprintf(stderr, "Failed to open file %s: %s\n", path, strerror(errno));
            return 1;
        }
        cat_file(path);

        free(path);
        fclose(blob_file);
    }
    else if (strcmp(command, "hash-object") == 0)
    {
        if (argc != 3 || (strcmp(argv[1], "-w") != 0))
        {
            fprintf(stderr, "Usage: hash-object -w <file>\n");
            return 1;
        }
        return hash_object(argv[2], 1);
    }

    else
    {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}
