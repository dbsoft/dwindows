/* edge.cpp
 *
 * Allows dw_html_new() to embed a Microsoft Edge (Chromium) browser.
 *
 * Requires Windows 10, 8 or 7 with Microsoft Edge (Chromium) installed.
 * 
 * Only included when BUILD_EDGE is defined, will fall back to embedded IE.
 */
#include "dw.h"
#include "WebView2.h"
#include <wrl.h>
#include <string>

using namespace Microsoft::WRL;

#define _DW_HTML_DATA_NAME "_dw_edge"
#define _DW_HTML_DATA_LOCATION "_dw_edge_location"
#define _DW_HTML_DATA_RAW "_dw_edge_raw"
#define _DW_HTML_DATA_ADD "_dw_edge_add"

extern "C" {

	/* Import the character conversion functions from dw.c */
	LPWSTR _dw_UTF8toWide(const char* utf8string, void* outbuf);
	char* _dw_WideToUTF8(LPCWSTR widestring, void* outbuf);
	#define UTF8toWide(a) _dw_UTF8toWide(a, a ? _alloca(MultiByteToWideChar(CP_UTF8, 0, a, -1, NULL, 0) * sizeof(WCHAR)) : NULL)
	#define WideToUTF8(a) _dw_WideToUTF8(a, a ? _alloca(WideCharToMultiByte(CP_UTF8, 0, a, -1, NULL, 0, NULL, NULL)) : NULL)
	LRESULT CALLBACK _dw_wndproc(HWND hWnd, UINT msg, WPARAM mp1, LPARAM mp2);
	void _dw_create_junction(LPWSTR source, LPWSTR target);
	LPWSTR _dw_get_edge_stable_path(void);
}

class EdgeBrowser
{
public:
	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	BOOL Detect(LPWSTR AppID);
protected:
	Microsoft::WRL::ComPtr<ICoreWebView2Environment> Env;
};

class EdgeWebView
{
public:
	VOID Action(int action);
	int Raw(const char* string);
	int URL(const char* url);
	int JavascriptRun(const char* script, void* scriptdata);
	int JavascriptAdd(const char* name);
	VOID DoSize(VOID);
	VOID Setup(HWND hwnd, ICoreWebView2Controller* webview);
	VOID Close(VOID);
protected:
	HWND hWnd = nullptr;
	Microsoft::WRL::ComPtr<ICoreWebView2> WebView;
	Microsoft::WRL::ComPtr<ICoreWebView2Controller> WebHost;
};

