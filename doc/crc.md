## CRC

源码见 src/hash/

---

```
/* init_value 默认应该为 0 */
unsigned short int zcrc16(const void *data, int size, unsigned short int init_value);
unsigned int zcrc32(const void *data, int size, unsigned int init_value);
unsigned long zcrc64(const void *data, int size, unsigned long init_value);

```

---

## 例子
见源码 sample/hash/
