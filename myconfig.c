/* -*- Mode: C; tab-width: 4; -*- */
/*
* Copyright (C) 2009, HustMoon Studio
*
* 文件名称：myconfig.c
* 摘	要：初始化认证参数
* 作	者：HustMoon@BYHH
* 邮	箱：www.ehust@gmail.com
*/
#include "myconfig.h"

#include "myini.h"
#include "myfunc.h"
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#include <pcap.h>

#define NIC_SIZE			100	/* 网卡名最大长度 */
#define MAX_PATH			255	/* FILENAME_MAX */
#define D_ECHOINTERVAL		30	/* 默认心跳间隔 */
#define D_RESTARTWAIT		15	/* 默认重连间隔 */
#define D_STARTMODE			1	/* 默认组播模式 */
#define D_DHCPMODE			1	/* 默认DHCP模式 */
#define D_DAEMONMODE		0	/* 默认daemon模式 */

wchar_t adapter[100];
char nic[NIC_SIZE] = {'\0'};	/* 网卡名 */
WCHAR wNic[(NIC_SIZE/2)+1] = {L'\0'};
char dataFile[MAX_PATH] = "";	/* 数据文件 */
char dhcpScript[MAX_PATH] = "";	/* DHCP脚本 */
u_int32_t ip = 0;	/* 本机IP */
u_int32_t mask = 0;	/* 子网掩码 */
u_int32_t gateway = 0;	/* 网关 */
u_char localMAC[6];	/* 本机MAC */
u_char destMAC[6];	/* 服务器MAC */
unsigned echoInterval = D_ECHOINTERVAL;	/* 心跳间隔 */
unsigned restartWait = D_RESTARTWAIT;	/* 失败等待 */
unsigned startMode = D_STARTMODE;	/* 组播模式 */
unsigned dhcpMode = D_DHCPMODE;	/* DHCP模式 */

short needSaveNic = 0;

pcap_t *hPcap = NULL;	/* Pcap句柄 */

static int readFile(int *daemonMode);	/* 读取配置文件来初始化 */
static int getAdapter();	/* 查找网卡名 */
static void printConfig();	/* 显示初始化后的认证参数 */
static int openPcap();	/* 初始化pcap、设置过滤器 */

extern void getStringByKeyName(LPWSTR key, LPWSTR buf, DWORD bufSize);
extern void writeStringForKeyName(LPWSTR key, LPWSTR value);

void initConfig(int argc, char **argv)
{
	wchar_t nicName[NIC_SIZE] = {L'\0'};
    
    printf("Ruijie heartbeat from mentohust.\n\n");

	getStringByKeyName(L"Nic",nicName,NIC_SIZE);
	WideCharToMultiByte(0,0,nicName,wcslen(nicName),nic,NIC_SIZE,NULL,FALSE);

	if (strlen(nic) == 0 || strcmp(nic,"") == 0 ) {
		needSaveNic = 1;
		if (getAdapter() == -1) {	/* 找不到（第一块）网卡？ */
            printf("Error:No adapters were found!\n");
            exit(EXIT_FAILURE);
        }
	}
    
    printConfig();

    if (fillHeader()==-1 || openPcap()==-1) {	/* 获取IP、MAC，打开网卡 */
        printf("Error:Adapter open fail!\n");
        exit(EXIT_FAILURE);
    }
}

static int readFile(int *daemonMode)
{
	/*wchar_t tmp[16], *buf;
	if (loadFileW(&buf, CFG_FILE) < 0)
		return -1;*/

	//getStringW(buf, L"MentoHUST", L"Nic", L"", adapter, sizeof(adapter));

	//getString(buf, "MentoHUST", "IP", "255.255.255.255", tmp, sizeof(tmp));
	//ip = inet_addr(tmp);
	//getString(buf, "MentoHUST", "Mask", "255.255.255.255", tmp, sizeof(tmp));
	//mask = inet_addr(tmp);
	//getString(buf, "MentoHUST", "Gateway", "0.0.0.0", tmp, sizeof(tmp));
	//gateway = inet_addr(tmp);
	//getString(buf, "MentoHUST", "DNS", "0.0.0.0", tmp, sizeof(tmp));

	////_timeout = getInt(buf, "MentoHUST", "_timeout", D__timeout) % 100;
	//echoInterval = getInt(buf, "MentoHUST", "EchoInterval", D_ECHOINTERVAL) % 1000;
	//restartWait = getInt(buf, "MentoHUST", "RestartWait", D_RESTARTWAIT) % 100;
	//startMode = getInt(buf, "MentoHUST", "StartMode", D_STARTMODE) % 3;
	//dhcpMode = getInt(buf, "MentoHUST", "DhcpMode", D_DHCPMODE) % 4;

	//*daemonMode = getInt(buf, "MentoHUST", "DaemonMode", D_DAEMONMODE) % 4;

	//free(buf);
/*
	wchar_t buf[50] = {L'0'};

	writeStringForName();
	getStringByName(buf,50,CFG_FILE);
*/
	return 0;
}


