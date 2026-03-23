#include "coro/engine.hpp"
#include "coro/io/io_info.hpp"
#include "coro/task.hpp"

namespace coro::detail
{
using std::memory_order_relaxed;

auto engine::init() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    m_upxy.init(config::kQueCap);
    linfo.egn      = this;
    m_running_io   = 0;
    m_io_to_submit = 0;
}

auto engine::deinit() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    m_upxy.deinit();
    m_running_io   = 0;
    m_io_to_submit = 0;
    // coroutine_handle<> handle;
    // while (m_task_queue.try_pop(handle))
    // {
    //     handle.destroy();
    // }
}

auto engine::ready() noexcept -> bool
{
    // TODO[lab2a]: Add you codes
    return m_task_queue.was_size() > 0;
}

auto engine::get_free_urs() noexcept -> ursptr
{
    // TODO[lab2a]: Add you codes
    return m_upxy.get_free_sqe();
}

auto engine::num_task_schedule() noexcept -> size_t
{
    // TODO[lab2a]: Add you codes
    return m_task_queue.was_size();
}

auto engine::schedule() noexcept -> coroutine_handle<>
{
    // TODO[lab2a]: Add you codes
    return m_task_queue.pop();
}

auto engine::submit_task(coroutine_handle<> handle) noexcept -> void
{
    // TODO[lab2a]: Add you codes
    m_task_queue.push(handle);
    m_upxy.write_eventfd(1);
    // wake_up();
}

auto engine::exec_one_task() noexcept -> void
{
    auto coro = schedule();
    coro.resume();
    if (coro.done())
    {
        clean(coro);
    }
}

auto engine::handle_cqe_entry(urcptr cqe) noexcept -> void
{
    auto data = reinterpret_cast<io::detail::io_info*>(io_uring_cqe_get_data(cqe));
    data->cb(data, cqe->res);
}

auto engine::poll_submit() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    auto submit = [&]()
    {
        if (m_io_to_submit > 0)
        {
            int submitted_count = m_upxy.submit();
            m_running_io.fetch_add(m_io_to_submit);
            // m_io_to_submit.fetch_sub(submitted_count);
            m_io_to_submit = 0;
        }
    };
    submit();
    // 只有在没有计算任务,但有IO在运行时,才等待
    auto cnt = m_upxy.wait_eventfd(); // 等待 IO 执行

    if (m_task_queue.was_size() == 0 && m_running_io > 0)
    {
        m_upxy.wait_eventfd();
    }

    // 获取已经完成的状态,然后减去

    // auto num =            m_upxy.peek_batch_cqe(m_urc.data(), m_num_io_running.load(std::memory_order_acquire));

    int completed_count = m_upxy.peek_batch_cqe(m_urc.data(), m_running_io);

    if (completed_count > 0)
    {
        m_running_io.fetch_sub(completed_count);

        for (int i = 0; i < completed_count; ++i)
        {
            handle_cqe_entry(m_urc[i]);
        }
        m_upxy.cq_advance(completed_count);
        m_running_io = m_running_io - completed_count;
    }
}

auto engine::add_io_submit() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    m_io_to_submit.fetch_add(1);
}

auto engine::empty_io() noexcept -> bool
{
    // TODO[lab2a]: Add you codes
    if (m_running_io == 0 and m_io_to_submit == 0)
    {
        return true;
    }

    return false;
}
}; // namespace coro::detail
