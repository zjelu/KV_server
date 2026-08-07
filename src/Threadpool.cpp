// the constructor just launches some amount of workers

inline ThreadPool::ThreadPool(size_t threads)
    :   stop(false)
{
    for(size_t i = 0;i<threads;++i)
        workers.emplace_back(
            [this]
            {
                for(;;)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                            [this]{ return this->stop || !this->tasks.empty(); });
                        if(this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            }
        );
}

// add new work item to the pool
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) //F是函数，args是参数
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    auto bound = std::bind(
    std::forward<F>(f),
    std::forward<Args>(args)...
    );

    using BoundType = decltype(bound);
    using ReturnType = std::invoke_result_t<F, Args...>;
    using PackagedType = std::packaged_task<ReturnType()>;

    auto task = std::make_shared<PackagedType>(
        std::move(bound)
    );
        
    std::future<ReturnType> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // don't allow enqueueing after stopping the pool
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([task](){ (*task)(); });
       
        /*这行代码只是类似于事件监听器，只挂了一个函数，没有调用它
        工作线程执行 task(); 
        ↓
        (调用) 第 6 层 std::function 的底层触发
        ↓
        (激活) 第 5 层 Lambda 表达式的包裹：执行 { (*task)(); }
        ↓
        (解引用) 通过第 4 层 shared_ptr 找到真实的 packaged_task
        ↓
        (触发) 第 3 层 std::packaged_task 的 operator()
        ↓
        (运行) 第 2 层 std::bind 绑定的环境
        ↓
        (最终执行) 第 1 层 你真正的函数 f(args...)
        ↓
        [计算出结果] -> 自动通过 packaged_task 传回给主线程的 std::future
        */
    }
    condition.notify_one();
    return res;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for(std::thread &worker: workers)
        worker.join();
}
