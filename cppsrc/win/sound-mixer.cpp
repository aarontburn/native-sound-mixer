#include "../headers/sound-mixer.hpp"
#include "./headers/sound-mixer-utils.hpp"

#include <iostream>

#define CHECK_RES(res)  \
	if (res != S_OK)    \
	{                   \
		return nullptr; \
	}

std::string GetProcNameFromId(DWORD id)
{
	if (id == 0)
	{
		return std::to_string(id);
	}
	HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, id);
	LPWSTR processName = (LPWSTR)CoTaskMemAlloc(256);
	DWORD size = 256;
	QueryFullProcessImageNameW(handle, 0, processName, &size);

	std::wstring name = std::wstring(processName);
	return std::string(name.begin(), name.end());
}

inline LPWSTR toLPWSTR(std::string str)
{

	size_t size = str.length() + 1;

	LPWSTR chars = (LPWSTR)CoTaskMemAlloc(size * sizeof(wchar_t));
	std::mbstowcs(chars, str.c_str(), size);
	return (LPWSTR)chars;
}

inline std::string toString(LPWSTR str)
{
	std::wstring wstr(str);

	return std::string(wstr.begin(), wstr.end());
}

inline Napi::Object deviceToObject(SoundMixerUtils::DeviceDescriptor const &desc, Napi::Env const &env)
{
	Napi::Object obj = Napi::Object::New(env);
	obj.Set("id", desc.id);
	obj.Set("name", desc.fullName);
	obj.Set("type", (int)desc.dataFlow);

	return obj;
}

using namespace SoundMixerUtils;

namespace SoundMixer
{

	template <class T>
	void SafeRelease(T **ppT)
	{
		if (ppT && *ppT)
		{
			(*ppT)->Release();
			*ppT = NULL;
		}
	}

	Napi::Object Init(Napi::Env env, Napi::Object exports)
	{
		exports.Set("GetSessions", Napi::Function::New(env, GetAudioSessionNames));
		exports.Set("GetDevices", Napi::Function::New(env, GetEndpoints));
		exports.Set("GetDefaultDevice", Napi::Function::New(env, GetDefaultEndpoint));

		exports.Set("SetEndpointVolume", Napi::Function::New(env, SetEndpointVolume));
		exports.Set("GetEndpointVolume", Napi::Function::New(env, GetEndpointVolume));
		exports.Set("SetEndpointMute", Napi::Function::New(env, SetEndpointMute));
		exports.Set("GetEndpointMute", Napi::Function::New(env, GetEndpointMute));

		exports.Set("SetAudioSessionVolume", Napi::Function::New(env, SetAudioSessionVolume));
		exports.Set("GetAudioSessionVolume", Napi::Function::New(env, GetAudioSessionVolume));
		exports.Set("SetAudioSessionMute", Napi::Function::New(env, SetAudioSessionMute));
		exports.Set("GetAudioSessionMute", Napi::Function::New(env, GetAudioSessionMute));

		return exports;
	}

	Napi::Array GetAudioSessionNames(Napi::CallbackInfo const &info)
	{

		Napi::Env env = info.Env();

		if (info.Length() != 1 || !info[0].IsString())
		{
			throw Napi::TypeError::New(env, "expected string as device id as only parameter");
		}

		CoInitialize(NULL);
		LPWSTR id = toLPWSTR(info[0].As<Napi::String>().Utf8Value());
		IMMDevice *pDevice = GetDeviceById(id);
		CoTaskMemFree(id);
		if (pDevice == nullptr)
		{
			throw Napi::Error::New(env, "Could not find device for specified id.");
		}

		std::vector<IAudioSessionControl2 *> sessions = GetAudioSessions(pDevice);
		SafeRelease(&pDevice);

		Napi::Array sessionNames = Napi::Array::New(env, sessions.size());
		int i = 0;
		DWORD procId = 0;
		LPWSTR guid;
		AudioSessionState state;
		for (IAudioSessionControl2 *pSession : sessions)
		{
			if (pSession->GetProcessId(&procId) == S_OK && pSession->GetSessionIdentifier(&guid) == S_OK && pSession->GetState(&state) == S_OK)
			{
				Napi::Object val = Napi::Object::New(env);
				val.Set("id", toString(guid));
				CoTaskMemFree(guid);
				val.Set("path", GetProcNameFromId(procId));
				val.Set("state", (int)state);
				sessionNames.Set(i++, val);
			}
			SafeRelease(&pSession);
		}

		CoUninitialize();

		return sessionNames;
	}

