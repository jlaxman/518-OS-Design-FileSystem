// File: writeonceFS.c

// Authors: Vennela Chava (vc494), Sai Laxman Jagarlamudi (sj1018)
// iLab machine tested on: --ilab1.cs.rutgers.edu

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
//disk_size = 4MB = 4*1024*1024 bytes
#define disk_size 4194304
//each block size = 1KB = 1024 bytes
#define block 1024
//total number of block = disk_size/block
#define total_blocks 4096
#define filename_size 16
#define fd_size 15
#define file_limit 100
int get_next_block(int current_block, char file_idx);
int get_fd(char file_idx);
char get_file(char* name);
int get_block(char file_idx);
static int disk_isopen = 0;  
static int file_hdl;      
typedef enum {false, true} bool;
typedef enum {WO_CREAT = 1} mode;
typedef enum {WO_RDONLY=2, WO_WRONLY=3, WO_RDWR=4} flags;
typedef struct{
    int inode_index;
    int inode_size;
    int data_idx;
}superBlock;

typedef struct{
    bool isUsed;                   
    char file_name[filename_size]; 
    int file_size;                    
    int first_block;                    
    int block_count;              
    int num_fd;                
}inode;

typedef struct{
    bool isUsed;                
    char file_index;              
    int off;             
} fd_details;
superBlock*  superBlock1;
inode*   file_info;                      
fd_details filedes_table[fd_size];

