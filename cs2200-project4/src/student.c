/*
 * student.c
 * Multithreaded OS Simulation for CS 2200
 * Spring 2023
 *
 * This file contains the CPU scheduler for the simulation.
 */

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "student.h"

#include <getopt.h>
#include <limits.h>
#include <float.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/** Function prototypes **/
extern void idle(unsigned int cpu_id);
extern void preempt(unsigned int cpu_id);
extern void yield(unsigned int cpu_id);
extern void terminate(unsigned int cpu_id);
extern void wake_up(pcb_t *process);

static unsigned int cpu_count;

/*
 * current[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * current[] should be updated by schedule() each time a process is scheduled
 * on a CPU.  Since the current[] array is accessed by multiple threads, you
 * will need to use a mutex to protect it.  current_mutex has been provided
 * for your use.
 *
 * rq is a pointer to a struct you should use for your ready queue
 * implementation. The head of the queue corresponds to the process
 * that is about to be scheduled onto the CPU, and the tail is for
 * convenience in the enqueue function. See student.h for the
 * relevant function and struct declarations.
 *
 * Similar to current[], rq is accessed by multiple threads,
 * so you will need to use a mutex to protect it. ready_mutex has been
 * provided for that purpose.
 *
 * The condition variable queue_not_empty has been provided for you
 * to use in conditional waits and signals.
 *
 * Please look up documentation on how to properly use pthread_mutex_t
 * and pthread_cond_t.
 *
 * A scheduler_algorithm variable and sched_algorithm_t enum have also been
 * supplied to you to keep track of your scheduler's current scheduling
 * algorithm. You should update this variable according to the program's
 * command-line arguments. Read student.h for the definitions of this type.
 */
static pcb_t **current;
static queue_t *rq;

static pthread_mutex_t current_mutex;
static pthread_mutex_t queue_mutex;
static pthread_cond_t queue_not_empty;

static sched_algorithm_t scheduler_algorithm;
static unsigned int cpu_count;
static unsigned int age_weight;
static int time_slice; //initialized as global variable


/** ------------------------Problem 3-----------------------------------
 * Checkout PDF Section 5 for this problem
 * 
 * priority_with_age() is a helper function to calculate the priority of a process
 * taking into consideration the age of the process.
 * 
 * It is determined by the formula:
 * Priority With Age = Priority + (Current Time - Enqueue Time) * Age Weight
 * 
 * @param current_time current time of the simulation
 * @param process process that we need to calculate the priority with age
 * 
 */
extern double priority_with_age(unsigned int current_time, pcb_t *process) {
    double age = (double) (current_time - process->enqueue_time);
    double priority = process->priority;
    return priority + age * age_weight;
}

/** ------------------------Problem 0 & 3-----------------------------------
 * Checkout PDF Section 2 and 5 for this problem
 * 
 * enqueue() is a helper function to add a process to the ready queue.
 * 
 * NOTE: For Priority scheduling, you will need to have additional logic
 * in this function and/or the dequeue function to account for enqueue time
 * and age to pick the process with the smallest age priority.
 * 
 * We have provided a function get_current_time() which is prototyped in os-sim.h.
 * Look there for more information.
 *
 * @param queue pointer to the ready queue
 * @param process process that we need to put in the ready queue
 */
void enqueue(queue_t *queue, pcb_t *process)
{
    pthread_mutex_lock(&queue_mutex);
    if (is_empty(queue)) {
        queue->head = process;
        queue->tail = process;
    }
    else {
        queue->tail->next = process;
        queue->tail = process;
    }
    process->next = NULL;
    pthread_cond_signal(&queue_not_empty);
    pthread_mutex_unlock(&queue_mutex);
}

/**
 * dequeue() is a helper function to remove a process to the ready queue.
 *
 * NOTE: For Priority scheduling, you will need to have additional logic
 * in this function and/or the enqueue function to account for enqueue time
 * and age to pick the process with the smallest age priority.
 * 
 * We have provided a function get_current_time() which is prototyped in os-sim.h.
 * Look there for more information.
 * 
 * @param queue pointer to the ready queue
 */

