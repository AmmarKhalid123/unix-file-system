#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
// #include <string>
/*
 *   ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ 
 *  |   |   |   |   |                       |   |
 *  | 0 | 1 | 2 | 3 |     .....             |127|
 *  |___|___|___|___|_______________________|___|
 *  |   \    <-----  data blocks ------>
 *  |     \
 *  |       \
 *  |         \
 *  |           \
 *  |             \
 *  |               \
 *  |                 \
 *  |                   \
 *  |                     \
 *  |                       \
 *  |                         \
 *  |                           \
 *  |                             \
 *  |                               \
 *  |                                 \
 *  |                                   \
 *  |                                     \
 *  |                                       \
 *  |                                         \
 *  |                                           \
 *  |     <--- super block --->                   \
 *  |______________________________________________|
 *  |               |      |      |        |       |
 *  |        free   |      |      |        |       |
 *  |       block   |inode0|inode1|   .... |inode15|
 *  |        list   |      |      |        |       |
 *  |_______________|______|______|________|_______|
 *
 *
 */

#define FILENAME_MAXLEN 8 // including the NULL char

/* 
 * inode 
 */

typedef struct inode{
    int dir; // boolean value. 1 if it's a directory.
    char name[FILENAME_MAXLEN];
    int size;         // actual file/directory size in bytes.
    int blockptrs[8]; // direct pointers to blocks containing file's content.
    int used;         // boolean value. 1 if the entry is in use.
    int rsvd;         // reserved for future use
} inode;

/* 
  * directory entry
  */

typedef struct dirent{
    char name[FILENAME_MAXLEN];
    int namelen; // length of entry name
    int inode;   // this entry inode index
} dirent;

typedef struct superblock{
    // char super_freelist;
    char freelist[128];
    inode inodelst[16];
} superblock;

typedef union block{
    char data[1024];
    dirent dirdata[1024 / sizeof(dirent)];
} block;

typedef struct harddisk{
    superblock sb;
    block blocks[127];
} harddisk;

/*
 * functions
 */

// checks if a starts from "/"
int isAbsolutePath(char *a){
    return strncmp(a, "/", 1);
}

// checks if there is a dir named "name" in the inode in
// returns -1 if "name" named dir doesnt exist in "in"(directory) else return inode no. of that directory
int pathExists(char *name, inode in, harddisk* sys){
    for (int i = 0; i < 8; i++){
        if (in.blockptrs[i] != -1){
            for (int j = 0; j < 64; j++){
                if (strcmp(sys->blocks[in.blockptrs[i]].dirdata[j].name, name) == 0){
                    return sys->blocks[in.blockptrs[i]].dirdata[j].inode;
                }
            }
        }
    }
    return -1;
}

//checks if "a" is a Valid Path
// args: sys: fs
//       a: path
//       src: if 1: include the last dir or file name in the end of path for validity
//               0: nclude the last dir or file name in the end of path for validity
int validPath(harddisk *sys, char *a, int src, int mv){
    char *token = strtok(a, "/");
    char *finall = strtok(NULL, "/");    
    int inode_num = pathExists(token, sys->sb.inodelst[0], sys); // inode number of dir to traverse
    
    if (src == 1){ // given path "a" is a source path
        if (inode_num == -1){
            printf("the directorys %s in the given path does not exist.\n", token);
            return inode_num;
        }
        while (finall != NULL){
            strcpy(token, finall);
            finall = strtok(NULL, "/");
            inode_num = pathExists(token, sys->sb.inodelst[inode_num], sys);
            if (inode_num == -1){
                if (mv == 0){
                    printf("the directorys %s in the given path doesnot exist.\n", token);
                }
                return -1;
            }
        }
        return inode_num;
    }
    else{
        if (finall == NULL){
            return 0;
        }
        else{
            while (finall != NULL && (inode_num != -1)){

                strcpy(token, finall);
                finall = strtok(NULL, "/");
                if (finall == NULL){
                    break;
                }
                inode_num = pathExists(token, sys->sb.inodelst[inode_num], sys);
            }
            if (inode_num == -1){
                printf("the directory %s in the given path does not exist.\n", token);
            }
            return inode_num;
        }
    }
}

