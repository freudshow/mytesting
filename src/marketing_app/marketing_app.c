/* marketing_app.c */
#include "marketing_app.h"
#include <unistd.h>
/* ===========================================
 * 任务管理模块实现
 * =========================================== */

/**
 * 初始化任务调度器
 */
void init_task_scheduler(TaskScheduler *scheduler)
{
    memset(scheduler, 0, sizeof(TaskScheduler));
    scheduler->current_priority = HIGHEST_PRIORITY;
    scheduler->highest_priority = HIGHEST_PRIORITY;
    scheduler->lowest_priority = LOWEST_PRIORITY;
}

/**
 * 添加采集任务
 */
int add_collection_task(TaskScheduler *scheduler, CollectionTask *task)
{
    if (scheduler->active_task_count >= MAX_TASKS)
    {
        return -1; /* 任务数量已达上限 */
    }

    /* 检查任务ID是否有效 */
    if (task->task_id < 0 || task->task_id >= MAX_TASKS)
    {
        return -2;
    }

    /* 检查优先级是否有效 */
    if (task->priority < 0 || task->priority >= MAX_PRIORITIES)
    {
        return -3;
    }

    /* 复制任务信息 */
    memcpy(&scheduler->tasks[task->task_id], task, sizeof(CollectionTask));

    /* 更新最高/最低优先级 */
    if (task->priority < scheduler->highest_priority)
    {
        scheduler->highest_priority = task->priority;
    }
    if (task->priority > scheduler->lowest_priority)
    {
        scheduler->lowest_priority = task->priority;
    }

    scheduler->active_task_count++;
    return 0;
}

/**
 * 激活采集任务
 */
void activate_task(TaskScheduler *scheduler, int task_id)
{
    if (task_id < 0 || task_id >= MAX_TASKS)
    {
        return;
    }

    CollectionTask *task = &scheduler->tasks[task_id];
    task->status = TASK_RUNNING;

    /* 重置数据项状态 */
    reset_data_item_status(task);

    /* 获取当前系统时间作为开始时间 */
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    task->start_time.year = tm_now->tm_year + 1900;
    task->start_time.month = tm_now->tm_mon + 1;
    task->start_time.day = tm_now->tm_mday;
    task->start_time.hour = tm_now->tm_hour;
    task->start_time.minute = tm_now->tm_min;
    task->start_time.second = tm_now->tm_sec;
}

/**
 * 重置数据项状态
 */
void reset_data_item_status(CollectionTask *task)
{
    for (int i = 0; i < task->data_item_count; i++)
    {
        task->data_items[i].status = DATA_ITEM_INIT;
        task->data_items[i].retry_count = 0;
    }
}

/**
 * 按优先级执行任务
 */
void execute_task_by_priority(TaskScheduler *scheduler, int priority)
{
    /* 按任务ID顺序遍历所有任务 */
    for (int task_id = 0; task_id < MAX_TASKS; task_id++)
    {
        CollectionTask *task = &scheduler->tasks[task_id];

        /* 只处理指定优先级的运行中任务 */
        if (task->status == TASK_RUNNING && task->priority == priority)
        {
            int meter_index, item_index;

            /* 获取下一个需要抄读的数据项 */
            if (get_next_data_item_to_read(task, &meter_index, &item_index) == 0)
            {
                MeterInfo *meter = &task->meters[meter_index];
                DataItem *item = &task->data_items[item_index];

                /* 执行抄表操作 */
                if (read_data_item(meter, item) == 0)
                {
                    /* 抄读成功，更新状态 */
                    update_data_item_status(item, DATA_ITEM_SUCCESS);
                    task->success_count++;
                }
                else
                {
                    /* 抄读失败，处理异常 */
                    item->retry_count++;
                    if (item->retry_count >= 3)
                    { /* 假设最大重试次数为3 */
                        update_data_item_status(item, DATA_ITEM_NO_RESPONSE);
                    }
                    else
                    {
                        update_data_item_status(item, DATA_ITEM_RETRY);
                    }
                }

                /* 更新总采集数量 */
                task->total_count++;
            }
            else
            {
                /* 任务完成 */
                task->status = TASK_COMPLETED;

                /* 记录结束时间 */
                time_t now = time(NULL);
                struct tm *tm_now = localtime(&now);
                task->end_time.year = tm_now->tm_year + 1900;
                task->end_time.month = tm_now->tm_mon + 1;
                task->end_time.day = tm_now->tm_mday;
                task->end_time.hour = tm_now->tm_hour;
                task->end_time.minute = tm_now->tm_min;
                task->end_time.second = tm_now->tm_sec;

                /* 计算并记录统计信息 */
                calculate_collection_statistics(task);
                log_collection_result(task);
            }
        }
    }
}

