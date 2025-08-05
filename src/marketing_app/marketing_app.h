/* marketing_app.h */
#ifndef _MARKETING_APP_H_
#define _MARKETING_APP_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ===========================================
 * 协议相关常量定义 (基于Q/GDW 11778-2017)
 * =========================================== */

/* 通信帧相关常量 */
#define FRAME_START_CHAR 0x68
#define FRAME_END_CHAR 0x16
#define BROADCAST_ADDR 0xAA

/* 任务优先级相关常量 */
#define MAX_TASKS 255
#define MAX_PRIORITIES 256
#define HIGHEST_PRIORITY 0
#define LOWEST_PRIORITY 255

/* 数据项状态定义 */
typedef enum {
    DATA_ITEM_INIT = 0,     /* 初始化状态 */
    DATA_ITEM_RETRY,        /* 重试状态 */
    DATA_ITEM_NO_RESPONSE,  /* 无响应状态 */
    DATA_ITEM_UNSUPPORTED,  /* 不支持状态 */
    DATA_ITEM_SUCCESS       /* 抄读成功 */
} DataItemStatus;

/* 表计协议类型 */
typedef enum {
    PROTOCOL_DL645 = 0,
    PROTOCOL_QGDW11778,
    PROTOCOL_TRANSPARENT /* 透明转发 */
} MeterProtocolType;

/* 表计状态 */
typedef enum {
    METER_ONLINE = 0,
    METER_OFFLINE,
    METER_ERROR
} MeterStatus;

/* 任务状态 */
typedef enum {
    TASK_NOT_STARTED = 0,
    TASK_RUNNING,
    TASK_COMPLETED,
    TASK_PAUSED
} TaskStatus;

/* 数据结构定义 */
typedef struct {
    unsigned char year;
    unsigned char month;
    unsigned char day;
    unsigned char hour;
    unsigned char minute;
    unsigned char second;
} DateTime;

/* 表计档案信息 */
typedef struct {
    char meter_addr[16];        /* 表计地址 */
    MeterProtocolType protocol; /* 协议类型 */
    int port;                   /* 配置端口 */
    MeterStatus status;         /* 表计状态 */
    DateTime last_comm_time;    /* 最后通信时间 */
} MeterInfo;

/* 数据项定义 */
typedef struct {
    unsigned int item_id;       /* 数据项ID */
    DataItemStatus status;      /* 数据项状态 */
    int retry_count;            /* 重试次数 */
    time_t last_attempt_time;   /* 上次尝试时间 */
    char data[256];             /* 数据存储 */
    int data_len;               /* 数据长度 */
} DataItem;

/* 采集任务定义 */
typedef struct {
    int task_id;                /* 任务ID (0-254) */
    int priority;               /* 优先级 (0-255, 0为最高) */
    TaskStatus status;          /* 任务状态 */
    int port;                   /* 配置端口 */
    int meter_count;            /* 表计数量 */
    MeterInfo *meters;          /* 表计列表 */
    int data_item_count;        /* 数据项数量 */
    DataItem *data_items;       /* 数据项列表 */
    DateTime start_time;        /* 任务开始时间 */
    DateTime end_time;          /* 任务结束时间 */
    int success_count;          /* 成功采集数量 */
    int total_count;            /* 总采集数量 */
} CollectionTask;

/* 任务调度器 */
typedef struct {
    CollectionTask tasks[MAX_TASKS]; /* 任务列表 */
    int active_task_count;           /* 激活任务数量 */
    int current_priority;            /* 当前执行的优先级 */
    int highest_priority;            /* 最高优先级 */
    int lowest_priority;             /* 最低优先级 */
} TaskScheduler;

/* 通信帧结构 (基于Q/GDW 11778-2017) */
typedef struct {
    unsigned char start_char;    /* 起始字符(68H) */
    unsigned short length;       /* 长度域L */
    unsigned char control_field; /* 控制域C */
    unsigned char addr;          /* 地址域A */
    unsigned short hcs;          /* 帧头校验HCS */
    unsigned char *user_data;    /* 链路用户数据 */
    int user_data_len;           /* 用户数据长度 */
    unsigned short fcs;          /* 帧校验FCS */
    unsigned char end_char;      /* 结束字符(16H) */
} ProtocolFrame;

/* 函数声明 */
/* 任务管理相关函数 */
void init_task_scheduler(TaskScheduler *scheduler);
int add_collection_task(TaskScheduler *scheduler, CollectionTask *task);
void activate_task(TaskScheduler *scheduler, int task_id);
void deactivate_task(TaskScheduler *scheduler, int task_id);
void execute_next_task(TaskScheduler *scheduler);
void execute_task_by_priority(TaskScheduler *scheduler, int priority);
void reset_data_item_status(CollectionTask *task);

/* 表计管理相关函数 */
int init_meter_info(MeterInfo *meter, const char *addr, MeterProtocolType protocol, int port);
int send_meter_request(MeterInfo *meter, const unsigned char *request, int req_len, 
                      unsigned char *response, int *resp_len, int timeout);
int parse_meter_response(MeterInfo *meter, const unsigned char *response, int resp_len);

/* 数据项管理相关函数 */
void update_data_item_status(DataItem *item, DataItemStatus status);
int get_next_data_item_to_read(CollectionTask *task, int *meter_index, int *item_index);
int read_data_item(MeterInfo *meter, DataItem *item);
void handle_data_item_response(DataItem *item, int response_status, 
                              const unsigned char *data, int data_len);
void retry_unsuccessful_data_items(CollectionTask *task);
void reset_no_response_items(CollectionTask *task);

/* 通信协议相关函数 */
ProtocolFrame* create_protocol_frame(int addr, int control_field, 
                                    const unsigned char *data, int data_len);
int parse_protocol_frame(const unsigned char *buffer, int buffer_len, ProtocolFrame *frame);
void free_protocol_frame(ProtocolFrame *frame);
unsigned short calculate_hcs(const unsigned char *data, int len);
unsigned short calculate_fcs(const unsigned char *data, int len);
int build_qgdw11778_request(unsigned char *buffer, int *buf_len, int meter_addr, 
                           int object_id, int attribute_id);
int parse_qgdw11778_response(const unsigned char *buffer, int buf_len, 
                            int *object_id, int *attribute_id, unsigned char *data, int *data_len);

/* 路由管理相关函数 */
void sync_archive(TaskScheduler *scheduler);
void restart_routing(TaskScheduler *scheduler);
void clear_routing(TaskScheduler *scheduler);
void check_and_maintain_routing(TaskScheduler *scheduler);

/* 抄表统计相关函数 */
void calculate_collection_statistics(CollectionTask *task);
void log_collection_result(CollectionTask *task);

#endif /* _MARKETING_APP_H_ */