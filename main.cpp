#include <stdio.h>

#include "LibFS.h"

#define block_size 512
void 
usage(char *prog)
{
    fprintf(stderr, "usage: %s <disk image file>\n", prog);
    exit(1);
}

int
main(int argc, char *argv[])
{
    if (argc != 2) {
	usage(argv[0]);
    }
    char *path = argv[1];
    char *buffer;
    int status,count,size,choice,fd,offset;
    char filename [16];
    FS_Boot(path);
    FS_Sync();
    printf("Disk Initialized\n");
    while(1)
    {	
		printf("-------------------------------------------------------------\n");
	    printf("Please enter your choice\n");
	    printf("0.Exit\n");
	    printf("1.File Create\t2.File Open\t3.File Read\t4.File Write\n");
	    printf("5.File Seek\t6.File Close\t7.File Unlink\n");
	    printf("8.Dir Create\t9.Dir Read\t10.Dir Unlink\n");
	    printf("-------------------------------------------------------------\n\n");
	    scanf("%d",&choice);
	    
	    //Menu selection cases
	    switch(choice)
	    {
		    case 0: //Exit the program
		    	printf("closing the system\n");
		    	unmounting();
			    exit(0);

		    case 1: //File Create
		    	printf("Enter the file name to create\n");
		    	scanf("%s",filename);
			    File_Create(filename);
			    break;

		    case 2: //File Open
		    	printf("Enter the file name to open\n");
		    	scanf("%s",filename);
		    	File_Open(filename);
		    	break;

		    case 3: //File Read
		    	printf("Enter the file name to read\n");
		    	scanf("%s",filename);
		    	File_Read(fd, buffer,size);
		    	break;

		    case 4: //File Write
		    	printf("Enter the file name to write\n");
		    	scanf("%s",filename);
		    	store_file_into_Disk(filename);

		    	//count=File_Write(filename, buffer,size);
		    	break;

		    case 5: //File Seek
		    	File_Seek(fd,offset);
		    	break;

		    case 6: //File Close
		    	printf("Enter the file name to close\n");
		    	scanf("%s",filename);
		    	//File_Close(filename);
		    	break;

		    case 7: //File Unlink
		    	printf("Enter the file name to unlink\n");
		    	scanf("%s",filename);
		    	status=File_Unlink(filename);
		    	break;

		    //Directory Operations
		    case 8: //Dir Create
		    	printf("Enter the dir path to create\n");
		    	scanf("%s",path);
		    	status=Dir_Create(path);
		    	break;

		    case 9: //Dir Read
		    	printf("Enter the dir path to read\n");
		    	scanf("%s",path);
		    	count=Dir_Read(path,buffer,size);
		    	break;

		    case 10: //Dir Unlink
		    	printf("Enter the dir path to unlink\n");
		    	scanf("%s",path);
		    	status = Dir_Unlink(path);
		    	break;

		    default:
		    	printf("Invalid Choice!!! Try Again\n");
	    }
    }

    return 0;
}
