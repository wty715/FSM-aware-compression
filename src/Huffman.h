/*
 * @Description: 
 * @Author: Wangzhe
 * @Date: 2020-11-13 19:21:34
 * @LastEditors: Wangzhe
 * @LastEditTime: 2020-11-13 19:24:47
 * @FilePath: \0_Desktop\Pro\FD_\src\Huffman.h
 */

#ifndef __HUFFMAN_H_
#define __HUFFMAN_H_

typedef struct node {
	int x;
	long long freq;
	char *code;
	int type;
	struct node *next;
	struct node *left;
	struct node *right;
} mNode, dNode, node;



#define INTERNAL 1
#define LEAF 0



node* HuffmanNewNode(uintA_t val);
void insert(node *p,node *m);
void HuffmanAddNode(node** head, int val);
void HuffmanMakeTree(node* head, node** root);
void HuffmanGenCode(node **head, node *p, char* code);






#endif // __HUFFMAN_H_