// checks if file or directory in "a" already exists or not in path, where "a" is complete path
int isUniqueName(harddisk *sys, char *a)
{
    char *token = strtok(a, "/");
    char *finall = strtok(NULL, "/");
    printf("%s\n", token);
    int inode_num = pathExists(token, sys->sb.inodelst[0], sys); // inode number of dir to traverse
    
    if (finall == NULL){
        if (inode_num == -1){
            return 1;
        }
        else{
            return -1;
        }
    }
    else{
        while (finall != NULL){
            strcpy(token, finall);
            finall = strtok(NULL, "/");
            inode_num = pathExists(token, sys->sb.inodelst[inode_num], sys);
            if (finall == NULL){
                break;
            }
        }
        if (inode_num == -1){
            return 1;
        }
        else{
            return -1;
        }
    }
}
//returns the free inode number
int find_inode(harddisk *sys){
    for (int i = 0; i < 16; i++){
        if (sys->sb.inodelst[i].used == 0){
            return i;
        }
    }
    return -1;
}

//returns the free block num
int find_free_block(harddisk *sys){
    for (int p = 0; p < 128; p++){
        if ((strcmp(sys->sb.freelist + p, "\0") == 0) | (strcmp(sys->sb.freelist + p, "0") == 0)){
            strcpy(sys->sb.freelist + p, "1");
            return p;
        }
    }
    return -1;
}

//checks if a, which is a path, has length greater than 8
int eight_char(char *a){
    char *token = strtok(a, "/");
    if (token == NULL){
        return 1;
    }
    char *final = strtok(NULL, "/");
    while (final != NULL){

        strcpy(token, final);
        final = strtok(NULL, "/");
    }
    if (strlen(token) < 8){
        return 1;
    }
    else{
        return -1;
    }
    
}

// ---------------------------------------------------- //
//create directory
// dirname: name of directory to be created
// in: inode number of parent directory
// i_num: inode number in which directory will be created
void createOnlyDirectory(char *dirname, int in, int i_num, harddisk *sys)
{
    int block_num, dirdata_num;
    int free_block = -1;
    for (block_num = 0; block_num < 8; block_num++){
        dirdata_num = 0;
        if (sys->sb.inodelst[in].blockptrs[block_num] == -1){
            free_block = find_free_block(sys);
            break;
        }

        while (dirdata_num < 64){
            if ((sys->blocks[sys->sb.inodelst[in].blockptrs[block_num]].dirdata[dirdata_num].namelen == 0)){
                break;
            }
            dirdata_num++;
        }
        if (dirdata_num < 64){
            break;
        }
    }
    //initializing dirent: "block_num" <- index of blocks -- and -- index of dirdata -> "dirdata_num"
    if (free_block > -1){
        sys->blocks[free_block].dirdata[0].namelen = strlen(dirname);
        sys->blocks[free_block].dirdata[0].inode = i_num;
        strcpy(sys->blocks[free_block].dirdata[0].name, dirname);
        sys->sb.inodelst[in].blockptrs[block_num] = free_block;
    }
    else{
        sys->blocks[sys->sb.inodelst[in].blockptrs[block_num]].dirdata[dirdata_num].namelen = strlen(dirname);
        sys->blocks[sys->sb.inodelst[in].blockptrs[block_num]].dirdata[dirdata_num].inode = i_num;
        strcpy(sys->blocks[sys->sb.inodelst[in].blockptrs[block_num]].dirdata[dirdata_num].name, dirname);
    }
    //updating size and blocks of parent inode
    // sys->sb.inodelst[in].size += sizeof(dirent);
    // in->blockptrs
    //creating inode for this dir
    sys->sb.inodelst[i_num].used = 1;
    strcpy(sys->sb.inodelst[i_num].name, dirname);
    sys->sb.inodelst[i_num].dir = 1;
}

