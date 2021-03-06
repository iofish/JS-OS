/*
 * initrd.c
 *
 * Copyright 2013 JS-OS <js@duck-squirell>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 *
 */

#include <system.h>

initrd_header_t *initrd_header;     // The header.
initrd_file_header_t *file_headers; // The list of file headers.
fs_node_t *initrd_root;             // Our root directory node.
fs_node_t *initrd_dev;              // We also add a directory node for /dev, so we can mount devfs later on.
fs_node_t *root_nodes;              // List of file nodes.
int nroot_nodes;                    // Number of file nodes.

u32int fs_location;

extern u32int initrd_location;
extern u32int initrd_end;

static struct dirent dirent;

u32int initrd_read(fs_node_t *node, u32int offset, u32int size, u8int *buffer)
{
  //get the index of the starting and ending blocks (inclusively)
  u32int startingBlock = (u32int)(offset / BLOCK_SIZE);

  /*the offset - 1 is because if we were to offset by 1 block (BLOCK_SIZE)
   * and the u32int size was the size of a block, we would actually, unintentually,
   * be entering the next block by one byte since an offset of 1024 bytes
   * is one byte into the next block */
  u32int endingBlock = (u32int)((offset - 1 + size) / BLOCK_SIZE);

  //error checks
  if(offset > node->length)
    return 0;

  if(offset + size > node->length)
    size = node->length - offset;

  u32int i, size_to_copy, block_size_at_index;
  u32int block;
  for(i = startingBlock; i < endingBlock + 1; i++)
  {
    //get the block size at a specific index
    block_size_at_index = blockSizeAtIndex(node->length, i, offset);

    /*if this block_size is greater than the size of the buffer to copy too
     * limit it to the size of the buffer */
    size_to_copy = block_size_at_index > size ? size : block_size_at_index;

    //get the address of the block that we will read from
    block = (u32int)block_of_set(node, i);

    /*If there is an offset and it is the startingBlock, we have to indent
     * the starting read location (in only the starting block)
     * by offset % (mod) BLOCK_SIZE
     *
     * Also, there is (i - startingBlock) because if i was to not start at 0,
     * then we would not start writing in the begining of the buffer, the
     * subtraction of the startingBlock offsets that so we start writing
     * in the begining, eventhough i did not start at 0
     *
     *The following examples assume that we are in the direct blocks of the node
     * the concept is the same for the indirect blocks
     *
     * (u32int*)block points to the address of the node->blocks[i]
     * e.g. &node->blocks[i]
     *
     * then the *(u32int*)block is the value of that address, i.e. it impersonates
     * that it (u32int block) is node->blocks[i], although
     * they are found at different memory locations, same concept with hard links
     */  

    if(offset && i == startingBlock)
      memcpy(buffer + (i - startingBlock) * BLOCK_SIZE, (u8int*)(*(u32int*)block + offset % BLOCK_SIZE), size_to_copy);
    else
      memcpy(buffer + (i - startingBlock) * BLOCK_SIZE, (u8int*)(*(u32int*)block), size_to_copy);

  }

  return size;
}

u32int initrd_write(fs_node_t *node, u32int offset, u32int size, u8int *buffer)
{
  u32int startingBlock = (u32int)(offset / BLOCK_SIZE);
  u32int endingBlock = (u32int)((offset + size) / BLOCK_SIZE);

  if(offset > node->length)
    return 0;

  if(offset + size > node->length)
    size = node->length - offset;

  u32int i, block_size_at_index, size_to_copy;
  u32int block;
  for(i = startingBlock; i < endingBlock + 1; i++)
  {

    //~ k_printf("\n%d", i);

    //get the writing size at a specific index
    block_size_at_index = blockSizeAtIndex(node->length, i, offset);

    //gets the address of the block to write to
    block = (u32int)block_of_set(node, i);

    /*If there is an offset and it is the startingBlock, we have to indent
     * the starting read location (in only the starting block)
     * by offset % (mod) BLOCK_SIZE
     *
     * Also, there is (i - startingBlock) because if i was to not start at 0,
     * then we would not start writing in the begining of the buffer, the
     * subtraction of the startingBlock offsets that so we start writing
     * in the begining, eventhough i did not start at 0
     *
     *The following examples assume that we are in the direct blocks of the node
     * the concept is the same for the indirect blocks
     *
     * (u32int*)block points to the address of the node->blocks[i]
     * e.g. &node->blocks[i]
     */

    if(offset && i == startingBlock)
        memcpy((u8int*)(*(u32int*)block), buffer + (i - startingBlock) * BLOCK_SIZE 
               + offset % BLOCK_SIZE, block_size_at_index);
    else
        memcpy((u8int*)(*(u32int*)block), buffer + (i - startingBlock) * BLOCK_SIZE, block_size_at_index);
     
  }

  return size;
}

