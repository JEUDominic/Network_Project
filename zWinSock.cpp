#pragma comment(linker, "/subsystem:Windows")
#pragma comment(lib, "wsock32.lib")

#include <stdio.h>
#include <WinSock2.h>
#include "resource.h"
#include "zWinSock.h"

static HINSTANCE s_hInstance = NULL;
static HWND s_hwnd = NULL;
static HFONT s_hFont = CreateFont (14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "宋体");
const int LENGTH_MAX = 0x7FFFFFFF;
const int PLAYER_MAX = 13;
static SOCKET s_player[PLAYER_MAX + 1] = { 0 };
static char s_nickname[PLAYER_MAX][14] = { 0 };
const int RENDER_WIDTH  = 800;
const int RENDER_HEIGHT = 700;

#ifdef NDEBUG
#define _log_print	((void)0)
#else
const bool ALWAYS_LOG = true; // 调试状态记录: false = 使用缓冲, true = 不使用缓冲
// (offset >= -13 && offset <= 13)时, 写调试状态记录, offset 控制空格缩进量
// 否则,若 offset 是某个窗口的句柄, 该函数充当显示调试状态记录的窗口的窗口过程函数
LRESULT CALLBACK _log_print(int offset, const char *format, ...)
{
	const int ID_EDIT = 13;

	if (offset < -13 || offset > 13)
	{
		HWND hwnd = (HWND) offset;
		if (::IsWindow (hwnd))
		{
			va_list arglist;
			va_start(arglist, offset);
			UINT   uMsg   = va_arg (arglist, UINT);
			WPARAM wParam = va_arg (arglist, WPARAM);
			LPARAM lParam = va_arg (arglist, LPARAM);
			va_end(arglist);

			switch (uMsg)
			{
			case WM_GETMINMAXINFO:
				if (LPMINMAXINFO lpmmi = (LPMINMAXINFO) lParam)
				{
					lpmmi->ptMinTrackSize.y = 233;
				}
				return 0;
			case WM_SIZE:
				if(wParam != SIZE_MINIMIZED)
				{
					::MoveWindow (::GetDlgItem (hwnd, ID_EDIT), 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
				}
				return 0;
			}
			
			return ::DefWindowProc (hwnd, uMsg, wParam, lParam);
		}

		return 0;
	}

	const int LOG_BUFFER = 8192;
	static char s_text[LOG_BUFFER];
	static char s_buffer[LOG_BUFFER * 2];
	static int s_tabs = 0;
	static int s_pos = 0;

	if (offset < 0)
	{
		s_tabs += offset;
	}

	if (format != NULL)
	{
		va_list arglist;
		va_start(arglist, format);
		int len = ::wvsprintf (s_text + s_tabs, format, arglist);
		va_end(arglist);
		::strcpy (s_text + s_tabs + len, " \r\n");
		

		::strcpy (s_buffer + s_pos, s_text);
		s_pos += ::strlen (s_text);
	}

	if (((ALWAYS_LOG || format == NULL) && s_pos != 0) || s_pos > LOG_BUFFER)
	{
		static HWND s_hWndLog = NULL;
		if (!::IsWindow (s_hWndLog))
		{
			LPCTSTR szClassName = "Sugar13.Log";
			static WORD s_flag = 0;
			if (!s_flag)
			{
				WNDCLASS wc = { sizeof (wc) };
				wc.lpfnWndProc = (WNDPROC) _log_print;
				wc.hInstance = s_hInstance;
				wc.hCursor = ::LoadCursor (NULL, IDC_ARROW);
				wc.lpszClassName = szClassName;

				s_flag = ::RegisterClass (&wc);
			}

			HWND hwnd = ::CreateWindow (
				szClassName, "zWinSock log", WS_OVERLAPPEDWINDOW,
				0, 0, 987, 610, HWND_DESKTOP, NULL, s_hInstance, 0);

			s_hWndLog = CreateWindow (
				"Edit", NULL, WS_CHILDWINDOW | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY,
				0, 0, 0, 0, hwnd, (HMENU) ID_EDIT, s_hInstance, NULL);

			::SendMessage (s_hWndLog, WM_SETFONT, (WPARAM) s_hFont, 0);
			::SendMessage (s_hWndLog, EM_LIMITTEXT, 0, 0);

			if (HMODULE hUser32 = ::LoadLibrary ("user32.dll"))
			{
				typedef BOOL (WINAPI *lpfnSLWA)(HWND, COLORREF, BYTE, DWORD);

				if (lpfnSLWA SetLayeredWindowAttributes = (lpfnSLWA) ::GetProcAddress (hUser32, "SetLayeredWindowAttributes"))
				{
					const int WS_EX_LAYERED2 = 0x00080000;
					const int LWA_ALPHA2 = 2;
					::SetWindowLong (hwnd, GWL_EXSTYLE, ::GetWindowLong (hwnd, GWL_EXSTYLE) | WS_EX_LAYERED2);
					SetLayeredWindowAttributes (hwnd, 0, 233, LWA_ALPHA2);
				}

				::FreeLibrary (hUser32);
			}

			::ShowWindow (hwnd, SW_SHOW);
			::BringWindowToTop (s_hwnd);
		}
		
		::wsprintf (s_buffer + s_pos, "------------- Tick = %u ------------- \r\n\r\n", ::GetTickCount ());
		::SendMessage (s_hWndLog, EM_SETSEL, 0x7FFFFFFF, 0x7FFFFFFF);
		::SendMessage (s_hWndLog, EM_REPLACESEL, FALSE, (LPARAM) (LPCTSTR) s_buffer);
		s_pos = 0;
	}

	if (offset > 0)
	{
		::memset (s_text + s_tabs, ' ', offset);
		s_tabs += offset;
	}

	return 0;
}
#endif

//////////////////////////////////////////////////////////////////////////

static char s_data[128] =
{
	0x53, 0x75, 0x67, 0x61, 0x72, 0x31, 0x33, 0x00, 0x05, 0x08, 0x00, 0x13, 0x21, 0x34, 0x55, 0x00,
	0x53, 0x75, 0x67, 0x61, 0x72, 0x20, 0x50, 0x6C, 0x61, 0x79, 0x65, 0x72, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

char *const LAST_NAME = s_data + 16;
char *const LAST_ADDR = s_data + 32;

void InitLast()
{
	const char *ARGV_CHAR = "-13T";

	if (__argc == 1)
	{
		char temp[MAX_PATH];
		GetTempPath (MAX_PATH, temp);
		strcat (temp, "zWinSock.13T");
		CopyFile (__argv[0], temp, FALSE);

		TCHAR szCmdLine[1024];
		HANDLE hProcess = OpenProcess (PROCESS_ALL_ACCESS, TRUE, GetCurrentProcessId ());

		wsprintf(szCmdLine, "\"%s\" %s \"%s\"", temp, ARGV_CHAR, __argv[0]);
		STARTUPINFO si = { sizeof (si) };
		PROCESS_INFORMATION pi;
		CreateProcess (NULL, szCmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
		exit (0);
	}
	
	if (__argc != 3)
	{
		exit (0);
	}
	
	if (strcmp (ARGV_CHAR, __argv[1]) != 0)
	{
		exit (0);
	}
}

DWORD FindOffset(const char *data, DWORD size)
{
	size -= sizeof (s_data);
	for (DWORD i = 0; i < size; i++)
	{
		if (memcmp (data + i, s_data, 16) == 0)
		{
			return i;
		}
	}

	return 0;
}

void ReadLast(HWND hwnd)
{
	HANDLE hFile = CreateFile (__argv[2], GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD size = GetFileSize (hFile, NULL);
		if (size != 0 && size != 0xFFFFFFFF)
		{
			char *data = new char [size];
			if (data != NULL)
			{
				DWORD dwRead;
				if (ReadFile (hFile, data, size, &dwRead, NULL))
				{
					if (dwRead == size)
					{
						if (DWORD offset = FindOffset (data, size))
						{
							memcpy (s_data, data + offset, sizeof (s_data));
							SetDlgItemText (hwnd, ID_NAME, LAST_NAME);
							SetDlgItemText (hwnd, ID_ADDR, LAST_ADDR);
						}
					}
				}

				delete data;
			}
		}

		CloseHandle (hFile);
	}
}

void WriteLast(HWND hwnd)
{
	HANDLE hFile = CreateFile (__argv[2], GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD size = GetFileSize (hFile, NULL);
		if (size != 0 && size != 0xFFFFFFFF)
		{
			char *data = new char [size];
			if (data != NULL)
			{
				DWORD dwRead;
				if (ReadFile (hFile, data, size, &dwRead, NULL))
				{
					if (dwRead == size)
					{
						if (DWORD offset = FindOffset (data, size))
						{
							GetDlgItemText (hwnd, ID_NAME, LAST_NAME, 14);
							GetDlgItemText (hwnd, ID_ADDR, LAST_ADDR, 22);

							SetFilePointer (hFile, offset, NULL, FILE_BEGIN);
							DWORD dwWritten;
							WriteFile (hFile, s_data, sizeof (s_data), &dwWritten, NULL);
						}
					}
				}
				
				delete data;
			}
		}
		
		CloseHandle (hFile);
	}
}

//////////////////////////////////////////////////////////////////////////

void AddInfo(char *format, ...)
{
	char szText[8192];
	
	SYSTEMTIME SystemTime;
	GetLocalTime (&SystemTime);
	wsprintf (szText,
		"----- %02u:%02u:%02u.%03u \r\n"
		, SystemTime.wHour
		, SystemTime.wMinute
		, SystemTime.wSecond
		, SystemTime.wMilliseconds
		);
	
	size_t len = strlen (szText);
	
	va_list ap;
	va_start  (ap, format);
	wvsprintf (szText + len, format, ap);
	va_end    (ap);
	
	strcat (szText, " \r\n");

	EnableWindow (GetDlgItem (s_hwnd, ID_INFO), FALSE);
	SendDlgItemMessage (s_hwnd, ID_INFO, EM_SETSEL, LENGTH_MAX, LENGTH_MAX);
	SendDlgItemMessage (s_hwnd, ID_INFO, EM_REPLACESEL, FALSE, (LPARAM)(LPCTSTR) szText);
	EnableWindow (GetDlgItem (s_hwnd, ID_INFO), TRUE);
}


const DWORD ID_TIMER_LOG = 13;

BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	_log_print (0, "Call Game::OnCreate");
	
	::SendDlgItemMessage (hwnd, ID_NAME, CB_LIMITTEXT,    13, 0); // 设定昵称的最大长度
	::SendDlgItemMessage (hwnd, ID_ADDR, EM_SETLIMITTEXT, 21, 0); // 设定地址的最大长度
	::SendDlgItemMessage (hwnd, ID_CHAT, EM_SETLIMITTEXT, 89, 0); // 设定聊天文字的最大长度
	::SendDlgItemMessage (hwnd, ID_INFO, EM_SETLIMITTEXT,  0, 0); // 设定信息窗口的最大长度
	//ReadLast (hwnd);

	EnableWindow (GetDlgItem (hwnd, ID_CLOSE), FALSE);
	SetTimer (hwnd, ID_TIMER_LOG, 64, NULL);
	SetFocus (GetDlgItem (hwnd, ID_SEND));

	char nickname[14] = "";
	long len = 14;
	RegQueryValue (HKEY_CURRENT_USER, "Software\\Sugar13\\zWinSock\\Nickname", nickname, &len);
	if (nickname[0] == 0)
	{
		DWORD user_len = 14;
		GetUserName (nickname, &user_len);
	}
	SetDlgItemText (hwnd, ID_NAME, nickname[0] == 0 ? "Sugar" : nickname);

	return FALSE;
}

void OnTimer(HWND hwnd, UINT id)
{
	switch (id)
	{
	case ID_TIMER_LOG:
		_log_print (0, NULL);
		return;

	}
}

void OnClose(HWND hwnd)
{
	_log_print (0, "Call OnClose");

	if (::MessageBox (hwnd, "真要退出吗?", "zWinSock", MB_YESNO | MB_ICONQUESTION) == IDYES)
	{
		::DestroyWindow (hwnd);
	}
}

void OnDestroy(HWND hwnd)
{
	_log_print (0, "Call OnDestroy");

	//WriteLast (hwnd);
	::PostQuitMessage (0);
}

bool BeginServerOrClient(bool server)
{
	char nickname[14];
	char dest_addr[22] = "";
	GetDlgItemText (s_hwnd, ID_NAME, nickname, 14);
	if (nickname[0] == 0)
	{
		AddInfo ("请输入昵称！");
		return false;
	}

	SOCKET s = socket (AF_INET, SOCK_STREAM, 0); // 建立Socket
	if (s == INVALID_SOCKET)
	{
		_log_print (0, "Fail to create a socket!");
		return false;
	}
	_log_print (0, "OK to create a socket!");

	char name[128] = "";
	if (gethostname (name, sizeof(name)) == SOCKET_ERROR)
	{
		_log_print (0, "Fail to get the standard host name for the local machine!");
		return false;
	}
	_log_print (0, "OK to get the standard host name for the local machine!");

	HOSTENT *pHostent = gethostbyname(name);
	if (pHostent == NULL)
	{
		_log_print (0, "Fail to get host information!");
		return false;
	}
	_log_print (0, "OK to get host information!");

	in_addr dest;
	for (size_t i = 0; pHostent->h_addr_list[i] != NULL ;i++)
	{
		memcpy (&dest, pHostent->h_addr_list[i], pHostent->h_length);
		if (i != 0)
		{
			AddInfo ("外网地址： %s", inet_ntoa (dest));
		}
		else
		{
			sockaddr_in sa;
			sa.sin_family = AF_INET;
			sa.sin_port = htons (0);
			sa.sin_addr = dest;

			if (bind (s, (LPSOCKADDR) &sa, sizeof (sa)) == SOCKET_ERROR)
			{
				_log_print (0, "Fail to associate a local address with a socket!");
				return false;
			}
			_log_print (0, "OK to associate a local address with a socket!");

			char addr[22];
			if (server)
			{
				int namelen = sizeof (sa);
				if (getsockname (s, (LPSOCKADDR) &sa, &namelen) == SOCKET_ERROR)
				{
					_log_print (0, "Fail to get the local name for a socket!");
					closesocket (s);
					return false;
				}
				_log_print (0, "OK to get the local name for a socket!");

				wsprintf (addr, "%s:%u", inet_ntoa (dest), ntohs (sa.sin_port));
				SetDlgItemText (s_hwnd, ID_ADDR, addr);
				AddInfo ("本机地址： %s", addr);

				if (listen (s, 13) == SOCKET_ERROR)
				{
					_log_print (0, "Fail to establish a socket to listen for incoming connection!");
					closesocket (s);
					return false;
				}
				_log_print (0, "OK to establish a socket to listen for incoming connection!");

				if (WSAAsyncSelect (s, s_hwnd, WM_USER, FD_ACCEPT) == SOCKET_ERROR)
				{
					_log_print (0, "Fail to request event notification for a socket!");
					closesocket (s);
					return false;
				}
				_log_print (0, "OK to request event notification for a socket!");

				s_player[0] = s;
				strcpy (s_nickname[0], nickname);
			}
			else
			{
				GetDlgItemText (s_hwnd, ID_ADDR, dest_addr, 23);
				strcpy (addr, dest_addr);
				DWORD addr_ip;
				WORD addr_port;
				
				char *port = strchr (addr, ':');
				if (port != NULL)
				{
					*port = 0;
					addr_ip = inet_addr (addr);
					addr_port = htons (atoi (port + 1));
					_log_print (0, "addr = %s, port = %u", addr, atoi (port + 1));
				}

				if (addr_ip == INADDR_NONE || port == NULL)
				{
					AddInfo ("请输入合法的地址！ \r\n形如： \r\n aaa.bbb.ccc.ddd:eeeee");
					return false;
				}

				sockaddr_in sa;
				sa.sin_family = AF_INET;
				sa.sin_port = addr_port;
				sa.sin_addr.s_addr = addr_ip;

				if (connect (s, (LPSOCKADDR) &sa, sizeof (sa)) == SOCKET_ERROR)
				{
					_log_print (0, "Fail to establish a connection to a peer!");
					closesocket (s);
					return false;
				}

				if (WSAAsyncSelect (s, s_hwnd, WM_USER + PLAYER_MAX, FD_READ | FD_CLOSE) == SOCKET_ERROR)
				{
					_log_print (0, "Fail to request event notification for a socket.");
					closesocket (s);
					return false;
				}
				
				s_player[PLAYER_MAX] = s;
			}
		}
	}

	HKEY hKey;
	if (RegCreateKey (HKEY_CURRENT_USER, "Software\\Sugar13\\zWinSock\\Nickname", &hKey) == ERROR_SUCCESS)
	{
		RegSetValue (hKey, NULL, REG_SZ, nickname, strlen (dest_addr));
		RegSetValue (hKey, nickname, REG_SZ, dest_addr, strlen (dest_addr));
		RegCloseKey (hKey);
	}

	return true;
}

void SendInfo(DWORD index, char *format, ...)
{
	char text[128];
	
	va_list ap;
	va_start  (ap, format);
	wvsprintf (text, format, ap);
	va_end    (ap);

	send (s_player[index], text, strlen (text) + 1, 0);
}

void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
	case ID_SERVER:
		if (s_player[0] == NULL && s_player[PLAYER_MAX] == NULL)
		{
			if (BeginServerOrClient (true))
			{
				SendDlgItemMessage (hwnd, ID_ADDR, EM_SETREADONLY, TRUE, 0);
				EnableWindow (GetDlgItem (hwnd, ID_NAME  ), FALSE);
				EnableWindow (GetDlgItem (hwnd, ID_SERVER), FALSE);
				EnableWindow (GetDlgItem (hwnd, ID_CLIENT), FALSE);
				EnableWindow (GetDlgItem (hwnd, ID_CLOSE ), TRUE );
			}
			else
			{
				MessageBox (hwnd, "创建服务器失败！", "警告", MB_OK);
				return;
			}
		}
		break;

	case ID_CLIENT:
		if (s_player[0] == NULL && s_player[PLAYER_MAX] == NULL)
		{
			if (BeginServerOrClient (false))
			{
				SendDlgItemMessage (hwnd, ID_ADDR, EM_SETREADONLY, TRUE, 0);
				EnableWindow (GetDlgItem (hwnd, ID_NAME  ), FALSE);
				EnableWindow (GetDlgItem (hwnd, ID_SERVER), FALSE);
				EnableWindow (GetDlgItem (hwnd, ID_CLIENT), FALSE);
				EnableWindow (GetDlgItem (hwnd, ID_CLOSE ), TRUE );
			}
			else
			{
				MessageBox (hwnd, "连接服务器失败！", "警告", MB_OK);
				return;
			}
		}
		break;

	case ID_CLOSE:
		if (MessageBox (hwnd, "确定断开服务器吗？", "zWinSock", MB_OKCANCEL) == IDOK)
		{
			if (s_player[0] != NULL)
			{
				for (size_t i = 0; i < PLAYER_MAX; i++)
				{
					if (s_player[i] != NULL)
					{
						closesocket (s_player[i]);
						s_player[i] = NULL;
					}
				}
			}
			else if (s_player[PLAYER_MAX] != NULL)
			{
				closesocket (s_player[PLAYER_MAX]);
				s_player[PLAYER_MAX] = NULL;
			}
			AddInfo ("您断开了服务器！");
			SendDlgItemMessage (hwnd, ID_LIST, LB_RESETCONTENT, 0, 0);
			SendDlgItemMessage (hwnd, ID_ADDR, EM_SETREADONLY, FALSE, 0);
			EnableWindow (GetDlgItem (hwnd, ID_NAME  ), TRUE );
			EnableWindow (GetDlgItem (hwnd, ID_SERVER), TRUE );
			EnableWindow (GetDlgItem (hwnd, ID_CLIENT), TRUE );
			EnableWindow (GetDlgItem (hwnd, ID_CLOSE ), FALSE);
			SetDlgItemText (s_hwnd, ID_ADDR, NULL);
		}
		break;

	case ID_SEND:
		{
			char text[128];
			GetDlgItemText (s_hwnd, ID_CHAT, text, 90);
			if (text[0] == 0)
			{
				return;
			}

			if (s_player[0] != NULL && s_player[PLAYER_MAX] == NULL) // 服务器状态
			{
				_log_print (0, "Server say: %s", text);
				for (size_t i = 1; i < PLAYER_MAX; i++)
				{
					if (s_player[i] != NULL)
					{
						SendInfo (i, "i<%s>发言： \r\n %s", s_nickname[0], text);
					}
				}

				AddInfo ("<%s>发言： \r\n %s", s_nickname[0], text);
			}

			if (s_player[0] == NULL && s_player[PLAYER_MAX] != NULL) // 客户端状态
			{
				_log_print (0, "Client say: %s", text + 1);
				SendInfo (PLAYER_MAX, "c%s", text);
			}
			
			SetDlgItemText (s_hwnd, ID_CHAT, NULL);
		}
		break;

	case ID_NAME:
		switch (codeNotify)
		{
		case CBN_DROPDOWN:
			{
				char oldname[14];
				GetDlgItemText (s_hwnd, ID_NAME, oldname, 14);
				SendDlgItemMessage (s_hwnd, ID_NAME, CB_RESETCONTENT, 0, 0);
				SendDlgItemMessage (s_hwnd, ID_NAME, CB_ADDSTRING, 0, (LPARAM) oldname);

				HKEY hKey;
				if (RegCreateKey (HKEY_CURRENT_USER, "Software\\Sugar13\\zWinSock\\Nickname", &hKey) == ERROR_SUCCESS)
				{
					DWORD sub;
					RegQueryInfoKey (hKey, 0, 0, NULL, &sub, 0, 0, 0, 0, 0, 0, 0);
					for (DWORD i = 0; i < sub; i++)
					{
						char nickname[14];
						if (RegEnumKey (hKey, i, nickname, 14) == ERROR_SUCCESS)
						{
							if (strcmp (oldname, nickname) != 0)
							{
								SendDlgItemMessage (s_hwnd, ID_NAME, CB_ADDSTRING, 0, (LPARAM) nickname);
							}
						}
					}

					RegCloseKey (hKey);
				}
				
				SetDlgItemText (s_hwnd, ID_NAME, oldname);
			}
			break;

		case CBN_SELCHANGE:
			{
				int item = SendDlgItemMessage (s_hwnd, ID_NAME, CB_GETCURSEL, 0, 0);
				if (item == CB_ERR)
				{
					return;
				}

				char nickname[14];
				SendDlgItemMessage (s_hwnd, ID_NAME, CB_GETLBTEXT, item, (LPARAM) nickname);

				HKEY hKey;
				if (RegCreateKey (HKEY_CURRENT_USER, "Software\\Sugar13\\zWinSock\\Nickname", &hKey) == ERROR_SUCCESS)
				{
					char addr[22];
					long len = 22;
					if (RegQueryValue (hKey, nickname, addr, &len) == ERROR_SUCCESS)
					{
						SetDlgItemText (s_hwnd, ID_ADDR, addr);
					}
					
					RegCloseKey (hKey);
				}
			}
			break;
		}
		return;

	default:
		return;
	}

	SetFocus (GetDlgItem (hwnd, ID_CHAT));
}

