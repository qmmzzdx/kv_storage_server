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
    enum LogLevel
    {
        DEBUG, INFO, WARN, ERROR
    };

    struct LogTask
    {
        LogTask() {}
        LogTask(const LogTask& t) : log_level(t.log_level), qlog(t.qlog) {}
        LogTask(const LogTask&& t) : log_level(t.log_level), qlog(std::move(t.qlog)) {}
        LogLevel log_level;
        std::queue<std::any> qlog;
    };

    class AsyncLog
    {
    public:
        static AsyncLog& Instance()
        {
            static AsyncLog instance;
            return instance;
        }

        ~AsyncLog()
        {
            Close();
            if (log_thread.joinable()) { log_thread.join(); }
            fout << "[INFO] [";
            RecordCurrentTime();
            fout << "]: Exit async logging......\n";
            fout.close();
        }

        void Close()
        {
            running = false;
            data_cond.notify_one();
        }

        template <typename Arg>
        void PushTask(std::shared_ptr<LogTask> task, Arg&& arg)
        {
            task->qlog.push(std::any(arg));
        }

        template <typename Arg, typename ...Args>
        void PushTask(std::shared_ptr<LogTask> task, Arg&& arg, Args&& ...args)
        {
            task->qlog.push(std::any(arg));
            PushTask(task, std::forward<Args>(args)...);
        }

        template <typename ...Args>
        void AsyncWrite(LogLevel level, Args&& ...args)
        {
            auto task = std::make_shared<LogTask>();
            PushTask(task, args...);
            task->log_level = level;

            std::unique_lock<std::mutex> lk(mtx);
            mq.push(task);
            lk.unlock();
            if (!mq.empty())
            {
                data_cond.notify_one();
            }
        }

    protected:
        void RecordCurrentTime()
        {
            const auto now_time = std::chrono::system_clock::now();
            const std::time_t t = std::chrono::system_clock::to_time_t(now_time);
            std::string&& curtime = std::string(std::ctime(&t));
            curtime.pop_back();
            fout << curtime;
        }

    private:
        bool running;
        std::ofstream fout;
        std::mutex mtx;
        std::thread log_thread;
        std::queue<std::shared_ptr<LogTask>> mq;
        std::condition_variable data_cond;

        AsyncLog(const AsyncLog&) = delete;
        AsyncLog& operator=(const AsyncLog&) = delete;

        AsyncLog() : running(true)
        {
            fout.open("./doc/log.txt", std::ios::app);
            log_thread = std::thread([this] {
                for (;;)
                {
                    std::unique_lock<std::mutex> lk(mtx);
                    data_cond.wait(lk, [this] { return !mq.empty() || !running; });
                    if (!running)
                    {
                        return;
                    }
                    auto log_task = mq.front();
                    mq.pop();
                    lk.unlock();
                    ProcessTask(log_task);
                }
            });
        }

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
                return false;
            }
            str = oss.str();
            return true;
        }

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
                    return;
                }
                size_t target_pos = res.find(spliter, pos);
                if (target_pos != std::string::npos)
                {
                    res.replace(target_pos, spliter.size(), sre);
                    pos = target_pos + spliter.size();
                }
                else
                {
                    res += " " + sre;
                }
            };
            (ReplaceString("{}", args), ...);
            return res;
        }

        void ProcessTask(std::shared_ptr<LogTask> task)
        {
            if (task->qlog.empty())
            {
                return;
            }
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
            RecordCurrentTime();
            fout << "]: ";
            auto qhead = task->qlog.front();
            task->qlog.pop();
            std::string fstr;
            if (!AnyToStr(qhead, fstr))
            {
                return;
            }
            while (!(task->qlog.empty()))
            {
                auto val = task->qlog.front();
                fstr = FormatString(fstr, val);
                task->qlog.pop();
            }
            fout << fstr;
        }
    };

    template <typename ...Args>
    void LOG_DEBUG(Args&& ...args)
    {
        AsyncLog::Instance().AsyncWrite(LogLevel::DEBUG, std::forward<Args>(args)...);
    }

    template <typename ...Args>
    void LOG_INFO(Args&& ...args)
    {
        AsyncLog::Instance().AsyncWrite(LogLevel::INFO, std::forward<Args>(args)...);
    }

    template <typename ...Args>
    void LOG_WARN(Args&& ...args)
    {
        AsyncLog::Instance().AsyncWrite(LogLevel::WARN, std::forward<Args>(args)...);
    }

    template <typename ...Args>
    void LOG_ERROR(Args&& ...args)
    {
        AsyncLog::Instance().AsyncWrite(LogLevel::ERROR, std::forward<Args>(args)...);
    }
};

#endif
