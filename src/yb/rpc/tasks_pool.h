//
// Copyright (c) YugaByte, Inc.
//

#include <boost/lockfree/queue.hpp>

#ifndef YB_RPC_TASKS_POOL_H
#define YB_RPC_TASKS_POOL_H

namespace yb {
namespace rpc {

class ThreadPool;

// Tasks pool that could be used in conjunction with ThreadPool.
// To preallocate buffer for fixed number of tasks.
template <class Task>
class TasksPool {
 public:
  explicit TasksPool(size_t size) : tasks_(size), queue_(size) {
    for (auto& task : tasks_) {
      CHECK(queue_.bounded_push(&task));
    }
  }

  template <class... Args>
  bool Enqueue(ThreadPool* thread_pool, Args&&... args) {
    WrappedTask* task = nullptr;
    if (queue_.pop(task)) {
      task->pool = this;
      new (&task->storage) Task(std::forward<Args>(args)...);
      thread_pool->Enqueue(task);
      return true;
    } else {
      return false;
    }
  }

  size_t size() const {
    return tasks_.size();
  }
 private:
  struct WrappedTask;
  friend struct WrappedTask;

  void Released(WrappedTask* task) {
    CHECK(queue_.bounded_push(task));
  }

  struct WrappedTask : public ThreadPoolTask {
    TasksPool<Task>* pool = nullptr;
    typename std::aligned_storage<sizeof(Task), alignof(Task)>::type storage;

    Task& task() {
      return *reinterpret_cast<Task*>(&storage);;
    }

    void Run() override {
      task().Run();
    }

    void Done(const Status& status) override {
      task().Done(status);
      task().~Task();
      TasksPool<Task>* tasks_pool = pool;
      pool = nullptr;
      tasks_pool->Released(this);
    }

    virtual ~WrappedTask() {
      CHECK(!pool);
    }
  };

  std::vector<WrappedTask> tasks_;
  boost::lockfree::queue<WrappedTask*> queue_;
};

} // namespace rpc
} // namespace yb

#endif // YB_RPC_TASKS_POOL_H