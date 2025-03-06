#ifndef GMLAN_H
#define GMLAN_H

// constants for GMLAN message masking & shifting
#define GMLAN_PRI_MASK 0x1C000000 // priority is first 3 bits
#define GMLAN_PRI_SHIFT 0x1A // shift priority right 1A (26) bits, for example 0b1110101... becomes 0b111
#define GMLAN_ARB_MASK 0x03FFE000 // target arb id is next 13 bits
#define GMLAN_ARB_SHIFT 0x0D // shift target arb id right 0D (13) bits
#define GMLAN_SND_MASK 0x00001FFF // sender arb id is last 13 bits
#define GMLAN_SND_SHIFT 0x00 // no need to shift sender arb id

// helper macros to shift/decipher GMLAN messages
#define GMLAN_MASK_AND_SHIFT(v, m, s) ((v & m) >> s)
#define GMLAN_PRI(v) GMLAN_MASK_AND_SHIFT(v, GMLAN_PRI_MASK, GMLAN_PRI_SHIFT)
#define GMLAN_ARB(v) GMLAN_MASK_AND_SHIFT(v, GMLAN_ARB_MASK, GMLAN_ARB_SHIFT)
#define GMLAN_SND(v) GMLAN_MASK_AND_SHIFT(v, GMLAN_SND_MASK, GMLAN_SND_SHIFT)

// helper macros to build GMLAN masks or messages from parts
#define GMLAN_UNSHIFT_AND_MASK(v, m, s) ((v << s) & m)
#define GMLAN_R_PRI(v) GMLAN_UNSHIFT_AND_MASK(v, GMLAN_PRI_MASK, GMLAN_PRI_SHIFT)
#define GMLAN_R_ARB(v) GMLAN_UNSHIFT_AND_MASK(v, GMLAN_ARB_MASK, GMLAN_ARB_SHIFT)
#define GMLAN_R_SND(v) GMLAN_UNSHIFT_AND_MASK(v, GMLAN_SND_MASK, GMLAN_SND_SHIFT)

// known messages
// Rear Park Assist: typically 0x103A80BB or 0x??3A8???
#define GMLAN_MSG_PARK_ASSIST 0x1D4UL
// Outside Temperature: typically 0x10424060 or 0x??424???
#define GMLAN_MSG_TEMPERATURE 0x212UL

#endif //GMLAN_H
