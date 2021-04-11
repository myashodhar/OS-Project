#include "LibFS.h"
#include "LibDisk.h"
// global errno value here
#include <bits/stdc++.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
using namespace std;

int read_block(int block,char* buf);


int block_write(int block, char *buf, int size_to_wrt, int start_pos_to_wrt);

//data structure

struct inode
{
    int inode_num;
    int type;
    int filesize;
    int pointer[30]; //contains DB nos of files (-1 if no files assigned)
};
struct inode inode_arr[NO_OF_INODES];


struct dir_entry
{
    char file_name[16]; //filename size is 15 max
    int inode_num;  // of this
};
struct dir_entry dir_struct_entry[NO_OF_INODES];

struct super_block  // map of directory  structure
{

    /* directory information */
    int firstDbOfDir = ceil( ((float)sizeof(super_block))/BLOCK_SIZE );
    int noOfDbUsedinDir = (sizeof(struct dir_entry)*NO_OF_INODES)/BLOCK_SIZE;
    /* data information */
    int startingInodesDb = firstDbOfDir+noOfDbUsedinDir;
    int total_Usable_DB_Used_for_inodes= ceil( ((float)(NO_OF_INODES * sizeof(struct inode))) / BLOCK_SIZE );
    int starting_DB_Index= startingInodesDb + total_Usable_DB_Used_for_inodes;
    int total_Usable_DB= DISK_BLOCKS - starting_DB_Index;
    char inode_Bitmap[NO_OF_INODES];
    char datablock_Bitmap[DISK_BLOCKS];
};

struct super_block sup_block;


char disk_name[100];
char filename[16];

map <string,int> directory_map;

vector<int> free_Db_list;

vector<int> free_Inode_list;

map <string,int> :: iterator it;

vector<int> free_FD_list;   //Free file descriptors.
int openfile_count=0 ; //number of files opened.
map <int,pair<int,int>> fileDescriptor_map; //fd as key and [Inode number, file pointer] as value

//main functionality starts
int osErrno;

FILE *fp;


//FS_Sync function
int FS_Sync()
{
    printf("FS_Sync\n");

    fp = fopen(disk_name,"rb+");
    //retrieve super block from virtual disk and store into global struct super_block sup_block
    char sup_block_buff[sizeof(sup_block)];
    memset(sup_block_buff,0,sizeof(sup_block));
    fread(sup_block_buff,1,sizeof(sup_block),fp);
    memcpy(&sup_block,sup_block_buff,sizeof(sup_block));


    //retrieve DS block from virtual disk and store into global struct dir_entry dir_struct_entry[NO_OF_INODES]
    fseek( fp, (sup_block.firstDbOfDir)*BLOCK_SIZE, SEEK_SET );
    char dir_buff[sizeof(dir_struct_entry)];
    memset(dir_buff,0,sizeof(dir_struct_entry));
    fread(dir_buff,1,sizeof(dir_struct_entry),fp);
    memcpy(dir_struct_entry,dir_buff,sizeof(dir_struct_entry));



    //retrieve Inode block from virtual disk and store into global struct inode inode_arr[NO_OF_INODES];
    fseek( fp, (sup_block.startingInodesDb)*BLOCK_SIZE, SEEK_SET );
    char inode_buff[sizeof(inode_arr)];
    memset(inode_buff,0,sizeof(inode_arr));
    fread(inode_buff,1,sizeof(inode_arr),fp);
    memcpy(inode_arr,inode_buff,sizeof(inode_arr));


    //storing all filenames into map
    for (int i = NO_OF_INODES - 1; i >= 0; --i)
        if(sup_block.inode_Bitmap[i]==1)
            directory_map[string(dir_struct_entry[i].file_name)]=dir_struct_entry[i].inode_num;
        else
            free_Inode_list.push_back(i);

    // populate free_Inode_list and free_Db_list
    for(int i = DISK_BLOCKS - 1; i>=sup_block.starting_DB_Index; --i)
        if(sup_block.datablock_Bitmap[i] == 0)
            free_Db_list.push_back(i);

    // Populate Free Filedescriptor vector
    for(int i=NO_OF_FILEDESCRIPTORS-1;i>=0;i--){
        free_FD_list.push_back(i);
    }

    printf("Disk is mounted now\n");


    return 1;
}

