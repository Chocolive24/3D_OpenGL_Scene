#pragma once

#include <string_view>

struct FileBuffer {
 public:
  FileBuffer() noexcept = default;
  FileBuffer(FileBuffer&& other) noexcept;
  FileBuffer& operator=(FileBuffer&& other) noexcept;
  FileBuffer(const FileBuffer& other) = delete;
  FileBuffer& operator=(const FileBuffer&) = delete;
  ~FileBuffer();

  unsigned char* data = nullptr;
  int size = 0;
};

namespace file_utility {
  std::string LoadFile(std::string_view path);
  FileBuffer LoadFileBuffer(std::string_view path);
  void LoadFileInBuffer(std::string_view path, FileBuffer* file_buffer);
  }