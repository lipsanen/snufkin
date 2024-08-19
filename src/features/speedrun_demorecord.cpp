#include "feature.hpp"
#include "Color.h"
#include "cdll_int.h"
#include "eiface.h"
#include "filesystem.h"
#include "../plugin.h"
#include "interfaces.hpp"
#include "signals.hpp"

enum RecordingMode
{
	DEMREC_DISABLED,

	// standard speedrun (deaths/reloads/etc)
	DEMREC_STANDARD
};

#define DemRecMsg(color, msg, ...) (ConColorMsg(0, color, "[Speedrun] " msg, __VA_ARGS__))
#define DemRecMsgSuccess(msg, ...) (DemRecMsg(Color(0, 255, 0, 255), msg, __VA_ARGS__))
#define DemRecMsgWarning(msg, ...) (DemRecMsg(Color(255, 87, 87, 255), msg, __VA_ARGS__))
#define DemRecMsgInfo(msg, ...) (DemRecMsg(Color(255, 165, 0, 255), msg, __VA_ARGS__))

#define CMD_SIZE 320
#define SESSION_DIR_SIZE 320
#define DEMO_NAME_SIZE 320
#define DEMO_LIST_SIZE 8

static RecordingMode recordMode = DEMREC_DISABLED;
static int retries = 0;
static std::string lastMapName;
static std::string currentMapName;
static char sessionDir[SESSION_DIR_SIZE] = {};
static char currentDemoName[DEMO_NAME_SIZE] = {};


// Function protos
static void findFirstMap();
static void createDirIfNonExistant(const char* modRelativePath);
static void GetDateAndTime(struct tm& ltime);
static void ConvertTimeToLocalTime(const time_t& t, struct tm& ltime);
static int demoExists(const char* curMap);

static ConVar sn_run_dir("sn_run_dir",
    "./",
    FCVAR_ARCHIVE | FCVAR_DONTRECORD,
    "Sets the directory for demos to record to.");
static ConVar sn_run_map("sn_run_map",
    "",
    FCVAR_ARCHIVE | FCVAR_DONTRECORD,
    "Sets the first map in the game which will be started when sn_run_start is executed.");
static ConVar sn_run_save(
    "sn_run_save",
    "",
    FCVAR_ARCHIVE | FCVAR_DONTRECORD,
    "If empty, sn_run_start will start using map specified in sn_run_map. If save is specified, sn_run_start "
    "will start using the save instead of a map. If the specified save does not exist, the speedrun will start using "
    "specified map. The save specified MUST BE in the SAVE folder!!");

// Ported demo recording code from Maxxuss' plugin
class SpeedrunDemoRecordFeature : public FeatureWrapper<SpeedrunDemoRecordFeature>
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;
};

static SpeedrunDemoRecordFeature feat_speedrun_demorecord;

//-----------------------------------------------------------------------------
// Purpose: Changes all '/' or '\' characters into separator
// Input  : *pname - 
//			separator - 
//-----------------------------------------------------------------------------
static void V_FixSlashes(char* pname, char separator /* = CORRECT_PATH_SEPARATOR */)
{
    while (*pname)
    {
        if (*pname == INCORRECT_PATH_SEPARATOR || *pname == CORRECT_PATH_SEPARATOR)
        {
            *pname = separator;
        }
        pname++;
    }
}

static bool FStrEq(const char* sz1, const char* sz2)
{
    return (Q_stricmp(sz1, sz2) == 0);
}

static void LevelInit(const char* pMapName)
{
	if (recordMode != DEMREC_DISABLED)
	{
		std::string str(pMapName);
		currentMapName = str;
	}
}

static void LevelShutdown()
{
	if (recordMode == DEMREC_STANDARD)
	{
		if (interfaces::g_pEngineClient->IsPlayingDemo() == false && interfaces::g_pEngineClient->IsRecordingDemo() == true)
		{
            interfaces::g_pEngineClient->ClientCmd("stop");
		}
	}
}

static void ClientConnect(bool* bAllowConnect, edict_t* pEntity, const char* pszName, const char* pszAddress, char* reject, int maxrejectlen)
{
    if (recordMode != DEMREC_DISABLED)
    {
        if (interfaces::g_pEngineClient->IsPlayingDemo() == false)
        {
            std::string& curMap = currentMapName;

            if (curMap.find("background") == -1)
            {
                char command[CMD_SIZE] = {};

                if (lastMapName == curMap && recordMode == DEMREC_STANDARD)
                {
                    retries++;
                    snprintf(currentDemoName, DEMO_NAME_SIZE, "%s_%d", curMap.c_str(), retries);
                }
                else
                {
                    int storedretries = demoExists(curMap.c_str());

                    if (storedretries != 0)
                    {
                        retries = storedretries;
                        snprintf(currentDemoName, DEMO_NAME_SIZE, "%s_%d", curMap.c_str(), retries);
                    }
                    else
                    {
                        retries = 0;
                        snprintf(currentDemoName, DEMO_NAME_SIZE, "%s", curMap.c_str());
                    }

                    lastMapName = curMap;
                }

                // Always ensure path exists before attempting to record
                // Otherwise, record command will fail!
                createDirIfNonExistant(sessionDir);
                snprintf(command, sizeof(command) / sizeof(char), "record %s%s\n", sessionDir, currentDemoName);
                interfaces::g_pEngineClient->ClientCmd(command);
            }
        }
    }
}

