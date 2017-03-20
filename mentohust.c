/*
* memtohust
* 原作者：HustMoon@BYHH
* 邮箱：www.ehust@gmail.com
* 
* ruijie_helper
* 此项目是在memtohust基础上修改
* 作者:baiyongrui
* 邮箱:baiyongrui@icloud.com
*/

#include <tchar.h>
#include <Winsock2.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <mmsystem.h>
#include <Tlhelp32.h>
#include <shlwapi.h>
#include <Psapi.h>

#include <pcap.h>

#include "myconfig.h"
#include "mystate.h"
#include "myfunc.h"
#include "myini.h"

#include <signal.h>
#include <string.h>
#include <locale.h>
//#include <stdlib.h>

extern pcap_t *hPcap;
extern volatile int state;
extern u_char *fillBuf;
extern const u_char *capBuf;
extern unsigned startMode, dhcpMode, maxFail;
extern u_char destMAC[];

extern MMRESULT timer_id;
extern short needSaveNic;
extern WCHAR wNic[];

extern void getStringByKeyName(LPWSTR key, LPWSTR buf, DWORD bufSize);
extern void writeStringForKeyName(LPWSTR key, LPWSTR value);

PROCESS_INFORMATION ProcessInfo;
STARTUPINFO StartupInfo;

static LPCWSTR path = L"C:\\Program Files\\锐捷网络\\Ruijie Supplicant\\RuijieSupplicant.exe";
//static TCHAR realPath[MAX_PATH];
static DWORD pathSize = MAX_PATH;
static TCHAR realPath[MAX_PATH] = {0};

#ifndef NO_ARP
extern u_int32_t rip, gateway;
extern u_char gateMAC[];
#endif

static void exit_handle(void);	/* 退出回调 */
static void pcap_handle(u_char *user, const struct pcap_pkthdr *h, const u_char *buf);	/* pcap_loop回调 */
static void showRuijieMsg(const u_char *buf, unsigned bufLen);	/* 显示锐捷服务器提示信息 */

static DWORD findAndKill8021X();

void sig_handle(int sig);	/* 信号回调 */

int main(int argc, char **argv)
{
	atexit(exit_handle);
	setlocale(LC_ALL,"chs");

	//	PROCESS_INFORMATION ProcessInfo;
	//   STARTUPINFO StartupInfo;

	getStringByKeyName(L"RjPath",realPath,MAX_PATH);
	if (wcslen(realPath) != 0 || wcscmp(realPath,L"") != 0) {
		wprintf(L"Found %ls! Ready to Launch.\n",realPath);

		ZeroMemory(&StartupInfo,sizeof(StartupInfo));
		StartupInfo.cb = sizeof StartupInfo;
		if (0 == CreateProcess(realPath,NULL,NULL,NULL,FALSE,NULL,NULL,NULL,&StartupInfo,&ProcessInfo))
		{
			wprintf(L"Failed to open %ls!\n",path);
			findRuijieSupplicant();
		}
	}
	else {
		findRuijieSupplicant();
	}

	initConfig(argc, argv);

	if (-1 == pcap_loop(hPcap, -1, pcap_handle, NULL)) { /* 开始捕获数据包 */
		wprintf(L"!! 捕获数据包失败，请检查网络连接！\n");
	}
	exit(EXIT_FAILURE);
}

static void exit_handle(void)
{
	//if (state != ID_DISCONNECT)
	//	switchState(ID_DISCONNECT);
	//	暂不做断开处理

	if (hPcap != NULL)
		pcap_close(hPcap);
	if (fillBuf != NULL)
		free(fillBuf);
	if(timer_id > 0)
		timeKillEvent(timer_id);
}

static DWORD findAndKill8021X() 
{ 
	TCHAR *p = L"8021x.exe";

	HANDLE hSnapshot; 
	PROCESSENTRY32 lppe; 
	//创建系统快照 
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL); //#include<Tlhelp32.h>

	if (hSnapshot == NULL) 
		return 0; 

	//初始化 lppe 的大小 
	lppe.dwSize = sizeof(lppe); 

	//查找第一个进程 
	if (!Process32First(hSnapshot, &lppe))
		return 0; 
	do 
	{ 
		if(StrCmp(lppe.szExeFile, p) == 0)//#include<shlwapi.h>
		{ 
			printf("Found 8021x.exe!Ready to kill.\n");

			if(TerminateProcess(OpenProcess(PROCESS_TERMINATE,FALSE,lppe.th32ProcessID),0))
				printf("Ruijie has been slain!!!\n");
			//return lppe.th32ProcessID;
			return 1;
		}   
	} 
	while (Process32Next(hSnapshot, &lppe)); //查找下一个进程  

	return 1;
}