static int getAdapter()
{
	pcap_if_t *alldevs, *d;
	int num = 0, avail = 0, i;
	char errbuf[PCAP_ERRBUF_SIZE] = {'\0'};
	if (pcap_findalldevs(&alldevs, errbuf)==-1 || alldevs==NULL)
	{
		printf("!! Find adapters fail: %s\n", errbuf);
		return -1;
	}
	for (d=alldevs; d!=NULL; d=d->next)
	{
		num++;
		if (!(d->flags & PCAP_IF_LOOPBACK) && strcmp(d->name, "any")!=0)
		{
			printf("** Adapter[%d]:\t%s\n-- Description:\t%s\n", num, d->name,d->description);
			avail++;
			i = num;
		}
	}
	if (avail == 0)
	{
		pcap_freealldevs(alldevs);
		printf("!! No adapters were found!\n");
		return -1;
	}
	if (avail > 1)
	{
		printf("?? Select a adapter[1-%d]: ", num);
		scanf("%d", &i);
		if (i < 1)
			i = 1;
		else if (i > num)
			i = num;
	}
	printf("** Adapter[%d] Selected.\n", i);
	for (d=alldevs; i>1; d=d->next, i--);
	strncpy(nic, d->name, strlen(d->name)+1);

	MultiByteToWideChar(0,0,nic,strlen(nic),wNic,NIC_SIZE/2);
	//writeStringForKeyName(L"Nic",wNic);	//不在这里写入配置文件,等接收到验证成功的数据包才写入

	pcap_freealldevs(alldevs);
	return 0;
}

static void printConfig()
{
	//char *addr[] = {"standard", "ruijie"};
	//char *dhcp[] = {"不使用", "二次认证", "认证后", "认证前"};

	printf("** Adapter: \t%s\n", nic);
	if (gateway)
		printf("** Gateway address:\t%s\n", formatIP(gateway));
	
	printf("** Heartbeat echo interval:\t%uS\n", echoInterval);
	printf("** Retry wait:\t%uS\n", restartWait);

	//printf("** 组播地址:\t%s\n", addr[startMode]);
	//printf("** DHCP方式:\t%s\n", dhcp[dhcpMode]);
}

static int openPcap()
{
	char buf[PCAP_ERRBUF_SIZE], *fmt;
	struct bpf_program fcode;

	if ((hPcap = pcap_open_live(nic, 2048, startMode >= 3  , 1000, buf)) == NULL)
	{
		printf("!! Adapter %s open fail: %s\n", nic, buf);
		return -1;
	}
	fmt = formatHex(localMAC, 6);
#ifndef NO_ARP
	sprintf(buf, "((ether proto 0x888e and (ether dst %s or ether dst 01:80:c2:00:00:03)) "
			"or ether proto 0x0806) and not ether src %s", fmt, fmt);
#else
	sprintf(buf, "ether proto 0x888e and (ether dst %s or ether dst 01:80:c2:00:00:03) "
			"and not ether src %s", fmt, fmt);
#endif
	if (pcap_compile(hPcap, &fcode, buf, 0, 0xffffffff) == -1
			|| pcap_setfilter(hPcap, &fcode) == -1)
	{
		printf("!! Can not set pcap filter: %s\n", pcap_geterr(hPcap));
		return -1;
	}
	pcap_freecode(&fcode);
	return 0;
}