LRESULT CALLBACK EdgeBrowser::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_SIZE:
		{
			// Resize the browser object to fit the window
			EdgeWebView *webview;

			// Retrieve the browser object's pointer we stored in our window's GWL_USERDATA when
			// we initially attached the browser object to this window.
			webview = (EdgeWebView*)dw_window_get_data(hWnd, _DW_HTML_DATA_NAME);
			// Resize WebView to fit the bounds of the parent window
			if (webview)
				webview->DoSize();
			return(0);
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			return(0);
		}

		case WM_CREATE:
		{
			// Step 3 - Create a single WebView within the parent window
			// Create a WebView, whose parent is the main window hWnd
			Env->CreateCoreWebView2Controller(hWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
				[hWnd](HRESULT result, ICoreWebView2Controller* webhost) -> HRESULT {
					EdgeWebView* WebView = new EdgeWebView;
					ICoreWebView2* webview;

					WebView->Setup(hWnd, webhost);
					dw_window_set_data(hWnd, _DW_HTML_DATA_NAME, DW_POINTER(WebView));

					if (SUCCEEDED(webhost->get_CoreWebView2(&webview))) {
						// Add a few settings for the webview
						// this is a redundant demo step as they are the default settings values
						ICoreWebView2Settings* Settings;
						webview->get_Settings(&Settings);
						Settings->put_IsScriptEnabled(TRUE);
						Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
						Settings->put_IsWebMessageEnabled(TRUE);
#ifndef DEBUG
						Settings->put_AreDevToolsEnabled(FALSE);
#endif

						// Save the token, we might need to dw_window_set_data() this value
						// for later use to remove the handlers
						EventRegistrationToken token;

						// Register a handler for the NavigationStarting event.
						webview->add_NavigationStarting(
							Callback<ICoreWebView2NavigationStartingEventHandler>(
								[hWnd](ICoreWebView2* sender,
									ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT
								{
									LPWSTR uri;
									sender->get_Source(&uri);

									_dw_wndproc(hWnd, WM_USER + 101, (WPARAM)DW_INT_TO_POINTER(DW_HTML_CHANGE_STARTED),
										!wcscmp(uri, L"about:blank") ? (LPARAM)"" : (LPARAM)WideToUTF8((LPWSTR)uri));

									return S_OK;
								}).Get(), &token);

						// Register a handler for the SourceChanged event.
						webview->add_SourceChanged(
							Callback<ICoreWebView2SourceChangedEventHandler >(
								[hWnd](ICoreWebView2* sender,
									ICoreWebView2SourceChangedEventArgs* args) -> HRESULT
								{
									LPWSTR uri;
									sender->get_Source(&uri);

									_dw_wndproc(hWnd, WM_USER + 101, (WPARAM)DW_INT_TO_POINTER(DW_HTML_CHANGE_REDIRECT),
										!wcscmp(uri, L"about:blank") ? (LPARAM)"" : (LPARAM)WideToUTF8((LPWSTR)uri));

									return S_OK;
								}).Get(), &token);

						// Register a handler for the ContentLoading event.
						webview->add_ContentLoading(
							Callback<ICoreWebView2ContentLoadingEventHandler >(
								[hWnd](ICoreWebView2* sender,
									ICoreWebView2ContentLoadingEventArgs* args) -> HRESULT
								{
									LPWSTR uri;
									sender->get_Source(&uri);

									_dw_wndproc(hWnd, WM_USER + 101, (WPARAM)DW_INT_TO_POINTER(DW_HTML_CHANGE_LOADING),
										!wcscmp(uri, L"about:blank") ? (LPARAM)"" : (LPARAM)WideToUTF8((LPWSTR)uri));

									return S_OK;
								}).Get(), &token);

						// Register a handler for the NavigationCompleted event.
						webview->add_NavigationCompleted(
							Callback<ICoreWebView2NavigationCompletedEventHandler>(
								[hWnd](ICoreWebView2* sender,
									ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT
								{
									LPWSTR uri;
									sender->get_Source(&uri);

									_dw_wndproc(hWnd, WM_USER + 101, (WPARAM)DW_INT_TO_POINTER(DW_HTML_CHANGE_COMPLETE),
										!wcscmp(uri, L"about:blank") ? (LPARAM)"" : (LPARAM)WideToUTF8((LPWSTR)uri));

									return S_OK;
								}).Get(), &token);
								
						// Register a handler for the WebMessageReceived event.
						webview->add_WebMessageReceived(
							Callback<ICoreWebView2WebMessageReceivedEventHandler>(
								[hWnd](ICoreWebView2* sender,
									ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT
								{
									LPWSTR message, name = NULL, body = NULL;
									args->TryGetWebMessageAsString(&message);

									// Locate the DWindows|<function name>| signature and body
									if(message && !wcsncmp(message, L"DWindows|", 9)) {
										name = message + 9;
										if(*name) {
											body = wcschr(name, L'|');
											if(body) {
												*body = 0;
												body++;
											}
										}
									}
									if(name && body)
										_dw_wndproc(hWnd, WM_USER + 103, (WPARAM)WideToUTF8(name), (LPARAM)WideToUTF8(body));

									return S_OK;
								}).Get(), &token);
					}

					// Resize WebView to fit the bounds of the parent window
					WebView->DoSize();

					// Handle cached load requests due to delayed
					// loading of the edge webview contexts
					char *url = (char *)dw_window_get_data(hWnd, _DW_HTML_DATA_LOCATION);
					if (url)
					{
						WebView->URL(url);
						dw_window_set_data(hWnd, _DW_HTML_DATA_LOCATION, NULL);
						free((void*)url);
					}
					char *raw = (char *)dw_window_get_data(hWnd, _DW_HTML_DATA_RAW);
					if (raw)
					{
						WebView->Raw(raw);
						dw_window_set_data(hWnd, _DW_HTML_DATA_RAW, NULL);
						free((void*)raw);
					}
					char *adds = (char *)dw_window_get_data(hWnd, _DW_HTML_DATA_ADD);
					if(adds)
					{
						char *start = adds, *separator;

						while((separator = strchr(start, '|')))
						{
							*separator = 0;
							WebView->JavascriptAdd(start);
							start = separator + 1;
						}
						WebView->JavascriptAdd(start);
						dw_window_set_data(hWnd, _DW_HTML_DATA_ADD, NULL);
						free((void *)adds);
					}
					return S_OK;
				}).Get());
			// Success
			return(0);
		}

		case WM_DESTROY:
		{
			// Detach the browser object from this window, and free resources.
			EdgeWebView *webview;

			// Retrieve the browser object's pointer we stored in our window's GWL_USERDATA when
			// we initially attached the browser object to this window.
			webview = (EdgeWebView*)dw_window_get_data(hWnd, _DW_HTML_DATA_NAME);
			if (webview)
			{
				dw_window_set_data(hWnd, _DW_HTML_DATA_NAME, NULL);
				webview->Close();
				delete webview;
			}
			return(TRUE);
		}
	}

	return(DefWindowProc(hWnd, uMsg, wParam, lParam));
}

