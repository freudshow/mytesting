#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>

// 任务优先级定义
typedef enum {
    TASK_PRIORITY_LOW,
    TASK_PRIORITY_MEDIUM,
    TASK_PRIORITY_HIGH,
    TASK_PRIORITY_URGENT,
    TASK_PRIORITY_MAX = TASK_PRIORITY_URGENT + 1,
    TASK_PRIORITY_COUNT = 255
} TaskPriority;

// 前向声明
typedef struct Task Task;
typedef struct SubSlot SubSlot;
typedef struct Slot Slot;
typedef struct TimeWheel TimeWheel;
typedef struct ThreadPool ThreadPool;

typedef void (*taskFunc_t)(void *arg);

// 任务结构
struct Task {
    taskFunc_t func;  // 任务函数
    void *arg;                // 任务参数
    TaskPriority priority;    // 任务优先级
    unsigned int interval;    // 周期任务的时间间隔(秒), 0表示一次性任务
    struct timespec scheduled_time; // 计划执行时间
    Task *next;               // 链表指针
};

// 线程池任务队列节点
typedef struct PoolTaskNode {
    Task *task;
    struct PoolTaskNode *next;
} PoolTaskNode;

// 线程池结构
struct ThreadPool {
    pthread_t *threads;
    int num_threads;
    PoolTaskNode **priority_queues;  // 按优先级的任务队列
    pthread_mutex_t *queue_mutexes;  // 每个优先级队列的互斥锁
    pthread_cond_t *queue_conds;     // 每个优先级队列的条件变量
    int shutdown;
};

// 子槽位结构(用于槽位分片)
struct SubSlot {
    Task *tasks[TASK_PRIORITY_MAX];  // 按优先级存储的任务链表
    pthread_mutex_t mutex;             // 保护子槽位的互斥锁
};

// 时间轮槽位结构
struct Slot {
    SubSlot *sub_slots;                // 子槽位数组
    int num_sub_slots;                 // 子槽位数量
};

// 时间轮结构
struct TimeWheel {
    Slot *slots;                       // 槽位数组
    int num_slots;                     // 槽位数量
    int current_slot;                  // 当前指针位置
    unsigned int tick_interval;        // 每 tick 的时间间隔(秒)
    pthread_t timer_thread;            // 时间轮线程
    pthread_mutex_t mutex;             // 保护时间轮的互斥锁
    pthread_cond_t cond;               // 条件变量
    int running;                       // 运行标志
    ThreadPool *thread_pool;           // 关联的线程池
};

void thread_pool_destroy(ThreadPool *pool);
void time_wheel_destroy(TimeWheel *tw);

// 线程池工作函数
static void* thread_pool_worker(void *arg)
{
    ThreadPool *pool = (ThreadPool*) arg;
    PoolTaskNode *node;
    int i;

    while (1)
    {
        // 先检查高优先级任务
        for (i = TASK_PRIORITY_URGENT; i >= 0; i--)
        {
            pthread_mutex_lock(&pool->queue_mutexes[i]);

            // 等待任务或关闭信号
            while (pool->priority_queues[i] == NULL && !pool->shutdown)
            {
                pthread_cond_wait(&pool->queue_conds[i], &pool->queue_mutexes[i]);
            }

            // 如果线程池要关闭了，退出
            if (pool->shutdown)
            {
                pthread_mutex_unlock(&pool->queue_mutexes[i]);
                pthread_exit(NULL);
            }

            // 从队列中获取任务
            if (pool->priority_queues[i] != NULL)
            {
                node = pool->priority_queues[i];
                pool->priority_queues[i] = node->next;
                pthread_mutex_unlock(&pool->queue_mutexes[i]);

                // 执行任务
                if (node->task && node->task->func)
                {
                    node->task->func(node->task->arg);
                }

                // 释放节点
                free(node->task->arg);  // 假设任务参数是动态分配的
                free(node->task);
                free(node);
                break;  // 处理完一个任务后重新检查优先级
            }

            pthread_mutex_unlock(&pool->queue_mutexes[i]);
        }
    }
}

