// copytext.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "copytext.h"


class DesktopSelectionManager
{
public:
  void start(HWND hwnd)
  {
    if (!GetCursorPos(&m_start_mouse_pos))
    {
      return;
    }

    if (!GetCursorPos(&m_end_mouse_pos))
    {
      return;
    }

    m_is_running = true;
    setup_pos(hwnd);
  }

  void stop(HWND hwnd)
  {
    SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_HIDEWINDOW);
    m_is_running = false;

    auto desktop_hdc = GetDC(0);
    auto bitmap_hdc = CreateCompatibleDC(desktop_hdc);
    auto capture_bitmap = CreateCompatibleBitmap(desktop_hdc, m_capture_rc.right - m_capture_rc.left, m_capture_rc.bottom - m_capture_rc.top);
    SelectObject(bitmap_hdc, capture_bitmap);
    BitBlt(bitmap_hdc, 0, 0, m_capture_rc.right - m_capture_rc.left, m_capture_rc.bottom - m_capture_rc.top, desktop_hdc, m_capture_rc.left, m_capture_rc.top, SRCCOPY);
    save_to_file(capture_bitmap, bitmap_hdc, "testbmp.bmp");
    ReleaseDC(0, desktop_hdc);
    DeleteDC(bitmap_hdc);
    DeleteObject(capture_bitmap);
  }

  void timer(HWND hwnd)
  {
    if (!m_is_running)
    {
      return;
    }

    if (!GetCursorPos(&m_end_mouse_pos))
    {
      return;
    }

    setup_pos(hwnd);
  }

  void set_pen(HPEN pen)
  {
    m_pen = pen;
  }

  void draw(PAINTSTRUCT* ps)
  {
    SelectObject(ps->hdc, m_pen);
    SelectObject(ps->hdc, GetStockObject(NULL_BRUSH));
    Rectangle(ps->hdc, 0, 0, m_capture_rc.right - m_capture_rc.left, m_capture_rc.bottom - m_capture_rc.top);
  }

  bool is_running() const
  {
    return m_is_running;
  }

private:
  void save_to_file(HBITMAP bitmap, HDC bitmap_hdc, char* filepath)
  {
    int iBits;
    WORD wBitCount;
    DWORD dwPaletteSize = 0, dwBmBitsSize = 0, dwDIBSize = 0, dwWritten = 0;
    BITMAP Bitmap0;
    BITMAPFILEHEADER bmfHdr;
    BITMAPINFOHEADER bi;
    LPBITMAPINFOHEADER lpbi;
    HANDLE fh, hDib, hPal, hOldPal2 = NULL;
    iBits = GetDeviceCaps(bitmap_hdc, BITSPIXEL) * GetDeviceCaps(bitmap_hdc, PLANES);
    if (iBits <= 1)
      wBitCount = 1;
    else if (iBits <= 4)
      wBitCount = 4;
    else if (iBits <= 8)
      wBitCount = 8;
    else
      wBitCount = 24;
    GetObject(bitmap, sizeof(Bitmap0), (LPSTR)&Bitmap0);
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = Bitmap0.bmWidth;
    bi.biHeight = -Bitmap0.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = wBitCount;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrImportant = 0;
    bi.biClrUsed = 256;
    dwBmBitsSize = ((Bitmap0.bmWidth * wBitCount + 31) & ~31) / 8
      * Bitmap0.bmHeight;
    hDib = GlobalAlloc(GHND, dwBmBitsSize + dwPaletteSize + sizeof(BITMAPINFOHEADER));
    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
    *lpbi = bi;

    hPal = GetStockObject(DEFAULT_PALETTE);
    HDC hDC;
    if (hPal)
    {
      hDC = GetDC(NULL);
      hOldPal2 = SelectPalette(hDC, (HPALETTE)hPal, FALSE);
      RealizePalette(hDC);
    }

    GetDIBits(bitmap_hdc, bitmap, 0, (UINT)Bitmap0.bmHeight, (LPSTR)lpbi + sizeof(BITMAPINFOHEADER)
      + dwPaletteSize, (BITMAPINFO *)lpbi, DIB_RGB_COLORS);

    if (hOldPal2)
    {
      SelectPalette(hDC, (HPALETTE)hOldPal2, TRUE);
      RealizePalette(hDC);
      ReleaseDC(NULL, hDC);
    }

    fh = CreateFileA(filepath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (fh == INVALID_HANDLE_VALUE)
      return;

    bmfHdr.bfType = 0x4D42; // "BM"
    dwDIBSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwPaletteSize + dwBmBitsSize;
    bmfHdr.bfSize = dwDIBSize;
    bmfHdr.bfReserved1 = 0;
    bmfHdr.bfReserved2 = 0;
    bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + dwPaletteSize;

    WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);

    WriteFile(fh, (LPSTR)lpbi, dwDIBSize, &dwWritten, NULL);
    GlobalUnlock(hDib);
    GlobalFree(hDib);
    CloseHandle(fh);
  }

  void setup_capture_rect()
  {
    m_capture_rc.left = min(m_start_mouse_pos.x, m_end_mouse_pos.x);
    m_capture_rc.right = m_capture_rc.left + abs(m_start_mouse_pos.x - m_end_mouse_pos.x);
    m_capture_rc.top = min(m_start_mouse_pos.y, m_end_mouse_pos.y);
    m_capture_rc.bottom = m_capture_rc.top + abs(m_start_mouse_pos.y - m_end_mouse_pos.y);
  }

  void setup_pos(HWND hwnd)
  {
    setup_capture_rect();
    SetWindowPos(hwnd, 0, m_capture_rc.left, m_capture_rc.top, m_capture_rc.right - m_capture_rc.left, m_capture_rc.bottom - m_capture_rc.top, SWP_SHOWWINDOW);
  }

  bool m_is_running = false;
  POINT m_start_mouse_pos;
  POINT m_end_mouse_pos;
  RECT m_capture_rc;
  HPEN m_pen;
};


