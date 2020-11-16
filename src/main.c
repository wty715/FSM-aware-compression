/*
 * @Description: 
 * @Author: Wangzhe
 * @Date: 2020-10-26 20:26:25
 * @LastEditors: Wangzhe
 * @LastEditTime: 2020-11-14 22:21:48
 * @FilePath: \Pro\FD_\src\main.c
 */

#include "main.h"
#include "Huffman.h"


#define DEBUG_FILE 0





typedef struct U {
    uintA_t MS;
    uintB_t DS;
}U;

union RMS {
    uintA_t MS;
    int len;
};

union RDS {
    uintA_t DS;
    int len;
};

typedef struct Pair {
    int first;
    int second;
}Pair;

int quantize_bit;
//char input_name[233] = "d:\\0_Desktop\\Pro\\FD_\\src\\in.txt";
char input_name[233];

int _B;

#define TABLE_MAX_SIZE    1000000
#define WINDOWS_BUFF_SIZE 2500000


char* MSST[1 << A + 10];
char* LMSST[TABLE_MAX_SIZE + 10]; // 取决于LMS中的最大值，姑且为表最大长度
char* MDST[1 << B + 10];
char* LMDST[TABLE_MAX_SIZE + 10];
char* tempTable[TABLE_MAX_SIZE + 10];

Pair MSSTCnt;
Pair LMSSTCnt;
Pair MDSTCnt;
Pair LMDSTCnt;


int recursionFlg;
char zero = '\0';
int addr;

int g_readline;
U u[LOAD_SIZE + 10];

uint8_t NVM[NVM_ACT_PAGE_NUM * PAGE_SIZE];
// 512KB,裸放的话能放32W点，压缩按照50W的话，一次buff按照5倍左右。

U win_buff[WINDOWS_BUFF_SIZE + 10];
uintA_t MSS[WINDOWS_BUFF_SIZE];
uintA_t LMSS[WINDOWS_BUFF_SIZE];
uintA_t DSS[WINDOWS_BUFF_SIZE];
uintA_t LDSS[WINDOWS_BUFF_SIZE];

int win_buff_idx = 0;
int RMSLen;
int RDSLen;


int _max(int a, int b) {
    if (a > b) return a;
    return b;
}


int readRawData(int beg, int len) {
    FILE* fp = fopen(input_name, "rb");
    fseek(fp, DATA_ONE_LINE_BYTE_SIZE * beg, SEEK_SET);
    //printf("now pos = %d\n", ftell(fp));
    memset(u, 0, sizeof u);
    if (!fp) return -1;
    int i = 0;
#if DEBUG_FILE
    FILE* fp1 = fopen("real_data_raw_raw.txt","a+");
#endif // DEBUG_FILE
    while (i < len) {
        if (feof(fp)) break;
        fread(&u[i].MS, sizeof(int), 1, fp);
        if (feof(fp)) break;
        fread(&u[i].DS, sizeof(int), 1, fp);
        if (feof(fp)) break;
        u[i].DS /= (1 << (9 - quantize_bit));
#if DEBUG_FILE
        fprintf(fp1, "%d %d\n", u[i].MS, u[i].DS);
#endif // DEBUG_FILE
        i++;
    }
    fclose(fp);
#if DEBUG_FILE
    fclose(fp1);
#endif // DEBUG_FILE
    u[i].MS = u[i].DS = -1;  // ending flag
    return i;
}

void NVM_write_bit(int addr, int len, int data) {
    int byte_num  = addr >> 3;
    int start_bit = addr & 0x7;
    int remain    = 8 - start_bit;
    if (remain >= len) { // write data directly
        for (int i = 0; i < len; ++i) {
            int now_bit = (data & (1 << i)) > 0;
            NVM[byte_num] |= (now_bit << (start_bit + i));
        }
    } else {
        for (int i = 0; i < remain; ++i) {
            int now_bit = (data & (1 << i)) > 0;
            NVM[byte_num] |= (now_bit << (start_bit + i));
        }
        for (int i = remain; i < len; ++i) {
            int now_bit = (data & (1 << i)) > 0;
            NVM[byte_num + 1] |= (now_bit << (i - remain));
        }
    }
}

