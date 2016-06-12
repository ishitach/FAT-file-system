#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int File_search(char *fileName); // return -1 if no exist, else the file descriptor.
int FreeBlockInFat_search(void);
void Free_Data_Block(int numBlock);
void Save_Oft(int fildes, int block, int offsetInBlock, int offsetFilePointer);
#define DISK_BLOCKS 5000        /* number of blocks on the disk              */
#define BLOCK_SIZE 4000    
#define META_SIZE 44000
char meta[META_SIZE];

int Directory_offset = 1 * BLOCK_SIZE;
int FAT_offset = 5 * BLOCK_SIZE; // File Allocation Table offset.
int OFT_offset = 9 * BLOCK_SIZE; // Open File Table offset.
int Unit_Directory = 8;
int Unit_FAT = 2;
int Unit_OFT = 4;

int FileSys_make(char *disk_name) {
	char buf[BLOCK_SIZE];
	if (create_disk(disk_name) == -1) return -1;	//  It creates a disk for the file system.
	if (Disk_open(disk_name) == -1) return -1;	//  It opens the disk . 
	strcpy(buf, disk_name);						
	if (writeBlock(0, buf) == -1) return -1;	// It writes the name of the file system to the disk.
	Disk_close(); 					// It closes the disk .
	return 0;					//The disk operations used are created in disk.c 
}

int FileSys_mount(char *disk_name) {     
	char buf[BLOCK_SIZE];
	int i, j;
	if (Disk_open(disk_name) == -1) return -1;        // Opens the disk and 
	                                                  // reads the metadata from the first 11 blocks of disk.
	for (i = 0; i < 11; i++) {
		if (readBlock(i, buf) == -1) return -1; // The loop runs for the first 11 blocks and the inner loop runs from 0- Block Size
		for (j = 0; j < 4000; j++) {
			meta[i * 4000 + j] = buf[j];      // which is 4kB. It updates the meta array.
		}
	}
	return 0;
}

int FileSys_dismount(char *disk_name) {
	char buf[BLOCK_SIZE];
	int i, j, oftFlag;                                 // Closes all the files that are open in the file system.
	for (i = 0; i < 8; i++) {
		oftFlag = meta[i * Unit_OFT + OFT_offset]; // It checks the OFT table for each file . If the flag is 1 which means that the 
		oftFlag = oftFlag & 1;                    // file is open then it closes that file.
		if (oftFlag == 1) {
			FileSys_close(i);
		}
	}
							//It writes the metadata to the first 11 blocks of disk.
	for (i = 0; i < 11; i++) {
		for (j = 0; j < 4000; j++) {
			buf[j] = meta[i * 4000 + j];
		}
		if (writeBlock(i, buf) == -1) return -1;
	}
							// Closes the disk at the end after writing to dismount the file system.
	Disk_close();
	return 0;
}

int FileSys_Open(char *name) {
	int i, oftFlag, currBlock;		// Searches for the file descriptor.
	int fildes = File_search(name);
	if (fildes == -1) {
		printf(" \n Error: File not found %s \n", name);
		return -1;
	}                                                
	                                                  // checks in the OFT table if the file has been opened before and not closed yet.
	oftFlag = meta[fildes * Unit_OFT + OFT_offset];   
	oftFlag = oftFlag & 1;                            // If the value is 1 , it means that the file is open.
	if (oftFlag == 1) {
		printf(" \nError: file %s has been opened \n", name);
		return -1;
	}								// If the file is not opened yet, open the file and update the 
	currBlock = meta[Directory_offset + fildes * Unit_Directory + 7]; // values in the OFT table . It saves the data block where the 
	meta[fildes * Unit_OFT + OFT_offset] = 1 | (currBlock << 1); // file pointer is located and updates the value in OFT from 0 to 1.
	return fildes;
}

int FileSys_close(int fildes) {
	int oftFlag;                                   // checks whether the file descriptor exists in directory.
	if (meta[Directory_offset + fildes * 8] == 0) {
		printf(" \nError: file descriptor %d does not exist \n", fildes);
		return -1;
	}						// checks whether the file descriptor is opened or not by checking in the OFT table
	oftFlag = meta[fildes * Unit_OFT + OFT_offset];
	oftFlag = oftFlag & 1;
	if (oftFlag == 0) {
		printf("\n Error: the fildes %d is not opened \n", fildes);
		return -1;
	}
							// set the values in the OFT table to 0 to indicate the file is closed.
	meta[fildes * Unit_OFT + OFT_offset] = 0;
	meta[fildes * Unit_OFT + 1 + OFT_offset] = 0;
	meta[fildes * Unit_OFT + 2 + OFT_offset] = 0;
	meta[fildes * Unit_OFT + 3 + OFT_offset] = 0;
	return 0;
}

