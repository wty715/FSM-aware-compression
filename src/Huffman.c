/*
 * @Description: 
 * @Author: Wangzhe
 * @Date: 2020-11-13 19:21:26
 * @LastEditors: Wangzhe
 * @LastEditTime: 2020-11-13 21:52:48
 * @FilePath: \Pro\FD_\src\Huffman.c
 */

#include "main.h"
#include "Huffman.h"




node* HuffmanNewNode(int val) {
	node *q;
	q = (node *)malloc(sizeof(node));
	q->x = val;
	q->type = LEAF;	//leafnode
	q->freq = 1LL;
	q->next = NULL;
	q->left = NULL;
	q->right = NULL;
	return q;
}


// insert p in list as per its freq., start from m to right,
// we cant place node smaller than m since we dont have ptr to node left to m
void insert(node *p,node *m) {
    if (m->next == NULL) {
        m->next = p; return;
    }
    while (m->next->freq < p->freq) {
        m = m->next;
        if(m->next == NULL) {
            m->next = p; 
            return ; 
        }
    }
    p->next = m->next;
    m->next = p;
}

void HuffmanAddNode(node** head, int val) {
    // q is p's father
    mNode *p, *q, *m;
    
    if ((*head) == NULL) {
        (*head) = HuffmanNewNode(val);
        return;
    }
    
    p = (*head); q = NULL;
    if (p->x == val) { //item found in HEAD
        p->freq++;
        if(p->next == NULL) {
            return ;
        }
        if(p->freq > p->next->freq) {
            (*head) = p->next;
            p->next = NULL;
            insert(p,(*head));
        }
        return ;
    }

    while (p->next != NULL && p->x != val) {
        q = p; 
        p = p->next;
    }

    if (p->x == val) {
        p->freq++;
        if (p->next == NULL) {
            return ;
        }
        if (p->freq > p->next->freq) {
            m = p->next;
            q->next = p->next;
            p->next = NULL;
            insert(p, (*head));
        }
    }
    //p->next==NULL , all list traversed c is not found, insert it at beginning
    else {
        q = HuffmanNewNode(val);
        if (q->freq <= (*head)->freq) {
            q->next = (*head);  //first because freq is minimum
            (*head) = q;
        } else {
            insert(q, (*head));
        }
    }
}

void HuffmanMakeTree(node* head, node** root) {
    node *p, *q;
    p = head;
	while (p != NULL) {
		q = HuffmanNewNode('@');
		q->type = INTERNAL;	//internal node
		q->left = p;		//join left subtree/node_fg
		q->freq = p->freq;
		if (p->next != NULL) {
			p = p->next;
			q->right = p;	//join right subtree /node
			q->freq += p->freq;
		}
		p = p->next;	//consider next node frm list
		if (p == NULL) {	//list ends
			break;
        } // if
		//insert new subtree rooted at q into list starting from p
		//if q smaller than p
        //place it before p
		if (q->freq <= p->freq) {
			q->next = p;
			p = q;
		} else {
            insert(q,p);	//find appropriate position
        } // else
	} //while
    *root = q;
}


/**
 * @description: 递归生成哈夫曼编码， 注意flag为static，当更换head时需要清空标志位
 * @param {type} 
 * @return {type} 
 */
void HuffmanGenCode(node **head, node *p, char* code) {
    char *lcode, *rcode;
    static node *s;
    static int flag;
    if (!recursionFlg) {
        flag = 0;
        recursionFlg = 1;
    }
    // if (preType == 0 && type == 1) flag = 0;
    //sort linked list as it was
    if (p != NULL) {
        //leaf node
        if (p->type == LEAF) {
            //first leaf node
        	if(flag == 0) {
                flag = 1; 
                *head = p;
            } else { //other leaf nodes
                s->next = p;
            }		//sorting LL
            p->next = NULL;
            s = p;
        }
        // preType = type;
        //assign code
        p->code = code;	//assign code to current node
        if (p->type == LEAF) {
            //printf("[%s] [len = %d]\n", p->code, strlen(p->code));
            int sb = 0;
        }
#if LOG
        if (p->x != 64)
            printf("[ %4d | %4d | %4d | %s\n",p->x,p->freq,p->type,p->code);
#endif
        lcode=(char *)malloc(strlen(code)+2);
        rcode=(char *)malloc(strlen(code)+2);
        sprintf(lcode, "%s0", code);
        sprintf(rcode, "%s1", code);
        
        //recursive DFS
        HuffmanGenCode(&(*head), p->left, lcode);		//left child has 0 appended to current node's code
        HuffmanGenCode(&(*head), p->right, rcode);
    }
}