// creates a directory where "a" is absolute path
void createDirectory(harddisk *sys, char *a){
    char *b = strdup(a);

    if (eight_char(b) == -1){
        printf("Cant handle directory/file name greater than 8 including NULL char.\n");
    }
    else{
        if (isAbsolutePath(a) == 0){
            if (strcmp(a, "/") == 0){ //to create root directory
                sys->sb.inodelst[0].used = 1;
                sys->sb.inodelst[0].dir = 1;
                strcpy(sys->sb.inodelst[0].name, "/");
            }
            else{ //to create any directory other than root
                char *cpy_one = strdup(a);
                char *cpy_two = strdup(a);
                if (validPath(sys, cpy_two, 0, 0) == -1){

                }
                else if (isUniqueName(sys, cpy_one) != 1){
                    printf("Directory Already Exist!\n");
                }
                else{

                    char *token = strtok(a, "/");
                    char *finall = strtok(NULL, "/");
                    // sys->sb.inodelst[0].size += sizeof(dirent);
                    int inode_num = pathExists(token, sys->sb.inodelst[0], sys); // inode number of dir to traverse
                    int new_inode_num;
                    if (finall == NULL){
                        new_inode_num = find_inode(sys);
                        if (new_inode_num == -1){
                            printf("Can't handle more than 16 file/directory.\n");
                        }
                        else{
                            // sys->sb.inodelst[0].size -= sizeof(dirent);
                            createOnlyDirectory(token, 0, new_inode_num, sys);
                        }
                    }
                    else{
                        while (finall != NULL && (inode_num != -1)){
                            strcpy(token, finall);
                            finall = strtok(NULL, "/");
                            if (finall == NULL){
                                break;
                            }
                            // sys->sb.inodelst[inode_num].size += sizeof(dirent);
                            inode_num = pathExists(token, sys->sb.inodelst[inode_num], sys);
                        }
                        if (inode_num == -1){
                            printf("the directory %s in the given path does not exist!\n", token);
                        }
                        else{
                            new_inode_num = find_inode(sys);
                            if (new_inode_num == -1){
                                printf("Can't handle more than 16 file/directory.\n");
                            }
                            else{
                                // sys->sb.inodelst[0].size -= sizeof(dirent);
                                createOnlyDirectory(token, inode_num, new_inode_num, sys);
                            }
                        }
                    }
                }
                free(cpy_two);
                free(cpy_one);
                }
        }
        else{
            char *token = strtok(a, "/");
            if (token == NULL){
                printf("the directory  in the given path does not exist!\n");
            }
            else
            {
                printf("the directory %s in the given path does not exist!\n", token);
            }
        }
    }
    free(b);
};

