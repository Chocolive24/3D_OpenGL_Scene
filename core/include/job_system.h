#pragma once

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

enum class JobType : std::int16_t {
  kNone = -1,
  kImageFileLoading,
  kImageFileDecompressing,
  kShaderFileLoading,
  kMeshCreating,
  kModelLoading,
  kMainThread,
};

class Job {
 public:
  Job(const JobType job_type) : type_(job_type){}

  Job(Job&& other) noexcept = default;
  Job& operator=(Job&& other) noexcept = default;
  Job(const Job& other) noexcept = delete;
  Job& operator=(const Job& other) noexcept = delete;

  virtual ~Job() noexcept = default;

  void Execute() noexcept;

  void WaitUntilJobIsDone() const noexcept;

  void AddDependency(const Job* dependency) noexcept;

  [[nodiscard]] bool IsDone() const noexcept { return status_ == JobStatus::kDone; }
  [[nodiscard]] bool HasStarted() const noexcept { return status_ == JobStatus::kStarted; }

  JobType type() const noexcept { return type_; }

 protected:
  std::vector<const Job*> dependencies_;
  std::promise<void> promise_;
  std::shared_future<void> future_ = promise_.get_future();
  JobStatus status_ = JobStatus::kNone;
  JobType type_ = JobType::kNone;

  virtual void Work() noexcept = 0;
};

/**
 * \brief JobQueue is a thread safe queue container.
 * TODO finish to code it and use it instead of std::queue.
 */
class JobQueue {
 public:
  void Pop() noexcept;
 private:
  std::queue<Job*> jobs_;
  std::shared_mutex mutex_;
};

class Worker {
 public:
  explicit Worker(std::queue<Job*>* jobs) noexcept;
  void Start() noexcept;
  void Join() noexcept;

 private:
  std::thread thread_{};
  std::queue<Job*>* jobs_; // TODO JobQueue* jobs_queue_
  bool is_running_ = true;

  void LoopOverJobs() noexcept;
};


class JobSystem {
 public:
  JobSystem() noexcept = default;
  void AddJob(Job* job) noexcept;
  void LaunchWorkers(int worker_count) noexcept;

  void JoinWorkers() noexcept;

 private:
  std::vector<Worker> workers_{};

  std::queue<Job*> img_file_loading_jobs_{};
  std::queue<Job*> img_decompressing_jobs_{};
  std::queue<Job*> shader_file_loading_jobs_{};
  std::queue<Job*> mesh_creating_jobs_{};
  std::queue<Job*> model_loading_jobs_{};
  std::vector<Job*> main_thread_jobs_{};

  void RunMainThreadWorkLoop(std::vector<Job*>& jobs) noexcept;
};