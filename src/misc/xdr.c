#include <stdio.h>
#include <stdlib.h>
#include <rpc/xdr.h>  // 包含 XDR 头文件
#include "basedef.h"

// 定义自定义结构体(用户信息)
typedef struct User {
    int id;         // 用户 ID
    char *name;     // 用户名(动态字符串)
    int age;        // 年龄
} User_s;

typedef struct ClassRootm {
    int roomID;
    double area;
    int stuCount;
    User_s *students;
} ClassRootm_s;

// 定义结构体的 XDR 处理函数(序列化/反序列化)
bool_t xdr_User(XDR *xdr, User_s *user)
{
    // 依次处理结构体成员：id -> name -> age
    // xdr_string 需传入字符串指针的地址，以及最大长度(这里限制为 100)
    if (!xdr_int(xdr, &user->id))
        return FALSE;
    if (!xdr_string(xdr, &user->name, 100))
        return FALSE;
    if (!xdr_int(xdr, &user->age))
        return FALSE;
    return TRUE;
}

bool_t xdr_ClassRoom(XDR *xdr, ClassRootm_s *room)
{
    // 1. 处理基本类型（roomID/area/stuCount）
    // 基本类型在 XDR_FREE 模式下无需特殊处理（无动态内存）
    if (!xdr_int(xdr, &room->roomID))
        return FALSE;
    if (!xdr_double(xdr, &room->area))
        return FALSE;
    if (!xdr_int(xdr, &room->stuCount))
        return FALSE;

    // 2. 处理动态数组 students（核心释放逻辑）
    // 区分模式：编码/解码 或 释放
    if (xdr->x_op == XDR_FREE)
    {
        // 【释放模式】：先释放数组中的每个 User_s 元素，再释放数组本身
        if (room->students != NULL && room->stuCount > 0)
        {
            // 释放数组中的每个学生（调用 xdr_User 的 XDR_FREE 逻辑）
            for (int i = 0; i < room->stuCount; i++)
            {
                xdr_User(xdr, &room->students[i]);  // 内部会释放 user->name
            }
            // 释放 students 数组本身（动态分配的内存）
            free(room->students);
            room->students = NULL;  // 置空避免悬垂指针
            room->stuCount = 0;     // 重置计数（可选，增强安全性）
        }
    }
    else
    {
        if (xdr->x_op == XDR_DECODE)
        {
            room->students = mem_alloc(room->stuCount * sizeof(User_s));
        }

        // 【编码/解码模式】：正常处理数组（假设解码时已分配数组内存）
        // 注意：XDR 本身不直接处理动态数组分配，需提前分配好 students 数组
        for (int i = 0; i < room->stuCount; i++)
        {
            if (!xdr_User(xdr, &room->students[i]))
                return FALSE;
        }
    }

    return TRUE;
}

int xdrmain(void)
{
    // 1. 准备本地数据
    User_s user_send = { 1, "Alice", 25 };
    User_s user_recv;  // 用于接收反序列化后的数据
    char buffer[8192];      // 缓冲区存储 XDR 格式数据

    // 2. 初始化 XDR 上下文(写模式：序列化到缓冲区)
    XDR xdr_write;
    xdrmem_create(&xdr_write, buffer, sizeof(buffer), XDR_ENCODE);  // XDR_ENCODE 表示序列化

    // 3. 序列化数据到缓冲区
    if (!xdr_User(&xdr_write, &user_send))
    {
        fprintf(stderr, "序列化失败\n");
        return 1;
    }

    xdr_destroy(&xdr_write);  // 结束写操作

    size_t encoded_len = xdr_getpos(&xdr_write);
    DEBUG_BUFF_FORMAT(buffer, encoded_len, "encode: --->>> ");

    // 4. 初始化 XDR 上下文(读模式：从缓冲区反序列化)
    XDR xdr_read;
    xdrmem_create(&xdr_read, buffer, sizeof(buffer), XDR_DECODE);  // XDR_DECODE 表示反序列化

    // 5. 反序列化数据到 user_recv
    user_recv.name = NULL;  // 初始化字符串指针(xdr_string 会动态分配内存)
    if (!xdr_User(&xdr_read, &user_recv))
    {
        fprintf(stderr, "反序列化失败\n");
        return 1;
    }

    xdr_destroy(&xdr_read);  // 结束读操作

    // 6. 打印反序列化结果
    printf("反序列化结果：id=%d, name=%s, age=%d\n",
            user_recv.id, user_recv.name, user_recv.age);

    // 7. 释放动态分配的内存(xdr_string 分配的)
    XDR xdr_freemem;
    xdrmem_create(&xdr_freemem, buffer, sizeof(buffer), XDR_FREE);
    if (!xdr_User(&xdr_freemem, &user_recv))
    {
        fprintf(stderr, "释放内存失败\n");
        return 1;
    }

    xdr_destroy(&xdr_freemem);  // 结束读操作

    /*-----------------------------------------------------------------------*/
    ClassRootm_s class = { 0 };
    class.roomID = 101;
    class.area = 50.5;
    class.stuCount = 2;
    // 分配 students 数组
    class.students = malloc(sizeof(User_s) * class.stuCount);
    // 初始化学生数据（模拟 XDR_DECODE 动态分配的场景）
    class.students[0].id = 1;
    class.students[0].name = strdup("Alice");  // 动态字符串
    class.students[0].age = 25;
    class.students[1].id = 2;
    class.students[1].name = strdup("Bob");    // 动态字符串
    class.students[1].age = 26;

    // 2. 初始化 XDR 为 FREE 模式，释放资源
    XDR xdrsEncode;
    xdrmem_create(&xdrsEncode, buffer, sizeof(buffer), XDR_ENCODE);  // 缓冲区无关，仅需模式
    if (xdr_ClassRoom(&xdrsEncode, &class))
    {
        encoded_len = xdr_getpos(&xdrsEncode);
        DEBUG_BUFF_FORMAT(buffer, encoded_len, "编码 class 成功: --->>> ");
    }
    else
    {
        DEBUG_TIME_LINE("编码 class 失败");
    }

    xdr_destroy(&xdrsEncode);

    XDR xdrsDecode;
    xdrmem_create(&xdrsDecode, buffer, sizeof(buffer), XDR_DECODE);
    if (xdr_ClassRoom(&xdrsEncode, &class))
    {
        encoded_len = xdr_getpos(&xdrsEncode);
        DEBUG_TIME_LINE("解码 class 成功:");
        printf("roomID: %d\narea: %f\nstuCount: %d\n", class.roomID, class.area, class.stuCount);
        for (int i = 0; i < class.stuCount; i++)
        {
            printf("\tstudent[%d]:\n", i);
            printf("\t\tid=%d\n\t\tname=%s\n\t\tage=%d\n",
                    class.students[i].id, class.students[i].name, class.students[i].age);
        }
    }
    else
    {
        DEBUG_TIME_LINE("编码 class 失败");
    }

    xdr_destroy(&xdrsDecode);

    // 3. 调用 xdr_ClassRoom 一键释放所有动态资源
    XDR xdrsfree;
    xdrmem_create(&xdrsfree, buffer, sizeof(buffer), XDR_FREE);  // 缓冲区无关，仅需模式
    if (xdr_ClassRoom(&xdrsfree, &class))
    {
        printf("班级资源释放成功\n");
        // 验证：students 数组已置空，内部 name 也已释放
        printf("释放后 students = %p\n", (void*) class.students);  // 输出 NULL
    }

    // 5. 清理 XDR 上下文
    xdr_destroy(&xdrsfree);
    /*-----------------------------------------------------------------------*/

    return 0;
}
