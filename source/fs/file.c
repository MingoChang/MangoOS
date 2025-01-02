/*
 * ==================================
 * auther: Mingo
 * date: 2024-12-9
 * ==================================
 */
#include "../include/file.h"

static file_t file_table[MAX_OPEN_FILES];

static mutex_t mutex;

void file_table_init()
{
    mutex_init(&mutex);
}

file_t* alloc_file()
{
    file_t *file = NULL;

    mutex_lock(&mutex);
    for (int i=0; i<MAX_OPEN_FILES; i++) {
        file = &file_table[i];
        if (file->ref == 0) {
            file->ref = 1;
            break;
        }
    }
    mutex_unlock(&mutex);

    return file;
}

void free_file(file_t *file)
{
    mutex_lock(&mutex);
    if (file->ref > 0) {
        file->ref--;
    }
    mutex_unlock(&mutex);
}

void file_inc_ref(file_t * file)
{
    mutex_lock(&mutex);
    file->ref++;
    mutex_unlock(&mutex);
}
