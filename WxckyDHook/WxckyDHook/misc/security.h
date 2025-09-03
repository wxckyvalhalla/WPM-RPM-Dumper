#pragma once
#include "../imports.h"
#include "../globals.h"

using _RtlCreateUserThread = NTSTATUS(NTAPI*)(
	HANDLE ProcessHandle,
	PSECURITY_DESCRIPTOR SecurityDescriptor,
	BOOLEAN CreateSuspended,
	ULONG StackZeroBits,
	PULONG StackReserved,
	PULONG StackCommit,
	void* StartAddress,
	void* StartParameter,
	PHANDLE ThreadHandle,
	void* ClientID
	);

namespace hyde {
	BOOL CreateThread(void* thread, HMODULE& hModule) {
		auto newaddr = 0x0007FFF8797D540;
		DWORD oProtect;
		VirtualProtect((void*)newaddr, 1000, PAGE_EXECUTE_READWRITE, &oProtect);
		CONTEXT cpuContext;
		HANDLE cpuHandle = nullptr;
		_RtlCreateUserThread NtThread = (_RtlCreateUserThread)(GetProcAddress(GetModuleHandleA(_XOR_("ntdll")), _XOR_("RtlCreateUserThread")));
		NtThread(GetCurrentProcess(), nullptr, TRUE, NULL, NULL, NULL, (PTHREAD_START_ROUTINE)newaddr, hModule, &cpuHandle, NULL);
		cpuContext.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL;
		GetThreadContext(cpuHandle, &cpuContext);
#ifdef _WIN64
		cpuContext.Rip = (ULONG64)thread;
#else
		cpuContext.Eip = (ULONG32)thread;
#endif
		SetThreadContext(cpuHandle, &cpuContext);
		ResumeThread(cpuHandle);
		return 0;
	}

}
