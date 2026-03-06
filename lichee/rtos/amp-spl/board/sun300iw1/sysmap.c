#include <include.h>
#include "internal.h"

typedef struct {
    volatile uint32_t SYSMAPADDR0;             /*!< Offset: 0x000 (R/W)  SYSMAP configure register */
    volatile uint32_t SYSMAPCFG0;              /*!< Offset: 0x004 (R/W)  SYSMAP configure register */
    volatile uint32_t SYSMAPADDR1;             /*!< Offset: 0x008 (R/W)  SYSMAP configure register */
    volatile uint32_t SYSMAPCFG1;              /*!< Offset: 0x00c (R/W)  SYSMAP configure register */
    volatile uint32_t SYSMAPADDR2;             /*!< Offset: 0x010 (R/W)  SYSMAP configure register */
    volatile uint32_t SYSMAPCFG2;              /*!< Offset: 0x014 (R/W)  SYSMAP configure register */
    volatile uint32_t SYSMAPADDR3;             /*!< Offset: 0x018 (R/W)  SYSMAP configure register */
    volatile uint32_t SYSMAPCFG3;              /*!< Offset: 0x01c (R/W)  SYSMAP configure register */
    volatile uint32_t SYSMAPADDR4;             /*!< Offset: 0x020 (R/W)  SYSMAP configure register */
    volatile uint32_t SYSMAPCFG4;              /*!< Offset: 0x024 (R/W)  SYSMAP configure register */
    volatile uint32_t SYSMAPADDR5;             /*!< Offset: 0x028 (R/W)  SYSMAP configure register */
    volatile uint32_t SYSMAPCFG5;              /*!< Offset: 0x02c (R/W)  SYSMAP configure register */
    volatile uint32_t SYSMAPADDR6;             /*!< Offset: 0x030 (R/W)  SYSMAP configure register */
    volatile uint32_t SYSMAPCFG6;              /*!< Offset: 0x034 (R/W)  SYSMAP configure register */
    volatile uint32_t SYSMAPADDR7;             /*!< Offset: 0x038 (R/W)  SYSMAP configure register */
    volatile uint32_t SYSMAPCFG7;              /*!< Offset: 0x03c (R/W)  SYSMAP configure register */
} sysmap_type;

#define SYSMAP_BASE       (0x3FFFF000UL)                            /*!< SYSMAP Base Address */
#define SYSMAP            ((sysmap_type*) SYSMAP_BASE)

#define SYSMAP_SYSMAPCFG_B_Pos                       0U                                    /*!< SYSMAP SYSMAPCFG: B Position */
#define SYSMAP_SYSMAPCFG_B_Msk                       (0x1UL << SYSMAP_SYSMAPCFG_B_Pos)     /*!< SYSMAP SYSMAPCFG: B Mask */

#define SYSMAP_SYSMAPCFG_C_Pos                       1U                                    /*!< SYSMAP SYSMAPCFG: C Position */
#define SYSMAP_SYSMAPCFG_C_Msk                       (0x1UL << SYSMAP_SYSMAPCFG_C_Pos)     /*!< SYSMAP SYSMAPCFG: C Mask */

#define SYSMAP_SYSMAPCFG_SO_Pos                      2U                                    /*!< SYSMAP SYSMAPCFG: SO Position */
#define SYSMAP_SYSMAPCFG_SO_Msk                      (0x1UL << SYSMAP_SYSMAPCFG_SO_Pos)    /*!< SYSMAP SYSMAPCFG: SO Mask */


void set_sysmapcfg(uint32_t idx, uint32_t sysmapxcfg)
{
    switch (idx) {
        case 0: SYSMAP->SYSMAPCFG0 = sysmapxcfg; break;
        case 1: SYSMAP->SYSMAPCFG1 = sysmapxcfg; break;
        case 2: SYSMAP->SYSMAPCFG2 = sysmapxcfg; break;
        case 3: SYSMAP->SYSMAPCFG3 = sysmapxcfg; break;
        case 4: SYSMAP->SYSMAPCFG4 = sysmapxcfg; break;
        case 5: SYSMAP->SYSMAPCFG5 = sysmapxcfg; break;
        case 6: SYSMAP->SYSMAPCFG6 = sysmapxcfg; break;
        case 7: SYSMAP->SYSMAPCFG7 = sysmapxcfg; break;
        default: return;
    }
}

void set_sysmapaddr(uint32_t idx, uint32_t sysmapxaddr)
{
    switch (idx) {
        case 0: SYSMAP->SYSMAPADDR0 = sysmapxaddr; break;
        case 1: SYSMAP->SYSMAPADDR1 = sysmapxaddr; break;
        case 2: SYSMAP->SYSMAPADDR2 = sysmapxaddr; break;
        case 3: SYSMAP->SYSMAPADDR3 = sysmapxaddr; break;
        case 4: SYSMAP->SYSMAPADDR4 = sysmapxaddr; break;
        case 5: SYSMAP->SYSMAPADDR5 = sysmapxaddr; break;
        case 6: SYSMAP->SYSMAPADDR6 = sysmapxaddr; break;
        case 7: SYSMAP->SYSMAPADDR7 = sysmapxaddr; break;
        default: return;
    }
}

void sysmap_config_region(uint32_t idx, uint32_t base_addr, uint32_t attr)
{
    uint32_t addr = 0;

    if (idx > 7) {
        return;
    }

    addr = base_addr >> 12;
    attr = attr << 2;

    set_sysmapaddr(idx, addr);
    set_sysmapcfg(idx, attr);
}

void sysmap_init(void)
{
	sysmap_config_region(0, 0x10000000, SYSMAP_SYSMAPCFG_C_Msk | SYSMAP_SYSMAPCFG_B_Msk); /* rom & sram */
	sysmap_config_region(1, 0x12000000, SYSMAP_SYSMAPCFG_C_Msk | SYSMAP_SYSMAPCFG_B_Msk); /* flash XIP maybe unused */
	sysmap_config_region(2, 0x30000000, SYSMAP_SYSMAPCFG_SO_Msk); /* reserved */
	sysmap_config_region(3, 0x40000000, SYSMAP_SYSMAPCFG_SO_Msk); /* tcip register */
	sysmap_config_region(4, 0x68000000, SYSMAP_SYSMAPCFG_SO_Msk); /* peripheral */
	sysmap_config_region(5, 0x69000000, SYSMAP_SYSMAPCFG_C_Msk | SYSMAP_SYSMAPCFG_B_Msk); /* wifi sram */
	sysmap_config_region(6, 0x80000000, SYSMAP_SYSMAPCFG_SO_Msk); /* wifi peripheral */
	sysmap_config_region(7, 0xFFFFFFFF, SYSMAP_SYSMAPCFG_C_Msk | SYSMAP_SYSMAPCFG_B_Msk); /* dram */
}
