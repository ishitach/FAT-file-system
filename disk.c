#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define DISK_BLOCKS 5000        		// These are the number of blocks on the disk              
#define BLOCK_SIZE 4000    			//This is the block size 

static int vir_open = 0;  			//to check if virtual disk is in use 
static int f_handle;      			//file f_handle for virtual disk                  

int create_disk(char *disk_name) 		// This is a function to create a disk. It takes the disk name as an input argument. 
{ 
  char buffer[BLOCK_SIZE];
  int  i,f;
  
  if (!disk_name) {				//It checks if the input argument for the name of the disk is valid or not .
    fprintf(stderr, "Creating disk with invalid name\n");
    return -1;
  }
  if ((f = open(disk_name,O_WRONLY | O_CREAT | O_TRUNC,0644))<0) // It creates a file and opens it 
  								// The permissions for this file are write only and truncate .
  {
    perror("File cannot be opened");
    return -1;
  }
  memset(buffer, 0, BLOCK_SIZE); 			// It sets all the bytes of a block to 0. 
  for (i= 0; i< DISK_BLOCKS; ++i)			// It writes the disk blocks with each block of the block size mentioned above 
    write(f, buffer, BLOCK_SIZE);			// and all the bytes of the all the blocks are set to 0.

  close(f);						// It closes the file.
  return 0;
}

int Disk_open(char *disk_name)
{
  int f;

  if (!disk_name) {						// It checks if the disk name is valid or not.
    fprintf(stderr, "Disk can't be opened due to invalid disk name\n");
    return -1;
  }  
  
  if (vir_open) {  						// It checks if the disk is already open or not . It checks the value of the
    fprintf(stderr, "The disk is already open\n");		// variable vir_open. If the value is 1 then the disk is already open or 
    return -1;
  }								//else not.
  
  if ((f = open(disk_name, O_RDWR, 0644)) < 0) { 		// It opens the disk in Read- Write mode. If the value returned is less than
    perror("Cannot open this file");					// 0 then the file cannot be opened. 
    return -1;
  }

  f_handle = f;  						// Sets the value of the file handler .
 vir_open = 1;              					// Sets the value of vir_open to 1 which indicates that the disk is open.

  return f_handle;
}

int Disk_close()
{
  if (!vir_open) {						// If the value of vir_open is 0 which means there is no disk open ,  
    fprintf(stderr, "There is no such disk open \n");		// It gives an error that there is no disk open. 
    return -1;
  }
  
  close(f_handle);                                        // It closes the file whose value is stored in the file handler when it was opened

  vir_open = f_handle = 0;                             // Sets the values of the file handler and vir_open as 0 indicating there is no file 
						      // open.
  return 0;
}

int writeBlock(int block, char *buffer)
{ 							// It checks if the disk is open or not . 
  if (!vir_open) {
    fprintf(stderr, "The disk is not active\n");
    return -1;
  }

  if ((block < 0) || (block >= DISK_BLOCKS)) {  	// If the block number where data is to be written is greater than the total number 
    fprintf(stderr, "The block index is out of bounds \n"); // of blocks then it gives an out of bounds error.
    return -1;
  }

  if (lseek(f_handle, block * BLOCK_SIZE, SEEK_SET) < 0) { // It sets the pointer to the block where the data is to be written 
    perror("lseek failed");                                
    return -1;
  }

  if (write(f_handle, buffer, BLOCK_SIZE) < 0) {   // It writes the data on the block. If the value returned is less than 0 then the  
    perror("Could not be written "); 	 	   // operation could not be performed.
    return -1;
  }

  return 0;
}

int readBlock(int block, char *buffer)
{
  if (!vir_open) {				// It checks if the disk is open or not .
    fprintf(stderr, "The disk is inactive \n");
    return -1;
  }

  if ((block < 0) || (block >= DISK_BLOCKS)) {
    fprintf(stderr, "The block index is out of bounds\n"); // It checks if the block number is less than the total number of blocks 
    return -1;						   // If not then it gives an out of bounds error.
  }

  if (lseek(f_handle, block * BLOCK_SIZE, SEEK_SET) < 0) { // It sets the pointer to the block from where the data is being read.
    perror("lseek failed");
    return -1;
  }

  if (read(f_handle, buffer, BLOCK_SIZE) < 0) {   // It reads the data from the block. The operation fails if it returns a value less 	
    perror("Could not be read");		// than 0. 
    return -1;
  }

  return 0;
}
