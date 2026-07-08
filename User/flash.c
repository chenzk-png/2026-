#include "debug.h"
#include "flash.h"
// CH32V307 256KB Flash 最后一页起始地址
#define SAVE_ADDR 0x0803FC00 

// 存一个32位变量
void Save_Data(uint32_t data) {
    FLASH_Unlock();
    FLASH_ErasePage(SAVE_ADDR);
    FLASH_ProgramWord(SAVE_ADDR, data);
    FLASH_Lock();
}

// 读一个32位变量
uint32_t Read_Data(void) {
    return *(volatile uint32_t *)SAVE_ADDR;
}