/**************************************************************************
 disk_set--  it will push default value in meta data and disk structure *                                             *
***************************************************************************/


void disk_Set()
{
    char buffer[BLOCK_SIZE];

    fp = fopen(disk_name,"wb");
    memset(buffer, 0, BLOCK_SIZE); //setting the buffer's value=NULL at all index

    for (int i = 0; i < DISK_BLOCKS; ++i)
        fwrite(buffer,1,BLOCK_SIZE,fp);
        //write(fp, buffer, BLOCK_SIZE);


    struct super_block sup_block;  //initializing sup_block

    //setting DB vecor array to 1(=used i.e cant assign to file) for all metadata DB and 0(=free) to all data DBs
    for (int i = 0; i < sup_block.starting_DB_Index; ++i)
        sup_block.datablock_Bitmap[i] = 1; //1 means DB is not free
    for (int i = sup_block.starting_DB_Index; i < DISK_BLOCKS; ++i)
        sup_block.datablock_Bitmap[i] = 0; //1 means DB is not free
    for(int i = 0; i < NO_OF_INODES; ++i)
        sup_block.inode_Bitmap[i]=0; // char 0;

    for (int i = 0; i < NO_OF_INODES; ++i)
    {   // we have 30 data blocks pointer
        for(int j = 0; j < 30; j++ )
        {
            inode_arr[i].pointer[j]=-1;
        }
    }


    //storing sup_block into begining of virtual disk
    fseek( fp, 0, SEEK_SET );
    char sup_block_buff[sizeof(struct super_block)];
    memset(sup_block_buff, 0, sizeof(struct super_block));
    memcpy(sup_block_buff,&sup_block,sizeof(sup_block));
    fwrite(sup_block_buff,1,sizeof(sup_block),fp);



    //storing DS after sup_block into virtual disk
    fseek( fp, (sup_block.firstDbOfDir)*BLOCK_SIZE, SEEK_SET );
    char dir_buff[sizeof(dir_struct_entry)];
    memset(dir_buff, 0, sizeof(dir_struct_entry));
    memcpy(dir_buff,dir_struct_entry,sizeof(dir_struct_entry));
    fwrite(dir_buff,1,sizeof(dir_struct_entry),fp);



    //storing inode DBs after sup_block & DB into virtual disk
    fseek( fp, (sup_block.startingInodesDb)*BLOCK_SIZE, SEEK_SET );
    char inode_buff[sizeof(inode_arr)];
    memset(inode_buff, 0, sizeof(inode_arr));
    memcpy(inode_buff,inode_arr,sizeof(inode_arr));
    fwrite(inode_buff,1,sizeof(inode_arr),fp);



    fclose(fp);
    printf("Virtual Disk %s created sucessfully\n",disk_name );

}


/*******************************************************************
   FS_BOOT() - it will boot the disk                               *
*******************************************************************/


int
FS_Boot(char *path)
{
    //disk_name = path;
    strcpy(disk_name, path);
    cout<<"disk name:" << disk_name<<endl;

    printf("FS_Boot %s\n", disk_name);

    if( access( disk_name, F_OK ) != -1 )
    {
        printf("The Disk already exists\n");
    }
    else
    {
        // oops, check for errors
        if (Disk_Init() == -1) {
    	printf("Disk_Init() failed\n");
    	osErrno = E_GENERAL;
    	return -1;
        }

	   //modified

	   Disk_Save(disk_name);
       disk_Set();
    }

   // FS_Sync();
    return 0;
}

