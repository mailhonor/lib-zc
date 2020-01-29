## 唯一id

线程安全的唯一id生成方法. 源码见 src/stdlib/unique\_id.c

---

```
#define zvar_unique_id_size 22

char *zbuild_unique_id(char *buf);

/* 从唯一id中获得时间戳(秒)并返回 */
long zget_time_from_unique_id(char *buf);

/* 检测是不是唯一id格式 */
zbool_t zis_unique_id(char *buf);

```

