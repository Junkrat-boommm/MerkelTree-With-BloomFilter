# MerkelTree-With-BloomFilter
利用布隆过滤器加速Merkel Tree的查询


# 测试方式
## 参数
通过修改`bench.c`中的`TEST_NUM`来指明测试数量级（请勿超过100000，如有需要，需修改`merkeltree.c`中的bloom_entry)

## 运行方式
```bash
make
./bench
```