/**
 * 执行下一个任务
 */
void execute_next_task(TaskScheduler *scheduler)
{
    /* 从当前优先级开始，查找有任务可执行的优先级 */
    for (int priority = scheduler->current_priority; priority <= scheduler->lowest_priority; priority++)
    {

        int has_active_task = 0;
        /* 检查该优先级是否有运行中的任务 */
        for (int i = 0; i < MAX_TASKS; i++)
        {
            if (scheduler->tasks[i].status == TASK_RUNNING && scheduler->tasks[i].priority == priority)
            {
                has_active_task = 1;
                break;
            }
        }

        if (has_active_task)
        {
            /* 执行该优先级的任务 */
            execute_task_by_priority(scheduler, priority);
            scheduler->current_priority = priority;
            return;
        }
    }

    /* 没有更高优先级任务，检查是否需要切换到低优先级 */
    for (int priority = HIGHEST_PRIORITY; priority < scheduler->current_priority; priority++)
    {

        int has_active_task = 0;
        /* 检查该优先级是否有运行中的任务 */
        for (int i = 0; i < MAX_TASKS; i++)
        {
            if (scheduler->tasks[i].status == TASK_RUNNING && scheduler->tasks[i].priority == priority)
            {
                has_active_task = 1;
                break;
            }
        }

        if (has_active_task)
        {
            /* 检查高优先级任务是否已全部完成 */
            int higher_priority_done = 1;
            for (int p = HIGHEST_PRIORITY; p < priority; p++)
            {
                int has_running_task = 0;
                for (int i = 0; i < MAX_TASKS; i++)
                {
                    if (scheduler->tasks[i].status == TASK_RUNNING && scheduler->tasks[i].priority == p)
                    {
                        has_running_task = 1;
                        break;
                    }
                }
                if (has_running_task)
                {
                    higher_priority_done = 0;
                    break;
                }
            }

            if (higher_priority_done)
            {
                /* 可以切换到该优先级 */
                execute_task_by_priority(scheduler, priority);
                scheduler->current_priority = priority;
                return;
            }
        }
    }
}

/* ===========================================
 * 表计管理模块实现
 * =========================================== */

/**
 * 初始化表计信息
 */
int init_meter_info(MeterInfo *meter, const char *addr, MeterProtocolType protocol, int port)
{
    if (strlen(addr) >= sizeof(meter->meter_addr))
    {
        return -1; /* 地址过长 */
    }

    strcpy(meter->meter_addr, addr);
    meter->protocol = protocol;
    meter->port = port;
    meter->status = METER_OFFLINE;

    /* 初始化时间为0 */
    memset(&meter->last_comm_time, 0, sizeof(DateTime));

    return 0;
}

/**
 * 发送表计请求
 */
int send_meter_request(MeterInfo *meter, const unsigned char *request, int req_len, unsigned char *response, int *resp_len, int timeout)
{
    /* 这里应该实现与硬件端口的交互 */
    /* 为简化示例，我们假设通过串口或网络发送请求 */

    printf("Sending request to meter %s on port %d\n", meter->meter_addr, meter->port);

    /* 实际应用中，这里会调用底层通信接口 */
    /* 例如：serial_send(meter->port, request, req_len); */

    /* 模拟响应 (实际应用中会从通信端口读取响应) */
    if (rand() % 10 > 2)
    { /* 80%概率成功 */
        /* 生成模拟响应 */
        int simulated_resp_len = rand() % 100 + 20;
        if (simulated_resp_len > *resp_len)
        {
            simulated_resp_len = *resp_len;
        }

        /* 填充模拟响应数据 */
        for (int i = 0; i < simulated_resp_len; i++)
        {
            response[i] = rand() % 256;
        }

        *resp_len = simulated_resp_len;
        meter->status = METER_ONLINE;

        /* 更新最后通信时间 */
        time_t now = time(NULL);
        struct tm *tm_now = localtime(&now);
        meter->last_comm_time.year = tm_now->tm_year + 1900;
        meter->last_comm_time.month = tm_now->tm_mon + 1;
        meter->last_comm_time.day = tm_now->tm_mday;
        meter->last_comm_time.hour = tm_now->tm_hour;
        meter->last_comm_time.minute = tm_now->tm_min;
        meter->last_comm_time.second = tm_now->tm_sec;

        return 0; /* 成功 */
    }
    else
    {
        meter->status = METER_OFFLINE;
        return -1; /* 失败 */
    }
}