pcb_t *dequeue(queue_t *queue)
{
    pthread_mutex_lock(&queue_mutex);
    pcb_t *out;
    if (is_empty(queue)) {
        out = NULL;
    } else {
        if (scheduler_algorithm == PA) {
            double min_pa = DBL_MAX;
            pcb_t *curr = queue->head;
            unsigned int current_time = get_current_time();
            while (curr){
                double pr_age = priority_with_age(current_time, curr);
                if (pr_age < min_pa){
                    min_pa = pr_age;
                    out = curr;
                }
                curr = curr->next;
            }
            if (queue->head == out) {
                queue->head = queue->head->next;
            } else {
                pcb_t *curr2 = queue->head->next;
                pcb_t *prev = queue->head;
                while (curr2) {
                    if (curr2 == out) {
                        prev->next = curr2->next;
                        if (curr2 == queue->tail) {
                            queue->tail = prev;
                        }
                    }
                    curr2 = curr2->next;
                    prev = prev->next;
                }
            }
            out->priority = min_pa;
        } else {
            out = queue->head;
            if (queue->head == queue->tail) {
                queue->head = NULL;
                queue->tail = NULL;
            } else {
                queue->head = queue->head->next;
            }
        }
    }
    pthread_mutex_unlock(&queue_mutex);
    return out;
}

/** ------------------------Problem 0-----------------------------------
 * Checkout PDF Section 2 for this problem
 * 
 * is_empty() is a helper function that returns whether the ready queue
 * has any processes in it.
 * 
 * @param queue pointer to the ready queue
 * 
 * @return a boolean value that indicates whether the queue is empty or not
 */
bool is_empty(queue_t *queue)
{
    return queue->head == NULL;
}

/** ------------------------Problem 1B-----------------------------------
 * Checkout PDF Section 3 for this problem
 * 
 * schedule() is your CPU scheduler.
 * 
 * Remember to specify the timeslice if the scheduling algorithm is Round-Robin
 * 
 * @param cpu_id the target cpu we decide to put our process in
 */
static void schedule(unsigned int cpu_id)
{
    pcb_t *proc = dequeue(rq);
    if (proc) {
        proc->state = PROCESS_RUNNING;
    }
    pthread_mutex_lock(&current_mutex);
    current[cpu_id] = proc;
    context_switch(cpu_id, proc, time_slice);
    pthread_mutex_unlock(&current_mutex);
}

/**  ------------------------Problem 1A-----------------------------------
 * Checkout PDF Section 3 for this problem
 * 
 * idle() is your idle process.  It is called by the simulator when the idle
 * process is scheduled. This function should block until a process is added
 * to your ready queue.
 *
 * @param cpu_id the cpu that is waiting for process to come in
 */
extern void idle(unsigned int cpu_id)
{
    pthread_mutex_lock(&queue_mutex);
    while(is_empty(rq)) {
        pthread_cond_wait(&queue_not_empty, &queue_mutex);
    }
    pthread_mutex_unlock(&queue_mutex);
    schedule(cpu_id);

    /*
     * REMOVE THE LINE BELOW AFTER IMPLEMENTING IDLE()
     *
     * idle() must block when the ready queue is empty, or else the CPU threads
     * will spin in a loop.  Until a ready queue is implemented, we'll put the
     * thread to sleep to keep it from consuming 100% of the CPU time.  Once
     * you implement a proper idle() function using a condition variable,
     * remove the call to mt_safe_usleep() below.
     */
    //mt_safe_usleep(1000000);
}

/** ------------------------Problem 2 & 3-----------------------------------
 * Checkout Section 4 and 5 for this problem
 * 
 * preempt() is the handler used in Round-robin and Preemptive Priority 
 * Scheduling
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 * 
 * @param cpu_id the cpu in which we want to preempt process
 */
