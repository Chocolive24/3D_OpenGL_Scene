#include "job_system.h"

#ifdef TRACY_ENABLE
#include <TracyC.h>

#include <Tracy.hpp>
#endif  // TRACY_ENABLE

Job::Job(Job&& other) noexcept {
  dependencies_ = std::move(other.dependencies_);
  promise_ = std::move(other.promise_);
  future_ = std::move(other.future_);
  status_ = std::move(other.status_);
  type_ = std::move(other.type_);

  other.status_ = JobStatus::kNone;
  other.type_ = JobType::kNone;
}

Job& Job::operator=(Job&& other) noexcept {
  dependencies_ = std::move(other.dependencies_);
  promise_ = std::move(other.promise_);
  future_ = std::move(other.future_);
  status_ = std::move(other.status_);
  type_ = std::move(other.type_);

  other.status_ = JobStatus::kNone;
  other.type_ = JobType::kNone;

  return *this;
}

void Job::Execute() noexcept {
  // Synchronization with all dependencies.
  // -------------------------------------
  for (const auto& dependency : dependencies_) {
    if (!dependency->IsDone()) {
      dependency->WaitUntilJobIsDone();
    }
  }

  // Do the work of the job.
  // -----------------------
  Work(); // Pure virtual method.

  // Set the promise to tell that the work is done.
  // ----------------------------------------------
  promise_.set_value();

  status_ = JobStatus::kDone;
}

void Job::WaitUntilJobIsDone() const noexcept { 
  if (!IsDone()) {
    future_.get(); 
  }
}

void Job::AddDependency(const Job* dependency) noexcept {
  dependencies_.push_back(dependency);
}

void Worker::RunWorkLoop(std::vector<Job*>& jobs) noexcept { 
  thread_ = std::thread([this](std::vector<Job*>& jobs_queue) {  
    while (is_running_) {
      Job* job = nullptr;
    
      if (!jobs_queue.empty()) {
        // Takes the job at the front of the vector and erases it.
        job = jobs_queue.front();
        jobs_queue.erase(jobs_queue.begin());  
      } 
      else {
        is_running_ = false;
        break;
      }
    
      if (job) {
        job->Execute();
      }
    } 
  }, jobs);
}

void Worker::Join() noexcept { thread_.join(); }

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
    workers_.emplace_back();

    switch (static_cast<JobType>(i)) { 
      case JobType::kImageFileLoading:
        workers_[i].RunWorkLoop(img_file_loading_jobs_);
        break;
      case JobType::kImageFileDecompressing:
        workers_[i].RunWorkLoop(img_decompressing_jobs_);
        break;
      case JobType::kShaderFileLoading:
        workers_[i].RunWorkLoop(shader_file_loading_jobs_);
        break;
      case JobType::kMeshCreating:
        workers_[i].RunWorkLoop(mesh_creating_jobs_);
        break;
    case JobType::kModelLoading:
        workers_[i].RunWorkLoop(model_loading_jobs_);
        break;
      case JobType::kNone:
      case JobType::kMainThread:
        break;
    }
  }

  RunMainThreadWorkLoop(main_thread_jobs_);
}

void JobSystem::RunMainThreadWorkLoop(std::vector<Job*>& jobs) noexcept {
  bool is_running = true;
  while (is_running) {
    Job* job = nullptr;

    if (!jobs.empty()) {
      // Takes the job at the front of the vector and erases it.
      job = jobs.front();
      jobs.erase(jobs.begin());
    } else {
      is_running = false;
      break;
    }

    if (job) {
      job->Execute();
    }
  }
}

void JobSystem::AddJob(Job* job) noexcept {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE
  switch (job->type()) {
    case JobType::kImageFileLoading:
      img_file_loading_jobs_.push_back(job);
      break;
    case JobType::kImageFileDecompressing:
      img_decompressing_jobs_.push_back(job);
      break;
    case JobType::kShaderFileLoading:
      shader_file_loading_jobs_.push_back(job);
      break;
    case JobType::kMeshCreating:
      mesh_creating_jobs_.push_back(job);
      break;
    case JobType::kModelLoading:
      model_loading_jobs_.push_back(job);
      break;
    case JobType::kMainThread:
      main_thread_jobs_.push_back(job);
      break;
    case JobType::kNone:
      break;
  }
}
