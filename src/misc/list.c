#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

Menu g_menuArray[]={//必须是一级菜单，然后二级菜单。。。。
	//一级菜单
	//level,    name,   		fun, 				ispasswd			pthis,
{{level0,"     ",		NULL, 				MENU_NOPASSWD},		NULL},
	{{level1,"测量点数据显示", 	NULL, 				MENU_NOPASSWD},		NULL},
		//二级菜单 测量点数据显示子菜单
		{{level2,"1.实时数据",	NULL, 	MENU_NOPASSWD},		NULL},
		{{level2,"2.日 数 据", 	NULL, 	MENU_NOPASSWD},		NULL},//0
		{{level2,"3.交采数据",	NULL, 	MENU_NOPASSWD},		NULL},
		{{level2,"4.月 数 据", 	NULL,   MENU_NOPASSWD},		NULL},//0
	{{level1,"参数设置与查看", 	NULL, 				MENU_NOPASSWD},		NULL},
		//二级菜单 参数设置与查看子菜单
		{{level2,"1.通信通道设置", 	NULL, 				MENU_ISPASSWD_EDITMODE},	NULL},
			////三级菜单 通信通道设置子菜单
			{{level3,"1.通信方式设置", 	NULL, 		MENU_NOPASSWD},		NULL},
				{{level4,"1.以太网通信方式",   NULL,        MENU_NOPASSWD},     NULL},//1111
				{{level4,"2.无线通信方式",     NULL,       MENU_NOPASSWD },    NULL},//1111
			{{level3,"2.电信卡专网参数", 	NULL, 	MENU_NOPASSWD},		NULL},//111
			{{level3,"3.主站通信参数", 	NULL, 		MENU_NOPASSWD},		NULL},//1
				{{level4,"1.以太网通信参数",	NULL,		MENU_NOPASSWD}},//1111
				{{level4,"2.无线通信参数",		NULL,	MENU_NOPASSWD}},//1111
				{{level4,"3.冀北地市选择",		NULL,	MENU_NOPASSWD}},//1111
			{{level3,"4.本地以太网配置", 	NULL, 		MENU_NOPASSWD},		NULL},//11
		{{level2,"2.电表参数设置", 	NULL, 	MENU_ISPASSWD_EDITMODE},	NULL},
			{{level3,"1.修改测量点", 		NULL,	MENU_ISPASSWD},	NULL},//11
			{{level3,"2.添加测量点", 		NULL,	MENU_ISPASSWD},	NULL},//11
			{{level3,"3.删除测量点", 		NULL,	MENU_ISPASSWD},	NULL},//11
		{{level2,"3.集中器时间设置",	NULL, 		MENU_NOPASSWD},		NULL},//11
		{{level2,"4.界面密码设置",		NULL, 	MENU_NOPASSWD},		NULL},//11
		{{level2,"5.集中器地址设置", 		NULL, 				MENU_ISPASSWD_EDITMODE},		NULL},//111
	{{level1,"终端管理与维护", 	NULL, 				MENU_NOPASSWD},		NULL},
		//二级菜单 终端管理与维护子菜单
		{{level2,"1.终端版本", 	NULL, 	MENU_NOPASSWD},		NULL},//11
		{{level2,"2.终端数据", 	NULL, 				MENU_NOPASSWD},		NULL},
			{{level3,"1.遥信状态", 	NULL, 		MENU_NOPASSWD},		NULL},
			{{level3,"2.时钟电池", 	NULL, 		MENU_NOPASSWD},		NULL},//0
//			////三级菜单 集中器数据子菜单
		{{level2,"3.终端管理", 	NULL, 				MENU_NOPASSWD},		NULL},
			////三级菜单 终端管理子菜单
			{{level3,"1.终端重启", 	NULL, 	MENU_ISPASSWD},		NULL},//111
			{{level3,"2.数据初始化", 	NULL, 	MENU_ISPASSWD},		NULL},
			{{level3,"3.事件初始化", 	NULL, 	MENU_ISPASSWD},		NULL},
			{{level3,"4.需量初始化", 	NULL, 	MENU_ISPASSWD},		NULL},
			{{level3,"5.恢复出厂设置",NULL, 	MENU_ISPASSWD},		NULL},
			{{level3,"6.密钥版本",NULL, 	MENU_ISPASSWD},		NULL},
		{{level2,"4.现场调试", 	NULL, 				MENU_NOPASSWD},		NULL},
		////三级菜单 现场调试子菜单
			{{level3,"1.本地IP设置",	NULL, 		MENU_NOPASSWD},		NULL},//111
			{{level3,"2.GPRSIP查看",	NULL, 		MENU_NOPASSWD},		NULL},//111
			{{level3,"3.液晶对比度", 	NULL, 	MENU_NOPASSWD},		NULL},
			{{level3,"4.交采芯片信息",NULL,		MENU_NOPASSWD},		NULL},
			{{level3,"5.采集任务监控",NULL,		MENU_NOPASSWD},NULL},
			{{level3,"6.系统电池电压",NULL,		MENU_NOPASSWD},NULL},
			{{level3,"7.载波抄表模式",NULL,		MENU_NOPASSWD},NULL},
			{{level3,"8.采集任务查看",NULL,		MENU_NOPASSWD},NULL},
		{{level2,"5.页面设置", 	NULL, 				MENU_NOPASSWD},		NULL},
		{{level2,"6.手动抄表", 	NULL, 				MENU_NOPASSWD},		NULL},
			{{level3,"1.根据表序号抄表", NULL, 	MENU_NOPASSWD},	NULL},
			{{level3,"2.根据表地址抄表",NULL,MENU_NOPASSWD},	NULL},
		{{level2,"7.485II设置", 	 NULL , MENU_NOPASSWD},		NULL},
		{{level2,"8.载波管理",	NULL, 				MENU_NOPASSWD},		NULL},
//		/////三级菜单 载波抄表子菜单
			{{level3,"1.重新抄表", 	NULL, 		MENU_NOPASSWD},		NULL},
			{{level3,"2.暂停抄表",	NULL,		MENU_NOPASSWD},		NULL},
			{{level3,"3.恢复抄表",	NULL,		MENU_NOPASSWD},		NULL},
	{{level1,"   终端重启   ", 	NULL, 	MENU_ISPASSWD},		NULL},
};//测量点数据显示