void STM32_first_write() {
    int write_NVM_addr_bit = 0;
    bool flg = false;
    while (write_NVM_addr_bit < NVM_TOTAL_BIT) {
        int readline = readRawData(g_readline, LOAD_SIZE);
        if (readline != LOAD_SIZE) {
            printf("readline = %d  ==>  end of file\n", readline);
            break;
        }
        if (write_NVM_addr_bit + readline * 13 > NVM_TOTAL_BIT) {
            break;
        }
#if DEBUG_FILE
        FILE* fp = fopen("real_data_in.txt","a+");
#endif // DEBUG_FILE
        for (int i = 0; i < LOAD_SIZE; ++i) {
            NVM_write_bit(write_NVM_addr_bit, 4, u[i].MS);
            write_NVM_addr_bit += 4;
            NVM_write_bit(write_NVM_addr_bit, 9, u[i].DS);
            write_NVM_addr_bit += 9;
#if DEBUG_FILE
            fprintf(fp, "%d %d\n", u[i].MS, u[i].DS);
#endif // DEBUG_FILE
        }
        g_readline += LOAD_SIZE;
#if DEBUG_FILE
        fclose(fp);
#endif // DEBUG_FILE
    }
    printf("write_sum_line = %d\n", g_readline);
    printf("write_NVM_addr_bit = %d\n", write_NVM_addr_bit);
}

int NVM_read_bit(int addr, int len) {
    int byte_num  = addr >> 3;
    int start_bit = addr & 0x7;
    int remain    = 8 - start_bit;
    int ret = 0;
    if (remain >= len) { // write data directly
        for (int i = 0; i < len; ++i) {
            int now_bit = (NVM[byte_num] & (1 << (i + start_bit))) > 0;
            ret |= (now_bit << i);
        }
    } else {
        for (int i = 0; i < remain; ++i) {
            int now_bit = (NVM[byte_num] & (1 << (i + start_bit))) > 0;
            ret |= (now_bit << i);
        }
        for (int i = remain; i < len; ++i) {
            int now_bit = (NVM[byte_num + 1] & (1 << (i - remain))) > 0;
            ret |= (now_bit << i);
        }
    }
    return ret;
}

/**
 * @description: 获得RMS和RDS中的每一个R_i
 * @param {type} 
 * @return {type} 
 */
void genRMDS(int begin, int end) {
    int mLen = 0;
    int dLen = 0;
    RMSLen = RDSLen = 0;
    uintA_t preMS = win_buff[begin].MS;
    uintB_t preDS = win_buff[begin].DS;

    memset(MSS, 0, sizeof MSS);
    memset(LMSS, 0, sizeof LMSS);
    memset(DSS, 0, sizeof DSS);
    memset(LDSS, 0, sizeof LDSS);

    // 防止 end 超过范围。
    int temp_MS = win_buff[end % WINDOWS_BUFF_SIZE].MS;
    int temp_DS = win_buff[end % WINDOWS_BUFF_SIZE].DS;

    // 最后一个可以识别到
    win_buff[end % WINDOWS_BUFF_SIZE].MS = win_buff[end % WINDOWS_BUFF_SIZE].DS = -1;

    for (int i = begin + 1; i <= end; ++i) {
        if (i >= WINDOWS_BUFF_SIZE) {
            i -= WINDOWS_BUFF_SIZE;
        }
        if (win_buff[i].MS != preMS) {
            MSS[RMSLen] = preMS;
            LMSS[RMSLen] = mLen + 1;
            RMSLen++;
            mLen = 0;
        } else mLen++;
        preMS = win_buff[i].MS;

        if (win_buff[i].DS != preDS) {
            DSS[RDSLen] = preDS;
            LDSS[RDSLen] = dLen + 1;
            RDSLen++;
            dLen = 0;
        } else dLen++;
        preDS = win_buff[i].DS;
    }
    // 还原之前的状态
    win_buff[end % WINDOWS_BUFF_SIZE].MS = temp_MS;
    win_buff[end % WINDOWS_BUFF_SIZE].DS = temp_DS;
}