// 创建线程池
ThreadPool* thread_pool_create(int num_threads)
{
    if (num_threads <= 0)
        return NULL;

    ThreadPool *pool = (ThreadPool*) malloc(sizeof(ThreadPool));
    if (!pool)
        return NULL;

    pool->num_threads = num_threads;
    pool->threads = (pthread_t*) malloc(num_threads * sizeof(pthread_t));
    pool->priority_queues = (PoolTaskNode**) calloc(TASK_PRIORITY_COUNT, sizeof(PoolTaskNode*));
    pool->queue_mutexes = (pthread_mutex_t*) malloc(TASK_PRIORITY_COUNT * sizeof(pthread_mutex_t));
    pool->queue_conds = (pthread_cond_t*) malloc(TASK_PRIORITY_COUNT * sizeof(pthread_cond_t));
    pool->shutdown = 0;

    // 初始化互斥锁和条件变量
    for (int i = 0; i < TASK_PRIORITY_COUNT; i++)
    {
        pthread_mutex_init(&pool->queue_mutexes[i], NULL);
        pthread_cond_init(&pool->queue_conds[i], NULL);
    }

    // 创建工作线程
    for (int i = 0; i < num_threads; i++)
    {
        if (pthread_create(&pool->threads[i], NULL, thread_pool_worker, pool) != 0)
        {
            // 错误处理: 释放已分配资源
            thread_pool_destroy(pool);
            return NULL;
        }
    }

    return pool;
}

// 向线程池添加任务
int thread_pool_add_task(ThreadPool *pool, Task *task)
{
    if (!pool || !task || pool->shutdown)
        return -1;

    PoolTaskNode *node = (PoolTaskNode*) malloc(sizeof(PoolTaskNode));
    if (!node)
        return -1;

    node->task = task;
    node->next = NULL;

    // 根据优先级添加到相应队列
    int prio = task->priority;
    if (prio < 0 || prio >= TASK_PRIORITY_COUNT)
        prio = TASK_PRIORITY_MEDIUM;

    pthread_mutex_lock(&pool->queue_mutexes[prio]);

    // 添加到队列尾部
    if (pool->priority_queues[prio] == NULL)
    {
        pool->priority_queues[prio] = node;
    }
    else
    {
        PoolTaskNode *curr = pool->priority_queues[prio];
        while (curr->next)
            curr = curr->next;
        curr->next = node;
    }

    // 唤醒一个等待的线程
    pthread_cond_signal(&pool->queue_conds[prio]);
    pthread_mutex_unlock(&pool->queue_mutexes[prio]);

    return 0;
}

// 销毁线程池
void thread_pool_destroy(ThreadPool *pool)
{
    if (!pool)
        return;

    // 设置关闭标志
    pool->shutdown = 1;

    // 唤醒所有等待的线程
    for (int i = 0; i < TASK_PRIORITY_COUNT; i++)
    {
        pthread_cond_broadcast(&pool->queue_conds[i]);
    }

    // 等待所有线程结束
    for (int i = 0; i < pool->num_threads; i++)
    {
        pthread_join(pool->threads[i], NULL);
    }

    // 释放任务队列
    for (int p = 0; p < TASK_PRIORITY_COUNT; p++)
    {
        PoolTaskNode *node = pool->priority_queues[p];
        while (node)
        {
            PoolTaskNode *tmp = node;
            node = node->next;
            free(tmp->task->arg);
            free(tmp->task);
            free(tmp);
        }
    }

    // 释放其他资源
    for (int i = 0; i < TASK_PRIORITY_COUNT; i++)
    {
        pthread_mutex_destroy(&pool->queue_mutexes[i]);
        pthread_cond_destroy(&pool->queue_conds[i]);
    }

    free(pool->threads);
    free(pool->priority_queues);
    free(pool->queue_mutexes);
    free(pool->queue_conds);
    free(pool);
}