int FileSys_create(char *name) {
	int i, j, len, firstBlock;    // checks the length of the file name. If the length greater than 4 bytes , it gives an error.
	len = strlen(name);
	if (strlen(name) > 4) {
		printf("\n Error: the length of file name - %s exceeds 4 bytes \n", name);
		return -1;
	}			// checks whether the file with that file name already exists or not .
	if (File_search(name) > -1) {
		printf("\n Error: file %s already exists in the file system \n", name);
		return -1;
	}			// Searches for an available directory entry. If all the directories are occupied then it gives an error
	for (i = 0; i < 8; i++) {
		if (meta[Directory_offset + i * Unit_Directory] == 0) break;
	}
	if (i >= 8) {  // Number of directories in the system at a point cannot exceed 8. 
		printf(" \n Error: the number of files exceeds 8 \n");
		return -1;
	}				// check if there are any free data blocks in file disk by cheking the FAT table.
	if ((firstBlock = FreeBlockInFat_search()) == -1) {
		return -1;
	}
	meta[Directory_offset + i * Unit_Directory] = 1;  // Saves the file name in the directory.
	for (j = 0; j < len; j++) {
		meta[Directory_offset + i * Unit_Directory + 3 + j] = name[j]; 
	}							// saves the information about first data block into directory.
	meta[Directory_offset + i * Unit_Directory + 7] = firstBlock;  
	meta[FAT_offset + (firstBlock - 32) * Unit_FAT] = 1;         // marks the block as used in FAT by assigning the value 1.
	return 0;
}

int FileSys_delete(char *name) {
	int fildes, firstBlock, i;		// checks if the file exists in the file system or not . 
	if ((fildes = File_search(name)) == -1) {
		printf(" \n Error: file %s does not exist in file system \n", name);
		return -1;
	}				// check if the file is open or not.If it is open then it needs to be closed before deleting.
	if ((meta[OFT_offset + fildes * Unit_OFT] & 1) == 1) {
		printf("\n Error: file %s is opened now(close the file before deleting). \n", name);
		return -1;
	}
	firstBlock = meta[Directory_offset + fildes * Unit_Directory + 7]; // Free all the data block of the file according to FAT.
	Free_Data_Block(firstBlock);  
	for (i = 0; i < Unit_Directory; i++) {  // Clear the directory entry by setting it to 0.
		meta[Directory_offset + fildes * Unit_Directory + i] = 0;
	}
	printf("  \n file %s DELETED \n", name);
	return 0;
}

int FileSys_read(int fildes, void *buf_, size_t nbyte) {
	int blockNo, nextBlockNo, offsetInBlock, offsetFilePointer, len = 0;
	char blockBuf[BLOCK_SIZE];
	char *buf = (char *) buf_;		
	if (fildes < 0 || fildes >= 8) { 	// checks if the file descriptors are valid or not. 
		printf("\n Error: invalid file descriptor %d \n", fildes);
		return -1;
	}
	
	if (meta[Directory_offset + fildes * Unit_Directory] == 0) { // checks if the file exists or not
		printf("\n Error: invalid file descriptor %d . The file does not exist in file system. \n", fildes);
		return -1;
	}
	
	if ((meta[OFT_offset + fildes * Unit_OFT] & 1) == 0) { // Checks if the file is opened or not 
		printf("\n Error: the file %d is not opened. \n", fildes);
		return -1;
	}
	
	blockNo = meta[OFT_offset + fildes * Unit_OFT] >> 1; // obtains the current file pointer from OFT and sets the offset pointers
	offsetInBlock = meta[OFT_offset + fildes * Unit_OFT + 1];
	offsetFilePointer = meta[OFT_offset + fildes * Unit_OFT + 3];
	offsetFilePointer = (offsetFilePointer << 8) + meta[OFT_offset + fildes * Unit_OFT + 2];
	
	while (len < nbyte) {  				// read the data upto nbyte and saves them to buf
		readBlock(blockNo, blockBuf);
		while (offsetInBlock < BLOCK_SIZE && len < nbyte && blockBuf[offsetInBlock] != 0) {
			buf[len] = blockBuf[offsetInBlock];
			len++;
			offsetInBlock++;
			offsetFilePointer++;
		}
		
		if (offsetInBlock < BLOCK_SIZE && blockBuf[offsetInBlock] == 0) { // if the end of file is reached then it saves the offsets
			Save_Oft(fildes, blockNo, offsetInBlock, offsetFilePointer); // in the OFT table .
			return len; 
		} 
		
		else if (len == nbyte) { // if the length is the same as nybte and the block offset is less than the block size then OFT 
			if (offsetInBlock < BLOCK_SIZE) { // table is updated.
				Save_Oft(fildes, blockNo, offsetInBlock, offsetFilePointer);
				return len;
			} else { // else it gets the offset of the next block and updates the OFT table accordingly.
				nextBlockNo = meta[FAT_offset + (blockNo - 32) * Unit_FAT + 1];
				if (nextBlockNo == 0) Save_Oft(fildes, blockNo, BLOCK_SIZE - 1, offsetFilePointer - 1);//It's the last block
				else Save_Oft(fildes, nextBlockNo, 0, offsetFilePointer);
				return len;
			}
		} else if (len < nbyte && offsetInBlock == BLOCK_SIZE) { // If the length is less than the nbyte but the block offset is 
			nextBlockNo = meta[FAT_offset + (blockNo - 32) * Unit_FAT + 1]; // equal to the block size 
			if (nextBlockNo == 0) { // Last block
				printf(" Warning: reached the end of file \n");
				Save_Oft(fildes, blockNo, BLOCK_SIZE - 1, offsetFilePointer - 1); // Update OFT 
				return len;
			} else {
				blockNo = nextBlockNo; // Goes to the next block and sets its offset as 0.
				offsetInBlock = 0;
			}
		} else {
			printf(" Warning: An abnormal situation is reached in the read function\n");
			return -1;
		}
	}
	return 0;
}

