#include "job_system.h"

#ifdef TRACY_ENABLE
#include <TracyC.h>
#include <Tracy.hpp>
#endif  // TRACY_ENABLE

void Job::Execute() noexcept {
  // Synchronization with all dependencies.
  // -------------------------------------
  for (const auto& dependency : dependencies_) {
    if (!dependency->IsDone()) {
      dependency->WaitUntilJobIsDone();
    }
  }

  status_ = JobStatus::kStarted;

  // Do the work of the job.
  // -----------------------
  Work(); // Pure virtual method.

  // Set the promise to tell that the work is done.
  // ----------------------------------------------
  promise_.set_value();

  status_ = JobStatus::kDone;
}

void Job::WaitUntilJobIsDone() const noexcept { 
  future_.get(); 
}

bool Job::IsReadyToStart() const noexcept {
  for (const auto& dependency : dependencies_)
  {
    if (!dependency->IsDone())
    {
      return false;
    }
  }
  return true;
}

void Job::AddDependency(const Job* dependency) noexcept {
  dependencies_.push_back(dependency);
}

void JobQueue::Push(Job* job) noexcept {
  std::scoped_lock lock(shared_mutex_);
  jobs_.push(job);
}
Job* JobQueue::Pop() noexcept {
  if (IsEmpty()) {
    return nullptr;
  }

  std::scoped_lock lock(shared_mutex_);
  if (!jobs_.empty()) {
    Job* job = jobs_.front();
    jobs_.pop();
    return job;
  }

  return nullptr;
}
bool JobQueue::IsEmpty() const noexcept {
  std::shared_lock lock(shared_mutex_);
  return jobs_.empty();
}

Worker::Worker(JobRingBuffer* jobs) noexcept : jobs_(jobs) {}

void Worker::Start() noexcept { 
  thread_ = std::thread(&Worker::LoopOverJobs, this);
}

void Worker::Join() noexcept { thread_.join(); }

void Worker::LoopOverJobs() const noexcept {
  Job* job = nullptr;
  while (jobs_->Pop(job)) {
    job->Execute();
  }
}

void JobSystem::JoinWorkers() noexcept {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE
  for (auto& worker : workers_) {
    worker.Join();
  }
}

void JobSystem::LaunchWorkers(const int worker_count) noexcept {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE

  workers_.reserve(worker_count);

  for (int i = 0; i < worker_count; i++) {
    workers_.emplace_back(&jobs_);
    workers_[i].Start();
  }
}

void JobSystem::AddJob(Job* job) noexcept {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE

  jobs_.Push(job);
}