int createdisk(char *disk_name){ 
    int file_descriptor, count=0;
    char buffer[block];
    if(!disk_name || ((file_descriptor = open(disk_name, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0)){
        fprintf(stderr, "createdisk: cannot open this disk\n");
        return -1;
    }
    memset(buffer, 0, block);
    while(count<total_blocks){
        write(file_descriptor, buffer, block);
        count=count+1;
    }
    close(file_descriptor);
    return 0;
}

int create_filesystem(char *disk_name){
    createdisk(disk_name);
    opendisk(disk_name);
    superBlock1 = (superBlock*)malloc(sizeof(superBlock));
    if(!superBlock1){
        return -1;
    }
    superBlock1->inode_index = 1;
    superBlock1->inode_size = 0;
    superBlock1->data_idx = 2;
    char buffer[block] = "";
    memset(buffer, 0, block);
    memcpy(buffer, &superBlock1, sizeof(superBlock));
    block_write(0, buffer);
    free(superBlock1);
    closedisk();
    printf("file system is successfully created\n");
    return 0;
}

int wo_mount(char* file_name, void* mem_address){
    create_filesystem(file_name);
    if(!file_name){
        return -1;
    }
    opendisk(file_name);
    if(!mem_address){
        mem_address = (char*)malloc(disk_size);
    }
    memset(mem_address, 0, block);
    read_block(0, mem_address);
    memcpy(&superBlock1, mem_address, sizeof(superBlock1));
    file_info = (inode*)malloc(block);
    memset(mem_address, 0, block);
    read_block(superBlock1->inode_index, mem_address);
    memcpy(file_info, mem_address, sizeof(inode) * superBlock1->inode_size);
    int i=0;
    while(i<fd_size){
        filedes_table[i].isUsed = false;
        i=i+1;
    }
    return 0;
}

int opendisk(char *disk_name){
    int file_descriptor;
    if(!disk_name || disk_isopen || ((file_descriptor = open(disk_name, O_RDWR, 0644)) < 0)){
        perror("opendisk: cannot open dsik");
        return -1;
    }  
    file_hdl = file_descriptor;
    disk_isopen = 1;
    return 0;
}

int read_block(int block1, char *buffer){
    if(!disk_isopen || block1<0 || (block1 >= total_blocks)){
        return -1;
    }
    if(lseek(file_hdl, block1 * block, SEEK_SET) < 0){
        perror("read_block: lseek failed");
        return -1;
    }
    if(read(file_hdl, buffer, block) < 0){
        perror("read_block: read of block failed");
        return -1;
    }
    return 0;
}

int wo_unmount(void* mem_address){
    int i=0;
    inode* file_ptr = (inode*)file_info;
    memset(mem_address, 0, block);
    char* block_ptr = (char*)mem_address;
    while(i < file_limit){
        if(file_info[i].isUsed == true){
            memcpy(block_ptr, &file_info[i], sizeof(file_info[i]));
            block_ptr += sizeof(inode);
        }
        i=i+1;
    }
    block_write(superBlock1->inode_index, mem_address);
    i=0;
    while(i<fd_size){
        if(filedes_table[i].isUsed == 1){
            filedes_table[i].isUsed = false;
            filedes_table[i].file_index = -1;
            filedes_table[i].off = 0;
        }
        i=i+1;
    }
    free(file_info);
    closedisk();
    return 0;
}

int block_write(int block1, char *buffer){
    if(!disk_isopen){
        fprintf(stderr, "block_write: disk is not open\n");
        return -1;
    }
    if((block1 < 0) || (block1 >= total_blocks)){
        fprintf(stderr, "block_write: index is out of bounds\n");
        return -1;
    }
    if(lseek(file_hdl, block1 * block, SEEK_SET) < 0){
        perror("block_write: lseek failed");
        return -1;
    }
    if(write(file_hdl, buffer, block) < 0){
        perror("block_write: write failed");
        return -1;
    }
    return 0;
}

int closedisk(){
    if(!disk_isopen){
        fprintf(stderr, "closedisk: there is no open disk\n");
        return -1;
    }
    close(file_hdl);
    disk_isopen =0;
    file_hdl = 0;
    return 0;
}

int wo_open(char* file_name, flags flg, mode m){
    if(m != 1){
        char file_idx = get_file(file_name);
        if(file_idx < 0){ 
            return -1;
        }
        else{
            if(flg==2 || flg==3 || flg==4){
                fprintf(stderr, "inside permission\n");
                int fd = get_fd(file_idx);
                if(fd < 0){
                    return -1;
                }
                file_info[file_idx].num_fd++;
                printf("successfully called wo_open() and  file [%s] opened.\n", file_name);
                return fd;
            }
            else{
                fprintf(stderr, "permission denied\n");
                return -1;
            }
        }
    }
    else{
        char file_idx = get_file(file_name);
        if(file_idx < 0){ 
            create_file(file_name);
            char file_idx = get_file(file_name);
            int fd = get_fd(file_idx);
            if(fd < 0){
                return -1;
            }
            file_info[file_idx].num_fd++;
            printf("fs_open()\t called successfully: file [%s] opened.\n", file_name);
            return fd;
        }
        return -1; 
    }
}

char get_file(char* name){
    char i=0;
    while(i < file_limit){
        if(file_info[i].isUsed == 1 && strcmp(file_info[i].file_name, name) == 0){
            return i; 
        }
        i=i+1;
    }
    return -1;        
}

int get_fd(char file_idx){
    int i=0;
    while(i < fd_size){
        if(filedes_table[i].isUsed == false){
            filedes_table[i].isUsed = true;
            filedes_table[i].file_index = file_idx;
            filedes_table[i].off = 0;
            return i;  
        }
        i=i+1;
    }
    fprintf(stderr, "get_fd() error: file descriptor is not available\n");
    return -1;        
}

int wo_read(int fd,  void* b, int bytes){
    if(bytes <= 0 || !filedes_table[fd].isUsed){
        return -1;
    }
    char *dst = b;
    char block1[block] = "";
    char file_idx = filedes_table[fd].file_index;
    inode* file = &file_info[file_idx];
    int block_index = file->first_block;
    int block_count = 0;
    int offset = filedes_table[fd].off;
    while(offset >= block){
        block_index = get_next_block(block_index, file_idx);
        block_count++;
        offset -= block;
    }
    read_block(block_index, block1);
    int read_count = 0, i=0, j=0;
    while(i<block){
        dst[read_count++] = block1[i];
        if(read_count == bytes){
            filedes_table[fd].off += read_count;
            return read_count;
        }
        i=i+1;
    }
    block_count++;
    strcpy(block1,"");
    while(read_count < bytes && block_count <= file->block_count){
        block_index = get_next_block(block_index, file_idx);
        strcpy(block1,"");
        read_block(block_index, block1);
        i=0;
        while(i < block){
            dst[read_count++] = block1[i];
            if(read_count == bytes){
                filedes_table[fd].off += read_count;
                return read_count;
            }
            i=i+1;
        }
        block_count++;
    }
    filedes_table[fd].off += read_count;
    return read_count;
}

int get_next_block(int current_block, char file_idx){
    char buffer[block] = "";
    int i;
    if(current_block < block){
        read_block(superBlock1->data_idx, buffer);
        i=current_block+1;
        while(i < block){
            if(buffer[i] == (file_idx + 1)){
                return i;
            }
            i=i+1;
        }
    }
    else{
        read_block(superBlock1->data_idx + 1, buffer);
        i=current_block-block+1;
        while(i < block){
            if(buffer[i] == (file_idx + 1)){
                return i + block;
            }
            i=i+1;
        }
    }
    return -1;
}

int wo_write(int fd,  void* b, int bytes){
    if(bytes <= 0 || !filedes_table[fd].isUsed){
        return -1;
    }
    int i = 0;
    char *src = b;
    char block1[block] = "";
    char file_idx = filedes_table[fd].file_index;
    inode* file = &file_info[file_idx];
    int block_index = file->first_block;
    int size = file->file_size;
    int block_count = 0;
    int offset = filedes_table[fd].off;
    while (offset >= block){
        block_index = get_next_block(block_index, file_idx);
        block_count++;
        offset -= block;
    }
    int write_count = 0;
    if(block_index != -1){
        read_block(block_index, block);
        i=offset;
        while(i < block){
            block1[i] = src[write_count++];
            if(write_count == bytes || write_count == strlen(src)){
                block_write(block_index, block1);
                filedes_table[fd].off += write_count;
                if(size < filedes_table[fd].off){
                    file->file_size = filedes_table[fd].off;
                }
                return write_count;
            }
            i=i+1;
        }
        block_write(block_index, block1);
        block_count++;
    }
    strcpy(block1, "");
    while(write_count < bytes && write_count < strlen(src) && block_count < file->block_count){
        block_index = get_next_block(block_index, file_idx);
        i=0;
        while(i < block){
            block1[i] = src[write_count++];
            if(write_count == bytes || write_count == strlen(src)){
                block_write(block_index, block1);
                filedes_table[fd].off += write_count;
                if(size < filedes_table[fd].off){
                    file->file_size = filedes_table[fd].off;
                }
                return write_count;
            }
            i=i+1;
        }
        block_write(block_index, block1);
        block_count++;
    }
    strcpy(block1, "");
    while(write_count < bytes && write_count < strlen(src)){
        block_index = get_block(file_idx);
        file->block_count++;
        if(file->first_block == -1){
            file->first_block = block_index;
        }
        if(block_index < 0){
            return -1;
        }
        i=0;
        while(i < block){
            block1[i] = src[write_count++];
            if(write_count == bytes || write_count == strlen(src)){
                block_write(block_index, block1);
                filedes_table[fd].off += write_count;
                if(size < filedes_table[fd].off){
                    file->file_size = filedes_table[fd].off;
                }
                return write_count;
            }
            i=i+1;
        }
        block_write(block_index, block1);
    }
    filedes_table[fd].off += write_count;
    if(size < filedes_table[fd].off){
        file->file_size = filedes_table[fd].off;
    }
    return write_count;
}

int get_block(char file_idx){
    char buffer1[block] = "";
    char buffer2[block] = "";
    read_block(superBlock1->data_idx, buffer1);
    read_block(superBlock1->data_idx + 1, buffer2);
    int i=4;
    while(i < block){
        if(buffer1[i] == '\0'){
            buffer1[i] = (char)(file_idx + 1);
            block_write(superBlock1->data_idx, buffer1);
            return i;  
        }
        i=i+1;
    }
    i=0;
    while(i < block){
        if(buffer2[i] == '\0'){
            buffer2[i] = (char)(file_idx + 1);
            block_write(superBlock1->data_idx, buffer2);
            return i;  
        }
        i=i+1;
    }
    fprintf(stderr, "get_block() error: blocks are not available\n");
    return -1;        
}

int wo_close(int fd){
    if(fd<0 || fd>=fd_size || !filedes_table[fd].isUsed){
        return -1;
    }
    fd_details* fds=&filedes_table[fd];
    file_info[fds->file_index].num_fd--;
    fds->isUsed=false;
    printf("successfully called wo_close() and file [%s] is closed\n", file_info[fds->file_index].file_name);
    return 0;
}

int create_file(char *name){
    char file_idx = get_file(name);
    if(file_idx < 0){  
        char i=0;
        while(i < file_limit){
            if(file_info[i].isUsed == false){
                superBlock1->inode_size++;
                file_info[i].isUsed = true;
                strcpy(file_info[i].file_name, name);
                file_info[i].file_size = 0;
                file_info[i].first_block = -1;
                file_info[i].block_count = 0;
                file_info[i].num_fd = 0;
                printf("create_file()\t called successfully: file [%s] created.\n", name);
                return 0;
            }
            i=i+1;
        }
        fprintf(stderr, "create_file() error: not able to create more files.\n");
        return -1;
    }
    else{              
        fprintf(stderr, "create_file() error: file [%s] already exists\n",name);
        return 0;
    }
}

int main(){
    char *disk_name = "file_system.txt";
    if(wo_mount(disk_name, NULL) < 0){
        fprintf(stderr, "mount_fs()\t error.\n");
    }
    mode CREATE=WO_CREAT;
    flags PERMISSION = WO_RDWR;
    char *test_file = "test.txt";
    int fdcheck= wo_open(test_file,PERMISSION,1);
    printf("This is fdcheck %d\n", fdcheck);
    wo_close(fdcheck);
    return 0;
}