/*********************************************************************
 File_Create function - it will create the blank file into disk      *
**********************************************************************/


int File_Create(char *filename)
{
    printf("FS_Create\n");

    int nextin, nextdb;
    //file is already present or not
    if(directory_map.find(filename)!= directory_map.end())
    {
        printf("Error : file can't be created,Max files created.\n");
        return -1;
    }
    if(free_Inode_list.size()==0) //sup_block.next_freeinode == -1
    {
        printf("Create File Error : No more files can be created.Max number of files reached.\n");
        return -1;
    }

    if(free_Db_list.size() == 0)
    {
        printf("Error : Memory is fulled.\n");
        return -1;
    }

    nextin = free_Inode_list.back();
    free_Inode_list.pop_back();
    nextdb = free_Db_list.back();
    free_Db_list.pop_back();
    inode_arr[nextin].pointer[0]=nextdb;


    inode_arr[nextin].filesize = 0;

    directory_map[string(filename)] = nextin;

    strcpy(dir_struct_entry[nextin].file_name,filename);
    dir_struct_entry[nextin].inode_num = nextin;

    printf(" Voila!! File %s Created.\n",filename );

    return 0;
}


/*********************************************************************
 File_Open - it will open the file                                   *
**********************************************************************/


int File_Open(char *name)
{
    printf("FS_Open\n");

    int inode_no;
    if ( directory_map.find(name) == directory_map.end() )
    {
        printf("Open File Error : File Not Found\n");
        return -1;
    }
    else
    {
        if(free_FD_list.size() == 0)
        {
            printf("Error :File descriptors is Not Available\n");
            return -1;
        }
        else
        {
            int inode = directory_map[name];
            int fd = free_FD_list.back();
            free_FD_list.pop_back();
            fileDescriptor_map[fd].first=inode;
            fileDescriptor_map[fd].second = 0;
            openfile_count++;
            printf("File descriptor %d\n",fd);

            return fd;

        }
    }
    return 0;
}


/*********************************************************************
 File_Read - it will read the file from the disk                     *
**********************************************************************/


int
File_Read(int fildes, void  *buf, int nbyte)
{
    printf("FS_Read\n");


    char dest_filename[16];
    int flag=0;
    if(fileDescriptor_map.find(fildes) == fileDescriptor_map.end()){
        printf("File Read Error: File not opened \n");
        return -1;
    }
    else
    {
        int inode =fileDescriptor_map[fildes].first;
        int noOfBlocks=ceil(((float)inode_arr[inode].filesize)/BLOCK_SIZE);
        int tot_block=noOfBlocks;
        strcpy(dest_filename, dir_struct_entry[inode].file_name);

        FILE* fp1 = fopen(dest_filename,"wb+");
        char read_buf[BLOCK_SIZE];

        for( int i = 0; i < 30; i++ )
        {       //Reading data present at pointers
            if(noOfBlocks == 0){
                break;
            }
            int block=inode_arr[inode].pointer[i];

            read_block(block,read_buf);


            if((tot_block-noOfBlocks >= FS/BLOCK_SIZE) && (noOfBlocks > 1))
            {
                if(flag==0)
                {
                    fwrite(read_buf+(FS%BLOCK_SIZE),1,(BLOCK_SIZE-FS%BLOCK_SIZE),fp1);
                    flag=1;
                }
                else
                    fwrite(read_buf,1,BLOCK_SIZE,fp1);

            }


            noOfBlocks--;
        }


        if(tot_block-FS/BLOCK_SIZE > 1)
            fwrite(read_buf,1,(inode_arr[inode].filesize)%BLOCK_SIZE,fp1);
        else if(tot_block-FS/BLOCK_SIZE == 1)
            fwrite(read_buf+(FS%BLOCK_SIZE),1,(inode_arr[inode].filesize)%BLOCK_SIZE - FS%BLOCK_SIZE,fp1);


        fclose(fp1);

    }
    return 0;
}


