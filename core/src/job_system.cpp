#include "job_system.h"

#include <iostream>

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

void JobQueue::Push(Job* job) noexcept {
  std::scoped_lock lock(shared_mutex_);
  jobs_.push(job);
}
Job* JobQueue::Pop() noexcept {
  std::scoped_lock lock(shared_mutex_);
  if (!jobs_.empty()) {
     Job* job = jobs_.front();
     jobs_.pop();
     return job;
  }

  return nullptr;
  //if (IsEmpty()) {
  //  return nullptr;
  //}

  //std::scoped_lock lock(shared_mutex_);
  //Job* job = jobs_.front();
  //jobs_.pop();
  //return job;
}
bool JobQueue::IsEmpty() const noexcept {
  std::shared_lock lock(shared_mutex_);
  return jobs_.empty();
}

Worker::Worker(JobQueue* jobs) noexcept : jobs_(jobs) {}

void Worker::Start() noexcept { 
  thread_ = std::thread(&Worker::LoopOverJobs, this);
}

void Worker::Join() noexcept { thread_.join(); }

void Worker::LoopOverJobs() noexcept {
  while (!jobs_->IsEmpty()) {
     Job* job = jobs_->Pop();

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
  std::cout << worker_count << '\n';
  workers_.reserve(worker_count);

  for (int i = 0; i < worker_count; i++) {
    workers_.emplace_back(&jobs_);
    workers_[i].Start();

    //switch (static_cast<JobType>(i)) { 
    //  case JobType::kImageFileLoading:
    //    workers_.emplace_back(&img_file_loading_jobs_);
    //    workers_[i].Start();
    //    break;
    //  case JobType::kImageFileDecompressing:
    //    workers_.emplace_back(&img_decompressing_jobs_);
    //    workers_[i].Start();
    //    break;
    //  case JobType::kShaderFileLoading:
    //    workers_.emplace_back(&shader_file_loading_jobs_);
    //    workers_[i].Start();
    //    break;
    //  case JobType::kMeshCreating:
    //    workers_.emplace_back(&mesh_creating_jobs_);
    //    workers_[i].Start();
    //    break;
    //case JobType::kModelLoading:
    //    workers_.emplace_back(&model_loading_jobs_);
    //    workers_[i].Start();
    //    break;
    //  case JobType::kNone:
    //  case JobType::kMainThread:
    //    break;
    //}
  }

}

void JobSystem::AddJob(Job* job) noexcept {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE

  jobs_.Push(job);

  //switch (job->type()) {
  //  case JobType::kImageFileLoading:
  //    img_file_loading_jobs_.push(job);
  //    break;
  //  case JobType::kImageFileDecompressing:
  //    img_decompressing_jobs_.push(job);
  //    break;
  //  case JobType::kShaderFileLoading:
  //    shader_file_loading_jobs_.push(job);
  //    break;
  //  case JobType::kMeshCreating:
  //    mesh_creating_jobs_.push(job);
  //    break;
  //  case JobType::kModelLoading:
  //    model_loading_jobs_.push(job);
  //    break;
  //  case JobType::kMainThread:
  //    main_thread_jobs_.push_back(job);
  //    break;
  //  case JobType::kNone:
  //    break;
  //}
}