	Napi::Array GetEndpoints(Napi::CallbackInfo const &info)
	{

		Napi::Env env = info.Env();

		std::vector<DeviceDescriptor> devices = GetDevices();
		Napi::Array result = Napi::Array::New(env, devices.size());
		int i = 0;
		for (DeviceDescriptor const &desc : devices)
		{
			result.Set(i++, deviceToObject(desc, env));
		}
		return result;
	}

	Napi::Object GetDefaultEndpoint(Napi::CallbackInfo const &info)
	{
		Napi::Env env = info.Env();
		if (info.Length() != 1 || !info[0].IsNumber())
		{
			throw Napi::Error::New(env, "wrong argument passed, expected number");
		}

		int val = info[0].As<Napi::Number>().Int32Value();
		if (val < 0 || val >= EDataFlow::EDataFlow_enum_count)
		{
			throw Napi::Error::New(env, "illegal argument passed, expected number ranged from 0 to 3");
		}

		EDataFlow dataFlow = (EDataFlow)val;
		DeviceDescriptor desc = GetDevice(dataFlow);
		if (desc.id.size() == 0)
		{
			throw Napi::Error::New(env, "An error occured when getting device for specified dataflow");
		}

		return deviceToObject(desc, env);
	}

	Napi::Number SetEndpointVolume(Napi::CallbackInfo const &info)
	{
		Napi::Env env = info.Env();

		if (info.Length() != 2 || !info[1].IsNumber() || !info[0].IsString())
		{
			throw Napi::TypeError::New(env, "expected 3 numbers as arguments");
		}

		float volume = info[1].As<Napi::Number>().FloatValue();
		if (volume > 1.F)
		{
			volume = 1.F;
		}
		else if (volume < 0.F)
		{
			volume = 0.F;
		}
		CoInitialize(NULL);
		LPWSTR id = toLPWSTR(info[0].As<Napi::String>().Utf8Value());
		IMMDevice *pDevice = GetDeviceById(id);
		CoTaskMemFree(id);
		if (pDevice == nullptr)
		{
			CoUninitialize();
			throw Napi::Error::New(env, "Invalid device id provided");
		}

		IAudioEndpointVolume *pEndpointVolume = GetDeviceEndpointVolume(pDevice);
		SafeRelease(&pDevice);
		if (pEndpointVolume == nullptr)
		{
			CoUninitialize();
			throw Napi::Error::New(env, "An error occured when getting volume.");
		}

		HRESULT res = pEndpointVolume->SetMasterVolumeLevelScalar(volume, NULL);

		SafeRelease(&pEndpointVolume);
		CoUninitialize();
		if (res != S_OK)
		{
			throw Napi::Error::New(env, "an error occured when setting the volume level");
		}

		return Napi::Number::New(env, volume);
	}

	Napi::Number GetEndpointVolume(Napi::CallbackInfo const &info)
	{
		Napi::Env env = info.Env();
		if (info.Length() != 1 || !info[0].IsString())
		{
			throw Napi::TypeError::New(env, "expected string as device id");
		}

		CoInitialize(NULL);

		LPWSTR id = toLPWSTR(info[0].As<Napi::String>().Utf8Value());
		IMMDevice *pDevice = GetDeviceById(id);
		CoTaskMemFree(id);
		if (pDevice == nullptr)
		{
			CoUninitialize();
			throw Napi::Error::New(env, "Invalid device id provided");
		}

		IAudioEndpointVolume *pEndpointVolume = GetDeviceEndpointVolume(pDevice);
		SafeRelease(&pDevice);
		if (pEndpointVolume == nullptr)
		{
			CoUninitialize();
			throw Napi::Error::New(env, "An error occured when getting volume.");
		}

		float levelScalar = 0.F;
		HRESULT res = pEndpointVolume->GetMasterVolumeLevelScalar(&levelScalar);
		SafeRelease(&pEndpointVolume);
		CoUninitialize();
		if (res != S_OK)
		{
			throw Napi::Error::New(env, "an error occured when getting the volume level");
		}

		return Napi::Number::New(env, levelScalar);
	}

	Napi::Boolean SetEndpointMute(Napi::CallbackInfo const &info)
	{
		Napi::Env env = info.Env();

		if (info.Length() != 2 || !info[1].IsBoolean() || !info[0].IsString())
		{
			throw Napi::TypeError::New(env, "expected {deviceId: string},{mute: boolean} as arguments");
		}

		CoInitialize(NULL);

		LPWSTR id = toLPWSTR(info[0].As<Napi::String>().Utf8Value());
		IMMDevice *pDevice = GetDeviceById(id);
		CoTaskMemFree(id);
		if (pDevice == nullptr)
		{
			CoUninitialize();
			throw Napi::Error::New(env, "Invalid device id provided");
		}

		IAudioEndpointVolume *pEndpointVolume = GetDeviceEndpointVolume(pDevice);
		SafeRelease(&pDevice);
		if (pEndpointVolume == nullptr)
		{
			CoUninitialize();
			throw Napi::Error::New(env, "An error occured when getting volume.");
		}

		bool mute = info[1].As<Napi::Boolean>().Value();

		HRESULT res = pEndpointVolume->SetMute(mute, NULL);
		SafeRelease(&pEndpointVolume);
		CoUninitialize();
		if (res != S_OK && res != S_FALSE)
		{
			throw Napi::Error::New(env, "an error occured when setting mute");
		}

		return Napi::Boolean::New(env, mute);
	}