void ServerAccept(SOCKET s, DWORD i) 
{
	if (i < PLAYER_MAX)
	{
		if (WSAAsyncSelect (s, s_hwnd, WM_USER + i, FD_READ | FD_CLOSE) == SOCKET_ERROR)
		{
			_log_print (0, "Fail to request event notification for a socket.");
			closesocket (s);
			return;
		}
		s_player[i] = s;
		send (s, "n", 2, 0); // 请求昵称
	}
	else
	{
		send (s, "f", 2, 0); // 拒绝连接
		closesocket (s);
	}
}

void ServerRecv(DWORD index, char buffer[], int len) 
{
	_log_print (0, "recv %s", buffer);

	switch (buffer[0])
	{
	case 'n': // 接收昵称
		{
			char *nickname = buffer + 1;
			DWORD i;
			for (i = 0; i < PLAYER_MAX; i++)
			{
				if (strcmp (nickname, s_nickname[i]) == 0)
				{
					send (s_player[index], "s", 2, 0); // 昵称已存在
					closesocket (s_player[index]);
					s_player[index] = NULL;
					return;
				}
			}

			if (strlen (nickname) > 13)
			{
				_log_print (0, "Too long nickname!");
				closesocket (s_player[index]);
				s_player[index] = NULL;
				return;
			}

			SendInfo (index, "p%s", s_nickname[0]);
			AddInfo ("<%s>来啦！", nickname);
			for (i = 1; i < PLAYER_MAX; i++)
			{
				if (s_nickname[i][0] != 0)
				{
					SendInfo (index, "p%s", s_nickname[i]);
					SendInfo (i, "p%s", nickname);
				}
			}

			strcpy (s_nickname[index], nickname);
			SendDlgItemMessage (s_hwnd, ID_LIST, LB_ADDSTRING, 0, (LPARAM) nickname);
		}
		return;

	case 'c':
		{
			for (size_t i = 1; i < PLAYER_MAX; i++)
			{
				if (s_player[i] != NULL)
				{
					SendInfo (i, "i<%s>发言： \r\n %s", s_nickname[index], buffer + 1);
				}
			}

			AddInfo ("<%s>发言： \r\n %s", s_nickname[index], buffer + 1);
		}
		return;
	}
}

