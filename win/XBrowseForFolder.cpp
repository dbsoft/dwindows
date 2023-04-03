// XBrowseForFolder.cpp  Version 1.2
//
// Author:  Hans Dietrich
//          hdietrich@gmail.com
//
// Description:
//     XBrowseForFolder.cpp implements XBrowseForFolder(), a function that
//     wraps SHBrowseForFolder().
//
// History
//     Version 1.2 - 2008 February 29
//     - Changed API to allow for initial CSIDL.
//     - Added option to set dialog caption, suggested by SimpleDivX.
//     - Added option to set root, suggested by Jean-Michel Reghem.
//
//     Version 1.1 - 2003 September 29 (not released)
//     - Added support for edit box
//
//     Version 1.0 - 2003 September 25
//     - Initial public release
//
// License:
//     This software is released into the public domain.  You are free to use
//     it in any way you like, except that you may not sell this source code.
//
//     This software is provided "as is" with no expressed or implied warranty.
//     I accept no liability for any damage or loss of business that this
//     software may cause.
//
///////////////////////////////////////////////////////////////////////////////

/* Make sure we get the right version */
#define _WIN32_IE 0x0500

/* Check that the compiler can support AEROGLASS */
#if defined(_MSC_VER) && _MSC_VER < 1600 && defined(AEROGLASS)
#undef AEROGLASS
#endif

#ifndef __AFX_H__
#include <windows.h>
#include <tchar.h>
#endif

#include <Shlobj.h>
#include <io.h>
#include "XBrowseForFolder.h"

#ifndef __MINGW32__
#pragma warning(disable: 4127)	// conditional expression is constant (_ASSERTE)
#pragma warning(disable : 4996)	// disable bogus deprecation warning
#endif

/* MingW does not have this */
#if !defined(BIF_NONEWFOLDERBUTTON)
# define BIF_NONEWFOLDERBUTTON 0x200
#endif

//=============================================================================
// struct to pass to callback function
//=============================================================================
struct FOLDER_PROPS
{
	LPCTSTR lpszTitle;
	LPCTSTR lpszInitialFolder;
	UINT ulFlags;
};

#ifndef __AFX_H__

///////////////////////////////////////////////////////////////////////////////
// CRect - a minimal CRect class

class CRect : public tagRECT
{
public:
	//CRect() { }
	CRect(int l = 0, int t = 0, int r = 0, int b = 0)
	{
		left = l;
		top = t;
		right = r;
		bottom = b;
	}
	int Width() const { return right - left; }
	int Height() const { return bottom - top; }
	void SwapLeftRight() { SwapLeftRight(LPRECT(this)); }
	static void SwapLeftRight(LPRECT lpRect) { LONG temp = lpRect->left;
											   lpRect->left = lpRect->right;
											   lpRect->right = temp; }
	operator LPRECT() { return this; }
};
#endif

///////////////////////////////////////////////////////////////////////////////
// ScreenToClientX - helper function in case non-MFC
static void ScreenToClientX(HWND hWnd, LPRECT lpRect)
{
	::ScreenToClient(hWnd, (LPPOINT)lpRect);
	::ScreenToClient(hWnd, ((LPPOINT)lpRect)+1);
}

///////////////////////////////////////////////////////////////////////////////
// MoveWindowX - helper function in case non-MFC
static void MoveWindowX(HWND hWnd, CRect& rect, BOOL bRepaint)
{
	::MoveWindow(hWnd, rect.left, rect.top,
		rect.Width(), rect.Height(), bRepaint);
}

