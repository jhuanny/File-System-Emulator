#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
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
int entryHuh(const char* dirName, uint32_t dirIno);
int findNextIno(const inodeMeta* entries, int numEntries, uint32_t* nextIno);
int updateDir(uint32_t curDirInode, uint32_t newIno, const char* dirName);
void cmdExit(const inodeMeta* entries, int numEntries);
void cmdLs(uint32_t curDirIno);
void cmdCd(const char* dirName, uint32_t* curDirInode, const inodeMeta* entries, int numEntries);
void cmdTouch(const char* dirName, uint32_t* curDirInode, inodeMeta* entries, int *numEntries);
void cmdMkdir(const char* dirName, uint32_t* curDirInode, inodeMeta* entries, int* numEntries);
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
    uint32_t inoIndex;
    char typ;
    while (numEntries < MAX_INODE && fread(&inoIndex, sizeof(uint32_t), 1, inoFile) == 1){
        if (fread(&typ, sizeof(char), 1, inoFile) == 1){
            if (typ != 'd' && typ != 'f'){
            printf("inode file type is invalid: %u %c\n", inoIndex, typ);
            continue;
        }
        if (inoIndex >= MAX_INODE){
            printf("inode number is invalid: %u\n", inoIndex);
            continue;
        }
        if (numEntries < MAX_INODE){
            entries[numEntries].inodeIndex = inoIndex;
            entries[numEntries].type = typ;
            printf("entry type: %c index: %d numEntries: %d\n", entries[numEntries].type, entries[numEntries].inodeIndex, numEntries);
            numEntries++;
        } else {
            printf("too many entries in inode file: %d", numEntries);
            exit(1);
            }
        printf("Valid inode entry: %u, Type: %c\n", inoIndex, typ);

        }
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
        printf("\nEnter a command: ");
        if (fgets(cmdInput, sizeof(cmdInput), stdin) == NULL){
            printf("End of input detected. Exiting");
            cmdExit(entries, numEntries);
        }

        //replace \n with \0, strcspn returns length where the \n reached
        cmdInput[strcspn(cmdInput, "\n")] = 0;
        char* token;
        char* rest = cmdInput;
        token = strtok(rest, " ");

        //check for exit command save and exit
        if(strcmp(cmdInput, "exit") == 0){
            cmdExit(entries, numEntries);
        } else if (strcmp(cmdInput, "ls") == 0){
            cmdLs(curDirInode);
        } else if (strcmp(token, "cd") == 0){
            token = strtok(NULL, " ");
            if (token != NULL){
                //pass address of curdirinode so we can change
                cmdCd(token, &curDirInode, entries, numEntries);
            } else{
                printf("Invalid directory name given in main: %s \n\n", token);
                continue;
            }
        } else if (strcmp(token, "touch") == 0){
            token = strtok(NULL, " ");
            if (token != NULL){
                //pass address of curdirinode so we can change
                cmdTouch(token, &curDirInode, entries, &numEntries);
            } else{
                printf("File name not given %s \n\n", token);
                continue;
            }
        } else if (strcmp(token, "mkdir") == 0){
            token = strtok(NULL, " ");
            if (token != NULL){
                //pass address of curdirinode so we can change
                cmdMkdir(token, &curDirInode, entries, &numEntries);
            } else{
                printf("File name not given %s \n\n", token);
                continue;
            }
        } else{
            printf("Invalid command entered in main\n\n");
            continue;
        }
    }
    return 0;
}
void cmdMkdir(const char* dirName, uint32_t* curDirInode, inodeMeta* entries, int* numEntries){
    //similar logic as cmdTouch but with directory vs file and . and ..
    if (entryHuh(dirName, *curDirInode)){
        printf("entry with name already exits, nothing will happen\n");
        return;
    }

    uint32_t newDirIno;
    if (findNextIno(entries, *numEntries, &newDirIno) == 1){
        printf("no inodes are avaliable in cmdMkdir\n");
        return;
    }

    if(updateDir(*curDirInode, newDirIno, dirName) == 1){
        printf("error when updating directory in cmdMkdir\n");
        return;
    }

    char dirFileName[MAX_INODE_STRING];
    sprintf(dirFileName, "%u", newDirIno);
    FILE* file = fopen(dirFileName, "wb");
    if (file == NULL){
        printf("couldn't open inodes file in cmdMkdir\n");
        return;
    }

    //create default . and ..
    dirContent dEntries[2];
    memset(dEntries, 0, sizeof(dEntries));
    //first default of .
    dEntries[0].inodeIndex = newDirIno;
    strncpy(dEntries[0].name, ".", MAX_NAME);
    //second default of ..
    //make sure to set to parent
    dEntries[1].inodeIndex = *curDirInode;
    strncpy(dEntries[1].name, "..", MAX_NAME);

    if (fwrite(dEntries, sizeof(dEntries), 1, file) != 1){
        printf("error writing fwrite in cmdMkdir\n");
        fclose(file);
        return;
    }
    fclose(file);
    entries[*numEntries].inodeIndex = newDirIno;
    entries[*numEntries].type = 'd';
    (*numEntries)++;

    printf("Directory '%s' created with inode %u.\n", dirName, newDirIno);
}

