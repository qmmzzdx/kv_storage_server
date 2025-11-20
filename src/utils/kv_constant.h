#ifndef KV_CONSTANT_H
#define KV_CONSTANT_H

#include <cstdint>

// 网络相关常量
constexpr int TIMEOUT_VAL = 5000;        // epoll等待超时时间（毫秒）
constexpr size_t MAX_MSG = 4096;         // 最大消息长度（字节）
constexpr size_t MAX_ARGS = 1024;        // 最大命令参数数量

// 连接状态常量
constexpr uint32_t STATE_REQ = 0;        // 请求读取状态
constexpr uint32_t STATE_RES = 1;        // 响应写入状态
constexpr uint32_t STATE_END = 2;        // 连接结束状态

// 序列化类型标识常量
constexpr char SERIAL_NIL = '0';         // 空值类型
constexpr char SERIAL_ERR = '1';         // 错误类型
constexpr char SERIAL_STR = '2';         // 字符串类型
constexpr char SERIAL_INT = '3';         // 整数类型
constexpr char SERIAL_ARR = '4';         // 数组类型

// 错误码常量
constexpr int32_t ERR_UNKNOWN = 1;       // 未知命令错误
constexpr int32_t ERR_TOO_BIG = 2;       // 消息过大错误
constexpr int32_t ERR_TYPE = 3;          // 类型错误
constexpr int32_t ERR_ARG = 4;           // 参数错误

#endif
