/*
   Group Members:

   Name : SAI KUMAR REDDY MOKKALA
   Student ID: 1001728207

   Name : NITISH REDDY MINNUPURI
   Student ID: 1001633261

*/


#define _GNU_SOURCE
#include<unistd.h>
#include<ctype.h>
#include<stdint.h>
#include<stdlib.h>
#include<errno.h>
#include<signal.h>
#include<string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include  <stdio.h>

#define WHITESPACE " \t\n"
#define MAX_COMMAND_SIZE 100

#define MAX_NUM_ARGUMENTS 12
#define MAX_HISTORY_COMMAND 15
#define SEMICOLON " ;
#define NUM_BLOCKS 4226                // Defining number of blocks
#define BLOCK_SIZE 8192                // Defining block size
#define NUM_FILES  128                 // Defining number of files
#define MAX_FILE_SIZE 10240000         // Defining maximum file size

FILE * fd;                   // File pointer

uint8_t blocks[NUM_BLOCKS][BLOCK_SIZE]; // Defining blocks


// Directory structure
struct Directory_Entry
{
    uint8_t  valid;
    char     filename[255];
    uint32_t inode;
    uint8_t  attrib;
};

// Inode structure
struct Inode
{
 uint8_t valid;
 uint8_t   attributes;
 uint32_t  size;
 uint32_t  blocks [1250];
};

struct Directory_Entry * dir;
struct Inode           * inodes;
uint8_t       *freeBlockList;
uint8_t       *freeInodeList;


// Initializing the free blocks
void initializeBlockList()
{
 int i;
 for (i=0; i < NUM_BLOCKS; i++)
 {
   freeBlockList[i] = 1;
 }
}

// Initializing the nodes whether they are valid or not
// Initializing size and attributes
void initializeInodes()
{
 int i;
 for (i=0; i < NUM_FILES; i++)
 {
   inodes[i].valid      = 0;
   inodes[i].size       = 0;
   inodes[i].attributes = 0;


   int j;
   for ( j =0 ; j < 1250 ; j++)
   {
       inodes[i].blocks[j] = -1;
   }
 }

}

void initializeInodeList()
{
 int i;
 for (i=0; i < NUM_FILES; i++)
 {
     freeInodeList[i] = 1;
 }
}

// Initializing the directory
void intializeDirectory()
{
 int i;
 for (i=0; i < NUM_FILES; i++)
    {
      dir[i].valid = 0;
      dir[i].inode = -1;

      memset( dir[i].filename,0, 255);
    }
}

//initializing the disk free space

int df()
{
 int i;
 int free_space = 0;
 for (i=0; i< NUM_BLOCKS; i++)
 {
   if ( freeBlockList[i] == 1 )
   {
     free_space = free_space + BLOCK_SIZE;
   }
 }
 return free_space;
}


// Function for finding the free block
int findFreeBlock ()
{
    // check for existing entry
    int i ;
    int ret = -1;
    for (i = 0; i < NUM_BLOCKS; i++)
    {
      if (freeBlockList[i] == 1)
      {
          freeBlockList[i] = 0;
          return i ;
      }
    }
    return ret;
}

// Function for finding the free block
int findFreeInode ()
{
    // check for existing entry
    int i  ;
    int ret = -1;
    for (i = 0; i < NUM_FILES; i++)
    {
      if (inodes[i].valid == 0)
      {
          inodes[i].valid = 1;
          return i ;
      }
    }
    return ret;
}

// Function for finding the directory entry

int findDirectoryEntry (char * filename)
{
    // check for existing entry
    int i ;
    int ret = -1;
    for (i = 0; i < NUM_FILES; i++)
    {
        if (strcmp(filename, dir[i].filename) == 0)
        {
            return i ;
        }

    }
    // find free space
    for (i=0; i< NUM_FILES; i++)
    {
        if (dir[i].valid == 0)
        {
           dir[i].valid = 1;
           return i;
        }

     }
  return ret;
}

