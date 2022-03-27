#include "thread_pool.h"
#include <iostream>
#include <conio.h>
#include <assert.h>
#include <chrono>

using namespace threading;

void sumTest() {
	std::cout << "Test #1 sum" << "\n";
	CThreadPool pool(1);

	auto future = pool.enqueueTask([](const int& a, const int& b) {
			return a + b;
		}, 
		5, 
		7);

	assert(future.get() == 12);

	std::cout << "Test sum passed" << "\n";
}

void countingTo10MillionTest() {
	std::cout << "\n" << "Test #2 on counting to 10 million" << "\n";

	std::atomic<uint64_t> counter(0);

	constexpr auto tasksNum = 100;
	CThreadPool pool(tasksNum);

	auto start = std::chrono::high_resolution_clock::now();

	for (std::size_t idx = 0; idx < tasksNum; idx++) {
		auto res = pool.enqueueTask([&counter]() {
			while (counter < 10'000'000)
				counter++;
			});

		res.get();
	}

	auto stop = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

	std::cout << "Time taken by function: " << duration.count() << " microseconds" << std::endl;

	assert(counter == 10'000'000);

	std::cout << "Test on counting 10 million passed" << "\n";
}

void checkPassingParams() {
	std::cout << "\n" << "Test #3 on checking passing params" << "\n";

	constexpr auto tasksNum = 50;
	CThreadPool pool(tasksNum);

	std::vector<std::future<std::size_t>> futures;

	for (std::size_t idx = 0; idx < tasksNum; idx++) {
		auto task = [index = idx]() { return index; };
		futures.push_back(pool.enqueueTask(task));
	}

	for (std::size_t idx = 0; idx < tasksNum; idx++) {
		assert(idx == futures[idx].get());
	}

	std::cout << "Test checking passing params passed" << "\n\n";
}

int main() {
	sumTest();
	countingTo10MillionTest();
	checkPassingParams();

	_getch();
	return 0;
}