#pragma once

#include "ring_buffer.h"

#include <vector>
#include <thread>
#include <future>
#include <queue>
#include <shared_mutex>


enum class JobStatus : std::int16_t {
  kStarted,
  kDone,
  kNone,
};

class Job {
 public:
  Job() noexcept = default;
  Job(Job&& other) noexcept = default;
  Job& operator=(Job&& other) noexcept = default;
  Job(const Job& other) noexcept = delete;
  Job& operator=(const Job& other) noexcept = delete;
  virtual ~Job() noexcept = default;

  void Execute() noexcept;
  void WaitUntilJobIsDone() const noexcept;
  /**
   * \brief IsReadyToStart is a method that checks if all the dependency
   * of the job are done, which means that the job can be executed.
   * \return If all the dependency
   * of the job are done, which means that the job can be executed
   */
  [[nodiscrad]] bool IsReadyToStart() const noexcept;
  void AddDependency(const Job* dependency) noexcept;

  [[nodiscard]] bool IsDone() const noexcept { return status_ == JobStatus::kDone; }

 protected:
  std::vector<const Job*> dependencies_;
  std::promise<void> promise_;
  std::shared_future<void> future_ = promise_.get_future();
  JobStatus status_ = JobStatus::kNone;

  virtual void Work() noexcept = 0;
};

/**
 * \brief JobQueue is a thread safe queue which stores jobs.
 */
class JobQueue {
 public:
  void Push(Job* job) noexcept;
  [[nodiscard]] Job* Pop() noexcept;

  [[nodiscard]] bool IsEmpty() const noexcept;

 private:
  std::queue<Job*> jobs_;
  mutable std::shared_mutex shared_mutex_;
};

using JobRingBuffer = RingBuffer<Job*, 300>;

class Worker {
 public:
  explicit Worker(JobRingBuffer* jobs) noexcept;
  void Start() noexcept;
  void Join() noexcept;

 private:
  std::thread thread_{};
  JobRingBuffer* jobs_;

  void LoopOverJobs() const noexcept;
};

class JobSystem {
 public:
  JobSystem() noexcept = default;
  void AddJob(Job* job) noexcept;
  void LaunchWorkers(int worker_count) noexcept;

  void JoinWorkers() noexcept;

 private:
  JobRingBuffer jobs_;
  std::vector<Worker> workers_{};
};