int FileSys_write(int fildes, void *buf_, size_t nbyte) {
	int blockNo, nextBlockNo, offsetInBlock, offsetFilePointer, len = 0, fileLength;
	char blockBuf[BLOCK_SIZE];
	char * buf = (char *) buf_;
	
	if (fildes < 0 || fildes >= 8) {   // Checks if the file descriptors are valid or not.
		printf("Error: invalid file descriptor %d \n", fildes);
		return -1;
	}
	
	if (meta[Directory_offset + fildes * Unit_Directory] == 0) {		 // checks if the file exists or not .
		printf("\n Error: invalid file descriptor %d . The file does not exist in file system. \n", fildes);
		return -1;
	}
	
	if ((meta[OFT_offset + fildes * Unit_OFT] & 1) == 0) { 			 // checks if the file is already opened or not.
		printf("\n Error: the file %d is not opened. \n", fildes);
		return -1;
	}
	
	blockNo = meta[OFT_offset + fildes * Unit_OFT] >> 1; 			// Obtain the current file pointer from OFT table and 
	offsetInBlock = meta[OFT_offset + fildes * Unit_OFT + 1];		// set the block offset and file offset
	offsetFilePointer = meta[OFT_offset + fildes * Unit_OFT + 3];
	offsetFilePointer = (offsetFilePointer << 8) + meta[OFT_offset + fildes * Unit_OFT + 2];
	fileLength = meta[Directory_offset + fildes * Unit_Directory + 2]; 	//get file length from directory.
	fileLength = (fileLength << 8) + meta[Directory_offset + fildes * Unit_Directory + 1];
	
	while (len < nbyte) { 							// write nbyte data to data blocks.
		readBlock(blockNo, blockBuf);
		while (offsetInBlock < BLOCK_SIZE && len < nbyte) {
			blockBuf[offsetInBlock] = buf[len];
			len++;
			offsetInBlock++;
			offsetFilePointer++;
		}
		writeBlock(blockNo, blockBuf); 
		if (len == nbyte) { 						// if length is equal to number of bytes to be read and
			if (offsetInBlock < BLOCK_SIZE) { 			// block offset is less than block size 
				Save_Oft(fildes, blockNo, offsetInBlock, offsetFilePointer); // Update OFT table 
			} 
			
			else {
				nextBlockNo = FreeBlockInFat_search();		 // Search for the next data block used in FAT
				if (nextBlockNo != -1) {
					meta[FAT_offset + (nextBlockNo - 32) * Unit_FAT] = 1; // if next block exist then set block offset 1
					meta[FAT_offset + (blockNo - 32) * Unit_FAT + 1] = nextBlockNo; 
					Save_Oft(fildes, nextBlockNo, 0, offsetFilePointer); 	// update OFT
				} else {
					meta[FAT_offset + (blockNo - 32) * Unit_FAT + 1] = 0;
					Save_Oft(fildes, blockNo, offsetInBlock - 1, offsetFilePointer - 1); // update OFT
				}
			}
			
			if (fileLength < offsetFilePointer) 	
			 fileLength = offsetFilePointer; 			 // update file length in metadata blocks
			 
			meta[Directory_offset + fildes * Unit_Directory + 2] = fileLength >> 8;
			meta[Directory_offset + fildes * Unit_Directory + 1] = fileLength & 0xff;
			return len;
		} 
		
		else {
			nextBlockNo = FreeBlockInFat_search();
			if (nextBlockNo != -1) {  // if next block exists then set offsets and entry in FAT
				meta[FAT_offset + (nextBlockNo - 32) * Unit_FAT] = 1;
				meta[FAT_offset + (blockNo - 32) * Unit_FAT + 1] = nextBlockNo;
				blockNo = nextBlockNo;
				offsetInBlock = 0;
			} else { 						//  if no free block available for writing data. Update the 											//OFT , FAT and file length in metadata blocks
				printf("\n Error: no free data block available for writing file \n");
				meta[FAT_offset + (blockNo - 32) * Unit_FAT + 1] = 0;
				Save_Oft(fildes, blockNo, offsetInBlock - 1, offsetFilePointer - 1);
				
				if (fileLength < offsetFilePointer) fileLength = offsetFilePointer;
				meta[Directory_offset + fildes * Unit_Directory + 2] = fileLength >> 8;
				meta[Directory_offset + fildes * Unit_Directory + 1] = fileLength & 0xff;
				return len;
			}
		}
	}
	return 0;
}