void ClientRecv(char buffer[], int len) 
{
	_log_print (0, "recv %s", buffer);

	switch (buffer[0])
	{
	case 'n': // 回复昵称
		{
			char nickname[16] = "n";
			GetDlgItemText (s_hwnd, ID_NAME, nickname + 1, 14);
			send (s_player[PLAYER_MAX], nickname, strlen (nickname) + 1, 0);
		}
		return;
	case 'f': // 被拒绝连接
		AddInfo ("服务器已满！");
		return;
	case 's': // 昵称已存在
		AddInfo ("昵称已存在！");
		return;
	case 'p': // 获取其他玩家昵称
		AddInfo ("<%s>来啦！", buffer + 1);
		SendDlgItemMessage (s_hwnd, ID_LIST, LB_ADDSTRING, 0, (LPARAM) (buffer + 1));
		return;
	case 'i': // 收到消息
		AddInfo (buffer + 1);
		return;
	case 'l': // 其他玩家离开
		{
			int item = SendDlgItemMessage (s_hwnd, ID_LIST, LB_FINDSTRINGEXACT, -1, (LPARAM) (buffer + 1));
			if (item != LB_ERR)
			{
				SendDlgItemMessage (s_hwnd, ID_LIST, LB_DELETESTRING, item, 0);
			}
			AddInfo ("<%s>离开！", buffer + 1);
		}
		return;
	}
}

