#include<stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include<string.h>
#include "bloomfilter.h"
#define HASH_MAX_SIZE 18

typedef struct Item {
	char *data;
	int size;
} Item;

/**
 * @brief  Merkel Tree节点
 */
typedef struct MerkelNode 
{
    struct MerkelNode *left;
    struct MerkelNode *right;
	struct MerkelNode *parent;
	struct bloom *bloom;
	int level;
	int is_dup;	 // 是否复制而来
    char hash[HASH_MAX_SIZE];
	char data[0]; 	// 柔性数组，节省空间
} MerkelNode;

/**
 * @brief  Merkel Tree结构体
 */
typedef struct MerkelTree {
	MerkelNode *root;
	MerkelNode **leafnode;	// 叶子节点链表
	int leafnode_num;
	int use_bloom;	// 是否使用bloom filter
} MerkelTree;

void create_leafnode_list(MerkelTree *mt , Item *data, int num);
struct MerkelNode *createnode(char hash[]);
struct MerkelTree *create_tree(Item *data, int num, int use_bloom);
void Print_Merkle_Tree(MerkelNode *mt, int high);

bool verify_data (MerkelTree *mt, Item *item);
bool verify_data_with_bloom(MerkelTree *mt, Item *item);
bool _verify_data_with_bloom(MerkelNode *node, Item *item);
bool verify_data_without_bloom(MerkelTree *mt, Item *item);

// Merkel Tree中使用的哈希算法
void decToHexa(unsigned int n, char *hexaDeciNum);
unsigned int smh(char input[],int size);
void concatHash(char input[], int size, char *c);
void hash_hash(char *hash1, char *hash2, char *hash);