int FileSys_GetFileSize(int fildes) {
	int fileLength;
	
	if (fildes < 0 || fildes >= 8) { // Check if the file descriptors are valid or not 
		printf("\n Error: invalid file descriptors %d \n", fildes);
		return -1;
	}
	
	if (meta[Directory_offset + fildes * Unit_Directory] == 0) { // check if the file exists or not.
		printf("\n Error: invalid file descriptor %d. The file does not exist in file system. \n", fildes);
		return -1;
	}

	fileLength = meta[Directory_offset + fildes * Unit_Directory + 2];  // Get the file length from the directory.
	fileLength = (fileLength << 8) + meta[Directory_offset + fildes * Unit_Directory + 1];
	return fileLength;
}
	
int FileSys_lseek(int fildes, off_t offset) {
	int blockNo, nextBlockNo, offsetInBlock, offsetFilePointer, fileLength, numBlock;
	
	if (fildes < 0 || fildes >= 8) { // // Checks if the file descriptors are valid or not 
		printf(" \n Error: invalid file descriptors %d \n", fildes);
		return -1;
	}
	
	if (meta[Directory_offset + fildes * Unit_Directory] == 0) { // check if the file exists or not.
		printf("\n Error: invalid file descriptor %d . the file does not exist in file system. \n", fildes);
		return -1;
	}
	
	if ((meta[OFT_offset + fildes * Unit_OFT] & 1) == 0) {  // check if the file is opened or not.
		printf("\n Error: the file %d is not opened. \n", fildes);
		return -1;
	}
	
	blockNo = meta[OFT_offset + fildes * Unit_OFT] >> 1; // Obtain the current file pointer from OFT and sets offsets 
	offsetInBlock = meta[OFT_offset + fildes * Unit_OFT + 1];
	offsetFilePointer = meta[OFT_offset + fildes * Unit_OFT + 3];
	offsetFilePointer = (offsetFilePointer << 8) + meta[OFT_offset + fildes * Unit_OFT + 2];
	fileLength = meta[Directory_offset + fildes * Unit_Directory + 2]; //get file length from directory.
	fileLength = (fileLength << 8) + meta[Directory_offset + fildes * Unit_Directory + 1];
	if (offset == 0) return 0;
	
	if (offsetFilePointer + offset < 0 || offsetFilePointer + offset > fileLength) { // Check the boundary of file pointers.
		printf(" \n Error: out of bounds \n");
		return -1;
	}
	
	offsetFilePointer += offset; 					// Calculate the new position of file pointer.
	numBlock = offsetFilePointer / BLOCK_SIZE;
	offsetInBlock = offsetFilePointer % BLOCK_SIZE;
	blockNo = meta[Directory_offset + fildes * Unit_Directory + 7];
	
	while (numBlock > 0) { 						// find the new position as the style of block and offset in block.
		nextBlockNo = blockNo;
		blockNo = meta[FAT_offset + (blockNo - 32) * Unit_FAT + 1];
		numBlock--;
	}
	if (blockNo == 0) Save_Oft(fildes, nextBlockNo, BLOCK_SIZE - 1, offsetFilePointer - 1);
	
	else Save_Oft(fildes, blockNo, offsetInBlock, offsetFilePointer); // update OFT with new file pointer.
	return 0;
}

