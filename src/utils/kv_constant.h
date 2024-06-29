#ifndef KV_CONSTANT_H
#define KV_CONSTANT_H

#include <cstdint>
constexpr int TIMEOUT_VAL = 5000;
constexpr size_t MAX_MSG = 4096;
constexpr size_t MAX_ARGS = 1024;

constexpr uint32_t STATE_REQ = 0;
constexpr uint32_t STATE_RES = 1;
constexpr uint32_t STATE_END = 2;

constexpr char SERIAL_NIL = '0';
constexpr char SERIAL_ERR = '1';
constexpr char SERIAL_STR = '2';
constexpr char SERIAL_INT = '3';
constexpr char SERIAL_ARR = '4';

constexpr int32_t ERR_UNKNOWN = 1;
constexpr int32_t ERR_TOO_BIG = 2;
constexpr int32_t ERR_TYPE = 3;
constexpr int32_t ERR_ARG = 4;

#endif