struct dirent *initrd_readdir(fs_node_t *dirNode, u32int index)
{

  if(dirNode->flags = FS_DIRECTORY) //just to check if the input node is a directory
  {

    u32int i = 0;
    u32int loop = 0, b = 0, block;

    block = (u32int)block_of_set(dirNode, b);

    //loop forever, we will break when we find it
    for(;;)
    {
      //if the loop equals the index we are looking for
      if(loop == index)
      {
        //if the rec_len of the direct has contents
        if(*(u16int*)(*(u32int*)block + i + sizeof(dirent.ino)))
        {
          static struct dirent dirent2;

          //extract the dirent information at the offset of i
          dirent2.ino = *(u32int*)(*(u32int*)block + i);
          dirent2.rec_len = *(u16int*)(*(u32int*)block + i + sizeof(dirent2.ino));
          dirent2.name_len = *(u8int*)(*(u32int*)block + i + sizeof(dirent2.ino) + sizeof(dirent2.rec_len));
          dirent2.file_type = *(u8int*)(*(u32int*)block + i + sizeof(dirent2.ino) + sizeof(dirent2.rec_len) + sizeof(dirent2.name_len));

          //clears junk that may be contained in memory when kmallocing
          dirent2.name = (char*)kmalloc(dirent2.name_len + 1);

          //clears junk that may be contained in memory when kmallocing
          memset(dirent2.name, 0, dirent2.name_len + 1);

          //copies the name to dirent2.name
          memcpy(dirent2.name, (char*)(*(u32int*)block + i + sizeof(dirent2.ino) + sizeof(dirent2.rec_len) + sizeof(dirent2.name_len) + sizeof(dirent2.file_type)), dirent2.name_len + 1 );

          *(dirent2.name + dirent2.name_len) = 0; //Adds terminating 0 to string

          return &dirent2;

        }else{
          //error
          return 0;
        }

      }else{

        //this dirent is not the last one (there are more dirents after this one)
        if(*(u16int*)(*(u32int*)block + i + sizeof(dirent.ino)))
        {
          //increase i with the rec_len that we get by moving fileheader sizeof(dirent.ino) (4 bytes) and reading its value
          i = i + *(u16int*)(*(u32int*)block + i + sizeof(dirent.ino));
          loop++;
        }else{ //this is the last direct, add 1 to block and reset the offset (i)
          i = 0;
          b++;

          //recalculate the block address
          block = (u32int)block_of_set(dirNode, b);
        }

      }

    }

  }else{
    return 0;
  }
}

fs_node_t *initrd_finddir(fs_node_t *dirNode, char *name)
{
  if(dirNode == initrd_root && !strcmp(name, "/"))
    return initrd_dev;

  if(dirNode->flags == FS_DIRECTORY)
  {
    struct dirent *dirent2;

    u32int i;
    for(i = 0; i < nroot_nodes; i++)
    {
      //get the dirent information at index i
      dirent2 = initrd_readdir(dirNode, i);

      //does this dirent exits?
      if(dirent2)
      {
        //the input name matches to the dirent2.name that we got
        if(!strcmp(name, dirent2->name))
        {
          kfree(dirent2->name);
          return &root_nodes[dirent2->ino];
        }

        kfree(dirent2->name);
      }
    }

  }

  //no file found, error
  return 0;
}