int FileSys_Truncate(int fildes, off_t length) {
	int i, fileLength, blockNo, numBlock, offsetInBlock, nextBlockNo;
	char blockBuf[BLOCK_SIZE];
	
	if (fildes < 0 || fildes >= 8) { // Checks if valid file descriptor.
		printf(" Error: invalid fildes %d \n", fildes);
		return -1;
	}

	if (meta[Directory_offset + fildes * Unit_Directory] == 0) {
	printf(" Error: invalid fildes %d : the file does not exist in file system. \n",fildes);// check whether the file exists or not.
		return -1;
	}
	if ((meta[OFT_offset + fildes * Unit_OFT] & 1) == 0) {
		printf("Error: the file %d is not opened.\n", fildes); // check whether the file is opened or not.
		return -1;
	}
	if (length < 0) {
		printf(" Error: invalid truncate length. it should be non-negative \n");// check the validation of truncate length.
		return -1;
	}

	fileLength = meta[Directory_offset + fildes * Unit_Directory + 2];		// Get file length from directory.
	fileLength = (fileLength << 8) + meta[Directory_offset + fildes * Unit_Directory + 1];
	if (length == fileLength) return 0;
	else if (length > fileLength) { 						 // Check the boundary of truncate.
	printf("Error: the truncate length is larger than the length of file \n");	// Calculate the new file end of the truncated file
		return -1;
	} else {						
		blockNo = meta[Directory_offset + fildes * Unit_Directory + 7];
		Save_Oft(fildes, blockNo, 0, 0);
		numBlock = length / BLOCK_SIZE;
		offsetInBlock = length % BLOCK_SIZE;
		while (numBlock > 0) {
			blockNo = meta[FAT_offset + (blockNo - 32) * Unit_FAT + 1];
			numBlock--;
		}
		nextBlockNo = meta[FAT_offset + (blockNo - 32) * Unit_FAT + 1];
		meta[FAT_offset + (blockNo - 32) * Unit_FAT + 1] = 0;
		
		Free_Data_Block(nextBlockNo);						// Free blocks of the truncated part
		readBlock(blockNo, blockBuf);
		for (i = offsetInBlock; i < BLOCK_SIZE; i++) {
			blockBuf[i] = 0;
		}
		writeBlock(blockNo, blockBuf);
		return 0;
	}
}

int File_search(char *fileName) {						
	int i, j, len;
	len = strlen(fileName);
	for (i = 0; i < 8; i++) {							//checks for the file name in the directory 
		for (j = 0; j < len; j++) {
			if (meta[Directory_offset + i * Unit_Directory + 3 + j] != fileName[j]) break;
		}  //if there is a mismatch in the name then it break from that loop and checks for the next file.
		if (j >= len) return i;
	}
	return -1;
}

int FreeBlockInFat_search(void) { // checks for the free block in fat.
	int i, fatFlag;	
	for (i = 0; i < 32; i++) {
		fatFlag = meta[FAT_offset + i * Unit_FAT]; // It checks if the 1st byte is 0 or 1
		if (fatFlag == 0) return (i + 32);
	}
	printf("Error: No available data blocks in file system.\n");
	return -1;
}

void Free_Data_Block(int numBlock) {
	char buf[BLOCK_SIZE];
	int nextBlock;
	memset(buf, 0, BLOCK_SIZE); // It creates a buffer of the length of the block size and sets all its bytes as 0
	while (numBlock != 0) {
		writeBlock(numBlock, buf);       // writes the values in the buf in that block. 
		nextBlock = meta[FAT_offset + (numBlock - 32) * Unit_FAT + 1];  
		meta[FAT_offset + (numBlock - 32) * Unit_FAT] = 0;		// set both the bytes of the block in FAT as 0
		meta[FAT_offset + (numBlock - 32) * Unit_FAT + 1] = 0;
		numBlock = nextBlock;
	}
}

void Save_Oft(int fildes, int block, int offsetInBlock, int offsetFilePointer) { // updates the oft table according to the offsets 
 	meta[OFT_offset + fildes * Unit_OFT] = (block << 1) | 1;
	meta[OFT_offset + fildes * Unit_OFT + 1] = offsetInBlock;
	meta[OFT_offset + fildes * Unit_OFT + 2] = offsetFilePointer & 0xff;
	meta[OFT_offset + fildes * Unit_OFT + 3] = offsetFilePointer >> 8;
} 
