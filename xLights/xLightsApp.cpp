/***************************************************************
 * Name:      xLightsApp.cpp
 * Purpose:   Code for Application Class
 * Author:    Matt Brown (dowdybrown@yahoo.com)
 * Created:   2012-11-03
 * Copyright: Matt Brown ()
 * License:
 **************************************************************/

#ifdef _MSC_VER
//#include <vld.h> // visual leak detector ... https://vld.codeplex.com/
#endif

#include "xLightsApp.h"
//#include "heartbeat.h" //DJ

//(*AppHeaders
#include "xLightsMain.h"
#include <wx/image.h>
//*)

#include <log4cpp/Category.hh>
#include <log4cpp/PropertyConfigurator.hh>
#include <log4cpp/Configurator.hh>


#ifdef LINUX
#include <GL/glut.h>
#endif

#ifndef __WXMSW__
#include <execinfo.h>
#else
#include <wx/textfile.h>
#include <algorithm>
#include <windows.h>
#include <imagehlp.h>

wxString windows_get_stacktrace(void *data)
{
    wxString trace;
    CONTEXT *context = (CONTEXT*)data;
    SymInitialize(GetCurrentProcess(), 0, true);

    STACKFRAME frame = { 0 };

    wxArrayString mapLines;
    wxFileName name = wxStandardPaths::Get().GetExecutablePath();
    name.SetExt("map");
    wxTextFile mapFile(name.GetFullPath());
    if (mapFile.Exists()) {
        mapFile.Open();
        wxString line = mapFile.GetFirstLine();
        while (!mapFile.Eof()) {
            line = mapFile.GetNextLine();
            line.Trim(true).Trim(false);
            if (line.StartsWith("0x")) {
                mapLines.Add(line);
            }
        }
        mapLines.Sort();
    } else {
        trace += name.GetFullPath() + " does not exist\n";
    }

    // setup initial stack frame
    frame.AddrPC.Offset         = context->Eip;
    frame.AddrPC.Mode           = AddrModeFlat;
    frame.AddrStack.Offset      = context->Esp;
    frame.AddrStack.Mode        = AddrModeFlat;
    frame.AddrFrame.Offset      = context->Ebp;
    frame.AddrFrame.Mode        = AddrModeFlat;

    while (StackWalk(IMAGE_FILE_MACHINE_I386 ,
                   GetCurrentProcess(),
                   GetCurrentThread(),
                   &frame,
                   context,
                   0,
                   SymFunctionTableAccess,
                   SymGetModuleBase,
                   0 ) )
    {
        static char symbolBuffer[ sizeof(IMAGEHLP_SYMBOL) + 255 ];
        memset( symbolBuffer , 0 , sizeof(IMAGEHLP_SYMBOL) + 255 );
        IMAGEHLP_SYMBOL * symbol = (IMAGEHLP_SYMBOL*) symbolBuffer;

        // Need to set the first two fields of this symbol before obtaining name info:
        symbol->SizeOfStruct    = sizeof(IMAGEHLP_SYMBOL) + 255;
        symbol->MaxNameLength   = 254;

        // The displacement from the beginning of the symbol is stored here: pretty useless
        unsigned displacement = 0;

        // Get the symbol information from the address of the instruction pointer register:
        if (SymGetSymFromAddr(
                    GetCurrentProcess()     ,   // Process to get symbol information for
                    frame.AddrPC.Offset     ,   // Address to get symbol for: instruction pointer register
                    (DWORD*) & displacement ,   // Displacement from the beginning of the symbol: whats this for ?
                    symbol                      // Where to save the symbol
                )) {
            // Add the name of the function to the function list:
            char buffer[2048]; sprintf( buffer , "0x%08x %s\n" ,  frame.AddrPC.Offset , symbol->Name );
            trace += buffer;
        } else {
            // Print an unknown location:
            // functionNames.push_back("unknown location");
            wxString buffer(wxString::Format("0x%08x" , frame.AddrPC.Offset));
            for (size_t x = 1; x < mapLines.GetCount(); x++) {
                if (wxString(buffer) < mapLines[x]) {
                    buffer += mapLines[x - 1].substr(12).Trim();
                    x = mapLines.GetCount();
                }
            }
            trace += buffer + "\n";
        }
    }

    SymCleanup( GetCurrentProcess() );
    return trace;
}

