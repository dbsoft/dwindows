/* edge.cpp
 *
 * Allows dw_html_new() to embed a Microsoft Edge (Chromium) browser.
 *
 * Requires Windows 10, 8 or 7 with Microsoft Edge (Chromium) installed.
 * 
 * Only included when BUILD_EDGE is defined, will fall back to embedded IE.
 *
 * Currently only buildable with Visual Studio since it requires the EDGE
 * SDK which is currently distributed as a nuget package. 
 */
#include "dw.h"
#include "webview2.h"
#include <wrl.h>

using namespace Microsoft::WRL;

#define _DW_HTML_DATA_NAME "_dw_edge"
#define _DW_HTML_DATA_LOCATION "_dw_edge_location"
#define _DW_HTML_DATA_RAW "_dw_edge_raw"

extern "C" {

	/* Import the character conversion functions from dw.c */
	LPWSTR _myUTF8toWide(const char* utf8string, void* outbuf);
	char* _myWideToUTF8(LPCWSTR widestring, void* outbuf);
	#define UTF8toWide(a) _myUTF8toWide(a, a ? _alloca(MultiByteToWideChar(CP_UTF8, 0, a, -1, NULL, 0) * sizeof(WCHAR)) : NULL)
	#define WideToUTF8(a) _myWideToUTF8(a, a ? _alloca(WideCharToMultiByte(CP_UTF8, 0, a, -1, NULL, 0, NULL, NULL)) : NULL)
	LRESULT CALLBACK _wndproc(HWND hWnd, UINT msg, WPARAM mp1, LPARAM mp2);
	BOOL CALLBACK _free_window_memory(HWND handle, LPARAM lParam);
}

class EdgeBrowser
{
public:
	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	BOOL Detect(VOID);
protected:
	Microsoft::WRL::ComPtr<IWebView2Environment> Env;
};