static int demoExists(const char* curMap)
{
    // 1) Search for demos with map name in filename
    // 2) If none found, return 0
    // 3) If found, check for an _#, if none return a 1, else if # > 1, add 1 to that number and return #+1

    FileFindHandle_t findHandle;
    int retriesTmp = 0;

    char path[MAX_PATH] = {};
    snprintf(path, sizeof(path) / sizeof(char), "%s%s*", sessionDir, curMap);

    const char* pFilename = interfaces::g_pFileSystem->FindFirstEx(path, "MOD", &findHandle);
    while (pFilename != NULL)
    {
        pFilename = interfaces::g_pFileSystem->FindNext(findHandle);
        retriesTmp++;
    }
    interfaces::g_pFileSystem->FindClose(findHandle);

    return retriesTmp;
}

//---------------------------------------------------------------------------------
// Purpose: checks if chosen directory exists, if not attempts to create folder
//---------------------------------------------------------------------------------
static void createDirIfNonExistant(const char* modRelativePath)
{
    if (!modRelativePath)
        return;

    if (!interfaces::g_pFileSystem->IsDirectory(modRelativePath, "MOD"))
    {
        char path[MAX_PATH] = {};
        strncpy(path, modRelativePath, sizeof(path) / sizeof(char));
        Q_FixSlashes(path);
        interfaces::g_pFileSystem->CreateDirHierarchy(path, "DEFAULT_WRITE_PATH");
    }
}

//---------------------------------------------------------------------------------
// Purpose: Tries to find first map in maplist.txt that doesnt have the word "background" in it.
// Should only be ran first time? Returns char array
//---------------------------------------------------------------------------------
static void findFirstMap()
{
    // check if it is empty true: stop, false: cont
    FileHandle_t firstMapConfig = interfaces::g_pFileSystem->Open("cfg/chapter1.cfg", "r", "MOD");

    // check if it exists is there: cont, dne: stop
    if (firstMapConfig)
    {
        int file_len = interfaces::g_pFileSystem->Size(firstMapConfig);
        char* firstMap = new char[file_len + 1];

        interfaces::g_pFileSystem->ReadLine(firstMap, file_len + 1, firstMapConfig);
        firstMap[file_len] = '\0';
        interfaces::g_pFileSystem->Close(firstMapConfig);

        *std::remove(firstMap, firstMap + strlen(firstMap), '\n') = '\0'; // Remove new lines
        *std::remove(firstMap, firstMap + strlen(firstMap), '\r') = '\0'; // Remove returns

        std::string mapName =
            std::string(firstMap).substr(std::string(firstMap).find("map ") + 4); // get rid of words "map "

        // Will this work? Might have to be FRstrEq(sn_run_map.GetString(), NULL) == true
        if (FStrEq(sn_run_map.GetString(), ""))
        {
            sn_run_map.SetValue(mapName.c_str());
        }

        delete[] firstMap;
    }
}

static void GetDateAndTime(struct tm& ltime)
{
    // get the time as an int64
    time_t t = time(NULL);

    ConvertTimeToLocalTime(t, ltime);
}

static void ConvertTimeToLocalTime(const time_t& t, struct tm& ltime)
{
    // convert it to a struct of time values
    ltime = *localtime(&t);

    // normalize the year and month
    ltime.tm_year = ltime.tm_year + 1900;
    ltime.tm_mon = ltime.tm_mon + 1;
}

bool SpeedrunDemoRecordFeature::ShouldLoadFeature()
{
	return true;
}

void SpeedrunDemoRecordFeature::InitHooks() {}

void SpeedrunDemoRecordFeature::UnloadFeature() {}