#endif

void InitialiseLogging(bool fromMain)
{
    static bool loggingInitialised = false;

    if (!loggingInitialised)
    {

#ifdef __WXMSW__
        std::string initFileName = "xlights.windows.properties";
#endif
#ifdef __WXOSX_MAC__
        std::string initFileName = "xlights.mac.properties";
        if (!wxFile::Exists(initFileName)) {
            if (fromMain) {
                return;
            } else if (wxFile::Exists(wxStandardPaths::Get().GetResourcesDir() + "/xlights.mac.properties")) {
                initFileName = wxStandardPaths::Get().GetResourcesDir() + "/xlights.mac.properties";
            }
        }
        loggingInitialised = true;
        
#endif
#ifdef __LINUX__
        std::string initFileName = "/usr/share/xLights/xlights.linux.properties";
#endif

        if (!wxFile::Exists(initFileName))
        {
#ifdef _MSC_VER
            // the app is not initialized so GUI is not available and no event loop.
            wxMessageBox(initFileName + " not found in " + wxGetCwd() + ". Logging disabled.");
#endif
        }
        else
        {
            try
            {
                log4cpp::PropertyConfigurator::configure(initFileName);
            }
            catch (log4cpp::ConfigureFailure& e) {
                // ignore config failure ... but logging wont work
                printf("Log issue:  %s\n", e.what());
            }
            catch (const std::exception& ex) {
                printf("Log issue: %s\n", ex.what());
            }
        }
    }
}

std::string DecodeOS(wxOperatingSystemId o)
{
    switch (o)
    {
    case wxOS_UNKNOWN:
        return "Call get get operating system failed.";
    case wxOS_MAC_OS:
        return "Apple Mac OS 8 / 9 / X with Mac paths.";
    case wxOS_MAC_OSX_DARWIN:
        return "Apple OS X with Unix paths.";
    case wxOS_MAC:
        return "An Apple Mac of some type.";
    case wxOS_WINDOWS_NT:
        return "Windows NT family(XP / Vista / 7 / 8 / 10).";
    case wxOS_WINDOWS:
        return "A Windows system of some type.";
    case wxOS_UNIX_LINUX:
        return "Linux.";
    case wxOS_UNIX_FREEBSD:
        return "FreeBSD.";
    case wxOS_UNIX_OPENBSD:
        return "OpenBSD.";
    case wxOS_UNIX_NETBSD:
        return "NetBSD.";
    case wxOS_UNIX_SOLARIS:
        return "Solaris.";
    case wxOS_UNIX_AIX:
        return "AIX.";
    case wxOS_UNIX_HPUX:
        return "HP / UX.";
    case wxOS_UNIX:
        return "Some flavour of Unix.";
    default:
        break;
    }

    return "Unknown Operating System.";
}

