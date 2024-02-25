#pragma once

#include "file_utility.h"

#include <string>
#include <vector>
#include <thread>
#include <future>
#include <memory>

enum class JobStatus : std::int8_t {
  kStarted,
  kDone,
  kNone,
};

enum class JobType : std::int8_t {
  kNone = -1,
  kFileReading,
  kFileDecompressing,
  kloadingTextureToGpu,
  kOther
};

class Job {
 public:
  Job(JobType job_type) : type_(job_type){};

  Job(Job&& other) noexcept;
  Job& operator=(Job&& other) noexcept;
  Job(const Job& other) noexcept = delete;
  Job& operator=(const Job& other) noexcept = delete;

  virtual ~Job() noexcept = default;

  void Execute() noexcept;

  void WaitUntilJobIsDone() const noexcept;

  void AddDependency(Job* dependency) noexcept;

  const bool IsDone() const noexcept { return status_ == JobStatus::kDone; }
  const bool HasStarted() const noexcept { return status_ == JobStatus::kStarted; }


  const JobType type() const noexcept { return type_; }

 protected:
  std::vector<const Job*> dependencies_;
  mutable std::promise<void> promise_;
  mutable std::future<void> future_ = promise_.get_future();
  JobStatus status_ = JobStatus::kNone;
  JobType type_ = JobType::kNone;

  virtual void Work() noexcept = 0;
};

class Worker {
 public:
  Worker() noexcept = default;
  void RunWorkLoop(std::vector<Job*>& jobs) noexcept;
  void Join() noexcept;

 private:
  std::thread thread_{};
  bool is_running_ = true;
};


class JobSystem {
 public:
  JobSystem() noexcept = default;
  void AddJob(Job* job) noexcept;
  void LaunchWorkers(int worker_count) noexcept;
  void RunMainThreadWorkLoop(std::vector<Job*>& jobs) noexcept;
  void JoinWorkers() noexcept;

 private:
  std::vector<Worker> workers_{};

  // Use vectors as queues by pushing elements back to the end while 
  // erasing those at the front.
  std::vector<Job*> img_reading_jobs_{};
  std::vector<Job*> img_decompressing_jobs_{};
  std::vector<Job*> loading_texture_to_gpu_jobs{};
};