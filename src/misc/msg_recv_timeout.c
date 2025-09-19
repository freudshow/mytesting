#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "basedef.h"

#define MMQ_FTOK_PROJ_ID        1//用于ftok()函数生成key的第二个参数: prj_id的值
#define MMQ_MSG_PERMISSION      (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)//消息队列的权限
#define SYSTEM_ALLOC_TIMERID_START 0x7FFF0000
#define SYSTEM_ALLOC_TIMERID_END 0x7FFFFFFF
#define DEFAULT_TIMER_TIMEOUT 3000  // 管道通讯接受延时时间 单位:ms
#define MULTI_TIMER_INTERVAL 500
#define MAX_TIMER_COUNT 3

#define OK      0
#define ERROR   -1
#define SD_TRUE		1
#define SD_FALSE	0
#define SD_SUCCESS 	0
#define SD_FAILURE 	1

#define ST_CHAR    char
#define ST_INT     signed int
#define ST_LONG    signed long int
#define ST_UCHAR   unsigned char
#define ST_UINT    unsigned int
#define ST_ULONG   unsigned long
#define ST_VOID    void
#define ST_DOUBLE  double
#define ST_FLOAT   float

/* General purpose return code						*/
#define ST_RET signed int

/* We need specific sizes for these types				*/
#define ST_INT8   signed char
#define ST_INT16  signed short
#define ST_INT32  signed int
#define ST_INT64  signed long long
#define ST_UINT8  unsigned char
#define ST_UINT16 unsigned short
#define ST_UINT32 unsigned int
#define ST_UINT64 unsigned long long
#define ST_BOOLEAN  unsigned char

typedef struct Message
{
    ST_LONG msg_type;   // 消息类型, 设定为mqtt报文发送序号，req和ack报文拥有相同的序号

#define MQTT_MESSAGE_NORMAL         0   //mqtt消息，接收正常
#define MQTT_MESSAGE_TIMEOUT        1   //mqtt消息，接收超时
    ST_CHAR msg_flag;   // 获取消息超时标志 =0:正常; =1:超时未获取消息
    ST_UINT32 msg_len;  // 数据的长度，~0UL表示数据异常
    ST_CHAR *buf;       //不再使用数组存储消息，消息长度未知，buf由接收消息处分配，使用或清空消息时释放。
                        //buf需要按实际长度分配，不能太大，避免内存浪费。
                        //需要注意的是，如果是多【进程】之间共享数据，不可使用指针指向一个内存区域，
                        //因为每个进程之间的内存空间是独立的，
                        //指针在进程A指向的内存值在进程B中可能是无效或者错误的。
    ST_BOOLEAN buf_need_free;   //获取消息后，是否需要释放buf, 0-不需要，1-需要
} __attribute__((aligned(1))) t_Message;

typedef struct timer_s
{
    ST_UINT32 tid;
    ST_UINT32 ms;
    ST_UINT32 ms_left;
    int (*callback)(int timer_id);
    //struct timer_s next;//暂时没用
    //BOOLEAN valid;
    ST_BOOLEAN valid;
} __attribute__((aligned(1))) multi_timer_t;

static multi_timer_t multi_timer[MAX_TIMER_COUNT];
static pthread_mutex_t s_msg_mutex;
static int s_mqtt_message_fd;
static ST_BOOLEAN s_bMessageDebugEnable = 0;

// 超时标志
static volatile int timeout_occurred = 0;

// 消息结构体
struct msgbuf {
    long mtype;     // 消息类型
    char mtext[1024]; // 消息内容
};

static void multi_timer_create_lock(void)
{
    pthread_mutex_init(&s_msg_mutex, NULL);
}

static void multi_timer_lock(void)
{
    pthread_mutex_lock(&s_msg_mutex);
}

static void multi_timer_unlock(void)
{
    pthread_mutex_unlock(&s_msg_mutex);
}

// 信号处理函数 - 处理定时器超时
void handle_timeout(int signum)
{
    timeout_occurred = 1;
}

