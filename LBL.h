#ifndef LBL_H
#define LBL_H

// EngExtCpp class name
// This define must be provided BEFORE including engextcpp.hpp.  This is because the header
// automatically references a class named EXT_CLASS to be used for all extension activity.
//
#define EXT_CLASS LBL_EXT

#include <engextcpp.hpp>




class LBL_EXT : public ExtExtension
{
public:
	EXT_COMMAND_METHOD(test);
	EXT_COMMAND_METHOD(memsave);


protected:
	bool IsLiveTarget();
	std::string GetOutputFromCommand(const std::string command);

#pragma region implementation of IDebugCallback

	//----------------------------------------------------------------------------
	//
	// Output capture helper class.
	//
	// This class is based on engextcpp.h 
	//----------------------------------------------------------------------------


	template<typename _CharType, typename _BaseClass>
	class LBLExtCaptureOutput : public _BaseClass
	{
	public:
		
		LBLExtCaptureOutput(void) : m_ReferenceCount(1), m_ExtEng(g_Ext)
		{
			m_Started = false;
			m_Text = NULL;
			m_CharTypeSize = (ULONG) sizeof(_CharType);
			Delete();
		}
		~LBLExtCaptureOutput(void)
		{
			Delete();
		}

		// IUnknown.
		STDMETHOD(QueryInterface)(
			THIS_
			_In_ REFIID InterfaceId,
			_Out_ PVOID* Interface
			)
		{
			*Interface = NULL;

			if (IsEqualIID(InterfaceId, __uuidof(IUnknown)) ||
				IsEqualIID(InterfaceId, __uuidof(_BaseClass)))
			{
				*Interface = (_BaseClass *)this;
				AddRef();
				return S_OK;
			}
			else
			{
				return E_NOINTERFACE;
			}
		}
		STDMETHOD_(ULONG, AddRef)(
			THIS
			)
		{

			return InterlockedIncrement(&m_ReferenceCount);
		}
		STDMETHOD_(ULONG, Release)(
			THIS
			)
		{
			ULONG count = InterlockedDecrement(&m_ReferenceCount);

			if (count == 0)
			{
				delete this;
			}
			return count;
		}

		VOID SwitchExtEng(ExtCheckedPointer<ExtExtension>& extEng){
			m_ExtEng = extEng;
		}

		VOID ResetExtEng(){
			m_ExtEng = g_Ext;
		}

		// IDebugOutputCallbacks*.
		STDMETHOD(Output)(
			THIS_
			_In_ ULONG Mask,
			_In_ const _CharType* Text
			)
		{
			ULONG Chars;
			ULONG CharTypeSize = (ULONG) sizeof(_CharType);

			UNREFERENCED_PARAMETER(Mask);

			if (CharTypeSize == sizeof(char))
			{
				Chars = (ULONG)strlen((PSTR)Text) + 1;
			}
			else
			{
				Chars = (ULONG)wcslen((PWSTR)Text) + 1;
			}
			if (Chars < 2)
			{
				return S_OK;
			}

			if (0xffffffff / CharTypeSize - m_UsedChars < Chars)
			{
				return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
			}

			if (m_UsedChars + Chars > m_AllocChars)
			{
				ULONG NewBytes;

				// Overallocate when growing to prevent
				// continuous allocation.
				if (0xffffffff / CharTypeSize - m_UsedChars - Chars > 256)
				{
					NewBytes = (m_UsedChars + Chars + 256) * CharTypeSize;
				}
				else
				{
					NewBytes = (m_UsedChars + Chars) * CharTypeSize;
				}
				PVOID NewMem = realloc(m_Text, NewBytes);
				if (!NewMem)
				{
					return E_OUTOFMEMORY;
				}

				m_Text = (_CharType*)NewMem;
				m_AllocChars = NewBytes / CharTypeSize;
			}

			memcpy(m_Text + m_UsedChars, Text,
				Chars * CharTypeSize);
			// Advance up to but not past the terminator
			// so that it gets overwritten by the next text.
			m_UsedChars += Chars - 1;
			return S_OK;
		}

