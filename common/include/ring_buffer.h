#pragma once

#include <array>
#include <atomic>
#include <iostream>

enum class PushMethodIfFull {
  kReplaceOldestData,
  kDiscardNewData,
};

template <typename T, std::size_t Size = 256>
class RingBuffer {
 public:
  void Push(const T& data, PushMethodIfFull push_method =
            PushMethodIfFull::kReplaceOldestData) noexcept;
  [[nodiscard]] bool Pop(T& returned_data) noexcept;

 private:
  std::atomic<std::size_t> read_index_ = 0;
  std::atomic<std::size_t> write_index_ = 0;
  std::array<T, Size> data_{};

  static constexpr std::size_t GetNextIndex(const std::size_t& index) {
    return index == Size ? 0 : index + 1;
  }
};

template <typename T, size_t Size>
inline void RingBuffer<T, Size>::Push(const T& data,
                                      PushMethodIfFull push_method) noexcept {
  const auto& old_write_idx = write_index_.load();
  const auto& new_write_idx = GetNextIndex(old_write_idx);

  if (push_method == PushMethodIfFull::kDiscardNewData)
  {
    if (new_write_idx == read_index_.load()) {
      return;
    }
  }

  data_[old_write_idx] = data;

  write_index_.store(new_write_idx);
}

template <typename T, std::size_t Size>
inline bool RingBuffer<T, Size>::Pop(T& returned_data) noexcept {
  if (read_index_.load() == write_index_.load()) {
    return false;
  }
  const auto& old_read_idx = read_index_.load();
  read_index_.store(GetNextIndex(old_read_idx));

  returned_data = std::move(data_[old_read_idx]);

  return true;
}