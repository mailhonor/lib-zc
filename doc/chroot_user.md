## chroot
源码见 src/stdlib/chroot_user.c

---

```
/* 如果user_name非空, 则改变实际用户为user_name */
/* 如果root_dir非空, 改变根(chroot)到root_dir */
/* 返回 -1: 失败, >=0: 成功 */
int zchroot_user(const char *root_dir, const char *user_name);
```

