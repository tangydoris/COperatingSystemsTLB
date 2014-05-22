/*Doris Tang
Operating Systems
Professor Goldberg
due November 25, 2013
*/

#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "tlb.h"
#include "cpu.h"
#include "mmu.h"

/* This is some of the code that I wrote. You may use any of this code
   you like, but you certainly don't have to.
*/

/* I defined the TLB as an array of entries,
   each containing the following:
   Valid bit: 1 bit
   Virtual Page: 20 bits
   Modified bit: 1 bit
   Reference bit: 1 bit
   Page Frame: 20 bits (of which only 18 are meaningful given 1GB RAM)
*/

//You can use a struct to get a two-word entry.
typedef struct {
  unsigned int vbit_and_vpage;  // 32 bits containing the valid bit and the 20bit
                                // virtual page number.
  unsigned int mr_pframe;       // 32 bits containing the modified bit, reference bit,
                                // and 20-bit page frame number
} TLB_ENTRY;



// This is the actual TLB array. It should be dynamically allocated
// to the right size, depending on the num_tlb_entries value 
// assigned when the simulation started running.

TLB_ENTRY *tlb;  

// This is the TLB size (number of TLB entries) chosen by the 
// user. 

unsigned int num_tlb_entries;

  //Since the TLB size is a power of 2, I recommend setting a
  //mask to perform the MOD operation (which you will need to do
  //for your TLB entry evicition algorithm, see below).
unsigned int mod_tlb_entries_mask;

//this must be set to TRUE when there is a tlb miss, FALSE otherwise.
BOOL tlb_miss; 


//If you choose to use the same representation of a TLB
//entry that I did, then these are masks that can be used to 
//select the various fields of a TLB entry.

#define VBIT_MASK   0x80000000  //VBIT is leftmost bit of first word
#define VPAGE_MASK  0x000FFFFF            //lowest 20 bits of first word
#define RBIT_MASK   0x80000000  //RIT is leftmost bit of second word
#define MBIT_MASK   0x40000000  //MBIT is second leftmost bit of second word
#define PFRAME_MASK 0x000FFFFF            //lowest 20 bits of second word


// Initialize the TLB (called by the mmu)
void tlb_initialize()
{
  //Here's how you can allocate a TLB of the right size
  tlb = (TLB_ENTRY *) malloc(num_tlb_entries * sizeof(TLB_ENTRY));

  //This is the mask to perform a MOD operation (see above)
  mod_tlb_entries_mask = num_tlb_entries - 1;  

  //Fill in rest here...
  tlb_miss = 0;
  
  //initialize bits in tlb array
  int i;
  for(i = 0; i < num_tlb_entries; i++){
    //set valid, reference an modified bits to 0
    tlb[i].vbit_and_vpage = 0x0;
    tlb[i].mr_pframe = 0x0;
  }
}

// This clears out the entire TLB, by clearing the
// valid bit for every entry.
void tlb_clear_all() 
{
  // FILL THIS IN
  int i;
  for(i = 0; i < num_tlb_entries; i++){
    tlb[i].vbit_and_vpage = (tlb[i].vbit_and_vpage & ~VBIT_MASK);
  }

  tlb_miss = 0;
}


//clears all the R bits in the TLB
void tlb_clear_all_R_bits() 
{
  // FILL THIS IN
  int i;
  for(i = 0; i < num_tlb_entries; i++){
    tlb[i].mr_pframe = (tlb[i].mr_pframe & ~RBIT_MASK);
  }
}

// This clears out the entry in the TLB for the specified
// virtual page, by clearing the valid bit for that entry.
void tlb_clear_entry(VPAGE_NUMBER vpage) {
  // FILL THIS IN
  int i;
  for(i = 0; i < num_tlb_entries; i++){
    if((tlb[i].vbit_and_vpage & VPAGE_MASK) == vpage)
      tlb[i].vbit_and_vpage = (tlb[i].vbit_and_vpage & ~VBIT_MASK);
  }
}

// Returns a page frame number if there is a TLB hit. If there is a TLB
// miss, then it sets tlb_miss (see above) to TRUE.  It sets the R
// bit of the entry and, if the specified operation is a STORE,
// sets the M bit.