static DWORD findRuijieSupplicant() 
{ 
	TCHAR *p = L"8021x.exe";
	TCHAR *replaceName = L"RuijieSupplicant.exe";

	HANDLE hSnapshot; 
	PROCESSENTRY32 lppe;

	printf("Waiting RuijieSupplicant process...\n");

	//创建系统快照 
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL); //#include<Tlhelp32.h>

	if (hSnapshot == NULL) 
		return 0; 

	//初始化 lppe 的大小 
	lppe.dwSize = sizeof(lppe); 

	while (1) {

		//查找第一个进程 
		if (!Process32First(hSnapshot, &lppe))
			return 0; 
		do 
		{ 
			if(StrCmp(lppe.szExeFile, p) == 0)//#include<shlwapi.h>
			{ 
				if(QueryFullProcessImageName(OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,lppe.th32ProcessID),0,realPath,(PDWORD)&pathSize))
				{
					wprintf(L"Found %ls! Save to config.ini\n",realPath);
					wcscpy(wcsstr(realPath,p),replaceName);
					writeStringForKeyName(L"RjPath",realPath);
				}

				return 1;
			}   
		} 
		while (Process32Next(hSnapshot, &lppe)); //查找下一个进程  

		hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	}

	return 1;
}


void sig_handle(int sig)
{
	if (-1 == switchState(state))
	{
		pcap_breakloop(hPcap);
		wprintf(L"!! 发送数据包失败, 请检查网络连接！\n");
		exit(EXIT_FAILURE);
	}

	//if (sig == SIGALRM)	 /* 定时器 */
	//{
	//	if (-1 == switchState(state))
	//	{
	//		pcap_breakloop(hPcap);
	//		printf("!! 发送数据包失败, 请检查网络连接！\n");
	//		exit(EXIT_FAILURE);
	//	}
	//}
	//else	/* 退出 */
	//{
	//	pcap_breakloop(hPcap);
	//	exit(EXIT_SUCCESS);
	//}
}

static void pcap_handle(u_char *user, const struct pcap_pkthdr *h, const u_char *buf)
{
	if (buf[0x0c]==0x88 && buf[0x0d]==0x8e) {
		if (buf[0x0F]==0x00 && buf[0x12]==0x03) {	/* 认证成功 */
			printf(">> Authentication succeed!\n");
			findAndKill8021X();
			if (needSaveNic) {
				writeStringForKeyName(L"Nic",wNic);
				needSaveNic = 0;
			}

			getEchoKey(buf);
			showRuijieMsg(buf, h->caplen);

			switchState(ID_ECHO);
		}
	}

}

static void showRuijieMsg(const u_char *buf, unsigned bufLen)
{
	//WCHAR *serverMsgW;
	char *serverMsg;
	int length = buf[0x1b];
	if (length > 0) {
		for (serverMsg=(char *)(buf+0x1c); *serverMsg=='\r'||*serverMsg=='\n'; serverMsg++,length--);
		if (strlen(serverMsg) < length)
			length = strlen(serverMsg);

		//if (length>0 && (serverMsg=gbk2utf(serverMsg, length))!=NULL) {
		if (length > 0) {
			//serverMsgW = (WCHAR *)malloc((sizeof(WCHAR) * length) + 1);
			//memset(serverMsgW,'\0',sizeof(serverMsgW));
			//MultiByteToWideChar(0,0,serverMsg,strlen(serverMsg),serverMsgW,length/2);
			if (strlen(serverMsg)) {
				printf("$$ 系统提示:\t%s\n", serverMsg);
			}
			//free(serverMsgW);
		}
	}
	if ((length=0x1c+buf[0x1b]+0x69+39) < bufLen) {
		serverMsg=(char *)(buf+length);
		if (buf[length-1]-2 > bufLen-length)
			length = bufLen - length;
		else
			length = buf[length-1]-2;
		for (; *serverMsg=='\r'||*serverMsg=='\n'; serverMsg++,length--);
		if (length > 0) {
			//serverMsgW = (WCHAR *)malloc((sizeof(WCHAR) * length) + 1);
			//memset(serverMsgW,'\0',sizeof(serverMsgW));
			//MultiByteToWideChar(0,0,serverMsg,strlen(serverMsg),serverMsgW,(length/2)+1);
			if (strlen(serverMsg)) {
				printf("$$ 计费提示:\t%s\n", serverMsg);
			}
			//free(serverMsgW);
		}
	}


}