/**
 * 解析表计响应
 */
int parse_meter_response(MeterInfo *meter, const unsigned char *response, int resp_len)
{
    /* 根据表计协议类型解析响应 */
    switch (meter->protocol)
    {
        case PROTOCOL_DL645:
            /* DL/T 645 协议解析 */
            printf("Parsing DL/T 645 response from meter %s\n", meter->meter_addr);
            /* 实现DL/T 645解析逻辑 */
            return 0;

        case PROTOCOL_QGDW11778:
            /* Q/GDW 11778 协议解析 */
            printf("Parsing Q/GDW 11778 response from meter %s\n", meter->meter_addr);
            /* 实现Q/GDW 11778解析逻辑 */
            return 0;

        case PROTOCOL_TRANSPARENT:
            /* 透明转发，不做解析 */
            printf("Transparent forwarding for meter %s\n", meter->meter_addr);
            return 0;

        default:
            return -1;
    }
}

/* ===========================================
 * 数据项管理模块实现
 * =========================================== */

/**
 * 更新数据项状态
 */
void update_data_item_status(DataItem *item, DataItemStatus status)
{
    item->status = status;
    if (status == DATA_ITEM_INIT)
    {
        item->retry_count = 0;
    }
    item->last_attempt_time = time(NULL);
}

/**
 * 获取下一个需要抄读的数据项
 * 返回0表示找到，-1表示没有需要抄读的数据项
 */
int get_next_data_item_to_read(CollectionTask *task, int *meter_index, int *item_index)
{
    /* 按照营销app需求6.5.1.3.2要求：
     a) 从最高优先级开始，到当前优先级为止，按顺序抄读所有未抄成功的数据项
     b) 先抄读在线可抄读的表计
     */

    /* 首先检查是否有初始化状态的数据项 */
    for (int m = 0; m < task->meter_count; m++)
    {
        for (int i = 0; i < task->data_item_count; i++)
        {
            if (task->data_items[i].status == DATA_ITEM_INIT)
            {
                *meter_index = m;
                *item_index = i;
                return 0;
            }
        }
    }

    /* 检查重试状态的数据项 */
    for (int m = 0; m < task->meter_count; m++)
    {
        for (int i = 0; i < task->data_item_count; i++)
        {
            if (task->data_items[i].status == DATA_ITEM_RETRY)
            {
                *meter_index = m;
                *item_index = i;
                return 0;
            }
        }
    }

    /* 没有需要抄读的数据项 */
    return -1;
}

/**
 * 读取数据项
 */
int read_data_item(MeterInfo *meter, DataItem *item)
{
    unsigned char request[256];
    int req_len = 0;
    unsigned char response[512];
    int resp_len = sizeof(response);

    /* 根据协议构建请求 */
    if (meter->protocol == PROTOCOL_QGDW11778)
    {
        /* 构建Q/GDW 11778协议请求 */
        if (build_qgdw11778_request(request, &req_len, atoi(meter->meter_addr), item->item_id, 2) != 0)
        {
            return -1;
        }
    }
    else
    {
        /* 其他协议的请求构建 */
        /* ... */
        return -1;
    }

    /* 发送请求并获取响应 */
    if (send_meter_request(meter, request, req_len, response, &resp_len, 5000) != 0)
    {
        return -1;
    }

    /* 解析响应 */
    int object_id, attribute_id;
    unsigned char data[256];
    int data_len = sizeof(data);

    if (parse_qgdw11778_response(response, resp_len, &object_id, &attribute_id, data, &data_len) != 0)
    {
        return -1;
    }

    /* 处理响应 */
    handle_data_item_response(item, 0, data, data_len);
    return 0;
}

/**
 * 处理数据项响应
 */