PAGEFRAME_NUMBER tlb_lookup(VPAGE_NUMBER vpage, OPERATION op)
{
  // FILL THIS IN
  int i;
  for(i = 0; i < num_tlb_entries; i++){
    if((tlb[i].vbit_and_vpage & VPAGE_MASK) == vpage){
      //set R bit of entry
      tlb[i].mr_pframe = (tlb[i].mr_pframe | RBIT_MASK);
      if(op == STORE)
	//set M bit of entry
	tlb[i].mr_pframe = (tlb[i].mr_pframe | MBIT_MASK);
      return (PAGEFRAME_NUMBER)(tlb[i].mr_pframe & PFRAME_MASK);
    }
  }
  
  //virtual page not found after iterating through tlb array
  tlb_miss = 1;
  return;
}

// Uses an NRU clock algorithm, where the first entry with
// either a cleared valid bit or cleared R bit is chosen.

int clock_hand = 0;  // points to next TLB entry to consider evicting


void tlb_insert(VPAGE_NUMBER new_vpage, PAGEFRAME_NUMBER new_pframe,
		BOOL new_mbit, BOOL new_rbit)
{

  // Starting at the clock_hand'th entry, find first entry to
  // evict with either valid bit  = 0 or the R bit = 0.
  //If there is no such entry, then just evict the entry
  //pointed to by the clock hand.
  int i;
  for(i = 0; i <= num_tlb_entries; i++){
    if((((tlb[clock_hand].mr_pframe & RBIT_MASK) >> 31) == 0) &&
       (((tlb[clock_hand].mr_pframe & MBIT_MASK) >> 30) == 0)){
      tlb[clock_hand].vbit_and_vpage = (tlb[clock_hand].vbit_and_vpage & ~VBIT_MASK);
      break;
    }
    else{
      clock_hand++;
      clock_hand = (clock_hand & mod_tlb_entries_mask);
    }
  }

  // Then, if the entry to evict has a valid bit = 1,
  // write the M and R bits of the of entry back to the M and R
  // bitmaps, respectively, in the MMU (see mmu_modify_rbit_bitmap, etc.
  // in mmu.h)
  if(tlb[clock_hand].vbit_and_vpage & VBIT_MASK){
    mmu_modify_rbit_bitmap(tlb[clock_hand].mr_pframe & PFRAME_MASK, (tlb[clock_hand].mr_pframe & RBIT_MASK) >> 31);
    mmu_modify_mbit_bitmap(tlb[clock_hand].mr_pframe & PFRAME_MASK, (tlb[clock_hand].mr_pframe & MBIT_MASK) >> 30);
  }

  // Then, insert the new vpage, pageframe, M bit, and R bit into the
  // TLB entry that was just found (and possibly evicted).
  tlb[clock_hand].vbit_and_vpage = (tlb[clock_hand].vbit_and_vpage | new_vpage);
  tlb[clock_hand].mr_pframe = (tlb[clock_hand].mr_pframe | new_pframe);
  if(new_mbit)
    tlb[clock_hand].mr_pframe = (tlb[clock_hand].mr_pframe | (1 << 30));
  if(new_rbit)
    tlb[clock_hand].mr_pframe = (tlb[clock_hand].mr_pframe | (1 << 31));

  // Finally, set clock_hand to point to the next entry after the
  // entry found above.
  clock_hand++;
  clock_hand = (clock_hand & mod_tlb_entries_mask);
}


//Writes the M  & R bits in the each valid TLB
//entry back to the M & R MMU bitmaps.
void tlb_write_back()
{
  //FILL THIS IN
  int i;
  for(i = 0; i < num_tlb_entries; i++){
    if(tlb[i].vbit_and_vpage & VBIT_MASK){
      mmu_modify_rbit_bitmap((tlb[i].mr_pframe & PFRAME_MASK), ((tlb[i].mr_pframe & RBIT_MASK) >> 31));
      mmu_modify_mbit_bitmap((tlb[i].mr_pframe & PFRAME_MASK), ((tlb[i].mr_pframe & MBIT_MASK) >> 30));
    }
  }
}

