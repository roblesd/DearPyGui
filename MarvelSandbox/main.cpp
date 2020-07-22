#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "mvStdOutput.h"
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <CLI11.hpp>

namespace fs = std::filesystem;
using namespace Marvel;

int main(int argc, char* argv[])
{
	wchar_t* program;
	bool errorMode = false;
	PyObject* m = nullptr;
	std::string addedPath;
	std::string AppName = "";
	std::string PathName = "";
	std::string LibraryPath = "";
	std::string theme = "Dark";
	bool documentation = false; // starts application with the documentation window shown
	bool editorMode = false; // starts application in editor mode
	bool ignoreConfig = false;

	CLI::App app{ "Marvel Sandbox" };
	app.allow_windows_style_options();

	// options
	app.add_option("-a, --app", AppName, "Name of the python file (without extension)");
	app.add_option("-p, --path", PathName, "Path to app file (default is location of MarvelSandbox.exe)");
	app.add_option("-l, --libs", LibraryPath, "Path to 3rd party python libraries");

	// flags
	app.add_flag("-d, --documentation", documentation, "Sets MarvelSandbox to Documentation Mode");
	app.add_flag("-e, --editor", editorMode, "Sets MarvelSandbox to Editor Mode");
	app.add_flag("-n, --noconfig", ignoreConfig, "Ignores config file");

	CLI11_PARSE(app, argc, argv);

	// no commandline arguments, so use config file
	if (!ignoreConfig)
	{
		std::ifstream stream("marvel_config.json");
		if (stream.good())
		{
			nlohmann::json j = nlohmann::json::parse(stream);

			if (j.contains("App") && AppName.empty())
			{
				AppName = j["App"];
			}
			else if (AppName.empty())
				AppName = "Demo";

			if (j.contains("Mode"))
			{

				std::string mode = j["Mode"];
				if (mode == "Documentation")
					documentation = true;
				else if (mode == "Editor")
					editorMode = true;
			}

			if (j.contains("Theme"))
				theme = j["Theme"];

			if (j.contains("Path"))
				PathName = PathName + std::string(j["Path"]);

			if (j.contains("PythonLibs"))
				LibraryPath = j["PythonLibs"];
		}
	}



#ifdef MV_RELEASE
	HWND hWnd = GetConsoleWindow();
	ShowWindow(hWnd, SW_HIDE);
#else
	HWND hWnd = GetConsoleWindow();
	ShowWindow(hWnd, SW_SHOW);
#endif

	// handle paths
	fs::path p = fs::path(argv[0]);
	p.replace_extension(" ");
	auto dependencies = p.parent_path().string() + "/Dependencies;";
	auto excpath = p.parent_path().string() + ";";

	addedPath = PathName + "\\";

	PathName = excpath + dependencies + PathName + ";Dependencies/python38.zip;" + LibraryPath + ";";

	program = Py_DecodeLocale(argv[0], NULL);
	if (program == NULL) {
		fprintf(stderr, "Fatal error: cannot decode argv[0]\n");
		exit(1);
	}
	Py_SetProgramName(program);  /* optional but recommended */

	// initialize python
	// add our custom module
	//PyImport_AppendInittab("sandboxout", &PyInit_embOut);

	// set path and start the interpreter
	wchar_t* deco = Py_DecodeLocale(PathName.c_str(), nullptr);
	Py_SetPath(deco);
	Py_NoSiteFlag = 1; // this must be set to 1
	Py_DontWriteBytecodeFlag = 1;

	Py_Initialize();
	if (!Py_IsInitialized())
	{
		printf("Error initializing Python interpreter\n");
		return 1;
	}

	// import our custom module to capture stdout/stderr
	//m = PyImport_ImportModule("sandboxout");
	//PySys_SetObject("stdout", m);
	//PySys_SetObject("stderr", m);

	// get module
	std::string themeCommand = "from marvel import *\nset_theme(\"" + theme + "\")";
	PyRun_SimpleString(themeCommand.c_str());

	if (editorMode)
	{
		std::string command = "start_marvel_editor(r\"" + std::string(argv[0]) + "\")";
		PyRun_SimpleString(command.c_str());
		return 0;
	}

	if (documentation)
	{
		PyRun_SimpleString("start_marvel_docs()");
		return 0;
	}

	PyObject* pModule = PyImport_ImportModule(AppName.c_str()); // new reference

	// check if error occurred
	if (!PyErr_Occurred() && pModule != nullptr)
	{
		Py_XDECREF(pModule);
		return 0;
	}

	else return 1;
	
}