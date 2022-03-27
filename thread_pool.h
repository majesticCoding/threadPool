#pragma once

#include <thread>
#include <functional>
#include <vector>
#include <queue>
#include <mutex>
#include <future>
#include <condition_variable>

namespace threading {

	class CThreadPool {
	public:
		CThreadPool() = delete;
		CThreadPool(const CThreadPool& pool) = delete;
		CThreadPool(CThreadPool&& pool) = delete;
		CThreadPool& operator=(const CThreadPool& pool) = delete;
		CThreadPool& operator=(CThreadPool&& pool) = delete;

		explicit CThreadPool(const std::size_t& numOfThreads) {
			run(numOfThreads);
		}

		~CThreadPool() {
			kill();
		}

		bool isArrayOfThreadsEmpty();
		bool isQueueOfTasksEmpty();

		template<typename Function, class... Args>
		auto enqueueTask(Function&& f, Args&&... args)->std::future<std::invoke_result_t<Function, Args...>>;

	private:
		void run(const std::size_t& numOfThreads);
		void kill();

		std::mutex m_lockMutex;
		std::condition_variable m_event;
		std::atomic<bool> m_bToBeDestroyed = false;
		std::vector<std::thread> m_aThreads;
		std::queue<std::function<void()>> m_tasksQueue;
	};

	void CThreadPool::run(const std::size_t& numOfThreads) {
		m_aThreads.reserve(numOfThreads);

		for (std::size_t idx = 0; idx < numOfThreads; idx++) {
			m_aThreads.emplace_back([=]() {
				while (true) {
					std::function<void()> task;

					std::unique_lock<std::mutex> locker(m_lockMutex);

					m_event.wait(locker, [=]() {
						return m_bToBeDestroyed.load() || !isQueueOfTasksEmpty();
						});

					if (m_bToBeDestroyed.load() && isQueueOfTasksEmpty()) {
						break;
					}

					task = std::move(m_tasksQueue.front());
					m_tasksQueue.pop();

					locker.unlock();

					task();
				}
				});
		}
	}

	void CThreadPool::kill() {
		std::unique_lock<std::mutex> locker(m_lockMutex);
		m_bToBeDestroyed.store(true);
		locker.unlock();

		m_event.notify_all();

		for (auto& thread : m_aThreads)
			thread.join();
	}

	bool CThreadPool::isArrayOfThreadsEmpty() {
		return m_aThreads.empty();
	}

	bool CThreadPool::isQueueOfTasksEmpty() {
		return m_tasksQueue.empty();
	}

	template<typename Function, class... Args>
	auto CThreadPool::enqueueTask(Function&& f, Args&&... args) -> std::future<std::invoke_result_t<Function, Args...>>
	{
		auto task = std::make_shared<std::packaged_task<std::invoke_result_t<Function, Args...>()>>(
			std::bind(std::forward<Function>(f), std::forward<Args>(args)...)
			);

		std::future<std::invoke_result_t<Function, Args...>> result = task->get_future();

		std::unique_lock<std::mutex> locker(m_lockMutex);

		m_tasksQueue.emplace([=]() {
			(*task)();
			});

		locker.unlock();

		m_event.notify_one();

		return result;
	}
}
