//list  双向循环链表
#ifndef LIST_H_
#define LIST_H_



#define level0 	0
#define level1 	1
#define level2 	2
#define level3 	3
#define level4 	4
#define level5 	5
#define level6 	6

//按键定义-
#define OFFSET_U	0
#define OFFSET_D	1
#define OFFSET_L	2
#define OFFSET_R	3
#define OFFSET_O	4
#define OFFSET_E	5
#define OFFSET_C	6

#define NOKEY	0
#define UP		(1<<OFFSET_U)
#define DOWN	(1<<OFFSET_D)
#define LEFT	(1<<OFFSET_L)
#define RIGHT	(1<<OFFSET_R)
#define OK		(1<<OFFSET_O)
#define ESC		(1<<OFFSET_E)
#define CANCEL	(1<<OFFSET_C)

//asc码表
#define ACK		0x06
#define CAN 	0x18

#define BEFORE	1
#define MIDDLE	2
#define AFTER	3

#define YES		1
#define NO		0

#define FOCUS	1
#define NOFOCUS	2

#define FLAG_BEFORE 0
#define FLAG_AFTER 1
//液晶反显标志
#define LCD_REV 	1   //反显
#define LCD_NOREV 	0	//不反显
//菜单是否需要密码
#define MENU_ISPASSWD_EDITMODE 2  //需要密码 并且需要查看/编辑模式
#define MENU_ISPASSWD 1  //需要密码 并且需要查看/编辑模式
#define MENU_NOPASSWD 0	//不需要密码

//结构体定义------------------
typedef void (*pFUN)();
typedef struct
{
	int level; //菜单项的目录级别
	char name[50];
	pFUN fun;
	char ispasswd;
} ElemType;

struct MenuNode;
typedef struct MenuNode MenuNode_s;
typedef MenuNode_s* pMenuNode;

struct MenuNode//菜单链表
{
	int idx;
	ElemType  *pData;
	pMenuNode prev;
	pMenuNode next;
	pMenuNode parent;
    pMenuNode child;
};

typedef struct //菜单数组
{
	ElemType data;
	pMenuNode pthis;			//存放此链表节点地址
} Menu;

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 */
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

/*获取某个链表项的地址*/
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)
//  &pos->member != (head);
/*获取链表第pos个链表项的地址*/
#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     pos->member != NULL; 	\
	     pos = list_entry(pos->member.next, typeof(*pos), member))


extern Menu g_menuArray[];

//初始化链表或者节点，使此节点的所有指针指向本身
void list_init(pMenuNode list);
void list_add_head(pMenuNode head, pMenuNode add);//在head和第一个节点之间加入一个节点
void list_add_tail(pMenuNode head, pMenuNode add);	//在head的链表尾加入一个节点
void list_del(pMenuNode entry);			//删除本节点
//判断链表是否为空    返回： 空为1 非空为0
int list_empty(pMenuNode head);
pMenuNode list_getnext(pMenuNode node);
pMenuNode list_getprev(pMenuNode node);
pMenuNode list_getparent(pMenuNode node);
pMenuNode list_getchild(pMenuNode node);
pMenuNode list_getlast(pMenuNode node);			//获得最后一个节点
pMenuNode list_getfirst(pMenuNode node);			//获得头节点
void list_print(pMenuNode node);
//--------------------------------------
//在链表中定位到node的前第num个节点，如果到达链表头则指向第一个节点
pMenuNode list_getPrevNumNode(pMenuNode head, pMenuNode node, int num);
//在链表中定位到node的后第num个节点，如果到达链表尾则指向最后一个节点
pMenuNode list_getNextNumNode(pMenuNode head, pMenuNode node, int num);
int list_getListNum(pMenuNode head);			//获得链表节点总数
int list_getListIndex(pMenuNode head, pMenuNode node);	//获得节点在链表中的位置
//判断node在不在从start起num个节点之间 在第一个几点之前返回1 在中间返回2在第二个节点之后返回3
int listbetween(pMenuNode node, pMenuNode start, int num);
pMenuNode ComposeDList(Menu *pMenu, int menu_count);
int getMenuSize();
void printMenuTree(pMenuNode node);
void makeEmpty(pMenuNode node);

#endif//end of list.h
