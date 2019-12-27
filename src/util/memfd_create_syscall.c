//
// Created by vkkoshelev on 27.12.2019.
//

#include <sys/syscall.h>
#include <unistd.h>

int memfd_create(const char *name, unsigned int flags) {
    return (int) syscall(__NR_memfd_create, name, flags);
}

