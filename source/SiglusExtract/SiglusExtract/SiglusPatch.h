//	 kDays_supp.h： 补充头文件kDays.dll
//	 作者： DLL to Lib version 3.00
//	 日期： Saturday, June 03, 2017
//	 描述： kDays.dll的入口点函数的声明。
//	 原型： BOOL WINAPI xxx_DllMain(HINSTANCE hinstance, DWORD fdwReason, LPVOID lpvReserved);
//	 参数: 
//		hinstance
//		 应用程序的当前实例的句柄。使用 AfxGetInstanceHandle()
//		 获取实例的句柄，如果您的项目具有 MFC 支持。
//		fdwReason
//		 指定一个指示为什么被称为入口点函数的标志。
//		lpvReserved 
//		 应经常初始化和清理指定DLL
//		 将设置为空；
//	 注释： 入口点的详细信息请参阅帮助文档
//		 功能
//	 主页： http://www.binary-soft.com
//	 Technical 支持： support@binary-soft.com
/////////////////////////////////////////////////////////////////////

#if !defined(D2L_KDAYS_SUPP_H__643A694E_453C_1A1D_1112_521E393C1937__INCLUDED_)
#define D2L_KDAYS_SUPP_H__643A694E_453C_1A1D_1112_521E393C1937__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef __cplusplus
extern "C" {
#endif


#include <windows.h>

	/* 这是kDays.dll的入口点函数。你应该把它做必要
	初始化和终止。 */

	BOOL WINAPI KDAYS_DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);


#ifdef __cplusplus
}
#endif

#endif // !defined(D2L_KDAYS_SUPP_H__643A694E_453C_1A1D_1112_521E393C1937__INCLUDED_)