void OnSocket(DWORD index, SOCKET s, WORD e) 
{
	if (index == 0)
	{
		switch (e)
		{
		case FD_ACCEPT:
			{
	_log_print (0, "index = %u, socket = 0x%p, event = FD_ACCEPT", index, s);
				DWORD i;
				for (i = 1; i < PLAYER_MAX; i++)
				{
					if (s_player[i] == 0)
					{
						break;
					}
				}

				sockaddr_in sa;
				int len = sizeof (sa);
				SOCKET ans = accept (s, (LPSOCKADDR) &sa, &len);

				_log_print (0, "ans = %p, s = %p", ans, s);
				if (ans != INVALID_SOCKET)
				{
					ServerAccept (ans, i);
				}
			}
			return;
		}
	}
	else if (index < PLAYER_MAX)
	{
		switch (e)
		{
		case FD_READ:
			{
	_log_print (0, "index = %u, socket = 0x%p, event = FD_READ", index, s);
				char buffer[128];
				int len = recv (s, buffer, 128, 0);
				if (len > 0)
				{
					buffer[len] = 0;
					ServerRecv (index, buffer, len);
				}
			}
			return;
		case FD_CLOSE:
			_log_print (0, "index = %u, socket = 0x%p, event = FD_CLOSE", index, s);
			{
				s_player[index] = NULL;
				int count = SendDlgItemMessage (s_hwnd, ID_LIST, LB_GETCOUNT, 0, 0);
				for (int item = 0; item < count; item++)
				{
					char text[14];
					SendDlgItemMessage (s_hwnd, ID_LIST, LB_GETTEXT, item, (LPARAM) text);
					if (strcmp (text, s_nickname[index]) == 0)
					{
						SendDlgItemMessage (s_hwnd, ID_LIST, LB_DELETESTRING, item, 0);
						break;
					}
				}
				AddInfo ("<%s>离开！", s_nickname[index]);
				for (DWORD i = 1; i < PLAYER_MAX; i++)
				{
					if (i != index && s_nickname[i][0] != 0)
					{
						SendInfo (i, "l%s", s_nickname[index]);
					}
				}
				s_nickname[index][0] = 0;
			}
			return;
		}
	}
	else
	{
		switch (e)
		{
		case FD_READ:
			{
	_log_print (0, "index = %u, socket = 0x%p, event = FD_READ", index, s);
				char buffer[128];
				int len = recv (s, buffer, 128, 0);
				if (len > 0)
				{
					buffer[len] = 0;
					ClientRecv (buffer, len);
				}
			}
			return;
		case FD_CLOSE:
			_log_print (0, "index = %u, socket = 0x%p, event = FD_CLOSE", index, s);
			s_player[PLAYER_MAX] = NULL;
			AddInfo ("服务器已断开！");
			SendDlgItemMessage (s_hwnd, ID_LIST, LB_RESETCONTENT, 0, 0);
			SendDlgItemMessage (s_hwnd, ID_ADDR, EM_SETREADONLY, FALSE, 0);
			EnableWindow (GetDlgItem (s_hwnd, ID_NAME  ), TRUE );
			EnableWindow (GetDlgItem (s_hwnd, ID_SERVER), TRUE );
			EnableWindow (GetDlgItem (s_hwnd, ID_CLIENT), TRUE );
			EnableWindow (GetDlgItem (s_hwnd, ID_CLOSE ), FALSE);
			return;
		}
	}
}

