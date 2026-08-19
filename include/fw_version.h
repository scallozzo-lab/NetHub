#ifndef __FW_VERSION__H__
    #define __FW_VERSION__H__

#include "main.h"


#define FW_VERSION_0            1
#define FW_VERSION_1            5
#define FW_VERSION_REV          'd'
#ifndef FW_TYPE
	#define FW_TYPE            '?'
#endif

#define _IDFLAG "@@ATLTX#" 
#define  FLASH_BASE_ADDRESS 	0x08000000LU	// Dirección absoluta del binario de boot
#define  FLASH_BASE_APP		 	0x08004000LU	// Dirección absoluta de NetHub como 2da app.
#define _FLASH_OFFSET_HEADER	0x010CLU		// Offset donde se encuentra el header de versión (después de la tabla de saltos)
#define  FLASH_MAX_LEN_FW   	49152LU 		//bytes

typedef struct __attribute__((packed)) {
	uint8_t id[8];
	uint8_t version[2];
	uint8_t rev;
	uint8_t fwtype[3];
	uint8_t lenid;
	uint8_t lenstr[8];
	uint8_t sigid;
	uint8_t sigstr[8];
}stfwverid;


stfwverid *Look4VersionId(uint8_t *pMem, uint32_t memlen);
extern const volatile uint8_t fw_ver_id[];
#endif
