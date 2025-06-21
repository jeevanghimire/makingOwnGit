#ifndef BLOB_H
#define BLOB_H

/* Includes */
#include <CommonCrypto/CommonCrypto.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <CommonCrypto/CommonDigest.h>
#include <errno.h>
#include <curl/curl.h>
#include <ctype.h>
#include <zlib.h>
#define SHA_LEN 100
#define CHUNK 16384
#define BUFFER_SIZE 4096
#define OBJ_DIR ".git/objects"
#define COMMITTER_NAME "Dorine Tipo"
#define COMMITTER_EMAIL "dorine.a.tipo@gmail.com"

typedef struct
{
    unsigned char hash[20];
} sha1_t;

typedef struct
{
    char mode[7];
    char name[256];
    sha1_t sha;
} tree_entry;

/* Function prototypes */
void get_file_path(char *file_path, char *object_hash);
int decompress_blob(FILE *file, unsigned char **blob_data, size_t *blob_size);
int hash_object(char *filename, int write_flag);
void die(const char *msg);
int cat_file(char *path);
void read_git_object(const char *hash, unsigned char **data, size_t *size);
void parse_tree(const unsigned char *data, size_t size, int name_only);
void ls_tree(const char *tree_file, int name_only);
void compute_sha1(const unsigned char *data, size_t len, sha1_t *out);
sha1_t write_tree(const char *dirpath);
sha1_t write_blob(const char *filepath);
void get_timestamp(char *buffer, size_t size);
void sha1_hash(const char *data, size_t len, char *out);
void write_commit_object(const char *content, const char *sha);
int commit_tree(const char *tree_sha, const char *parent_sha, const char *message);
int clone_repo(const char *remote_url, const char *target_dir);

#endif