void handle_data_item_response(DataItem *item, int response_status, const unsigned char *data, int data_len)
{
    if (response_status == 0)
    {
        /* 成功响应 */
        if (data_len > 0 && data_len <= sizeof(item->data))
        {
            memcpy(item->data, data, data_len);
            item->data_len = data_len;
            update_data_item_status(item, DATA_ITEM_SUCCESS);
        }
        else
        {
            /* 数据长度异常 */
            update_data_item_status(item, DATA_ITEM_RETRY);
        }
    }
    else if (response_status == 1)
    {
        /* 表计否认帧，不支持该数据项 */
        update_data_item_status(item, DATA_ITEM_UNSUPPORTED);
    }
    else
    {
        /* 其他错误 */
        update_data_item_status(item, DATA_ITEM_RETRY);
    }
}

/**
 * 重试未成功抄读的数据项
 */
void retry_unsuccessful_data_items(CollectionTask *task)
{
    for (int i = 0; i < task->data_item_count; i++)
    {
        if (task->data_items[i].status == DATA_ITEM_RETRY || task->data_items[i].status == DATA_ITEM_NO_RESPONSE)
        {
            /* 重置状态为初始化，以便再次尝试 */
            update_data_item_status(&task->data_items[i], DATA_ITEM_INIT);
        }
    }
}

/**
 * 重置无响应数据项状态
 * (当优先级状态发生变化时调用)
 */
void reset_no_response_items(CollectionTask *task)
{
    for (int i = 0; i < task->data_item_count; i++)
    {
        if (task->data_items[i].status == DATA_ITEM_NO_RESPONSE)
        {
            update_data_item_status(&task->data_items[i], DATA_ITEM_INIT);
        }
    }
}

/* ===========================================
 * 通信协议实现 (基于Q/GDW 11778-2017)
 * =========================================== */

/**
 * 创建协议帧
 */
ProtocolFrame* create_protocol_frame(int addr, int control_field, const unsigned char *data, int data_len)
{
    ProtocolFrame *frame = (ProtocolFrame*) malloc(sizeof(ProtocolFrame));
    if (!frame)
        return NULL;

    /* 初始化帧结构 */
    frame->start_char = FRAME_START_CHAR;
    frame->length = data_len;
    frame->control_field = control_field;
    frame->addr = addr;

    /* 计算HCS (帧头校验) */
    unsigned char header[6];
    header[0] = frame->length & 0xFF;
    header[1] = (frame->length >> 8) & 0xFF;
    header[2] = frame->control_field;
    header[3] = frame->addr;
    frame->hcs = calculate_hcs(header, 4);

    /* 复制用户数据 */
    if (data_len > 0)
    {
        frame->user_data = (unsigned char*) malloc(data_len);
        if (!frame->user_data)
        {
            free(frame);
            return NULL;
        }
        memcpy(frame->user_data, data, data_len);
        frame->user_data_len = data_len;
    }
    else
    {
        frame->user_data = NULL;
        frame->user_data_len = 0;
    }

    /* 计算FCS (帧校验) */
    unsigned char *fcs_data = (unsigned char*) malloc(6 + data_len);
    if (!fcs_data)
    {
        free_protocol_frame(frame);
        return NULL;
    }

    fcs_data[0] = frame->length & 0xFF;
    fcs_data[1] = (frame->length >> 8) & 0xFF;
    fcs_data[2] = frame->control_field;
    fcs_data[3] = frame->addr;
    fcs_data[4] = frame->hcs & 0xFF;
    fcs_data[5] = (frame->hcs >> 8) & 0xFF;
    if (data_len > 0)
    {
        memcpy(fcs_data + 6, data, data_len);
    }

    frame->fcs = calculate_fcs(fcs_data, 6 + data_len);
    free(fcs_data);

    frame->end_char = FRAME_END_CHAR;

    return frame;
}

/**
 * 解析协议帧
 */