	Napi::Boolean GetEndpointMute(Napi::CallbackInfo const &info)
	{
		Napi::Env env = info.Env();
		if (info.Length() != 1 || !info[0].IsString())
		{
			throw Napi::TypeError::New(env, "device id as only argument");
		}

		CoInitialize(NULL);

		LPWSTR id = toLPWSTR(info[0].As<Napi::String>().Utf8Value());
		IMMDevice *pDevice = GetDeviceById(id);
		CoTaskMemFree(id);
		if (pDevice == nullptr)
		{
			CoUninitialize();
			throw Napi::Error::New(env, "Invalid device id provided");
		}

		IAudioEndpointVolume *pEndpointVolume = GetDeviceEndpointVolume(pDevice);
		SafeRelease(&pDevice);
		if (pEndpointVolume == nullptr)
		{
			CoUninitialize();
			throw Napi::Error::New(env, "An error occured when getting volume.");
		}
		BOOL mute = 0;
		HRESULT res = pEndpointVolume->GetMute(&mute);
		SafeRelease(&pEndpointVolume);
		CoUninitialize();
		if (res != S_OK)
		{
			throw Napi::Error::New(env, "an error occured when getting the volume level");
		}

		return Napi::Boolean::New(env, (bool)mute);
	}

	Napi::Number SetAudioSessionVolume(Napi::CallbackInfo const &info)
	{
		Napi::Env env = info.Env();

		if (info.Length() != 3 || !info[0].IsString() || !info[1].IsString() || !info[2].IsNumber())
		{
			throw Napi::TypeError::New(env, "expected 3 numbers as arguments");
		}

		float volume = info[2].As<Napi::Number>().FloatValue();
		if (volume > 1.F)
		{
			volume = 1.F;
		}
		else if (volume < 0.F)
		{
			volume = 0.F;
		}

		CoInitialize(NULL);

		LPWSTR deviceId = toLPWSTR(info[0].As<Napi::String>().Utf8Value());
		IMMDevice *pDevice = GetDeviceById(deviceId);
		CoTaskMemFree(deviceId);
		if (pDevice == nullptr)
		{
			CoUninitialize();
			throw Napi::Error::New(env, "Invalid device id provided");
		}

		LPWSTR sessionId = toLPWSTR(info[1].As<Napi::String>().Utf8Value());
		IAudioSessionControl2 *pSessionControl = GetAudioSessionByGUID(pDevice, sessionId);
		CoTaskMemFree(sessionId);
		SafeRelease(&pDevice);
		if (pSessionControl == nullptr)
		{
			CoUninitialize();
			throw Napi::Error::New(env, "Invalid session id provided");
		}

		ISimpleAudioVolume *pSessionVolume = GetSessionVolume(pSessionControl);
		SafeRelease(&pSessionControl);
		if (pSessionVolume == nullptr)
		{
			CoUninitialize();
			throw Napi::Error::New(env, "An error occured when getting volume.");
		}

		HRESULT res = pSessionVolume->SetMasterVolume(volume, NULL);
		SafeRelease(&pSessionVolume);
		CoUninitialize();
		if (res != S_OK)
		{
			throw Napi::Error::New(env, "an error occured when setting the volume level for audioSession");
		}

		return Napi::Number::New(env, volume);
	}

