//
// Created by yuanlu on 2022/11/2.
//

#ifndef IFR_OPENCV_SEMAPHORE_H
#define IFR_OPENCV_SEMAPHORE_H

#include<mutex>
#include<condition_variable>

/**
 * 信号量
 */
class semaphore {
public:
    explicit semaphore(long count = 0) : count(count) {}

    /**
     * 请求一个资源
     */
    void wait() {
        std::unique_lock<std::mutex> lock(mx);
        cond.wait(lock, [&]() { return count > 0; });
        --count;
    }

    /**
     * 释放一个资源
     */
    void signal() {
        std::unique_lock<std::mutex> lock(mx);
        ++count;
        cond.notify_one();
    }

private:
    std::mutex mx;
    std::condition_variable cond;
    long count;
};

#endif //IFR_OPENCV_SEMAPHORE_H