// 设置定时器
int set_timeout(int seconds)
{
    struct itimerval timer;

    // 注册信号处理函数
    signal(SIGALRM, handle_timeout);

    // 设置超时时间
    timer.it_value.tv_sec = seconds;    // 首次超时时间
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;       // 不重复
    timer.it_interval.tv_usec = 0;

    return setitimer(ITIMER_REAL, &timer, NULL);
}

//根据tag和msgindex创建ack id，用以匹配req和ack
//要作为msgrcv的msg type，必须大于0
//符号位要为0，所以使用7f匹配首字节
ST_LONG create_ack_id(ST_UINT32 tag, int msgid)
{
    ST_LONG id = ((tag << 8) & 0xFF000000) | ((tag << 16) & 0x00FF0000) | (msgid & 0xFFFF);

    return id;
}

static int timer_default_callback(int msgid)
{
    if (s_mqtt_message_fd < 0)
    {
        return -1;
    }

    t_Message message = { 0 };

    message.msg_type = msgid;	//mqtt msg id as msg_type
    message.msg_flag = MQTT_MESSAGE_TIMEOUT;

    DEBUG_TIME_LINE("msgid: %u", msgid);
    if (msgsnd(s_mqtt_message_fd, (void*) &message, sizeof(message) - sizeof(ST_LONG), 0) < 0)
    {
        DEBUG_TIME_LINE("msgsnd failed!!\n");
    }

    DEBUG_TIME_LINE("msgid: %u", msgid);

    return 0;
}

static void multi_timer_callback(int signal)
{
    int i;

    DEBUG_TIME_LINE("multi_timer_callback called!!\n");

    for (i = 0; i < MAX_TIMER_COUNT; i++)
    {
        if (multi_timer[i].valid && multi_timer[i].callback)
        {
            multi_timer[i].callback(multi_timer[i].tid);
        }
    }
}

int add_timer(ST_UINT32 ms, ST_UINT32 timer_id, int (*callback)(int timer_id))
{
    if (ms == 0 || callback == NULL)
    {
        return ERROR;
    }

    if (timer_id == 0)
    {
        return ERROR;
    }

    multi_timer_lock();
    //寻找相同tid，替换
    int empty_slot = -1;
    for (int i = 0; i < MAX_TIMER_COUNT; i++)
    {
        //无效槽添加timer用
        if (empty_slot == -1 && !multi_timer[i].valid)
            empty_slot = i;

        if (multi_timer[i].valid && timer_id == multi_timer[i].tid)
        {
            multi_timer[i].callback = callback;
            multi_timer[i].ms = ms;
            multi_timer[i].ms_left = ms;

            multi_timer_unlock();

            return OK;
        }
    }

    if (empty_slot == -1)
    {
        multi_timer_unlock();

        DEBUG_TIME_LINE("no timer slot!!\n");
        return ERROR;
    }

    multi_timer[empty_slot].tid = timer_id;
    multi_timer[empty_slot].callback = callback;
    multi_timer[empty_slot].ms = ms;
    multi_timer[empty_slot].ms_left = ms;
    multi_timer[empty_slot].valid = SD_TRUE;

    multi_timer_unlock();

    return OK;
}

int add_default_timer(ST_UINT32 timer_id)
{
    return add_timer(DEFAULT_TIMER_TIMEOUT, timer_id, timer_default_callback);
}

void del_timer(int timer_id)
{
    int i;

    multi_timer_lock();

    for (i = 0; i < MAX_TIMER_COUNT; i++)
    {
        if (multi_timer[i].valid && multi_timer[i].tid == timer_id)
        {
            multi_timer[i].valid = SD_FALSE;
        }
    }

    multi_timer_unlock();

    if (s_bMessageDebugEnable)
    {
        DEBUG_TIME_LINE("timer deleted!! tid 0x%x\n", timer_id);
    }
}

