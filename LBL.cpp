#include <vector>
#include <fstream>
#include "LBL.h"

// EXT_DECLARE_GLOBALS must be used to instantiate
// the framework's assumed globals.
EXT_DECLARE_GLOBALS();


EXT_COMMAND(memsave, "save memory range to file", "{;ed;r; start address; start address}"
	"{;ed;r; bytesRead; bytes read from the target's memory}"
	"{;s;r; file name; where memory content saved}"
	"{p;b; ;physical mem flag; read physical memory}"){
	// catch exception

	// check arg
	if (GetNumUnnamedArgs() == 3){
		const ULONG64 startAddr = GetUnnamedArgU64(0);
		const ULONG64 bytes2Read = GetUnnamedArgU64(1);
		PCSTR fileName = GetUnnamedArgStr(2);
		std::vector<char> bufVec((ULONG)bytes2Read);
		ULONG bytesRead=0, bytesLeft = (ULONG)bytes2Read, bufCursor=0, retryTime=0, readFlag;
		if (IsLiveTarget()){
			readFlag = DEBUG_PHYSICAL_UNCACHED;
		}
		else{
			readFlag = DEBUG_PHYSICAL_DEFAULT;
		}
		const ULONG retryLimit = 100;
		HRESULT status;
		while (bytesLeft && retryLimit >= retryTime){
			if (HasArg("p")){
				status = m_Data4->ReadPhysical2(startAddr, readFlag, &bufVec[bufCursor], bytesLeft, &bytesRead);
			}
			else{
				status = m_Data->ReadVirtualUncached(startAddr, &bufVec[bufCursor], bytesLeft, &bytesRead);
			}
			if (FAILED(status)){
				retryTime++;
				continue;
			}
			bytesLeft -= bytesRead;
			bufCursor += bytesRead;
		}		
		std::ofstream memFile(fileName, std::ofstream::binary | std::ofstream::out);
		memFile.write(&bufVec[0], bytes2Read);
		memFile.close();
	}
	
}

bool LBL_EXT::IsLiveTarget(){
	ULONG classType, qualifier;
	m_Control->GetDebuggeeType(&classType, &qualifier);
	if ((qualifier == DEBUG_KERNEL_CONNECTION) 
		|| (qualifier == DEBUG_KERNEL_LOCAL) 
		|| (qualifier == DEBUG_KERNEL_EXDI_DRIVER)){
		return true;
	}
	return false;
}