/*********************************************************************
 File_write - it will write the file into disk                       *
**********************************************************************/

int
File_Write(int fd, char *buf, int size)
{


    //take user filename input in global var filename[20] (already declared) and size of file in a local var write_file_size (why local coz it'll get updated in inode at the end of function)
    //check if avail DB size > write_file_size else throw error & return

    int cur_pos=fileDescriptor_map[fd].second, db_to_write;

    if( cur_pos%BLOCK_SIZE == 0 && size > BLOCK_SIZE )
    {
        printf("more than 1 DB size needed\n");
        return -1;
    }
    else if( (BLOCK_SIZE - cur_pos%BLOCK_SIZE) < size )
    {
        printf("size of data to write is more than remaining empty size of DB\n");
    }

    int total_Usable_DB_used_till_cur_pos = cur_pos / BLOCK_SIZE ;


        if(inode_arr[fileDescriptor_map[fd].first].filesize == 0)
        {
            block_write(inode_arr[fileDescriptor_map[fd].first].pointer[0], buf, size, 0 );
            inode_arr[fileDescriptor_map[fd].first].filesize += size;
            fileDescriptor_map[fd].second += size;
        }
        else //if filesize is not 0
        {
            if(cur_pos<inode_arr[fileDescriptor_map[fd].first].filesize)
            {
                if(total_Usable_DB_used_till_cur_pos < 10 )
                {
                    db_to_write  = inode_arr[fileDescriptor_map[fd].first].pointer[total_Usable_DB_used_till_cur_pos];

                    block_write(db_to_write, buf, size, 0 );

                    if( inode_arr[fileDescriptor_map[fd].first].filesize - cur_pos < size )
                        inode_arr[fileDescriptor_map[fd].first].filesize += size - (inode_arr[fileDescriptor_map[fd].first].filesize - cur_pos);
                    fileDescriptor_map[fd].second += size;
                }
                else if(total_Usable_DB_used_till_cur_pos < 1034 )
                {
                    int block = inode_arr[fileDescriptor_map[fd].first].pointer[10];
                    int blockPointers[1024];        //Contains the array of data block pointers.
                    char read_buf[(1<<15)];
                    read_block(block,read_buf);  //reading the block into read_buf
                    memcpy(blockPointers,read_buf,sizeof(read_buf));



                    db_to_write  = blockPointers[total_Usable_DB_used_till_cur_pos-10];

                    block_write(db_to_write, buf, size, 0);

                    if( inode_arr[fileDescriptor_map[fd].first].filesize - cur_pos < size )  //updating the filesize
                        inode_arr[fileDescriptor_map[fd].first].filesize += size - (inode_arr[fileDescriptor_map[fd].first].filesize - cur_pos);
                    fileDescriptor_map[fd].second += size;  //updating the cur pos in the file
                }
                else
                {

                    int block = inode_arr[fileDescriptor_map[fd].first].pointer[10];
                    int blockPointers[1024];        //Contains the array of data block pointers.
                    char read_buf[(1<<15)];
                    read_block(block,read_buf);  //reading the block into read_buf
                    memcpy(blockPointers,read_buf,sizeof(read_buf));

                    int block2 = blockPointers[(total_Usable_DB_used_till_cur_pos-1034)/1024];
                    int blockPointers2[1024];       //Contains the array of data block pointers.
                    read_block(block2,read_buf);  //reading the block2 into read_buf
                    memcpy(blockPointers2,read_buf,sizeof(read_buf));



                    db_to_write  = blockPointers2[(total_Usable_DB_used_till_cur_pos-1034)%1024];
                    block_write(db_to_write, buf, size, 0 );

                    if( inode_arr[fileDescriptor_map[fd].first].filesize - cur_pos < size )  //updating the filesize
                        inode_arr[fileDescriptor_map[fd].first].filesize += size - (inode_arr[fileDescriptor_map[fd].first].filesize - cur_pos);
                    fileDescriptor_map[fd].second += size;  //updating the cur pos in the file

                }
            }
            else //if cur pos == filesze and its end of block i.e muliple of (1<<15)
            {
                if(total_Usable_DB_used_till_cur_pos < 30 )
                {
                    db_to_write  =  free_Db_list.back();
                    free_Db_list.pop_back();
                    inode_arr[fileDescriptor_map[fd].first].pointer[total_Usable_DB_used_till_cur_pos];
                    block_write(db_to_write, buf, size, 0 );

                    inode_arr[fileDescriptor_map[fd].first].filesize += size;
                    fileDescriptor_map[fd].second += size;  //updating the cur pos in the file
                }



            }
        }

}