/**
 * @description: 将输入的table数组，进行哈夫曼编码，结果存在全局变量 tempTable 中。编码过程如下
 *               1. 将初始数组中的元素逐个插入到链表head中，head自动进行升序排序
 *               2. 将排序好的链表，进行两两合并，最终到一个节点记为根节点root
 *               3. 根据归并好的哈夫曼树，进行先序遍历获得每一个节点的编码。
 *               4. 清空 tempTable 数组，因为要求好几次编码
 *               5. 遍历节点，打表返回
 * @param {type} 
 * @return {type} 
 */
void GetHuffmanEncoding(int* table, int len) {
    // puts("1 NOW!!!");
    node *head = NULL, *root = HuffmanNewNode(0), *tableHead = HuffmanNewNode(0);
    // 1. 将初始数组中的元素逐个插入到链表head中，head自动进行升序排序
    for (int i = 0; i < len; ++i) {
        HuffmanAddNode(&head, table[i]);
    }

    // node *ff = head;
    // while (ff) {
    //     printf("x = %d, freq = %d, type = %d\n", ff->x, ff->freq, ff->type);
    //     ff = ff->next;
    // }


    // puts("2 NOW!!!");
    // 2. 将排序好的链表，进行两两合并，最终到一个节点记为根节点root
    HuffmanMakeTree(head, &root);
    // puts("3 NOW!!!");
#if LOG
    printf("root->x = %d, root->frq = %d\n", root->x, root->freq);
    printf("head->x = %d, head->frq = %d\n", head->x, head->freq);
#endif
    
    // 3. 根据归并好的哈夫曼树，进行先序遍历获得每一个节点的编码。
    recursionFlg = 0;

    // puts("123 NOW!!!");

    HuffmanGenCode(&tableHead, root, &zero);

    // 4. 清空 tempTable 数组，因为要求好几次编码
    for (int i = 0; i < TABLE_MAX_SIZE; ++i) {
        if (tempTable[i] != NULL) {
            free(tempTable[i]);
            tempTable[i] = NULL;
        }
    }

    // 5. 遍历节点，打表返回
    node* p = tableHead;
    while (p) {
        int len = strlen(p->code);
        tempTable[p->x] = (char*)malloc(sizeof(char) * (strlen(p->code) + 1));
        memcpy(tempTable[p->x], p->code, strlen(p->code) + 1);
#if LOG
        printf("[%4d | %4d | %s\n", p->x, p->freq, p->code);
        printf("[%s] [len = %d]\n", tempTable[p->x], strlen(tempTable[p->x]));
#endif
        p = p->next;
    }
}


/**
 * @description: 考虑需要生成4个表，封装了个copy，遍历表中不为NULL的节点
 * @param {type} 
 * @return {int} 返回表中节点的个数 
 */
Pair CopyTable(char* list[], int len) {
    Pair ret = {0,0};
    for (int i = 0; i <= len; ++i) {
        list[i] = NULL;
        if (tempTable[i] != NULL) {
            ret.first++;
            ret.second += strlen(tempTable[i]);
            // printf("now str %d = %s\n", i, tempTable[i]);
            list[i] = (char*)malloc(strlen(tempTable[i]) + 1);
            memcpy(list[i], tempTable[i], strlen(tempTable[i]) + 1);
        }
    }
    return ret;
}


int getUnFixedLen(int x) {
    if (x == 0) return UNFIXED_LEN;
    int real_bit_len = (int)(log2(x)) + 1;
    int ret = 0;
    if (real_bit_len % (UNFIXED_LEN - 1) == 0) {
        ret =  (real_bit_len / (UNFIXED_LEN - 1)) * UNFIXED_LEN;
    } else {
        ret =  (real_bit_len / (UNFIXED_LEN - 1)) * UNFIXED_LEN + UNFIXED_LEN;
    }
    return ret;
}

