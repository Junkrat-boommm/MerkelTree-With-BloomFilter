#include <strings.h>
#include <stdint.h>
#include <memory.h>
#include "merkeltree.h"
#define DEBUG 0
#define bloom_entry 100000
#define bloom_error ((double)0.01)

// #define IS_DUP = 0b00000001

// dup node1 to node2
void node_dup(MerkelNode *node1, MerkelNode *node2) {
	node2->is_dup = 1;
	if(0 != strlen(node1->data)) memcpy(node2->data, node1->data, strlen(node1->data));
	memcpy(node2->hash, node1->hash, strlen(node1->hash));
	node2->bloom = node1->bloom;
	node2->left = node1->left;
	node2->right = node1->right;
	node2->parent = node1->parent;
	node2->level = node1->level;
	return;
}

/**
 * @brief  创建Merkel Tree中由叶子节点构成的链表
 * @note   mt中的leafnode_list将被用于生成MerKel Tree
 * @param  *mt: Merkel Tree
 * @param  *data: 指向被存放到mt中的数据项
 * @param  num: 数据项的个数
 * @retval None
 */
void create_leafnode_list(MerkelTree *mt, Item *data, int num) {
	uint32_t n = num;
	if (num % 2) n = num + 1;
	struct MerkelNode **leaf_node_list = (struct MerkelNode **)malloc(n * sizeof(struct MerkelNode*));
	for (int i = 0; i < num; i++) {
		char hash[HASH_MAX_SIZE];
		concatHash(data[i].data, data[i].size, hash);
		MerkelNode *leafnode = (struct MerkelNode *) malloc (sizeof(struct MerkelNode) + data[i].size + 1);
		memcpy(leafnode->hash, hash, strlen(hash)+1);
		memcpy(leafnode->data, data[i].data, data[i].size);
		leafnode->data[data[i].size] = '\0';
		leafnode->left = NULL;
		leafnode->right = NULL;
		if(mt->use_bloom) {
			leafnode->bloom = (struct bloom *) malloc(sizeof(struct bloom));
			bloom_init(leafnode->bloom, bloom_entry, bloom_error);
			bloom_add(leafnode->bloom, data[i].data, data[i].size);	// add to bloomfilter
		}
		leaf_node_list[i] = leafnode;
	}
	if (n > num) {	// 复制一个结点，用于上一层节点的构造
		MerkelNode *leafnode = (struct MerkelNode *) malloc (sizeof(struct MerkelNode) + data[num-1].size + 1);
		node_dup(leaf_node_list[num-1], leafnode);
		leaf_node_list[num] = leafnode;
	}
	mt->leafnode = leaf_node_list;
	mt->leafnode_num = num;
    return ;
}

/**
 * @brief  根据传入的hash值构造中间节点
 * @note   
 * @param  hash[]: 哈希值
 * @retval 中间节点
 */
struct MerkelNode *createnode(char hash[])
{
    struct MerkelNode *node = (struct MerkelNode *)malloc(sizeof(struct MerkelNode));
    memcpy(node->hash, hash, strlen(hash)+1);
    node->left = NULL;
    node->right = NULL;
    return node;
}  


/**
 * @brief  构造树
 * @note   利用递归的方式自底向上，逐层构造Merkel Tree
 * @param  *mt: Merkel Tree
 * @param  **leafnodes: mt->leafnode_list
 * @param  n: 叶子节点个数
 * @param  level: 最大层数，用于打印
 * @retval 根节点
 */
struct MerkelNode *generateTree(MerkelTree *mt, MerkelNode **leafnodes, int n, int level)
{
    if (n == 1) {
        return leafnodes[0];
	}
	n = n + (n % 2);
    int y = n / 2, isOdd = 0;
    if (y % 2 != 0 && y != 1)
    {
        y++;
        isOdd = 1;
    }
    struct MerkelNode **temp = malloc(y * sizeof(struct MerkelNode *));
	int e = 0;
	// 构造上一层节点
	for (int i = 0; i < n; i++) {
		if (i % 2 != 0) {
			char hash[40];
			hash_hash(leafnodes[i-1]->hash, leafnodes[i]->hash, hash);
			temp[e] = createnode(hash);
			temp[e]->left = leafnodes[i - 1];
			temp[e]->right = leafnodes[i];
			temp[e]->level = level+1;
			temp[e]->parent = NULL;
			leafnodes[i-1]->parent=temp[e];
			leafnodes[i]->parent=temp[e];
			// 添加到布隆过滤器中
			if(mt->use_bloom) 
			{
				temp[e]->bloom = (struct bloom *)malloc(sizeof(struct bloom));
				bloom_and(leafnodes[i]->bloom, leafnodes[i-1]->bloom, temp[e]->bloom);
			}
			e++;
		}
	}
	if (isOdd) {
		MerkelNode *leafnode = (struct MerkelNode *) malloc (sizeof(struct MerkelNode)); // 没有data
		node_dup(temp[e-1], leafnode);
		temp[e] = leafnode;
	}
    return generateTree(mt, temp, y, level+1);
}

// 带布隆过滤器的查找，对外提供的接口
bool verify_data_with_bloom(MerkelTree *mt, Item *item) {
	_verify_data_with_bloom(mt->root, item);
}

/**
 * @brief  带布隆过滤器查找的真正实现
 * @note   从根节点开始，逐层判断item是否存在，并向下递归，直到查找到对应的叶子节点
 * @param  *node: 当前节点
 * @param  *item: 需要查找的内容项
 * @retval true: 存在； false: 不存在
 */
