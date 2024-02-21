#pragma once

#include <string_view>

class FileBuffer {
 public:
  FileBuffer() noexcept = default;
  FileBuffer(const FileBuffer& other) = delete;
  FileBuffer(FileBuffer&& other) noexcept;
  FileBuffer& operator=(const FileBuffer&) = delete;
  FileBuffer& operator=(FileBuffer&& other) noexcept;
  ~FileBuffer();

  unsigned char* data = nullptr;
  int size = 0;
};

namespace file_utility {
  std::string LoadFile(std::string_view path);
  FileBuffer LoadFileBuffer(std::string_view path);
}