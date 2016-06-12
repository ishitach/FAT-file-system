#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
//This file is used for checking the function made in the other files

#define Size_of_buf 80
char tempBuf[Size_of_buf];


size_t byte_no;
off_t Offset;
off_t length;
int fdone, fdtwo, fdthree, fdfour, fdfive; //The file descriptors
int i;
int main() {

  FileSys_make("mydisk"); //making the disk
  FileSys_mount("mydisk"); //mounting on the disk
  FileSys_create("fa1"); 	//creating a file 
  fdone = FileSys_Open("fa1");  //opening a file
  char bufa[] = "This is our first project";
  byte_no = 24;
  FileSys_write(fdone,bufa,byte_no); //writing on a file


//creating another file, opening and writing on it
  FileSys_create("fb");
  fdtwo = FileSys_Open("fb");
  char bufb[] = "today is sunday";
  byte_no = 15;
  FileSys_write(fdtwo,bufb,byte_no);


//Doing the same on another files also
  FileSys_create("fc");
  fdthree = FileSys_Open("fc");
  char bufc[] = "Today was the first day we implemented our virtual file system on operating system";
  byte_no = 66;
  FileSys_write(fdthree,bufc,byte_no);

  FileSys_create("fd");
  fdfour = FileSys_Open("fd");
  char bufd[] = "Today is monday";
  byte_no = 15;
  FileSys_write(fdfour,bufd,byte_no);

  FileSys_create("fe");

  length = FileSys_GetFileSize(fdone);
  printf("The file size of the file fa1 = %d\n", (int) length); 

  byte_no = 20;

  Offset = -24;
  FileSys_lseek(fdone, Offset);
  FileSys_read(fdone, tempBuf, byte_no); 

//for (i = 0; i < byte_no; i++)
//	  printf("%c", tempBuf[i]); 
//  printf("\nhi");

  Offset = -58;
  FileSys_lseek(fdthree, Offset);    //performing various functions by calling them from filesys.c
  byte_no = 10;
  FileSys_read(fdthree, tempBuf, byte_no);
  for (i = 0; i < byte_no; i++)
	  printf("%c", tempBuf[i]); 
  printf("\n");

  length = 30;
  FileSys_Truncate(fdthree, length);
  byte_no = 30;
  FileSys_read(fdthree, tempBuf, byte_no); 
  for (i = 0; i < byte_no; i++)
	  printf("%c", tempBuf[i]); 
  printf("\n");
  
  FileSys_close(fdtwo);

//  FileSys_read(fdtwo, tempBuf, byte_no);


  fdfive = FileSys_Open("fe");
  char bufe[] = "This is the last project of operating system";
  byte_no = 51;
  FileSys_write(fdfive,bufe,byte_no);

  FileSys_delete("fc"); //deleting a file
  
  FileSys_dismount("mydisk");
  FileSys_mount("mydisk");

  fdone = FileSys_Open("fa1");
  fdtwo = FileSys_Open("fb");
  printf("The status of the file fb is = %d\n",fdtwo); 
  fdthree = FileSys_Open("fc");
  fdfour = FileSys_Open("fd");
  fdfive = FileSys_Open("fe");

byte_no = 9;
  FileSys_read(fdfour, tempBuf, byte_no);
  for (i = 0; i < byte_no; i++)
	  printf("%c", tempBuf[i]); 
  printf("\n");


  byte_no = 24;
  FileSys_read(fdone, tempBuf, byte_no);
  for (i = 0; i < byte_no; i++)
	  printf("%c", tempBuf[i]); 
  printf("\n");

  byte_no = 51;
  FileSys_read(fdfive, tempBuf, byte_no);
  for (i = 0; i < byte_no; i++)
	  printf("%c", tempBuf[i]); 
  printf("\n");


  byte_no = 30;
  FileSys_read(fdthree, tempBuf, byte_no);
  for (i = 0; i < byte_no; i++)
	  printf("%c", tempBuf[i]); 
  printf("\n");

 

  FileSys_dismount("mydisk");
  return 0;
}