fs_node_t *initialise_initrd(u32int location)
{
  fs_location = kmalloc(FS_SIZE);

  u32int tempLoc = kmalloc(initrd_end - initrd_location); //random size, make it defined

  /*size = the size of the initrd module (address of end - address of start),
   * end and start are defined by the bootloader*/
  memcpy(tempLoc, location, initrd_end - initrd_location);

  // Initialise the main and file header pointers and populate the root directory.
  initrd_header = (initrd_header_t *)tempLoc; //first part of location is the number of headers (in struct)

  //the location offset with the struct that contains the number of headers
  file_headers = (initrd_file_header_t *)(tempLoc + sizeof(initrd_header_t));

  // Initialise the root directory.
  initrd_root = (fs_node_t*)kmalloc(sizeof(fs_node_t));
  strcpy(initrd_root->name, "/");

  initrd_root->magic = M_VFS;
  initrd_root->mask = initrd_root->uid = initrd_root->gid = initrd_root->inode = initrd_root->length = 0;
  initrd_root->flags = FS_DIRECTORY;
  initrd_root->read = 0;
  initrd_root->write = 0;
  initrd_root->readdir = &initrd_readdir;
  initrd_root->finddir = &initrd_finddir;
  initrd_root->ptr = 0;
  initrd_root->impl = 0;

  //allocate space for the root nodes, default 32KB size
  root_nodes = (fs_node_t*)kmalloc(sizeof(fs_node_t) * (0x8000));
  nroot_nodes = initrd_header->nfiles;

  // For every file...
  u32int i, a, allocSize, nBlocks;
  u32int block;
  for(i = 0; i < initrd_header->nfiles; i++)
  {
    // Edit the file's header - currently it holds the file offset
    // relative to the start of the ramdisk. We want it relative to the start
    // of memory.
    file_headers[i].offset += tempLoc;
    // Create a new file node.
    strcpy(root_nodes[i].name, &file_headers[i].name);

    root_nodes[i].magic = M_VFS;
    root_nodes[i].mask = 0b110110100; //user rw, group rw, other r
    root_nodes[i].uid = root_nodes[i].gid = 0;
    root_nodes[i].length = 0;
    //~ root_nodes[i].length = file_headers[i].length;
    root_nodes[i].inode = i;
    root_nodes[i].flags = FS_FILE;
    root_nodes[i].read = &initrd_read;
    root_nodes[i].write = &initrd_write;
    root_nodes[i].readdir = 0;
    root_nodes[i].finddir = 0;
    root_nodes[i].impl = 0;

    //calculates the number of blocks this file has
    nBlocks = ((u32int)((file_headers[i].length - 1) / BLOCK_SIZE) + 1);

    for(a = 0; a < nBlocks; a++)
    {
      /* /\* This section copies the file's data into the blocks of the node
       *  *
       *  * ((u32int)(root_nodes[i].length / BLOCK_SIZE) + 1) calculates the
       *  * number of blocks this file will take up at a given length
       *  * the rootnode[i].length -1 is to offset that the first bit is at position 0*\/
       * for(a = 0; a < nBlocks; a++)
       * {
       *   /\*This section allocates memory for any initial sets apart from
       *    * the direct blocks, as by default, the set pointers in fs_node_t
       *    * typedef point to nowhere *\/
       *   //if this file only has a singly set
       *   if(a >= BLOCKS_DIRECT && a < BLOCKS_DIRECT + BLOCKS_SINGLY)
       *   {
       *     //allocate the singly typedef
       *     if(!root_nodes[i].singly)
       *     {
       *       root_nodes[i].singly = (u32int*)kmalloc(BLOCK_SIZE);
       * 
       *       //clear the singly block of any junk
       *       memset(root_nodes[i].singly, 0x0, BLOCK_SIZE);
       *     }
       * 
       *     //if this file has a singly set and a doubly set
       *   }else if(a >= BLOCKS_DIRECT + BLOCKS_SINGLY &&
       *            a < BLOCKS_DIRECT + BLOCKS_SINGLY + BLOCKS_DOUBLY)
       *   {
       *     //allocate the doubly typedef
       *     if(!root_nodes[i].doubly)
       *     {
       *       root_nodes[i].doubly = (u32int*)kmalloc(BLOCK_SIZE);
       * 
       *       //clear the doubly block of any junk
       *       memset(root_nodes[i].doubly, 0x0, BLOCK_SIZE);
       *     }
       *     //the doubly_offset is the offset of the doubly block's node pointing towards the singly block          
       *     u32int doubly_offset;
       *     doubly_offset = (a - (BLOCKS_DIRECT + BLOCKS_SINGLY)) / BLOCKS_SINGLY;
       * 
       *     //allocate the singly typedef
       *     if(!*(root_nodes[i].doubly + doubly_offset))
       *       *(root_nodes[i].doubly + doubly_offset) = kmalloc(BLOCK_SIZE);
       * 
       *     //if this file has a singly set, a doubly set, and a triply set
       *   }else if(a >= BLOCKS_DIRECT + BLOCKS_SINGLY + BLOCKS_DOUBLY&&
       *            a < BLOCKS_DIRECT + BLOCKS_SINGLY + BLOCKS_DOUBLY + BLOCKS_TRIPLY)
       *   {
       * 
       *     //allocate the triply typedef
       *     if(!root_nodes[i].triply)
       *       root_nodes[i].triply = (u32int*)kmalloc(BLOCK_SIZE);
       * 
       *     //disregard the other blocks
       *     u32int offset = a - (BLOCKS_DIRECT + BLOCKS_SINGLY + BLOCKS_DOUBLY);
       * 
       *     /\*trpily_offset is the offset in the triply block of the node pointing to the doubly block, 
       *      * the doubly_offset is the offset inside the doubly block of the triply block that points to the
       *      * singly block*\/
       * 
       *     u32int triply_offset = (offset) / BLOCKS_DOUBLY;
       *     u32int doubly_offset = (offset % BLOCKS_DOUBLY) / BLOCKS_DOUBLY;
       * 
       *     //allocate the doubly typedef
       *     if(!*(root_nodes[i].triply + triply_offset))
       *       *(root_nodes[i].triply + triply_offset) = kmalloc(BLOCK_SIZE);          
       * 
       *     //allocate the singly typedef
       *     if(!*((u32int*)*(root_nodes[i].triply + triply_offset) + doubly_offset))
       *       *((u32int*)*(root_nodes[i].triply + triply_offset) + doubly_offset) = kmalloc(BLOCK_SIZE);
       * 
       *   } */

      //~ k_printf("a is : %d, size %d\n", a, file_headers[i].length);
 
      //get the size of data to copy
      allocSize = blockSizeAtIndex(file_headers[i].length, a, 0);

      expand_node(&root_nodes[i], allocSize);

      //get the address of the block to write to
      block = (u32int)block_of_set(&root_nodes[i], a);

      /*The following examples assume that we are in the direct blocks of the node
       * the concept is the same for the indirect blocks
       *
       * (u32int*)block points to the address of the rootnodes[i].blocks[a]
       * e.g. &rootnodes[i].blocks[a]
       *
       * then the *(u32int*)block is the value of that address, i.e. it impersonates
       * that it (u32int block) is rootnodes[i].blocks[a], although
       * they are found at different memory locations, same concept with hard links */

      //allocate space for a block
      *(u32int*)block = kmalloc(BLOCK_SIZE);

      //copy the data to that block
      memcpy(*(u32int*)block, file_headers[i].offset + a * BLOCK_SIZE, allocSize);
    
    }
  }

  initrd_dev = vfs_createDirectory(initrd_root, "/");

  addHardLinkToDir(initrd_dev, initrd_dev, "."); //adds hardlink for root
  addHardLinkToDir(initrd_dev, initrd_dev, ".."); //adds hardlink for root

  setCurrentDir(initrd_dev);

  //create the /dev directory for the virtual files
  fs_node_t *dev = vfs_createDirectory(initrd_dev, "dev");

  //create and open the stdin, stdout, and stderr files and file descriptors
  fs_node_t *stdin = vfs_createFile(dev, "stdin", 1024);
  fs_node_t *stdout = vfs_createFile(dev, "stdout", 1024);
  fs_node_t *stderr = vfs_createFile(dev, "stderr", 1024);

  //create the /etc directory
  fs_node_t *etc = vfs_createDirectory(initrd_dev, "etc");

  //set the initial file descriptor
  initial_fdesc = (file_desc_t*)kmalloc(sizeof(file_desc_t));

  //set the mask for "rw", read and write
  initial_fdesc->permisions = FDESC_READ | FDESC_WRITE;
  initial_fdesc->node = stdin;
  initial_fdesc->next = 0;

  //open the std files, stdin has alreadt been open when setting the initial file_desc
  if(!f_open("stdout", dev, "rw") || !f_open("stderr", dev, "rw"))
  {
    //some sort of error
    kfree((void*)tempLoc);
    return 0;
  }

  //assigns all of the files that were in the file_headers to this dev directory
  for(i = 0; i < initrd_header->nfiles; i++)
    addFileToDir(initrd_dev, &root_nodes[i]);

  kfree((void*)tempLoc);

  return initrd_dev;
}
