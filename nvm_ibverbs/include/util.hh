#pragma once
#ifndef UTIL_HH
#define UTIL_HH
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <thread>
#include <Sherman/Debug.h>

#include<string>
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/mman.h>

int stick_this_thread_to_core(int core_id);

// for nvm
void opennvm(char **buffer, const std::string pm_file_path="/dev/dax1.0", size_t mysize=(size_t)1024 * 1024 * 1024 * 2);

#endif