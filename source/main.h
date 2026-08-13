#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define OEMRESOURCE

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dwmapi.h>
#include <dxgi1_2.h>
#include <cstdio>
#include <cstdint>
#include <chrono>
#include <thread>
#include <xmmintrin.h>
#include <filesystem>

#include "utils.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "dxgi.lib")

//

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HINSTANCE InitWindowHandler();
void CreateDimWindow(HINSTANCE hInst, HWND& hwnd);
void UpdateDimWindow(HWND& hwnd);
void CleanUpD3D();
bool ReInitD3D(HWND hwnd);
void Render();