DesktopSelectionManager g_select_mgr;


bool is_hotkey_pressed_now()
{
  auto ctrl = GetAsyncKeyState(VK_CONTROL);
  auto alt = GetAsyncKeyState(VK_MENU);
  auto c_key = GetAsyncKeyState(0x43);
  return ctrl && alt && c_key;
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_COMMAND:
  {
    return DefWindowProc(hwnd, message, wParam, lParam);
  }
  break;
  case WM_PAINT:
  {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    g_select_mgr.draw(&ps);
    EndPaint(hwnd, &ps);
  }
  break;
  case WM_TIMER:
  {
    auto hotkey_pressed = is_hotkey_pressed_now();
    if (hotkey_pressed)
    {
      if (!g_select_mgr.is_running())
      {
        g_select_mgr.start(hwnd);
      }
    }
    else
    {
      if (g_select_mgr.is_running())
      {
        g_select_mgr.stop(hwnd);
      }
    }

    g_select_mgr.timer(hwnd);
  }
  break;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  default:
    return DefWindowProc(hwnd, message, wParam, lParam);
  }
  return 0;
}


int APIENTRY wWinMain(HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPWSTR lpCmdLine,
  int nCmdShow)
{
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  WNDCLASSEXW wcex;
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_COPYTEXT));
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wcex.lpszMenuName = nullptr;
  wcex.lpszClassName = L"cls1";
  wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
  RegisterClassExW(&wcex);


  HWND hwnd = CreateWindowEx(WS_EX_NOACTIVATE | WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
    L"cls1", L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);
  if (!hwnd)
  {
    return FALSE;
  }
  SetLayeredWindowAttributes(hwnd, RGB(255, 255, 255), 255, LWA_COLORKEY);
  ShowWindow(hwnd, nCmdShow);
  UpdateWindow(hwnd);

  g_select_mgr.set_pen(CreatePen(PS_DASH, 1, RGB(0,0,0)));

  HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_COPYTEXT));
  SetTimer(hwnd, 0, 30, NULL);

  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0))
  {
    if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  return (int)msg.wParam;
}