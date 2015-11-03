// diploma.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "diploma.h"

// Raytracer stuffs
#include "raytracer.h"

// GetOpenFileName and stuff
#include <Windows.h>
#include <Commdlg.h>

// Temporary stuff
#define SCRWIDTH	1000
#define SCRHEIGHT	1000
// /////////////////

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_DIPLOMA, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DIPLOMA));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DIPLOMA));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_DIPLOMA);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}


//////////////////////////////////////////
HDC hdc;
unsigned int *imgpixels = 0;	// check for load!

raytracer::CanvasData *canv = 0;

static char bitmapbuffer[sizeof( BITMAPINFO ) + 16];	// Hack to draw bmp on screen
static BITMAPINFO* bh;
//////////////////////////////////////////



//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	
	hInst = hInstance; // Store instance handle in our global variable
	
	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX ,
	   CW_USEDEFAULT, 0, SCRWIDTH, SCRHEIGHT, NULL, NULL, hInstance, NULL);
	/*    hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);*/ // Original!
		
	if (!hWnd)
	{
	   return FALSE;
	}
	// ======================
	// Preparations code
	hdc = GetDC(hWnd);	// Drawing Context
	canv = new raytracer::CanvasData(SCRWIDTH,SCRHEIGHT);
	canv->Clear(0);
	imgpixels = canv->pixels;
	// Some dark magic related to bitmap conversion/drawing. Taken from Jacco Bikker example
	for ( int cc = 0; cc < sizeof( BITMAPINFOHEADER ) + 16; cc++ ) bitmapbuffer[cc] = 0;
	bh = (BITMAPINFO *)&bitmapbuffer;
	bh->bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
	bh->bmiHeader.biPlanes = 1;
	bh->bmiHeader.biBitCount = 32;
	bh->bmiHeader.biCompression = BI_BITFIELDS;
	bh->bmiHeader.biWidth = SCRWIDTH, bh->bmiHeader.biHeight = -SCRHEIGHT;
	((unsigned long*)bh->bmiColors)[0] = 255 << 16;
	((unsigned long*)bh->bmiColors)[1] = 255 << 8;
	((unsigned long*)bh->bmiColors)[2] = 255;
   // ======================
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	
	return TRUE;
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
		

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case IDM_DORENDER:
			InvalidateRect(hWnd, NULL, NULL);
			raytracer::DrawRaytraced(*canv);
			break;
		case IDM_OPENFILE:
			{OPENFILENAME ofn;       // common dialog box structure
			WCHAR szFile[260];       // buffer for file name
			HANDLE hf;               // file handle
			
			// Initialize OPENFILENAME
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hWnd;
			ofn.lpstrFile = szFile;
			// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
			// use the contents of szFile to initialize itself.
			ofn.lpstrFile[0] = '\0';
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = L".scene Files\0*.scene\0All\0*.*\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
			if (GetOpenFileName(&ofn)) {
				std::wstring conv(szFile);
				raytracer::LoadScene(std::string(conv.begin(),conv.end()));
				// use open.whatever to get data about the selected file
			}
			break;}
		case IDM_SAVEFILE:
			{
			OPENFILENAME ofn;       // common dialog box structure
			WCHAR szFile[260];      // buffer for file name
			HANDLE hf;              // file handle
			
			// Initialize OPENFILENAME
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hWnd;
			ofn.lpstrFile = szFile;

			ofn.lpstrFile[0] = '\0';

			ofn.lStructSize = sizeof(ofn); 
			ofn.hwndOwner = NULL;
			ofn.lpstrFilter = (LPCWSTR)L"Bitmap Image Files (*.bmp)\0*.bmp\0";
			ofn.lpstrFile = (LPWSTR)szFile;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
			ofn.lpstrDefExt = (LPCWSTR)L"bmp";
			if (GetSaveFileName(&ofn)) {
				std::wstring conv(szFile);
				raytracer::SaveRenderImage(std::string(conv.begin(),conv.end()), *canv);
			}

			break;}
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		if (!imgpixels) break;
		StretchDIBits( hdc, 0, 0, SCRWIDTH, SCRHEIGHT, 0, 0, SCRWIDTH, SCRHEIGHT, imgpixels, bh, DIB_RGB_COLORS, SRCCOPY );
		ValidateRect( hWnd, NULL ); // Re-draw it!
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
