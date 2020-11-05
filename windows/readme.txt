This is the README file of Metax project for Windows by Leviathan CJSC.

CONTENTS
        1. AUTHOR
        2. INTRODUCTION
        3. PREREQUISITES
        4. INSTALL
        5. DIRECTORY STRUCTURE
        6. USAGE
        7. Other

1. AUTHOR
Leviathan CJSC
E-mail: info@leviathan.am
Tel: +1-408-454-6172
     +49-893-8157-1771
     +374-10-248411
Fax: +374-10-242919
ggg.leviathan.am

Project Maintainer
	Leviathan CJSC
	info@leviathan.am

2. INTRODUCTION

3. PREREQUISITES
The following third party tools and libraries should be installed on the
Windows platform before the compilation.

Tools:
Visual Studio 2015 or 2017 or 2019

Libraries:
Poco C++ lib
If need to make static builds type as follows, e.g. with VS2015:
	"buildwin 140 build static_mt release Win32 nosamples notests msbuild"
If need only for debugging build poco by default:
        "buildwin 140"

4. BUILD
Change directory into one of the windows_x folder to build with a specific
version of VS build tools:
140 - VS2015
141 - VS2017
142 - VS2019

The following command builds the current solution:
	run [Action] [Config] [Platform]
Where:
	Action: building type, can be set Build/Rebuild/Clean
		default is Build
	Config: built type, can be set Debug/Release (Release to built static)
		default is Debug.
	Platform: platform processor architecture, can be set x86/x64
		default is x86.

Note to build Metax a specified architecture you need to have OpenSSL and
Poco libraries build for that architecture

5. DIRECTORY STRUCTURE
Each of the windows_x directory has the following structure
windows.sln     - Top solution file
[metax\,platform\,platform_test\,metax_web_api\] -
   the Project Folder contains project file with specific configs
run.cmd         - top level batch file to run build/rebuild/clean the solution
[windows.sdf, xxx.vcxproj.filters]    - See https://msdn.microsoft.com/en-us/library/hx0cxhaw.aspx

6. USAGE
To build by
- command line
    If built successfully run the output file bin\<Debug/Release>\xxx.exe
    Building logs are placed in projects Debug/Release folders.
    Top level runable exe is: metax_web_api.exe
    For more information about building tools type "msbuild /help".

- Visual Studio User Interface
    Double click to windows.sln file and run "Start Without Debugging".

7. OTHER
- Make sure POCO_BASE environment variable is defined and points to a folder
  where Poco library is located:
    e.g. if Poco location is "C:\Poco-1.8.1":
    open windows "Edit environment variables",
    edit as follows:
          Variable Name - POCO_BASE
          Variable Value - C:\Poco-1.6.1
    then save, close, logout and login.
    Or set by command line
          SET POCO_BASE=[absolute path of built Poco root folder]

    Check if it presents by command:
          echo %POCO_BASE%

- Do the same for with OpenSSL library by defining the following environment
  variables:
  OPENSSL_BASE_STATIC or OPENSSL_BASE

// vim:et:tabstop=8:shiftwidth=8:smartindent:textwidth=76:
