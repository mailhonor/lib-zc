## ZCDB

一种新的静态db, 不支持修改. 源码见 src/cdb/

---

```
typedef struct zcdb_t zcdb_t;
typedef struct zcdb_walker_t zcdb_walker_t;
typedef struct zcdb_builder_t zcdb_builder_t;

#define zvar_cdb_code_version "0001"

/* 打开句柄 */
zcdb_t *zcdb_open(const char *cdb_pathname);
zcdb_t *zcdb_open2(const char *cdb_pathname, zbuf_t *error_msg);
void zcdb_close(zcdb_t *cdb);

/* cdb成员个数 */
int zcdb_get_count(zcdb_t *cdb);

/* -1:出错, 0: 没找到, 1: 找到. 线程安全 */
int zcdb_find(zcdb_t *cdb, const void *key, int klen, void **val, int *vlen);

/* 创建遍历器 */
zcdb_walker_t *zcdb_walker_create(zcdb_t *cdb);
void zcdb_walker_free(zcdb_walker_t *walker);

/* -1:出错, 0: 没找到, 1: 找到. 非线程安全 */
int zcdb_walker_walk(zcdb_walker_t *walker, void **key, int *klen, void **val, int *vlen);

/* 重置 */
void zcdb_walker_reset(zcdb_walker_t *walker);

/* 创建生成器 */
zcdb_builder_t *zcdb_builder_create();
void zcdb_builder_free(zcdb_builder_t *builder);

/* 更新值 */
void zcdb_builder_update(zcdb_builder_t *builder, const void *key, int klen, const void *val, int vlen);

/* 生成zcddb文件, 保存(覆盖写)在 dest_db_pathname */
int zcdb_builder_build(zcdb_builder_t *builder, const char *dest_db_pathname);

```

---

### 例子
见源码 sample/cdb/
