#pragma once

#include "sys/socket.h"
#include "socketpair.h"
#define PF_LOCAL AF_UNIX

// this ice: using lwip's alloc/free hooks
#define mem_free plctag_mem_free
#define mem_alloc plctag_mem_alloc
#include_next "platform.h"
