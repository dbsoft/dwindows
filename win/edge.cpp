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

#define _DW_HTML_DATA_NAME (char *)"_dw_edge"
#define _DW_HTML_DATA_LOCATION (char *)"_dw_edge_location"
#define _DW_HTML_DATA_RAW (char *)"_dw_edge_raw"

extern "C" {

	/* Import the character conversion functions from dw.c */
	LPWSTR _myUTF8toWide(char *utf8string, void *outbuf);
	char *_myWideToUTF8(LPWSTR widestring, void *outbuf);
	#define UTF8toWide(a) _myUTF8toWide(a, a ? _alloca(MultiByteToWideChar(CP_UTF8, 0, a, -1, NULL, 0) * sizeof(WCHAR)) : NULL)
	#define WideToUTF8(a) _myWideToUTF8(a, a ? _alloca(WideCharToMultiByte(CP_UTF8, 0, a, -1, NULL, 0, NULL, NULL)) : NULL)
	LRESULT CALLBACK _wndproc(HWND hWnd, UINT msg, WPARAM mp1, LPARAM mp2);
	BOOL CALLBACK _free_window_memory(HWND handle, LPARAM lParam);
	extern HWND DW_HWND_OBJECT;
	BOOL DW_EDGE_DETECTED = FALSE;

	/******************************* dw_edge_detect() **************************
	 * Attempts to create a temporary Edge (Chromium) browser context...
	 * If we succeed return TRUE and use Edge for HTML windows.
	 * If it fails return FALSE and fall back to using embedded IE.
	 */
	BOOL _dw_edge_detect(VOID)
	{
		HWND hWnd = DW_HWND_OBJECT;

		CreateWebView2EnvironmentWithDetails(nullptr, nullptr, nullptr,
			Callback<IWebView2CreateWebView2EnvironmentCompletedHandler>(
				[hWnd](HRESULT result, IWebView2Environment* env) -> HRESULT {
					// Successfully created Edge environment, return TRUE 
					DW_EDGE_DETECTED = TRUE;
					return S_OK;
				}).Get());
		return DW_EDGE_DETECTED;
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
		IWebView2WebView* webview;

		// Retrieve the browser object's pointer we stored in our window's GWL_USERDATA when
		// we initially attached the browser object to this window.
		webview = (IWebView2WebView*)dw_window_get_data(hwnd, _DW_HTML_DATA_NAME);

		// We want to get the base address (ie, a pointer) to the IWebView2WebView object embedded within the browser
		// object, so we can call some of the functions in the former's table.
		if (webview)
		{
			// Call the desired function
			switch (action)
			{
			case DW_HTML_GOBACK:
			{
				// Call the IWebView2WebView object's GoBack function.
				webview->GoBack();
				break;
			}

			case DW_HTML_GOFORWARD:
			{
				// Call the IWebView2WebView object's GoForward function.
				webview->GoForward();
				break;
			}

			case DW_HTML_GOHOME:
			{
				// Call the IWebView2WebView object's GoHome function.
				dw_html_url(hwnd, (char *)DW_HOME_URL);
				break;
			}

			case DW_HTML_SEARCH:
			{
				// Call the IWebView2WebView object's GoSearch function.
				//webview->GoSearch();
				break;
			}

			case DW_HTML_RELOAD:
			{
				// Call the IWebView2WebView object's Refresh function.
				webview->Reload();
			}

			case DW_HTML_STOP:
			{
				// Call the IWebView2WebView object's Stop function.
				//webview->Stop();
			}
			}
		}
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

	int _dw_edge_raw(HWND hwnd, char *string)
	{
		IWebView2WebView* webview;

		// Retrieve the browser object's pointer we stored in our window's GWL_USERDATA when
		// we initially attached the browser object to this window.
		webview = (IWebView2WebView*)dw_window_get_data(hwnd, _DW_HTML_DATA_NAME);

		if (webview)
			webview->NavigateToString(UTF8toWide(string));
		else
			dw_window_set_data(hwnd, _DW_HTML_DATA_RAW, _wcsdup(UTF8toWide(string)));
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

	int _dw_edge_url(HWND hwnd, char *url)
	{
		IWebView2WebView* webview;

		// Retrieve the browser object's pointer we stored in our window's GWL_USERDATA when
		// we initially attached the browser object to this window.
		webview = (IWebView2WebView*)dw_window_get_data(hwnd, _DW_HTML_DATA_NAME);

		if (webview)
			webview->Navigate(UTF8toWide(url));
		else
			dw_window_set_data(hwnd, _DW_HTML_DATA_LOCATION, _wcsdup(UTF8toWide(url)));
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

	int _dw_edge_javascript_run(HWND hwnd, char *script, void *scriptdata)
	{
		IWebView2WebView* webview;

		// Retrieve the browser object's pointer we stored in our window's GWL_USERDATA when
		// we initially attached the browser object to this window.
		webview = (IWebView2WebView*)dw_window_get_data(hwnd, _DW_HTML_DATA_NAME);

		if (webview)
			webview->ExecuteScript(UTF8toWide(script),
				Callback<IWebView2ExecuteScriptCompletedHandler>(
					[hwnd, scriptdata](HRESULT error, PCWSTR result) -> HRESULT
					{
						_wndproc(hwnd, WM_USER + 100, (error == S_OK ? (WPARAM)WideToUTF8((LPWSTR)result) : NULL), (LPARAM)scriptdata);
						return S_OK;
					}).Get());
		return DW_ERROR_NONE;
	}

	/************************** edgeWindowProc() *************************
	 * Our message handler for our window to host the browser.
	 */

	LRESULT CALLBACK _edgeWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_SIZE:
		{
			// Resize the browser object to fit the window
			IWebView2WebView* webview;

			// Retrieve the browser object's pointer we stored in our window's GWL_USERDATA when
			// we initially attached the browser object to this window.
			webview = (IWebView2WebView*)dw_window_get_data(hWnd, _DW_HTML_DATA_NAME);
			// Resize WebView to fit the bounds of the parent window
			if (webview)
			{
				RECT bounds;

				GetClientRect(hWnd, &bounds);
				webview->put_Bounds(bounds);
			}
			return(0);
		}

		case WM_CREATE:
		{
			// Step 3 - Create a single WebView within the parent window
			// Locate the browser and set up the environment for WebView
			CreateWebView2EnvironmentWithDetails(nullptr, nullptr, nullptr,
				Callback<IWebView2CreateWebView2EnvironmentCompletedHandler>(
					[hWnd](HRESULT result, IWebView2Environment* env) -> HRESULT {

						// Create a WebView, whose parent is the main window hWnd
						env->CreateWebView(hWnd, Callback<IWebView2CreateWebViewCompletedHandler>(
							[hWnd](HRESULT result, IWebView2WebView* webview) -> HRESULT {
								if (webview != nullptr) {
									dw_window_set_data(hWnd, _DW_HTML_DATA_NAME, DW_POINTER(webview));
								}

								// Add a few settings for the webview
								// this is a redundant demo step as they are the default settings values
								IWebView2Settings* Settings;
								webview->get_Settings(&Settings);
								Settings->put_IsScriptEnabled(TRUE);
								Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
								Settings->put_IsWebMessageEnabled(TRUE);

								// Resize WebView to fit the bounds of the parent window
								RECT bounds;
								GetClientRect(hWnd, &bounds);
								webview->put_Bounds(bounds);

								// Save the token, we might need to dw_window_set_data() this value
								// for later use to remove the handlers
								EventRegistrationToken token;

								// Register a handler for the NavigationStarting event.
								// This handler will check the domain being navigated to, and if the domain
								// matches a list of blocked sites, it will cancel the navigation and
								// possibly display a warning page.  It will also disable JavaScript on
								// selected websites.
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
								// This handler will read the webview's source URI and update
								// the app's address bar.
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
								// If the navigation was successful, update the back and forward buttons.
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
								LPCWSTR url = (LPCWSTR)dw_window_get_data(hWnd, _DW_HTML_DATA_LOCATION);
								if(url)
								{
									webview->Navigate(url);
									free((void *)url);
								}
								LPCWSTR raw = (LPCWSTR)dw_window_get_data(hWnd, _DW_HTML_DATA_RAW);
								if (raw)
								{
									webview->NavigateToString(raw);
									free((void *)raw);
								}
								return S_OK;
							}).Get());
						return S_OK;
					}).Get());

			// Success
			return(0);
		}

		case WM_DESTROY:
		{
			// Detach the browser object from this window, and free resources.
			IWebView2WebView* webview;

			// Retrieve the browser object's pointer we stored in our window's GWL_USERDATA when
			// we initially attached the browser object to this window.
			webview = (IWebView2WebView*)dw_window_get_data(hWnd, _DW_HTML_DATA_NAME);
			if (webview)
			{
				dw_window_set_data(hWnd, _DW_HTML_DATA_NAME, NULL);
				webview->Close();
			}
			_free_window_memory(hWnd, 0);
			return(TRUE);
		}
		}

		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
	}
}