// ---------------------create file--------------------- //
// dirname: name of file to be created
// in: inode number of parent directory
// i_num: inode number in which file will be created
// size: size of file in Bytes
void createOnlyFile(char *dirname, int in, int i_num, harddisk *sys, int size){
    int block_num, dirdata_num;
    int free_block = -1;
    for (block_num = 0; block_num < 8; block_num++){
        dirdata_num = 0;
        if (sys->sb.inodelst[in].blockptrs[block_num] == -1){
            free_block = find_free_block(sys);
            break;
        }

        while (dirdata_num < 64){
            if ((sys->blocks[sys->sb.inodelst[in].blockptrs[block_num]].dirdata[dirdata_num].namelen == 0)){
                break;
            }
            dirdata_num++;
        }
        if (dirdata_num < 64){
            break;
        }
    }
    //initializing dirent: "block_num" <- index of blocks -- and -- index of dirdata -> "dirdata_num" and blockptrs of parent
    if (free_block > -1)
    {
        sys->blocks[free_block].dirdata[0].namelen = strlen(dirname);
        sys->blocks[free_block].dirdata[0].inode = i_num;
        strcpy(sys->blocks[free_block].dirdata[0].name, dirname);
        sys->sb.inodelst[in].blockptrs[block_num] = free_block;
    }
    else
    {
        sys->blocks[sys->sb.inodelst[in].blockptrs[block_num]].dirdata[dirdata_num].namelen = strlen(dirname);
        sys->blocks[sys->sb.inodelst[in].blockptrs[block_num]].dirdata[dirdata_num].inode = i_num;
        strcpy(sys->blocks[sys->sb.inodelst[in].blockptrs[block_num]].dirdata[dirdata_num].name, dirname);
    }
    //updating size and blocks of parent inode
    sys->sb.inodelst[in].size += size;
    // in->blockptrs
    //creating inode for this dir
    sys->sb.inodelst[i_num].used = 1;
    strcpy(sys->sb.inodelst[i_num].name, dirname);
    sys->sb.inodelst[i_num].dir = 0;
    sys->sb.inodelst[i_num].size = size;
    free_block = find_free_block(sys);
    int total_blocks = 0;
    sys->sb.inodelst[i_num].blockptrs[total_blocks] = free_block;
    //writing on block no. "free_block" with size
    int i = 0, j = 0;
    char c;
    if (size < 26){
        for (c = 'a'; c <= 'z'; ++c){
            if (i == size){
                break;
            }
            sys->blocks[free_block].data[i] = c;
            i++;
        }
    }
    else{
        for (int k = 0; k < floor(size / 26); k++){
            for (c = 'a'; c <= 'z'; ++c){
                sys->blocks[free_block].data[i] = c;
                i++;
                j++;

                if (i == 1024){
                    i = 0;
                    total_blocks++;
                    free_block = find_free_block(sys);
                    sys->sb.inodelst[i_num].blockptrs[total_blocks] = free_block;
                }
            }
        }
        i = 0;
        for (c = 'a'; c <= 'z'; ++c){
            if (j == size){
                break;
            }
            sys->blocks[free_block].data[i] = c;
            i++;
            j++;
        }
    }
}
//creates file in absolute path "a" of size "size"
void createFile(harddisk *sys, char *a, int size){
    char *b = strdup(a);
    if (size > 8192){
        printf("Cant handle file size greater than 8KB.\n");
    }
    else if (eight_char(b) == -1){
        printf("Cant handle directory/file name greater than 8 characters including NULL.\n");
    }
    else{
        if (isAbsolutePath(a) == 0){
            char *cpy_one = strdup(a);
            char *cpy_two = strdup(a);
            if (validPath(sys, cpy_two, 0, 0) == -1){
            }
            else if (isUniqueName(sys, cpy_one) != 1){
                printf("File Already Exist!\n");
            }
            else{
                char *token = strtok(a, "/");
                char *finall = strtok(NULL, "/");
                sys->sb.inodelst[0].size += size;
                int inode_num = pathExists(token, sys->sb.inodelst[0], sys); // inode number of dir to traverse
                int new_inode_num;
                if (finall == NULL){
                    new_inode_num = find_inode(sys);
                    if(new_inode_num == -1){
                        printf("Cannot handle more than 16 files!\n");
                    }
                    else{
                        sys->sb.inodelst[0].size -= size;
                        createOnlyFile(token, 0, new_inode_num, sys, size);
                    }
                    
                }
                else{
                    while (finall != NULL && (inode_num != -1)){
                        strcpy(token, finall);
                        finall = strtok(NULL, "/");
                        if (finall == NULL){
                            break;
                        }
                        sys->sb.inodelst[inode_num].size += size;
                        inode_num = pathExists(token, sys->sb.inodelst[inode_num], sys);
                    }
                    if (inode_num != -1){
                        new_inode_num = find_inode(sys);
                        if(new_inode_num == -1){
                            printf("Cannot handle more than 16 files!\n");
                        }
                        else{
                            createOnlyFile(token, inode_num, new_inode_num, sys, size);
                        }

                    }
                }
            }
            free(cpy_two);
            free(cpy_one);
        }
        else{
            char *token = strtok(a, "/");
            if (token == NULL){
                printf("the directory  in the given path does not exist!\n");
            }
            else{
                printf("the directory %s in the given path does not exist!\n", token);
            }
        }
    }
    free(b);
}