int
File_Seek(int fd, int offset)
{
    printf("File_Seek\n");
    if(offset < 0){
        printf("File Seek: Negative Offset.\n");
        osErrno = E_SEEK_OUT_OF_BOUNDS;
        return -1;
    }
    else if(get_file_size(fd) == -1){
        printf("File Seek: File Not Open.\n");
        osErrno = E_BAD_FD;
        return -1;
    }
    else if(get_file_size(fd) < offset){
        printf("File Seek: Seek Out of Bound.\n");
        osErrno = E_SEEK_OUT_OF_BOUNDS;
        return -1;
    return 0;
    }
}
int
File_Close(char* name)
{   int i, table_sector, offset;
    char buff[SECTOR_SIZE];

    if(fd < 64){
        table_sector = 4;
        offset = (fd*8);
    }
    else if(fd > 63 && fd < 128){
        table_sector = 5;
        offset = (fd*8) - 64;
    }
    else if(fd > 127 && fd < 192){
        table_sector = 6;
        offset = (fd*8) - 128;
    }
    else if(fd > 191 && fd < 256){
        table_sector = 7;
        offset = (fd*8) - 192;
    }

    Disk_Read(table_sector, buff);
    if(buff[offset] == 0){
        printf("File Close: File Not Open\n");
        osErrno = E_BAD_FD;
        return -1;
    }

    for(i = offset; i < 8; i++){
        buff[i] = '\0';
    }
    Disk_Write(table_sector, buff);
    FS_Sync();
    return 0;
}

int
File_Unlink(char *file)
{
    printf("FS_Unlink\n");
    return 0;
}




// directory ops
int
Dir_Create(char *path)
{
    printf("Dir_Create %s\n", path);
    return 0;
}

int
Dir_Size(char *path)
{
    printf("Dir_Size\n");
    return 0;
}

int
Dir_Read(char *path, void *buffer, int size)
{
    printf("Dir_Read\n");
    return 0;
}

int
Dir_Unlink(char *path)
{
    printf("Dir_Unlink\n");
    return 0;
}


int read_block(int block,char* buf)
{
    if ((block < 0) || (block >= DISK_BLOCKS)) {
    printf("Block read Error : block index out of bounds\n");
    return -1;
  }

  if (fseek(fp, block * BLOCK_SIZE, SEEK_SET) < 0) {
    printf("Block read Error : failed to lseek");
    return -1;
  }
  if (fread(buf, 1, BLOCK_SIZE,fp) < 0) {
    printf("Block read Error : failed to read");
    return -1;
  }
  return 0;
}

int block_write(int block, char *buf, int size_to_wrt, int start_pos_to_wrt)
{
    cout<<"block_number:"<<block<<endl;
  if ((block < 0) || (block >= DISK_BLOCKS))
  {
    printf("Block write Error : block index out of bounds\n");
    return -1;
  }

  if (fseek(fp, (block * BLOCK_SIZE)+start_pos_to_wrt, SEEK_SET) < 0)
  {
    printf("Block write Error : failed to lseek");
    return -1;
  }

  if (fwrite(buf, 1, size_to_wrt,fp) < 0)
  {
    perror("block_write: failed to write");
    return -1;
  }

  return 0;
}


