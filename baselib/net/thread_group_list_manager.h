#ifndef NET_THREAD_GROUP_LIST_MANAGER_H_
#define NET_THREAD_GROUP_LIST_MANAGER_H_

#include <thread>

#include <atomic>
#include <string>
#include <thread>

#include <folly/Conv.h>
#include <folly/Range.h>
#include <folly/ThreadName.h>
#include <folly/Singleton.h>
#include <folly/io/async/EventBase.h>

#include <wangle/concurrent/ThreadFactory.h>
#include <wangle/concurrent/IOThreadPoolExecutor.h>

#include "base/configurable.h"

/////////////////////////////////////////////////////////////////////////////
//inline size_t GetIOThreadSize() {
//    auto threads = std::thread::hardware_concurrency();
//    if (threads <= 0) {
//        // Reasonable mid-point for concurrency when actual value unknown
//        threads = 8;
//    }
//    return threads;
//}

/////////////////////////////////////////////////////////////////////////////
// 线程描述:
//  线程类型
//  线程ID
// accept(独立)
// client(独立)
// conn(accept/clinent混合), 先检查conn, 如果conn设置则accept和client失效
// fiber
// db
// redis
enum class ThreadType : int {
    NORMAL = 1,         // 普通线程
    CONN_ACCEPT = 2,    // Accept, 需要EventBase支持
    CONN_CLIENT = 4,    // Client, 需要EventBase支持
    CONN = 6,           // Accept|Client, 需要EventBase支持
    FIBER = 8,          // 协程, 需要EventBase支持
    DB = 16,            // DB
    REDIS = 32,         // REDIS
    MAX = 33,
};

inline const char* ToString(ThreadType thread_type) {
    const char* s = "UNKNOWN";
    
    switch(thread_type) {
        case ThreadType::NORMAL:
            s = "NORMAL";
            break;
        case ThreadType::CONN_ACCEPT:
            s = "CONN_ACCEPT";
            break;
        case ThreadType::CONN_CLIENT:
            s = "CONN_CLIENT";
            break;
        case ThreadType::CONN:
            s = "CONN";
            break;
        case ThreadType::FIBER:
            s = "FIBER";
            break;
        case ThreadType::DB:
            s = "DB";
            break;
        case ThreadType::REDIS:
            s = "REDIS";
            break;
        default:
            break;
    }
    
    return s;
}
    
typedef int ThreadID;

struct ThreadInfo {
    int thread_type;
    int thread_id;
};

// struct ThreadData;
class ThreadGroup;
typedef std::function<size_t(ThreadGroup*, wangle::ThreadPoolExecutor::ThreadHandle*)> NewThreadDataCallback;
    
// class ThreadGroup;
//
// 需求:
// IOThread（Accept/Client/Accept_Clinet）支持thread_id
// IOThreadPoolExceutor
// CPUThreadPoolExecutor
// 网络IO(IOThreadConnManager)
class ThreadGroup {
public:
    class ThreadGroupObserver : public wangle::ThreadPoolExecutor::Observer {
    public:
        explicit ThreadGroupObserver(ThreadGroup* thread_group, const NewThreadDataCallback& cb)
            : thread_group_(thread_group),
              callback_(cb) {
        }
        
        // Impl from Observer
        void threadStarted(wangle::ThreadPoolExecutor::ThreadHandle* handle) override;
        void threadStopped(wangle::ThreadPoolExecutor::ThreadHandle* handle) override;

    private:
        ThreadGroup* thread_group_;
        NewThreadDataCallback callback_;
    };
    
    ThreadGroup(std::shared_ptr<wangle::ThreadPoolExecutor> pool, ThreadType thread_type, const NewThreadDataCallback& cb);
    ~ThreadGroup() = default;

    ThreadType GetThreadType() const {
        return thread_type;
    }
    
    std::shared_ptr<wangle::ThreadPoolExecutor> GetThreadPool() const {
        return thread_pool;
    }
    
    std::string ToString() const {
        return "";
    }
    
    std::shared_ptr<wangle::ThreadPoolExecutor> thread_pool;   // IO, CPU
    ThreadType thread_type {ThreadType::NORMAL};
    std::vector<size_t> thread_idxs;

private:
    friend struct ThreadData;
    std::shared_ptr<ThreadGroupObserver> thread_observer_;
};

typedef std::shared_ptr<ThreadGroup> ThreadGroupPtr;
    
struct ThreadData {
    ThreadData(ThreadGroup* group, size_t idx, folly::EventBase* eb = nullptr)
        : thread_group(group), thread_idx(idx), evb(eb) {}
    
    ThreadGroup* thread_group {nullptr};    // 所属线程组
    // ThreadType thread_type;         //
    size_t thread_idx {0};               // 线程ID
    folly::EventBase* evb {nullptr};          // evb
};


// 线程管理器
// IOThreadConnManager
// CPUThreadManager
//
// ThreadPoolExecutor
// 1: 单线程（ACCEPT和CONN都使用一个线程）
// 0: 4个线程（连接），其它的多线程
// 未指定
//
//
//std::shared_ptr<wangle::ThreadPoolExecutor> MakeThreadGroup(ThreadType thread_type, int thread_size) {
//}

    
// thread_pool配置
/**
 "thread_pool" : {
    "accept" : 0,
    "client" : 4,
    "conn"   : 4,
    "fiber"  : 4,
    "db"     : 4,
    "redis"  : 4
 }
 */
// 初始化线程列表
struct ThreadGroupListOption : public Configurable {
    
    // 先检查conn，如果conn已经设置则accept和client无效
    bool SetConf(const std::string& conf_name, const nlohmann::json& conf) override;
    
    struct ThreadGroupOption {
        ThreadGroupOption(ThreadType type, int size)
            : thread_type(type), thread_size(size) {}
        
        ThreadType thread_type;
        int thread_size;
    };
    
    std::vector<ThreadGroupOption> options;
};
    

class ThreadGroupListManager {
public:
    explicit ThreadGroupListManager(const ThreadGroupListOption& options);
    ~ThreadGroupListManager() = default;
    
    // 通过ThreadID找evb
    inline folly::EventBase* GetEventBaseByThreadID(size_t idx) const {
        return thread_datas_[idx].evb;
    }

    // 通过ThreadType获取IOThreadPoolExecutor
    std::shared_ptr<wangle::IOThreadPoolExecutor> GetIOThreadPoolExecutor(ThreadType thread_type) const;
    
    // 通过ThreadType找
    ThreadGroupPtr GetThreadGroupByThreadType(ThreadType thread_type) const {
        return thread_groups_[static_cast<size_t>(thread_type)];
    }

    folly::EventBase* GetEventBaseByThreadType(ThreadType thread_type) const;

    const std::vector<ThreadData>& thread_datas() const {
        return thread_datas_;
    }

private:
    bool Initialize(const ThreadGroupListOption& options);
    ThreadGroupPtr MakeThreadGroup(ThreadType thread_type, int thread_size);

    std::vector<ThreadGroupPtr> thread_groups_;
    std::vector<ThreadData> thread_datas_;
};
    
#endif
