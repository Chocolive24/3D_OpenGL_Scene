#include "file_utility.h"

#include <fstream>

FileBuffer::FileBuffer(FileBuffer&& other) noexcept {
  std::swap(data, other.data);
  std::swap(size, other.size);
}

FileBuffer& FileBuffer::operator=(FileBuffer&& other) noexcept {
  std::swap(data, other.data);
  std::swap(size, other.size);

  return *this;
}

FileBuffer::~FileBuffer() {
  if (data != nullptr) {
    delete[] data;
    data = nullptr;
    size = 0;
  }
}

namespace file_utility {
std::string LoadFile(std::string_view path) {
  std::string content;
  std::ifstream t(path.data());

  t.seekg(0, std::ios::end);
  content.reserve(t.tellg());
  t.seekg(0, std::ios::beg);

  content.assign((std::istreambuf_iterator<char>(t)),
                 std::istreambuf_iterator<char>());
  return content;
}

FileBuffer LoadFileBuffer(std::string_view path) {
  FileBuffer file_buffer;

  std::ifstream t(path.data(), std::ios::binary);
  if (!t.is_open()) {
    file_buffer.data = nullptr;
    file_buffer.size = 0;
    return file_buffer;
  }

  t.seekg(0, std::ios::end);
  file_buffer.size = static_cast<int>(t.tellg());
  t.seekg(0, std::ios::beg);

  file_buffer.data = new unsigned char[file_buffer.size];

  t.read(reinterpret_cast<char*>(file_buffer.data), file_buffer.size);

  return file_buffer;
}

}  // namespace file_utility