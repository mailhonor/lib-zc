/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2025-01-01
 * ================================
 */

#pragma once

#include "zcc/zcc_sqlite3.h"

#define SQLITE_CONFIG_SINGLETHREAD 1 /* nil */
#define SQLITE_CONFIG_MULTITHREAD 2  /* nil */
#define SQLITE_CONFIG_SERIALIZED 3   /* nil */
#define SQLITE_CONFIG_MALLOC 4       /* sqlite3_mem_methods* */
#define SQLITE_CONFIG_GETMALLOC 5    /* sqlite3_mem_methods* */
#define SQLITE_CONFIG_SCRATCH 6      /* No longer used */
#define SQLITE_CONFIG_PAGECACHE 7    /* void*, int sz, int N */
#define SQLITE_CONFIG_HEAP 8         /* void*, int nByte, int min */
#define SQLITE_CONFIG_MEMSTATUS 9    /* boolean */
#define SQLITE_CONFIG_MUTEX 10       /* sqlite3_mutex_methods* */
#define SQLITE_CONFIG_GETMUTEX 11    /* sqlite3_mutex_methods* */
/* previously SQLITE_CONFIG_CHUNKALLOC 12 which is now unused. */
#define SQLITE_CONFIG_LOOKASIDE 13           /* int int */
#define SQLITE_CONFIG_PCACHE 14              /* no-op */
#define SQLITE_CONFIG_GETPCACHE 15           /* no-op */
#define SQLITE_CONFIG_LOG 16                 /* xFunc, void* */
#define SQLITE_CONFIG_URI 17                 /* int */
#define SQLITE_CONFIG_PCACHE2 18             /* sqlite3_pcache_methods2* */
#define SQLITE_CONFIG_GETPCACHE2 19          /* sqlite3_pcache_methods2* */
#define SQLITE_CONFIG_COVERING_INDEX_SCAN 20 /* int */
#define SQLITE_CONFIG_SQLLOG 21              /* xSqllog, void* */
#define SQLITE_CONFIG_MMAP_SIZE 22           /* sqlite3_int64, sqlite3_int64 */
#define SQLITE_CONFIG_WIN32_HEAPSIZE 23      /* int nByte */
#define SQLITE_CONFIG_PCACHE_HDRSZ 24        /* int *psz */
#define SQLITE_CONFIG_PMASZ 25               /* unsigned int szPma */
#define SQLITE_CONFIG_STMTJRNL_SPILL 26      /* int nByte */
#define SQLITE_CONFIG_SMALL_MALLOC 27        /* boolean */
#define SQLITE_CONFIG_SORTERREF_SIZE 28      /* int nByte */
#define SQLITE_CONFIG_MEMDB_MAXSIZE 29       /* sqlite3_int64 */

//

#define SQLITE_OK 0 /* Successful result */
/* beginning-of-error-codes */
#define SQLITE_ERROR 1       /* Generic error */
#define SQLITE_INTERNAL 2    /* Internal logic error in SQLite */
#define SQLITE_PERM 3        /* Access permission denied */
#define SQLITE_ABORT 4       /* Callback routine requested an abort */
#define SQLITE_BUSY 5        /* The database file is locked */
#define SQLITE_LOCKED 6      /* A table in the database is locked */
#define SQLITE_NOMEM 7       /* A malloc() failed */
#define SQLITE_READONLY 8    /* Attempt to write a readonly database */
#define SQLITE_INTERRUPT 9   /* Operation terminated by sqlite3_interrupt()*/
#define SQLITE_IOERR 10      /* Some kind of disk I/O error occurred */
#define SQLITE_CORRUPT 11    /* The database disk image is malformed */
#define SQLITE_NOTFOUND 12   /* Unknown opcode in sqlite3_file_control() */
#define SQLITE_FULL 13       /* Insertion failed because database is full */
#define SQLITE_CANTOPEN 14   /* Unable to open the database file */
#define SQLITE_PROTOCOL 15   /* Database lock protocol error */
#define SQLITE_EMPTY 16      /* Internal use only */
#define SQLITE_SCHEMA 17     /* The database schema changed */
#define SQLITE_TOOBIG 18     /* String or BLOB exceeds size limit */
#define SQLITE_CONSTRAINT 19 /* Abort due to constraint violation */
#define SQLITE_MISMATCH 20   /* Data type mismatch */
#define SQLITE_MISUSE 21     /* Library used incorrectly */
#define SQLITE_NOLFS 22      /* Uses OS features not supported on host */
#define SQLITE_AUTH 23       /* Authorization denied */
#define SQLITE_FORMAT 24     /* Not used */
#define SQLITE_RANGE 25      /* 2nd parameter to sqlite3_bind out of range */
#define SQLITE_NOTADB 26     /* File opened that is not a database file */
#define SQLITE_NOTICE 27     /* Notifications from sqlite3_log() */
#define SQLITE_WARNING 28    /* Warnings from sqlite3_log() */
#define SQLITE_ROW 100       /* sqlite3_step() has another row ready */
#define SQLITE_DONE 101      /* sqlite3_step() has finished executing */

