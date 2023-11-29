/**
 *    ||          ____  _ __
 * +------+      / __ )(_) /_______________ _____  ___
 * | 0xBC |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * +------+    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *  ||  ||    /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie control firmware
 *
 * Copyright (C) 2011-2012 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * worker.c - Worker system that can execute asynchronous actions in tasks
 */
#include "worker.h"

#include <errno.h>


#include <zephyr\kernel.h>
#include "console.h"

#define WORKER_QUEUE_LENGTH 5
#define WORKER_QUEUE_PRIORITY 1

struct worker_work {
  struct k_work work;
  void* arg;
};

struct k_work_q workerQueue;
K_THREAD_STACK_DEFINE(workerQueueStack, WORKER_QUEUE_LENGTH * sizeof(struct worker_work));


void workerInit()
{
  if (workerQueue.flags)
    return;

  k_work_queue_init(&workerQueue);
  k_work_queue_start(&workerQueue, workerQueueStack,
                   K_THREAD_STACK_SIZEOF(workerQueueStack), WORKER_QUEUE_PRIORITY,
                   NULL);
}

void workerLoop()
{
  if (workerQueue.flags)
    return;

  k_work_queue_start(&workerQueue, workerQueueStack,
                   K_THREAD_STACK_SIZEOF(workerQueueStack), WORKER_QUEUE_PRIORITY,
                   NULL);
}

int workerScheduler(struct k_work *work, void *arg)
{
  if(k_work_submit_to_queue(&workerQueue, work) != 0){
    return ENOEXEC;
  }

  return 0;
}