	Napi::Number GetAudioSessionVolume(Napi::CallbackInfo const &info)
	{
		Napi::Env env = info.Env();
		if (info.Length() != 2 || !info[1].IsString() || !info[0].IsString())
		{
			throw Napi::TypeError::New(env, "expected {<deviceId>: string}, {<audio session id>: string} as arguments");
		}

		CoInitialize(NULL);

		LPWSTR deviceId = toLPWSTR(info[0].As<Napi::String>().Utf8Value());
		IMMDevice *pDevice = GetDeviceById(deviceId);
		CoTaskMemFree(deviceId);
		if (pDevice == nullptr)
		{
			CoUninitialize();
			throw Napi::Error::New(env, "Invalid device id provided");
		}

		LPWSTR sessionId = toLPWSTR(info[1].As<Napi::String>().Utf8Value());
		IAudioSessionControl2 *pSessionControl = GetAudioSessionByGUID(pDevice, sessionId);
		CoTaskMemFree(sessionId);
		SafeRelease(&pDevice);
		if (pSessionControl == nullptr)
		{
			CoUninitialize();
			throw Napi::Error::New(env, "Invalid session id provided");
		}

		ISimpleAudioVolume *pSessionVolume = GetSessionVolume(pSessionControl);
		SafeRelease(&pSessionControl);
		if (pSessionVolume == nullptr)
		{
			CoUninitialize();
			throw Napi::Error::New(env, "An error occured when getting volume.");
		}

		float volume = 0.F;
		HRESULT res = pSessionVolume->GetMasterVolume(&volume);
		SafeRelease(&pSessionVolume);
		CoUninitialize();
		if (res != S_OK)
		{
			throw Napi::Error::New(env, "an error occured when getting the volume level for audioSession");
		}

		return Napi::Number::New(env, volume);
	}

	Napi::Boolean SetAudioSessionMute(Napi::CallbackInfo const &info)
	{
		Napi::Env env = info.Env();
		if (info.Length() != 3 || !info[0].IsString() || !info[1].IsString() || !info[2].IsBoolean())
		{
			throw Napi::TypeError::New(env, "expected 3 numbers as arguments");
		}

		bool mute = info[2].As<Napi::Boolean>().Value();
		LPWSTR deviceId = toLPWSTR(info[0].As<Napi::String>().Utf8Value());
		IMMDevice *pDevice = GetDeviceById(deviceId);
		CoTaskMemFree(deviceId);
		if (pDevice == nullptr)
		{
			CoUninitialize();
			throw Napi::Error::New(env, "Invalid device id provided");
		}

		LPWSTR sessionId = toLPWSTR(info[1].As<Napi::String>().Utf8Value());
		IAudioSessionControl2 *pSessionControl = GetAudioSessionByGUID(pDevice, sessionId);
		CoTaskMemFree(sessionId);
		SafeRelease(&pDevice);
		if (pSessionControl == nullptr)
		{
			CoUninitialize();
			throw Napi::Error::New(env, "Invalid session id provided");
		}

		ISimpleAudioVolume *pSessionVolume = GetSessionVolume(pSessionControl);
		SafeRelease(&pSessionControl);
		if (pSessionVolume == nullptr)
		{
			CoUninitialize();
			throw Napi::Error::New(env, "An error occured when getting volume.");
		}

		HRESULT res = pSessionVolume->SetMute(mute, NULL);
		SafeRelease(&pSessionVolume);
		CoUninitialize();
		if (res != S_OK)
		{
			throw Napi::Error::New(env, "an error occured when muting audioSession");
		}

		return Napi::Boolean::New(env, mute);
	}

	Napi::Boolean GetAudioSessionMute(Napi::CallbackInfo const &info)
	{
		Napi::Env env = info.Env();

		if (info.Length() != 2 || !info[0].IsString() || !info[1].IsString())
		{
			throw Napi::TypeError::New(env, "expected 3 numbers as arguments");
		}

		CoInitialize(NULL);

		LPWSTR deviceId = toLPWSTR(info[0].As<Napi::String>().Utf8Value());
		IMMDevice *pDevice = GetDeviceById(deviceId);
		CoTaskMemFree(deviceId);
		if (pDevice == nullptr)
		{
			CoUninitialize();
			throw Napi::Error::New(env, "Invalid device id provided");
		}

		LPWSTR sessionId = toLPWSTR(info[1].As<Napi::String>().Utf8Value());
		IAudioSessionControl2 *pSessionControl = GetAudioSessionByGUID(pDevice, sessionId);
		CoTaskMemFree(sessionId);
		SafeRelease(&pDevice);
		if (pSessionControl == nullptr)
		{
			CoUninitialize();
			throw Napi::Error::New(env, "Invalid session id provided");
		}

		ISimpleAudioVolume *pSessionVolume = GetSessionVolume(pSessionControl);
		SafeRelease(&pSessionControl);
		if (pSessionVolume == nullptr)
		{
			CoUninitialize();
			throw Napi::Error::New(env, "An error occured when getting volume.");
		}

		BOOL mute = 0;
		HRESULT res = pSessionVolume->GetMute(&mute);
		SafeRelease(&pSessionVolume);
		CoUninitialize();

		if (res != S_OK)
		{
			throw Napi::Error::New(env, "an error occured when getting mute audioSession");
		}

		return Napi::Boolean::New(env, (bool)mute);
	}

} // namespace SoundMixer