//数据中心返回消息超时机制，将一对req和ack使用消息队列的msgtype定向匹配
//发送消息时addtimer，收到消息后deltimer
//智慧台区TTU项目专用，注意和其他定时器可能有冲突
//用的话自己看实现自己定制，概不负责
static int init_multi_timer(void)
{
    unsigned int ms = DEFAULT_TIMER_TIMEOUT;

    signal(SIGALRM, multi_timer_callback);	// 初始化定时器

    struct itimerval tick = { 0 };

    tick.it_value.tv_sec = ms / 1000;
    tick.it_value.tv_usec = (ms % 1000) * 1000;

    if (setitimer(ITIMER_REAL, &tick, NULL) < 0)
    {
        if (s_bMessageDebugEnable)
            DEBUG_TIME_LINE("setitimer failed! ms = %u\n", ms);

        return ERROR;
    }

    multi_timer_create_lock();

    multi_timer_lock();
    memset(multi_timer, 0x0, sizeof(multi_timer));
    multi_timer_unlock();

    return OK;
}

int msg_create(char *name)
{
    if (name == NULL)
    {
        return -1;
    }

    int fd = -1;

    if (access(name, F_OK) != 0)
    {
        fd = open(name, O_CREAT | O_WRONLY, 0644);
        if (fd < 0)
        {
            DEBUG_TIME_LINE("fail to create ftok file");
            return -1;
        }

        close(fd);
    }

    key_t key = ftok(name, MMQ_FTOK_PROJ_ID);
    int count = 100;

    while ((fd = msgget(key, IPC_CREAT | IPC_EXCL | MMQ_MSG_PERMISSION)) == -1
            && count > 0)
    {
        if (errno == EEXIST)
        {
            // if MQ with the same key already exists, remove it and try again
            fd = msgget(key, 0);
            if (fd == -1)
            {
                DEBUG_TIME_LINE("fail to get old queue id");
                return -1;
            }

            if (msgctl(fd, IPC_RMID, NULL) == -1)
            {
                DEBUG_TIME_LINE("msgget() fail to delete old queue");
                return -1;
            }

            DEBUG_TIME_LINE("Removed old message queue (id=%d)\n", fd);
        }
        else
        {
            DEBUG_TIME_LINE("msgget() failed");
            return -1;
        }

        count--;
    }

    return fd;
}

int MessageInit(char *name)
{
    s_mqtt_message_fd = msg_create(name);
    if (s_mqtt_message_fd < 0)
    {
        return ERROR;
    }

    if (init_multi_timer() != OK)
    {
        return ERROR;
    }

    return OK;
}

// 带超时的消息接收
ssize_t msgrcv_timeout(int msqid, void *msgp, size_t msgsz, long msgtyp,
        int msgflg, int timeout_sec)
{
    ssize_t ret;

    // 重置超时标志
    timeout_occurred = 0;

    // 设置定时器
    if (set_timeout(timeout_sec) == -1)
    {
        perror("setitimer failed");
        return -1;
    }

    // 接收消息
    ret = msgrcv(msqid, msgp, msgsz, msgtyp, msgflg);

    // 关闭定时器
    struct itimerval zero_timer = { 0 };
    setitimer(ITIMER_REAL, &zero_timer, NULL);

    // 检查结果
    if (ret == -1)
    {
        if (errno == EINTR && timeout_occurred)
        {
            // 超时导致的中断
            errno = ETIMEDOUT;
            return -1;
        }
        // 其他错误
        return -1;
    }

    return ret;
}

int wait_for_smiOS_ack(ST_UINT32 tag, int msgid, ST_UINT32 *ack)
{
    if (s_mqtt_message_fd == -1)
    {
        return ERROR;
    }

    ST_UINT32 ackType = create_ack_id(tag, msgid);
    add_default_timer(ackType);

    t_Message msg = { 0 };
    msg.msg_type = ackType;

    if (msgrcv(s_mqtt_message_fd, (char*) &msg, sizeof(msg), ackType, 0) < 0)
    {
        return ERROR;
    }

    del_timer(ackType);

    if (msg.buf == NULL || msg.msg_len == 0 || msg.msg_flag == MQTT_MESSAGE_TIMEOUT)
    {
        return ERROR;
    }

    *ack = msg.buf[0];

    if (msg.buf_need_free)
    {
        free(msg.buf);
    }

    return OK;
}

void messagetest(char *name)
{
    ST_LONG type = create_ack_id(0x00050001, 0x1235);
    printf("type: %08lX\n", type);
}
