/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#include <linux/slab.h>
#else
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */

    if(buffer == NULL || entry_offset_byte_rtn == NULL)
        return NULL;

    //If buffer is empty return NULL
    if((buffer->in_offs ==  buffer->out_offs) && (buffer->full == false))
    {
        return NULL;
    }

    size_t find = char_offset;
    printf("The char offset to find is %ld\r\n",find);

    uint8_t length = buffer->out_offs - buffer->in_offs;
    
    if(length == 0)
        if(buffer->full == true)
            length = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - 1;
    
    printf("The length is %d \r\n",length);

    //offset shouldn't be greater than total length
    int tracker =0;
    size_t total_size;
    while(tracker < length)
    {
        total_size += buffer->entry[tracker++].size;
        tracker++;
    }

    printf("The total size is %ld\r\n", total_size);
    if(find>total_size)
        return NULL;

    size_t trace = 0;
    int i= buffer->out_offs;
    int track_len = 0;

    printf("\r\nbefore while\r\n");
    while((track_len <= (length)) && (trace<find) )
    {
        trace += buffer->entry[i].size;
        printf("The trace is %ld \r\n", trace);
        if(i == (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - 1))
            i = 0;
        else 
            i++;

        printf("The new i is %d\r\n", i); 
        track_len++;

    };
    printf("\r\nafter while\r\n");
    // Print buffer entry 
    printf("The buffer entry here is %s",buffer->entry[i].buffptr);
    //what we want is trace - find
     *entry_offset_byte_rtn = trace - find;
     return &(buffer->entry[i]);  
   // return &(buffer->entry[i].buffptr(buffer->entry[i].size - (trace-find)));     
}


//out+offs  
//in+offs

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
int aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description
    */
    
    if(buffer == NULL || add_entry == NULL || add_entry->size == 0 || add_entry->buffptr == NULL)
        return -1;

    //else we can add doesn't matter if full or not 
   
    buffer->entry[buffer->in_offs].buffptr = add_entry->buffptr;
    printf("Entry is %s\r\n", add_entry->buffptr);

    printf("AAdded Entry is %s\r\n",  buffer->entry[buffer->in_offs].buffptr);
    buffer->entry[buffer->in_offs].size = add_entry->size; 
    printf("Added at offset %d\r\n", buffer->in_offs);  

    if(buffer->full == false)
    {
        if(buffer->in_offs == (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - 1))
            buffer->in_offs  = 0;
        else
            buffer->in_offs += 1;

        printf("New in_offs is %d\r\n", buffer->in_offs);
        printf("New out_offs is %d\r\n", buffer->out_offs);

        if(buffer->in_offs == buffer->out_offs)
            buffer->full = true;

        return 0;
    }
    else
    {
        //buffer is full
        if(buffer->in_offs == (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - 1))
            buffer->in_offs = 0;
        else
            buffer->in_offs += 1;
        
        printf("New in_offs is %d\r\n", buffer->in_offs);

        if(buffer->out_offs == (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - 1))
            buffer->out_offs = 0;
        else
            buffer->out_offs += 1;

        printf("New out_offs is %d\r\n", buffer->out_offs);
       
        return 0;
    }

}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}