///////////////////////////////////////////////////////////////////////////////
// SizeBrowseDialog - resize dialog, move controls
static void SizeBrowseDialog(HWND hWnd, FOLDER_PROPS *fp)
{
	// find the folder tree and make dialog larger
	HWND hwndTree = FindWindowEx(hWnd, NULL, TEXT("SysTreeView32"), NULL);

	if (!hwndTree)
	{
		// ... this usually means that BIF_NEWDIALOGSTYLE is enabled.
		// Then the class name is as used in the code below.
		hwndTree = FindWindowEx(hWnd, NULL, TEXT("SHBrowseForFolder ShellNameSpace Control"), NULL);
	}

	CRect rectDlg;

	if (hwndTree)
	{
		// check if edit box
		int nEditHeight = 0;
		HWND hwndEdit = FindWindowEx(hWnd, NULL, TEXT("Edit"), NULL);
		CRect rectEdit;
		if (hwndEdit && (fp->ulFlags & BIF_EDITBOX))
		{
			::GetWindowRect(hwndEdit, &rectEdit);
			ScreenToClientX(hWnd, &rectEdit);
			nEditHeight = rectEdit.Height();
		}
		else if (hwndEdit)
		{
			::MoveWindow(hwndEdit, 20000, 20000, 10, 10, FALSE);
			::ShowWindow(hwndEdit, SW_HIDE);
			hwndEdit = 0;
		}

		// make the dialog larger
		::GetWindowRect(hWnd, &rectDlg);
		rectDlg.right += 40;
		rectDlg.bottom += 30;
		if (hwndEdit)
			rectDlg.bottom += nEditHeight + 5;
		MoveWindowX(hWnd, rectDlg, TRUE);
		::GetClientRect(hWnd, &rectDlg);

		int hMargin = 10;
		int vMargin = 10;

		// check if new dialog style - this means that there will be a resizing
		// grabber in lower right corner
		if (fp->ulFlags & BIF_NEWDIALOGSTYLE)
			hMargin = ::GetSystemMetrics(SM_CXVSCROLL);

		// move the Cancel button
		CRect rectCancel;
		HWND hwndCancel = ::GetDlgItem(hWnd, IDCANCEL);
		if (hwndCancel)
			::GetWindowRect(hwndCancel, &rectCancel);
		ScreenToClientX(hWnd, &rectCancel);
		int h = rectCancel.Height();
		int w = rectCancel.Width();
		rectCancel.bottom = rectDlg.bottom - vMargin;//nMargin;
		rectCancel.top = rectCancel.bottom - h;
		rectCancel.right = rectDlg.right - hMargin; //(scrollWidth + 2*borderWidth);
		rectCancel.left = rectCancel.right - w;
		if (hwndCancel)
		{
			MoveWindowX(hwndCancel, rectCancel, FALSE);
		}

		// move the OK button
		CRect rectOK(0, 0, 0, 0);
		HWND hwndOK = ::GetDlgItem(hWnd, IDOK);
		if (hwndOK)
			::GetWindowRect(hwndOK, &rectOK);
		ScreenToClientX(hWnd, &rectOK);
		rectOK.bottom = rectCancel.bottom;
		rectOK.top = rectCancel.top;
		rectOK.right = rectCancel.left - 10;
		rectOK.left = rectOK.right - w;
		if (hwndOK)
		{
			MoveWindowX(hwndOK, rectOK, FALSE);
		}

		// expand the folder tree to fill the dialog
		CRect rectTree;
		::GetWindowRect(hwndTree, &rectTree);
		ScreenToClientX(hWnd, &rectTree);
		if (hwndEdit)
		{
			rectEdit.left = hMargin;
			rectEdit.right = rectDlg.right - hMargin;
			rectEdit.top = vMargin;
			rectEdit.bottom = rectEdit.top + nEditHeight;
			MoveWindowX(hwndEdit, rectEdit, FALSE);
			rectTree.top = rectEdit.bottom + 5;
		}
		else
		{
			rectTree.top = vMargin;
		}
		rectTree.left = hMargin;
		rectTree.bottom = rectOK.top - 10;//nMargin;
		rectTree.right = rectDlg.right - hMargin;
		MoveWindowX(hwndTree, rectTree, FALSE);
	}
}