class EdgeWebView
{
public:
	VOID Action(int action);
	int Raw(const char* string);
	int URL(const char* url);
	int JavascriptRun(const char* script, void* scriptdata);
	VOID DoSize(VOID);
	VOID Setup(HWND hwnd, IWebView2WebView* webview);
	VOID Close(VOID);
protected:
	HWND hWnd = nullptr;
	Microsoft::WRL::ComPtr<IWebView2WebView> WebView;
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
		HDC hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		return(0);
	}

	case WM_CREATE:
	{
		// Step 3 - Create a single WebView within the parent window
		// Create a WebView, whose parent is the main window hWnd
		Env->CreateWebView(hWnd, Callback<IWebView2CreateWebViewCompletedHandler>(
			[hWnd](HRESULT result, IWebView2WebView* webview) -> HRESULT {
				EdgeWebView* WebView = new EdgeWebView;

				WebView->Setup(hWnd, webview);
				dw_window_set_data(hWnd, _DW_HTML_DATA_NAME, DW_POINTER(WebView));

				// Add a few settings for the webview
				// this is a redundant demo step as they are the default settings values
				IWebView2Settings* Settings;
				webview->get_Settings(&Settings);
				Settings->put_IsScriptEnabled(TRUE);
				Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
				Settings->put_IsWebMessageEnabled(TRUE);

				// Resize WebView to fit the bounds of the parent window
				WebView->DoSize();

				// Save the token, we might need to dw_window_set_data() this value
				// for later use to remove the handlers
				EventRegistrationToken token;

				// Register a handler for the NavigationStarting event.
				webview->add_NavigationStarting(
					Callback<IWebView2NavigationStartingEventHandler>(
						[hWnd](IWebView2WebView* sender,
							IWebView2NavigationStartingEventArgs* args) -> HRESULT
						{
							LPWSTR uri;
							sender->get_Source(&uri);

							_wndproc(hWnd, WM_USER + 101, (WPARAM)DW_INT_TO_POINTER(DW_HTML_CHANGE_STARTED),
								!wcscmp(uri, L"about:blank") ? (LPARAM)"" : (LPARAM)WideToUTF8((LPWSTR)uri));

							return S_OK;
						}).Get(), &token);

				// Register a handler for the DocumentStateChanged event.
				webview->add_DocumentStateChanged(
					Callback<IWebView2DocumentStateChangedEventHandler>(
						[hWnd](IWebView2WebView* sender,
							IWebView2DocumentStateChangedEventArgs* args) -> HRESULT
						{
							LPWSTR uri;
							sender->get_Source(&uri);

							_wndproc(hWnd, WM_USER + 101, (WPARAM)DW_INT_TO_POINTER(DW_HTML_CHANGE_LOADING),
								!wcscmp(uri, L"about:blank") ? (LPARAM)"" : (LPARAM)WideToUTF8((LPWSTR)uri));

							return S_OK;
						}).Get(), &token);

				// Register a handler for the NavigationCompleted event.
				webview->add_NavigationCompleted(
					Callback<IWebView2NavigationCompletedEventHandler>(
						[hWnd](IWebView2WebView* sender,
							IWebView2NavigationCompletedEventArgs* args) -> HRESULT
						{
							LPWSTR uri;
							sender->get_Source(&uri);

							_wndproc(hWnd, WM_USER + 101, (WPARAM)DW_INT_TO_POINTER(DW_HTML_CHANGE_COMPLETE),
								!wcscmp(uri, L"about:blank") ? (LPARAM)"" : (LPARAM)WideToUTF8((LPWSTR)uri));

							return S_OK;
						}).Get(), &token);

				// Handle cached load requests due to delayed
				// loading of the edge webview contexts
				char *url = (char *)dw_window_get_data(hWnd, _DW_HTML_DATA_LOCATION);
				if (url)
				{
					WebView->URL(url);
					free((void*)url);
				}
				char *raw = (char *)dw_window_get_data(hWnd, _DW_HTML_DATA_RAW);
				if (raw)
				{
					WebView->Raw(raw);
					free((void*)raw);
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
		_free_window_memory(hWnd, 0);
		return(TRUE);
	}
	}

	return(DefWindowProc(hWnd, uMsg, wParam, lParam));
}

VOID EdgeWebView::DoSize(VOID)
{
	RECT bounds;

	GetClientRect(hWnd, &bounds);
	WebView->put_Bounds(bounds);
}

BOOL EdgeBrowser::Detect(VOID)
{
	CreateWebView2EnvironmentWithDetails(nullptr, nullptr, nullptr,
		Callback<IWebView2CreateWebView2EnvironmentCompletedHandler>(
			[this](HRESULT result, IWebView2Environment* env) -> HRESULT {
				// Successfully created Edge environment, return TRUE 
				Env = env;
				return S_OK;
			}).Get());
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
			//WebView->Stop();
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
			Callback<IWebView2ExecuteScriptCompletedHandler>(
				[thishwnd, scriptdata](HRESULT error, PCWSTR result) -> HRESULT
				{
					_wndproc(thishwnd, WM_USER + 100, (error == S_OK ? (WPARAM)WideToUTF8((LPWSTR)result) : NULL), (LPARAM)scriptdata);
					return S_OK;
				}).Get());
	return DW_ERROR_NONE;
}

VOID EdgeWebView::Setup(HWND hwnd, IWebView2WebView* webview)
{
	hWnd = hwnd;
	WebView = webview;
}

VOID EdgeWebView::Close(VOID)
{
	if (WebView)
		WebView->Close();
}

EdgeBrowser *DW_EDGE = NULL;

extern "C" {
	/******************************* dw_edge_detect() **************************
	 * Attempts to create a temporary Edge (Chromium) browser context...
	 * If we succeed return TRUE and use Edge for HTML windows.
	 * If it fails return FALSE and fall back to using embedded IE.
	 */
	BOOL _dw_edge_detect(VOID)
	{
		DW_EDGE = new EdgeBrowser;
		if (DW_EDGE)
		{
			BOOL result = DW_EDGE->Detect();
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
		else
			dw_window_set_data(hwnd, _DW_HTML_DATA_RAW, strdup(string));
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
		else
			dw_window_set_data(hwnd, _DW_HTML_DATA_LOCATION, strdup(url));
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

	/************************** edgeWindowProc() *************************
	 * Our message handler for our window to host the browser.
	 */

	LRESULT CALLBACK _edgeWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (DW_EDGE)
			return DW_EDGE->WndProc(hWnd, uMsg, wParam, lParam);
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}