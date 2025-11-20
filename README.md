# 高并发键值存储系统

## 项目简介

本项目是一个基于C++17开发的高性能键值存储系统，采用现代化的软件架构和高效的数据结构设计。系统实现了完整的客户端-服务器模型，支持多种数据类型操作，具备高并发处理能力和完善的错误处理机制。

## 核心特性

- **高性能网络模型**: 基于epoll的事件驱动架构，支持数千并发连接
- **多数据结构支持**: 字符串(String)和有序集合(Sorted Set)
- **内存安全**: 使用智能指针和RAII技术管理资源
- **异步日志系统**: 后台线程处理日志写入，不影响主业务性能
- **完整协议支持**: 自定义TLV序列化协议，支持复杂数据类型
- **全面测试覆盖**: 单元测试、集成测试、性能测试

## 技术架构

### 系统组件

- **客户端**: 命令行交互界面，支持所有数据操作命令
- **服务器**: 基于epoll的多路复用I/O服务器，高性能事件处理
- **存储引擎**: 
  - 哈希表: 字符串键值对存储
  - 跳表(SkipList): 有序集合实现，O(log n)时间复杂度
- **日志系统**: 异步日志记录，支持多级别日志输出

### 网络协议

采用类型-长度-值(TLV)编码格式：

```
命令请求:
+------+-----+------+-----+------+-----+-----+------+
| nstr | len | str1 | len | str2 | ... | len | strn |
+------+-----+------+-----+------+-----+-----+------+

命令响应:
+-----+---------+
| res | data... |
+-----+---------+
```

## 快速开始

### 环境要求

- Linux 操作系统
- GCC 7.0+ 或 Clang 5.0+ (支持C++17)
- Make 构建工具

### 编译安装

```bash
# 克隆项目
git clone <repository-url>
cd kv_storage

# 编译项目
make kv_storage

# 运行测试
make test
```

### 启动服务

```bash
# 终端1: 启动服务器
./bin/kv_server

# 终端2: 启动客户端
./bin/kv_client
```

## 命令参考

### 字符串操作

| 命令 | 语法 | 描述 | 示例 |
|------|------|------|------|
| keys | `keys` | 列出所有键 | `keys` |
| get | `get str <key>` | 获取字符串值 | `get str username` |
| set | `set str <key> <value>` | 设置字符串值 | `set str username john` |
| del | `del str <key>` | 删除字符串键 | `del str username` |

### 有序集合操作

| 命令 | 语法 | 描述 | 示例 |
|------|------|------|------|
| zadd | `zadd zset <score> <member>` | 添加有序集合成员 | `zadd zset 100 player1` |
| zrem | `zrem zset <member>` | 删除有序集合成员 | `zrem zset player1` |
| zscore | `zscore zset <member>` | 获取成员分数 | `zscore zset player1` |
| zcard | `zcard zset` | 获取集合元素数量 | `zcard zset` |

### 使用示例

```bash
# 连接服务器
./bin/kv_client

# 字符串操作示例
kv> set str user:1001 "Alice"
kv> set str user:1002 "Bob"
kv> get str user:1001
(str) Alice
kv> keys
(arr) len = 2
(str) user:1001
(str) user:1002
(arr) end

# 有序集合操作示例
kv> zadd zset leaderboard 1500 "player_alpha"
kv> zadd zset leaderboard 1800 "player_beta" 
kv> zadd zset leaderboard 1200 "player_gamma"
kv> zscore zset leaderboard player_beta
(int) 1800
kv> zcard zset leaderboard
(int) 3
```

## 项目结构

