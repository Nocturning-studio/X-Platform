/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class SOFTX_API ThreadPool
{
  public:
	ThreadPool(size_t numThreads) : stop(false), activeTasks(0)
	{
		for (size_t i = 0; i < numThreads; ++i)
		{
			workers.emplace_back([this, i] {
				PROFILE_THREAD(("SoftX Worker " + std::to_string(i)).c_str());
				while (true)
				{
					std::function<void()> task;
					{
						std::unique_lock<std::mutex> lock(queueMutex);
						condition.wait(lock, [this] { return stop || !tasks.empty(); });
						if (stop && tasks.empty())
							return;

						task = std::move(tasks.front());
						tasks.pop();
						++activeTasks; // под локом
					}

					task();

					{
						std::unique_lock<std::mutex> lock(queueMutex);
						--activeTasks;
						// notify_all внутри лока — главный поток не может
						// пропустить сигнал: он либо ещё не вошёл в wait()
						// (тогда увидит актуальный activeTasks при входе),
						// либо уже спит (тогда разбудим его)
						condition.notify_all();
					}
				}
			});
		}
	}

	~ThreadPool()
	{
		{
			std::unique_lock<std::mutex> lock(queueMutex);
			stop = true;
		}
		condition.notify_all();
		for (auto& worker : workers)
			worker.join();
	}

	template <class F> void enqueue(F&& task)
	{
		{
			std::unique_lock<std::mutex> lock(queueMutex);
			tasks.emplace(std::forward<F>(task));
		}
		condition.notify_one();
	}

	void wait()
	{
		PROFILE_SCOPE("Waiting for workers");
		PROFILE_TAG("Blocked", "true");
		std::unique_lock<std::mutex> lock(queueMutex);
		condition.wait(lock, [this] { return tasks.empty() && activeTasks == 0; });
	}

	size_t threadCount() const
	{
		return workers.size();
	}

  private:
	std::vector<std::thread> workers;
	std::queue<std::function<void()>> tasks;
	std::mutex queueMutex;
	std::condition_variable condition;
	bool stop;
	std::atomic<int> activeTasks;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