void WIN_first_read() {
    // puts("WIN_first_read");
    int read_NVM_addr_bit = 0;
#if DEBUG_FILE
    FILE* fp = fopen("real_data_out.txt","a+");
#endif // DEBUG_FILE
    for (int i = 0; i < g_readline; ++i) {
        win_buff[win_buff_idx].MS = NVM_read_bit(read_NVM_addr_bit, 4); 
        read_NVM_addr_bit += 4;
        win_buff[win_buff_idx++].DS = NVM_read_bit(read_NVM_addr_bit, 9); 
#if DEBUG_FILE
        fprintf(fp, "%d %d\n", win_buff[win_buff_idx - 1].MS, win_buff[win_buff_idx - 1].DS);
#endif // DEBUG_FILE

        read_NVM_addr_bit += 9;
        if (win_buff_idx == WINDOWS_BUFF_SIZE) {
            win_buff_idx = 0;
        }
    }
#if DEBUG_FILE
    fclose(fp);
#endif // DEBUG_FILE
    genRMDS(0, g_readline);
    puts("genRMDS Finished");
    // int MSFixedLen = 0;
    // int MDFixedLen = 0;

    // puts("================");
    // for (int i = 0; i < 10; ++i) {
    //     printf("MSS[%d] = %d\n", i, MSS[i]);
    // }
    // puts("================");
    // for (int i = 0; i < 10; ++i) {
    //     MSFixedLen = _max(MSFixedLen, LMSS[i]);
    //     printf("LMSS[%d] = %d\n", i, LMSS[i]);
    // }
    // puts("================");
    // for (int i = 0; i < 10; ++i) {
    //     printf("DSS[%d] = %d\n", i, DSS[i]);
    // }
    // puts("================");
    // for (int i = 0; i < 10; ++i) {
    //     MDFixedLen = _max(MDFixedLen, LDSS[i]);
    //     printf("LDSS[%d] = %d\n", i, LDSS[i]);
    // }
    
    GetHuffmanEncoding(MSS,  RMSLen);
    MSSTCnt = CopyTable(MSST, 1 << A);

    // for (int i = 0; i < 1 << A; ++i) {
    //     printf("MSST[%d] = %s\n", i, MSST[i]);
    // }
    
    GetHuffmanEncoding(LMSS, RMSLen);
    LMSSTCnt = CopyTable(LMSST, TABLE_MAX_SIZE);

    // for (int i = 0; i < 100; ++i) {
    //     printf("LMSST[%d] = %s\n", i, LMSST[i]);
    // }

    GetHuffmanEncoding(DSS, RDSLen);
    
    // for (int i = 0; i < 1000; ++i) {
    //     printf("--MDST[%d] = %s\n", i, tempTable[i]);
    // }
    MDSTCnt = CopyTable(MDST, 1 << _B);
    // for (int i = 0; i < 1000; ++i) {
    //     printf("MDST[%d] = %s\n", i, MDST[i]);
    // }



    GetHuffmanEncoding(LDSS, RDSLen);
    LMDSTCnt = CopyTable(LMDST, TABLE_MAX_SIZE);

    int sum_MS_huffman_bit = 0;
    int sum_LMS_huffman_bit = 0;
    int sum_DS_huffman_bit = 0;
    int sum_LDS_huffman_bit = 0;

    int UnFixed_sum_MS_bit = 0;
    int UnFixed_sum_LMS_bit = 0;
    int UnFixed_sum_DS_bit = 0;
    int UnFixed_sum_LDS_bit = 0;

    int MS_val_max = 0;
    int LMS_val_max = 0;
    int DS_val_max = 0;
    int LDS_val_max = 0;

    int MS_huffman_encode_max_len = 0;
    int LMS_huffman_encode_max_len = 0;
    int DS_huffman_encode_max_len = 0;
    int LDS_huffman_encode_max_len = 0;

    puts("GetHuffmanEncoding Finished");

    // puts("MS");
    for (int i = 0; i < RMSLen; ++i) {
        sum_MS_huffman_bit  += strlen(MSST[MSS[i]]);
        sum_LMS_huffman_bit += strlen(LMSST[LMSS[i]]);

        UnFixed_sum_MS_bit  += getUnFixedLen(MSS[i]);
        UnFixed_sum_LMS_bit += getUnFixedLen(LMSS[i]);

        // MSFixedLen = _max(MSFixedLen, LMSS[i]);

        MS_val_max = _max(MS_val_max, MSS[i]);
        LMS_val_max = _max(LMS_val_max, LMSS[i]);
        MS_huffman_encode_max_len = _max(MS_huffman_encode_max_len, strlen(MSST[MSS[i]]));
        LMS_huffman_encode_max_len = _max(LMS_huffman_encode_max_len, strlen(LMSST[LMSS[i]]));
        // if (i < 100) {
        //     printf("line %d, data = %d \thuffman_bit = %-3d \tUnFixed_bit = %-3d\n", i, LMSS[i], strlen(LMSST[LMSS[i]]), getUnFixedLen(LMSS[i]));
        // }
    }

    // puts("DS");
    for (int i = 0; i < RDSLen; ++i) {
        sum_DS_huffman_bit  += strlen(MDST[DSS[i]]);
        sum_LDS_huffman_bit += strlen(LMDST[LDSS[i]]);

        UnFixed_sum_DS_bit  += getUnFixedLen(DSS[i]);
        UnFixed_sum_LDS_bit += getUnFixedLen(LDSS[i]);

        // MDFixedLen = _max(MDFixedLen, LDSS[i]);

        DS_val_max = _max(DS_val_max, DSS[i]);
        LDS_val_max = _max(LDS_val_max, LDSS[i]);
        DS_huffman_encode_max_len = _max(DS_huffman_encode_max_len, strlen(MDST[DSS[i]]));
        LDS_huffman_encode_max_len = _max(LDS_huffman_encode_max_len, strlen(LMDST[LDSS[i]]));
        // if (i < 100) {
        //     printf("line %d, data = %d \thuffman_bit = %-3d \tUnFixed_bit = %-3d\n", i, LDSS[i], strlen(LMDST[LDSS[i]]), getUnFixedLen(LDSS[i]));
        // }
    }

    puts("ALL Finished");

    // log
    {
    puts("=== FIRST TABLE INFO ===");
    puts("------------------------------------------");
    printf("RMSLen = %d   RDSLen = %d   g_readline = %d\n", RMSLen, RDSLen, g_readline);

    printf("MS__val_max = %-5d\n",   MS_val_max);
    printf("LMS_val_max = %-5d\n",   LMS_val_max);
    printf("DS__val_max = %-5d\n",   DS_val_max);
    printf("LDS_val_max = %-5d\n\n", LDS_val_max);

    printf("MS__huffman_encode_max_len = %-5d\n", MS_huffman_encode_max_len);
    printf("LMS_huffman_encode_max_len = %-5d\n", LMS_huffman_encode_max_len);
    printf("DS__huffman_encode_max_len = %-5d\n", DS_huffman_encode_max_len);
    printf("LDS_huffman_encode_max_len = %-5d\n\n", LDS_huffman_encode_max_len);

    printf("MSSTcnt    = %-5d \t all_bit = %-6d\n", MSSTCnt.first, MSSTCnt.second);
    printf("LMSSTCnt   = %-5d \t all_bit = %-6d\n", LMSSTCnt.first, LMSSTCnt.second);
    printf("MDSTCnt    = %-5d \t all_bit = %-6d\n", MDSTCnt.first, MDSTCnt.second);
    printf("LMDSTCnt   = %-5d \t all_bit = %-6d\n", LMDSTCnt.first, LMDSTCnt.second);
    puts("------------------------------------------\n");
    
    // UNFIXED
    {
    // puts("/*UNFIXED的意思是，里面的数据用变长的存储格式来存，比如每5位设置1bit的停止位（对大数据友好）*/");
    // puts("=== FIRST UNFIXED STAT INFO ===");
    // puts("------------------------------------------");
    // printf("RMSLen = %d  RDSLen = %d g_readline = %d\n", RMSLen, RDSLen, g_readline);
    // printf("UnFixed_sum_MS_bit  = %-6d\n", UnFixed_sum_MS_bit);
    // printf("UnFixed_sum_LMS_bit = %-6d\n", UnFixed_sum_LMS_bit);
    // printf("UnFixed_sum_DS_bit  = %-6d\n", UnFixed_sum_DS_bit);
    // printf("UnFixed_sum_LDS_bit = %-6d\n", UnFixed_sum_LDS_bit);
    // puts("------------------------------------------\n");

    // puts("=== FIRST COMPRESS UnFixed INFO ===");
    // puts("------------------------------------------");
    // printf("MS  bit compresstion radio = %.5f\n", (sum_MS_huffman_bit + sum_LMS_huffman_bit) * 1.0 / (UnFixed_sum_MS_bit + UnFixed_sum_LMS_bit));                                             
    // printf("DS  bit compresstion radio = %.5f\n", (sum_DS_huffman_bit + sum_LDS_huffman_bit) * 1.0 / (UnFixed_sum_DS_bit + UnFixed_sum_LDS_bit));
    // printf("all bit compresstion radio = %.5f\n", (sum_MS_huffman_bit + sum_LMS_huffman_bit + sum_DS_huffman_bit + sum_LDS_huffman_bit) * 1.0 / (UnFixed_sum_MS_bit + UnFixed_sum_LMS_bit + UnFixed_sum_DS_bit + UnFixed_sum_LDS_bit));                                      
    // puts("------------------------------------------\n");
    }

    puts("          === HUFFMAN STAT INFO ===");
    puts("--------------------------------------------------------");
    printf("RMSLen = %d  RDSLen = %d g_readline = %d\n", RMSLen, RDSLen, g_readline);
    printf("Sum_MS__huffman_bit = %-6d\n", sum_MS_huffman_bit);
    printf("Sum_LMS_huffman_bit = %-6d\n", sum_LMS_huffman_bit);
    printf("Sum_DS__huffman_bit = %-6d\n", sum_DS_huffman_bit);
    printf("Sum_LDS_huffman_bit = %-6d\n", sum_LDS_huffman_bit);
    puts("--------------------------------------------------------\n");

    puts("          === After RLE STAT INFO ===");
    puts("--------------------------------------------------------");
    printf("RMSLen = %d  RDSLen = %d g_readline = %d\n", RMSLen, RDSLen, g_readline);
    printf("Raw_sum_MS_bit  = %-6d\n", A * RMSLen);
    printf("Raw_sum_DS_bit  = %-6d\n", _B * RDSLen); // should using "_B" -> quantize_bit
    printf("Raw_sum_bit     = %-6d\n", A * RMSLen + _B * RDSLen);
    puts("--------------------------------------------------------\n");

    puts("             ===  RAW STAT INFO  ===");
    puts("--------------------------------------------------------");
    printf("RMSLen = %d  RDSLen = %d g_readline = %d\n", RMSLen, RDSLen, g_readline);
    printf("Raw_sum_MS_bit  = %-6d\n", A * g_readline);
    printf("Raw_sum_DS_bit  = %-6d\n", B * g_readline); // should using "B" -> 9
    printf("Raw_sum_bit     = %-6d\n", A * g_readline + _B * g_readline);
    puts("--------------------------------------------------------\n");

    puts(" === COMPRESSION RATIO [ huffman + RLE : RLE ] INFO ===");
    puts("--------------------------------------------------------");
    printf("H1_MS  bit compression ratio = %.5f\n", (sum_MS_huffman_bit + sum_LMS_huffman_bit) * 1.0 / (A * RMSLen));                                             
    printf("H1_DS  bit compression ratio = %.5f\n", (sum_DS_huffman_bit + sum_LDS_huffman_bit) * 1.0 / (_B * RDSLen));    // should using "_B" -> quantize_bit
    printf("H1_All bit compression ratio = %.5f\n", (sum_MS_huffman_bit + sum_LMS_huffman_bit + sum_DS_huffman_bit + sum_LDS_huffman_bit) * 1.0 / (A * RMSLen + _B * RDSLen));                                      
    puts("--------------------------------------------------------\n");

    puts(" === COMPRESSION RATIO [ huffman + RLE : Raw ] INFO ===");
    puts("--------------------------------------------------------");
    printf("H2_MS  bit compression ratio = %.5f\n", (sum_MS_huffman_bit + sum_LMS_huffman_bit) * 1.0 / (A * g_readline));                                             
    printf("H2_DS  bit compression ratio = %.5f\n", (sum_DS_huffman_bit + sum_LDS_huffman_bit) * 1.0 / (B * g_readline)); // should using "B" -> 9
    printf("H2_All bit compression ratio = %.5f\n", (sum_MS_huffman_bit + sum_LMS_huffman_bit + sum_DS_huffman_bit + sum_LDS_huffman_bit) * 1.0 / (A * g_readline + B * g_readline));                                      
    puts("--------------------------------------------------------\n");
    }
}

