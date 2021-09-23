#include <windows.h>
#include <iostream>
#include <string>

// totalcmd
#include "fsplugin.h"


// python
#define PY_SSIZE_T_CLEAN
#include <Python.h>




BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	return TRUE;
}


// common variables
int plugin_number;
bool python_initialized = false;

// main module and functions for python
PyObject* moduleMain;
PyObject* func_find_first;
PyObject* func_find_next;
PyObject* func_get_file;


// private methods

bool file_exists(char* script_path) {
	struct stat buffer;
	return (stat(script_path, &buffer) == 0);
}

void InitPython(char* script_path)
{
	Py_Initialize();

	PyObject* moduleMainString = PyUnicode_FromString("__main__");
	moduleMain = PyImport_Import(moduleMainString);

	PyObject* obj = Py_BuildValue("s", script_path);
	FILE* file = _Py_fopen_obj(obj, "r+");
	if (file != NULL) {
		PyRun_SimpleFile(file, "test.py");
	}

	func_find_first = PyObject_GetAttrString(moduleMain, "find_first");
	func_find_next = PyObject_GetAttrString(moduleMain, "find_next");
	func_get_file = PyObject_GetAttrString(moduleMain, "get_file");

	python_initialized = true;
}

// totalcmd methods


int __stdcall FsInit(int PluginNr, tProgressProc pProgressProc, tLogProc pLogProc, tRequestProc pRequestProc)
{
	plugin_number = PluginNr;
	return 0;
}

void __stdcall FsSetDefaultParams(FsDefaultParamStruct* dps)
{
	char fsplugin_ini[MAX_PATH];
	char script_path[MAX_PATH];
	const char* not_found = "not_found.xxx";
	const char* section = "TotalPython";

	strcpy_s(fsplugin_ini, dps->DefaultIniName);

	
	GetPrivateProfileString(section, "ScriptPath", not_found, script_path, MAX_PATH, fsplugin_ini);

	if (strcmp(script_path, not_found) == 0)
	{
		MessageBox(
			0
			, ("Python script path for this plugin has to be defined in file '" + std::string(fsplugin_ini) + "' in section 'TotalPython' with key 'ScriptPath'.").c_str()
			, "Python script is not defined"
			, 0);
		return;
	}

	if (!file_exists(script_path))
	{
		MessageBox(
			0
			, ("Python script '" + std::string(script_path) + "' doesn't exist.").c_str()
			, "Python script not found"
			, 0);
		return;
	}

	InitPython(script_path);
}

HANDLE __stdcall FsFindFirst(char* Path, WIN32_FIND_DATA* FindData)
{
	if (!python_initialized)
	{
		return INVALID_HANDLE_VALUE;
	}

	long size = 0;

	PyObject* filenameObj = nullptr;
	PyObject* attributesObj = nullptr;
	PyObject* sizeObj = nullptr;
	PyObject* updatedObj = nullptr;

	HANDLE ret = nullptr;
	DWORD last_error = ERROR_SUCCESS;

	PyObject* arg0 = PyUnicode_FromString(Path);
	PyObject* args = PyTuple_Pack(1, arg0);
	PyObject* result = PyObject_CallObject(func_find_first, args);

	if (!PyDict_Check(result))
	{
		last_error = 10001;
		ret = INVALID_HANDLE_VALUE;
		goto exit;
	}

	filenameObj = PyDict_GetItemString(result, "filename");

	if (filenameObj == Py_None)
	{
		last_error = ERROR_NO_MORE_FILES;
		ret = INVALID_HANDLE_VALUE;
		goto exit;
	}

	attributesObj = PyDict_GetItemString(result, "attributes");
	sizeObj = PyDict_GetItemString(result, "size");
	updatedObj = PyDict_GetItemString(result, "updated");


	memset(FindData, 0, sizeof(WIN32_FIND_DATA));

	////https://docs.microsoft.com/en-us/windows/win32/fileio/file-attribute-constants
	FindData->dwFileAttributes = PyLong_AsLong(attributesObj);
	FindData->ftLastWriteTime.dwHighDateTime = 0xFFFFFFFF;
	FindData->ftLastWriteTime.dwLowDateTime = 0xFFFFFFFE;

	// FindData->nFileSizeHigh = TODO, fix if size > int.max 
	FindData->nFileSizeLow = PyLong_AsLong(sizeObj);
	strcpy_s(FindData->cFileName, PyUnicode_AsUTF8(filenameObj));

exit:
	Py_XDECREF(arg0);
	Py_XDECREF(args);
	Py_XDECREF(result);

	SetLastError(last_error);
	return ret;
}


BOOL __stdcall FsFindNext(HANDLE Hdl, WIN32_FIND_DATA* FindData)
{
	long size = 0;

	PyObject* filenameObj = nullptr;
	PyObject* attributesObj = nullptr;
	PyObject* sizeObj = nullptr;
	PyObject* updatedObj = nullptr;

	BOOL ret = true;
	DWORD last_error = ERROR_SUCCESS;

	PyObject* args = PyTuple_Pack(0);
	PyObject* result = PyObject_CallObject(func_find_next, args);

	if (!PyDict_Check(result))
	{
		last_error = 10001;
		ret = false;
		goto exit;
	}

	filenameObj = PyDict_GetItemString(result, "filename");

	if (filenameObj == Py_None)
	{
		last_error = ERROR_NO_MORE_FILES;
		ret = false;
		goto exit;
	}

	attributesObj = PyDict_GetItemString(result, "attributes");
	sizeObj = PyDict_GetItemString(result, "size");
	updatedObj = PyDict_GetItemString(result, "updated");


	memset(FindData, 0, sizeof(WIN32_FIND_DATA));

	// to cleanup
	FindData->dwFileAttributes = PyLong_AsLong(attributesObj);
	FindData->ftLastWriteTime.dwHighDateTime = 0xFFFFFFFF;
	FindData->ftLastWriteTime.dwLowDateTime = 0xFFFFFFFE;
	FindData->nFileSizeLow = PyLong_AsLong(sizeObj);
	strcpy_s(FindData->cFileName, PyUnicode_AsUTF8(filenameObj));

exit:
	
	Py_XDECREF(args);
	Py_XDECREF(result);

	SetLastError(last_error);
	return ret;
}

int __stdcall FsFindClose(HANDLE Hdl)
{
	return 0;
}

int __stdcall FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	PyObject* arg0 = PyUnicode_FromString(RemoteName);
	PyObject* arg1 = PyUnicode_FromString(LocalName);
	PyObject* args = PyTuple_Pack(2, arg0, arg1);
	PyObject* result = PyObject_CallObject(func_get_file, args);

	long size = PyLong_AsLong(result);

	Py_XDECREF(arg0);
	Py_XDECREF(arg1);
	Py_XDECREF(args);
	Py_XDECREF(result);

	return FS_FILE_OK;
}