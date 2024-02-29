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

Worker::Worker(std::queue<Job*>* jobs) noexcept : jobs_(jobs){}

void Worker::Start() noexcept { 
  thread_ = std::thread(&Worker::LoopOverJobs, this);
}

void Worker::Join() noexcept { thread_.join(); }

void Worker::LoopOverJobs() noexcept {
  while (is_running_) {
    Job* job = nullptr;

    if (!jobs_->empty()) {
      job = jobs_->front();
      jobs_->pop();
    }
    else {
      is_running_ = false;
      break;
    }

    if (job) {
      job->Execute();
    }
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
    switch (static_cast<JobType>(i)) { 
      case JobType::kImageFileLoading:
        workers_.emplace_back(&img_file_loading_jobs_);
        workers_[i].Start();
        break;
      case JobType::kImageFileDecompressing:
        workers_.emplace_back(&img_decompressing_jobs_);
        workers_[i].Start();
        break;
      case JobType::kShaderFileLoading:
        workers_.emplace_back(&shader_file_loading_jobs_);
        workers_[i].Start();
        break;
      case JobType::kMeshCreating:
        workers_.emplace_back(&mesh_creating_jobs_);
        workers_[i].Start();
        break;
    case JobType::kModelLoading:
        workers_.emplace_back(&model_loading_jobs_);
        workers_[i].Start();
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
      img_file_loading_jobs_.push(job);
      break;
    case JobType::kImageFileDecompressing:
      img_decompressing_jobs_.push(job);
      break;
    case JobType::kShaderFileLoading:
      shader_file_loading_jobs_.push(job);
      break;
    case JobType::kMeshCreating:
      mesh_creating_jobs_.push(job);
      break;
    case JobType::kModelLoading:
      model_loading_jobs_.push(job);
      break;
    case JobType::kMainThread:
      main_thread_jobs_.push_back(job);
      break;
    case JobType::kNone:
      break;
  }
}