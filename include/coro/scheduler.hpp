#pragma once

#include "config.h"
#include "detail/atomic_helper.hpp" // 必须包含这个文件
#include <atomic>
#include <memory>
#include <thread>
#include <vector>
#ifdef ENABLE_MEMORY_ALLOC
    #include "coro/allocator/memory.hpp"
#endif
#include "coro/dispatcher.hpp"

namespace coro
{

/**
 * @brief scheduler just control context to run and stop,
 * it also use dispatcher to decide which context can accept the task
 *
 */
class scheduler
{
    friend context;

public:
    [[CORO_TEST_USED(lab2b)]] inline static auto init(size_t ctx_cnt = std::thread::hardware_concurrency()) noexcept
        -> void
    {
        if (ctx_cnt == 0)
        {
            ctx_cnt = std::thread::hardware_concurrency();
        }
        get_instance()->init_impl(ctx_cnt);
    }

    /**
     * @brief loop work mode, auto wait all context finish job
     *
     */
    [[CORO_TEST_USED(lab2b)]] inline static auto loop() noexcept -> void { get_instance()->loop_impl(); }

    static inline auto submit(task<void>&& task) noexcept -> void
    {
        auto handle = task.handle();
        task.detach();
        submit(handle);
    }

    static inline auto submit(task<void>& task) noexcept -> void { submit(task.handle()); }

    [[CORO_TEST_USED(lab2b)]] inline static auto submit(std::coroutine_handle<> handle) noexcept -> void
    {
        get_instance()->submit_task_impl(handle);
    }

private:
    static auto get_instance() noexcept -> scheduler*
    {
        static scheduler sc;
        return &sc;
    }

    [[CORO_TEST_USED(lab2b)]] auto init_impl(size_t ctx_cnt) noexcept -> void;

    [[CORO_TEST_USED(lab2b)]] auto loop_impl() noexcept -> void;

    auto stop_impl() noexcept -> void;

    [[CORO_TEST_USED(lab2b)]] auto submit_task_impl(std::coroutine_handle<> handle) noexcept -> void;

    // TODO[lab2b]: Add more function if you need
    // atomic_ref_wrapper 具体定义在 include/detail/atomic_helper.hpp 中
    template<typename T>
    struct alignas(config::kCacheLineSize) atomic_ref_wrapper
    {
        // alignas 是 std::atomic_ref 要求的地址对齐
        alignas(std::atomic_ref<T>::required_alignment) T val;
        // 这里为何不直接用 std::atomic<T>呢？
        // 因为我们要在容器里存储多个该类型，由于 std::atomic<T>禁用了
        // 拷贝构造和拷贝赋值，所以直接用 std::atomic<T>会导致容器无法初始化
    };

    // 上文提到的引用计数
    using stop_token_type = std::atomic<int>;

    // 每个 context 对应的状态，只有 0 和 1 两个值，0 表示 context 已完成所有任务，1 表示 context 还在执行任务中
    using stop_flag_type = std::vector<detail::atomic_ref_wrapper<int>>;

private:
    size_t                                              m_ctx_cnt{0};
    detail::ctx_container                               m_ctxs;
    detail::dispatcher<coro::config::kDispatchStrategy> m_dispatcher;
    // TODO[lab2b]: Add more member variables if you need
    stop_flag_type  m_ctx_stop_flag; // 数组存储各个 context 的执行状态 0 或 1
    stop_token_type m_stop_token;    // 引用计数成员变量 全系统内处于“活跃状态”（即 flag 为 1）的 context 总数

#ifdef ENABLE_MEMORY_ALLOC
    // Memory Allocator
    coro::allocator::memory::memory_allocator<coro::config::kMemoryAllocator> m_mem_alloc;
#endif
};

inline void submit_to_scheduler(task<void>&& task) noexcept
{
    scheduler::submit(std::move(task));
}

inline void submit_to_scheduler(task<void>& task) noexcept
{
    scheduler::submit(task.handle());
}

inline void submit_to_scheduler(std::coroutine_handle<> handle) noexcept
{
    scheduler::submit(handle);
}

}; // namespace coro