int parse_protocol_frame(const unsigned char *buffer, int buffer_len, ProtocolFrame *frame)
{
    if (buffer_len < 11)
    { /* 最小帧长度 */
        return -1;
    }

    /* 检查起始字符 */
    if (buffer[0] != FRAME_START_CHAR || buffer[1] != FRAME_START_CHAR)
    {
        return -1;
    }

    /* 提取长度域 */
    frame->length = buffer[2] | (buffer[3] << 8);
    if (frame->length + 8 > buffer_len)
    { /* 帧长度不匹配 */
        return -1;
    }

    /* 提取其他字段 */
    frame->control_field = buffer[4];
    frame->addr = buffer[5];
    frame->hcs = buffer[6] | (buffer[7] << 8);

    /* 验证HCS */
    unsigned char header[4];
    header[0] = buffer[2];
    header[1] = buffer[3];
    header[2] = buffer[4];
    header[3] = buffer[5];
    unsigned short calculated_hcs = calculate_hcs(header, 4);
    if (calculated_hcs != frame->hcs)
    {
        return -2; /* HCS校验失败 */
    }

    /* 提取用户数据 */
    frame->user_data_len = frame->length;
    if (frame->user_data_len > 0)
    {
        frame->user_data = (unsigned char*) malloc(frame->user_data_len);
        if (!frame->user_data)
        {
            return -3;
        }
        memcpy(frame->user_data, buffer + 8, frame->user_data_len);
    }
    else
    {
        frame->user_data = NULL;
    }

    /* 提取FCS和结束字符 */
    frame->fcs = buffer[8 + frame->user_data_len] | (buffer[9 + frame->user_data_len] << 8);
    frame->end_char = buffer[10 + frame->user_data_len];

    /* 验证FCS */
    int fcs_data_len = 6 + frame->user_data_len;
    unsigned char *fcs_data = (unsigned char*) malloc(fcs_data_len);
    if (!fcs_data)
    {
        return -3;
    }

    fcs_data[0] = buffer[2];
    fcs_data[1] = buffer[3];
    fcs_data[2] = buffer[4];
    fcs_data[3] = buffer[5];
    fcs_data[4] = buffer[6];
    fcs_data[5] = buffer[7];
    if (frame->user_data_len > 0)
    {
        memcpy(fcs_data + 6, buffer + 8, frame->user_data_len);
    }

    unsigned short calculated_fcs = calculate_fcs(fcs_data, fcs_data_len);
    free(fcs_data);

    if (calculated_fcs != frame->fcs)
    {
        return -4; /* FCS校验失败 */
    }

    /* 检查结束字符 */
    if (frame->end_char != FRAME_END_CHAR)
    {
        return -5;
    }

    return 0;
}

/**
 * 释放协议帧资源
 */
void free_protocol_frame(ProtocolFrame *frame)
{
    if (frame->user_data)
    {
        free(frame->user_data);
    }
    free(frame);
}

/**
 * 计算HCS (帧头校验)
 * 根据Q/GDW 11778-2017附录D实现
 */
unsigned short calculate_hcs(const unsigned char *data, int len)
{
    unsigned short hcs = 0xFFFF;
    for (int i = 0; i < len; i++)
    {
        hcs ^= (data[i] << 8);
        for (int j = 0; j < 8; j++)
        {
            if (hcs & 0x8000)
            {
                hcs = (hcs << 1) ^ 0x1021;
            }
            else
            {
                hcs <<= 1;
            }
            hcs &= 0xFFFF;
        }
    }
    return hcs;
}

/**
 * 计算FCS (帧校验)
 * 根据Q/GDW 11778-2017附录D实现
 */
unsigned short calculate_fcs(const unsigned char *data, int len)
{
    return calculate_hcs(data, len);
}

/**
 * 构建Q/GDW 11778协议请求
 */
int build_qgdw11778_request(unsigned char *buffer, int *buf_len, int meter_addr, int object_id, int attribute_id)
{
    /* 根据Q/GDW 11778-2017构建GET请求 */

    /* 示例：构建一个GET-Request Normal请求 */
    int pos = 0;

    /* 应用连接建立 (假设已建立) */

    /* GET-Request Normal */
    buffer[pos++] = 0x05; /* GET-Request */
    buffer[pos++] = 0x01; /* GetRequestNormal */

    /* 服务序号-优先级 (PIID) */
    buffer[pos++] = 0x01; /* 服务序号1，优先级0 */

    /* OAD (对象属性描述符) */
    /* OI (对象标识) - 2字节 */
    buffer[pos++] = (object_id >> 8) & 0xFF;
    buffer[pos++] = object_id & 0xFF;

    /* 属性ID */
    buffer[pos++] = attribute_id;

    /* 无时间标签 */
    buffer[pos++] = 0x00;

    *buf_len = pos;

    /* 实际应用中需要构建完整的通信帧 */
    return 0;
}