// Function for fetching the filename with get in command prompt
int get ( char * filename)
{
    struct stat buf;
    int    ret;
    int    status;

    ret = stat(filename, &buf);

    if ( ret == -1)
    {
       printf("File does not exist \n");
       return -1;
    }

    int size = buf.st_size;
    printf("%d\n", size);

    if (size > MAX_FILE_SIZE)
    {
        printf("FIle size too big\n");
        return -1;
    }
    if (size > df())
    {
        printf("FIle exceeds remaining disk space \n");
        return -1;
    }
//put file in image
int directoryIndex = findDirectoryEntry(filename);
int InodeIndex =  findFreeInode();

status =  stat( filename, &buf );

  // If stat did not return -1 then we know the input file exists and we can use it.
  if( status != -1 )
  {

    // Open the input file read-only
    FILE *ifp = fopen ( filename, "r" );
    printf("Reading %d bytes from %s\n", (int) buf.st_size, filename );

    // Save off the size of the input file since we'll use it in a couple of places and
    // also initialize our index variables to zero.
    int copy_size   = buf.st_size;

    // We want to copy and write in chunks of BLOCK_SIZE. So to do this
    // we are going to use fseek to move along our file stream in chunks of BLOCK_SIZE.
    // We will copy bytes, increment our file pointer by BLOCK_SIZE and repeat.
    int offset      = 0;

    // We are going to copy and store our file in BLOCK_SIZE chunks instead of one big
    // memory pool. Why? We are simulating the way the file system stores file data in
    // blocks of space on the disk. block_index will keep us pointing to the area of
    // the area that we will read from or write to.
    int block_index = 0;

    // copy_size is initialized to the size of the input file so each loop iteration we
    // will copy BLOCK_SIZE bytes from the file then reduce our copy_size counter by
    // BLOCK_SIZE number of bytes. When copy_size is less than or equal to zero we know
    // we have copied all the data from the input file.
    while( copy_size > 0 )
    {

      // Index into the input file by offset number of bytes.  Initially offset is set to
      // zero so we copy BLOCK_SIZE number of bytes from the front of the file.  We
      // then increase the offset by BLOCK_SIZE and continue the process.  This will
      // make us copy from offsets 0, BLOCK_SIZE, 2*BLOCK_SIZE, 3*BLOCK_SIZE, etc.
      fseek( ifp, offset, SEEK_SET );

      // Read BLOCK_SIZE number of bytes from the input file and store them in our
      // data array.
      int bytes  = fread( blocks[block_index], BLOCK_SIZE, 1, ifp );

      // If bytes == 0 and we haven't reached the end of the file then something is
      // wrong. If 0 is returned and we also have the EOF flag set then that is OK.
      // It means we've reached the end of our input file.
      if( bytes == 0 && !feof( ifp ) )
      {
        printf("An error occured reading from the input file.\n");
        return -1;
      }

      // Clear the EOF file flag.
      clearerr( ifp );

      // Reduce copy_size by the BLOCK_SIZE bytes.
      copy_size -= BLOCK_SIZE;

      // Increase the offset into our input file by BLOCK_SIZE.  This will allow
      // the fseek at the top of the loop to position us to the correct spot.
      offset    += BLOCK_SIZE;

      // Increment the index into the block array
      block_index ++;
    }

    // We are done copying from the input file so close it out.
    fclose( ifp );

}
return 0;

}

