#include "coro/context.hpp"
#include "coro/scheduler.hpp"

namespace coro
{
context::context() noexcept
{
    m_id = ginfo.context_id.fetch_add(1, std::memory_order_relaxed);
}

auto context::init() noexcept -> void
{
    // TODO[lab2b]: Add you codes

    linfo.ctx = this;
    m_engine.init();
}

auto context::deinit() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    linfo.ctx = {nullptr};
    m_engine.deinit();
}

auto context::start() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_job = make_unique<jthread>(
        [this](stop_token token)
        {
            this->init();
            // 如果外部没有注入 stop_cb,那么自行为其添加逻辑
            if (!(this->m_stop_cb))
            {
                m_stop_cb = [&]() { m_job->request_stop(); };
            }
            this->run(token);
            this->deinit();
        });
}

auto context::notify_stop() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_job->request_stop();
    m_engine.wake_up();
}

auto context::submit_task(std::coroutine_handle<> handle) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_engine.submit_task(handle);
}

auto context::register_wait(int register_cnt) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_num_wait_task.fetch_add((size_t)register_cnt);
}

auto context::unregister_wait(int register_cnt) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_num_wait_task.fetch_sub((size_t)register_cnt);
}

auto context::process_work() noexcept -> void
{
    int num = m_engine.num_task_schedule();
    for (size_t i = 0; i < num; ++i)
    {
        m_engine.exec_one_task();
    }
}

auto context::poll_work() noexcept -> void
{
    m_engine.poll_submit();
}
// 没有 io 任务了就停止

auto context::run(stop_token token) noexcept -> void
{
    // TODO[lab2b]: Add you codes

    // 这也是要用来解读和对比的之前的代码，因为下面的部分是最终的写法，这个是暂停的。
    //  因为测试的代码在 scheduler 之前的测试没有调用 notify_stop,所以这里的 run 不能直接以 stop_token
    //  来控制循环,而是要自己判断什么时候停止 while (1)
    //  {
    //      process_work();

    //     // 只要 1. 等待的任务为0且没有IO (empty_all)
    //     // 且 2. 当前任务队列为空 (!m_engine.ready())
    //     // 就不管有没有 stop_token,直接退出循环
    //     if (empty_all() && !m_engine.ready())
    //     {
    //         break;
    //     }

    //     poll_work();

    //     // 唤醒后再次检查
    //     if (empty_all() && !m_engine.ready())
    //     {
    //         break;
    //     }
    // }

    while (!token.stop_requested())
    {
        process_work();
        if (empty_all())
        {
            if (!m_engine.ready())
            {
                // 此处表明 contetx 已执行完所有任务,那么调用停止逻辑
                m_stop_cb();
            }
            else
            {
                continue;
            }
        }

        poll_work();
    }
}
}; // namespace coro