VOID EdgeWebView::DoSize(VOID)
{
	RECT bounds;
	BOOL isVisible;

	GetClientRect(hWnd, &bounds);
	WebHost->put_Bounds(bounds);
	WebHost->get_IsVisible(&isVisible);
	if(!isVisible)
		WebHost->put_IsVisible(TRUE);
}

BOOL EdgeBrowser::Detect(LPWSTR AppID)
{
	// Combine two buffer lengths, ".WebView2\" (10) and a NULL
	WCHAR tempdir[MAX_PATH+_DW_APP_ID_SIZE+11] = {0};

	GetTempPathW(MAX_PATH, tempdir);
	wcscat(tempdir, AppID);
	wcscat(tempdir, L".WebView2\\");
	CreateDirectoryW(tempdir, NULL);

	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

	CreateCoreWebView2EnvironmentWithOptions(nullptr, tempdir, nullptr,
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
				// Successfully created Edge environment, return TRUE 
				Env = env;
				return S_OK;
			}).Get());
	// If our first attempt was unsuccessful, attempt to load Edge Stable instead
	if(!Env)
	{
		// Combine tempdir length, "EdgeStable" (10) and a NULL
		WCHAR edgepath[sizeof(tempdir)+10] = {0};

		wcscpy(edgepath, tempdir);
		wcscat(edgepath, L"EdgeStable");

		// Create the NTFS junction to get around Microsoft's path blacklist
		_dw_create_junction(_dw_get_edge_stable_path(), edgepath);

		CreateCoreWebView2EnvironmentWithOptions(edgepath, tempdir, nullptr,
			Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
				[this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
					// Successfully created Edge environment, return TRUE 
					Env = env;
					return S_OK;
				}).Get());
	}
	return Env ? TRUE : FALSE;
}

