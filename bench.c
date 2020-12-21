#include "bloomfilter.h"
#include "merkeltree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <sys/time.h>

typedef long clock_t;

#define TEST_NUM 100000
#define use_bloom 1

void test_create_leafnode_list() {
    Item src[5] = {{"abx", 3}, {"xyz4",4}, {"nihao", 5}, {"thank you", 9}, {"ganquan",7}};
    MerkelTree *mt = create_tree(src, 5, false);
    MerkelNode *root = mt->root;
    Print_Merkle_Tree(root, root->level);
    assert(1);
}

void test_verify() {
    Item src[15] = {{"abx", 3}, {"xyz4",4}, {"nihao", 5}, {"thank you", 9}, {"ganquan",7}
                    ,{"1",1}, {"11",2}, {"111", 3}, {"1111",4}, {"1234",4}
                    ,{"a",1}, {"aa",2}, {"aaa",3},{"aaaa",4},{"aaaaa",5}
    };
    MerkelTree *mt = create_tree(src, 15, false);
    MerkelNode *root = mt->root;
    Print_Merkle_Tree(root, root->level);
    // assert(verify_data_without_bloom(mt, &exist_item[0]));
    // int len = sizeof(exist_item)/sizeof(Item);
    for (int i=0; i<15; i++) {        
        printf("%d\n",verify_data_without_bloom(mt, &src[i]));
    }
}

void test_many_data() {
    Item *test = (Item*)malloc (TEST_NUM * sizeof(Item));
    uint32_t* buf = NULL;
    buf = malloc(sizeof(uint32_t)*TEST_NUM);
    for(int i = 0; i<TEST_NUM; i++) {
        // buf[i] = i+1;
        // printf("%u\n",buf[i]); 
        test[i].data = (char *)&buf[i];
        // '1' --- '10000'
        char *t = test[i].data;
        int f = 0;
        int j = i+1; 
        while(j >= 10) {
            int y = j%10;
            j = j/10;
            t[f] = '0'+y;
            f++;
        }
        t[f] = '0'+j;
        test[i].size = strlen(test[i].data);
        //printf("%d\n",test[i].size); 
    }

    MerkelTree *mt = create_tree(test, TEST_NUM, true);
    
    MerkelNode *root = mt->root;
    //Print_Merkle_Tree(mt->root, mt->root->level);


    struct timeval start_with_bloomfilter;
    struct timeval end_with_bloomfilter;
    unsigned long timer_with_bloomfilter;
    /* 程序开始之前计时start */
    gettimeofday(&start_with_bloomfilter, NULL);
    
    // clock_t verify_data_with_bloom_start = clock();
    for(int i=0; i<TEST_NUM; i++) {
        assert(verify_data_with_bloom(mt, &test[i]));
    }
    // clock_t verify_data_with_bloom_end = clock();
    /* 程序块结束后计时end */
    gettimeofday(&end_with_bloomfilter, NULL);
    /* 统计程序段运行时间(unit is usec)*/
    timer_with_bloomfilter = 1000000 * (end_with_bloomfilter.tv_sec - start_with_bloomfilter.tv_sec) + end_with_bloomfilter.tv_usec - start_with_bloomfilter.tv_usec;


    struct timeval start_without_bloomfilter;
    struct timeval end_without_bloomfilter;
    unsigned long timer_without_bloomfilter;
    /* 程序开始之前计时start */
    gettimeofday(&start_without_bloomfilter, NULL);
    
    // clock_t verify_data_with_bloom_start = clock();
    for(int i=0; i<TEST_NUM; i++) {
        assert(verify_data_without_bloom(mt, &test[i]));
    }
    // clock_t verify_data_with_bloom_end = clock();
    /* 程序块结束后计时end */
    gettimeofday(&end_without_bloomfilter, NULL);
    /* 统计程序段运行时间(unit is usec)*/
    timer_without_bloomfilter = 1000000 * (end_without_bloomfilter.tv_sec - start_without_bloomfilter.tv_sec) + end_without_bloomfilter.tv_usec - start_without_bloomfilter.tv_usec;
    printf("verify data with bloomfilter takes %ld us.\n", timer_with_bloomfilter);
    printf("verify data without bloomfilter takes %ld us.\n", timer_without_bloomfilter);

}

void main() {
    test_many_data();
    // test_verify();
    return;
}


