// Task Scheduler logon registration (interactive token, limited rights).

#include "App/AutoStartTaskScheduler.h"

#include "App/AutoStartRegistration.h"
#include "App/AutoStartWinUtil.h"

#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <taskschd.h>
#include <comdef.h>
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsuppw.lib")
#endif

#ifdef _WIN32
namespace
{
HRESULT connectTaskService(ITaskService** serviceOut)
{
    ITaskService* service = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_TaskScheduler,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITaskService,
        reinterpret_cast<void**>(&service));
    if (FAILED(hr))
    {
        return hr;
    }
    hr = service->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr))
    {
        service->Release();
        return hr;
    }
    *serviceOut = service;
    return S_OK;
}

bool configureTaskSettings(ITaskDefinition* definition, std::string& errorOut)
{
    ITaskSettings* settings = nullptr;
    HRESULT hr = definition->get_Settings(&settings);
    if (FAILED(hr) || settings == nullptr)
    {
        errorOut = autoStartFormatHresult("get_Settings failed", hr);
        return false;
    }
    hr = settings->put_StartWhenAvailable(VARIANT_TRUE);
    if (SUCCEEDED(hr))
    {
        hr = settings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
    }
    if (SUCCEEDED(hr))
    {
        hr = settings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
    }
    if (SUCCEEDED(hr))
    {
        hr = settings->put_ExecutionTimeLimit(_bstr_t(L"PT0S"));
    }
    if (SUCCEEDED(hr))
    {
        hr = settings->put_MultipleInstances(TASK_INSTANCES_IGNORE_NEW);
    }
    settings->Release();
    if (FAILED(hr))
    {
        errorOut = autoStartFormatHresult("settings setup failed", hr);
        return false;
    }
    return true;
}

bool configurePrincipal(ITaskDefinition* definition, std::string& errorOut)
{
    IPrincipal* principal = nullptr;
    HRESULT hr = definition->get_Principal(&principal);
    if (FAILED(hr) || principal == nullptr)
    {
        errorOut = autoStartFormatHresult("get_Principal failed", hr);
        return false;
    }
    // Current interactive user; do not set highest privileges.
    hr = principal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
    if (SUCCEEDED(hr))
    {
        hr = principal->put_RunLevel(TASK_RUNLEVEL_LUA);
    }
    principal->Release();
    if (FAILED(hr))
    {
        errorOut = autoStartFormatHresult("principal setup failed", hr);
        return false;
    }
    return true;
}

bool addLogonTrigger(ITaskDefinition* definition, std::string& errorOut)
{
    ITriggerCollection* triggers = nullptr;
    HRESULT hr = definition->get_Triggers(&triggers);
    if (FAILED(hr) || triggers == nullptr)
    {
        errorOut = autoStartFormatHresult("get_Triggers failed", hr);
        return false;
    }
    ITrigger* trigger = nullptr;
    hr = triggers->Create(TASK_TRIGGER_LOGON, &trigger);
    triggers->Release();
    if (FAILED(hr) || trigger == nullptr)
    {
        errorOut = autoStartFormatHresult("Create logon trigger failed", hr);
        return false;
    }
    hr = trigger->put_Enabled(VARIANT_TRUE);
    trigger->Release();
    if (FAILED(hr))
    {
        errorOut = autoStartFormatHresult("enable logon trigger failed", hr);
        return false;
    }
    return true;
}

bool bindExecPathArgs(IExecAction* exec, const std::wstring& exePath, std::string& errorOut)
{
    HRESULT hr = exec->put_Path(_bstr_t(exePath.c_str()));
    if (SUCCEEDED(hr))
    {
        hr = exec->put_Arguments(_bstr_t(kAutoSessionFlag));
    }
    if (SUCCEEDED(hr))
    {
        const std::wstring workingDir = autoStartDirectoryOfExe(exePath);
        if (!workingDir.empty())
        {
            hr = exec->put_WorkingDirectory(_bstr_t(workingDir.c_str()));
        }
    }
    if (FAILED(hr))
    {
        errorOut = autoStartFormatHresult("Exec path/arguments/working directory failed", hr);
        return false;
    }
    return true;
}

bool addExecAction(
    ITaskDefinition* definition,
    const std::wstring& exePath,
    std::string& errorOut)
{
    IActionCollection* actions = nullptr;
    HRESULT hr = definition->get_Actions(&actions);
    if (FAILED(hr) || actions == nullptr)
    {
        errorOut = autoStartFormatHresult("get_Actions failed", hr);
        return false;
    }
    IAction* action = nullptr;
    hr = actions->Create(TASK_ACTION_EXEC, &action);
    actions->Release();
    if (FAILED(hr) || action == nullptr)
    {
        errorOut = autoStartFormatHresult("Create exec action failed", hr);
        return false;
    }
    IExecAction* exec = nullptr;
    hr = action->QueryInterface(IID_IExecAction, reinterpret_cast<void**>(&exec));
    action->Release();
    if (FAILED(hr) || exec == nullptr)
    {
        errorOut = autoStartFormatHresult("IExecAction QI failed", hr);
        return false;
    }
    const bool ok = bindExecPathArgs(exec, exePath, errorOut);
    exec->Release();
    return ok;
}

