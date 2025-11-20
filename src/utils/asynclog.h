#ifndef ASYNCLOG_H
#define ASYNCLOG_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <any>
#include <queue>
#include <mutex>
#include <thread>
#include <chrono>
#include <typeinfo>
#include <condition_variable>

namespace AsyncLog
{
    // 日志级别枚举
    enum LogLevel
    {
        DEBUG, INFO, WARN, ERROR
    };

    // 日志任务结构体
    struct LogTask
    {
        LogTask() {}
        LogTask(const LogTask& t) : log_level(t.log_level), qlog(t.qlog) {}
        LogTask(const LogTask&& t) : log_level(t.log_level), qlog(std::move(t.qlog)) {}
        LogLevel log_level;              // 日志级别
        std::queue<std::any> qlog;       // 日志参数队列
    };

    // 异步日志类
    class AsyncLog
    {
    public:
        // 获取单例实例
        static AsyncLog& Instance()
        {
            static AsyncLog instance;
            return instance;
        }

        // 析构函数，等待日志线程结束
        ~AsyncLog()
        {
            if (log_thread.joinable()) { log_thread.join(); }
        }

        // 关闭日志系统
        void Close()
        {
            running = false;
            data_cond.notify_one();  // 唤醒日志线程
            fout << "[INFO] [";
            RecordCurrentTime();
            fout << "]: Exit async logging......\n" << std::flush;
            fout.close();
        }

        // 将参数推入日志任务队列
        template <typename Arg>
        void PushTask(std::shared_ptr<LogTask> task, Arg&& arg)
        {
            task->qlog.push(std::any(arg));
        }

        // 可变参数模板：将多个参数推入日志任务队列
        template <typename Arg, typename ...Args>
        void PushTask(std::shared_ptr<LogTask> task, Arg&& arg, Args&& ...args)
        {
            task->qlog.push(std::any(arg));
            PushTask(task, std::forward<Args>(args)...);
        }

        // 异步写入日志
        template <typename ...Args>
        void AsyncWrite(LogLevel level, Args&& ...args)
        {
            auto task = std::make_shared<LogTask>();
            PushTask(task, args...);  // 将参数推入队列
            task->log_level = level;

            std::unique_lock<std::mutex> lk(mtx);
            mq.push(task);
            lk.unlock();
            if (!mq.empty())
            {
                data_cond.notify_one();  // 通知日志线程有新任务
            }
        }

    protected:
        // 记录当前时间到输出流
        void RecordCurrentTime()
        {
            const auto now_time = std::chrono::system_clock::now();
            const std::time_t t = std::chrono::system_clock::to_time_t(now_time);
            std::string&& curtime = std::string(std::ctime(&t));
            curtime.pop_back();  // 移除换行符
            fout << curtime;
        }

    private:
        bool running;                                    // 运行标志
        std::ofstream fout;                              // 输出文件流
        std::mutex mtx;                                  // 互斥锁
        std::thread log_thread;                          // 日志线程
        std::queue<std::shared_ptr<LogTask>> mq;         // 任务队列
        std::condition_variable data_cond;               // 条件变量

        AsyncLog(const AsyncLog&) = delete;              // 禁止拷贝
        AsyncLog& operator=(const AsyncLog&) = delete;   // 禁止赋值

        // 私有构造函数
        AsyncLog() : running(true)
        {
            fout.open("./doc/log.txt", std::ios::out | std::ios::app);  // 打开日志文件
            log_thread = std::thread([this] {
                for (;;)
                {
                    std::unique_lock<std::mutex> lk(mtx);
                    // 等待任务或停止信号
                    data_cond.wait(lk, [this] { return !mq.empty() || !running; });
                    if (!running)
                    {
                        return;  // 停止运行
                    }
                    auto log_task = mq.front();
                    mq.pop();
                    lk.unlock();
                    ProcessTask(log_task);  // 处理日志任务
                }
            });
        }

