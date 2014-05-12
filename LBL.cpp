#include <vector>
#include <string>
#include <fstream>
#include "LBL.h"

// EXT_DECLARE_GLOBALS must be used to instantiate
// the framework's assumed globals.
EXT_DECLARE_GLOBALS();

EXT_COMMAND(test, "test function for all functions",""){

	// test the new output callback functions
	std::string sampleCommand(".echo executing echo command");
	m_LBLCaptureOutputA.Execute(sampleCommand.c_str());
	std::string outStr(m_LBLCaptureOutputA.GetTextNonNull());
	m_Control->Output(DEBUG_OUTPUT_NORMAL, "This is from capture: %s\n", outStr.c_str());
	ExtCheckedPointer<ExtExtension> currentEng("current extension Engine");
	currentEng.Set(this);
	m_LBLCaptureOutputA.SwitchExtEng(currentEng);
	std::string newCommand(".echo switched the engine");
	m_LBLCaptureOutputA.Execute(newCommand.c_str());
	std::string newStr(m_LBLCaptureOutputA.GetTextNonNull());
	m_Control->Output(DEBUG_OUTPUT_NORMAL, "This is from capture: %s\n", newStr.c_str());
	m_LBLCaptureOutputA.ResetExtEng();
	m_LBLCaptureOutputA.Execute(sampleCommand.append(" with reset engine").c_str());
	m_Control->Output(DEBUG_OUTPUT_NORMAL, "This is from capture: %s\n", m_LBLCaptureOutputA.GetTextNonNull());
	std::string outputFromOneFunc(GetOutputFromCommand(".echo getoutputfromcommand"));
	m_Control->Output(DEBUG_OUTPUT_NORMAL, "This is from capture: %s\n", outputFromOneFunc.c_str());

}

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
	return (m_DebuggeeQual == DEBUG_KERNEL_LOCAL)
		|| (m_DebuggeeQual == DEBUG_KERNEL_LOCAL)
		|| (m_DebuggeeQual == DEBUG_KERNEL_EXDI_DRIVER);
}

std::string LBL_EXT::GetOutputFromCommand(const std::string command){
	m_LBLCaptureOutputA.Execute(command.c_str());
	return m_LBLCaptureOutputA.GetTextNonNull();
}

/*    void DmlCmdLink(_In_ PCSTR Text,
                    _In_ PCSTR Cmd)
    {
        Dml("<link cmd=\"%s\">%s</link>", Cmd, Text);
    }
    void DmlCmdExec(_In_ PCSTR Text,
                    _In_ PCSTR Cmd)
    {
        Dml("<exec cmd=\"%s\">%s</exec>", Cmd, Text);
    }
	*/