void DumpConfig()
{
    static log4cpp::Category &logger_base = log4cpp::Category::getInstance(std::string("log_base"));
    logger_base.info("Version: " + std::string(xlights_version_string.c_str()));
    logger_base.info("Build Date: " + std::string(xlights_build_date.c_str()));
    logger_base.info("Machine configuration:");
    wxMemorySize s = wxGetFreeMemory();
    if (s != -1)
    {
        logger_base.info("  Free Memory:%ld.", s);
    }
    logger_base.info("  Current directory: " + std::string(wxGetCwd().c_str()));
    logger_base.info("  Machine name: " + std::string(wxGetHostName().c_str()));
    logger_base.info("  OS: " + std::string(wxGetOsDescription().c_str()));
    int verMaj = -1;
    int verMin = -1;
    wxOperatingSystemId o = wxGetOsVersion(&verMaj, &verMin);
    logger_base.info("  OS: %s %d.%d.%d", (const char *)DecodeOS(o).c_str(), verMaj, verMin);
    if (wxIsPlatform64Bit())
    {
        logger_base.info("      64 bit");
    }
    else
    {
        logger_base.info("      NOT 64 bit");
    }
    if (wxIsPlatformLittleEndian())
    {
        logger_base.info("      Little Endian");
    }
    else
    {
        logger_base.info("      Big Endian");
    }
#ifdef LINUX
    wxLinuxDistributionInfo l = wxGetLinuxDistributionInfo();
    logger_base.info("  " + std::string(l.Id.c_str()) \
        + " " + std::string(l.Release.c_str()) \
        + " " + std::string(l.CodeName.c_str()) \
        + " " + std::string(l.Description.c_str()));
#endif
}

#ifdef LINUX
    #include <X11/Xlib.h>
#endif // LINUX
//IMPLEMENT_APP(xLightsApp)
int main(int argc, char **argv)
{
    InitialiseLogging(true);
    // Dan/Chris ... if you get an exception here the most likely reason is the line
    // appender.A1.fileName= in the xlights.xxx.properties file
    // it seems to need to be a folder that exists ... ideally it would create it but it doesnt seem to
    // it needs to be:
    //     a folder the user can write to
    //     predictable ... as we want the handleCrash function to be able to locate the file to include it in the crash
    static log4cpp::Category &logger_base = log4cpp::Category::getInstance(std::string("log_base"));
    logger_base.info("******* XLights main function executing.");

#ifdef LINUX
    XInitThreads();
#endif
    wxDISABLE_DEBUG_SUPPORT();

    logger_base.info("Main: Starting wxWidgets ...");
    int rc =  wxEntry(argc, argv);
    logger_base.info("Main: wxWidgets exited with rc=" + wxString::Format("%d", rc));
    return rc;
}

#ifdef _MSC_VER
IMPLEMENT_APP(xLightsApp);
#else
wxIMPLEMENT_APP_NO_MAIN(xLightsApp);
#endif

#include <wx/debugrpt.h>

