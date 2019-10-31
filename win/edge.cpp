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

#define _DW_HTML_DATA_NAME (char *)"_dw_edge"

BOOL _dw_edge_detect(VOID)
{
	return TRUE;
}

/******************************* dw_html_action() **************************
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
 *
 * NOTE: EmbedBrowserObject() must have been successfully called once with the
 * specified window, prior to calling this function. You need call
 * EmbedBrowserObject() once only, and then you can make multiple calls to
 * this function to display numerous pages in the specified window.
 */

void _dw_edge_action(HWND hwnd, int action)
{
	IWebView2WebView* webview;

	// Retrieve the browser object's pointer we stored in our window's GWL_USERDATA when
	// we initially attached the browser object to this window.

	webview = *((IWebView2WebView**)dw_window_get_data(hwnd, _DW_HTML_DATA_NAME));
	// We want to get the base address (ie, a pointer) to the IWebBrowser2 object embedded within the browser
	// object, so we can call some of the functions in the former's table.
	if (webview)
	{
		// Ok, now the pointer to our IWebBrowser2 object is in 'webBrowser2', and so its VTable is
		// webBrowser2->lpVtbl.

		// Call the desired function
		switch (action)
		{
			case DW_HTML_GOBACK:
			{
				// Call the IWebBrowser2 object's GoBack function.
				webview->GoBack();
				break;
			}

			case DW_HTML_GOFORWARD:
			{
				// Call the IWebBrowser2 object's GoForward function.
				webview->GoForward();
				break;
			}

			case DW_HTML_GOHOME:
			{
				// Call the IWebBrowser2 object's GoHome function.
				//webview->GoHome();
				break;
			}

			case DW_HTML_SEARCH:
			{
				// Call the IWebBrowser2 object's GoSearch function.
				//webview->GoSearch();
				break;
			}

			case DW_HTML_RELOAD:
			{
				// Call the IWebBrowser2 object's Refresh function.
				webview->Reload();
			}

			case DW_HTML_STOP:
			{
				// Call the IWebBrowser2 object's Stop function.
				//webview->Stop();
			}
		}
	}
}

/******************************* dw_html_raw() ****************************
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
 *
 * NOTE: EmbedBrowserObject() must have been successfully called once with the
 * specified window, prior to calling this function. You need call
 * EmbedBrowserObject() once only, and then you can make multiple calls to
 * this function to display numerous pages in the specified window.
 */

int _dw_edge_raw(HWND hwnd, LPCWSTR string)
{
	IWebView2WebView* webview;

	// Retrieve the browser object's pointer we stored in our window's GWL_USERDATA when
	// we initially attached the browser object to this window.
	webview = *((IWebView2WebView**)dw_window_get_data(hwnd, _DW_HTML_DATA_NAME));

	if (webview)
	{
		return DW_ERROR_NONE;
	}
	return DW_ERROR_UNKNOWN;
}

/******************************* dw_html_url() ****************************
 * Displays a URL, or HTML file on disk.
 *
 * hwnd =		Handle to the window hosting the browser object.
 * webPageName =	Pointer to nul-terminated name of the URL/file.
 *
 * RETURNS: 0 if success, or non-zero if an error.
 *
 * NOTE: EmbedBrowserObject() must have been successfully called once with the
 * specified window, prior to calling this function. You need call
 * EmbedBrowserObject() once only, and then you can make multiple calls to
 * this function to display numerous pages in the specified window.
 */

int _dw_edge_url(HWND hwnd, LPCWSTR url)
{
	IWebView2WebView * webview;

	// Retrieve the browser object's pointer we stored in our window's GWL_USERDATA when
	// we initially attached the browser object to this window.
	webview = *((IWebView2WebView**)dw_window_get_data(hwnd, _DW_HTML_DATA_NAME));

	if (webview)
	{
		webview->Navigate(url);
		return DW_ERROR_NONE;
	}
	return DW_ERROR_UNKNOWN;
}

/************************** browserWindowProc() *************************
 * Our message handler for our window to host the browser.
 */

LRESULT CALLBACK _edgeWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_SIZE:
		{
			// Resize the browser object to fit the window
			RECT bounds;
			IWebView2WebView* webview;

			// Retrieve the browser object's pointer we stored in our window's GWL_USERDATA when
			// we initially attached the browser object to this window.
			webview = *((IWebView2WebView**)dw_window_get_data(hWnd, _DW_HTML_DATA_NAME));
			GetClientRect(hWnd, &bounds);
			// Resize WebView to fit the bounds of the parent window
			if(webview)
				webview->put_Bounds(bounds);
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
			webview = *((IWebView2WebView**)dw_window_get_data(hWnd, _DW_HTML_DATA_NAME));
			if (webview)
			{
				dw_window_set_data(hWnd, _DW_HTML_DATA_NAME, NULL);
				webview->Close();

			}
			return(TRUE);
		}
	}

	return(DefWindowProc(hWnd, uMsg, wParam, lParam));
}