// 时间轮线程函数
static void* time_wheel_thread(void *arg)
{
    TimeWheel *tw = (TimeWheel*) arg;
    struct timespec next_tick;
    clock_gettime(CLOCK_MONOTONIC, &next_tick);

    while (tw->running)
    {
        // 计算下一个tick的时间
        next_tick.tv_sec += tw->tick_interval;

        // 等待到下一个tick
        int rc;
        while ((rc = pthread_cond_timedwait(&tw->cond, &tw->mutex, &next_tick)) == ETIMEDOUT)
        {
            // 处理当前槽位的任务
            pthread_mutex_lock(&tw->mutex);

            Slot *current = &tw->slots[tw->current_slot];
            // 遍历所有子槽位
            for (int s = 0; s < current->num_sub_slots; s++)
            {
                SubSlot *sub_slot = &current->sub_slots[s];
                pthread_mutex_lock(&sub_slot->mutex);

                // 遍历所有优先级的任务
                for (int p = 0; p < TASK_PRIORITY_COUNT; p++)
                {
                    Task *task = sub_slot->tasks[p];
                    while (task)
                    {
                        Task *next = task->next;

                        // 将任务添加到线程池执行
                        thread_pool_add_task(tw->thread_pool, task);

                        task = next;
                    }
                    sub_slot->tasks[p] = NULL;  // 清空已处理的任务
                }

                pthread_mutex_unlock(&sub_slot->mutex);
            }

            // 移动到下一个槽位
            tw->current_slot = (tw->current_slot + 1) % tw->num_slots;

            pthread_mutex_unlock(&tw->mutex);
        }

        if (rc != 0 && rc != ETIMEDOUT)
        {
            // 发生错误
            perror("pthread_cond_timedwait");
            break;
        }
    }

    return NULL;
}

// 创建时间轮
TimeWheel* time_wheel_create(int num_slots, int num_sub_slots_per_slot,
        unsigned int tick_interval, ThreadPool *thread_pool)
{
    if (num_slots <= 0 || num_sub_slots_per_slot <= 0 || tick_interval <= 0 || !thread_pool)
        return NULL;

    TimeWheel *tw = (TimeWheel*) malloc(sizeof(TimeWheel));
    if (!tw)
        return NULL;

    tw->num_slots = num_slots;
    tw->current_slot = 0;
    tw->tick_interval = tick_interval;
    tw->running = 1;
    tw->thread_pool = thread_pool;

    // 初始化互斥锁和条件变量
    pthread_mutex_init(&tw->mutex, NULL);
    pthread_cond_init(&tw->cond, NULL);

    // 分配并初始化槽位
    tw->slots = (Slot*) malloc(num_slots * sizeof(Slot));
    for (int i = 0; i < num_slots; i++)
    {
        tw->slots[i].num_sub_slots = num_sub_slots_per_slot;
        tw->slots[i].sub_slots = (SubSlot*) malloc(num_sub_slots_per_slot * sizeof(SubSlot));

        // 初始化子槽位
        for (int j = 0; j < num_sub_slots_per_slot; j++)
        {
            memset(tw->slots[i].sub_slots[j].tasks, 0, sizeof(Task*) * TASK_PRIORITY_COUNT);
            pthread_mutex_init(&tw->slots[i].sub_slots[j].mutex, NULL);
        }
    }

    // 创建时间轮线程
    if (pthread_create(&tw->timer_thread, NULL, time_wheel_thread, tw) != 0)
    {
        // 错误处理: 释放已分配资源
        time_wheel_destroy(tw);
        return NULL;
    }

    return tw;
}

// 计算任务应该放入哪个槽位
static int calculate_slot(TimeWheel *tw, unsigned int delay_seconds)
{
    if (!tw || delay_seconds == 0)
        return tw->current_slot;

    // 计算需要多少个tick
    unsigned int ticks = (delay_seconds + tw->tick_interval - 1) / tw->tick_interval;
    return (tw->current_slot + ticks) % tw->num_slots;
}