xLightsFrame *topFrame = NULL;
void handleCrash(void *data) {
    static log4cpp::Category &logger_base = log4cpp::Category::getInstance(std::string("log_base"));
    logger_base.crit("Crash handler called.");
	wxDebugReportCompress *report = new wxDebugReportCompress();
    report->SetCompressedFileDirectory(topFrame->CurrentDir);
    report->AddAll(wxDebugReport::Context_Exception);
    report->AddAll(wxDebugReport::Context_Current);
    
    wxFileName fn(topFrame->CurrentDir, "xlights_networks.xml");
    if (fn.Exists()) {
        report->AddFile(fn.GetFullPath(), "xlights_networks.xml");
    }
    if (wxFileName(topFrame->CurrentDir, "xlights_rgbeffects.xml").Exists()) {
        report->AddFile(wxFileName(topFrame->CurrentDir, "xlights_rgbeffects.xml").GetFullPath(), "xlights_rgbeffects.xml");
    }
    if (topFrame->UnsavedRgbEffectsChanges &&  wxFileName(topFrame->CurrentDir, "xlights_rgbeffects.xbkp").Exists()) {
        report->AddFile(wxFileName(topFrame->CurrentDir, "xlights_rgbeffects.xbkp").GetFullPath(), "xlights_rgbeffects.xbkp");
    }

    wxString dir;
#ifdef __WXMSW__
    wxGetEnv("APPDATA", &dir);
    std::string filename = std::string(dir.c_str()) + "/xLights_l4cpp.log";
#endif
#ifdef __WXOSX_MAC__
    wxFileName home;
    home.AssignHomeDir();
    dir = home.GetFullPath();
    std::string filename = std::string(dir.c_str()) + "/Library/Logs/xLights_l4cpp.log";
#endif
#ifdef __LINUX__
    std::string filename = "/tmp/xLights_l4cpp.log";
#endif
    
    if (wxFile::Exists(filename))
    {
        report->AddFile(filename, "xLights_l4cpp.log");
    }
    else if (wxFile::Exists(wxFileName(topFrame->CurrentDir, "xLights_l4cpp.log").GetFullPath()))
    {
        report->AddFile(wxFileName(topFrame->CurrentDir, "xLights_l4cpp.log").GetFullPath(), "xLights_l4cpp.log");
    }
    else if (wxFile::Exists(wxFileName(wxGetCwd(), "xLights_l4cpp.log").GetFullPath()))
    {
        report->AddFile(wxFileName(wxGetCwd(), "xLights_l4cpp.log").GetFullPath(), "xLights_l4cpp.log");
    }

    if (topFrame->GetSeqXmlFileName() != "") {
        wxFileName fn(topFrame->GetSeqXmlFileName());
        if (fn.Exists() && !fn.IsDir()) {
            report->AddFile(topFrame->GetSeqXmlFileName(), fn.GetName());
            wxFileName fnb(fn.GetPath() + "/" + fn.GetName() + ".xbkp");
            if (fnb.Exists())
            {
                report->AddFile(fnb.GetFullPath(), fnb.GetName());
            }
        }
        else
        {
            wxFileName fnb(topFrame->CurrentDir + "/" + "__.xbkp");
            if (fnb.Exists())
            {
                report->AddFile(fnb.GetFullPath(), fnb.GetName());
            }
        }
    }
    else
    {
        wxFileName fnb(topFrame->CurrentDir + "/" + "__.xbkp");
        if (fnb.Exists())
        {
            report->AddFile(fnb.GetFullPath(), fnb.GetName());
        }
    }
    wxString trace = wxString::Format("xLights version %s\n\n", xlights_version_string);

#ifndef __WXMSW__
    void* callstack[128];
    int i, frames = backtrace(callstack, 128);
    char** strs = backtrace_symbols(callstack, frames);
    for (i = 0; i < frames; ++i) {
        trace += strs[i];
        trace += "\n";
    }
    free(strs);
#else
    trace += windows_get_stacktrace(data);
#endif

    int id = (int)wxThread::GetCurrentId();
    trace += wxString::Format("\nCrashed thread id: %X\n", id);
#ifndef LINUX
    trace += topFrame->GetThreadStatusReport();
#endif // LINUX

	logger_base.crit(trace);

    report->AddText("backtrace.txt", trace, "Backtrace");
    if (!wxThread::IsMain() && topFrame != nullptr) {
        topFrame->CallAfter(&xLightsFrame::CreateDebugReport, report);
        wxSleep(600000);
    } else {
        topFrame->CreateDebugReport(report);
    }
}
wxString xLightsFrame::GetThreadStatusReport() {
    return jobPool.GetThreadStatus();
}

void xLightsFrame::CreateDebugReport(wxDebugReportCompress *report) {
    if (wxDebugReportPreviewStd().Show(*report)) {
        report->Process();
        SendReport("crashUpload", *report);
        wxMessageBox("Crash report saved to " + report->GetCompressedFileName());
    }
    static log4cpp::Category &logger_base = log4cpp::Category::getInstance(std::string("log_base"));
    logger_base.crit("Exiting after creating debug report: " + report->GetCompressedFileName());
	delete report;
	exit(1);
}


#if !(wxUSE_ON_FATAL_EXCEPTION)
#include <windows.h>
//MinGW needs to do this manually
LONG WINAPI windows_exception_handler(EXCEPTION_POINTERS * ExceptionInfo)
{
    handleCrash(ExceptionInfo->ContextRecord);
}
#endif


