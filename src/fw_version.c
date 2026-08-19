#include "fw_version.h"

    const volatile uint8_t fw_ver_id[32] __attribute__((section(".fw_header"))) = {
        '@','@','A','T','L','T','X','#', 
        (uint8_t)FW_VERSION_0 + 0x30, 
        (uint8_t)FW_VERSION_1 + 0x30, 
        (uint8_t)FW_VERSION_REV,
        'F','W', (uint8_t)FW_TYPE,
        '#','_','_','_','_','_','_','_','_',    // Longitud en ASCII-HEXA 8  
        '#','_','_','_','_','_','_','_','_'     // Signature en ASCII-HEXA 8 
};