void Rander(HDC hDC) 
{
	HFONT hFont = CreateFont (200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "宋体");
	SelectObject (hDC, hFont);
	SetTextColor (hDC, RGB (170, 51, 255));
	SetBkMode (hDC, TRANSPARENT);
	TextOut (hDC, 0, 0, "ChatChat", 6);
	DeleteObject (hFont);
}

void OnPaint(HWND hwnd)
{
	PAINTSTRUCT ps;
	::BeginPaint (hwnd, &ps);

	BITMAPINFO bmi = { sizeof (BITMAPINFOHEADER) };
	bmi.bmiHeader.biWidth = RENDER_WIDTH;
	bmi.bmiHeader.biHeight = RENDER_HEIGHT;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biSizeImage = 4 * RENDER_WIDTH * RENDER_HEIGHT;

	void * pvBits;
	HDC buffer = ::CreateCompatibleDC (ps.hdc);
	HBITMAP bitmap = ::CreateDIBSection (ps.hdc, &bmi, 0, &pvBits, NULL, 0);
	::SelectObject (buffer, bitmap);
	::BitBlt (buffer, 0, 0, RENDER_WIDTH, RENDER_HEIGHT, NULL, 0, 0, WHITENESS);

	Rander (buffer);

	::BitBlt (ps.hdc, 0, 0, RENDER_WIDTH, RENDER_HEIGHT, buffer, 0, 0, SRCCOPY);
	::DeleteDC (buffer);
	::DeleteObject (bitmap);
	::EndPaint (hwnd, &ps);
}

void OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT * lpMeasureItem)
{
	switch (lpMeasureItem->CtlID)
	{
	case ID_SERVER:
		lpMeasureItem->itemWidth  = 52;
		lpMeasureItem->itemHeight = 15;
//		lpMeasureItem->itemData;  
		return;
	}
}

void OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem)
{
	switch (lpDrawItem->CtlID)
	{
	case ID_SERVER:
		_log_print (0, "OnDrawItem, lpDrawItem->rcItem, left = %d, top = %d, right = %d, bottom = %d", lpDrawItem->rcItem);

		SetTextColor (lpDrawItem->hDC, (lpDrawItem->itemState & ODS_DISABLED) ? RGB (192, 192, 192) : RGB (255, 255, 0));
		SetBkColor (lpDrawItem->hDC, RGB (170, 0, 204));
		ExtTextOut (lpDrawItem->hDC, 8, 5, ETO_CLIPPED | ETO_OPAQUE, &lpDrawItem->rcItem, "创建服务器", 10, NULL);
		if (lpDrawItem->itemAction & ODA_SELECT)
		{
			SelectObject (lpDrawItem->hDC, GetStockObject (BLACK_PEN));
			MoveToEx (lpDrawItem->hDC, 0,                        lpDrawItem->rcItem.bottom - 2, NULL);
			LineTo   (lpDrawItem->hDC, lpDrawItem->rcItem.right, lpDrawItem->rcItem.bottom - 2);
		}
		return;
	}
}

