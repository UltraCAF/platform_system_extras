/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SIMPLE_PERF_EVENT_FD_H_
#define SIMPLE_PERF_EVENT_FD_H_

#include <sys/types.h>

#include <memory>
#include <string>
#include <vector>

#include <android-base/macros.h>

#include "perf_event.h"

struct PerfCounter {
  uint64_t value;         // The value of the event specified by the perf_event_file.
  uint64_t time_enabled;  // The enabled time.
  uint64_t time_running;  // The running time.
  uint64_t id;            // The id of the perf_event_file.
};

struct pollfd;

// EventFd represents an opened perf_event_file.
class EventFd {
 public:
  static std::unique_ptr<EventFd> OpenEventFile(const perf_event_attr& attr, pid_t tid, int cpu,
                                                bool report_error = true);

  ~EventFd();

  uint64_t Id() const;

  pid_t ThreadId() const {
    return tid_;
  }

  int Cpu() const {
    return cpu_;
  }

  bool ReadCounter(PerfCounter* counter) const;

  // Call mmap() for this perf_event_file, so we can read sampled records from mapped area.
  // mmap_pages should be power of 2.
  bool MmapContent(size_t mmap_pages);

  // When the kernel writes new sampled records to the mapped area, we can get them by returning
  // the start address and size of the data.
  size_t GetAvailableMmapData(char** pdata);

  // Prepare pollfd for poll() to wait on available mmap_data.
  void PreparePollForMmapData(pollfd* poll_fd);

 private:
  EventFd(int perf_event_fd, const std::string& event_name, pid_t tid, int cpu)
      : perf_event_fd_(perf_event_fd),
        id_(0),
        event_name_(event_name),
        tid_(tid),
        cpu_(cpu),
        mmap_addr_(nullptr),
        mmap_len_(0) {
  }

  // Give information about this perf_event_file, like (event_name, tid, cpu).
  std::string Name() const;

  // Discard how much data we have read, so the kernel can reuse this part of mapped area to store
  // new data.
  void DiscardMmapData(size_t discard_size);

  int perf_event_fd_;
  mutable uint64_t id_;
  const std::string event_name_;
  pid_t tid_;
  int cpu_;

  void* mmap_addr_;
  size_t mmap_len_;
  perf_event_mmap_page* mmap_metadata_page_;  // The first page of mmap_area.
  char* mmap_data_buffer_;  // Starts from the second page of mmap_area, containing records written
                            // by then kernel.
  size_t mmap_data_buffer_size_;

  // As mmap_data_buffer is a ring buffer, it is possible that one record is wrapped at the
  // end of the buffer. So we need to copy records from mmap_data_buffer to data_process_buffer
  // before processing them.
  static std::vector<char> data_process_buffer_;

  DISALLOW_COPY_AND_ASSIGN(EventFd);
};

bool IsEventAttrSupportedByKernel(perf_event_attr attr);

#endif  // SIMPLE_PERF_EVENT_FD_H_