void EdgeWebView::Action(int action)
{
	// We want to get the base address (ie, a pointer) to the IWebView2WebView object embedded within the browser
	// object, so we can call some of the functions in the former's table.
	if (WebView)
	{
		// Call the desired function
		switch (action)
		{
			case DW_HTML_GOBACK:
			{
				// Call the IWebView2WebView object's GoBack function.
				WebView->GoBack();
				break;
			}

			case DW_HTML_GOFORWARD:
			{
				// Call the IWebView2WebView object's GoForward function.
				WebView->GoForward();
				break;
			}

			case DW_HTML_GOHOME:
			{
				// Call the IWebView2WebView object's GoHome function.
				dw_html_url(hWnd, (char*)DW_HOME_URL);
				break;
			}

			case DW_HTML_SEARCH:
			{
				// Call the IWebView2WebView object's GoSearch function.
				//WebView->GoSearch();
				break;
			}

			case DW_HTML_RELOAD:
			{
				// Call the IWebView2WebView object's Refresh function.
				WebView->Reload();
			}

			case DW_HTML_STOP:
			{
				// Call the IWebView2WebView object's Stop function.
				WebView->Stop();
			}
		}
	}
}

int EdgeWebView::Raw(const char* string)
{
	if (WebView)
		WebView->NavigateToString(UTF8toWide(string));
	return DW_ERROR_NONE;
}

int EdgeWebView::URL(const char* url)
{
	if (WebView)
		WebView->Navigate(UTF8toWide(url));
	return DW_ERROR_NONE;
}

int EdgeWebView::JavascriptRun(const char* script, void* scriptdata)
{
	HWND thishwnd = hWnd;

	if (WebView)
		WebView->ExecuteScript(UTF8toWide(script),
			Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
				[thishwnd, scriptdata](HRESULT error, PCWSTR result) -> HRESULT
				{
					char *scriptresult;
					
					/* Result is unquoted "null" when we should return NULL */
					if (result && _wcsicmp(result, L"null") == 0)
						scriptresult = NULL;
					else
						scriptresult = result ? WideToUTF8((LPWSTR)result) : NULL;
					
					/* String results are enclosed in quotations, remove the quotes */
					if(scriptresult && *scriptresult == '\"')
					{
						char *end = strrchr(scriptresult, '\"');
						if(end)
							*end = '\0';
						scriptresult++;
					}
					void *params[2] = { (void *)scriptresult, DW_INT_TO_POINTER((error == S_OK ? DW_ERROR_NONE : DW_ERROR_UNKNOWN)) };
					_dw_wndproc(thishwnd, WM_USER + 100, (WPARAM)params, (LPARAM)scriptdata);
					return S_OK;
				}).Get());
	return DW_ERROR_NONE;
}

int EdgeWebView::JavascriptAdd(const char* name)
{
	if (WebView) {
		std::wstring wname = std::wstring(UTF8toWide(name));
		std::wstring script = L"function " + wname + L"(body) {window.chrome.webview.postMessage('DWindows|" + wname + L"|' + body);}";
		WebView->AddScriptToExecuteOnDocumentCreated(script.c_str(),
			Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
					[this](HRESULT error, PCWSTR id) -> HRESULT {
						return S_OK;
					}).Get());
		return DW_ERROR_NONE;
	}
	return DW_ERROR_UNKNOWN;
}

VOID EdgeWebView::Setup(HWND hwnd, ICoreWebView2Controller* host)
{
	hWnd = hwnd;
	WebHost = host;
	host->get_CoreWebView2(&WebView);
}

VOID EdgeWebView::Close(VOID)
{
	if (WebHost)
		WebHost->Close();
}

EdgeBrowser *DW_EDGE = NULL;