struct DefinitionParts
{
    ITaskService* service = nullptr;
    const std::wstring* exePath = nullptr;
};

bool buildDefinition(
    const DefinitionParts& parts,
    ITaskDefinition** definitionOut,
    std::string& errorOut)
{
    ITaskDefinition* definition = nullptr;
    HRESULT hr = parts.service->NewTask(0, &definition);
    if (FAILED(hr) || definition == nullptr)
    {
        errorOut = autoStartFormatHresult("NewTask failed", hr);
        return false;
    }
    if (!configureTaskSettings(definition, errorOut)
        || !configurePrincipal(definition, errorOut)
        || !addLogonTrigger(definition, errorOut)
        || !addExecAction(definition, *parts.exePath, errorOut))
    {
        definition->Release();
        return false;
    }
    *definitionOut = definition;
    return true;
}

bool registerDefinition(
    ITaskFolder* folder,
    ITaskDefinition* definition,
    std::string& errorOut)
{
    IRegisteredTask* registered = nullptr;
    const HRESULT hr = folder->RegisterTaskDefinition(
        _bstr_t(kAutoStartTaskName),
        definition,
        TASK_CREATE_OR_UPDATE,
        _variant_t(),
        _variant_t(),
        TASK_LOGON_INTERACTIVE_TOKEN,
        _variant_t(L""),
        &registered);
    if (FAILED(hr))
    {
        errorOut = autoStartFormatHresult("RegisterTaskDefinition failed", hr);
        return false;
    }
    if (registered != nullptr)
    {
        registered->Release();
    }
    return true;
}

bool openRootAndRegister(
    ITaskService* service,
    const std::wstring& exeWide,
    std::string& errorOut)
{
    ITaskFolder* folder = nullptr;
    HRESULT hr = service->GetFolder(_bstr_t(L"\\"), &folder);
    if (FAILED(hr) || folder == nullptr)
    {
        errorOut = autoStartFormatHresult("GetFolder failed", hr);
        return false;
    }

    ITaskDefinition* definition = nullptr;
    DefinitionParts parts{service, &exeWide};
    if (!buildDefinition(parts, &definition, errorOut))
    {
        folder->Release();
        return false;
    }

    const bool ok = registerDefinition(folder, definition, errorOut);
    definition->Release();
    folder->Release();
    return ok;
}

std::string buildSchedulerSuccessMessage(const std::wstring& exeWide)
{
    return std::string("Auto-Start registered via Task Scheduler (logon, ")
        + "interactive token, limited rights). task=" + kAutoStartTaskName
        + " exe=" + autoStartWideToUtf8(exeWide)
        + " args=" + buildAutoStartActionArguments();
}
} // namespace
#endif

bool registerAutoStartTaskScheduler(
    const std::wstring& exeWide,
    std::string& messageOut,
    std::string& errorOut)
{
#ifdef _WIN32
    AutoStartComScope com;
    if (!com.ok)
    {
        errorOut = "CoInitializeEx failed for Task Scheduler";
        return false;
    }

    ITaskService* service = nullptr;
    const HRESULT connectHr = connectTaskService(&service);
    if (FAILED(connectHr) || service == nullptr)
    {
        errorOut = autoStartFormatHresult("Task Scheduler Connect failed", connectHr);
        return false;
    }

    const bool ok = openRootAndRegister(service, exeWide, errorOut);
    service->Release();
    if (!ok)
    {
        return false;
    }
    messageOut = buildSchedulerSuccessMessage(exeWide);
    return true;
#else
    (void)exeWide;
    (void)messageOut;
    errorOut = "Task Scheduler Auto-Start requires Windows";
    return false;
#endif
}

bool unregisterAutoStartTaskScheduler(
    std::string& messageOut,
    std::string& errorOut)
{
#ifdef _WIN32
    AutoStartComScope com;
    if (!com.ok)
    {
        errorOut = "CoInitializeEx failed for Task Scheduler";
        return false;
    }

    ITaskService* service = nullptr;
    const HRESULT connectHr = connectTaskService(&service);
    if (FAILED(connectHr) || service == nullptr)
    {
        errorOut = autoStartFormatHresult("Task Scheduler Connect failed", connectHr);
        return false;
    }

    ITaskFolder* folder = nullptr;
    HRESULT hr = service->GetFolder(_bstr_t(L"\\"), &folder);
    if (FAILED(hr) || folder == nullptr)
    {
        service->Release();
        errorOut = autoStartFormatHresult("GetFolder failed", hr);
        return false;
    }

    hr = folder->DeleteTask(_bstr_t(kAutoStartTaskName), 0);
    folder->Release();
    service->Release();
    if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        errorOut = autoStartFormatHresult("DeleteTask failed", hr);
        return false;
    }

    messageOut = std::string("Auto-Start unregistered (Task Scheduler). task=")
        + kAutoStartTaskName;
    return true;
#else
    (void)messageOut;
    errorOut = "Task Scheduler Auto-Start requires Windows";
    return false;
#endif
}
