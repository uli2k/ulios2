/*	pci.c for ulios driver
	作者：孙亮
	功能：PCI总线驱动
	最后修改日期：2019-11-22
*/

#include "../lib/string.h"
#include "basesrv.h"

typedef struct _PCI_CONFIG
{
	WORD VendorID, DeviceID;
	WORD command, status;
	BYTE RevisionID, ProgIF, SubClass, ClassCode;
	BYTE CacheLineSize, LatencyTimer, HeaderType, BIST;
	DWORD BaseAddress[6];
	DWORD CardbusCISPointer;
	WORD SubVendorID, SubID;
	DWORD ExROMBaseAddress;
	WORD CapabilitiesPointer, Reserved1;
	DWORD Reserved2;
	BYTE InterruptLine, InterruptPIN, MinGrant, MaxLatency;
}PCI_CONFIG;	/*PCI配置寄存器空间*/

static inline void WritePci(DWORD addr, DWORD data)
{
	outl(0xCF8, addr);
	outl(0xCFC, data);
}

static inline DWORD ReadPci(DWORD addr)
{
	outl(0xCF8, addr);
	return inl(0xCFC);
}

// offset必须4字节对齐
static inline DWORD ReadPciConfigWord(DWORD bus, DWORD device, DWORD function, DWORD offset)
{
//	31		30-24(7b)	23-16(8b)	15-11(5b)	10-8(3b)	7-0(8b)
//	enable	reserved	bus			device		function	offset
	DWORD address;
	address = 0x80000000 | (bus << 16) | (device << 11) | (function << 8) | offset;
	return ReadPci(address);
}

WORD GetVendorID(DWORD bus, DWORD device, DWORD function)
{
	return (WORD)ReadPciConfigWord(bus, device, function, 0);
}

WORD GetDeviceID(DWORD bus, DWORD device, DWORD function)
{
	return (WORD)(ReadPciConfigWord(bus, device, function, 0) >> 16);
}

WORD GetClass(DWORD bus, DWORD device, DWORD function)
{
	return (WORD)(ReadPciConfigWord(bus, device, function, 0x8) >> 16);
}

BYTE GetHeaderType(DWORD bus, DWORD device, DWORD function)
{
	return (BYTE)(ReadPciConfigWord(bus, device, function, 0xC) >> 16);
}

DWORD GetSecondaryBus(DWORD bus, DWORD device, DWORD function)
{
	char buf[256];
	sprintf(buf, "GetSecondaryBus(%d, %d, %d)\n", bus, device, function);
	KDebug(buf);
	return 0;
}

void CheckFunction(DWORD bus, DWORD device, DWORD function)
{
	char buf[256];
	DWORD SecondaryBus;

	sprintf(buf, "PCI device: %u:%u:%u\t%x:%x\n", bus, device, function, GetVendorID(bus, device, function), GetDeviceID(bus, device, function));
	KDebug(buf);
	if (GetClass(bus, device, function) == 0x0604)
	{
		SecondaryBus = GetSecondaryBus(bus, device, function);
		CheckBus(SecondaryBus);
	}
}

void CheckDevice(DWORD bus, DWORD device)
{
	if (GetVendorID(bus, device, 0) == 0xFFFF)	// 设备不存在
		return;
	CheckFunction(bus, device, 0);
	if (GetHeaderType(bus, device, 0) & 0x80)	// 多功能设备,检查剩余功能
	{
		DWORD function;
		for (function = 1; function < 8; function++)
		{
			if (GetVendorID(bus, device, function) != 0xFFFF)
				CheckFunction(bus, device, function);
		}
	}
}

void CheckBus(DWORD bus)
{
	DWORD device;
	for (device = 0; device < 32; device++)
		CheckDevice(bus, device);
}

void ScanPci()
{
	if (GetHeaderType(0, 0, 0) & 0x80)	// 多PCI主控制器
	{
		DWORD function;
		for (function = 0; function < 8; function++)
		{
			if (GetVendorID(0, 0, function) != 0xFFFF)
				break;
			CheckBus(function);
		}
	}
	else	// 单PCI主控制器
		CheckBus(0);
}

void CheckAllBuses()
{
	DWORD bus;
	DWORD device;

	for (bus = 0; bus < 256; bus++)
		for (device = 0; device < 32; device++)
			CheckDevice(bus, device);
}

int main()
{
	long res;		/*返回结果*/

	KDebug("booting... pci driver\n");
	if ((res = KRegKnlPort(SRV_PCI_PORT)) != NO_ERROR)	/*注册服务端口号*/
		return res;
	CheckAllBuses();
	KUnregKnlPort(SRV_ATHD_PORT);
	return res;
}
