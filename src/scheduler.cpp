#include "coro/scheduler.hpp"

namespace coro
{
auto scheduler::init_impl(size_t ctx_cnt) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    detail::init_meta_info();
    m_ctx_cnt = ctx_cnt;
    m_ctxs    = detail::ctx_container{};
    m_ctxs.reserve(m_ctx_cnt);

    m_ctx_stop_flag = stop_flag_type(m_ctx_cnt, detail::atomic_ref_wrapper<int>{.val = 1});
    m_stop_token    = m_ctx_cnt;
    for (int i = 0; i < m_ctx_cnt; i++)
    {
        m_ctxs.emplace_back(std::make_unique<context>());
    }
    m_dispatcher.init(m_ctx_cnt, &m_ctxs);

#ifdef ENABLE_MEMORY_ALLOC
    coro::allocator::memory::mem_alloc_config config;
    m_mem_alloc.init(config);
    ginfo.mem_alloc = &m_mem_alloc;
#endif
}

auto scheduler::loop_impl() noexcept -> void
{
    // TODO[lab2b]: Add you codes

    for (int i = 0; i < m_ctx_cnt; i++)
    {
        m_ctxs[i]->set_stop_cb(
            [this, i]()
            {
                // 为每个 context 设置一个 stop_cb,当 context 发现自己没有任务可做时就调用这个回调函数,
                // 该回调函数会将对应 context 的 flag 位置 0,
                //  并将 scheduler的引用计数减 1

                // 检测当前的上下文是否再工作 也就是 m_ctx_stop_flag 0 或 1
                // fetch_and(0)把我的状态标记为 0(闲置),并告诉我刚才的状态是多少.”
                // 所以cnt只会是 1 或 0,如果是 1 说明之前是忙碌状态,现在变成闲置了,如果是 0
                auto cnt = std::atomic_ref(this->m_ctx_stop_flag[i].val).fetch_and(0, memory_order_acq_rel);

                // 将 scheduler 的引用计数减 1,如果引用计数降至 0,那么触发 scheduler 发送停止信号
                if (this->m_stop_token.fetch_sub(cnt) == cnt)
                {
                    this->stop_impl();
                }
            });

        m_ctxs[i]->start();
    }

    for (int i = 0; i < m_ctx_cnt; i++)
    {
        m_ctxs[i]->join();
    }
}

auto scheduler::stop_impl() noexcept -> void
{
    // TODO[lab2b]: example function
    // This is an example which just notify stop signal to each context,
    // if you don't need this, function just ignore or delete it
    for (int i = 0; i < m_ctx_cnt; i++)
    {
        m_ctxs[i]->notify_stop();
    }
}

auto scheduler::submit_task_impl(std::coroutine_handle<> handle) noexcept -> void
{
    assert(this->m_stop_token.load(std::memory_order_acquire) != 0 && "error! submit task after scheduler loop finish");
    size_t ctx_id = m_dispatcher.dispatch();

    m_stop_token.fetch_add(
        1 - std::atomic_ref(m_ctx_stop_flag[ctx_id].val).fetch_or(1, memory_order_acq_rel), memory_order_acq_rel);
    m_ctxs[ctx_id]->submit_task(handle);
}
}; // namespace coro