// 主窗口的窗口过程函数
BOOL CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg >= WM_USER && uMsg <= WM_USER + PLAYER_MAX)
	{
		OnSocket (uMsg - WM_USER, wParam, WSAGETSELECTEVENT (lParam));
	}

 	switch (uMsg)
 	{
	case WM_INITDIALOG:
		return OnInitDialog (hwndDlg, (HWND) wParam, lParam);

	case WM_CLOSE:
		OnClose (hwndDlg);
		return TRUE;

	case WM_DESTROY:
		OnDestroy (hwndDlg);
		return TRUE;

	case WM_TIMER:
		OnTimer (hwndDlg, wParam);
		return TRUE;

	case WM_COMMAND:
		OnCommand (hwndDlg, LOWORD (wParam), (HWND) lParam, HIWORD (wParam));
		return TRUE;

	case WM_PAINT:
		OnPaint (hwndDlg);
		return TRUE;

	case WM_MEASUREITEM:
		OnMeasureItem (hwndDlg, (MEASUREITEMSTRUCT *) lParam);
		return TRUE;
		
	case WM_DRAWITEM:
		OnDrawItem (hwndDlg, (const DRAWITEMSTRUCT *) lParam);
		return TRUE;

// 		HANDLE_MSG (hwnd, WM_ERASEBKGND,	OnEraseBkgnd);
// 		HANDLE_MSG (hwnd, WM_GETMINMAXINFO,	OnGetMinMaxInfo);
// 		HANDLE_MSG (hwnd, WM_PAINT,			OnPaint);
// 		HANDLE_MSG (hwnd, WM_KEYDOWN,		OnKey);
// 		HANDLE_MSG (hwnd, WM_LBUTTONDOWN,	OnLButtonDown);
// 		HANDLE_MSG (hwnd, WM_MOUSEMOVE,		OnMouseMove);
// 		HANDLE_MSG (hwnd, WM_LBUTTONUP,		OnLButtonUp);
// 		HANDLE_MSG (hwnd, WM_RBUTTONDOWN,	OnRButtonDown);
 	}

	return FALSE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nCmdShow)
{
// 	InitLast();

    WSADATA wsaData;
	if (WSAStartup (MAKEWORD (2, 2), &wsaData) != 0)
    {
        AddInfo ("网络服务初始化失败！");
        return 0;
	}

	s_hInstance = hInstance;
	_log_print (1, "Begin Call WINAPI WinMain, hInstance = 0x%p, lpCmdLine = 0x%p, nCmdShow = %d",
		hInstance, lpCmdLine, nCmdShow);

	s_hwnd = CreateDialog (hInstance, "D", NULL, DialogProc);
	if (s_hwnd == NULL)
	{
		return 0;
	}
	
	AddInfo ("欢迎！");

	MSG msg;
	while (::GetMessage (&msg, NULL, 0, 0))
	{
		if (!IsDialogMessage (s_hwnd, &msg))
		{
			::TranslateMessage (&msg);
			::DispatchMessage (&msg);
		}
	}

	WSACleanup ();

	return msg.wParam;
}

