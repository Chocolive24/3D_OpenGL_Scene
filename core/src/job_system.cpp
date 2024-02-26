#include "job_system.h"

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
  // Synchronization with all dependecies.
  // -------------------------------------
  for (const auto& dependecy : dependencies_) {
    if (!dependecy->IsDone()) {
      dependecy->WaitUntilJobIsDone();
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

void Job::AddDependency(Job* dependency) noexcept {
  dependencies_.push_back(dependency);
}

void Worker::RunWorkLoop(std::vector<Job*>& jobs) noexcept { 
  thread_ = std::thread([this](std::vector<Job*>& jobs) {  
    while (is_running_) {
      Job* job = nullptr;
    
      if (!jobs.empty()) {
        // Takes the job at the front of the vector and erases it.
        job = std::move(jobs.front());
        jobs.erase(jobs.begin());  
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
  for (auto& worker : workers_) {
    worker.Join();
  }
}

void JobSystem::LaunchWorkers(int worker_count) noexcept {
  workers_.reserve(worker_count);

  for (std::int8_t i = 0; i < worker_count; i++) {
    workers_.emplace_back(Worker());

    switch (static_cast<JobType>(i)) { 
      case JobType::kFileReading:
        workers_[i].RunWorkLoop(img_reading_jobs_);
        break;
      case JobType::kFileDecompressing:
        workers_[i].RunWorkLoop(img_decompressing_jobs_);
        break;
      default:
        break;
    }
  }

  RunMainThreadWorkLoop(main_thread_jobs);
}

void JobSystem::RunMainThreadWorkLoop(std::vector<Job*>& jobs) noexcept {
  bool is_running = true;
  while (is_running) {
    Job* job = nullptr;

    if (!jobs.empty()) {
      // Takes the job at the front of the vector and erases it.
      job = std::move(jobs.front());
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
  switch (job->type()) {
    case JobType::kFileReading:
      img_reading_jobs_.push_back(std::move(job));
      break;
    case JobType::kFileDecompressing:
      img_decompressing_jobs_.push_back(std::move(job));
      break;
    case JobType::kMainThread:
      main_thread_jobs.push_back(std::move(job));
      break;
    default:
      break;
  }
}