// Function for retrieving the file and copying to other file
int get2 ( char * filename, char * newfilename)
{
    struct stat buf;
    int ret;
    int    status;

    ret = stat(filename, &buf);

      // If stat did not return -1 then we know the input file exists and we can use it.
  if( status != -1 )
  {

    // Open the input file read-only
    FILE *ifp = fopen ( filename, "r" );
    printf("Reading %d bytes from %s\n", (int) buf.st_size, filename );

    // Save off the size of the input file since we'll use it in a couple of places and
    // also initialize our index variables to zero.
    int copy_size   = buf.st_size;

    // We want to copy and write in chunks of BLOCK_SIZE. So to do this
    // we are going to use fseek to move along our file stream in chunks of BLOCK_SIZE.
    // We will copy bytes, increment our file pointer by BLOCK_SIZE and repeat.
    int offset      = 0;

    // We are going to copy and store our file in BLOCK_SIZE chunks instead of one big
    // memory pool. Why? We are simulating the way the file system stores file data in
    // blocks of space on the disk. block_index will keep us pointing to the area of
    // the area that we will read from or write to.
    int block_index = 0;

    // copy_size is initialized to the size of the input file so each loop iteration we
    // will copy BLOCK_SIZE bytes from the file then reduce our copy_size counter by
    // BLOCK_SIZE number of bytes. When copy_size is less than or equal to zero we know
    // we have copied all the data from the input file.
    while( copy_size > 0 )
    {

      // Index into the input file by offset number of bytes.  Initially offset is set to
      // zero so we copy BLOCK_SIZE number of bytes from the front of the file.  We
      // then increase the offset by BLOCK_SIZE and continue the process.  This will
      // make us copy from offsets 0, BLOCK_SIZE, 2*BLOCK_SIZE, 3*BLOCK_SIZE, etc.
      fseek( ifp, offset, SEEK_SET );

      // Read BLOCK_SIZE number of bytes from the input file and store them in our
      // data array.
      int bytes  = fread( blocks[block_index], BLOCK_SIZE, 1, ifp );

      // If bytes == 0 and we haven't reached the end of the file then something is
      // wrong. If 0 is returned and we also have the EOF flag set then that is OK.
      // It means we've reached the end of our input file.
      if( bytes == 0 && !feof( ifp ) )
      {
        printf("An error occured reading from the input file.\n");
        return -1;
      }

      // Clear the EOF file flag.
      clearerr( ifp );

      // Reduce copy_size by the BLOCK_SIZE bytes.
      copy_size -= BLOCK_SIZE;

      // Increase the offset into our input file by BLOCK_SIZE.  This will allow
      // the fseek at the top of the loop to position us to the correct spot.
      offset    += BLOCK_SIZE;

      // Increment the index into the block array
      block_index ++;
    }

    // We are done copying from the input file so close it out.
    fclose( ifp );


    FILE *ofp;
    ofp = fopen(newfilename, "w");

    if( ofp == NULL )
    {
      printf("Could not open output file: %s\n", newfilename );
      perror("Opening output file returned");
      return -1;
    }

    // Initialize our offsets and pointers just we did above when reading from the file.
    block_index = 0;
    copy_size   = buf.st_size;
    offset      = 0;

    printf("Writing %d bytes to %s\n", (int) buf.st_size, newfilename );

    // Using copy_size as a count to determine when we've copied enough bytes to the output file.
    // Each time through the loop, except the last time, we will copy BLOCK_SIZE number of bytes from
    // our stored data to the file fp, then we will increment the offset into the file we are writing to.
    // On the last iteration of the loop, instead of copying BLOCK_SIZE number of bytes we just copy
    // how ever much is remaining ( copy_size % BLOCK_SIZE ).  If we just copied BLOCK_SIZE on the
    // last iteration we'd end up with gibberish at the end of our file.
    while( copy_size > 0 )
    {

      int num_bytes;

      // If the remaining number of bytes we need to copy is less than BLOCK_SIZE then
      // only copy the amount that remains. If we copied BLOCK_SIZE number of bytes we'd
      // end up with garbage at the end of the file.
      if( copy_size < BLOCK_SIZE )
      {
        num_bytes = copy_size;
      }

      else
      {
        num_bytes = BLOCK_SIZE;
      }

      // Write num_bytes number of bytes from our data array into our output file.
      fwrite( blocks[block_index], num_bytes, 1, ofp );

      // Reduce the amount of bytes remaining to copy, increase the offset into the file
      // and increment the block_index to move us to the next data block.
      copy_size -= BLOCK_SIZE;
      offset    += BLOCK_SIZE;
      block_index ++;

      // Since we've copied from the point pointed to by our current file pointer, increment
      // offset number of bytes so we will be ready to copy to the next area of our output file.
      fseek( ofp, offset, SEEK_SET );
    }

    // Close the output file, we're done.
    fclose( ofp );
}
  else
  {
    printf("Unable to open file: %s\n", filename );
    perror("Opening the input file returned: ");
    return -1;
  }

return 0;

}