void cmdTouch(const char* fileName, uint32_t* curDirInode, inodeMeta* entries, int *numEntries){
    if (entryHuh(fileName, *curDirInode)){
        printf("entry with name already exits, nothing will happen\n");
        return;
    }

    uint32_t newIno;
    if (findNextIno(entries, *numEntries, &newIno) == 1){
        printf("no inodes are avaliable in cmdTouch\n");
        return;
    }

    if(updateDir(*curDirInode, newIno, fileName) == 1){
        printf("error when updating direcotry in cmdTouch\n");
        return;
    }

    char inoFileName[MAX_INODE_STRING];
    sprintf(inoFileName, "%u", newIno);
    FILE* file = fopen(inoFileName, "wb");
    if (file == NULL){
        printf("couldn't open inodes file in cmdTouch\n");
        return;
    }
    
    //must ust fprintf to write to file
    fprintf(file, "%s", fileName);
    fclose(file);

    entries[*numEntries].inodeIndex = newIno;
    entries[*numEntries].type = 'f';
    (*numEntries)++;

    printf("File '%s' created with inode %u.\n", fileName, newIno);

}

void cmdCd(const char* dirName, uint32_t* curDirInode, const inodeMeta* entries, int numEntries){
    //check for . or .. default
    if (strcmp(dirName, ".") == 0){
        printf("No directory change since cd .\n");
        return;

    //check if cd ..
    } else if (strcmp(dirName, "..") == 0){
        char pcurFileName[MAX_INODE_STRING];
        sprintf(pcurFileName, "%u", *curDirInode);
        FILE *pdirFile = fopen(pcurFileName, "rb");
        if (pdirFile == NULL){
            printf("couldn't open current directory file in cmdCd %s\n", pcurFileName);
            return;
        }
        dirContent pdirEntry;
        while (fread(&pdirEntry, sizeof(pdirEntry), 1, pdirFile)){
            if (strcmp(pdirEntry.name, "..") == 0){
                *curDirInode = pdirEntry.inodeIndex;
                printf("changed to parent directory\n");
                fclose(pdirFile);
                return;
            }
        }
    }

    //other regular cd types 
    //max size is inode(1023) uint_32 to string
    char curFileName[MAX_INODE_STRING];
    //convert inode num to string
    sprintf(curFileName, "%u", *curDirInode);
    //read binary given the string of file name
    FILE *dirFile = fopen(curFileName, "rb");
    if (dirFile == NULL){
        printf("couldn't open current directory file in cmdCd %s\n", curFileName);
        return;
    }
    //found? checker
    int foundHuh = 0;
    dirContent dirEntry;
    while (fread(&dirEntry, sizeof(dirContent), 1, dirFile) == 1){
        //if the name of the directory matches
        if(strcmp(dirEntry.name, dirName) == 0){
            printf("Matching dirEntry and dirName\n");
            //check if the type is a directory given the name

            for(int i = 0; i <= numEntries; i++){
                //printf("%c\n",entries[i].type);
                if (entries[i].inodeIndex == dirEntry.inodeIndex){
                    if (entries[i].type == 'd'){
                        printf("Matching entry inodeIno and dirEntry inodeIno and entries type id d\n");
                        foundHuh = 1;
                        //change current directory to the matched one 
                        *curDirInode = dirEntry.inodeIndex;
                        break;
                    } else if (entries[i].type == 'f'){
                        printf("Provided name is not d type, inode: %u type%c\n", entries[i].inodeIndex, entries[i].type);
                    } else{
                        printf("Provided name is unknown type: %u %c\n", entries[i].inodeIndex, entries[i].type);
                    }
                }
            }
            break;
        }
    }
    fclose(dirFile);

    if (foundHuh == 0){
        printf("Invalid directory given in cmdCd: %s\n", dirName);
    } else {
        printf("Directory changed to: %s Inode Number: %u\n", dirName, *curDirInode);
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
        printf("couldn't open inodes file\n");
        exit(1);
    }

    for (int i = 0; i < numEntries; i++){
        if (fwrite(&entries[i].inodeIndex, sizeof(uint32_t), 1, inoFile) != 1){
            printf("couldn't write inode index to inodes file\n");
            fclose(inoFile);
            exit(1);
        }

        if (fwrite(&entries[i].type, sizeof(char), 1, inoFile) != 1){
            printf("couldn't write inode index to inodes file\n");
            fclose(inoFile);
            exit(1);
        }
    }

    fclose(inoFile);
    printf("Successfully written inodes_list and exiting now\n");
    exit(0);
}

