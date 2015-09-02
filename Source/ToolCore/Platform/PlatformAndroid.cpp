
#include <Atomic/IO/MemoryBuffer.h>
#include <Atomic/IO/FileSystem.h>

#include "../ToolEnvironment.h"
#include "../Subprocess/SubprocessSystem.h"

#include "PlatformAndroid.h"

namespace ToolCore
{

PlatformAndroid::PlatformAndroid(Context* context) : Platform(context)
{

}

PlatformAndroid::~PlatformAndroid()
{

}

BuildBase* PlatformAndroid::NewBuild(Project *project)
{
    return 0;
}

void PlatformAndroid::PrependAndroidCommandArgs(Vector<String> args)
{
#ifdef ATOMIC_PLATFORM_WINDOWS
    // android is a batch file on windows, so have to run with cmd /c
    args.Push("/c");
    args.Push("\"" + GetAndroidCommand() + "\"");
#endif

}

void PlatformAndroid::HandleRefreshAndroidTargetsEvent(StringHash eventType, VariantMap& eventData)
{
    if (eventType == E_SUBPROCESSOUTPUT)
    {
        targetOutput_ += eventData[SubprocessOutput::P_TEXT].GetString();
    }
    else if (eventType == E_SUBPROCESSCOMPLETE)
    {
        refreshAndroidTargetsProcess_ = 0;

        androidTargets_.Clear();

        MemoryBuffer reader(targetOutput_.CString(), targetOutput_.Length() + 1);

        while (!reader.IsEof())
        {
            String line = reader.ReadLine();
            if (line.StartsWith("id:"))
            {
                //id: 33 or "Google Inc.:Google APIs (x86 System Image):19"
                Vector<String> elements = line.Split('\"');
                if (elements.Size() == 2)
                {
                    String api = elements[1];

                    androidTargets_.Push(api);
                }
            }
        }

        SendEvent(E_ANDROIDTARGETSREFRESHED);
    }

}

void PlatformAndroid::RefreshAndroidTargets()
{
    if (refreshAndroidTargetsProcess_.NotNull())
        return;

    SubprocessSystem* subs = GetSubsystem<SubprocessSystem>();

    String androidCommand = GetAndroidCommand();

    Vector<String> args;
    PrependAndroidCommandArgs(args);
    args.Push("list");
    args.Push("targets");

    targetOutput_.Clear();
    refreshAndroidTargetsProcess_ = subs->Launch(androidCommand, args);

    if (refreshAndroidTargetsProcess_.NotNull())
    {

        SubscribeToEvent(refreshAndroidTargetsProcess_, E_SUBPROCESSCOMPLETE, HANDLER(PlatformAndroid, HandleRefreshAndroidTargetsEvent));
        SubscribeToEvent(refreshAndroidTargetsProcess_, E_SUBPROCESSOUTPUT, HANDLER(PlatformAndroid, HandleRefreshAndroidTargetsEvent));


    }

}

String PlatformAndroid::GetAndroidCommand() const
{
    ToolPrefs* prefs = GetSubsystem<ToolEnvironment>()->GetToolPrefs();

    String androidCommand = GetNativePath(prefs->GetAndroidSDKPath());

    if (!androidCommand.Length())
        return String::EMPTY;

#ifdef ATOMIC_PLATFORM_OSX
    //Vector<String> args = String("list targets").Split(' ');
    androidCommand += "/tools/android";
#else

    // android is a batch file on windows, so have to run with cmd /c
    androidCommand += "\\tools\\android.bat";

    //androidCommand = "cmd";
#endif

    return androidCommand;

}


}