/*

D:\\0_Desktop\\Pro\\FD_\\input\\inputs/2020-11-14_13_04_18\\25\\iptA\\our\\0_from_0_to_250000.txt

D:\\0_Desktop\\Pro\\FD_\\input\\inputs/2020-11-14_21_47_34\\25\\iptA\\our\\0_from_0_to_250000.txt

*/



int main(int argc, char* args[]) {

#if DEBUG_FILE
    FILE* fp1 = fopen("real_data_in.txt","w");
    FILE* fp2 = fopen("real_data_out.txt","w");
    FILE* fp3 = fopen("real_data_raw_raw.txt","w");
    fclose(fp1);
    fclose(fp2);
    fclose(fp3);
#endif // DEBUG_FILE
    
    Process_status process_status = STM32;
    STM32_status   stm32_status   = STM32_FIRST;
    WIN_status     win_status     = WIN_FIRST;
    
    assert(argc == 3);
    
    quantize_bit = args[1][0] - '0';
    memcpy(input_name, args[2], strlen(args[2]) + 1);
    //memcpy(input_name, args[2], strlen("d:\\0_Desktop\\Pro\\FD_\\src\\in.txt") + 1);
    _B = quantize_bit;
    printf("_B = [%d]\n", _B);
    printf("path = [%s]\n", input_name);
    


    bool stop = false;
    while (!stop) {
        switch(process_status) {
            case STM32: {
                switch(stm32_status) {
                    case STM32_FIRST: {
                        STM32_first_write();
                        stm32_status = STM32_NORMAL_STATUS;
                        break;
                    }
                    case STM32_NORMAL_STATUS: {
                        bool isFull = false;
                        STM32_normal_status stm_normal_status = STM32_LOAD;
                        while (!isFull) {
                            switch(stm_normal_status) {
                                case STM32_LOAD:
                                    break;
                                case STM32_INIT:
                                    break;
                                case STM32_QUANTIZE:
                                    break;
                                case STM32_ENCODING:
                                    break;
                                case STM32_WRITE_NVM:
                                    break;
                            }
                        }
                        break;
                    }
                    default: 
                        break;
                }
                process_status = WINDOWS;
                break;
            }
            case WINDOWS: {
                switch(win_status) {
                    case WIN_FIRST:
                        WIN_first_read();
                        return 0; // just stop
                        win_status = WIN_NORMAL_STATUS;
                        break;
                    case WIN_NORMAL_STATUS:
                        // WIN_LOAD,
                        // WIN_DECODE,
                        // WIN_EVALUATE,
                        // WIN_UPDATE_TABLE
                        break;
                }
                process_status = STM32;
                break;
            }
            default:
                break;
        }
    }
    return 0;
}