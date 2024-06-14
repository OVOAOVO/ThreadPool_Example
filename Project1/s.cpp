#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <condition_variable>
#include <queue>
#include <vector>
#include <functional>

	//生产消费模型
class ThreadPool{
public:
	//生产加线程加锁
	ThreadPool(int num) :stop(false){
		for (int i = 0; i < num; i++)
		{
			threads.emplace_back([this] {
				while(1)
				{
					std::unique_lock<std::mutex> lock(mtx);
					condition.wait(lock, [this] {
						return !tasks.empty() || stop;
						});
					if (stop && tasks.empty())
					{
						return;
					}
					std::function<void()> task(std::move(tasks.front()));
					tasks.pop();
					lock.unlock();
					task();
				}
				});
		}
	}

	//释放通知线程解锁（释放也是多线程所以释放要加锁）
	~ThreadPool()
	{
		{
			std::unique_lock<std::mutex> lock(mtx);
			stop = true;
		}

		condition.notify_all();
		for (auto& t : threads)
		{
			t.join();
		}
	}

	//放入
	template<class F, class ... Args>
	void enqueue(F &&f,Args&&... args )
	{
		//std::bind生成一个新的可调用对象，将给定的函数或可调用对象f与一些参数args绑定起来。
		//std::forward是用于实现完美转发的，它可以保持原始参数的类型（左值右值）。
		std::function<void()>task =
			std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		{
			std::unique_lock<std::mutex> lock(mtx);
			tasks.emplace(std::move(task)); //对列 emplace
		}
		condition.notify_one();
	}

private:
	std::vector<std::thread> threads;
	std::queue<std::function<void()>> tasks;

	std::mutex mtx;
	std::condition_variable condition;

	bool stop;
};

int main()
{
	ThreadPool pool(4);

	for (int i = 0; i < 10; i++)
	{
		pool.enqueue([i]
			{
				std::cout << "task : " << i <<" "<< "is running" << std::endl;
				std::this_thread::sleep_for(std::chrono::seconds(1));
				std::cout << "task : " << i <<" "<< "is done" << std::endl;
			});
	}
	return 0;
}