bool _verify_data_with_bloom(MerkelNode *node, Item *item) {
	if(NULL != node) {
		if (node->is_dup) return false;// 不查找复制而来的节点
		if (bloom_check(node->bloom, item->data, item->size)) {// 检查布隆过滤器 
			if(DEBUG) printf("%s\n",node->hash);	// 用于测试验证
			MerkelNode *left = node->left;
			MerkelNode *right = node->right;
			if(left == NULL && right == NULL) {
				if(DEBUG) printf("%s\n", node->data);
				return true;
			}
			else if(_verify_data_with_bloom(left, item)) return true;
			else return _verify_data_with_bloom(right, item);
		}
		else return false;
	}
	return false;
}

/**
 * @brief  不带布隆过滤器的查找
 * @note   查找树mt中使用存有item项
 * @param  *mt: Merkel Tree
 * @param  *item: 需要查找的数据项
 * @retval true: 存在； false: 不存在
 */
bool verify_data_without_bloom(MerkelTree *mt, Item *item) {
	MerkelNode **leafnode_list = mt->leafnode;
	for (int i = 0; i < mt->leafnode_num; i++) {
		MerkelNode *leafnode = leafnode_list[i];
		if(item->size == strlen(leafnode->data) && !memcmp(item->data, leafnode->data, item->size)) {
			// check hash
			if(DEBUG) printf("i=%d\n",i);
			MerkelNode *parent = leafnode->parent;
			while (parent != NULL) {
				if(DEBUG) printf("%s\n", parent->hash);
				MerkelNode *left = parent->left;
				MerkelNode *right = parent->right;
				char parent_hash[40];
				hash_hash(left->hash, right->hash, parent_hash); 
				int equal = !strcmp(parent_hash, parent->hash);
				if (!equal) return false;
				parent = parent->parent;
			}
			return true;
		}
	}
	return false;
}

// 根据是否使用bloomfilter执行查找
bool verify_data (MerkelTree *mt, Item *item) {
	if (mt->root == NULL) return false;
	if(0 == mt->use_bloom) {
		verify_data_without_bloom(mt, item);
 	}
	else {
		if(mt == NULL || mt->root == NULL) return false;
		return verify_data_with_bloom(mt, item);
	}
}


struct MerkelTree *create_tree(Item *data, int num, int use_bloom) {
	MerkelTree *mt = (MerkelTree *) malloc (sizeof(MerkelTree));
	mt->use_bloom = use_bloom;
	create_leafnode_list(mt, data, num);
	MerkelNode *root = generateTree(mt, mt->leafnode, mt->leafnode_num, 1);
	mt->root = root;
	return  mt;
}

int first = 1;
/**
 * @brief  打印函数
 * @note   
 * @param  *mt: Merkel Tree的根节点
 * @param  high: 根节点的层数
 * @retval None
 */
void Print_Merkle_Tree(MerkelNode *mt, int high)
{
	MerkelNode *p=mt;
	int i; 

	if(p==NULL){
		return;
	}
	if(p->left== NULL && p->right==NULL){
		printf("\n");
		for(i=0; i < high - p -> level; i++)
			printf("         ");
			
		
		// if (p->is_dup != 1) printf("--->%s\(%s\)\n", p->hash, p->data);
		printf("--->%s\(%s\), %d\n", p->hash, p->data, p->is_dup);
		first = 1;
		
		return;	
	}else{
		Print_Merkle_Tree(mt->left, high); 
	
		if(first==1){
			for(i=0; i< high- p->level; i++)
			printf("         ");
				
			printf("--->");	
		}else
			printf("--->");	
		// if (p->is_dup != 1) printf("%10s", p-> hash);
		printf("%10s,%d", p-> hash, p->is_dup);
		first=0;
			
		Print_Merkle_Tree(mt->right, high); 
	}
} 

// HASH
void decToHexa(unsigned int n, char *hexaDeciNum) 
{    
   
    int i = 0; 
    while(n!=0) 
    {    
        // temporary variable to store remainder 
        int temp  = 0; 
          
        // storing remainder in temp variable. 
        temp = n % 16; 
          
         
        if(temp < 10) 
        { 
            hexaDeciNum[i] = (char) (temp + 48); 
            i++; 
        } 
        else
        { 
            hexaDeciNum[i] = (char) (temp + 55);
            i++; 
        } 
          
        n = n/16; 
    } //function to convert decimal to hexadcimal so to make hashed value alphanumeric
    char hexfile[8];
    hexaDeciNum[i] = '\0';
} 
  


unsigned int smh(char input[],int size){
    
    unsigned int init=1737199;
    unsigned int magic=7139379;
    unsigned int hash=0;
    for(int i=0;i<size;i++)
    {
        hash=hash^(input[i]);
        hash=hash*magic;
   }
    return hash;

  
}//This is the simple hashing function based on Knuth Multiplicative hashing algorithm it generates 8Charcters length fixed hash code which is 
 //unique for each file

void concatHash(char input[], int size, char *c)
{
    decToHexa(smh(input, size), c);
}

// 两个哈希值的哈希
void hash_hash(char *hash1, char *hash2, char *hash) {
	char temp[40];
	strcpy(temp, hash1);
	strcat(temp, hash2);
	concatHash(temp, strlen(temp), hash);
}