extern "C" {
	// Create a junction to Edge Stable current version so we can load it...
	// For now we are using CreateProcess() to execute the mklink command...
	// May switch to using C code  but that seems to be overly complicated
	void _dw_create_junction(LPWSTR source, LPWSTR target)
	{
		// Command line must be at least 2 MAX_PATHs and "cmd /c mklink /J "<path1>" "<path2>"" (22) and a NULL
		WCHAR cmdLine[(MAX_PATH*2)+23] = L"cmd /c mklink /J \"";
		STARTUPINFO si = {sizeof(si)};
		PROCESS_INFORMATION pi = {0};
		
		// Safety check
		if(!source[0] || !target[0])
			return;

		// Combine the command line components
		wcscat(cmdLine, target);
		wcscat(cmdLine, L"\" \"");
		wcscat(cmdLine, source);
		wcscat(cmdLine, L"\"");

		// First remove any existing junction to an old version
		RemoveDirectoryW(target);

		// Create the junction to the new version
		if(!CreateProcessW(NULL, cmdLine, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
			return;

		// Wait until child process exits.
		WaitForSingleObject(pi.hProcess, INFINITE);

		// Close process and thread handles. 
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	// Return the path the the current Edge Stable version
	LPWSTR _dw_get_edge_stable_path(void)
	{
		HKEY hKey;
		WCHAR szBuffer[100] = {0};
		DWORD dwBufferSize = sizeof(szBuffer);
		static WCHAR EdgeStablePath[MAX_PATH+1] = {0};

		/* If we haven't successfully gotten the path, try to find it in the registry */
		if(!EdgeStablePath[0])
		{
			// Handle the case we are running on x64
			if(RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Microsoft\\EdgeUpdate\\Clients\\{56EB18F8-B008-4CBD-B6D2-8C97FE7E9062}", 0, KEY_READ, &hKey) == ERROR_SUCCESS &&
				RegQueryValueExW(hKey, L"pv", 0, NULL, (LPBYTE)szBuffer, &dwBufferSize) == ERROR_SUCCESS)
			{
				wcscpy(EdgeStablePath, L"C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\");
				wcscat(EdgeStablePath, szBuffer);
			}
			// and also the case we are running x86
			else if(RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\EdgeUpdate\\Clients\\{56EB18F8-B008-4CBD-B6D2-8C97FE7E9062}", 0, KEY_READ, &hKey) == ERROR_SUCCESS &&
					RegQueryValueExW(hKey, L"pv", 0, NULL, (LPBYTE)szBuffer, &dwBufferSize) == ERROR_SUCCESS)
			{
				wcscpy(EdgeStablePath, L"C:\\Program Files\\Microsoft\\Edge\\Application\\");
				wcscat(EdgeStablePath, szBuffer);
			}
		}
		return EdgeStablePath;
	 }

	/******************************* dw_edge_detect() **************************
	 * Attempts to create a temporary Edge (Chromium) browser context...
	 * If we succeed return TRUE and use Edge for HTML windows.
	 * If it fails return FALSE and fall back to using embedded IE.
	 */
	BOOL _dw_edge_detect(LPWSTR AppID)
	{
		DW_EDGE = new EdgeBrowser;
		if (DW_EDGE)
		{
			BOOL result = DW_EDGE->Detect(AppID);
			if (!result)
			{
				delete DW_EDGE;
				DW_EDGE = NULL;
			}
			return result;
		}
		return FALSE;
	}

	/******************************* dw_edge_action() **************************
	 * Implements the functionality of a "Back". "Forward", "Home", "Search",
	 * "Refresh", or "Stop" button.
	 *
	 * hwnd =		Handle to the window hosting the browser object.
	 * action =		One of the following:
	 *				0 = Move back to the previously viewed web page.
	 *				1 = Move forward to the previously viewed web page.
	 *				2 = Move to the home page.
	 *				3 = Search.
	 *				4 = Refresh the page.
	 *				5 = Stop the currently loading page.
	 */

	void _dw_edge_action(HWND hwnd, int action)
	{
		EdgeWebView* webview = (EdgeWebView *)dw_window_get_data(hwnd, _DW_HTML_DATA_NAME);
		if (webview)
			webview->Action(action);
	}

	/******************************* dw_edge_raw() ****************************
	 * Takes a string containing some HTML BODY, and displays it in the specified
	 * window. For example, perhaps you want to display the HTML text of...
	 *
	 * <P>This is a picture.<P><IMG src="mypic.jpg">
	 *
	 * hwnd =		Handle to the window hosting the browser object.
	 * string =		Pointer to nul-terminated string containing the HTML BODY.
	 *				(NOTE: No <BODY></BODY> tags are required in the string).
	 *
	 * RETURNS: 0 if success, or non-zero if an error.
	 */

	int _dw_edge_raw(HWND hwnd, const char* string)
	{
		EdgeWebView* webview = (EdgeWebView*)dw_window_get_data(hwnd, _DW_HTML_DATA_NAME);
		if (webview)
			return webview->Raw(string);
		else {
			char *oldstring = (char *)dw_window_get_data(hwnd, _DW_HTML_DATA_RAW);
			dw_window_set_data(hwnd, _DW_HTML_DATA_RAW, _strdup(string));
			if(oldstring)
				free((void *)oldstring);
		}
		return DW_ERROR_NONE;
	}

	/******************************* dw_edge_url() ****************************
	 * Displays a URL, or HTML file on disk.
	 *
	 * hwnd =		Handle to the window hosting the browser object.
	 * url	=		Pointer to nul-terminated name of the URL/file.
	 *
	 * RETURNS: 0 if success, or non-zero if an error.
	 */

	int _dw_edge_url(HWND hwnd, const char* url)
	{
		EdgeWebView* webview = (EdgeWebView*)dw_window_get_data(hwnd, _DW_HTML_DATA_NAME);
		if (webview)
			return webview->URL(url);
		else {
			char *oldurl = (char *)dw_window_get_data(hwnd, _DW_HTML_DATA_LOCATION);
			dw_window_set_data(hwnd, _DW_HTML_DATA_LOCATION, _strdup(url));
			if(oldurl)
				free((void *)oldurl);
		}
		return DW_ERROR_NONE;
	}

	/******************************* dw_edge_javascript_run() ****************************
	 * Runs a javascript in the specified browser context.
	 *
	 * hwnd			=	Handle to the window hosting the browser object.
	 * script		=	Pointer to nul-terminated javascript string.
	 * scriptdata	=   Pointer to user data to be passed to the callback.
	 *
	 * RETURNS: 0 if success, or non-zero if an error.
	 */

	int _dw_edge_javascript_run(HWND hwnd, const char* script, void* scriptdata)
	{
		EdgeWebView* webview = (EdgeWebView*)dw_window_get_data(hwnd, _DW_HTML_DATA_NAME);
		if (webview)
			return webview->JavascriptRun(script, scriptdata);
		return DW_ERROR_UNKNOWN;
	}

	/******************************* dw_edge_javascript_add() ****************************
	 * Adds a javascript function in the specified browser context.
	 *
	 * hwnd			=	Handle to the window hosting the browser object.
	 * name			=	Pointer to nul-terminated javascript function name string.
	 *
	 * RETURNS: 0 if success, or non-zero if an error.
	 */

	int _dw_edge_javascript_add(HWND hwnd, const char* name)
	{
		if(name) {
			EdgeWebView* webview = (EdgeWebView*)dw_window_get_data(hwnd, _DW_HTML_DATA_NAME);
			if (webview)
				return webview->JavascriptAdd(name);
			else {
				char *oldadd = (char *)dw_window_get_data(hwnd, _DW_HTML_DATA_ADD);
				char *newadd = (char *)calloc(strlen(name) + (oldadd ? strlen(oldadd) : 0) + 2, 1);
				if(oldadd) {
					strcpy(newadd, oldadd);
					strcat(newadd, "|");
				}
				strcat(newadd, name);
				dw_window_set_data(hwnd, _DW_HTML_DATA_ADD, newadd);
				if(oldadd)
					free((void *)oldadd);
				return DW_ERROR_NONE;
			}
		}
		return DW_ERROR_UNKNOWN;
	}

	/************************** edgeWindowProc() *************************
	 * Our message handler for our window to host the browser.
	 */

	LRESULT CALLBACK _dw_edgewndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (DW_EDGE)
			return DW_EDGE->WndProc(hWnd, uMsg, wParam, lParam);
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}