// ------------------------remove/delete file------------------------- //
void deleteFile(harddisk *sys, char *a){
    char *cpy_one = strdup(a);
    char *cpy_two = strdup(a);
    int i_num = validPath(sys, cpy_one, 1, 0);
    int parent_i_num = validPath(sys, cpy_two, 0, 0);
    //check correction of paths
    if (i_num == -1){
    }
    char *token = strtok(a, "/");
    char *finall = strtok(NULL, "/");
    int inode_num = pathExists(token, sys->sb.inodelst[0], sys); // inode number of dir to traverse
    sys->sb.inodelst[0].size -= sys->sb.inodelst[i_num].size;
    // int new_inode_num;

    while (finall != NULL){

        sys->sb.inodelst[inode_num].size -= sys->sb.inodelst[i_num].size;
        strcpy(token, finall);
        finall = strtok(NULL, "/");
        inode_num = pathExists(token, sys->sb.inodelst[inode_num], sys);
    }
    //update parent
    int block_num, p = 0, b = 0, c = 0;
    while(sys->sb.inodelst[parent_i_num].blockptrs[p] != -1){
        block_num = sys->sb.inodelst[parent_i_num].blockptrs[p];
        for (int k = 0; k < 1024/sizeof(dirent); k++){
            if (sys->blocks[block_num].dirdata[k].namelen != 0){
                if (strcmp(sys->blocks[block_num].dirdata[k].name, sys->sb.inodelst[i_num].name) == 0){
                    sys->blocks[block_num].dirdata[k].namelen = 0;
                    strcpy(sys->blocks[block_num].dirdata[k].name, "");
                    sys->blocks[block_num].dirdata[k].inode = 0;
                    for (int q = 0; q < 1024/sizeof(dirent); q++){
                        if (sys->blocks[block_num].dirdata[q].namelen != 0){
                            b = 1;
                        }
                    }
                    c = 1;
                    break;
                }
            }
        }
        if (c == 1){
            break;
        }
        p++;
    }
    if (b == 0){
        strcpy(sys->sb.freelist + sys->sb.inodelst[parent_i_num].blockptrs[p], "0");
        sys->sb.inodelst[parent_i_num].blockptrs[p] = -1; 
    }

    //update i_node
    strcpy(sys->sb.inodelst[i_num].name, "");
    sys->sb.inodelst[i_num].size = 0;
    sys->sb.inodelst[i_num].used = 0;
    int i = 0;
    while (sys->sb.inodelst[i_num].blockptrs[i] != -1){
        // "Emptying block sys->sb.inodelst[i_num].blockptrs[i] Before: %s\n"
        strcpy(sys->blocks[sys->sb.inodelst[i_num].blockptrs[i]].data, "\0");
        sys->sb.inodelst[i_num].blockptrs[i] = -1;
        i++;
        if (i >= 8){
            break;
        }
    }

    free(cpy_one);
    free(cpy_two);
}


// ---------------------------copy file------------------------- //
void copyFile(harddisk *sys, char *src, char *dest){
    char *b = strdup(dest);
    if (eight_char(b) == -1){
        printf("Cant handle directory/file name name greater than 8.\n");
    }
    else{
        char *dest_cpy = strdup(dest);
        char *dest_cpy_two = strdup(dest);
        char *dest_cpy_three = strdup(dest);
        char *src_cpy = strdup(src);
        char *src_cpy_two = strdup(src);
        int src_inode = validPath(sys, src_cpy, 1, 0);
        int dest_inode = validPath(sys, dest_cpy, 1, 0);
        //check correction of paths
        
        if (src_inode == -1){
        }
        else if (sys->sb.inodelst[src_inode].dir == 1){
            printf("Can't handle directory.\n");
        }
        else if (sys->sb.inodelst[dest_inode].dir == 1){
            printf("Can't handle directory.\n");
        }
        else if (validPath(sys, dest_cpy, 0, 0) == -1){
        }
        //check if dest already exists: create
        else if (isUniqueName(sys, dest_cpy_two) == 1){
            //check for space (empty inode!)
            int free_inode = find_inode(sys);
            if (free_inode == -1){
                printf("Not enough space!\n");
            }
            else{
                printf("dest to copy to %s\n", dest_cpy_three);
                createFile(sys, dest_cpy_three, sys->sb.inodelst[src_inode].size);
            }
        }
        //overwrite
        else{
            deleteFile(sys, dest);
            createFile(sys, dest, sys->sb.inodelst[src_inode].size);
        }

        free(dest_cpy);
        free(dest_cpy_two);
        free(dest_cpy_three);
        free(src_cpy);
        free(src_cpy_two);
    }
    free(b);
}


