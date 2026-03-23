#include "coro/engine.hpp"
#include "coro/io/io_info.hpp"
#include "coro/task.hpp"

namespace coro::detail
{
using std::memory_order_relaxed;

// engine 的初始化函数
auto engine::init() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    // 初始化 io_uring 代理,设置队列容量
    m_upxy.init(config::kQueCap);
    // 将当前 engine 实例的指针关联到当前线程的 linfo.egn
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
    // 1. 将协程句柄加入到待执行的任务队列
    m_task_queue.push(handle);

    // 2. 唤醒 engine 工作线程
    //    如果 engine 正阻塞在 wait_eventfd(),通过向 eventfd 写入值来唤醒它,
    //    使其能够检测到新加入的任务并继续执行.
    m_upxy.write_eventfd(1);
    // wake_up(); // 官方答案的封装,功能同上
}

auto engine::exec_one_task() noexcept -> void
{
    // 从任务队列获取一个协程句柄,然后恢复协程的执行 如果协程执行完毕,进行清理
    auto coro = schedule();
    coro.resume();
    if (coro.done())
    {
        clean(coro);
    }
}

// handle_cqe_entry() 函数:处理单个 io_uring 完成CQE
auto engine::handle_cqe_entry(urcptr cqe) noexcept -> void
{
    // 从 CQE 中获取用户数据(一个 io_info 指针,内含回调函数)
    auto data = reinterpret_cast<io::detail::io_info*>(io_uring_cqe_get_data(cqe));
    // 调用回调函数,传递 IO 结果,该回调将负责恢复因 IO 而挂起的协程
    data->cb(data, cqe->res);
}

// poll_submit() 函数:engine 的核心 I/O 调度与处理函数
auto engine::poll_submit() noexcept -> void
{
    // 1. 提交 IO:检查是否有待提交的 IO,并将其发送给内核
    auto submit = [&]()
    {
        if (m_io_to_submit > 0) // 检查是否有 IO 请求待提交
        {
            // 调用 io_uring 代理提交 IO 请求给内核,返回实际提交的数量
            int submitted_count = m_upxy.submit();

            if (submitted_count > 0) // 如果有 IO 被成功提交
            {
                // 将实际提交的 IO 数量加到 运行中 的计数器
                m_running_io.fetch_add(submitted_count);

                // 从“待提交”计数器中减去实际提交的数量
                m_io_to_submit -= submitted_count;
            }
        }
    };
    submit(); // 执行提交操作

    // 2. 等待事件(阻塞)
    //    阻塞等待,直到有事件发生(IO 完成或被唤醒),避免 CPU 空转.
    auto cnt = m_upxy.wait_eventfd();

    // 3. 处理完成的 IO
    //    检查是否有 IO 操作已经完成
    //    peek_batch_cqe 尝试从完成队列(CQ)获取已完成的 IO 结果(CQEs)
    int completed_count = m_upxy.peek_batch_cqe(m_urc.data(), m_running_io.load());

    if (completed_count > 0)
    {
        // 更新计数器:已完成的 IO 不再计入 running
        m_running_io.fetch_sub(completed_count);

        // 循环处理每一个已完成的 IO 条目 (CQE)
        for (int i = 0; i < completed_count; ++i)
        {
            // 调用 handle_cqe_entry 处理单个 CQE,恢复对应的协程
            handle_cqe_entry(m_urc[i]);
        }

        // 通知 io_uring 这些 CQE 已经被处理,内核可以重用 CQ 空间
        m_upxy.cq_advance(completed_count);
    }
}

auto engine::add_io_submit() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    m_io_to_submit.fetch_add(1);
}

// empty_io() 函数:判断当前是否没有任何 I/O 任务(待提交或运行中)
auto engine::empty_io() noexcept -> bool
{
    // TODO[lab2a]: Add you codes
    if (m_running_io == 0 && m_io_to_submit == 0)
    {
        return true;
    }

    return false;
}
}; // namespace coro::detail