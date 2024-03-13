#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <unistd.h> 

#define MAX_INODE 1024
#define MAX_NAME 32
#define MAX_INODE_STRING 5

typedef struct{
    uint32_t inodeIndex;
    char type;
} inodeMeta;

typedef struct{
    uint32_t inodeIndex;
    char name[MAX_NAME];
} dirContent;

int inoZero(const inodeMeta* entries, int numEntries);
void cmdExit(const inodeMeta* entries, int numEntries);
void cmdLs(uint32_t curDirIno);
void cmdCd(const char* dirName, uint32_t* curDirInode, const inodeMeta* entries, int numEntries);

int main(int argc, char *argv[]) {
    //root inode starts at 0
    uint32_t curDirInode = 0;
//////////requirement 1////////////
    if (argc != 2) {
        fprintf(stderr, "incorrect # of arguments passed");
        exit(1);
    }
    
    //opendir verifies that directory exists, doesn't change working directory
    DIR *dirName = opendir(argv[1]);
    if (dirName == NULL) {
        printf("cannot open provided directory: %s\n", strerror(errno));
        exit(1);
    }
    printf("directory was checked\n");
    closedir(dirName);

////////requirement 2//////////
    //change to provided directory as working directory
    if(chdir(argv[1]) != 0){
        printf("couldn't change to provided directory: %s\n", strerror(errno));
        exit(1);
    }

    //read inodes list file in binary  (rb)
    FILE* inoFile = fopen("inodes_list", "rb");
    if (inoFile == NULL){
        printf("couldn't open inodes file");
        exit(1);
    }
    printf("inodes list file was opened\n");
    int numEntries = 0;
    inodeMeta entries[MAX_INODE];
    //hold inidividual inodemeta from fread
    inodeMeta buffer;
    while (fread(&buffer, sizeof(inodeMeta), 1, inoFile) == 1){
        if (buffer.type != 'd' && buffer.type != 'f'){
            printf("inode file type is invalid: %u %c\n", buffer.inodeIndex, buffer.type);
            continue;
        }
        if (buffer.inodeIndex >= MAX_INODE){
            printf("inode number is invalid: %u\n", buffer.inodeIndex);
            continue;
        }
        if (numEntries < MAX_INODE){
            entries[numEntries] = buffer;
            numEntries++;
        } else {
            printf("too many entries in inode file: %d", numEntries);
            exit(1);
        }
        printf("Valid inode entry: %u, Type: %c\n", buffer.inodeIndex, buffer.type);
    }
    fclose(inoFile);
    printf("inodes list file was closed\n");

//////////requirement 3////////
    if (inoZero(entries, numEntries) != 1){
        printf("inode 0 is not correctly set: %d\n", numEntries);
        exit(1);
    }
//////////requirement 4/////////////
    //run loop forever since cmdExit will exit
    while (1){
        char cmdInput[256];
        printf("Enter a command: ");
        if (fgets(cmdInput, sizeof(cmdInput), stdin) == NULL){
            printf("End of input detected. Exiting");
            cmdExit(entries, numEntries);
        }

        //replace \n with \0, strcspn returns length where the \n reached
        cmdInput[strcspn(cmdInput, "\n")] = 0;
        char* token;
        char* rest = cmdInput;
        token = strtok_r(rest, " ", &rest);

        //check for exit command save and exit
        if(strcmp(cmdInput, "exit") == 0){
            cmdExit(entries, numEntries);
        } else if (strcmp(cmdInput, "ls") == 0){
            cmdLs(curDirInode);
        } else if (strcmp(token, "cd") == 0){
            token = strtok_r(NULL, " ", &rest);
            if (token != NULL){
                cmdCd(token, &curDirInode, entries, numEntries);
            } else{
                printf("Invalid directory given: %s \n", token);
                continue;
            }
        } else{
            printf("Invalid command entered\n");
            continue;
        }
    }
    return 0;
}
void cmdCd(const char* dirName, uint32_t* curDirInode, const inodeMeta* entries, int numEntries){
    //max size is inode(1023) uint_32 to string
    char curFileName[MAX_INODE_STRING];
    //convert inode num to string
    sprintf(curFileName, "%u", *curDirInode);
    //read binary given the string of file name
    FILE *dirFile = fopen(curFileName, "rb");
    if (dirFile == NULL){
        printf("couldn't open current directory file %s\n", curFileName);
        exit(1);
    }

    //found? checker
    int foundHuh = 0;
    dirContent dirEntry;
    while (fread(&dirEntry, sizeof(dirContent), 1, dirFile) == 1){
        //if the name of the directory matches
        if(strcmp(dirEntry.name, dirName) == 0){
            //check if the type is a directory given the name
            for(int i = 0; i < numEntries; i++){
                if (entries[i].inodeIndex == dirEntry.inodeIndex && entries[i].type == 'd'){
                    foundHuh = 1;
                    //change current directory to the matched one 
                    *curDirInode = dirEntry.inodeIndex;
                    break;
                }
            }
        }
    }
    fclose(dirFile);

    if (foundHuh == 0){
        printf("Invalid directory given: %s\n", dirName);
    } else {
        printf("Directory changed: %s\n", dirName);
    }
}

void cmdLs(uint32_t curDirInode){
    //max size is inode(1023) uint_32 to string
    char curFileName[MAX_INODE_STRING];
    //convert inode num to string
    sprintf(curFileName, "%u", curDirInode);
    //read binary given the string of file name
    FILE *dirFile = fopen(curFileName, "rb");
    if (dirFile == NULL){
        printf("couldn't open current directory file %s\n", curFileName);
        exit(1);
    }

    dirContent dirEntry;
    //read one at a time
    while (fread(&dirEntry, sizeof(dirContent), 1, dirFile) == 1){
        //dirEntry.name[MAX_NAME] = '\0';
        printf("Inode: %u, Name: %s\n", dirEntry.inodeIndex, dirEntry.name);
    }
    fclose(dirFile);
}
void cmdExit(const inodeMeta* entries, int numEntries){
    //open inodes list but with wb to write to it 
    FILE *inoFile = fopen("inodes_list", "wb");
    if (inoFile == NULL){
        printf("couldn't open inodes file");
        exit(1);
    }

    if(fwrite(entries, sizeof(inodeMeta), numEntries, inoFile) != numEntries){
        printf("couldn't write to inodes file");
        exit(1);
    }
    fclose(inoFile);
    printf("Successfully written inodes_list and exiting now");
    exit(0);
}

int inoZero(const inodeMeta* entries, int numEntries){
    if (numEntries > MAX_INODE){
        printf("Too many entries!");
        exit(1);
    }
    for (int i = 0; i < numEntries; i++){
        if (entries[i].inodeIndex == 0 && entries[i].type == 'd'){
            return 1;
        }
    }
    return 0;
}