// ----------------move a file---------------------- //
void moveFile(harddisk *sys, char *src, char *dest){
    char *dest_cpy = strdup(dest);
    char *dest_cpy_two = strdup(dest);
    char *dest_cpy_three = strdup(dest);
    char *src_cpy = strdup(src);
    char *src_cpy_two = strdup(src);
    int src_inode = validPath(sys, src_cpy, 1, 0);
    int dest_inode = validPath(sys, dest_cpy, 1, 1);
    //check correction of paths
    if (dest_inode != -1){
        if (sys->sb.inodelst[dest_inode].dir == 1){
            printf("Cannot handle directories.\n");
        }
    }
    else{
        dest_inode = validPath(sys, dest_cpy_three, 0, 0);
        if (sys->sb.inodelst[src_inode].dir == 1){
            printf("Can't handle directories.\n");
        }
        else if (dest_inode != -1){
         
        }
        else if (isUniqueName(sys, dest_cpy_two) == 1){
            //check for space (empty inode!)
            int size = sys->sb.inodelst[src_inode].size;
            deleteFile(sys, src);
            createFile(sys, dest, size);
        }
        //overwrite
        else{
            int size = sys->sb.inodelst[src_inode].size;
            deleteFile(sys, src);
            deleteFile(sys, dest_cpy_three);
            createFile(sys, dest, size);
        }

    }
    
    free(dest_cpy);
    free(dest_cpy_two);
    free(dest_cpy_three);
    free(src_cpy);
    free(src_cpy_two);
}

// --------------------list file info----------------------- //
void listFile(harddisk *sys){
    for (int i = 0; i < 16; i++){
        if (sys->sb.inodelst[i].used == 1){
            if (sys->sb.inodelst[i].dir == 1){
                printf("Directory ");
            }
            if (sys->sb.inodelst[i].dir == 0){
                printf("File ");
            }
            printf("Name: %s  Size: %d\n", sys->sb.inodelst[i].name, sys->sb.inodelst[i].size);
        }
    }
}

// ---------------------------------------------------- //
// remove a directory
void deleteDirectory(harddisk *sys, char *a){
    char *copy_one = strdup(a);
    char *copy_two = strdup(a);
    char *copy_four = strdup(a);
    char *copy_three = strdup(a);
    int i_num = validPath(sys, copy_three, 1, 0);
    int parent_inode = validPath(sys, copy_four, 0, 0);
    free(copy_four);
    if (i_num == -1){
    }
    else if (eight_char(copy_two) == -1){
        printf("Directory/file name should be less than 8 characters including NULL character.\n");
    }
    else{
        int i = 0;
        char s[100];
        while(sys->sb.inodelst[i_num].blockptrs[i] != -1){
            int block_num = sys->sb.inodelst[i_num].blockptrs[i];
            for(int j = 0; j < 1024/sizeof(dirent); j++){
                if (sys->blocks[block_num].dirdata[j].namelen != 0){

                    int k = sys->blocks[block_num].dirdata[j].inode;
                    if (sys->sb.inodelst[k].dir == 0){
                        strcpy(s, sys->sb.inodelst[k].name);
                        strcat(copy_one, "/");
                        strcat(copy_one, s);
                        deleteFile(sys, copy_one);
                    }
                    else if(sys->sb.inodelst[k].dir == 1){
                        strcpy(s, sys->sb.inodelst[k].name);
                        strcat(copy_one, "/");
                        strcat(copy_one, s);
                        deleteDirectory(sys, copy_one);
                    }
                }
            }
            i++;
            if (i >= 8){
                break;
            }
        }
        //free parent me dirent
        int block_num, p = 0, b = 0, c = 0;
        while(sys->sb.inodelst[parent_inode].blockptrs[p] != -1){
            block_num = sys->sb.inodelst[parent_inode].blockptrs[p];
            for (int k = 0; k < 1024/sizeof(dirent); k++){
                if (sys->blocks[block_num].dirdata[k].namelen != 0){
                    if (strcmp(sys->blocks[block_num].dirdata[k].name, sys->sb.inodelst[i_num].name) == 0){
                        sys->blocks[block_num].dirdata[k].namelen = 0;
                        strcpy(sys->blocks[block_num].dirdata[k].name, "");
                        sys->blocks[block_num].dirdata[k].inode = 0;
                        for (int q = 0; q < 1024/sizeof(dirent); q++){
                            if (sys->blocks[block_num].dirdata[q].namelen != 0){
                                b = 1;
                            }
                        }
                        c = 1;
                        break;
                    }
                }
            }
            if (c == 1){
                break;
            }
            p++;
        }
        if (b == 0){
            strcpy(sys->sb.freelist + sys->sb.inodelst[parent_inode].blockptrs[p], "0");
            sys->sb.inodelst[parent_inode].blockptrs[p] = -1;
        }
        //free self inode
        strcpy(sys->sb.inodelst[i_num].name, "");
        sys->sb.inodelst[i_num].size = 0;
        sys->sb.inodelst[i_num].used = 0;
        sys->sb.inodelst[i_num].dir = 0;
        int f = 0;
        while (sys->sb.inodelst[i_num].blockptrs[f] != -1){
            // Emptying block sys->sb.inodelst[i_num].blockptrs[f]
            strcpy(sys->blocks[sys->sb.inodelst[i_num].blockptrs[f]].data, "\0");
            sys->sb.inodelst[i_num].blockptrs[f] = -1;
            f++;
        }
    }
    free(copy_one);
    free(copy_two);
    free(copy_three);

}