void list_init(pMenuNode list)
{
	if (list != NULL)
	{
		list->idx = 0;
		list->pData = NULL;
		list->next = NULL;
		list->prev = NULL;
		list->parent = NULL;
		list->child = NULL;
	}
}

int list_is_empty(pMenuNode list)
{
	if (list != NULL)
	{
		if (list->idx == 0 &&
		    list->pData == NULL &&
			list->next == NULL &&
			list->prev == NULL &&
			list->parent == NULL &&
			list->child == NULL
		   )
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	return 1;
}

//在head和第一个节点之间加入一个节点
void list_add_head(pMenuNode head, pMenuNode add)
{
	head->next->prev = add;
	add->next = head->next;
	add->prev = head;
	head->next = add;
}

//在head的链表尾加入一个节点
void list_add_tail(pMenuNode head, pMenuNode add)
{
	while (head->next != NULL)
		head = head->next;
	head->next = add;
	add->prev = head;
}

//删除本节点
void list_del(pMenuNode entry)
{
	if (entry->next != NULL && entry->prev != NULL)
	{
		entry->prev->next = entry->next;
		entry->next->prev = entry->prev;
		list_init(entry);
	} else if (entry->next == NULL && entry->prev != NULL)
	{
		entry->prev->next = NULL;
		list_init(entry);
	}
}

//判断链表是否为空    返回： 空为1 非空为0
int list_empty(pMenuNode head)
{
	return head->next == NULL;
}

pMenuNode list_getnext(pMenuNode node)
{
	return node = node->next;
}

pMenuNode list_getprev(pMenuNode node)
{
	return node = node->prev;
}

pMenuNode list_getparent(pMenuNode node)
{
	return node = node->parent;
}
pMenuNode list_getchild(pMenuNode node)
{
	return node = node->child;
}
//获得最后一个节点
pMenuNode list_getlast(pMenuNode node)
{
	pMenuNode pos = node;
	while (pos->next != NULL)
		pos = pos->next;
	return pos;
}
//获得头节点
pMenuNode list_getfirst(pMenuNode node)
{
	pMenuNode pos = node;
	while (pos->prev != NULL)
		pos = pos->prev;
	return pos;
}

//打印链表
void list_print(pMenuNode node)
{
	pMenuNode pos = node;
	fprintf(stderr, "head=%p\n", node);
	while (pos->next != NULL)
	{
		pos = pos->next;
		fprintf(stderr, "pos=%p prev=%p next=%p\n", pos, pos->prev, pos->next);
	}

	return;
}
//--------------------------------------
//在链表head中定位到node的前第num个节点，如果到达链表头则指向第一个节点
pMenuNode list_getPrevNumNode(pMenuNode head, pMenuNode node, int num)
{
	int i = 0;
	if (node == NULL)
		return NULL;
	while (node->prev != NULL)
	{
		i++;
		if (i > num)
			break;
		node = node->prev;
		if (node == head)
		{
			node = head->next;
			break;
		}
	}

	return node;
}
//在链表中定位到node的后第num个节点，如果到达链表尾则指向最后一个节点
pMenuNode list_getNextNumNode(pMenuNode head, pMenuNode node, int num)
{
	int i = 0;
	if (node == NULL)
		return NULL;
	while (node->next != NULL)
	{
		i++;
		if (i > num)
			break;

		node = node->next;
	}
	return node;
}
//获得链表节点总数(不包括子节点，只是兄弟节点)
int list_getListNum(pMenuNode head)
{
	int count = 0;
	if (head == NULL)
		return 0;

	while (head->next != NULL)
	{
		head = head->next;
		count++;
	}
	return count;
}

int list_getListIndex(pMenuNode head, pMenuNode node)
{
	pMenuNode pos = head;
	int index = 0;
	if (node == NULL)
		return 0;
	while (pos->next != NULL)
	{
		pos = pos->next;
		index++;
		if (pos == node)
			break;
	}
	return index;
}

//判断node在不在从start起num个节点之间 在第一个节点之前返回1 在中间返回2在第二个节点之后返回3
int listbetween(pMenuNode node, pMenuNode start, int num)
{
	int node_pos = 0, start_pos = 0;
	int ret = 0;
	node_pos = list_getListIndex(list_getfirst(node), node);
	start_pos = list_getListIndex(list_getfirst(node), start);
	if (node_pos < start_pos)
		ret = BEFORE;
	else if (node_pos > start_pos + num)
		ret = AFTER;
	else
		ret = MIDDLE;
	return ret;
}

void makeEmpty(pMenuNode node)
{
	if (node != NULL)
	{
		int i = 0;
		for (i = 0; i < node->pData->level; i++)
		{
			printf("\t");
		}

		printf("free node[%d]: %p, level: %d, name: %s\n", node->idx, node,
				node->pData->level, node->pData->name);

		makeEmpty(node->child);

		pMenuNode pNode = node->next;
		list_init(node);
		free(node);

		makeEmpty(pNode);
	}
}

void printMenuTree(pMenuNode node)
{
	if (list_is_empty(node))
	{
		return;
	}

	int i = 0;
	for (i = 0; i < node->pData->level; i++)
	{
		printf("\t");
	}

	printf("[%d]level: %d, name: %s ", node->idx, node->pData->level,
			node->pData->name);
	printf("node: %p, prev: %p, next: %p, parent: %p, child: %p\n", node,
			node->prev, node->next, node->parent, node->child);

	printMenuTree(node->child);
	printMenuTree(node->next);
}

int getMenuSize()
{
	return sizeof(g_menuArray) / sizeof(g_menuArray[0]);
}

pMenuNode MakeMenuListItem(Menu *menu)
{
	pMenuNode pitem = NULL;
	if (menu == NULL)
		return NULL;

	pitem = (pMenuNode) calloc(1, sizeof(MenuNode_s));
	if (pitem != NULL)
	{
		list_init(pitem);
		pitem->pData = &menu->data;
		menu->pthis = pitem;
	}

	return pitem;
}

pMenuNode GetMenuItembyID(Menu *menu, int menu_count, int item_id)
{
	if (menu == NULL)
		return NULL;

	return menu[item_id].pthis;
}

pMenuNode menu_getparent(Menu *menu, int menu_count, int currmenu_index)
{
	int i, currmenu_level = menu[currmenu_index].data.level; //获取当前菜单级别
	pMenuNode list_parent = NULL;
	if (currmenu_index <= 0)
		return NULL;

	for (i = currmenu_index; i >= 0; i--)
	{
		//往前找，找到第一个级别比它小的就是它的父节点
		if (menu[i].data.level < currmenu_level)
		{
			list_parent = GetMenuItembyID(menu, menu_count, i);
			break;
		}
	}

	return list_parent;
}

int isCata(Menu *menu, int menu_count, int currmenu_index)
{
	int iscata = 0; //1 是目录 0 不是
	if (currmenu_index + 1 >= menu_count)
		return iscata;
	//下一个菜单项的目录级别大于当前菜单项的目录级别则判定为此菜单项有子菜单，
	//比如说当前菜单项的level是1，如果下一个菜单项的
	//level是2，那么则说明该菜单项包含子菜单项
	if (menu[currmenu_index + 1].data.level > menu[currmenu_index].data.level)
		iscata = 1;
	return iscata;
}

pMenuNode ComposeDList(Menu *pMenu, int menu_count)
{
	int i = 0, level;
	pMenuNode list_parent = NULL;
	pMenuNode pitem = NULL, pmenulist_head = NULL;

	for (level = 0; level <= level6; level++)
	{
		for (i = 0; i < menu_count; i++)
		{
			if (pMenu[i].data.level != level)
				continue;

			pitem = MakeMenuListItem(&pMenu[i]); //生成一个节点
			pitem->idx = i;

			if (pitem == NULL)
			{
				makeEmpty(pmenulist_head);
				return NULL;
			}

			list_parent = (pMenuNode) menu_getparent(pMenu, menu_count, i); //获得链表中父节点指针

			if (list_parent == NULL) //顶层菜单的头节点  只有顶层菜单的头节点没有父节点
			{
				pmenulist_head = pitem;
			}
			else
			{
				if(list_parent->child == NULL)
				{
					list_parent->child = pitem;
				}
				else
				{
					list_add_tail(list_parent->child, pitem);
				}
			}

			pitem->parent = list_parent;
		}
	}

	return pmenulist_head;
}