extern void preempt(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    current[cpu_id]->state = PROCESS_READY;
    enqueue(rq, current[cpu_id]);
    current[cpu_id] = NULL;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}

/**  ------------------------Problem 1A-----------------------------------
 * Checkout PDF Section 3 for this problem
 * 
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * @param cpu_id the cpu that is yielded by the process
 */
extern void yield(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    current[cpu_id]->state = PROCESS_WAITING;
    current[cpu_id] = NULL;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}

/**  ------------------------Problem 1A-----------------------------------
 * Checkout PDF Section 3
 * 
 * terminate() is the handler called by the simulator when a process completes.
 * 
 * @param cpu_id the cpu we want to terminate
 */
extern void terminate(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    current[cpu_id]->state = PROCESS_TERMINATED;
    current[cpu_id] = NULL;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}

/**  ------------------------Problem 1A & 3---------------------------------
 * Checkout PDF Section 3 and 5 for this problem
 * 
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes. This method will also need to handle priority, 
 * Look in section 5 of the PDF for more info.
 * 
 * We have provided a function get_current_time() which is prototyped in os-sim.h.
 * Look there for more information.
 * 
 * When a process wakes up, it will preempt the currently running process with the lowest functional priority
 * if the functional priority is smaller than the functional priority of the woken process. This preemption takes
 * place only if there are no free CPUs
 * 
 * @param process the process that finishes I/O and is ready to run on CPU
 */
extern void wake_up(pcb_t *process)
{
    process->state = PROCESS_READY;
    enqueue(rq, process);

    if (scheduler_algorithm == PA) {
        pthread_mutex_lock(&current_mutex);
        for (unsigned int i = 0; i < cpu_count; i++) {
            if (!current[i]) {
                pthread_mutex_unlock(&current_mutex);
                return;
            }
        }

        double low = DBL_MAX;
        unsigned int idx = __INT_MAX__;
        double new_pa = priority_with_age(get_current_time(), process);
        for (unsigned int i = 0; i < cpu_count; i++) {
            if (current[i]->priority < low) {
                low = current[i]->priority;
                idx = i;
            }
        }
        pthread_mutex_unlock(&current_mutex);
        if (low < new_pa) {
            force_preempt(idx);
        }
    }
}

/**
 * main() simply parses command line arguments, then calls start_simulator().
 * Add support for -r and -p parameters. If no argument has been supplied, 
 * you should default to FCFS.
 * 
 * HINT:
 * Use the scheduler_algorithm variable (see student.h) in your scheduler to 
 * keep track of the scheduling algorithm you're using.
 */
int main(int argc, char *argv[])
{
    scheduler_algorithm = FCFS;
    age_weight = 0;
    int opt;
    if (argc < 2) //if (argc != 2)
    {
        fprintf(stderr, "CS 2200 Project 4 -- Multithreaded OS Simulator\n"
                        "Usage: ./os-sim <# CPUs> [ -r <time slice> | -p <age weight> ]\n"
                        "    Default : FCFS Scheduler\n"
                        "         -r : Round-Robin Scheduler\n1\n"
                        "         -p : Priority Aging Scheduler\n");
        return -1;
    }   
    else
    {
        opt = getopt(argc, argv, "r:p");
        switch(opt)
        {
            case 'r':
                time_slice = atoi(optarg);
                scheduler_algorithm = RR;
                break;
            case 'p':
                scheduler_algorithm = PA;
                break;
            default:
                break;
        }
    }


    /* Parse the command line arguments */
    cpu_count = strtoul(argv[1], NULL, 0);

    /* Allocate the current[] array and its mutex */
    current = malloc(sizeof(pcb_t *) * cpu_count);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_not_empty, NULL);
    rq = (queue_t *)malloc(sizeof(queue_t));
    assert(rq != NULL);

    /* Start the simulator in the library */
    start_simulator(cpu_count);

    return 0;
}

#pragma GCC diagnostic pop
