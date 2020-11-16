/*
 * @Description: 
 * @Author: Wangzhe
 * @Date: 2020-11-09 13:08:18
 * @LastEditors: Wangzhe
 * @LastEditTime: 2020-11-14 21:29:26
 * @FilePath: \0_Desktop\Pro\FD_\src\main.h
 */
#ifndef __MAIN_H_
#define __MAIN_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <malloc.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdbool.h>

typedef enum {
    STM32,
    WINDOWS
} Process_status;

typedef enum {
    STM32_FIRST,
    STM32_NORMAL_STATUS
} STM32_status;

typedef enum {
    STM32_LOAD,
    STM32_INIT,
    STM32_QUANTIZE,
    STM32_ENCODING,
    STM32_WRITE_NVM
} STM32_normal_status;

typedef enum {
    WIN_FIRST,
    WIN_NORMAL_STATUS
} WIN_status;

typedef enum {
    WIN_LOAD,
    WIN_DECODE,
    WIN_EVALUATE,
    WIN_UPDATE_TABLE
} WIN_normal_status;


#define LOG 0



typedef int uintA_t;
typedef int uintB_t;
typedef int uint1_t;
typedef int uintn_t;

#define UNFIXED_LEN 4
#define DEFAULT_QUANTIZE_BIT 9

#define A 4
#define B DEFAULT_QUANTIZE_BIT


#define DATA_ONE_LINE_BYTE_SIZE 8



#define PAGE_SIZE 256              // bytes 
#define HEADER_SIZE 1              // base page 
#define HUFFMAN_TABLE_SIZE 20      // base page 

#define NVM_HEADER_BASEADDR 0
#define NVM_TABLE_BASEADDR  (HEADER_SIZE * PAGE_SIZE) 
#define NVM_DATA_BASEADDR   ((HEADER_SIZE + HUFFMAN_TABLE_SIZE) * PAGE_SIZE) 

#define NVM_ENDADDR  (1 << 21) // 2MB, enough one million testbench size, (2 * 1024 * 1024 * 8) / 13 > 1e6

#define NVM_DATA_PAGE_NUM ((NVM_ENDADDR - NVM_DATA_BASEADDR) / PAGE_SIZE) 
#define NVM_ACT_PAGE_NUM (NVM_ENDADDR / PAGE_SIZE) 

#define NVM_TOTAL_BIT (NVM_ENDADDR * 8) // 8 Mb

#define LOAD_SIZE 10000 // 结合MCU的SRAM，适量


extern int recursionFlg;


#endif // __MAIN_H_
