#ifndef KRX_NANOKERN_KI_H
#define KRX_NANOKERN_KI_H

#include "kdk/nanokern.h"

/*!
 * Upcall from MD to Ke - process the DPC queue on a CPU.
 *
 * \pre IPL=dpc
 */
void ki_dispatch_dpcs(kcpu_t *cpu);

/*!
 * Upcall from MD to Ke - tick the local hard clock.
 *
 * \pre IPL=high
 */
bool ki_cpu_hardclock(md_intr_frame_t *frame, void *arg);

/*! @brief Initialise a kcpu structure. */
void ki_cpu_init(kcpu_t *cpu, kthread_t *idle_thread);

/*!
 * @brief Wake waiters previously queued up by ki_signal()
 * \pre Scheduler lock held
 */
void ki_wake_waiters(kwaitblock_queue_t *queue);

/*!
 * @brief Signal an object; satisfies waiters and places them on @p wakeQueue.
 * \pre Object lock held
 */
void ki_signal(kdispatchheader_t *hdr, kwaitblock_queue_t *wakeQueue);

void ki_thread_common_init(kthread_t *thread, kcpu_t *last_cpu,
    kprocess_t *proc, const char *name);

/*!
 * @brief Resume a thread.
 * @pre Scheduler lock held.
 */
void ki_thread_resume_locked(kthread_t *thread);

void ki_timer_enqueue_locked(ktimer_t *callout);
bool ki_timer_dequeue_locked(ktimer_t *callout);

void md_send_invlpg_ipi(kcpu_t *cpu);
void md_raise_dpc_interrupt(void);
void md_cpu_init(kcpu_t *cpu);

/*!
 * Reschedule this core.
 *
 * \pre Dispatcher DB locked.
 */
void ki_reschedule(void);

void ki_rcu_quiet();
void ki_rcu_per_cpu_init(struct ki_rcu_per_cpu_data *data);

void ki_tlb_flush_vaddr_locally(uintptr_t vaddr);
void ki_tlb_flush_vaddr_globally(uintptr_t vaddr);

void ki_enter_user_mode(uintptr_t ip, uintptr_t sp);

#endif /* KRX_NANOKERN_KI_H */