#ifdef AEROGLASS
extern "C" {
/* Include necessary variables and prototypes from dw.c */
extern int _DW_DARK_MODE_SUPPORTED;
extern int _DW_DARK_MODE_ENABLED;
extern BOOL (WINAPI * _dw_should_apps_use_dark_mode)(VOID);

BOOL _dw_is_high_contrast(VOID);
BOOL _dw_is_color_scheme_change_message(LPARAM lParam);
void _dw_refresh_titlebar_theme_color(HWND window);
BOOL CALLBACK _dw_set_child_window_theme(HWND window, LPARAM lParam);
}
#endif

///////////////////////////////////////////////////////////////////////////////
// BrowseCallbackProc - SHBrowseForFolder callback function
static int CALLBACK BrowseCallbackProc(HWND hWnd,		// Window handle to the browse dialog box
									   UINT uMsg,		// Value identifying the event
									   LPARAM lParam,	// Value dependent upon the message
									   LPARAM lpData)	// Application-defined value that was
														// specified in the lParam member of the
														// BROWSEINFO structure
{
	switch (uMsg)
	{
#ifdef AEROGLASS
		case WM_SETTINGCHANGE:
		{
			if(_DW_DARK_MODE_SUPPORTED && _dw_is_color_scheme_change_message(lpData))
			{
				_DW_DARK_MODE_ENABLED = _dw_should_apps_use_dark_mode() && !_dw_is_high_contrast();

				_dw_refresh_titlebar_theme_color(hWnd);
				_dw_set_child_window_theme(hWnd, 0);
				EnumChildWindows(hWnd, _dw_set_child_window_theme, 0);
			}
		}
		break;
#endif
		case BFFM_INITIALIZED:		// sent when the browse dialog box has finished initializing.
		{
			// remove context help button from dialog caption
			LONG lStyle = ::GetWindowLong(hWnd, GWL_STYLE);
			lStyle &= ~DS_CONTEXTHELP;
			::SetWindowLong(hWnd, GWL_STYLE, lStyle);
			lStyle = ::GetWindowLong(hWnd, GWL_EXSTYLE);
			lStyle &= ~WS_EX_CONTEXTHELP;
			::SetWindowLong(hWnd, GWL_EXSTYLE, lStyle);

			FOLDER_PROPS *fp = (FOLDER_PROPS *) lpData;
			if (fp)
			{
				if (fp->lpszInitialFolder && fp->lpszInitialFolder[0])
				{
					// set initial directory
					::SendMessage(hWnd, BFFM_SETSELECTION, TRUE, (LPARAM)fp->lpszInitialFolder);
				}

				if (fp->lpszTitle && fp->lpszTitle[0])
				{
					// set window caption
					::SetWindowText(hWnd, fp->lpszTitle);
				}
			}

#ifdef AEROGLASS
			if(_DW_DARK_MODE_SUPPORTED)
			{
				_dw_set_child_window_theme(hWnd, 0);
				EnumChildWindows(hWnd, _dw_set_child_window_theme, 0);
				_dw_refresh_titlebar_theme_color(hWnd);
			}
#endif
			SizeBrowseDialog(hWnd, fp);
		}
		break;

		case BFFM_SELCHANGED:		// sent when the selection has changed
		{
			TCHAR szDir[MAX_PATH*2] = { 0 };

			// fail if non-filesystem
			BOOL bRet = SHGetPathFromIDList((LPITEMIDLIST) lParam, szDir);
			if (bRet)
			{
				// fail if folder not accessible
				if (_taccess(szDir, 00) != 0)
				{
					bRet = FALSE;
				}
				else
				{
					SHFILEINFO sfi;
					::SHGetFileInfo((LPCTSTR)lParam, 0, &sfi, sizeof(sfi),
							SHGFI_PIDL | SHGFI_ATTRIBUTES);

					// fail if pidl is a link
					if (sfi.dwAttributes & SFGAO_LINK)
					{
						bRet = FALSE;
					}
				}
			}

			// if invalid selection, disable the OK button
			if (!bRet)
			{
				::EnableWindow(GetDlgItem(hWnd, IDOK), FALSE);
			}
		}
		break;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// XBrowseForFolder()
//
// Purpose:     Invoke the SHBrowseForFolder API.  If lpszInitialFolder is
//              supplied, it will be the folder initially selected in the tree
//              folder list.  Otherwise, the initial folder will be set to the
//              current directory.  The selected folder will be returned in
//              lpszBuf.
//
// Parameters:  hWnd              - handle to the owner window for the dialog
//              lpszInitialFolder - initial folder in tree;  if NULL, the initial
//                                  folder will be the current directory; if
//                                  if this is a CSIDL, must be a real folder.
//              nRootFolder       - optional CSIDL of root folder for tree;
//                                  -1 = use default.
//              lpszCaption       - optional caption for folder dialog
//              lpszBuf           - buffer for the returned folder path
//              dwBufSize         - size of lpszBuf in TCHARs
//              bEditBox          - TRUE = include edit box in dialog
//
// Returns:     BOOL - TRUE = success;  FALSE = user hit Cancel
//
BOOL _cdecl XBrowseForFolder(HWND hWnd,
					  LPCTSTR lpszInitialFolder,
					  int nRootFolder,
					  LPCTSTR lpszCaption,
					  LPTSTR lpszBuf,
					  DWORD dwBufSize,
					  BOOL bEditBox /*= FALSE*/)
{
	if (lpszBuf == NULL || dwBufSize < MAX_PATH)
		return FALSE;

	ZeroMemory(lpszBuf, dwBufSize*sizeof(TCHAR));

	BROWSEINFO bi = { 0 };

	// check if there is a special root folder
	LPITEMIDLIST pidlRoot = NULL;
	if (nRootFolder != -1)
	{
		if (SUCCEEDED(SHGetSpecialFolderLocation(hWnd, nRootFolder, &pidlRoot)))
			bi.pidlRoot = pidlRoot;
	}

	TCHAR szInitialPath[MAX_PATH*2] = { 0 };
	if (lpszInitialFolder)
	{
		// is this a folder path string or a csidl?
		if (HIWORD(lpszInitialFolder) == 0)
		{
			// csidl
			int nFolder = LOWORD((UINT)(UINT_PTR)lpszInitialFolder);
			SHGetSpecialFolderPath(hWnd, szInitialPath, nFolder, FALSE);
		}
		else
		{
			// string
			_tcsncpy(szInitialPath, lpszInitialFolder,
						sizeof(szInitialPath)/sizeof(TCHAR)-2);
		}
	}

	if (!szInitialPath[0] && !bi.pidlRoot)
	{
		// no initial folder and no root, set to current directory
		::GetCurrentDirectory(sizeof(szInitialPath)/sizeof(TCHAR)-2,
				szInitialPath);
	}

	FOLDER_PROPS fp;

	bi.hwndOwner = hWnd;
	bi.ulFlags   = BIF_RETURNONLYFSDIRS;	// do NOT use BIF_NEWDIALOGSTYLE,
											// or BIF_STATUSTEXT
	if (bEditBox)
		bi.ulFlags |= BIF_EDITBOX;
	bi.ulFlags  |= BIF_NONEWFOLDERBUTTON;
	bi.lpfn      = BrowseCallbackProc;
	bi.lParam    = (LPARAM) &fp;

	fp.lpszInitialFolder = szInitialPath;
	fp.lpszTitle = lpszCaption;
	fp.ulFlags = bi.ulFlags;

	BOOL bRet = FALSE;

	LPITEMIDLIST pidlFolder = SHBrowseForFolder(&bi);

	if (pidlFolder)
	{
		TCHAR szBuffer[MAX_PATH*2] = { 0 };

		if (SHGetPathFromIDList(pidlFolder, szBuffer))
		{
			_tcsncpy(lpszBuf, szBuffer, dwBufSize-1);
			bRet = TRUE;
		}
	}

	// free up pidls
	IMalloc *pMalloc = NULL;
	if (SUCCEEDED(SHGetMalloc(&pMalloc)) && pMalloc)
	{
		if (pidlFolder)
			pMalloc->Free(pidlFolder);
		if (pidlRoot)
			pMalloc->Free(pidlRoot);
		pMalloc->Release();
	}

	return bRet;
}