/**
 * 解析Q/GDW 11778协议响应
 */
int parse_qgdw11778_response(const unsigned char *buffer, int buf_len, int *object_id, int *attribute_id, unsigned char *data, int *data_len)
{
    /* 根据Q/GDW 11778-2017解析GET响应 */

    if (buf_len < 8)
    {
        return -1; /* 响应太短 */
    }

    /* 检查是否为GET-Response */
    if (buffer[0] != 0x85)
    { /* [133] GET-Response */
        return -2;
    }

    /* 检查是否为GetResponseNormal */
    if (buffer[1] != 0x01)
    { /* [1] GetResponseNormal */
        return -3;
    }

    /* 服务序号-优先级-ACD */
    /* buffer[2] */

    /* OAD */
    *object_id = (buffer[3] << 8) | buffer[4];
    *attribute_id = buffer[5];

    /* 数据类型 */
    int data_type = buffer[6];
    (void) data_type; /* 未使用 */

    /* 数据长度 */
    int length = buffer[7];

    /* 检查数据长度是否匹配 */
    if (length > *data_len || 8 + length > buf_len)
    {
        return -4;
    }

    /* 复制数据 */
    memcpy(data, &buffer[8], length);
    *data_len = length;

    return 0;
}

/* ===========================================
 * 路由管理模块实现
 * =========================================== */

/**
 * 档案同步
 */
void sync_archive(TaskScheduler *scheduler)
{
    /* 每日过零点后执行档案同步 */
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);

    /* 检查是否为零点后 */
    if (tm_now->tm_hour == 0 && tm_now->tm_min < 5)
    {
        printf("Performing archive synchronization at %04d-%02d-%02d %02d:%02d:%02d\n", tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);

        /* 暂停数据抄读 */
        for (int i = 0; i < MAX_TASKS; i++)
        {
            if (scheduler->tasks[i].status == TASK_RUNNING)
            {
                scheduler->tasks[i].status = TASK_PAUSED;
            }
        }

        /* 执行差异部分的档案增减 */
        /* 这里应该实现与数据中心的交互 */

        /* 恢复数据抄读 */
        for (int i = 0; i < MAX_TASKS; i++)
        {
            if (scheduler->tasks[i].status == TASK_PAUSED)
            {
                scheduler->tasks[i].status = TASK_RUNNING;
            }
        }
    }
}

/**
 * 重启路由
 */
void restart_routing(TaskScheduler *scheduler)
{
    /* 检查与本地通信管理APP是否长时间无交互 */
    time_t now = time(NULL);

    for (int i = 0; i < MAX_TASKS; i++)
    {
        if (scheduler->tasks[i].status == TASK_RUNNING)
        {
            /* 检查最后通信时间 */
            for (int m = 0; m < scheduler->tasks[i].meter_count; m++)
            {
                time_t last_comm = mktime((struct tm[] ) { { scheduler->tasks[i].meters[m].last_comm_time.year - 1900, scheduler->tasks[i].meters[m].last_comm_time.month - 1, scheduler->tasks[i].meters[m].last_comm_time.day, scheduler->tasks[i].meters[m].last_comm_time.hour, scheduler->tasks[i].meters[m].last_comm_time.minute, scheduler->tasks[i].meters[m].last_comm_time.second, 0, 0, -1 } });

                if (now - last_comm > 300)
                { /* 5分钟无通信 */
                    printf("Restarting routing for meter %s (no communication for 5 minutes)\n", scheduler->tasks[i].meters[m].meter_addr);
                    /* 这里应该实现重启本地通信管理APP的逻辑 */
                }
            }
        }
    }
}

/**
 * 清除路由
 */