// 向时间轮添加任务
int time_wheel_add_task(TimeWheel *tw, Task *task, unsigned int delay_seconds)
{
    if (!tw || !task || !task->func)
        return -1;

    // 计算目标槽位
    int slot_idx = calculate_slot(tw, delay_seconds);

    // 简单的分片策略: 基于任务地址的哈希
    unsigned long hash = (unsigned long) task;
    int sub_slot_idx = hash % tw->slots[slot_idx].num_sub_slots;

    // 获取当前时间作为计划执行时间
    clock_gettime(CLOCK_MONOTONIC, &task->scheduled_time);
    task->scheduled_time.tv_sec += delay_seconds;

    // 将任务添加到相应的子槽位
    pthread_mutex_lock(&tw->slots[slot_idx].sub_slots[sub_slot_idx].mutex);

    // 按优先级插入到链表头部
    task->next = tw->slots[slot_idx].sub_slots[sub_slot_idx].tasks[task->priority];
    tw->slots[slot_idx].sub_slots[sub_slot_idx].tasks[task->priority] = task;

    pthread_mutex_unlock(&tw->slots[slot_idx].sub_slots[sub_slot_idx].mutex);

    return 0;
}

// 创建任务
Task* create_task(void (*func)(void*), void *arg, TaskPriority priority, unsigned int interval)
{
    Task *task = (Task*) malloc(sizeof(Task));
    if (!task)
        return NULL;

    task->func = func;
    task->arg = arg;
    task->priority = priority;
    task->interval = interval;
    task->next = NULL;

    return task;
}

// 销毁时间轮
void time_wheel_destroy(TimeWheel *tw)
{
    if (!tw)
        return;

    // 停止时间轮线程
    tw->running = 0;
    pthread_cond_signal(&tw->cond);
    pthread_join(tw->timer_thread, NULL);

    // 释放槽位和子槽位
    for (int i = 0; i < tw->num_slots; i++)
    {
        for (int j = 0; j < tw->slots[i].num_sub_slots; j++)
        {
            // 释放剩余任务
            for (int p = 0; p < TASK_PRIORITY_COUNT; p++)
            {
                Task *task = tw->slots[i].sub_slots[j].tasks[p];
                while (task)
                {
                    Task *next = task->next;
                    free(task->arg);
                    free(task);
                    task = next;
                }
            }
            pthread_mutex_destroy(&tw->slots[i].sub_slots[j].mutex);
        }
        free(tw->slots[i].sub_slots);
    }
    free(tw->slots);

    // 销毁同步对象
    pthread_mutex_destroy(&tw->mutex);
    pthread_cond_destroy(&tw->cond);

    free(tw);
}

// 示例任务函数: 短任务
void short_task(void *arg)
{
    int *id = (int*) arg;
    printf("执行短任务 #%d (线程ID: %lu)\n", *id, pthread_self());
}

// 示例任务函数: 长任务
void long_task(void *arg)
{
    int *id = (int*) arg;
    printf("开始执行长任务 #%d (线程ID: %lu)\n", *id, pthread_self());
    sleep(3);  // 模拟长时间运行
    printf("完成执行长任务 #%d (线程ID: %lu)\n", *id, pthread_self());
}

void timewheelmain(void)
{
    // 创建线程池(4个工作线程)
    ThreadPool *pool = thread_pool_create(4);
    if (!pool)
    {
        fprintf(stderr, "无法创建线程池\n");
        return;
    }

    // 创建时间轮: 60个槽位, 每个槽位4个子槽位, 1秒tick
    TimeWheel *tw = time_wheel_create(60, 4, 1, pool);
    if (!tw)
    {
        fprintf(stderr, "无法创建时间轮\n");
        thread_pool_destroy(pool);
        return;
    }

    printf("时间轮和线程池初始化完成, 添加示例任务...\n");

    // 添加一些示例任务
    for (int i = 0; i < 10; i++)
    {
        int *id = (int*) malloc(sizeof(int));
        *id = i;

        Task *task;
        if (i % 3 == 0)
        {
            // 每3个任务创建一个长任务, 高优先级
            task = create_task(long_task, id, TASK_PRIORITY_HIGH, 0);
        }
        else
        {
            // 短任务, 普通优先级
            task = create_task(short_task, id, TASK_PRIORITY_MEDIUM, 0);
        }

        // 分散在5秒内执行
        time_wheel_add_task(tw, task, i % 5 + 1);
    }

    // 运行10秒后退出
    sleep(10);

    printf("开始清理资源...\n");

    // 销毁时间轮和线程池
    time_wheel_destroy(tw);
    thread_pool_destroy(pool);

    printf("程序结束\n");
}