// Function for writing new file
int put(char *filename)

{
    struct stat buf;
    int ret;
    int    status;

    ret = stat(filename, &buf);


    int size = buf.st_size;
    printf("%d\n", size);

    if (size > MAX_FILE_SIZE)
    {
        printf("FIle size too big\n");
        return -1;
    }
    if (size > df())
    {
        printf("FIle exceeds remaining disk space \n");
        return -1;
    }
//put file in image
   int directoryIndex = findDirectoryEntry(filename);
   int InodeIndex =  findFreeInode();

   status =  stat( filename, &buf );



    strcpy(dir[directoryIndex].filename, filename);
    // Open the input file read-only
    FILE *ofp;
    ofp = fopen(filename, "w");


    if( ofp == NULL )
    {
      printf("Could not open output file: %s\n", filename );
      perror("Opening output file returned");
      return -1;
    }

    // Initialize our offsets and pointers just we did above when reading from the file.
    int  block_index = 0;
    int copy_size   = buf . st_size;
    int offset      = 0;

    printf("Writing %d bytes to %s\n", (int) buf . st_size, filename );

    // Using copy_size as a count to determine when we've copied enough bytes to the output file.
    // Each time through the loop, except the last time, we will copy BLOCK_SIZE number of bytes from
    // our stored data to the file fp, then we will increment the offset into the file we are writing to.
    // On the last iteration of the loop, instead of copying BLOCK_SIZE number of bytes we just copy
    // how ever much is remaining ( copy_size % BLOCK_SIZE ).  If we just copied BLOCK_SIZE on the
    // last iteration we'd end up with gibberish at the end of our file.
    while( copy_size > 0 )
    {

      int num_bytes;

      // If the remaining number of bytes we need to copy is less than BLOCK_SIZE then
      // only copy the amount that remains. If we copied BLOCK_SIZE number of bytes we'd
      // end up with garbage at the end of the file.
      if( copy_size < BLOCK_SIZE )
      {
        num_bytes = copy_size;
      }
      else
      {
        num_bytes = BLOCK_SIZE;
      }

      // Write num_bytes number of bytes from our data array into our output file.
      fwrite( blocks[block_index], num_bytes, 1, ofp );

      // Reduce the amount of bytes remaining to copy, increase the offset into the file
      // and increment the block_index to move us to the next data block.
      copy_size -= BLOCK_SIZE;
      offset    += BLOCK_SIZE;
      block_index ++;

      // Since we've copied from the point pointed to by our current file pointer, increment
      // offset number of bytes so we will be ready to copy to the next area of our output file.
      fseek( ofp, offset, SEEK_SET );
    }

    // Close the output file, we're done.
    fclose( ofp );





return 0;

}



// Function for listing the directories
void list()
{
    int i ;
    for (i=0; i< NUM_FILES; i++)
    {
        if (dir[i].valid ==1)
        {
            int inode_idx = dir[i].inode;
            printf("%s %d \n ", dir[i].filename, inodes[inode_idx].size);
        }
    }
}


// Function for creating the file image
void createfs( char * filename)
{
  fd = fopen(filename, "w");

  memset (&blocks[0], 0 ,NUM_BLOCKS * BLOCK_SIZE);
  intializeDirectory();
  initializeBlockList();
  initializeInodeList();
  initializeInodes();
  fwrite (&blocks[0], NUM_BLOCKS, BLOCK_SIZE, fd);
  fclose(fd);
}