void write_back(harddisk *sys){
    FILE *myfss = fopen("myfs", "wb");
    fwrite(sys, sizeof(harddisk), 1, myfss);
    fclose(myfss);
}

/*
 * main
 */
int main(int argc, char *argv[])
{
    if (argc != 2){
        printf("Usage ./a.out filename\n.");
        return 0;
    }

    //reading command file
    FILE *commands;
    char command[138];
    commands = fopen(argv[1], "r");
    char *command_name;

    if (commands == NULL){
        printf("Couldn't read file! + %s\n", argv[1]);
        return 0;
    }
    //initializing harddisk and creating root dir
    harddisk system;
    for (int i = 0; i < 16; i++){
        for (int j = 0; j < 8; j++){
            system.sb.inodelst[i].blockptrs[j] = -1;
        }
    }
    //checking for myfs
    FILE *myfs = fopen("myfs", "rb");
    if (myfs == NULL){
        // if myfs is not present, initializing our system with root directory and writing to myfs 
        char a[] = "/";
        createDirectory(&system, a);
        write_back(&system);
    }
    else{
        // if myfs is present, reading and retaining the state
        fread(&system, sizeof(harddisk), 1, myfs);
        fclose(myfs);
    }
    //reading each line of txt file and calling appropriate functions
    while (fgets(command, sizeof(command), commands))
    {
        command_name = strtok(command, " ");
        if (strcmp(command_name, "CR") == 0){
            char *filepath = strtok(NULL, " ");
            char *size = strtok(NULL, "\n");
            createFile(&system, filepath, atoi(size));
            write_back(&system);
        }
        else if (strcmp(command_name, "DL") == 0){
            char *filepath = strtok(NULL, "\n");
            deleteFile(&system, filepath);
            write_back(&system);
        }

        else if (strcmp(command_name, "CP") == 0){
            printf("CP START\n");
            char *src = strtok(NULL, " ");
            char *dest = strtok(NULL, "\n");
            copyFile(&system, src, dest);
            write_back(&system);
            printf("CP START\n");
        }

        else if (strcmp(command_name, "MV") == 0){
            char *src = strtok(NULL, " ");
            char *dest = strtok(NULL, "\n");
            moveFile(&system, src, dest);
            write_back(&system);
        }
        else if (strcmp(command_name, "CD") == 0){
            char *filepath = strtok(NULL, "\n");
            createDirectory(&system, filepath);
            write_back(&system);
        }
        else if (strcmp(command_name, "DD") == 0){

            char *filepath = strtok(NULL, "\n");
            deleteDirectory(&system, filepath);
            write_back(&system);

        }
        else if (strcmp(command_name, "LL\n") == 0){
            listFile(&system);
        }
    }
    fclose(commands);

    return 0;
}
