#include <vector>
#include <string>
#include <fstream>
#include <Windows.h>
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
//	OpenLink(TEXT("www.bing.com"));

	// check debuggee status
	m_Control->OutputCurrentState(DEBUG_OUTCTL_ALL_CLIENTS, DEBUG_CURRENT_SYMBOL);// | DEBUG_CURRENT_SOURCE_LINE);

	// get kernel data structure
	PCSTR mn = "nt";
	ULONG start_index = 0, curr_index = 0;
	ULONG64 m_base = 0;
	m_Symbols->GetModuleByModuleName(mn, start_index, &curr_index, &m_base);
	m_Control->Output(DEBUG_OUTPUT_NORMAL, "index %lx and base %lx \n", curr_index, m_base);
	ULONG tid = 0, t_size = 0;
	m_Symbols->GetTypeId(m_base, "CONTEXT", &tid);
	m_Symbols->GetTypeSize(m_base, tid, &t_size);
	m_Control->Output(DEBUG_OUTPUT_NORMAL, "type id is %lx and size is %lx\n", tid, t_size);

	// get stack info
	// in windbg ?? sizeof(CONTEXT)
	const ULONG ARM_CONTEXT_SIZE = 0x1a0;
	const ULONG buf_size = 1024;
	const ULONG MAX_FRAME_COUNT = 64;
	char ctxt_buf[ARM_CONTEXT_SIZE];
	m_Advanced->GetThreadContext(ctxt_buf, ARM_CONTEXT_SIZE);
	const ULONG f_size = MAX_FRAME_COUNT * ARM_CONTEXT_SIZE;
	DEBUG_STACK_FRAME frames[MAX_FRAME_COUNT];
	char fctxt_buf[f_size];
	ULONG filled_frames = 0;
	HRESULT hr = m_Control4->GetContextStackTrace(ctxt_buf, ARM_CONTEXT_SIZE,
		frames, MAX_FRAME_COUNT, fctxt_buf, f_size, ARM_CONTEXT_SIZE, &filled_frames);
	m_Control->Output(DEBUG_OUTPUT_NORMAL, "return is %d \n", hr);
	hr = m_Control4->OutputContextStackTrace(DEBUG_OUTCTL_ALL_CLIENTS,
		frames, filled_frames, fctxt_buf, filled_frames * ARM_CONTEXT_SIZE, ARM_CONTEXT_SIZE, DEBUG_STACK_FRAME_ADDRESSES);
	m_Control->Output(DEBUG_OUTPUT_NORMAL, "return is %d\n", hr);
}

EXT_COMMAND(gotocr, "link to the cr page", "{;s;r; cr number; the right cr number}"){
	if (GetNumUnnamedArgs() == 1){
		LPCSTR crNumber = GetUnnamedArgStr(0);
		// convert https://msdn.microsoft.com/en-us/library/ms235631%28v=vs.80%29.aspx
		size_t n_size = strlen(crNumber) + 1;
		wchar_t wcrNumber[20];
		size_t converted_chars = 0;
		mbstowcs_s(&converted_chars, wcrNumber, n_size, _strdup(crNumber), _TRUNCATE);
		std::wstring prismURL(TEXT("http://prism/CR/"));
		prismURL.append(wcrNumber);
		OpenLink(prismURL.c_str());
	}
	else{
		m_Control->Output(DEBUG_OUTPUT_NORMAL, "please provide a right CR# %s\n", "e.g. 88899");
	}
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
	return (m_DebuggeeQual == DEBUG_KERNEL_CONNECTION)
		|| (m_DebuggeeQual == DEBUG_KERNEL_LOCAL)
		|| (m_DebuggeeQual == DEBUG_KERNEL_EXDI_DRIVER);
}

std::string LBL_EXT::GetOutputFromCommand(const std::string command){
	m_LBLCaptureOutputA.Execute(command.c_str());
	return m_LBLCaptureOutputA.GetTextNonNull();
}

void LBL_EXT::OpenLink(LPCWSTR url){
	ShellExecute(NULL, TEXT("open"), url,
		NULL, NULL, SW_SHOWNORMAL);
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