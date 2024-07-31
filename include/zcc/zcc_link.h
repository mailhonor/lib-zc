/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2016-01-15
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_MLINK___
#define ZCC_LIB_INCLUDE_MLINK___

#include "./zcc_stdlib.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

/* mlink ############################################################ */
// 一组宏, 可实现, 栈, 链表等
// head: 头; tail: 尾; node:节点变量; prev:head/tail 指向的struct成员的属性"前一个"

// 追加node到尾部
#define ZCC_MLINK_APPEND(head, tail, node, prev, next)                \
    {                                                                 \
        auto _head_1106 = head, _tail_1106 = tail, _node_1106 = node; \
        if (_head_1106 == 0)                                          \
        {                                                             \
            _node_1106->prev = _node_1106->next = 0;                  \
            _head_1106 = _tail_1106 = _node_1106;                     \
        }                                                             \
        else                                                          \
        {                                                             \
            _tail_1106->next = _node_1106;                            \
            _node_1106->prev = _tail_1106;                            \
            _node_1106->next = 0;                                     \
            _tail_1106 = _node_1106;                                  \
        }                                                             \
        head = _head_1106;                                            \
        tail = _tail_1106;                                            \
    }

/* 追加node到首部 */
#define ZCC_MLINK_PREPEND(head, tail, node, prev, next)               \
    {                                                                 \
        auto _head_1106 = head, _tail_1106 = tail, _node_1106 = node; \
        if (_head_1106 == 0)                                          \
        {                                                             \
            _node_1106->prev = _node_1106->next = 0;                  \
            _head_1106 = _tail_1106 = _node_1106;                     \
        }                                                             \
        else                                                          \
        {                                                             \
            _head_1106->prev = _node_1106;                            \
            _node_1106->next = _head_1106;                            \
            _node_1106->prev = 0;                                     \
            _head_1106 = _node_1106;                                  \
        }                                                             \
        head = _head_1106;                                            \
        tail = _tail_1106;                                            \
    }

/* 插入node到before前 */
#define ZCC_MLINK_ATTACH_BEFORE(head, tail, node, prev, next, before)                        \
    {                                                                                        \
        auto _head_1106 = head, _tail_1106 = tail, _node_1106 = node, _before_1106 = before; \
        if (_head_1106 == 0)                                                                 \
        {                                                                                    \
            _node_1106->prev = _node_1106->next = 0;                                         \
            _head_1106 = _tail_1106 = _node_1106;                                            \
        }                                                                                    \
        else if (_before_1106 == 0)                                                          \
        {                                                                                    \
            _tail_1106->next = _node_1106;                                                   \
            _node_1106->prev = _tail_1106;                                                   \
            _node_1106->next = 0;                                                            \
            _tail_1106 = _node_1106;                                                         \
        }                                                                                    \
        else if (_before_1106 == _head_1106)                                                 \
        {                                                                                    \
            _head_1106->prev = _node_1106;                                                   \
            _node_1106->next = _head_1106;                                                   \
            _node_1106->prev = 0;                                                            \
            _head_1106 = _node_1106;                                                         \
        }                                                                                    \
        else                                                                                 \
        {                                                                                    \
            _node_1106->prev = _before_1106->prev;                                           \
            _node_1106->next = _before_1106;                                                 \
            _before_1106->prev->next = _node_1106;                                           \
            _before_1106->prev = _node_1106;                                                 \
        }                                                                                    \
        head = _head_1106;                                                                   \
        tail = _tail_1106;                                                                   \
    }

/* 去掉节点node */
#define ZCC_MLINK_DETACH(head, tail, node, prev, next)                \
    {                                                                 \
        auto _head_1106 = head, _tail_1106 = tail, _node_1106 = node; \
        if (_node_1106->prev)                                         \
        {                                                             \
            _node_1106->prev->next = _node_1106->next;                \
        }                                                             \
        else                                                          \
        {                                                             \
            _head_1106 = _node_1106->next;                            \
        }                                                             \
        if (_node_1106->next)                                         \
        {                                                             \
            _node_1106->next->prev = _node_1106->prev;                \
        }                                                             \
        else                                                          \
        {                                                             \
            _tail_1106 = _node_1106->prev;                            \
        }                                                             \
        head = _head_1106;                                            \
        tail = _tail_1106;                                            \
    }

#define ZCC_MLINK_CONCAT(head_1, tail_1, head_2, tail_2, prev, next)                             \
    {                                                                                            \
        auto _head_1106 = head_1, _tail_1106 = tail_1, _head_2206 = head_2, _tail_2206 = tail_2; \
        if (_head_2206)                                                                          \
        {                                                                                        \
            if (_head_1106)                                                                      \
            {                                                                                    \
                _tail_1106->next = _head_2206;                                                   \
                _head_2206->prev = _tail_1106;                                                   \
            }                                                                                    \
            else                                                                                 \
            {                                                                                    \
                _head_1106 = _head_2206;                                                         \
            }                                                                                    \
            _tail_1106 = _tail_2206;                                                             \
        }                                                                                        \
        head_1 = _head_1106;                                                                     \
        tail_1 = _tail_1106;                                                                     \
    }
zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_MLINK___