void clear_routing(TaskScheduler *scheduler)
{
    static time_t last_clear_time = 0;
    time_t now = time(NULL);

    /* 每月仅允许执行一次清除路由动作 */
    struct tm *tm_now = localtime(&now);
    struct tm *tm_last = localtime(&last_clear_time);

    if (tm_now->tm_year > tm_last->tm_year || (tm_now->tm_year == tm_last->tm_year && tm_now->tm_mon > tm_last->tm_mon))
    {
        /* 统计近期抄表成功率 */
        int success_count = 0;
        int total_count = 0;

        for (int i = 0; i < MAX_TASKS; i++)
        {
            if (scheduler->tasks[i].status == TASK_COMPLETED)
            {
                success_count += scheduler->tasks[i].success_count;
                total_count += scheduler->tasks[i].total_count;
            }
        }

        if (total_count > 0)
        {
            float success_rate = (float) success_count / total_count;
            /* 假设均值为90% */
            if (success_rate < 0.85)
            { /* 低于85% */
                printf("Clearing routing due to low collection success rate (%.2f%%)\n", success_rate * 100);

                /* 清除本地通信模块的路由信息 */
                /* 重新执行档案同步 */

                last_clear_time = now;
            }
        }
    }
}

/**
 * 检查并维护路由
 */
void check_and_maintain_routing(TaskScheduler *scheduler)
{
    /* 定期检查路由状态 */
    restart_routing(scheduler);
    clear_routing(scheduler);
}

/* ===========================================
 * 抄表统计模块实现
 * =========================================== */

/**
 * 计算采集统计信息
 */
void calculate_collection_statistics(CollectionTask *task)
{
    if (task->total_count > 0)
    {
        task->success_count = 0;
        for (int i = 0; i < task->data_item_count; i++)
        {
            if (task->data_items[i].status == DATA_ITEM_SUCCESS)
            {
                task->success_count++;
            }
        }
    }
}

/**
 * 记录采集结果
 */
void log_collection_result(CollectionTask *task)
{
    float success_rate = 0.0;
    if (task->total_count > 0)
    {
        success_rate = (float) task->success_count / task->total_count;
    }

    printf("Task %d (Priority %d) completed:\n", task->task_id, task->priority);
    printf("  Start Time: %04d-%02d-%02d %02d:%02d:%02d\n", task->start_time.year, task->start_time.month, task->start_time.day, task->start_time.hour, task->start_time.minute, task->start_time.second);
    printf("  End Time: %04d-%02d-%02d %02d:%02d:%02d\n", task->end_time.year, task->end_time.month, task->end_time.day, task->end_time.hour, task->end_time.minute, task->end_time.second);
    printf("  Success Rate: %.2f%% (%d/%d)\n", success_rate * 100, task->success_count, task->total_count);
}

/* ===========================================
 * 主程序
 * =========================================== */

/**
 * 模拟主循环
 */
int marketapp_main()
{
    TaskScheduler scheduler;
    init_task_scheduler(&scheduler);

    /* 创建并添加任务 */
    CollectionTask task1;
    task1.task_id = 0;
    task1.priority = 5; /* 优先级5 */
    task1.port = 1;
    task1.meter_count = 10;
    task1.data_item_count = 5;

    /* 分配表计和数据项内存 */
    task1.meters = (MeterInfo*) malloc(task1.meter_count * sizeof(MeterInfo));
    task1.data_items = (DataItem*) malloc(task1.data_item_count * sizeof(DataItem));

    /* 初始化表计 */
    for (int i = 0; i < task1.meter_count; i++)
    {
        char addr[100];
        snprintf(addr, sizeof(addr) - 1, "100%d", i);
        init_meter_info(&task1.meters[i], addr, PROTOCOL_QGDW11778, task1.port);
    }

    /* 初始化数据项 */
    for (int i = 0; i < task1.data_item_count; i++)
    {
        task1.data_items[i].item_id = i + 1;
        task1.data_items[i].status = DATA_ITEM_INIT;
        task1.data_items[i].retry_count = 0;
        task1.data_items[i].data_len = 0;
    }

    /* 添加任务到调度器 */
    add_collection_task(&scheduler, &task1);

    /* 激活任务 */
    activate_task(&scheduler, 0);

    /* 模拟执行任务 */
    printf("Starting task execution...\n");
    for (int i = 0; i < 20; i++)
    { /* 模拟20次执行周期 */
        printf("\n=== Execution Cycle %d ===\n", i + 1);
        execute_next_task(&scheduler);

        /* 检查路由 */
        check_and_maintain_routing(&scheduler);

        /* 档案同步 (每日过零点) */
        sync_archive(&scheduler);

        /* 短暂延迟 */
#ifdef _WIN32
        Sleep(1000);
        #else
        sleep(1);
#endif
    }

    /* 释放内存 */
    free(task1.meters);
    free(task1.data_items);

    printf("\nProgram completed.\n");
    return 0;
}
