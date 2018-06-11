#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "scheduler.h"
#include "util.h"

// Struct to represent a single node in the LinkedList
typedef struct node {
  job_t task_fn;
  size_t interval;
  int time_remaining; // Time remaining to meet the deadline 
  struct node* next;
} task_node;

task_node *head = NULL;
task_node *current = NULL;
task_node *earliestDeadlineTaskNode = NULL;
bool shouldRunCurrentJob = true;

size_t run_time = 0;
size_t sleep_time = 0;

// Credit to Blake & Joel as we based our implementation of removeTask() 
void removeTask() {

}

// Add a periodic job to the job list. The function `job_fn` should run every `interval` ms
void add_job(job_t task_fn, size_t interval) {
  task_node* new_node;
  new_node = malloc(sizeof(task_node));
  new_node->task_fn = task_fn;
  new_node->interval = interval;
  new_node->next = head;
  new_node->time_remaining = interval;
  head = new_node;
}

// Remove the current job from the job list
void remove_job() {
  if (head == NULL) {
    return;
  } else {
    if (head == earliestDeadlineTaskNode) {
      head = head->next;
    } else { // find and remove job
      current = earliestDeadlineTaskNode;
      while (current->next != earliestDeadlineTaskNode) { 
        if (current->next == NULL) { 
          current = head;
        }
        else {
          current = current->next;
        }
      }
      current->next = current->next->next; 
    }

    free(earliestDeadlineTaskNode);
  }
}

// Change the interval of the current job
void update_job_interval(size_t interval) {
  earliestDeadlineTaskNode->interval = interval;
}

// Stop running the scheduler after the current job finishes
void stop_scheduler() {
  shouldRunCurrentJob = false;
}

// HUGE Credits to Marcel Champagne for debugging our code and helping us with the seg-fault
// Run the game scheduler until there are no jobs left to run or the scheduler is stopped
void run_scheduler() {
  int start_time = 0;
  int end_time = 0;

  while (shouldRunCurrentJob) {
    // Set current to head
    current = head;
    earliestDeadlineTaskNode = head;

    //Get the node with the closest deadline
    while (current != NULL) {
      if (current->time_remaining < earliestDeadlineTaskNode->time_remaining) {
        earliestDeadlineTaskNode = current;          
      }
      current = current->next;
    }
    
    // Having found the next job to run, we want to sleep until it's ready
    sleep_time = earliestDeadlineTaskNode->time_remaining;

    sleep_ms(sleep_time); //sleep until job is ready

    
    // Once time_remaining is up, run the job, and measure how long it took.
    start_time = time_ms();

    earliestDeadlineTaskNode->task_fn();

    end_time = time_ms();
    run_time = end_time - start_time; // Update run_time

    current = head;
    while (current != NULL) {
      current->time_remaining = current->time_remaining - (sleep_time + run_time);
      current = current->next;
    }
    earliestDeadlineTaskNode->time_remaining = earliestDeadlineTaskNode->interval; 
  }
}
