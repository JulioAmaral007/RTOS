#include "mem.h"


/*********************************************************************
 * Function:        unsigned char * SRAMalloc(unsigned char length)
 *
 * PreCondition:    A memory block must be allocated in the linker, 
 *					and	the memory headers and tail must already be 
 *					set via the	function SRAMInitHeap().
 *
 * Input:        	unsigned char nBytes - Number of bytes to allocate.	
 *                  
 * Output:         	unsigned char * - A pointer to the requested block
 *					of memory.
 *
 * Side Effects:    
 *
 * Overview:        This functions allocates a chunk of memory from
 *					the heap. The maximum segment size for this 
 *					version is 126 bytes. If the heap does not have
 *					an available segment of sufficient size it will
 *					attempt to create a segment; otherwise a NULL 
 *					pointer is returned. If allocation is succeessful
 *					then a pointer to the requested block is returned.
 *
 * Note:            The calling function must maintain the pointer
 *					to correctly free memory at runtime.
 ********************************************************************/
unsigned char * NEAR SRAMalloc(NEAR unsigned char nBytes)
{
	SALLOC * NEAR pHeap;
	SALLOC * NEAR temp;
	NEAR SALLOC segHeader;
	NEAR unsigned char segLen;
	
	// Do not allow allocation above the max minus one bytes
	if (nBytes > (_MAX_SEGMENT_SIZE - 1)) return (0);
	
	// Init the pointer to the heap
	pHeap = (SALLOC *)_uDynamicHeap;
	
	while (1)
	{
		// Get the header of the segment
		segHeader = *pHeap;
		
		// Extract the segment length from the segment
		segLen = segHeader.bits.count - 1;

		// A null segment indicates the end of the table
		if (segHeader.byte == 0) return (0);
				
		// If this segment is not allocated then attempt to allocate it
		if (!(segHeader.bits.alloc))
		{
			// If the free segment is too small then attempt to merge
			if (nBytes > segLen)
			{
				// If the merge fails them move on to the next segment
				if (!(_SRAMmerge(pHeap))) pHeap += segHeader.bits.count;	
			}
			else
		
			// If the segment length matches the request then allocate the
			// header and return the pointer
			if (nBytes == segLen)
			{
				// Allocate the segment
				(*pHeap).bits.alloc = 1;  
					
				// Return the pointer to the caller
				return ((unsigned char *)(pHeap + 1));
			}
	
			// Else create a new segment
			else 
			{
				// Reset the header to point to a new segment
				(*pHeap).byte = nBytes + 0x81;
	
				// Remember the pointer to the first segment
				temp = pHeap + 1;
	
				// Point to the new segment
				pHeap += (nBytes + 1);
	
				// Insert the header for the new segment
				(*pHeap).byte = segLen - nBytes;  
					
				// Return the pointer to the user
				return ((unsigned char *) temp);
			}			
		}

		// else set the pointer to the next segment header in the heap
		else 
		{
			pHeap += segHeader.bits.count;
		}
	}
}



/*********************************************************************
 * Function:        void SRAMfree(unsigned char * pSRAM)
 *
 * PreCondition:  	The pointer must have been returned from a 
 *					previously allocation via SRAMalloc().  
 *
 * Input:        	unsigned char * pSRAM - pointer to the allocated 	
 *                  
 * Output:         	void
 *
 * Side Effects:    
 *
 * Overview:       	This function de-allocates a previously allocated
 *					segment of memory. 
 *
 * Note:            The pointer must be a valid pointer returned from
 *					SRAMalloc(); otherwise, the segment may not be 
 *					successfully de-allocated, and the heap may be 
 *					corrupted.
 ********************************************************************/
void SRAMfree(unsigned char * NEAR pSRAM)
{	
	// Release the segment
	(*(SALLOC *)(pSRAM - 1)).bits.alloc = 0;
}



/*********************************************************************
 * Function:        void SRAMInitHeap(void)
 *
 * PreCondition:    
 *
 * Input:  			void      	
 *                  
 * Output:         	void
 *
 * Side Effects:    
 *
 * Overview:      	This function initializes the dynamic heap. It 
 *					inserts segment headers to maximize segment space.
 *
 * Note:            This function must be called at least one time. 
 *					And it could be called more times to reset the 
 *					heap.
 ********************************************************************/
void SRAMInitHeap(void)
{
	unsigned char * NEAR pHeap;
	NEAR unsigned int count;
	
	pHeap = _uDynamicHeap;
	count = _MAX_HEAP_SIZE;

	while (1)
	{
		if (count > _MAX_SEGMENT_SIZE)
		{
			*pHeap = _MAX_SEGMENT_SIZE;
			pHeap += _MAX_SEGMENT_SIZE;
			count = count - _MAX_SEGMENT_SIZE;
		}
		else
		{
			*pHeap = count;
			*(pHeap + count) = 0;
			return;
		}
	}
}


/*********************************************************************
 * Function:        unsigned char _SRAMmerge(SALLOC * NEAR pSegA)
 *
 * PreCondition:    
 *
 * Input:      		SALLOC * NEAR pSegA - pointer to the first segment.  	
 *                  
 * Output:         	usnigned char - returns the length of the 
 *					merged segment or zero if failed to merge.
 *
 * Side Effects:    
 *
 * Overview:        This function tries to merge adjacent segments
 *					that have not been allocated. The largest possible
 *					segment is merged if possible.
 *
 * Note:            
 ********************************************************************/
NEAR unsigned char _SRAMmerge(SALLOC * NEAR pSegA)
{
	SALLOC * NEAR pSegB;
	NEAR SALLOC uSegA, uSegB, uSum;
	

	// Init the pointer to the heap
	pSegB = pSegA + (*pSegA).byte;
		
	// Extract the headers for faster processing
	uSegA = *pSegA;
	uSegB = *pSegB;
	
	// Quit if the tail has been found
	if (uSegB.byte == 0) return (0);
	
	// If either segment is allocated then do not merge
	if (uSegA.bits.alloc || uSegB.bits.alloc) return (0);
		
	// If the first segment is max then nothing to merge
	if (uSegA.bits.count == _MAX_SEGMENT_SIZE) return (0);
		
	// Get the sum of the two segments		
	uSum.byte = uSegA.byte + uSegB.byte;	
		
		
	// If the sum of the two segments are > than the largest segment
	// then create a new segment equal to the max segment size and
	// point to the next segments			
	if ((uSum.byte) > _MAX_SEGMENT_SIZE)
	{
		(*pSegA).byte = _MAX_SEGMENT_SIZE;
		pSegA += _MAX_SEGMENT_SIZE; //(*pSeg1).byte;  
		pSegB += uSegB.byte; //(*pSeg2).byte ;   
		(*pSegA).byte = pSegB - pSegA;
		
		return (_MAX_SEGMENT_SIZE);
	}
	// Else combine the two segments into one segment and
	// do not adjust the pointers to the next segment
	else
	{
		return ((*pSegA).byte = uSum.byte);
	}
}