CON_COMMAND_F(sn_run_start, "starts run", FCVAR_DONTRECORD)
{
    // No map or save set? Throw error
    if (FStrEq(sn_run_map.GetString(), "") && FStrEq(sn_run_save.GetString(), ""))
    {
        Warning("Please set a map with sn_run_map or save with sn_run_save first.\n");
    }
    else
    {
        // Let the user know
        DemRecMsgSuccess("Speedrun starting now...\n");

        // Init standard recording mode
        recordMode = DEMREC_STANDARD;
        lastMapName = "";

        // Get current time
        struct tm ltime;
        ConvertTimeToLocalTime(time(NULL), ltime);

        // Parse time
        char tmpDir[32] = {};
        snprintf(tmpDir,
            sizeof(tmpDir) / sizeof(char),
            "%04i.%02i.%02i-%02i.%02i.%02i",
            ltime.tm_year,
            ltime.tm_mon,
            ltime.tm_mday,
            ltime.tm_hour,
            ltime.tm_min,
            ltime.tm_sec);

        // Create dir
        snprintf(sessionDir, SESSION_DIR_SIZE, "%s%s\\", sn_run_dir.GetString(), tmpDir);
        Q_FixSlashes(sessionDir);
        interfaces::g_pFileSystem->CreateDirHierarchy(sessionDir, "DEFAULT_WRITE_PATH");

        // Store dir in a resume txt file incase of crash
        int sessionDirLen = Q_strlen(sessionDir);

        // Path to default directory
        char path[MAX_PATH] = {};
        snprintf(path,
            sizeof(path) / sizeof(char),
            "%sspeedrun_democrecord_resume_info.txt",
            sn_run_dir.GetString());

        // Print to file, let us know it was successful and play a silly sound :P
        interfaces::g_pFileSystem->AsyncWrite(path, sessionDir, sessionDirLen, false);
        interfaces::g_pFileSystem->AsyncFinishAllWrites();

        // Check to see if a save is specified in sn_run_save, if not use specified map in sn_run_map
        // Make sure save exisits (only checking in SAVE folder), if none load specified map.
        char tmpSav[MAX_PATH] = {};
        snprintf(tmpSav, sizeof(tmpSav) / sizeof(char), ".\\SAVE\\%s.sav", sn_run_save.GetString());
        Q_FixSlashes(tmpSav);

        char command[CMD_SIZE] = {};
        if (interfaces::g_pFileSystem->FileExists(tmpSav, "MOD"))
        {
            // Load save else...
            Msg("Loading from save...\n");
            snprintf(command, sizeof(command) / sizeof(char), "load %s.sav\n", sn_run_save.GetString());
            interfaces::g_pEngineClient->ClientCmd(command);
        }
        else
        {
            // Start run
            snprintf(command, sizeof(command) / sizeof(char), "map \"%s\"\n", sn_run_map.GetString());
            interfaces::g_pEngine->ServerCommand(command);
        }
    }
}

CON_COMMAND_F(sn_run_resume, "resume a speedrun after a crash", FCVAR_DONTRECORD)
{
    if (recordMode == DEMREC_DISABLED)
    {
        // Path to default directory
        char path[MAX_PATH] = {};
        snprintf(path,
            sizeof(path) / sizeof(char),
            "%sspeedrun_democrecord_resume_info.txt",
            sn_run_dir.GetString());
        FileHandle_t resumeFile = interfaces::g_pFileSystem->Open(path, "r", "MOD");
        if (resumeFile)
        {
            int file_len = interfaces::g_pFileSystem->Size(resumeFile);
            char* contents = new char[file_len + 1];

            interfaces::g_pFileSystem->ReadLine(contents, file_len + 1, resumeFile);
            contents[file_len] = '\0';
            interfaces::g_pFileSystem->Close(resumeFile);
            snprintf(sessionDir, SESSION_DIR_SIZE, "%s", contents);

            // Init standard recording mode
            recordMode = DEMREC_STANDARD;
            lastMapName = "";

            DemRecMsgSuccess("Past speedrun successfully loaded, please load your last save now.\n");
            delete[] contents;
        }
        else
        {
            Warning("Error opening speedrun_democrecord_resume_info.txt, cannot resume speedrun!\n");
        }
    }
    else
    {
        Warning("Please stop all other speedruns with sn_run_stop before resuming a speedrun.\n");
    }
}

CON_COMMAND_F(sn_run_stop, "stops run", FCVAR_DONTRECORD)
{
    if (recordMode == DEMREC_DISABLED)
    {
        DemRecMsgWarning("No speedrun in progress.\n");
    }
    else
    {
        DemRecMsgSuccess("Speedrun will STOP now...\n");

        lastMapName = currentMapName;

        if (interfaces::g_pEngineClient->IsRecordingDemo() == true)
        {
            interfaces::g_pEngineClient->ClientCmd("stop");
        }

        if (recordMode == DEMREC_STANDARD)
        {
            // Path to default directory
            char path[MAX_PATH] = {};
            snprintf(path,
                sizeof(path) / sizeof(char),
                "%sspeedrun_democrecord_resume_info.txt",
                sn_run_dir.GetString());

            // Delete resume file
            interfaces::g_pFileSystem->RemoveFile(path, "MOD");
        }

        recordMode = DEMREC_DISABLED;
    }
}


void SpeedrunDemoRecordFeature::LoadFeature() {
    recordMode = DEMREC_DISABLED;
    retries = 0;
    lastMapName = "UNKNOWN_MAP";
    currentMapName = "UNKNOWN_MAP";

    findFirstMap();

    InitConcommandBase(sn_run_start_command);
    InitConcommandBase(sn_run_resume_command);
    InitConcommandBase(sn_run_stop_command);
    InitConcommandBase(sn_run_dir);
    InitConcommandBase(sn_run_map);
    InitConcommandBase(sn_run_save);

    signals::LevelInit.Connect(LevelInit);
    signals::LevelShutdown.Connect(LevelShutdown);
    signals::ClientConnect.Connect(ClientConnect);
}