//
#define SQLITE_OPEN_READONLY 0x00000001       /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_READWRITE 0x00000002      /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_CREATE 0x00000004         /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_DELETEONCLOSE 0x00000008  /* VFS only */
#define SQLITE_OPEN_EXCLUSIVE 0x00000010      /* VFS only */
#define SQLITE_OPEN_AUTOPROXY 0x00000020      /* VFS only */
#define SQLITE_OPEN_URI 0x00000040            /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_MEMORY 0x00000080         /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_MAIN_DB 0x00000100        /* VFS only */
#define SQLITE_OPEN_TEMP_DB 0x00000200        /* VFS only */
#define SQLITE_OPEN_TRANSIENT_DB 0x00000400   /* VFS only */
#define SQLITE_OPEN_MAIN_JOURNAL 0x00000800   /* VFS only */
#define SQLITE_OPEN_TEMP_JOURNAL 0x00001000   /* VFS only */
#define SQLITE_OPEN_SUBJOURNAL 0x00002000     /* VFS only */
#define SQLITE_OPEN_MASTER_JOURNAL 0x00004000 /* VFS only */
#define SQLITE_OPEN_NOMUTEX 0x00008000        /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_FULLMUTEX 0x00010000      /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_SHAREDCACHE 0x00020000    /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_PRIVATECACHE 0x00040000   /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_WAL 0x00080000            /* VFS only */
#define SQLITE_OPEN_NOFOLLOW 0x01000000       /* Ok for sqlite3_open_v2() */

extern "C"
{
    struct zcc_sqlite3;
    struct zcc_sqlite3_stmt;
    extern int sqlite3_config(int op, ...);
    extern int sqlite3_initialize(void);
    extern int sqlite3_shutdown(void);
    extern char *sqlite3_mprintf(const char *zFormat, ...);
    extern int sqlite3_open_v2(const char *filename, struct zcc_sqlite3 **ppDb, int flags, const char *zVfs);
    extern int sqlite3_busy_handler(struct zcc_sqlite3 *db, int (*xBusy)(void *ptr, int count), void *ptr);
    extern int sqlite3_close(struct zcc_sqlite3 *db);
    extern int sqlite3_exec(struct zcc_sqlite3 *db, const char *sql, int (*callback)(void *, int, char **, char **), void *arg, char **errmsg);
    extern int sqlite3_prepare_v2(struct zcc_sqlite3 *db, const char *zSql, int nByte, struct zcc_sqlite3_stmt **ppStmt, const char **pzTail);
    extern int sqlite3_bind_parameter_count(struct zcc_sqlite3_stmt *pStmt);
    extern const char *sqlite3_bind_parameter_name(struct zcc_sqlite3_stmt *pStmt, int index);
    extern int sqlite3_bind_blob64(struct zcc_sqlite3_stmt *pStmt, int index, const void *data, uint64_t n, void (*xDel)(void *));
    extern int sqlite3_bind_text(struct zcc_sqlite3_stmt *pStmt, int index, const char *text, int n, void (*xDel)(void *));
    extern int sqlite3_bind_int64(struct zcc_sqlite3_stmt *pStmt, int index, int64_t value);
    extern int sqlite3_bind_int(struct zcc_sqlite3_stmt *pStmt, int index, int value);
    extern int sqlite3_bind_double(struct zcc_sqlite3_stmt *pStmt, int index, double value);
    extern int sqlite3_step(struct zcc_sqlite3_stmt *pStmt);
    extern int sqlite3_reset(struct zcc_sqlite3_stmt *pStmt);
    extern int sqlite3_finalize(struct zcc_sqlite3_stmt *pStmt);
    extern const void *sqlite3_column_blob(struct zcc_sqlite3_stmt *pStmt, int iCol);
    extern int sqlite3_column_bytes(struct zcc_sqlite3_stmt *pStmt, int iCol);
    extern const unsigned char *sqlite3_column_text(struct zcc_sqlite3_stmt *pStmt, int iCol);
    extern int sqlite3_column_int(struct zcc_sqlite3_stmt *pStmt, int iCol);
    extern int64_t sqlite3_column_int64(struct zcc_sqlite3_stmt *pStmt, int iCol);
    extern double sqlite3_column_double(struct zcc_sqlite3_stmt *pStmt, int iCol);
    extern const char *sqlite3_errmsg(struct zcc_sqlite3 *db);
}