        // 将any类型转换为字符串
        bool AnyToStr(const std::any& val, std::string& str)
        {
            std::ostringstream oss;
            if (val.type() == typeid(int))
            {
                oss << std::any_cast<int>(val);
            }
            else if (val.type() == typeid(float))
            {
                oss << std::any_cast<float>(val);
            }
            else if (val.type() == typeid(double))
            {
                oss << std::any_cast<double>(val);
            }
            else if (val.type() == typeid(std::string))
            {
                oss << std::any_cast<std::string>(val);
            }
            else if (val.type() == typeid(char*))
            {
                oss << std::any_cast<char*>(val);
            }
            else if (val.type() == typeid(const char*))
            {
                oss << std::any_cast<const char*>(val);
            }
            else
            {
                return false;  // 不支持的类型
            }
            str = oss.str();
            return true;
        }

        // 格式化字符串，替换占位符
        template <typename ...Args>
        std::string FormatString(const std::string& fstr, Args ...args)
        {
            size_t pos = 0;
            std::string res(fstr);

            auto ReplaceString = [&](const std::string& spliter, const std::any& replacement)
            {
                std::string sre;
                if (!AnyToStr(replacement, sre))
                {
                    return;  // 转换失败，跳过
                }
                size_t target_pos = res.find(spliter, pos);
                if (target_pos != std::string::npos)
                {
                    res.replace(target_pos, spliter.size(), sre);  // 替换占位符
                    pos = target_pos + spliter.size();
                }
                else
                {
                    res += " " + sre;  // 未找到占位符，追加到末尾
                }
            };
            (ReplaceString("{}", args), ...);  // 折叠表达式处理所有参数
            return res;
        }

        // 处理单个日志任务
        void ProcessTask(std::shared_ptr<LogTask> task)
        {
            if (task->qlog.empty())
            {
                return;  // 空任务，直接返回
            }
            // 根据日志级别输出前缀
            switch (task->log_level)
            {
                case LogLevel::DEBUG:
                {
                    fout << "[DEBUG] [";
                    break;
                }
                case LogLevel::INFO:
                {
                    fout << "[INFO] [";
                    break;
                }
                case LogLevel::WARN:
                {
                    fout << "[WARN] [";
                    break;
                }
                case LogLevel::ERROR:
                {
                    fout << "[ERROR] [";
                    break;
                }
            }
            RecordCurrentTime();  // 记录时间戳
            fout << "]: ";
            auto qhead = task->qlog.front();
            task->qlog.pop();
            std::string fstr;
            if (!AnyToStr(qhead, fstr))
            {
                return;  // 转换失败，跳过
            }
            // 处理剩余参数
            while (!(task->qlog.empty()))
            {
                auto val = task->qlog.front();
                fstr = FormatString(fstr, val);  // 格式化字符串
                task->qlog.pop();
            }
            fout << fstr;  // 输出最终日志内容
        }
    };

    // 日志宏定义

    // DEBUG级别日志
    template <typename ...Args>
    void LOG_DEBUG(Args&& ...args)
    {
        AsyncLog::Instance().AsyncWrite(LogLevel::DEBUG, std::forward<Args>(args)...);
    }

    // INFO级别日志
    template <typename ...Args>
    void LOG_INFO(Args&& ...args)
    {
        AsyncLog::Instance().AsyncWrite(LogLevel::INFO, std::forward<Args>(args)...);
    }

    // WARN级别日志
    template <typename ...Args>
    void LOG_WARN(Args&& ...args)
    {
        AsyncLog::Instance().AsyncWrite(LogLevel::WARN, std::forward<Args>(args)...);
    }

    // ERROR级别日志
    template <typename ...Args>
    void LOG_ERROR(Args&& ...args)
    {
        AsyncLog::Instance().AsyncWrite(LogLevel::ERROR, std::forward<Args>(args)...);
    }
};

#endif