bool xLightsApp::OnInit()
{
    InitialiseLogging(false);
    static log4cpp::Category &logger_base = log4cpp::Category::getInstance(std::string("log_base"));
    logger_base.info("******* OnInit: XLights started.");
    DumpConfig();

#ifdef _MSC_VER
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); 
#endif

#if wxUSE_ON_FATAL_EXCEPTION
    wxHandleFatalExceptions();
#else
    SetUnhandledExceptionFilter(windows_exception_handler);
#endif

//    heartbeat("init", true); //tell monitor active now -DJ
//check for options on command line: -DJ
//TODO: maybe use wxCmdLineParser instead?
//do this before instantiating xLightsFrame so it can use info gathered here
    wxString unrecog, info;

    static const wxCmdLineEntryDesc cmdLineDesc [] =
    {
        { wxCMD_LINE_SWITCH, "h", "help", "displays help on the command line parameters", wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
        { wxCMD_LINE_SWITCH, "n", "noauto", "enable auto-run prompt"},
        { wxCMD_LINE_SWITCH, "d", "debug", "enable debug mode"},
        { wxCMD_LINE_SWITCH, "r", "render", "render files and exit"},
        { wxCMD_LINE_OPTION, "m", "media", "specify media directory"},
        { wxCMD_LINE_OPTION, "s", "show", "specify show directory"},
        { wxCMD_LINE_PARAM, "", "", "sequence file", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE},
        { wxCMD_LINE_NONE }
    };

    wxCmdLineParser parser(cmdLineDesc, argc, argv);
    switch (parser.Parse()) {
    case -1:
        // help was given
        return false;
    case 0:
        WantDebug = parser.Found("d");
        if (WantDebug) {
            info += _("Debug is ON\n");
        }
        RunPrompt = parser.Found("n");
        if (RunPrompt) {
            info += _("Auto-run prompt is ON\n");
        }
        if (parser.Found("s", &showDir)) {
            info += _("Setting show directory to ") + showDir + "\n";
        }
        if (parser.Found("m", &mediaDir)) {
            info += _("Setting media directory to ") + mediaDir + "\n";
        } else if (!showDir.IsNull()) {
            mediaDir = showDir;
        }
        for (int x = 0; x < parser.GetParamCount(); x++) {
            wxString sequenceFile = parser.GetParam(x);
            if (x == 0) {
                info += _("Loading sequence ") + sequenceFile + "\n";
            }
            if (showDir.IsNull()) {
                showDir=wxPathOnly(sequenceFile);
            }
            sequenceFiles.push_back(sequenceFile);
        }
        if (!parser.Found("r") && !info.empty()) wxMessageBox(info, _("Command Line Options")); //give positive feedback*/
        break;
    default:
        wxMessageBox(_("Unrecognized command line parameters"),_("Command Line Error"));
        return false;
    }

    //(*AppInitialize
    bool wxsOK = true;
    wxInitAllImageHandlers();
    if ( wxsOK )
    {
    	xLightsFrame* Frame = new xLightsFrame(0);
    	Frame->Show();
    	SetTopWindow(Frame);
    }
    //*)
    topFrame = (xLightsFrame* )GetTopWindow();

    if (parser.Found("r")) {
        topFrame->CallAfter(&xLightsFrame::OpenRenderAndSaveSequences, sequenceFiles);
    }
    
    wxImage::AddHandler(new wxPNGHandler);
    #ifdef LINUX
        glutInit(&(wxApp::argc), wxApp::argv);
    #endif

    logger_base.info("XLightsApp OnInit Done.");

    return wxsOK;
}


void xLightsApp::OnFatalException() {
    handleCrash(NULL);
}


//global flags from command line:
bool xLightsApp::WantDebug = false;
bool xLightsApp::RunPrompt = false; //prompt before running schedule (allows override) -DJ
wxString xLightsApp::DebugPath;
wxString xLightsApp::mediaDir;
wxString xLightsApp::showDir;
wxArrayString xLightsApp::sequenceFiles;