```
kv_storage/
├── bin/                     # 可执行文件
│   ├── kv_server            # 服务器程序
│   ├── kv_client            # 客户端程序
│   └── test_*               # 测试程序
├── src/
│   ├── client/              # 客户端代码
│   │   └── client_utils.h   # 网络通信工具
│   ├── server/              # 服务器代码
│   │   └── server_utils.h   # 服务器核心逻辑
│   └── utils/
│       ├── asynclog.h       # 异步日志系统
│       └── kv_constant.h    # 常量和配置
├── tests/                   # 测试套件
│   ├── test_skip_list.cpp   # 跳表数据结构测试
│   ├── test_kv_storage.cpp  # 存储引擎测试
│   ├── test_network.cpp     # 网络协议测试
│   └── performance_test.cpp # 性能基准测试
├── doc/
│   └── log.txt              # 运行日志
├── Makefile                 # 构建配置
└── README.md                # 项目文档
```

## 核心技术

### 1. Epoll事件驱动
采用Linux epoll作为I/O多路复用机制，在大量并发连接中保持高性能。服务器使用边缘触发(ET)模式，配合非阻塞I/O，实现高效的网络通信。

### 2. 跳表数据结构
有序集合使用跳表(SkipList)实现，具有以下优势：
- 平均时间复杂度：插入O(log n)、删除O(log n)、查询O(log n)
- 支持范围查询和排序操作
- 实现相对简单，性能稳定

### 3. 异步日志系统
基于生产者-消费者模式的日志架构：
- 日志写入在独立后台线程执行
- 使用条件变量进行线程间同步
- 支持DEBUG、INFO、WARN、ERROR多级别日志
- 日志格式包含时间戳、文件位置、日志内容

### 4. 协议序列化
自定义TLV(Type-Length-Value)协议：
- **类型**: 标识数据类型（NIL、ERR、STR、INT、ARR）
- **长度**: 数据内容长度
- **值**: 实际数据内容

支持嵌套数据结构，易于扩展新的数据类型。

### 5. 连接管理
使用状态机管理客户端连接：
- **STATE_REQ**: 请求读取状态
- **STATE_RES**: 响应写入状态  
- **STATE_END**: 连接结束状态

每个连接维护独立的读写缓冲区，支持请求管道化处理。

## 测试体系

项目包含完整的测试套件，确保代码质量和系统稳定性：

### 单元测试
```bash
# 跳表数据结构测试
make test_skip_list
./bin/test_skip_list

# 存储引擎功能测试
make test_kv_storage  
./bin/test_kv_storage

# 网络协议测试
make test_network
./bin/test_network
```

### 性能测试
```bash
# 性能基准测试
make test_performance
./bin/test_performance
```

### 集成测试
```bash
# 运行所有测试
make test
```

测试覆盖包括：
- 数据结构正确性验证
- 命令处理逻辑测试
- 网络协议兼容性
- 并发安全性检查
- 性能基准测量

## 性能特点

- **高并发支持**: epoll模型支持数千并发连接
- **低延迟**: 内存数据结构，快速响应请求
- **高效内存使用**: 智能指针自动管理资源
- **稳定可靠**: 完善的错误处理和异常恢复

## 扩展开发

### 添加新命令
1. 在`do_request()`函数中添加命令解析逻辑
2. 实现对应的命令处理函数
3. 添加相应的测试用例

### 添加新数据类型
1. 在协议序列化中添加新的类型标识
2. 实现数据的序列化/反序列化逻辑
3. 更新客户端响应处理函数

### 配置调优
通过修改`kv_constant.h`中的常量配置：
- 调整消息大小限制
- 修改超时时间设置
- 配置跳表参数

## 故障排除

### 常见问题

**连接失败**
- 检查服务器是否正常运行
- 确认端口1234未被占用
- 验证防火墙设置

**编译错误**  
- 确认编译器支持C++17标准
- 检查依赖库是否完整安装

**性能问题**
- 调整epoll事件数量配置
- 优化跳表层级参数
- 监控系统资源使用情况

### 日志调试
系统运行日志保存在`doc/log.txt`：
```bash
tail -f doc/log.txt
```

日志包含详细的操作记录和错误信息，便于问题诊断。
