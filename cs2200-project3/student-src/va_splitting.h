#pragma once

#include "mmu.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/**
 * --------------------------------- PROBLEM 1 --------------------------------------
 * Checkout PDF Section 3 For this Problem
 *
 * Split the virtual address into its virtual page number and offset.
 * 
 * HINT: 
 *      -Examine the global defines in pagesim.h, which will be necessary in 
 *      implementing these functions.
 * ----------------------------------------------------------------------------------
 */
static inline vpn_t vaddr_vpn(vaddr_t addr) {
    // TODO: return the VPN from virtual address addr.
    return (vpn_t)(addr >> OFFSET_LEN);//since removing offset by bit shifting
}

static inline uint16_t vaddr_offset(vaddr_t addr) {
    // TODO: return the offset into the frame from virtual address addr.
    return (uint16_t)(addr & ((1 << OFFSET_LEN)) - 1);//since removing vpn
    /**
     * testing
     * 
     * 0100 0111
     * 
     * 0000 0001
     * 0000 1000
     * 
     * 0100 0111
     * 0000 1000 - 1
     * 
     * 0100 0111
     * 0000 0111
     */
}

#pragma GCC diagnostic pop