		void Start(void)
		{
			HRESULT Status;

			if (m_CharTypeSize == sizeof(char))
			{
				if ((Status = m_ExtEng->m_Client->
					GetOutputCallbacks((IDebugOutputCallbacks**)
					&m_OldOutCb)) != S_OK)
				{
					m_ExtEng->ThrowStatus(Status,
						"Unable to get previous output callback");
				}
				if ((Status = m_ExtEng->m_Client->
					SetOutputCallbacks((IDebugOutputCallbacks*)
					this)) != S_OK)
				{
					m_ExtEng->ThrowStatus(Status,
						"Unable to set capture output callback");
				}
			}
			else
			{
				if ((Status = m_ExtEng->m_Client5->
					GetOutputCallbacksWide((IDebugOutputCallbacksWide**)
					&m_OldOutCb)) != S_OK)
				{
					m_ExtEng->ThrowStatus(Status,
						"Unable to get previous output callback");
				}
				if ((Status = m_ExtEng->m_Client5->
					SetOutputCallbacksWide((IDebugOutputCallbacksWide*)
					this)) != S_OK)
				{
					m_ExtEng->ThrowStatus(Status,
						"Unable to set capture output callback");
				}
			}

			m_UsedChars = 0;
			m_Started = true;
		}

		void Stop(void)
		{
			HRESULT Status;

			m_Started = false;

			if (m_CharTypeSize == sizeof(char))
			{
				if ((Status = m_ExtEng->m_Client->
					SetOutputCallbacks((IDebugOutputCallbacks*)
					m_OldOutCb)) != S_OK)
				{
					m_ExtEng->ThrowStatus(Status,
						"Unable to restore output callback");
				}
			}
			else
			{
				if ((Status = m_ExtEng->m_Client5->
					SetOutputCallbacksWide((IDebugOutputCallbacksWide*)
					m_OldOutCb)) != S_OK)
				{
					m_ExtEng->ThrowStatus(Status,
						"Unable to restore output callback");
				}
			}

			m_OldOutCb = NULL;
		}

		void Delete(void)
		{
			if (m_Started)
			{
				Stop();
			}

			free(m_Text);
			m_Text = NULL;
			m_AllocChars = 0;
			m_UsedChars = 0;
		}

		void Execute(_In_ PCSTR Command)
		{
			Start();

			// Hide all output from the execution
			// and don't save the command.
			m_ExtEng->m_Control->Execute(DEBUG_OUTCTL_THIS_CLIENT |
				DEBUG_OUTCTL_OVERRIDE_MASK |
				DEBUG_OUTCTL_NOT_LOGGED,
				Command,
				DEBUG_EXECUTE_NOT_LOGGED |
				DEBUG_EXECUTE_NO_REPEAT);

			Stop();
		}

		const _CharType* GetTextNonNull(void)
		{
			if (m_CharTypeSize == sizeof(char))
			{
				return (_CharType*)(m_Text ? (PCSTR)m_Text : "");
			}
			else
			{
				return (_CharType*)(m_Text ? (PCWSTR)m_Text : L"");
			}
		}

		bool m_Started;
		ULONG m_AllocChars;
		ULONG m_UsedChars;
		ULONG m_CharTypeSize;
		ULONG m_ReferenceCount;
		_CharType* m_Text;

		_BaseClass* m_OldOutCb;
		ExtCheckedPointer<ExtExtension> &m_ExtEng;
	};

#pragma endregion
	typedef LBLExtCaptureOutput<char, IDebugOutputCallbacks> LBLExtCaptureOutputA;
	typedef LBLExtCaptureOutput<WCHAR, IDebugOutputCallbacksWide> LBLExtCaptureOutputW;
private:
	LBLExtCaptureOutputA m_LBLCaptureOutputA;
};

#endif  // LBL_H