int main(int argc, char *argv[])
{

     int i = 0;
     char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
     FILE *fp;
     int fileopen = 0; // check if FAT32.img is open or not
     int tokennumber = 0; // Initialize argument count to 0

     dir           = (struct Directory_Entry * )&blocks[0];
     inodes        = (struct Inode * )&blocks[9];
     freeInodeList = (uint8_t *)&blocks[7];
     freeBlockList = (uint8_t *)&blocks[8];
     /*
     intializeDirectory();
     initializeBlockList();
     initializeInodeList();
     initializeInodes();
     */

     while( 1 )
     {


           // It will print out the msh prompt


           printf ("mfs> ");
           /*
           It will read the command from the command line. The size of the maximum command is
           MAX_COMMAND_SIZE. The while command will wait until the user will type something
           and it will return NULL if the user does not tyoe anything.
           */

           while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );
           char *token[MAX_NUM_ARGUMENTS];   // Parse input;

           int   token_count = 0;            // Variable for counting tokens

           // Pointer to point to the token parsed by strsep

           char *arg_ptr;

           char *working_str  = strdup( cmd_str );
           /*
             we are going to move the working_str pointer so that it
             keep tracks of its original value because of this we can deallocate
             the correct amount at end
           */

           char *working_root = working_str;
           // We will be going to tokenize the input by whitespace as delimiter

           while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) &&(token_count<MAX_NUM_ARGUMENTS))

          {
                token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );

                if( strlen( token[token_count] ) == 0 )
                {
                    token[token_count] = NULL;
                }

                token_count++;
          }


         free(working_root);






        // If token is null, just prompt the mav shell with printing msh

        if( token [0]== NULL)
        {
               continue;
        }

        else if (strcmp(token[0],"df")== 0)
        {
              // Printing the remaining disk space
              printf("Disk space remaining %d\n",df());
        }

        else if (strcmp(token[0],"list")== 0)

        {     // Listing the directories
              list();

        }
        else if (strcmp(token[0],"createfs")== 0)

        {
            // If there is no filename given, prompt file not found
            if(token[1]== NULL)
            {
                printf("file not found\n");
            }
            else
            {
                 // If file name is given, create the file image
                 createfs(token[1]);
                 printf("file created\n");

            }


        }

        // Fetch the file from the directory if get is given in command shell
        else if (strcmp(token[0],"get")== 0 && token[2]== NULL)

        {



            get(token[1]);


        }

        // Fetch file from directory and copy into the new file
        else if (strcmp(token[0],"get")== 0 && token[2]!= NULL)

        {



            get2(token[1], token[2]);


        }



       // Wrie the new file into directory if put is given

       else if (strcmp(token[0],"put")== 0)

       {

            put(token[1]);


       }



      // If file is not open, open the file

      else if(strcmp(token[tokennumber], "open")==0 && token_count==3 && fileopen==0)
      {


           int i;
           fp = fopen(token[tokennumber+1], "r");

           if(fp == NULL)
           {

                printf("error opening the file");
           }


           for(i=0; i< 16; i++)
           {

           fread(&dir[i], sizeof(struct Directory_Entry), 1, fp);
           }

           for (i=0; i< 16; i++)
           {
             char file[12];
             memcpy(file, dir[i].filename,11);

             file[11]= '\0';

           }



           fileopen++;

       }

       // If file is open and is opened again
       else if ((strcmp(token[tokennumber],"open")==0)&& fileopen != 0)
       {

       printf("Error: File not present or already open\n");
       continue;
       }

       // If file is open and then closed
       else if((strcmp(token[tokennumber], "close")==0) && fileopen != 0)
       {
       fclose(fp);
       fileopen =0;
       }

       // If file is closed and is being closed again
       else if((strcmp(token[tokennumber], "close")== 0) && fileopen ==0)
       {

       printf("File system is not open\n");
       }

}
}