int updateDir(uint32_t curDirInode, uint32_t newIno, const char* dirName){
    char dirFileName[MAX_INODE_STRING]; // size 5
    sprintf(dirFileName, "%u", curDirInode);

    FILE* dirFile = fopen(dirFileName, "ab");
    if (dirFile == NULL){
        printf("couldn't open file in updateDir\n");
        return 1;
    }
    dirContent newEntry;
    memset(&newEntry, 0, sizeof(newEntry));
    newEntry.inodeIndex = newIno;

    //make sure not longer than 32
    size_t lenCheck;
    if (strlen(dirName) < MAX_NAME){
        lenCheck = strlen(dirName);
        strncpy(newEntry.name, dirName, lenCheck);
        newEntry.name[MAX_NAME - 1] = '\0';
    } else {
        lenCheck = MAX_NAME;
        //printf("%zu", lenCheck);
        strncpy(newEntry.name, dirName, lenCheck);
    }

    if (fwrite(&newEntry, sizeof(newEntry), 1, dirFile) != 1){
        printf("error writing fwrite in updateDir\n");
        fclose(dirFile);
        return 1;
    }
    fclose(dirFile);
    return 0;
}

int findNextIno(const inodeMeta* entries, int numEntries, uint32_t* nextIno){
    char used[MAX_INODE] = {0};

    for (int i = 0; i < numEntries; i++){
        if (entries[i].inodeIndex < MAX_INODE){
            used[entries[i].inodeIndex] = 1;
        }
    }

    for (uint32_t j = 0; j < MAX_INODE; j++){
        if (used[j] == 0){
            *nextIno = j;
            //if there is avaliable node, good
            return 0;
        }
    }

    //if all the inodes were used
    return 1;
}

int entryHuh(const char* dirName, uint32_t dirIno){
    char dirFileName[MAX_INODE_STRING];
    sprintf(dirFileName, "%u", dirIno);

    FILE *file = fopen(dirFileName, "rb");
    if (file == NULL){
        printf("couldn't open file in entryHuh");
        return 1;
    }

    dirContent entry;
    //check if there exists a matching entry
    while (fread(&entry, sizeof(dirContent), 1, file) == 1) {
        entry.name[MAX_NAME - 1] = '\0';
        if(strcmp(entry.name, dirName) == 0){
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
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