int store_file_into_Disk(char * filename)
{
    int fp2_filesize, fd2, size_buf2, total_Usable_DB_in_src_file, remain_size_OfLast_db;
    char buf2[(1<<15)-1];
    FILE *fp2;
    fp2=fopen(filename,"rb");

    if(fp2 == NULL)
    {
        printf("Source File doen not exists\n");
        return -1;
    }

    fseek(fp2, 0L, SEEK_END); // file pointer at end of file
    fp2_filesize = ftell(fp2);  //getting the position
    //total_Usable_DB_in_src_file = fp2_filesize / BLOCK_SIZE;
    fseek( fp2, 0, SEEK_SET );


    if( File_Create(filename) < 0 )
    {
        printf("Error in File_Create()\n");
        return -1;
    }
    if( (fd2=File_Open(filename)) < 0)
    {
        printf("Error in open_file()\n");
        return -1;
    }



    remain_size_OfLast_db=BLOCK_SIZE;


    if( remain_size_OfLast_db >= fp2_filesize )
    {
        fread(buf2, fp2_filesize, 1, fp2);
        buf2[fp2_filesize] = '\0';
        File_Write(fd2, buf2, fp2_filesize);
    }
    else
    {

        fread(buf2, remain_size_OfLast_db, 1, fp2);
        buf2[remain_size_OfLast_db] = '\0';
        File_Write(fd2, buf2, remain_size_OfLast_db);

        int remaining_block_count = (fp2_filesize - remain_size_OfLast_db) / BLOCK_SIZE;
        while(remaining_block_count--)
        {
            fread(buf2, BLOCK_SIZE, 1, fp2);
            buf2[BLOCK_SIZE] = '\0';
            File_Write(fd2, buf2, BLOCK_SIZE);

        }

        int remaining_size = (fp2_filesize - remain_size_OfLast_db) % BLOCK_SIZE;

        fread(buf2, remaining_size, 1, fp2);
        buf2[remaining_size] = '\0';
        File_Write(fd2, buf2, remaining_size);

    }

   // close_file(fd2);
    if ( fileDescriptor_map.find(fd2) == fileDescriptor_map.end() )
    {
        printf("File Descripter %d not found\n",fd2);
    }
    else
    {
        fileDescriptor_map.erase(fd2);
        openfile_count--;
        free_FD_list.push_back(fd2);
        printf("FD %d closed successfully\n",fd2);
    }

    return 0;

}


int unmounting()
{

    //storing sup_block into begining of virtual disk
    fseek( fp, 0, SEEK_SET );
    char sup_block_buff[sizeof(struct super_block)];
    memset(sup_block_buff, 0, sizeof(struct super_block));
    memcpy(sup_block_buff,&sup_block,sizeof(sup_block));
    fwrite(sup_block_buff,1,sizeof(sup_block),fp);



    //storing Direc Struct after sup_block into virtual disk
    fseek( fp, (sup_block.firstDbOfDir)*BLOCK_SIZE, SEEK_SET );
    char dir_buff[sizeof(dir_struct_entry)];
    memset(dir_buff, 0, sizeof(dir_struct_entry));
    memcpy(dir_buff,dir_struct_entry,sizeof(dir_struct_entry));
    fwrite(dir_buff,1,sizeof(dir_struct_entry),fp);



    //storing inode DBs after sup_block & DB into virtual disk
    fseek( fp, (sup_block.startingInodesDb)*BLOCK_SIZE, SEEK_SET );
    char inode_buff[sizeof(inode_arr)];
    memset(inode_buff, 0, sizeof(inode_arr));
    memcpy(inode_buff,inode_arr,sizeof(inode_arr));
    fwrite(inode_buff,1,sizeof(inode_arr),fp);



    printf("Disk Unmounted\n");
    fclose(fp);
    return 1;
}
