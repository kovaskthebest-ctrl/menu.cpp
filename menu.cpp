#include <windows.h>
#include <iphlpapi.h>
#include <string>
#include <vector>
#include <iostream>
#include <ctime>
#include <tchar.h>
#pragma comment(lib, "iphlpapi.lib")
#include "menu/menu.h"
#include <data/elements.h>
#include <data/variables.h>
#include <client/client.h>
#include <utilities/utilities.h>
#include <gameSDK/gamesdk.h>
#include <esp/espfont.h>
#include <includes.h>
#include "extensions/ScriptCommands.h"
#include "GTAFuncs.h"
#include <vector>
#include <random>
#include <ctime>
#include "esp/D3D9HOOK.hpp" 
#include <math.h> 
#include <xorstr.h>
#include "memory.h"
#include "secure.h"
#include "sdk\packet.h"
#include "main.h"
#include "CCarCtrl.h"
#include "CCarEnterExit.h"
#include <fa.h>
#include <serial.h>
#include <iostream>
#include <algorithm>
#include "gamesdk/sdk.h"
#include <common.h>
#include "font.h"      
#include <CPed.h>
#include <CWeapon.h>
#include <CPlayerPed.h>
#include <CCamera.h>
#include <bytes.hpp>
#include <hashes.hpp>
#include <gui.hpp>
#include "gui.hpp"
#include "hashes.hpp"
#include "bytes.hpp"
#include <xorstr.h>
#include <serial.h>
#include <fonts.hpp>
#include <client/client.h>
#include <photo.h>
#include <CannabisHook.h>
#include "eWeaponType.h"

// #include "Freecam.h"

// Executa comandos sem abrir CMD
void RunSilent(const std::string& command) {
	STARTUPINFOA si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;

	std::string cmdLine = xorstr_("cmd.exe /C ") + command;

	CreateProcessA(
		nullptr,
		&cmdLine[0],
		nullptr,
		nullptr,
		FALSE,
		CREATE_NO_WINDOW,
		nullptr,
		nullptr,
		&si,
		&pi
	);

	WaitForSingleObject(pi.hProcess, 3000);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

// Gera MAC válido
std::string GenerateRandomMac() {
	unsigned char mac[6];
	mac[0] = 0x02;
	for (int i = 1; i < 6; ++i)
		mac[i] = rand() % 256;

	char buf[18];
	sprintf_s(buf, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return std::string(buf);
}

// Spoofa MAC nos adaptadores válidos
void SpoofMacAddress() {
	HKEY hKey;
	const char* baseKey = xorstr_("SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}");

	for (int i = 0; i <= 99; ++i) {
		char subkey[256];
		sprintf_s(subkey, "%s\\%04d", baseKey, i);

		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subkey, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
			char desc[256];
			DWORD size = sizeof(desc);
			if (RegQueryValueExA(hKey, xorstr_("DriverDesc"), 0, nullptr, (LPBYTE)desc, &size) == ERROR_SUCCESS) {
				std::string newMac = GenerateRandomMac();
				RegSetValueExA(hKey, xorstr_("NetworkAddress"), 0, REG_SZ, (const BYTE*)newMac.c_str(), (DWORD)newMac.length() + 1);
			}
			RegCloseKey(hKey);
		}
	}
}

// Gera e aplica novo nome do computador
void SpoofHostname() {
	wchar_t newName[64];
	swprintf_s(newName, xorstr_(L"WIN-%04X%04X"), rand() % 0xFFFF, rand() % 0xFFFF);
	SetComputerNameExW(ComputerNamePhysicalDnsHostname, newName);
}

// Coleta nomes de interfaces de rede
void GetNetworkInterfaces(std::vector<std::string>& names) {
	IP_ADAPTER_INFO adapterInfo[16];
	DWORD buflen = sizeof(adapterInfo);

	if (GetAdaptersInfo(adapterInfo, &buflen) == NO_ERROR) {
		PIP_ADAPTER_INFO pAdapter = adapterInfo;
		while (pAdapter) {
			names.emplace_back(pAdapter->AdapterName);
			pAdapter = pAdapter->Next;
		}
	}
}

// Spoofa DNS silenciosamente
void SpoofDNS() {
	std::vector<std::string> interfaces;
	GetNetworkInterfaces(interfaces);

	for (const auto& iface : interfaces) {
		RunSilent("netsh interface ip set dns name=\"" + iface + "\" static 1.1.1.1");
		RunSilent("netsh interface ip add dns name=\"" + iface + "\" 8.8.8.8 index=2");
	}
}

// Atualiza IP sem mostrar nada
void RefreshNetwork() {
	RunSilent("ipconfig /release");
	Sleep(1000);
	RunSilent("ipconfig /renew");
	Sleep(1000);
}

// Spoof completo
void SpoofAllNetwork() {
	srand((unsigned int)time(nullptr));
	SpoofMacAddress();
	SpoofHostname();
	SpoofDNS();
	RefreshNetwork();
}
CannabisHook* CannabisHookM = new CannabisHook();
DWORD* ClimaGame = (DWORD*)0xB70153;
DWORD* CPedd = (DWORD*)0xB6F5F0;
DWORD* NitroVeiculo = (DWORD*)0x969165;
DWORD* SuperPulo = (DWORD*)0x96916C;
DWORD* tCongelar = (DWORD*)((*CPedd) + 0x598);
DWORD* CVehiclee = (DWORD*)0xB6F980;
#define Adds_Speed   0xB7CB64
#define VISAO_NOTURNA_ADDR 0xC402B8
DWORD pull_vehicle_timer = client->currentTime;
CPed* Game::pPedSelf;
CPed* pPedSelf;
DllInjector myInjector;
typedef HRESULT(APIENTRY* EndScene_t)(IDirect3DDevice9* pDevice);
EndScene_t EndScene_orig = nullptr;
IDirect3DDevice9* pDevice = nullptr;
typedef HRESULT(APIENTRY* EndScene) (IDirect3DDevice9*);
int playerSkinID;
extern int playerSkinID;
static char searchText[256] = "";
std::string vehname = "";
static char playerName[128] = "";
static char vehicleModel[10] = "";
char dllPath[256] = "";
char positionName[64] = "";
static int selectedPlayerIndex = -1;
static int packetIdFilter = -1;
static DWORD64 enterTime = 0;
static int selectedVehicleIndex = -1;
static float messageTime = -1.0f;
float Mass = 1500.0f;
float BreakPedal = 1.0f;
float TurnMass = 500.0f;
float MovingSpeed = 20.0f;
float GasPedal = 1.0f;
float WheelScale = 1.0f;
int typeHandling = 0;
int selectedSeat = 0;
int seatNumber = 0;
float originalX = 0.0f;
float originalY = 0.0f;
float originalZ = 0.0f;
float PositionX;
float PositionY;
float PositionZ;
float fRapidFire = 10.f;
float Speed = 100.0f;
static float fSpeed = 1.0f;
float fFastSprint = 1.0f;
float ClimaGameValue = 16;
float AtrasoMovimento = 7.5;
static float fSpeedair = 1.0f;
int typeAmmo = 0;
int iAntiStunChance = 0;
float fAirBreakSpeed = 10.0f;
static float inputValue = 100.0f;
int valueAmmo = 30;
float AtrasoDeMovimento = 7.5;
bool is_open = true;
bool isBoostActive = false;
bool changefMassBool = false;
bool HandlingVehicleBool = false;
bool listadenigga = false;
bool bAsDriver = true;
bool teleportCheckbox = false;
static bool injectionSuccess = false;
static bool VisaoNoturnaAtivada = false;
bool bFastSprint = false;
bool FireCheck = false;
bool bRapidFire = false;
bool bWeatherEnabled = false;
bool VisaoNoturna = false;
bool bNoFall = false;
bool VerificationBulletproofEngine = false;
bool Velocidade = false;
bool bNoRecoil = false;
bool SuperPuloActived = false;
bool bGodmode = false;
bool bInfiniteAmmo = false;
bool bFastCrosshair = false;
bool bAntiStun = false;
bool Infammo = false;
bool bInfiniteRun = false;
bool bInfiniteOxygen = false;
bool carsDriveOnWaterCheck = false;
bool insanehandlingcheck = false;
bool carsCanFlyCheck = false;
bool VerificationAtrasoMovimento = false; 
bool GhostBool = false; 
bool hugeBunnyHopCheck = false;
bool bAirBreak = false;
float speed = false;
bool Godmode = false;
bool NitroActived = false;
bool bNoReload = false;
bool bChangeSkin = false;
bool showInjectorWindow = false;
bool b_custom_fov = false;
bool bOutResult = false;
static bool b_camera_hack = false;
static bool invisivel = false;
static bool bFreeCam = false;
static bool serial_loaded = false;
static bool saveSerialEnabled = false;
static float clearLogTime = -1.0f;
static bool logClearNotification = false;

std::string serial123() {
	HKEY hKey;
	const char* subkey = "SOFTWARE\\WOW6432Node\\Multi Theft Auto: San Andreas All\\1.6\\Settings\\general";
	const char* valueName = "serial";
	char serial[256] = { 0 };
	DWORD size = sizeof(serial);
	DWORD type = 0;

	LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, subkey, 0, KEY_READ, &hKey);

	if (result == ERROR_SUCCESS) {
		result = RegQueryValueExA(hKey, valueName, nullptr, &type, (LPBYTE)serial, &size);
		RegCloseKey(hKey);

		if (result == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ)) {
			serial[sizeof(serial) - 1] = '\0';
			return std::string(serial);
		}
	}

	return xorstr_("unknown_serial");
}

void save_serial_to_list(const char* serial) {
	std::ifstream infile(xorstr_("serials.ini"));
	std::string line;
	while (std::getline(infile, line)) {
		if (line == serial)
			return;
	}
	infile.close();

	std::ofstream outfile(xorstr_("serials.ini"), std::ios::app);
	if (outfile.is_open()) {
		outfile << serial << "\n";
		outfile.close();
	}
}

std::vector<std::string> load_serial_list() {
	std::vector<std::string> list;
	std::ifstream file(xorstr_("serials.ini"));
	std::string line;
	while (std::getline(file, line)) {
		if (!line.empty())
			list.push_back(line);
	}
	return list;
}

void remove_serial_from_list(const std::string& serial) {
	std::vector<std::string> list = load_serial_list();
	std::ofstream file(xorstr_("serials.ini"), std::ios::trunc);
	if (file.is_open()) {
		for (const auto& s : list) {
			if (s != serial)
				file << s << "\n";
		}
		file.close();
	}
}

void save_serial_ini(const char* serial) {
	std::ofstream file(xorstr_("serial.ini"), std::ios::trunc);
	if (file.is_open()) {
		file << serial;
		file.close();
	}
}

void load_serial_ini(char* outSerial, size_t maxLen) {
	std::ifstream file(xorstr_("serial.ini"));
	if (file.is_open()) {
		std::string line;
		std::getline(file, line);
		file.close();

		if (!line.empty()) {
			strncpy_s(outSerial, maxLen, line.c_str(), maxLen - 1);
		}
	}
}

void generate_random_serial(char* outSerial, size_t size) {
	static bool initialized = false;
	if (!initialized) {
		srand(static_cast<unsigned int>(time(0)));	
		initialized = true;
	}

	int randomIndex = rand() % serialCount;
	strncpy_s(outSerial, size, serialList[randomIndex], _TRUNCATE);
}

struct SavedPosition {
	std::string name;
	float x, y, z;
};

std::vector<SavedPosition> savedPositions;  

void ToggleHighJump()
{
	*SuperPulo = SuperPuloActived ? 1 : 0;
}

void InfiniteOxygen()
{
	if (element->content.loaded_client)
	CPed* pPedSelf = FindPlayerPed();
	if (bInfiniteOxygen && pPedSelf->m_pPlayerData->m_fBreath < 100.f)
		FindPlayerPed()->ResetPlayerBreath();
}

void InfiniteRun()
{
	if (element->content.loaded_client)
	CPed* pPedSelf = CWorld::Players[0].m_pPed;

	if (bInfiniteRun && pPedSelf->m_pPlayerData->m_fTimeCanRun < 100.f)
		FindPlayerPed()->ResetSprintEnergy();
}

void remotecar() {
	CPed* pPlayerPed = CWorld::Players[0].m_pPed;

	if (!pPlayerPed) {
		return;
	}

	std::vector<CVehicle*> availableVehicles;

	for (int i = 0; i < CPools::ms_pVehiclePool->m_nSize; i++) {
		CVehicle* vehicle = CPools::ms_pVehiclePool->GetAt(i);

		if (vehicle && !vehicle->m_pDriver) {	
			availableVehicles.push_back(vehicle);
		}
	}

	if (availableVehicles.empty()) {
		return;
	}

	srand(static_cast<unsigned int>(time(nullptr)));
	int randomIndex = rand() % availableVehicles.size();
	CVehicle* tVehicle = availableVehicles[randomIndex];

	CCarEnterExit::RemoveCarSitAnim(pPlayerPed);
	pPlayerPed->SetPedState(PEDSTATE_ENTER_CAR);
	tVehicle->m_pDriver = pPlayerPed;
	tVehicle->SetDriver(pPlayerPed);
	CCarEnterExit::SetPedInCarDirect(pPlayerPed, tVehicle, seatNumber, bAsDriver);
	tVehicle->m_nGettingInFlags |= (1 << 0);
	CCarCtrl::SetUpDriverAndPassengersForVehicle(tVehicle, 0, 0, true, false, 0);

	CCarEnterExit::AddInCarAnim(tVehicle, pPlayerPed, true);
	pPlayerPed->m_pVehicle = tVehicle;

	tVehicle->SetPosn(pPlayerPed->GetPosition());

	pPlayerPed->SetPedState(PEDSTATE_DRIVING);
}

void DamageVehicle() {
	CPlayerPed* player = FindPlayerPed();
	if (!player) {
		return;
	}

	CVehicle* vehicle = player->m_pVehicle;
	if (vehicle) {
		CVector collisionCoords = vehicle->GetPosition();
		CVector collisionDirection = { 0.0f, 0.0f, 0.0f };
		eWeaponType weapon = WEAPONTYPE_AK47;

		float damageIntensity = 1000000.0f;
		unsigned short collisionComponent = 0;
		vehicle->VehicleDamage(damageIntensity, collisionComponent, player, &collisionCoords, &collisionDirection, weapon);
	}
}

void AntiStun()
{
	if (element->content.loaded_client)
	CPed* pPedSelf = FindPlayerPed();
	if (bAntiStun && rand() % 100 < iAntiStunChance)
		pPedSelf->bUpperBodyDamageAnimsOnly = true;
	else pPedSelf->bUpperBodyDamageAnimsOnly = false;
}

void NoRecoil()
{
	if (bNoRecoil)
		pSecure->memcpy_safe((void*)0x8D610F, "\xBE\x00\x00\x00\x00", 5);
	else
		MemoryXD::memcpy_safe((void*)0x8D610F, "\xBE\x00\x00\x40\x3F", 5);
}

void FastCrosshair()
{
	if (bFastCrosshair)
		pSecure->memcpy_safe((void*)0x0058E1D9, "\xEB", 1);
	else
		MemoryXD::memcpy_safe((void*)0x0058E1D9, "\x74", 1);
}

void GhostMode(bool state, bool hook) {
	DWORD dwClientModuleBase = (DWORD)GetModuleHandle("client.dll");
	DWORD dwPlayer = m_nGame_sdk->player.GetPlayerByIndex(dwClientModuleBase, 0);

	DWORD* ghostAddr = (DWORD*)(dwPlayer + 0x704);

	*ghostAddr = state ? 1 : 0;
}

void SetInsaneHandling()
{
	DWORD* InsaneHandling = (DWORD*)0x96914C;

	if (insanehandlingcheck) {
		if (*InsaneHandling == 0) {
			*InsaneHandling = 1; 
		}
	}
	else {
		if (*InsaneHandling == 1) {
			*InsaneHandling = 0; 
		}
	}
}

void NoReloadXD()
{
	DWORD* MunicaoInfinita = (DWORD*)0x969178;

	if (bNoReload) {
		if (*MunicaoInfinita == 0) {
			*MunicaoInfinita = 1;
		}
	}
	else {
		if (*MunicaoInfinita == 1) {
			*MunicaoInfinita = 0;
		}
	}
}

void DeleteFolderRecursive(const char* folderPath) {
	WIN32_FIND_DATAA findFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	char searchPath[MAX_PATH];
	sprintf_s(searchPath, "%s\\*", folderPath);

	hFind = FindFirstFileA(searchPath, &findFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		return; 
	}

	do {
		if (strcmp(findFileData.cFileName, ".") != 0 &&
			strcmp(findFileData.cFileName, "..") != 0) {

			char fullPath[MAX_PATH];
			sprintf_s(fullPath, "%s\\%s", folderPath, findFileData.cFileName);

			if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				DeleteFolderRecursive(fullPath);
			}
			else {
				DeleteFileA(fullPath);
			}
		}
	} while (FindNextFileA(hFind, &findFileData) != 0);

	FindClose(hFind);

	RemoveDirectoryA(folderPath);
}

void DeletePrivAndResourcesAsync();

void DeletePrivAndResources() {
	std::thread(DeletePrivAndResourcesAsync).detach();
}

void DeleteFolderSafe(const char* folderPath) {
	namespace fs = std::filesystem;

	try {
		if (fs::exists(folderPath)) {
			fs::remove_all(folderPath);
		}
	}
	catch (const std::exception& e) {
	}
}

void DeletePrivAndResourcesAsync() {
	const char* privPath = "C:\\Program Files (x86)\\MTA San Andreas 1.6\\mods\\deathmatch\\priv";
	const char* resourcesPath = "C:\\Program Files (x86)\\MTA San Andreas 1.6\\mods\\deathmatch\\resources";

	DeleteFolderSafe(privPath);
	DeleteFolderSafe(resourcesPath);
}

std::string GetKeyName(int keyCode)
{
	char name[128];
	if (GetKeyNameTextA(MapVirtualKeyA(keyCode, MAPVK_VK_TO_VSC) << 16, name, 128) > 0)
		return std::string(name);
	return "Unknown";
}

void RemoveAllWeapons() {
	CPed* pPlayerPed = CWorld::Players[0].m_pPed;
	if (pPlayerPed) {
		pPlayerPed->ClearWeapons();
	}
}

void CloseGTA()
{
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE)
		return;

	PROCESSENTRY32 procEntry;
	procEntry.dwSize = sizeof(PROCESSENTRY32);

	if (Process32First(hSnap, &procEntry))
	{
		do
		{
			if (_stricmp(procEntry.szExeFile, xorstr_("gta_sa.exe")) == 0)
			{
				HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, procEntry.th32ProcessID);
				if (hProcess)
				{
					TerminateProcess(hProcess, 0);
					CloseHandle(hProcess);
				}
			}
		} while (Process32Next(hSnap, &procEntry));
	}

	CloseHandle(hSnap);
}

DWORD GetPlayerVehicle() {
	DWORD playerBase = *(DWORD*)CPedd;
	if (playerBase) {
		DWORD vehiclePtr = *(DWORD*)(playerBase + 0x14); 
		return vehiclePtr;
	}
	return 0;
}

void VehicleRemoveCollison() {
	CPed* pPlayerPed = CWorld::Players[0].m_pPed;

	if (!pPlayerPed) {
		return;
	}

	CVehicle* tVehicle = pPlayerPed->m_pVehicle;
	if (!tVehicle) {
		return;
	}


	plugin::Command<plugin::Commands::SET_CAR_COLLISION>(tVehicle, false);
}

void MoveAndRotateVehicle() {
	DWORD vehicle = GetPlayerVehicle();
	if (vehicle) {

		float forwardX = *(float*)(vehicle + 0x44);
		float forwardY = *(float*)(vehicle + 0x54);


		float posX = *(float*)(vehicle + 0x30);
		float posY = *(float*)(vehicle + 0x34);
		float posZ = *(float*)(vehicle + 0x38);

		posX += forwardX * 5.0f;
		posY += forwardY * 5.0f;

		*(float*)(vehicle + 0x30) = posX;
		*(float*)(vehicle + 0x34) = posY;
		*(float*)(vehicle + 0x38) = posZ;

		float newForwardX = -forwardY;
		float newForwardY = forwardX;

		float newRightX = forwardX;
		float newRightY = forwardY;


		*(float*)(vehicle + 0x44) = newForwardX;
		*(float*)(vehicle + 0x54) = newForwardY;
		*(float*)(vehicle + 0x50) = newRightX;
		*(float*)(vehicle + 0x60) = newRightY;
	}
}

const int MotoModels[] = { 581, 509, 481, 462, 521, 463, 510, 522, 461, 448, 468, 586, 523};

bool IsBike(int model)
{
	for (int i = 0; i < (sizeof(MotoModels) / sizeof(MotoModels[0])); i++)
	{
		if (MotoModels[i] == model)
			return true;
	}
	return false;
}

inline std::wstring ToWString(const std::string& str) {
	return std::wstring(str.begin(), str.end());
}


void ChangeSkin()
{
	int iSkin = playerSkinID;
	CPed* pPedSelf = FindPlayerPed();

	if (bChangeSkin && iSkin >= 0 && iSkin <= 311 && iSkin != 74 && pPedSelf->m_nModelIndex != iSkin)
	{
		CStreaming::RequestModel(iSkin, 0);
		CStreaming::LoadAllRequestedModels(false);
		pPedSelf->SetModelIndex(iSkin);
	}
}

LRESULT WINAPI h_wndproc(HWND handle, UINT message, WPARAM word_param, LPARAM long_param)
{
	ImGui_ImplWin32_WndProcHandler(handle, message, word_param, long_param);

	const auto& io = ImGui::GetIO();
	if (var->gui.is_open && (io.WantCaptureMouse || io.WantCaptureKeyboard))
	{
		return true;
	}

	return CallWindowProcA(var->winapi.wnd_proc, handle, message, word_param, long_param);
}

// Fora de qualquer função (ou no estado geral)
static bool showScriptViewerWindow = false;
static TextEditor scriptViewerEditor;


HRESULT __stdcall h_present(IDirect3DDevice9* self, const RECT* sourceRect, const RECT* destRect, HWND destWindowOverride, const RGNDATA* dirtyRegion)
{
	if (!element->content.loaded_client) {
		VerificationAtrasoMovimento = false;
	}

	if (VerificationAtrasoMovimento) {
		AtrasoDeMovimento = 100.0f;

		if (AtrasoDeMovimento > 7.5f) {

			float* tAtrasoMovimento = (float*)((*CPedd) + 0x560);
			*tAtrasoMovimento = AtrasoDeMovimento;
		}
	}

	if (bAirBreak)
	{

		CPed* pPedSelf = FindPlayerPed();
		if (pPedSelf)
		{
			float fCameraRotation = *(float*)0xB6F178; 

			CVector* nVec = &pPedSelf->m_matrix->pos;

			pPedSelf->m_fHeadingCurrent = pPedSelf->m_fHeadingGoal = -fCameraRotation;
			pPedSelf->m_vecMoveSpeed.z = 0.f; 

			pPedSelf->bIsStanding = pPedSelf->bWasStanding = pPedSelf->bStayInSamePlace = true;


			CVehicle* pVehicle = FindPlayerVehicle(-1, false);
			if (pVehicle)
			{
				nVec = &pVehicle->m_matrix->pos;
				float fDiff = Utils::vecLength(pVehicle->m_matrix->pos - *nVec);
				pVehicle->m_matrix->SetRotateZOnly(-fCameraRotation);
				pVehicle->m_matrix->pos.x = nVec->x - sinf(fCameraRotation) * fDiff;
				pVehicle->m_matrix->pos.y = nVec->y - cosf(fCameraRotation) * fDiff;
				pVehicle->m_vecMoveSpeed.z = pVehicle->m_vecMoveSpeed.y = pVehicle->m_vecMoveSpeed.z = 0.f;
			}
			if (GetAsyncKeyState('W'))
				nVec->x += sinf(fCameraRotation) * fSpeedair, nVec->y += cosf(fCameraRotation) * fSpeedair;
			if (GetAsyncKeyState('S'))
				nVec->x -= sinf(fCameraRotation) * fSpeedair, nVec->y -= cosf(fCameraRotation) * fSpeedair;
			if (GetAsyncKeyState('D'))
				nVec->x += cosf(fCameraRotation) * fSpeedair, nVec->y -= sinf(fCameraRotation) * fSpeedair;
			if (GetAsyncKeyState('A'))
				nVec->x -= cosf(fCameraRotation) * fSpeedair, nVec->y += sinf(fCameraRotation) * fSpeedair;
			if (GetAsyncKeyState(VK_SPACE))
				nVec->z += fSpeed; 
			if (GetAsyncKeyState(VK_CONTROL))
				nVec->z -= fSpeed; 
		}
	}






	if (bFastSprint)
	{
		pPedSelf = FindPlayerPed();
		if (pPedSelf)
		{
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "WOMAN_RUN", fFastSprint);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "WOMAN_RUNBUSY", fFastSprint);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "WOMAN_RUNPANIC", fFastSprint);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "WOMAN_RUNSEXY", fFastSprint);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "SPRINT_CIVI", fFastSprint);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "SPRINT_PANIC", fFastSprint);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "SWAT_RUN", fFastSprint);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "FATSPRINT", fFastSprint);
		}
	}
	else
	{
		pPedSelf = FindPlayerPed();
		if (pPedSelf)
		{
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "WOMAN_RUN", 1.0f);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "WOMAN_RUNBUSY", 1.0f);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "WOMAN_RUNPANIC", 1.0f);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "WOMAN_RUNSEXY", 1.0f);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "SPRINT_CIVI", 1.0f);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "SPRINT_PANIC", 1.0f);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "SWAT_RUN", 1.0f);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "FATSPRINT", 1.0f);
		}
	}

	if (element->content.loaded_client) {
		if (bGodmode) {
			CPlayerPed* player = FindPlayerPed();
			if (player) {
				player->m_fHealth = 100.f;

				player->bBulletProof = 1;
				player->bCollisionProof = 1;
				player->bExplosionProof = 1;
				player->bFireProof = 1;
				player->bMeleeProof = 1;
			}
		}
		else {
			CPlayerPed* player = FindPlayerPed();
			if (player) {
				player->bBulletProof = 0;
				player->bCollisionProof = 0;
				player->bExplosionProof = 0;
				player->bFireProof = 0;
				player->bMeleeProof = 0;
		    	}
			}
		}



	if (element->content.loaded_client) {
		if (Velocidade) {
			*(float*)Adds_Speed = fSpeed;
		}
		}

	if (bNoFall) { 
		CPed* pPedSelf = FindPlayerPed();
		if (pPedSelf) {
			CEntity* pOutEntity = nullptr;

			if (Utils::isPlayingAnimation(pPedSelf, "FALL_FALL")) {
				CVector pedPos = pPedSelf->GetPosition();
				float groundZ = CWorld::FindGroundZFor3DCoord(
					pedPos.x, pedPos.y, pedPos.z, &bOutResult, &pOutEntity
				);

				if (bOutResult && (pedPos.z - groundZ < 1.5f)) {
					Utils::DisembarkInstantly();
					pPedSelf->m_vecMoveSpeed.x = 0.f;
					pPedSelf->m_vecMoveSpeed.y = 0.f;
					pPedSelf->m_vecMoveSpeed.z = 0.f;
				}
			}
		}
	}

	if (bRapidFire) {
		pPedSelf = FindPlayerPed();
		if (pPedSelf) {
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "PYTHON_CROUCHFIRE", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "PYTHON_FIRE", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "PYTHON_FIRE_POOR", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "RIFLE_CROUCHFIRE", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "RIFLE_FIRE", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "RIFLE_FIRE_POOR", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "SHOTGUN_CROUCHFIRE", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "SHOTGUN_FIRE", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "SHOTGUN_FIRE_POOR", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "SILENCED_CROUCH_FIRE", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "SILENCED_FIRE", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "TEC_crouchfire", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "TEC_fire", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "UZI_crouchfire", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "UZI_fire", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "UZI_fire_POOR", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "idle_rocket", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "Rocket_Fire", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "run_rocket", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "walk_rocket", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "WALK_start_rocket", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "WEAPON_sniper", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "buddy_crouchfire", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "buddy_fire", fRapidFire);
			plugin::Command<plugin::Commands::SET_CHAR_ANIM_SPEED>(pPedSelf, "buddy_fire_POOR", fRapidFire);
		}
	}


	var->winapi.device_dx9 = self;

	if (var->winapi.device_dx9)
	{
		menu->initialize();
		menu->draw(self);
	}

	return menu->o_present(self, sourceRect, destRect, destWindowOverride, dirtyRegion);
}
bool showVehicleList = false;
HRESULT __stdcall h_reset(IDirect3DDevice9* self, D3DPRESENT_PARAMETERS* presentationParameters)
{
	menu->shutdown(true);
	HRESULT result = menu->o_reset(self, presentationParameters);
	menu->shutdown(false);
	return result;
}

BOOL WINAPI h_cursor(int pos_x, int pos_y)
{
	if (var->gui.is_open)
		return FALSE;

	return menu->o_cursor(pos_x, pos_y);
}

ImFont* Consolas = nullptr;

void c_menu::initialize()
{
	if (var->gui.initialized)
		return;

	var->winapi.device_dx9->GetCreationParameters(&var->winapi.device_par);
	var->winapi.wnd_proc = (WNDPROC)SetWindowLongW(var->winapi.device_par.hFocusWindow, GWL_WNDPROC, (LONG)h_wndproc);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.IniFilename = NULL;
	ImGui::StyleColorsDark();
	io.Fonts->AddFontFromMemoryTTF(museo500_binary, sizeof museo500_binary, 14);
	static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	ImFontConfig icons_config;
	icons_config.MergeMode = true;
	icons_config.PixelSnapH = true;
	io.Fonts->AddFontFromMemoryTTF(&font_awesome_binary, sizeof font_awesome_binary, 13, &icons_config, icon_ranges);

	io.Fonts->AddFontFromMemoryTTF(museo900_binary, sizeof museo900_binary, 28);
	ImGui_ImplWin32_Init(var->winapi.device_par.hFocusWindow);
	ImGui_ImplDX9_Init(var->winapi.device_dx9);
	ImGui_ImplDX9_InvalidateDeviceObjects();

	var->gui.initialized = true;
}

typedef bool(__cdecl* ptrAddProjectile)(CEntity* creator, eWeaponType weaponType, CVector posn, float force, CVector* direction, CEntity* victim);
ptrAddProjectile AddProjectile = (ptrAddProjectile)0x737C80;
CEntity* __stdcall GetLocalEntity(void)
{
	return ((CEntity * (__cdecl*)(int))0x56E120)(-1);

}

struct FriendPlayer {
	DWORD dwPlayer;
	PlayerESPMTA* espConfig;

	FriendPlayer(DWORD player, PlayerESPMTA* config)
		: dwPlayer(player), espConfig(config) {
	}
};

std::vector<FriendPlayer> friendPlayers;
void AddFriend(DWORD dwPlayer) {
	for (auto& friendPlayer : friendPlayers) {
		if (friendPlayer.dwPlayer == dwPlayer) {
			return;
		}
	}

	friendPlayers.emplace_back(dwPlayer, espg);
}

bool espfilledbox = true;
bool espweapon = true;
ImColor weaponColor;


#include <d3d9.h>
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx9.h"
#include "ImGui/imgui_impl_win32.h"
#include <xorstr.hpp>
static bool block_all_events = false;

inline void DrawCornerBox(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImColor color, float thickness = 2.0f, float length = 8.0f) {
	// Top left corner
	drawList->AddLine(topLeft, ImVec2(topLeft.x + length, topLeft.y), color, thickness);
	drawList->AddLine(topLeft, ImVec2(topLeft.x, topLeft.y + length), color, thickness);

	// Top right corner
	drawList->AddLine(ImVec2(bottomRight.x, topLeft.y), ImVec2(bottomRight.x - length, topLeft.y), color, thickness);
	drawList->AddLine(ImVec2(bottomRight.x, topLeft.y), ImVec2(bottomRight.x, topLeft.y + length), color, thickness);

	// Bottom left corner
	drawList->AddLine(ImVec2(topLeft.x, bottomRight.y), ImVec2(topLeft.x + length, bottomRight.y), color, thickness);
	drawList->AddLine(ImVec2(topLeft.x, bottomRight.y), ImVec2(topLeft.x, bottomRight.y - length), color, thickness);

	// Bottom right corner
	drawList->AddLine(bottomRight, ImVec2(bottomRight.x - length, bottomRight.y), color, thickness);
	drawList->AddLine(bottomRight, ImVec2(bottomRight.x, bottomRight.y - length), color, thickness);
}

CVector* stVisuals::getBonePosition(CPed* pPed, ePedBones bone, CVector* vecPosition)
{
	pPed->GetBonePosition(*(RwV3d*)vecPosition, bone, true);
	return vecPosition;
}

void DrawPlayerSkeleton(CPed* pPed, ImColor color, float thickness)
{
	if (!pPed || !pPed->IsAlive())
		return;

	constexpr int boneIDs[] = { 5, 4, 3, 2, 1 }; // Só tronco pra testar

	CVector vecBone[100]{};

	for (int iBone : boneIDs)
	{
		stVisuals::getBonePosition(pPed, (ePedBones)iBone, &vecBone[iBone]);
	}

	CVector headScreen;
	Utils::CalcScreenCoors(&vecBone[5], &headScreen);
	// Comente o teste para debug
	//if (headScreen.z < 1.0f)
	//    return;

	g_pImRender->D3DLine(vecBone[5], vecBone[4], color, thickness);
	g_pImRender->D3DLine(vecBone[4], vecBone[3], color, thickness);
	g_pImRender->D3DLine(vecBone[3], vecBone[2], color, thickness);
	g_pImRender->D3DLine(vecBone[2], vecBone[1], color, thickness);
}

void render(IDirect3DDevice9* self)
{
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(1920, 1080));
	ImGui::SetNextWindowBgAlpha(0.0f);
	ImGui::Begin("##overlay", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);

	CVector localPlayerPos;
	localPlayerPos.x = *(float*)(cclient + 0x140);
	localPlayerPos.y = *(float*)(cclient + 0x144);
	localPlayerPos.z = *(float*)(cclient + 0x148);

	DWORD dwLocalPlayer = m_nGame_sdk->player.GetPlayerByIndex(cclient, 0);

	for (unsigned int i = 0; i < m_nGame_sdk->player.GetPlayerPoolSize(cclient); i++)
	{
		DWORD dwPlayer = m_nGame_sdk->player.GetPlayerByIndex(cclient, i);

		if (stVisuals::bIgnoreSelf && dwPlayer == dwLocalPlayer)
			continue;

		CVector worldPos;
		worldPos.x = *(float*)(dwPlayer + 0x140);
		worldPos.y = *(float*)(dwPlayer + 0x144);
		worldPos.z = *(float*)(dwPlayer + 0x148);

		float distance = m_nGame_sdk->misc.CalculateDistance(localPlayerPos, worldPos);
		if ((int)Utils::getDistance(worldPos) > espg->iMaxPedDistance)
			continue;

		ImColor espColorName = espg->playerNameColor;
		ImColor espColorBox = espg->boxColor;
		ImColor espColorHealth = espg->healthColor;
		ImColor espColorArmor = espg->armorColor;
		ImColor espColorDistance = espg->distanceColor;

		float hpPercentage = espg->esphealth ? std::clamp(m_nGame_sdk->player.GetPlayerHealth(dwPlayer) / 100.0f, 0.0f, 1.0f) : 0.0f;
		float armorPercentage = espg->esparmor ? std::clamp(m_nGame_sdk->player.GetPlayerArmor(dwPlayer) / 100.0f, 0.0f, 1.0f) : 0.0f;

		CVector vecHead, vecFoot, vecHeadScreen, vecFootScreen;

		vecHead.x = worldPos.x;
		vecHead.y = worldPos.y;
		vecHead.z = worldPos.z + 1.0f;

		vecFoot.x = worldPos.x;
		vecFoot.y = worldPos.y;
		vecFoot.z = worldPos.z - 1.1f;

		Utils::CalcScreenCoors(&vecHead, &vecHeadScreen);
		Utils::CalcScreenCoors(&vecFoot, &vecFootScreen);

		if (vecHeadScreen.x < 0 || vecHeadScreen.x > 1920 || vecHeadScreen.y < 0 || vecHeadScreen.y > 1080)
			continue;

		float boxWidth = fabsf(vecFootScreen.y - vecHeadScreen.y) / 4.0f;

		ImVec2 topLeft(vecHeadScreen.x - boxWidth, vecHeadScreen.y);
		ImVec2 bottomRight(vecFootScreen.x + boxWidth, vecFootScreen.y);

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		// Filled Box
		if (espg->espfilledbox)
			drawList->AddRectFilled(topLeft, bottomRight, IM_COL32(0, 0, 0, 80));

		// Box
		if (espg->espboxes)
		{
			if (espg->useCornerBox)
				DrawCornerBox(drawList, topLeft, bottomRight, espColorBox, 2.0f, 8.0f);
			else
				drawList->AddRect(topLeft, bottomRight, espColorBox, 0.0f, 0, 2.0f);
		}

		// Snapline
		if (espg->espsnapline)
		{
			ImVec2 screenCenter = ImVec2(1920.0f / 2.0f, 1080.0f);
			drawList->AddLine(screenCenter, ImVec2(vecFootScreen.x, vecFootScreen.y), espColorBox, 1.5f);
		}

		// Skeleton
		if (espg->espskeleton)
		{
			CPed* pPed = (CPed*)dwPlayer;
			DrawPlayerSkeleton(pPed, espg->skeletonColor, espg->skeletonThickness);
		}

		// Distância
		if (espg->espdistance)
		{
			char szDistance[32];
			sprintf(szDistance, xorstr_("[ %.2f m ]"), Utils::getDistance(worldPos));

			CVector distancePos;
			distancePos.x = vecFootScreen.x;
			distancePos.y = vecFootScreen.y + 15.0f;
			distancePos.z = 0;

			g_pImRender->DrawString(pESPFont, szDistance, distancePos, 14.0f, espColorDistance, true);
		}

		// Nome
		if (espg->playername)
		{
			std::string playerName = m_nGame_sdk->player.GetPlayerName(dwPlayer);

			CVector namePos;
			namePos.x = vecHeadScreen.x;
			namePos.y = vecHeadScreen.y - 15.0f;
			namePos.z = 0;

			g_pImRender->DrawString(pESPFont, playerName.c_str(), namePos, 14.0f, espColorName, true);
		}

		// Ping
		if (espg->espping)
		{
			int ping = m_nGame_sdk->player.GetPlayerPing(dwPlayer);
			char szPing[32];
			sprintf(szPing, xorstr_("[ %d ms ]"), ping);

			CVector pingPos;
			pingPos.x = vecFootScreen.x;
			pingPos.y = vecFootScreen.y + 30.0f;
			pingPos.z = 0;

			g_pImRender->DrawString(pESPFont, szPing, pingPos, 14.0f, espg->pingColor, true);
		}

		// Health / Armor bars
		float barHeight = vecFootScreen.y - vecHeadScreen.y;
		float barPadding = 5.0f;

		for (int i = 0; i < 2; i++)
		{
			float percentage = i == 0 ? hpPercentage : armorPercentage;
			if (percentage <= 0.0f)
				continue;

			float currentHeight = barHeight * percentage;
			ImVec2 barBase(vecHeadScreen.x - boxWidth - barPadding - i * 4.0f, vecHeadScreen.y);

			drawList->AddRectFilled(
				ImVec2(barBase.x, barBase.y + (barHeight - currentHeight)),
				ImVec2(barBase.x + 2.0f, barBase.y + barHeight),
				i == 0
				? IM_COL32(espColorHealth.Value.x * 255, espColorHealth.Value.y * 255, espColorHealth.Value.z * 255, 255)
				: IM_COL32(espColorArmor.Value.x * 255, espColorArmor.Value.y * 255, espColorArmor.Value.z * 255, 255)
			);
		}
	}

	ImGui::End();
}

static bool openPacketWindow = false;
static bool openEventWindow = false;

TextEditor editor;
c_client cclientx;

DWORD ccore;
std::map<int, const char*> vehicleList = std::map<int, const char*>();

void InitializeVehicleList()
{
	vehicleList[577] = "AT-400";
	vehicleList[511] = "Beagle";
	vehicleList[512] = "Cropduster";
	vehicleList[593] = "Dodo";
	vehicleList[520] = "Hydra";
	vehicleList[553] = "Nevada";
	vehicleList[476] = "Rustler";
	vehicleList[519] = "Shamal";
	vehicleList[460] = "Skimmer";
	vehicleList[513] = "Stuntplane";
	vehicleList[548] = "Cargobob";
	vehicleList[417] = "Leviathan";
	vehicleList[487] = "Maverick";
	vehicleList[488] = "News Chopper";
	vehicleList[497] = "Police Maverick";
	vehicleList[563] = "Raindance";
	vehicleList[447] = "Seasparrow";
	vehicleList[469] = "Sparrow";
	vehicleList[472] = "Coastguard";
	vehicleList[473] = "Dinghy";
	vehicleList[493] = "Jetmax";
	vehicleList[595] = "Launch";
	vehicleList[484] = "Marquis";
	vehicleList[430] = "Predator";
	vehicleList[453] = "Reefer";
	vehicleList[452] = "Speeder";
	vehicleList[446] = "Squalo";
	vehicleList[454] = "Tropic";
	vehicleList[485] = "Baggage";
	vehicleList[431] = "Bus";
	vehicleList[438] = "Cabbie";
	vehicleList[437] = "Coach";
	vehicleList[574] = "Sweeper";
	vehicleList[420] = "Taxi";
	vehicleList[525] = "Towtruck";
	vehicleList[408] = "Trashmaster";
	vehicleList[552] = "Utility Van";
	vehicleList[416] = "Ambulance";
	vehicleList[433] = "Barracks";
	vehicleList[427] = "Enforcer";
	vehicleList[490] = "FBI Rancher";
	vehicleList[528] = "FBI Truck";
	vehicleList[407] = "Fire Truck";
	vehicleList[544] = "Fire Truck";
	vehicleList[523] = "HPV1000";
	vehicleList[470] = "Patriot";
	vehicleList[596] = "Police LS";
	vehicleList[598] = "Police LV";
	vehicleList[599] = "Police Ranger";
	vehicleList[597] = "Police SF";
	vehicleList[432] = "Rhino";
	vehicleList[601] = "S.W.A.T.";
	vehicleList[428] = "Securicar";
	vehicleList[499] = "Benson";
	vehicleList[609] = "Black Boxville";
	vehicleList[498] = "Boxville";
	vehicleList[524] = "Cement Truck";
	vehicleList[532] = "Combine Harvester";
	vehicleList[578] = "DFT-30";
	vehicleList[486] = "Dozer";
	vehicleList[406] = "Dumper";
	vehicleList[573] = "Dune";
	vehicleList[455] = "Flatbed";
	vehicleList[588] = "Hotdog";
	vehicleList[403] = "Linerunner";
	vehicleList[423] = "Mr.Whoopee";
	vehicleList[443] = "Packer";
	vehicleList[515] = "Roadtrain";
	vehicleList[510] = "Mountain Bike";
	vehicleList[514] = "Tanker";
	vehicleList[531] = "Tractor";
	vehicleList[456] = "Yankee";
	vehicleList[536] = "Blade";
	vehicleList[575] = "Broadway";
	vehicleList[534] = "Remington";
	vehicleList[567] = "Savanna";
	vehicleList[535] = "Slamvan";
	vehicleList[576] = "Tornado";
	vehicleList[412] = "Voodoo";
	vehicleList[402] = "Buffalo";
	vehicleList[542] = "Clover";
	vehicleList[603] = "Phoenix";
	vehicleList[475] = "Sabre";
	vehicleList[581] = "BF-400";
	vehicleList[400] = "Landstalker";
	vehicleList[509] = "Bike";
	vehicleList[481] = "BMX";
	vehicleList[462] = "Faggio";
	vehicleList[521] = "FCR-900";
	vehicleList[463] = "Freeway";
	vehicleList[522] = "NRG-500";
	vehicleList[461] = "PCJ-600";
	vehicleList[448] = "Pizza Boy";
	vehicleList[468] = "Sanchez";
	vehicleList[586] = "Wayfarer";
	vehicleList[602] = "Alpha";
	vehicleList[496] = "Blista Compact";
	vehicleList[401] = "Bravura";
	vehicleList[527] = "Cadrona";
	vehicleList[589] = "Club";
	vehicleList[419] = "Esperanto";
	vehicleList[587] = "Euros";
	vehicleList[533] = "Feltzer";
	vehicleList[526] = "Fortune";
	vehicleList[474] = "Hermes";
	vehicleList[545] = "Hustler";
	vehicleList[517] = "Majestic";
	vehicleList[410] = "Manana";
	vehicleList[600] = "Picador";
	vehicleList[436] = "Previon";
	vehicleList[439] = "Stallion";
	vehicleList[549] = "Tampa";
	vehicleList[491] = "Virgo";
	vehicleList[445] = "Admiral";
	vehicleList[604] = "Damaged Glendale";
	vehicleList[507] = "Elegant";
	vehicleList[585] = "Emperor";
	vehicleList[466] = "Glendale";
	vehicleList[492] = "Greenwood";
	vehicleList[546] = "Intruder";
	vehicleList[516] = "Nebula";
	vehicleList[467] = "Oceanic";
	vehicleList[426] = "Premier";
	vehicleList[547] = "Primo";
	vehicleList[405] = "Sentinel";
	vehicleList[580] = "Stafford";
	vehicleList[409] = "Stretch";
	vehicleList[550] = "Sunrise";
	vehicleList[566] = "Tahoma";
	vehicleList[540] = "Vincent";
	vehicleList[421] = "Washington";
	vehicleList[529] = "Willard";
	vehicleList[459] = "Berkley's RC Van";
	vehicleList[422] = "Bobcat";
	vehicleList[482] = "Burrito";
	vehicleList[605] = "Damaged Sadler";
	vehicleList[530] = "Forklift";
	vehicleList[418] = "Moonbeam";
	vehicleList[572] = "Mower";
	vehicleList[582] = "News Van";
	vehicleList[413] = "Pony";
	vehicleList[440] = "Rumpo";
	vehicleList[543] = "Sadler";
	vehicleList[583] = "Tug";
	vehicleList[478] = "Walton";
	vehicleList[554] = "Yosemite";
	vehicleList[429] = "Banshee";
	vehicleList[541] = "Bullet";
	vehicleList[415] = "Cheetah";
	vehicleList[480] = "Comet";
	vehicleList[562] = "Elegy";
	vehicleList[565] = "Flash";
	vehicleList[434] = "Hotknife";
	vehicleList[494] = "Hotring Racer";
	vehicleList[502] = "Hotring Racer 2";
	vehicleList[503] = "Hotring Racer 3";
	vehicleList[411] = "Infernus";
	vehicleList[559] = "Jester";
	vehicleList[561] = "Stratum";
	vehicleList[560] = "Sultan";
	vehicleList[506] = "Super GT";
	vehicleList[451] = "Turismo";
	vehicleList[558] = "Uranus";
	vehicleList[555] = "Windsor";
	vehicleList[477] = "ZR-350";
	vehicleList[441] = "RC Bandit";
	vehicleList[464] = "RC Baron";
	vehicleList[594] = "RC Cam";
	vehicleList[501] = "RC Goblin";
	vehicleList[465] = "RC Raider";
	vehicleList[564] = "RC Tiger";
	vehicleList[606] = "Baggage Trailer";
	vehicleList[607] = "Baggage Trailer";
	vehicleList[610] = "Farm Trailer";
	vehicleList[584] = "Petrol Trailer";
	vehicleList[611] = "Trailer";
	vehicleList[608] = "Trailer";
	vehicleList[435] = "Trailer 1";
	vehicleList[450] = "Trailer 2";
	vehicleList[591] = "Trailer 3";
	vehicleList[590] = "Box Freight";
	vehicleList[538] = "Brown Streak";
	vehicleList[570] = "Brown Streak Carriage";
	vehicleList[569] = "Flat Freight";
	vehicleList[537] = "Freight";
	vehicleList[449] = "Tram";
	vehicleList[568] = "Bandito";
	vehicleList[424] = "BF Injection";
	vehicleList[504] = "Bloodring Banger";
	vehicleList[457] = "Caddy";
	vehicleList[483] = "Camper";
	vehicleList[508] = "Journey";
	vehicleList[571] = "Kart";
	vehicleList[500] = "Mesa";
	vehicleList[444] = "Monster";
	vehicleList[556] = "Monster 2";
	vehicleList[557] = "Monster 3";
	vehicleList[471] = "Quadbike";
	vehicleList[495] = "Sandking";
	vehicleList[539] = "Vortex";
	vehicleList[579] = "Huntley";
	vehicleList[404] = "Perennial";
	vehicleList[489] = "Rancher";
	vehicleList[505] = "Rancher";
	vehicleList[479] = "Regina";
	vehicleList[442] = "Romero";
	vehicleList[458] = "Solair";
}
std::string ToLowerCase(const std::string& str) {
	std::string result = str;
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}

static const char* weapon_names[] = {
	"Unarmed", "Brass Knuckle", "Golf Club", "Nightstick", "Knife", "Baseball Bat", "Shovel", "Pool Cue",
	"Katana", "Chainsaw", "Dildo 1", "Dildo 2", "Vibrator 1", "Vibrator 2", "Flowers", "Cane", "Grenade",
	"Tear Gas", "Molotov", "Rocket", "Rocket HS", "Freefall Bomb", "Pistol", "Silenced Pistol", "Desert Eagle",
	"Shotgun", "Sawnoff", "SPAS-12", "Micro Uzi", "MP5", "AK-47", "M4", "TEC-9", "Country Rifle", "Sniper Rifle",
	"Rocket Launcher", "Rocket Launcher HS", "Flamethrower", "Minigun", "Satchel Charge", "Detonator", "Spraycan",
	"Extinguisher", "Camera", "Nightvision", "Infrared", "Parachute", "Unused", "Armour", "Flare"
};

static int selected_weapon = 0;

void ActivateInfiniteAmmo() {

	CPed* pPlayerPed = CWorld::Players[0].m_pPed;

	if (pPlayerPed) {
		int activeWeaponSlot = pPlayerPed->m_nSelectedWepSlot;

		if (typeAmmo == 1) {
			pPlayerPed->m_aWeapons[activeWeaponSlot].m_nAmmoInClip = valueAmmo;
		}

		if (typeAmmo == 2) {
			pPlayerPed->m_aWeapons[activeWeaponSlot].m_nAmmoTotal = valueAmmo;
		}
	}
}

void ModifyHandling(int type) {

	CPed* pPlayerPed = CWorld::Players[0].m_pPed;

	if (!pPlayerPed) {
		return;
	}

	CVehicle* tVehicle = pPlayerPed->m_pVehicle;
	if (!tVehicle) {
		return;
	}

	if (type == 1) {
		tVehicle->m_fMass = Mass;
	}
	if (type == 2) {
		tVehicle->m_fBreakPedal = BreakPedal;
	}
	if (type == 3) {
		tVehicle->m_fTurnMass = TurnMass;
	}
	if (type == 4) {
		tVehicle->m_fMovingSpeed = MovingSpeed;
	}
	if (type == 5) {
		tVehicle->m_fGasPedal = GasPedal;
	}
	if (type == 6) {
		tVehicle->m_fWheelScale = WheelScale;
	}
	if (type == 7) {
		tVehicle->m_fMass = 1500.0f;
		tVehicle->m_fBreakPedal = 1.0f;
		tVehicle->m_fTurnMass = 500.0f;
		tVehicle->m_fMovingSpeed = 20.0f;
		tVehicle->m_fGasPedal = 1.0f;
		tVehicle->m_fWheelScale = 1.0f;

	}
}

void TeleportToPosition(float x, float y, float z) {
	CPlayerPed* player = FindPlayerPed();
	player->SetPosn(CVector(x, y, z));
}

bool bStickLoopEnabled = false;
std::thread stickLoopThread;

void TeleportPlayer(float x, float y, float z) {
	CPed* pPed = FindPlayerPed();
	if (pPed) {
		pPed->SetPosn(x, y, z);
	}

	CVehicle* pVeh = FindPlayerVehicle(-1, false);
	if (pVeh) {
		pVeh->SetPosn(x, y, z);
	}
}

void LaunchPlayerToSky(DWORD dwPlayer) {
	if (!dwPlayer) return;

	// Offset comum de velocidade (m_vecMoveSpeed)
	CVector* vecSpeed = (CVector*)(dwPlayer + 0xC0);

	if (vecSpeed) {
		vecSpeed->x = 0.0f;
		vecSpeed->y = 0.0f;
		vecSpeed->z = 25.0f; // Velocidade vertical alta (lança pra cima)
	}
}

void tick_counter_update()
{
	client->currentTime = GetTickCount();
}

struct CCameraCustom {
	char pad1[0x30];      // até m_nActiveCam
	int m_nActiveCam;
	char pad2[0x10];      // até m_asCams
	struct {
		CVector Source;   // posição da câmera
		CVector Front;    // direção da câmera
		CVector Up;       // orientação da câmera
		char pad[0x24];
	} m_asCams[2];
};

bool bSpectateEnabled = false;
std::thread spectateThread;
// Adicione onde você guarda variáveis
bool bHighlightSelectedPlayer = false;
bool bIsFavoritePlayer = false;
std::string favoritePlayerName = "";

void CopyToClipboard(const std::string& text) {
	if (OpenClipboard(nullptr)) {
		EmptyClipboard();
		HGLOBAL hGlob = GlobalAlloc(GMEM_FIXED, text.size() + 1);
		if (hGlob) {
			memcpy(hGlob, text.c_str(), text.size() + 1);
			SetClipboardData(CF_TEXT, hGlob);
		}
		CloseClipboard();
	}
}

void c_menu::draw(IDirect3DDevice9* self)
{
	if (element->binds.menu_bind != 0 && GetAsyncKeyState(element->binds.menu_bind) & 1)
	{
		var->gui.is_open = !var->gui.is_open;
	}

	if (element->binds.airbreak_bind != 0 && GetAsyncKeyState(element->binds.airbreak_bind) & 1)
	{
		bAirBreak = !bAirBreak;
	}

	if (b_camera_hack)
	{
		TheCamera.LerpFOV(TheCamera.FindCamFOV(), 120.0f, 100, true);
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGuiIO& io = ImGui::GetIO();

	static bool last_cursor_state = false;
	if (var->gui.is_open != last_cursor_state)
	{
		last_cursor_state = var->gui.is_open;

		if (var->gui.is_open)
		{
			io.MouseDrawCursor = true;
			ShowCursor(TRUE);
			ClipCursor(nullptr);
		}
		else
		{
			io.MouseDrawCursor = false;
			while (ShowCursor(FALSE) >= 0);
			ClipCursor(nullptr);
		}
	}

	ImGui::NewFrame();

	cclient = (DWORD)GetModuleHandle("client.dll");
	if (cclient) {
		render(self); // ESSENCIAL
		g_aimbot->AimAtPed(cclient);
		m_pSdk->Render(self);
		InitializeVehicleList();
		if (stVisuals::bEnableESP) {
			stVisuals::Draw();
		}
	}

	if (var->gui.is_open)
	{
		mylogonigga(self);

		static bool bools[50]{};
		static int ints[50]{}, combo = 0;
		static char buf[64];
		static float color[4] = { 1.f, 1.f, 1.f, 1.f };

		PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Hello, world!", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
		{
			auto io = ImGui::GetIO();
			auto window = GetCurrentWindow();
			auto draw = window->DrawList;
			auto pos = window->Pos;
			auto size = window->Size;
			auto style = GetStyle();

			gui.m_anim = ImLerp(gui.m_anim, 1.f, 0.045f);
			SetWindowSize(ImVec2(905, 500)); // Mantém o tamanho total fixo

			// HEADER (avatar + título animado)
			const char* text = "Fex Menu - discord.gg/ZZqSEBfV9W";
			float font_size = io.Fonts->Fonts[1]->FontSize;

			ImVec2 avatar_size = ImVec2(48, 48);
			ImVec2 avatar_pos = pos + ImVec2(15, 15);

			draw->AddImageRounded(
				menutexture,
				avatar_pos,
				avatar_pos + avatar_size,
				ImVec2(0, 0), ImVec2(1, 1),
				ImColor(1.f, 1.f, 1.f, 1.f),
				avatar_size.x
			);

			ImVec2 base_pos = avatar_pos + ImVec2(avatar_size.x + 10, (avatar_size.y - font_size) / 2);

			// Animated title gradient
			ImVec4 start = gui.header_gradient_start.to_im_color().Value;
			ImVec4 end = gui.header_gradient_end.to_im_color().Value;

			float time = ImGui::GetTime();
			float fade_factor = (sinf(time * 2.0f) + 1.0f) / 2.0f;
			ImVec4 animated_start = ImLerp(start, end, fade_factor);
			ImVec4 animated_end = ImLerp(end, start, fade_factor);

			float text_width = io.Fonts->Fonts[1]->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text).x;
			float x = base_pos.x;

			for (int i = 0; text[i] != '\0'; ++i) {
				char c[2] = { text[i], '\0' };
				float char_width = io.Fonts->Fonts[1]->CalcTextSizeA(font_size, FLT_MAX, 0.0f, c).x;
				float t = (x - base_pos.x) / text_width;

				ImVec4 color;
				color.x = animated_start.x + (animated_end.x - animated_start.x) * t;
				color.y = animated_start.y + (animated_end.y - animated_start.y) * t;
				color.z = animated_start.z + (animated_end.z - animated_start.z) * t;
				color.w = 1.0f;

				draw->AddText(io.Fonts->Fonts[1], font_size, ImVec2(x, base_pos.y), ImColor(color), c);
				x += char_width;
			}
			// TABS (barra lateral com altura exata para caber todas sem sobra)
			SetCursorPos(ImVec2(10, 70));
			BeginChild("##tabs", ImVec2(150, 0), false, ImGuiWindowFlags_NoScrollbar);

			gui.group_title("Local");
			if (gui.tab(ICON_FA_CROSSHAIRS, "Aimbot", gui.m_tab == 0) && gui.m_tab != 0) gui.m_tab = 0, gui.m_anim = 0.f;
			if (gui.tab(ICON_FA_ID_BADGE, "LocalPlayer", gui.m_tab == 1) && gui.m_tab != 1) gui.m_tab = 1, gui.m_anim = 0.f;
			if (gui.tab(ICON_FA_CAR, "Vehicle", gui.m_tab == 2) && gui.m_tab != 2) gui.m_tab = 2, gui.m_anim = 0.f;

			Spacing(); Spacing(); Spacing();

			gui.group_title("Visuals");
			if (gui.tab(ICON_FA_USERS, "Players", gui.m_tab == 3) && gui.m_tab != 3) gui.m_tab = 3, gui.m_anim = 0.f;
			if (gui.tab(ICON_FA_CAR_SIDE, "Vehicles", gui.m_tab == 4) && gui.m_tab != 4) gui.m_tab = 4, gui.m_anim = 0.f;

			Spacing(); Spacing(); Spacing();

			gui.group_title("Lua");
			if (gui.tab(ICON_FA_CODE, "Lua Executor", gui.m_tab == 5) && gui.m_tab != 5) gui.m_tab = 5, gui.m_anim = 0.f;
			if (gui.tab(ICON_FA_FOLDER_OPEN, "Resources Manager", gui.m_tab == 6) && gui.m_tab != 6) gui.m_tab = 6, gui.m_anim = 0.f;
			if (gui.tab(ICON_FA_COGS, "Config", gui.m_tab == 11) && gui.m_tab != 11) gui.m_tab = 11, gui.m_anim = 0.f;

			EndChild();

			// RIGHT SIDE CHILD
			PushStyleVar(ImGuiStyleVar_Alpha, gui.m_anim);
			PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
			SetCursorPos(ImVec2(185, 81 - (5 * gui.m_anim)));
			BeginChild("##childs", ImVec2(size.x - 200, size.y - 96));

			// Aqui continuam seus switches dos conteúdos das abas
			switch (gui.m_tab) {
				//conteudo funções etc
			case 0:

				gui.group_box(ICON_FA_CROSS " Normal Aim", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 400)); {
					ImGui::Checkbox("Enable Aimbot", &g_aimbot->enable_aimbot);
					ImGui::Checkbox("Show Fov", &g_aimbot->view_fov);
					ImGui::Checkbox("Visible Check", &g_aimbot->aimbot_visible);
					ImGui::Checkbox("Ignore Death", &g_aimbot->aimbot_death);
					//ImGui::SliderFloat("Smooth Value", &g_aimbot->smooth_percent, 1.0f, 10.0f);
					ImGui::SliderFloat("Fov Value", &g_aimbot->fovValue, 1.0f, 800.0f);
					ImGui::Text("Select Weapon:");
					ImGui::SetNextItemWidth(250);
					ImGui::Combo("##WeaponCombo", &selected_weapon, weapon_names, IM_ARRAYSIZE(weapon_names));

					if (ImGui::Button("Clear Weapon", ImVec2(GetWindowWidth(), 25)))
						RemoveAllWeapons();

					if (ImGui::Button("Weapon Pack", ImVec2(GetWindowWidth(), 25)))
						if (element->content.loaded_client)
						{
							DWORD dwOffset = 0x4385B0;
							__asm { call dwOffset }
						}

					ImGui::InputInt("Pull Ammo", &valueAmmo);
					if (ImGui::Button("Set in clip", ImVec2(GetWindowWidth(), 25))) {
						typeAmmo = 1;
						ActivateInfiniteAmmo();
					}
					ImGui::SameLine();
					if (ImGui::Button("Set Ammo")) {
						typeAmmo = 2;
						ActivateInfiniteAmmo();
					}

					if (ImGui::Button("Pull", ImVec2(GetWindowWidth(), 25))) {
						unsigned int weapon_id = static_cast<unsigned int>(selected_weapon);
						CStreaming::RequestModel(weapon_id, 2);
						CStreaming::LoadAllRequestedModels(false);
						pPedSelf->GiveWeapon(static_cast<eWeaponType>(weapon_id), 1, true);
						pPedSelf->SetCurrentWeapon(static_cast<eWeaponType>(weapon_id));
						CStreaming::SetModelIsDeletable(weapon_id);
					}

				} gui.end_group_box();

				SameLine();

				gui.group_box(ICON_FA_SUPERSCRIPT " Silent Aim", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 400)); {
					ImGui::Checkbox("Enable Silent", &element->cnetapi.SilentAim);
					ImGui::Checkbox("Show Silent Fov", &element->cnetapi.SilentAimFovDraw);
					ImGui::SliderInt("Fov Value", &element->cnetapi.bFovSilentLineSize, 1, 890);
					ImGui::SliderFloat("Max Distance", &element->cnetapi.maxDistance, 1.0f, 5000.0f);
					ImGui::SliderFloat("HitKill Damage", &element->cnetapi.maxdamage, 1.0f, 5000.0f);
					ImGui::Checkbox("Enable HitKill", &element->cnetapi.HitKillInPlayer);
					ImGui::Checkbox("Lag Server On Fire", &element->cnetapi.LagServer);
					ImGui::Checkbox("Pull on shoot", &element->cnetapi.killall);
					ImGui::Checkbox("One Shoot", &element->cnetapi.OneShot);
					ImGui::Checkbox("DamagerX2", &element->cnetapi.DamagerX2);
					ImGui::Checkbox("DamagerX5", &element->cnetapi.DamagerX5);
					ImGui::Checkbox("Rapid Fire", &bRapidFire);
					if (bRapidFire)
						ImGui::SliderFloat("Fire Speed", &fRapidFire, 1.0f, 50.0f);

					if (ImGui::Checkbox("Fast Crosshair", &bFastCrosshair))
						FastCrosshair();

					if (ImGui::Checkbox("Infinite Ammo", &Infammo))
					{
						bNoReload = !bNoReload;
						NoReloadXD();
					}

					if (ImGui::Checkbox("No Recoil", &bNoRecoil))
						NoRecoil();

				} gui.end_group_box();

				break;

			case 1:

				gui.group_box(ICON_FA_GAMEPAD " Exploits", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 400)); {
					ImGui::Checkbox("AirBreak", &bAirBreak);
					ImGui::SameLine();
					ImGui::Text("Key:");
					ImGui::SameLine();

					if (element->binds.airbreak_bind == 0)
						element->binds.airbreak_bind = VK_SHIFT;

					if (element->binds.is_waiting_for_airbreak)
					{
						ImGui::Button("Press a key...");
						for (int key = 0x01; key <= 0xFE; key++)
						{
							if (GetAsyncKeyState(key) & 0x8000)
							{
								element->binds.airbreak_bind = key;
								element->binds.is_waiting_for_airbreak = false;
								break;
							}
						}
					}
					else
					{
						std::string abLabel = GetKeyName(element->binds.airbreak_bind);
						if (ImGui::Button(abLabel.c_str()))
						{
							element->binds.is_waiting_for_airbreak = true;
						}
					}

					if (bAirBreak)
						ImGui::SliderFloat("AirBreak Speed", &fSpeedair, 0.1f, 100.0f, "Speed: %.2f");

					ImGui::Checkbox("Anti Stun", &bAntiStun);
					if (bAntiStun)
						ImGui::SliderInt("AntiStun Chance", &iAntiStunChance, 1, 100);

					ImGui::Checkbox("Fast Sprint", &bFastSprint);
					if (bFastSprint)
						ImGui::SliderFloat("Sprint Speed", &fFastSprint, 1.0f, 10.0f, "Speed: %.2f");

					ImGui::Checkbox("Infinity Run", &bInfiniteRun);
					ImGui::Checkbox("Infinity Oxygen", &bInfiniteOxygen);
					ImGui::Checkbox("Godmode", &b_player_godmode);
					ImGui::Checkbox("Semi Godmode", &bGodmode);

					if (ImGui::Checkbox("High Jump", &SuperPuloActived))
					{
						if (element->content.loaded_client)
							ToggleHighJump();
					}

					ImGui::Checkbox("Close No Fall", &bNoFall);

					ImGui::Checkbox("Game Speed", &Velocidade);
					if (Velocidade && element->content.loaded_client)
					{
						ImGui::SliderFloat("Speed Value", &fSpeed, 1.0f, 1000.0f, "Speed: %.2f");
						*(float*)Adds_Speed = fSpeed;
					}

					if (ImGui::Button("Full Health", ImVec2(GetWindowWidth(), 25)))
					{
						if (element->content.loaded_client)
						{
							CPlayerPed* pPedSelf = FindPlayerPed();
							if (pPedSelf)
								pPedSelf->m_fHealth = 100.0f;
						}
					}

					float buttonWidth = (GetWindowWidth() - 10.0f) / 2; // divide a largura da janela em 2 botões, com espaçamento
					float buttonHeight = 20.0f;

					if (ImGui::Button("Suicide", ImVec2(buttonWidth, buttonHeight)))
					{
						if (element->content.loaded_client)
						{
							CPlayerPed* pPedSelf = FindPlayerPed();
							if (pPedSelf)
								pPedSelf->m_fHealth = 0.0f;
						}
					}
					ImGui::SameLine();
					if (ImGui::Button("Full Armour", ImVec2(buttonWidth, buttonHeight)))
					{
						if (element->content.loaded_client)
						{
							CPlayerPed* pPedSelf = FindPlayerPed();
							if (pPedSelf)
								pPedSelf->m_fArmour = 100.0f;
						}
					}

					if (ImGui::Button("Remove Armor", ImVec2(GetWindowWidth(), 25)))
					{
						if (element->content.loaded_client)
						{
							CPlayerPed* pPedSelf = FindPlayerPed();
							if (pPedSelf)
								pPedSelf->m_fArmour = 0.0f;
						}
					}

					ImGui::Checkbox("Show Player List", &listadenigga);
					if (listadenigga)
					{
						// Bem mais largo e mais baixo
						ImGui::SetNextWindowSize(ImVec2(700, 350), ImGuiCond_Always);

						ImGui::Begin("Player List - Manager", nullptr,
							ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);

						// Lado esquerdo (lista)
						ImGui::BeginChild("##ConfigLeft", ImVec2(250, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
						{
							static char searchText[256] = "";
							ImGui::InputText("Search", searchText, IM_ARRAYSIZE(searchText));

							ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

							for (unsigned int i = 0; i < m_nGame_sdk->player.GetPlayerPoolSize(cclient); i++)
							{
								DWORD dwPlayer = m_nGame_sdk->player.GetPlayerByIndex(cclient, i);
								if (!dwPlayer) continue;

								std::string playerName = m_nGame_sdk->player.GetPlayerName(dwPlayer);
								if (strlen(searchText) > 0 && playerName.find(searchText) == std::string::npos)
									continue;

								if (ImGui::Selectable(playerName.c_str(), selectedPlayerIndex == i))
									selectedPlayerIndex = i;
							}
						}
						ImGui::EndChild();

						ImGui::SameLine();

						// Lado direito (ações no player)
						ImGui::BeginChild("##ConfigRight", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
						{
							if (selectedPlayerIndex != -1)
							{
								DWORD dwSelectedPlayer = m_nGame_sdk->player.GetPlayerByIndex(cclient, selectedPlayerIndex);
								if (dwSelectedPlayer)
								{
									std::string selectedPlayerName = m_nGame_sdk->player.GetPlayerName(dwSelectedPlayer);
									CVector selectedPlayerPos;
									selectedPlayerPos.x = *(float*)(dwSelectedPlayer + 0x140);
									selectedPlayerPos.y = *(float*)(dwSelectedPlayer + 0x144);
									selectedPlayerPos.z = *(float*)(dwSelectedPlayer + 0x148);

									ImGui::Text("Player: %s", selectedPlayerName.c_str());
									ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

									// 🌟 Extras
									if (ImGui::Button("Copy Name"))
									{
										CopyToClipboard(selectedPlayerName);
									}
									ImGui::SameLine();
									if (ImGui::Button("Copy Position"))
									{
										char buffer[128];
										sprintf(buffer, "%.2f, %.2f, %.2f", selectedPlayerPos.x, selectedPlayerPos.y, selectedPlayerPos.z);
										CopyToClipboard(buffer);
									}

									ImGui::SameLine();
									if (ImGui::Button(bIsFavoritePlayer ? "Unmark Favorite" : "Mark Favorite"))
									{
										if (!bIsFavoritePlayer)
										{
											favoritePlayerName = selectedPlayerName;
											bIsFavoritePlayer = true;
										}
										else
										{
											favoritePlayerName = "";
											bIsFavoritePlayer = false;
										}
									}

									ImGui::SameLine();
									if (ImGui::Button(bHighlightSelectedPlayer ? "Unhighlight" : "Highlight"))
									{
										bHighlightSelectedPlayer = !bHighlightSelectedPlayer;
									}

									ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

									// ✅ Seus já existentes
									if (ImGui::Button("Try Teleport"))
									{
										selectedPlayerPos.x += 2.0f;
										CPed* pPedSelf = FindPlayerPed();
										if (pPedSelf)
										{
											pPedSelf->SetPosn(selectedPlayerPos.x, selectedPlayerPos.y, selectedPlayerPos.z);
											CVehicle* pVehicle = FindPlayerVehicle(-1, false);
											if (pVehicle)
												pVehicle->SetPosn(selectedPlayerPos.x, selectedPlayerPos.y, selectedPlayerPos.z);
										}
									}

									ImGui::SameLine();
									if (ImGui::Button("Try Attack"))
									{
										CVector dir(1250.0f, -1350.0f, 14.0f);
										AddProjectile(GetLocalEntity(), WEAPONTYPE_GRENADE, selectedPlayerPos, 15.0f, &dir, nullptr);
									}

									ImGui::SameLine();
									if (ImGui::Button("Try Slap"))
									{
										CVector dir(1250.0f, -1350.0f, 14.0f);
										AddProjectile(GetLocalEntity(), WEAPONTYPE_TEARGAS, selectedPlayerPos, 15.0f, &dir, nullptr);
										Sleep(50);
										AddProjectile(GetLocalEntity(), WEAPONTYPE_TEARGAS, selectedPlayerPos, 15.0f, &dir, nullptr);
									}

									ImGui::SameLine();
									if (ImGui::Button("Try Fire"))
									{
										CVector dir(1250.0f, -1350.0f, 14.0f);
										AddProjectile(GetLocalEntity(), WEAPONTYPE_MOLOTOV, selectedPlayerPos, 15.0f, &dir, nullptr);
									}

									if (ImGui::Button("Add Friend"))
										AddFriend(dwSelectedPlayer);

									ImGui::SameLine();
									if (ImGui::Button(bStickLoopEnabled ? "Disable StickPlayer" : "Stick Player"))
									{
										if (!bStickLoopEnabled)
										{
											bStickLoopEnabled = true;
											stickLoopThread = std::thread([=]() {
												while (bStickLoopEnabled)
												{
													CVector pos;
													pos.x = *(float*)(dwSelectedPlayer + 0x140) + 2.0f;
													pos.y = *(float*)(dwSelectedPlayer + 0x144);
													pos.z = *(float*)(dwSelectedPlayer + 0x148);

													CPed* pPedSelf = FindPlayerPed();
													if (pPedSelf) {
														pPedSelf->SetPosn(pos.x, pos.y, pos.z);
														CVehicle* pVehicle = FindPlayerVehicle(-1, false);
														if (pVehicle)
															pVehicle->SetPosn(pos.x, pos.y, pos.z);
													}

													std::this_thread::sleep_for(std::chrono::milliseconds(30));
												}
												});
										}
										else
										{
											bStickLoopEnabled = false;
											if (stickLoopThread.joinable())
												stickLoopThread.join();
										}
									}

									ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

									// ✅ Infos extras
									ImGui::Text("Health: %.2f", m_nGame_sdk->player.GetPlayerHealth(dwSelectedPlayer));
									ImGui::Text("Armor: %.2f", m_nGame_sdk->player.GetPlayerArmor(dwSelectedPlayer));
									ImGui::Text("Ping: %d", m_nGame_sdk->player.GetPlayerPing(dwSelectedPlayer));
									ImGui::Text("Pos: X %.2f Y %.2f Z %.2f", selectedPlayerPos.x, selectedPlayerPos.y, selectedPlayerPos.z);

									// ✅ Você pode também checar arma
									DWORD weaponID = *(DWORD*)(dwSelectedPlayer + 0xAC); // Verifique se esse offset está certo
									ImGui::Text("Weapon ID: %u", weaponID);
								}
							}

						}
						ImGui::EndChild();

						ImGui::End();
					}




				} gui.end_group_box();

				SameLine();

				gui.group_box(ICON_FA_PLAY " Player", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 400)); {
					ImGui::Checkbox("Change Weather", &bWeatherEnabled);
					if (bWeatherEnabled)
					{
						ImGui::SliderFloat("Weather", &ClimaGameValue, 1.0f, 20.0f);
						if (ClimaGameValue != 16 && element->content.loaded_client)
						{
							*ClimaGame = ClimaGameValue;
						}
					}

					if (ImGui::Checkbox("Ghost mode", &GhostBool))
					{
						if (element->content.loaded_client)
							GhostMode(GhostBool, false);
					}

					ImGui::Checkbox("Skin Changer", &bChangeSkin);
					if (bChangeSkin)
					{
						ImGui::SliderInt("Skin ID", &playerSkinID, 0, 311);
						if (ImGui::Button("Set", ImVec2(ImGui::GetContentRegionAvail().x, 33)))
						{
							if (element->content.loaded_client)
								ChangeSkin();
						}
					}

					if (ImGui::Checkbox("Invisible", &invisivel))
					{
						if (element->content.loaded_client)
						{
							DWORD dwClientModuleBase = (DWORD)GetModuleHandle("client.dll");
							DWORD dwPlayer = m_nGame_sdk->player.GetPlayerByIndex(dwClientModuleBase, 0);

							oSetAlpha(dwPlayer, invisivel ? 0 : 255);
							GhostMode(invisivel, false);
						}
					}

					if (ImGui::Checkbox("Night Vision", &VisaoNoturnaAtivada))
					{
						if (element->content.loaded_client)
							*(DWORD*)VISAO_NOTURNA_ADDR = VisaoNoturnaAtivada ? 1 : 0;
					}

					ImGui::Checkbox("Fast Rotation", &VerificationAtrasoMovimento);

					if (ImGui::Checkbox("FreeCam", &bFreeCam))
					{
						if (element->content.loaded_client)
						{
							DWORD dwClientModuleBase = (DWORD)GetModuleHandle("client.dll");
							DWORD dwPlayer = m_nGame_sdk->player.GetPlayerByIndex(dwClientModuleBase, 0);

							bAirBreak = bFreeCam;

							oSetAlpha(dwPlayer, bFreeCam ? 0 : 255);
							GhostMode(bFreeCam, false);
						}
					}

					ImGui::Checkbox("Camera Hack", &b_camera_hack);

					if (ImGui::Button("Jetpack", ImVec2(GetWindowWidth(), 25)))
					{
						if (CPedd != nullptr && *CPedd != 0)
						{
							DWORD addr = 0x439600;
							__asm call addr;
						}
					}

					if (ImGui::Button("Main Street", ImVec2(GetWindowWidth(), 25))) TeleportToPosition(1223, -1425, 14);
					if (ImGui::Button("Skate Park", ImVec2(GetWindowWidth(), 25))) TeleportToPosition(1873, -1392, 14);
					if (ImGui::Button("Airport", ImVec2(GetWindowWidth(), 25))) TeleportToPosition(1970, -2200, 14);
					//if (ImGui::Button("CJ House", ImVec2(GetWindowWidth(), 25))) TeleportToPosition(2483, -1663, 14);
					//if (ImGui::Button("Wooden Pier", ImVec2(GetWindowWidth(), 25))) TeleportToPosition(840, -1818, 13);

					float buttonWidth = (GetWindowWidth() - 10.0f) / 2; // divide a largura entre os 2 botões
					float buttonHeight = 20.0f;

					if (ImGui::Button("CJ Hospital", ImVec2(buttonWidth, buttonHeight)))
						TeleportToPosition(2022, -1414, 17);

					ImGui::SameLine();

					if (ImGui::Button("Los Santos Hospital", ImVec2(buttonWidth, buttonHeight)))
						TeleportToPosition(1174, -1323, 14);

				} gui.end_group_box();

				break;


			case 2:
				gui.group_box("Vehicle Tools", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 400)); {
					if (ImGui::Button("Repair Engine", ImVec2(GetWindowWidth(), 25))) {
						CPed* pPlayerPed = CWorld::Players[0].m_pPed;

						if (!pPlayerPed) {
							return;
						}

						CVehicle* tVehicle = pPlayerPed->m_pVehicle;
						if (!tVehicle) {
							return;
						}
						tVehicle->m_fHealth = 1000.0f;
					}

					if (ImGui::Button("Set Engine on Fire", ImVec2(GetWindowWidth(), 25))) {
						if (element->content.loaded_client)
						{
							DWORD CCar = *(DWORD*)0xB6F980;
							*(float*)(CCar + 1216) = 0.0f;
						}
					}

					if (ImGui::Button("Turn On Vehicle", ImVec2(GetWindowWidth(), 25))) {
						CPed* pPlayerPed = CWorld::Players[0].m_pPed;

						if (!pPlayerPed) {
							return;
						}

						CVehicle* tVehicle = pPlayerPed->m_pVehicle;
						if (!tVehicle) {
							return;
						}
						tVehicle->bEngineOn = 1;
					}

					if (ImGui::Button("Turn Off Vehicle", ImVec2(GetWindowWidth(), 25))) {
						if (element->content.loaded_client)
						{
							DWORD* TurnOffVehicle = (DWORD*)((*CVehiclee) + 0x428);
							*TurnOffVehicle = 0;
						}
					}

					if (ImGui::Button("Unlock Vehicle", ImVec2(GetWindowWidth(), 25))) {
						if (element->content.loaded_client)
						{
							DWORD* veiculoBase = (DWORD*)(*CVehiclee + 0x47C);
							if (*veiculoBase != 0)
							{
								DWORD* veiculoIntermediario = (DWORD*)(*veiculoBase + 0x120);
								if (*veiculoIntermediario != 0)
								{
									DWORD* estadoTrava = (DWORD*)(*veiculoIntermediario + 0x4F8);
									*estadoTrava = 1;
								}
							}
						}
					}

					if (ImGui::Button("Lock Vehicle", ImVec2(GetWindowWidth(), 25))) {
						if (element->content.loaded_client)
						{
							DWORD* veiculoBase = (DWORD*)(*CVehiclee + 0x47C);
							if (*veiculoBase != 0)
							{
								DWORD* veiculoIntermediario = (DWORD*)(*veiculoBase + 0x120);
								if (*veiculoIntermediario != 0)
								{
									DWORD* estadoTrava = (DWORD*)(*veiculoIntermediario + 0x4F8);
									*estadoTrava = 2;
								}
							}
						}
					}

					if (ImGui::Button("Control Romote", ImVec2(GetWindowWidth(), 25))) remotecar();

					if (ImGui::Button("Remove Colision", ImVec2(GetWindowWidth(), 25))) VehicleRemoveCollison();

					if (ImGui::Button("Delete ALL Vehicles", ImVec2(GetWindowWidth(), 25)))
					{
						if (element->content.loaded_client)
						{
							for (unsigned int i = 1; i < m_nGame_sdk->vehicles.GetVehiclePoolSize(cclient); i++)
							{
								DWORD veh = m_nGame_sdk->vehicles.GetVehicleByIndex(cclient, i);
								client->hkWarpPedIntoVeh(m_nGame_sdk->player.GetPlayerByIndex(cclient, 0), veh, 0);
								client->hkBlowJob(veh, VehicleBlowFlags());
								client->hkRemoveFromDrogas(m_nGame_sdk->player.GetPlayerByIndex(cclient, 0));
							}
						}
					}

				if (ImGui::Button("Fuck Position Unoccupied Vehicle", ImVec2(GetWindowWidth(), 25)))
				{
					if (element->content.loaded_client)
						SetCarTotal();
				}

				if (ImGui::Button("Destroy Unoccupied Vehicle", ImVec2(GetWindowWidth(), 25)))
				{
					if (element->content.loaded_client)
						SetCarTotal_2(false);
				}
				

				} gui.end_group_box();

				SameLine();

				gui.group_box("Vehicle Settings", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 400)); {
					if (ImGui::Button("Add Nitro", ImVec2(GetWindowWidth(), 25))) {
						if (element->content.loaded_client) {
							*NitroVeiculo = NitroActived ? 0 : 1;
							NitroActived = !NitroActived;
						}
					}

					if (ImGui::Button("Repair Wheels", ImVec2(GetWindowWidth(), 25))) {
						if (element->content.loaded_client)
						{
							CVehicle* pVehicle = reinterpret_cast<CVehicle*>(*CVehiclee);

							if (pVehicle && !IsBike(pVehicle->m_nModelIndex))
							{
								DWORD* Pneu11 = (DWORD*)((uintptr_t)pVehicle + 0x5A5);
								DWORD* Pneu22 = (DWORD*)((uintptr_t)pVehicle + 0x5A6);
								DWORD* Pneu33 = (DWORD*)((uintptr_t)pVehicle + 0x5A7);
								DWORD* Pneu44 = (DWORD*)((uintptr_t)pVehicle + 0x5A8);
								*Pneu11 = 0;
								*Pneu22 = 0;
								*Pneu33 = 0;
								*Pneu44 = 0;
							}
						}
					}

					if (ImGui::Button("Drilling wheels", ImVec2(GetWindowWidth(), 25))) {
						if (element->content.loaded_client)
						{
							CVehicle* pVehicle = reinterpret_cast<CVehicle*>(*CVehiclee);

							if (pVehicle && !IsBike(pVehicle->m_nModelIndex))
							{
								DWORD* Pneu11 = (DWORD*)((uintptr_t)pVehicle + 0x5A5);
								DWORD* Pneu22 = (DWORD*)((uintptr_t)pVehicle + 0x5A6);
								DWORD* Pneu33 = (DWORD*)((uintptr_t)pVehicle + 0x5A7);
								DWORD* Pneu44 = (DWORD*)((uintptr_t)pVehicle + 0x5A8);
								*Pneu11 = 1;
								*Pneu22 = 1;
								*Pneu33 = 1;
								*Pneu44 = 1;
							}
						}
					}

					if (ImGui::Button("Fly Unoccupied Vehicle", ImVec2(GetWindowWidth(), 25)))
					{
						if (element->content.loaded_client)
							SetCarTotal_3();
					}

					static float messageTime = -1.0f;
					static bool invalidModel = false;

					ImGui::InputText("Vehicle Model ID", vehicleModel, IM_ARRAYSIZE(vehicleModel));
					if (invalidModel)
					{
						if (ImGui::GetTime() - messageTime < 2.0f)
							ImGui::Text("Invalid Model ID");
						else
							invalidModel = false;
					}

					if (ImGui::Button("Set Vehicle Model", ImVec2(GetWindowWidth(), 25)))
					{
						DWORD player = m_nGame_sdk->player.GetPlayerByIndex(cclient, 0);
						DWORD veh = m_nGame_sdk->vehicles.GetVehicleNearestByPlayer(cclient, player, 10.f);

						if (veh && player)
						{
							int modelId = atoi(vehicleModel);

							if (modelId >= 400 && modelId <= 611)
							{
								if (element->content.loaded_client)
									cclientx.SetChangeModel(veh, modelId, 0, 0);
							}
							else
							{
								invalidModel = true;
								messageTime = ImGui::GetTime();
							}
						}
					}

					ImGui::Checkbox("Bike Jump", &hugeBunnyHopCheck);
					if (element->content.loaded_client) {
						DWORD* HugeBunnyHop = (DWORD*)0x969161;
						*HugeBunnyHop = hugeBunnyHopCheck ? 1 : 0;
					}

					ImGui::Checkbox("Handling Vehicle", &HandlingVehicleBool);
					if (HandlingVehicleBool && element->content.loaded_client) {
						ImGui::Begin("Vehicle Handling - Manager");

						ImGui::SliderFloat("m_fMass", &Mass, 1.0f, 15000.0f);
						ImGui::SameLine();
						if (ImGui::Button("Apply##Mass", ImVec2(262, 26))) {
							typeHandling = 1;
							ModifyHandling(typeHandling);
						}

						ImGui::SliderFloat("m_fBreakPedal", &BreakPedal, 100.0f, 10000.0f);
						ImGui::SameLine();
						if (ImGui::Button("Apply##BreakPedal", ImVec2(262, 26))) {
							typeHandling = 2;
							ModifyHandling(typeHandling);
						}

						ImGui::SliderFloat("m_fTurnMass", &TurnMass, 500.0f, 10000.0f);
						ImGui::SameLine();
						if (ImGui::Button("Apply##TurnMass", ImVec2(262, 26))) {
							typeHandling = 3;
							ModifyHandling(typeHandling);
						}

						ImGui::SliderFloat("m_fMovingSpeed", &MovingSpeed, 1.0f, 400.0f);
						ImGui::SameLine();
						if (ImGui::Button("Apply##MovingSpeed", ImVec2(262, 26))) {
							typeHandling = 4;
							ModifyHandling(typeHandling);
						}

						ImGui::SliderFloat("m_fGasPedal", &GasPedal, 50.0f, 700.0f);
						ImGui::SameLine();
						if (ImGui::Button("Apply##GasPedal", ImVec2(262, 26))) {
							typeHandling = 5;
							ModifyHandling(typeHandling);
						}

						ImGui::SliderFloat("m_fWheelScale", &WheelScale, 1.0f, 100.0f);
						ImGui::SameLine();
						if (ImGui::Button("Apply##WheelScale", ImVec2(262, 26))) {
							typeHandling = 6;
							ModifyHandling(typeHandling);
						}

						if (ImGui::Button("Default Handling", ImVec2(262, 26))) {
							Mass = 1500.0f;
							BreakPedal = 1.0f;
							TurnMass = 500.0f;
							MovingSpeed = 20.0f;
							GasPedal = 1.0f;
							WheelScale = 1.0f;
							ModifyHandling(7);
						}

						ImGui::End();
					}

					ImGui::Checkbox("Insane Handling", &insanehandlingcheck);
					if (element->content.loaded_client) {
						DWORD* InsaneHandling = (DWORD*)0x96914C;
						*InsaneHandling = insanehandlingcheck ? 1 : 0;
					}

					ImGui::Checkbox("Car Can Drive on Water", &carsDriveOnWaterCheck);
					if (element->content.loaded_client) {
						DWORD* CarsDriveOnWater = (DWORD*)0x969152;
						*CarsDriveOnWater = carsDriveOnWaterCheck ? 1 : 0;
					}

					ImGui::Checkbox("Car Can Fly", &carsCanFlyCheck);
					if (element->content.loaded_client) {
						DWORD* CarsCanFly = (DWORD*)0x969160;
						*CarsCanFly = carsCanFlyCheck ? 1 : 0;
					}

					ImGui::Checkbox("Vehicle Godmode", &VerificationBulletproofEngine);
					if (VerificationBulletproofEngine) {
						DWORD CCar = *(DWORD*)0xB6F980;
						*(float*)(CCar + 1216) = 100000000000.0f;
					}

					ImGui::Checkbox("Show Vehicle List", &showVehicleList);
					if (showVehicleList)
					{
						// Mesmo tamanho da player list
						ImGui::SetNextWindowSize(ImVec2(700, 350), ImGuiCond_Always);

						ImGui::Begin("Vehicle List - Manager", nullptr,
							ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);

						// Lado esquerdo (lista com busca)
						ImGui::BeginChild("##ConfigLeft", ImVec2(250, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
						{
							static char searchVehicleText[256] = "";
							ImGui::InputText("Search", searchVehicleText, IM_ARRAYSIZE(searchVehicleText));

							ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

							for (unsigned int i = 0; i < m_nGame_sdk->vehicles.GetVehiclePoolSize(cclient); i++)
							{
								DWORD dwVehicle = m_nGame_sdk->vehicles.GetVehicleByIndex(cclient, i);
								if (!dwVehicle) continue;

								int vehicleID = m_nGame_sdk->vehicles.GetCarID(dwVehicle);
								std::string vehicleName = "Unknown";
								if (vehicleList.find(vehicleID) != vehicleList.end()) {
									vehicleName = vehicleList[vehicleID];
								}

								if (strlen(searchVehicleText) > 0 && ToLowerCase(vehicleName).find(ToLowerCase(searchVehicleText)) == std::string::npos)
									continue;

								if (ImGui::Selectable((vehicleName + " (ID: " + std::to_string(vehicleID) + ")").c_str(), selectedVehicleIndex == i))
									selectedVehicleIndex = i;
							}
						}
						ImGui::EndChild();

						ImGui::SameLine();

						// Lado direito (ações no veículo)
						ImGui::BeginChild("##ConfigRight", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
						{
							if (selectedVehicleIndex != -1)
							{
								DWORD dwSelectedVehicle = m_nGame_sdk->vehicles.GetVehicleByIndex(cclient, selectedVehicleIndex);
								if (dwSelectedVehicle)
								{
									int vehicleID = m_nGame_sdk->vehicles.GetCarID(dwSelectedVehicle);
									std::string selectedVehicleName = "Unknown";
									auto it = vehicleList.find(vehicleID);
									if (it != vehicleList.end()) {
										selectedVehicleName = it->second;
									}

									CVector selectedVehiclePos;
									selectedVehiclePos.x = *(float*)(dwSelectedVehicle + 0x140);
									selectedVehiclePos.y = *(float*)(dwSelectedVehicle + 0x144);
									selectedVehiclePos.z = *(float*)(dwSelectedVehicle + 0x148);

									ImGui::Text("Vehicle: %s", selectedVehicleName.c_str());
									ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

									if (ImGui::Button("Copy Position"))
									{
										char buffer[128];
										sprintf(buffer, "%.2f, %.2f, %.2f", selectedVehiclePos.x, selectedVehiclePos.y, selectedVehiclePos.z);
										CopyToClipboard(buffer);
									}

									ImGui::SameLine();
									if (ImGui::Button("Teleport"))
									{
										selectedVehiclePos.x += 2.0f;
										CPed* pPedSelf = FindPlayerPed();
										if (pPedSelf)
										{
											pPedSelf->SetPosn(selectedVehiclePos.x, selectedVehiclePos.y, selectedVehiclePos.z);
											CVehicle* pVehicle = FindPlayerVehicle(-1, false);
											if (pVehicle)
												pVehicle->SetPosn(selectedVehiclePos.x, selectedVehiclePos.y, selectedVehiclePos.z);
										}
									}

									ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
									ImGui::Text("Pos: X %.2f Y %.2f Z %.2f", selectedVehiclePos.x, selectedVehiclePos.y, selectedVehiclePos.z);
									ImGui::Text("Vehicle ID: %d", vehicleID);
								}
							}
						}
						ImGui::EndChild();

						ImGui::End();
					}


					const char* boostModes[] = { "Off", "Car Fucker 1", "Car Fucker 2 (Shift)" };
					ImGui::Combo("Car Fucker Mode", &element->buzina_boost.boostMode, boostModes, IM_ARRAYSIZE(boostModes));
					ImGui::SliderFloat("Boost Force", &element->buzina_boost.maxSpeed, 1.0f, 100.0f);

				} gui.end_group_box();

				break;
			case 3:

				gui.group_box(ICON_FA_USER " Players", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 400)); {
					ImGui::Checkbox("LocalPlayer", &espg->showLocalPlayer);
					ImGui::Checkbox("Enable ESP Boxes", &espg->espboxes);
					ImGui::Checkbox("Use Corner Box", &espg->useCornerBox);
					ImGui::Checkbox("Filled Box Background", &espg->espfilledbox);
					ImGui::Checkbox("ESP Snapline", &espg->espsnapline);
					//ImGui::Checkbox("ESP Player Names", &espg->playername);
					//ImGui::Checkbox("ESP Distance", &espg->espdistance);
					ImGui::Checkbox("ESP Health Bar", &espg->esphealth);
					ImGui::Checkbox("ESP Armor Bar", &espg->esparmor);
					//ImGui::Checkbox("ESP Ping", &espg->espping);
					ImGui::Checkbox(("ESP Skeleton"), &stVisuals::bBoneESP);
					ImGui::SliderInt("Max ESP Distance", &espg->iMaxPedDistance, 10, 500);

				} gui.end_group_box();
				SameLine();
				gui.group_box("Esp Color", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 400)); {

				} gui.end_group_box();


				break;
			case 4:

				gui.group_box(ICON_FA_CAR " Vehicles", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 400)); {

					ImGui::Checkbox(("Enable ESP"), &stVisuals::bEnableVehiclesESP);
					ImGui::Checkbox(("Show Name"), &stVisuals::bVehicleNameTagsESP);
					ImGui::Checkbox(("Show Health"), &stVisuals::bVehicleHPESP);
					ImGui::Checkbox(("Show Engine"), &stVisuals::bVehicleEngineESP);
					ImGui::Checkbox(("Show Snapline"), &stVisuals::bVehicleSnaplineStyle);
					ImGui::Checkbox(("Show Status"), &stVisuals::bVehicleStatusESP);
					ImGui::Checkbox(("Show Distance"), &stVisuals::bVehicleDistanceESP);
					ImGui::Checkbox(("Show Bounding Box"), &stVisuals::bVehicleBoundingBoxESP);
					ImGui::Checkbox(("Show Filled Box"), &stVisuals::bVehicleFilled);
					ImGui::Combo(("Snapline Style"), &stVisuals::iVehicleSnaplineStyle, ("Above\0Down\0"));
					ImGui::SliderFloat(("Snapline Thickness"), &stVisuals::fVehicleESPThickness, 1.f, 3.f);
					ImGui::SliderInt(("Max Vehicle Distance"), &stVisuals::iMaxVehicleDistance, 10, 7000, ("%d m"));
					ImGui::ColorEdit4(("Vehicle Color"), (float*)&stVisuals::vehicleColor);

				} gui.end_group_box();

				SameLine();

				gui.group_box("Esp Color", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 400)); {


				} gui.end_group_box();

				break;
			case 5:
				gui.group_box(ICON_FA_CODE " Lua Executor", ImVec2(GetWindowWidth() - GetStyle().ItemSpacing.x, 400));
				{
					static bool showMessage = true;
					static TextEditor luaTextEditor;

					luaTextEditor.SetShowWhitespaces(false);
					luaTextEditor.SetReadOnly(false);
					luaTextEditor.SetPalette(TextEditor::GetDarkPalette());
					luaTextEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());

					if (showMessage) {
						luaTextEditor.SetText(xorstr_("outputChatBox(7331)"));
						showMessage = false;
					}

					ImVec2 availableSize = ImGui::GetContentRegionAvail();

					luaTextEditor.Render("##LuaEditor", ImVec2(availableSize.x, availableSize.y - 60.0f));

					ImGui::Spacing();

					if (ImGui::Button("execute", ImVec2(availableSize.x, 20.0f)))
					{
						std::string luaScript = luaTextEditor.GetText();

						if (element->content.loaded_client && !luaScript.empty())
						{
							if (element->executor.item_current >= 0 &&
								element->executor.item_current < element->executor.resources_list.size())
							{
								const auto& selected_resource = element->executor.resources_list[element->executor.item_current];
								var->Send_Script_Packet = true;

								client->load_code(
									selected_resource.resource_name.c_str(),
									xorstr_("client.lua"),
									luaScript.c_str(),
									luaScript.length()
								);
							}
						}
					}
				}
				gui.end_group_box();

				break;
			case 6:
			{
				gui.group_box(ICON_FA_FOLDER_OPEN " Scripts e Arquivos", ImVec2(GetWindowWidth() - GetStyle().ItemSpacing.x, 400));
				{
					ImGui::InputTextWithHint(" script search", "Find script...", element->resource.search_input, sizeof(element->resource.search_input));

					ImGui::BeginChild("script list", ImVec2(0, 160), true);

					if (element->resource.resources_list.empty()) {
						ImGui::TextDisabled("Nenhum resource encontrado.");
						element->resource.item_current = -1;
					}
					else {
						for (int i = 0; i < element->resource.resources_list.size(); ++i) {
							const auto& res = element->resource.resources_list[i];

							if (strlen(element->resource.search_input) > 0 &&
								std::string(res.resource_name).find(element->resource.search_input) == std::string::npos) {
								continue;
							}

							bool isSelected = (i == element->resource.item_current);
							if (ImGui::Selectable(res.resource_name.c_str(), isSelected)) {
								element->resource.item_current = i;
							}

							if (isSelected)
								ImGui::SetItemDefaultFocus();
						}
					}

					ImGui::EndChild();

					if (element->resource.item_current >= 0 && element->resource.item_current < element->resource.resources_list.size()) {
						const auto& selected_resource = element->resource.resources_list[element->resource.item_current];
						ImGui::Text("Resource selecionado: %s", selected_resource.resource_name.c_str());

						ImGui::Separator();
						ImGui::Spacing();

						if (ImGui::Button("Stop Resource", ImVec2(-1, 0))) {
							if (element->content.loaded_client && selected_resource.resource_ptr != nullptr) {
								client->stop_resource(selected_resource.resource_ptr);
								element->resource.item_current = -1;
							}
						}

						if (ImGui::Button("View Selected Script", ImVec2(-1, 0))) {
							if (element->content.loaded_client) {
								bool found = false;
								for (const auto& scr : scriptList) {
									if (scr.name == selected_resource.resource_name) {
										scriptViewerEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
										scriptViewerEditor.SetText(scr.content);
										showScriptViewerWindow = true;
										found = true;
										break;
									}
								}

								if (!found) {
									printf("[DEBUG] Script não encontrado em scriptList: %s\n", selected_resource.resource_name.c_str());
								}
							}
						}
						if (showScriptViewerWindow) {
							ImGui::SetNextWindowSize(ImVec2(650, 500), ImGuiCond_FirstUseEver);
							ImGui::Begin("Script Viewer", &showScriptViewerWindow, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

							ImGui::Text("Conteúdo do script selecionado:");
							ImGui::Separator();

							scriptViewerEditor.Render("##ScriptViewerEditor", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 50));

							ImGui::Spacing();
							if (ImGui::Button("Fechar", ImVec2(120, 0))) {
								showScriptViewerWindow = false;
							}

							ImGui::End();
						}

						if (ImGui::Button("Dump All Resources", ImVec2(-1, 0))) {
							if (element->content.loaded_client) {
								DumpScripts();
							}
						}

						if (openPacketWindow) {
							ImGui::Begin("Packet Manipulation", &openPacketWindow);

							static int selectedPacketIndex = -1;
							static std::vector<size_t> displayedPackets;

							ImGui::Text("Packet Manipulation");

							const auto& packets = packet->GetPackets();
							static int selectedAction[100];
							const char* actions[] = { "None", "Resend" };

							static int resendTimes[100];
							const char* resendOptions[] = { "1x", "10x", "50x", "100x", "1000x" };
							int resendValues[] = { 1, 10, 50, 100, 1000 };

							if (displayedPackets.size() != packets.size()) {
								displayedPackets.clear();
								for (size_t i = 0; i < packets.size(); i++) {
									displayedPackets.push_back(i);
								}
							}

							ImGui::BeginChild("Packets", ImVec2(0, 200), true);
							for (size_t i = 0; i < displayedPackets.size(); i++) {
								const auto& m_packet = packets[displayedPackets[i]];
								ImGui::PushID(static_cast<int>(i));

								bool isSelected = (selectedPacketIndex == i);
								if (ImGui::Selectable(("Packet ID: " + std::to_string(m_packet.packetId)).c_str(), isSelected)) {
									selectedPacketIndex = i;
								}

								ImGui::SameLine();
								ImGui::Text("| Time: %.8s", m_packet.timestamp.c_str() + 11);

								bool blocked = packet->IsBlocked(m_packet.packetId);
								ImGui::SameLine();
								if (ImGui::Checkbox("Block", &blocked)) {
									packet->block(m_packet.packetId, blocked);
								}

								ImGui::Separator();
								ImGui::PopID();
							}
							ImGui::EndChild();

							if (ImGui::Button("Clear")) {
								displayedPackets.clear();
								for (size_t i = 0; i < packets.size(); i++) {
									displayedPackets.push_back(i);
								}
							}

							ImGui::Separator();

							ImGui::Text("Selected Packet Tools");
							if (selectedPacketIndex >= 0 && selectedPacketIndex < packets.size()) {
								const auto& selectedPacket = packets[displayedPackets[selectedPacketIndex]];
								ImGui::Text("Selected Packet ID: %d", selectedPacket.packetId);

								bool isBlocked = packet->IsBlocked(selectedPacket.packetId);
								if (ImGui::Checkbox("Block Packet", &isBlocked)) {
									packet->block(selectedPacket.packetId, isBlocked);
								}

								if (ImGui::Button("Print Raw Content")) {
									// packet->printRaw(displayedPackets[selectedPacketIndex]);
								}
							}
							else {
								ImGui::Text("No packet selected.");
							}

							ImGui::End();
						}

						float buttonWidth = (GetWindowWidth() - 10.0f) / 2; // divide a largura da janela em 2 botões, com espaçamento
						float buttonHeight = 20.0f;

						if (ImGui::Button("Open Packet Manipulation", ImVec2(buttonWidth, buttonHeight)))
						{
							openPacketWindow = true;
						}
						ImGui::SameLine();
						if (ImGui::Button("Open Event Manipulation", ImVec2(buttonWidth, buttonHeight)))
						{
							openEventWindow = true;
						}
						if (openEventWindow) {
							ImGui::Begin("Event Manipulation", &openEventWindow);

							static int selectedEventIndex = -1;
							static std::vector<std::pair<std::string, std::chrono::system_clock::time_point>> capture_events;

							ImGui::Text("Event Manipulation");

							ImGui::BeginChild("EventList", ImVec2(0, 200), true);
							for (const auto& event : element->event.events_list) {
								if (!event.event_name.empty()) {
									capture_events.push_back({ event.event_name, std::chrono::system_clock::now() });
								}
							}
							element->event.events_list.clear();

							for (size_t i = 0; i < capture_events.size(); ++i) {
								ImGui::PushID(static_cast<int>(i));

								bool isSelected = (selectedEventIndex == static_cast<int>(i));
								if (ImGui::Selectable(capture_events[i].first.c_str(), isSelected)) {
									selectedEventIndex = static_cast<int>(i);
								}

								ImGui::SameLine();
								auto time = std::chrono::system_clock::to_time_t(capture_events[i].second);
								std::stringstream ss;
								ss << std::put_time(std::localtime(&time), "%H:%M:%S");
								ImGui::TextDisabled("| %s", ss.str().c_str());

								ImGui::SameLine(ImGui::GetWindowWidth() - 60);
								if (ImGui::Button("Resend", ImVec2(50, 0))) {
									packet->resendByEvent(capture_events[i].first, capture_events[i].second);
								}

								if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
									ImGui::SetClipboardText(capture_events[i].first.c_str());
								}

								ImGui::Separator();
								ImGui::PopID();
							}
							ImGui::EndChild();

							if (ImGui::Button("Clear", ImVec2(-1, 30))) {
								capture_events.clear();
								element->event.events_list.clear();
							}

							ImGui::Separator();

							ImGui::Text("Selected Event Tools");
							if (selectedEventIndex >= 0 && selectedEventIndex < capture_events.size()) {
								const auto& selectedEvent = capture_events[selectedEventIndex];
								ImGui::Text("Selected Event: %s", selectedEvent.first.c_str());

								if (ImGui::Button("Resend 1x", ImVec2(120, 30))) {
									packet->resendByEvent(selectedEvent.first, selectedEvent.second);
								}

								if (ImGui::Button("Copy to Clipboard", ImVec2(120, 30))) {
									ImGui::SetClipboardText(selectedEvent.first.c_str());
								}

								auto time = std::chrono::system_clock::to_time_t(selectedEvent.second);
								std::stringstream ss;
								ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
								ImGui::TextDisabled("Captured at: %s", ss.str().c_str());
							}
							else {
								ImGui::Text("No event selected.");
							}

							ImGui::End();
						}

					}
					else {
						ImGui::TextDisabled("Nenhum resource selecionado.");
					}
				}
				gui.end_group_box();
				break;
			}

			case 11:

				gui.group_box(ICON_FA_USER " Players", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 400)); {
					//ImGui::Checkbox("Bypass World Special property", &element->info.bypass_world);
					ImGui::Checkbox("Bypass DebugHooks", &element->info.bypass_lua);
					ImGui::Checkbox("Stop All TriggerServerEvents", &block_all_events);
					ImGui::Checkbox("Anti-ScreenShot", &element->info.bypass_screenshot);
					ImGui::Checkbox("ScreenShot Alert", &element->info.Admin_alert);
					ImGui::Checkbox("Show Injector", &showInjectorWindow);
					if (showInjectorWindow)
					{
						ImGui::Begin("Injector DLL", &showInjectorWindow, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

						ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "be careful with the dll you will inject!");

						ImGui::Spacing();

						ImGui::Text("DLL Path:");
						ImGui::InputText("##dllPath", dllPath, sizeof(dllPath));

						if (ImGui::Button("Inject DLL"))
						{
							std::string dllPathStr(dllPath);
							bool result = myInjector.InjectDll(xorstr_("gta_sa.exe"), dllPathStr);

							if (result)
							{
								injectionSuccess = true;
								messageTime = ImGui::GetTime();
							}
							else
							{
								injectionSuccess = false;
								messageTime = ImGui::GetTime();
							}
						}

						if (messageTime >= 0.0f && ImGui::GetTime() - messageTime < 2.0f)
						{
							if (injectionSuccess)
							{
								ImGui::Text(xorstr_("DLL injected successfully"));
							}
							else
							{
								ImGui::Text(xorstr_("Failed to inject DLL."));
							}
						}

						ImGui::End();
					}
					ImGui::Text(("Cheat Key:"));
					ImGui::SameLine();

					if (element->binds.menu_bind == 0) {
						element->binds.menu_bind = VK_DELETE;
					}

					if (element->binds.is_waiting_for_key)
					{
						ImGui::Button(("Press a key..."));
						for (int key = 0x01; key <= 0xFE; key++)
						{
							if (GetAsyncKeyState(key) & 0x8000)
							{
								element->binds.menu_bind = key;
								element->binds.is_waiting_for_key = false;
								break;
							}
						}
					}
					else
					{
						std::string buttonLabel = ("Current Key: ") + GetKeyName(element->binds.menu_bind);
						if (ImGui::Button(buttonLabel.c_str()))
						{
							element->binds.is_waiting_for_key = true;
						}
					}
					if (ImGui::Button("Explode All Players", ImVec2(GetWindowWidth(), 25)))
					if (element->content.loaded_client)
					{
						CVector TargetPos;
						DWORD pedPoolUsageInfo = *(DWORD*)0xB74490;
						DWORD pedPoolBegining = *(DWORD*)pedPoolUsageInfo;
						DWORD byteMapAddr = *(DWORD*)(pedPoolUsageInfo + 4);

						for (BYTE i = 1; i < 140; i++) {
							BYTE activityStatus = *(BYTE*)(byteMapAddr + i);

							if (activityStatus > 0 && activityStatus < 128) {
								DWORD CPed = (pedPoolBegining + i * 1988);
								DWORD Matrix = *(DWORD*)(CPed + 0x14);

								TargetPos.x = *(float*)(Matrix + 0x30);
								TargetPos.y = *(float*)(Matrix + 0x34);
								TargetPos.z = *(float*)(Matrix + 0x38);

								float health = *(float*)(CPed + 0x540);
								if (health > 0.f)
								{
									CVector direction(1250.0f, -1350.0f, 14.0f);

									AddProjectile(GetLocalEntity(), WEAPONTYPE_GRENADE, TargetPos, 500.0f, &direction, nullptr);
									AddProjectile(GetLocalEntity(), WEAPONTYPE_MOLOTOV, TargetPos, 500.0f, &direction, nullptr);
									AddProjectile(GetLocalEntity(), WEAPONTYPE_ROCKET, TargetPos, 500.0f, &direction, nullptr);
									AddProjectile(GetLocalEntity(), WEAPONTYPE_TEARGAS, TargetPos, 500.0f, &direction, nullptr);
									AddProjectile(GetLocalEntity(), WEAPONTYPE_FREEFALL_BOMB, TargetPos, 500.0f, &direction, nullptr);
									AddProjectile(GetLocalEntity(), WEAPONTYPE_DETONATOR, TargetPos, 500.0f, &direction, nullptr);
									AddProjectile(GetLocalEntity(), WEAPONTYPE_ROCKET_HS, TargetPos, 500.0f, &direction, nullptr);
								}
								}
							}
					}
					if (ImGui::Button("Close MTA:SA", ImVec2(GetWindowWidth(), 25)))
					{
						CloseGTA();
					}

					if (block_all_events) {
						if (element->content.loaded_client) {
							for (auto& event : element->event.events_list) {
								event.is_blocked = true;
							}
						}
					}

					if (ImGui::Button("Halloween", ImVec2(GetWindowWidth(), 25))) { gui.set_theme(1); }

					if (ImGui::Button("Xmas2025", ImVec2(GetWindowWidth(), 25))) { gui.set_theme(2); }

					if (ImGui::Button("Cyber", ImVec2(GetWindowWidth(), 25))) { gui.set_theme(3); }

					if (ImGui::Button("Candy", ImVec2(GetWindowWidth(), 25))) { gui.set_theme(4); }

					if (ImGui::Button("Full Black", ImVec2(GetWindowWidth(), 25))) { gui.set_theme(5); }


				} gui.end_group_box();
				SameLine();
				gui.group_box("Esp Color", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 400)); {
					std::string server_name = xorstr_("not connected");
					std::string serial = serial123();
					unsigned int totalPlayers = 0;
					unsigned int totalVehicles = 0;

					if (element->content.loaded_client) {
						std::string sn = netc->o_get_connected_server(netc->c_net_manager, true);
						if (!sn.empty())
							server_name = sn;

						totalPlayers = m_nGame_sdk->player.GetPlayerPoolSize(cclient);
						totalVehicles = m_nGame_sdk->vehicles.GetVehiclePoolSize(cclient);
					}

					ImGui::Text(xorstr_("Server Connected: %s"), server_name.c_str());

					ImGui::Separator();

					ImGui::Text(xorstr_("Serial: %s"), serial.c_str());

					ImGui::Separator();

					if (totalPlayers == 0) {
						ImGui::Text(xorstr_("No players online."));
					}
					else {
						ImGui::Text(xorstr_("Total Players: %u"), totalPlayers);
					}

					ImGui::Separator();

					if (totalVehicles == 0) {
						ImGui::Text(xorstr_("No vehicles in the server."));
					}
					else {
						ImGui::Text(xorstr_("Total Vehicles: %u"), totalVehicles);
					}

					if (!serial_loaded) {
						char savedSerial[33] = { 0 };
						load_serial_ini(savedSerial, sizeof(savedSerial));
						if (strlen(savedSerial) > 0) {
							strcpy_s(netc->serial_to_spoof, sizeof(netc->serial_to_spoof), savedSerial);
							netc->updated_serial = true;
						}
						serial_loaded = true;
					}

					ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "%s Serial Changer", ICON_FA_PLUS);

					if (netc->serial_to_spoof && strlen(netc->serial_to_spoof) > 0) {
						ImGui::Spacing();
						ImGui::TextWrapped("%s", netc->serial_to_spoof);
					}

					ImGui::Spacing();

					float buttonWidth = (ImGui::GetContentRegionAvail().x - 10) * 0.5f;

					if (ImGui::Button("Generate New Serial", ImVec2(buttonWidth, 0))) {
						netc->updated_serial = true;

						char newSerial[33] = { 0 };
						generate_random_serial(newSerial, sizeof(newSerial));
						strcpy_s(netc->serial_to_spoof, sizeof(netc->serial_to_spoof), newSerial);

						if (saveSerialEnabled) {
							save_serial_ini(newSerial);
							save_serial_to_list(newSerial);
						}

						generate_new_serial();
					}

					ImGui::SameLine();

					if (ImGui::Button("Reset To Default", ImVec2(buttonWidth, 0))) {
						netc->updated_serial = false;
						generate_new_serial();
					}

					if (ImGui::Button("Clean All", ImVec2(GetWindowWidth(), 25))) {
						for (int i = 0; i < 3; ++i) {
							std::wstring mta_path = CannabisHookM->getRegistryValue(L"SOFTWARE\\WOW6432Node\\Multi Theft Auto: San Andreas All\\1.6", L"Last Run Location") + L"\\mods\\deathmatch\\resources";
							std::string path_tostr(mta_path.begin(), mta_path.end());
							std::string processName = ByPcn("WmiPrvSE.exe");
							std::wstring wProcessName(processName.begin(), processName.end());
							CannabisHookM->StopProcess(wProcessName);
							CannabisHookM->PcnMortes();

							std::string directories[] = {
								ByPcn("C:\\ProgramData\\NT"),
								ByPcn("C:\\ProgramData\\NT2"),
								ByPcn("C:\\ProgramData\\MTA San Andreas All"),
								ByPcn("C:\\Program Files (x86)\\MTA San Andreas 1.6\\mods\\deathmatch\\priv")
							};
							for (const auto& directory : directories) {
								CannabisHookM->PcnRandam(directory);
							}
							std::wstring keys[] = {
								ByPcn(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID2"),
								ByPcn(L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Connections"),
								ByPcn(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FeatureUsage\\AppSwitched"),
								ByPcn(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FeatureUsage\\ShowJumpView"),
								ByPcn(L"SOFTWARE\\WOW6432Node\\Multi Theft Auto: San Andreas All\\1.6\\Settings\\diagnostics"),
								ByPcn(L"SOFTWARE\\WOW6432Node\\Multi Theft Auto: San Andreas All\\1.6\\Settings\\general"),
								ByPcn(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Multi Theft Auto: San Andreas All\\1.6\\Settings")
							};
							for (const auto& key : keys) {
								CannabisHookM->PcnNaldoBenny(key);
							}
							Sleep(10);
							CannabisHookM->PcnDa160(ByPcn("C:\\ProgramData\\MTA San Andreas All"));
							CannabisHookM->PcnDa160(ByPcn("C:\\ProgramData\\MTA San Andreas All\\Common"));
							CannabisHookM->PcnRandam(path_tostr);
							CannabisHookM->PcnDa160(path_tostr);
							CannabisHookM->PcnMaracutaia(path_tostr);
						}
					}

					if (ImGui::Button("Change Internet", ImVec2(GetWindowWidth(), 25))) {
						SpoofAllNetwork();
					}

					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();
					ImGui::Text("Saved Serials:");
					ImGui::SameLine();

					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - (ImGui::GetFontSize() / 2.5f));

					std::string buttonText = std::string("Save Serial (") + (saveSerialEnabled ? "On" : "Off") + ")";
					if (ImGui::Button(buttonText.c_str())) {
						saveSerialEnabled = !saveSerialEnabled;
					}

					std::vector<std::string> savedSerials = load_serial_list();
					for (const auto& serial : savedSerials) {
						ImGui::PushID(serial.c_str());

						ImGui::TextWrapped("%s", serial.c_str());
						ImGui::SameLine();

						if (ImGui::Button("Load")) {
							strcpy_s(netc->serial_to_spoof, sizeof(netc->serial_to_spoof), serial.c_str());
							if (saveSerialEnabled) {
								save_serial_ini(serial.c_str());
							}
							netc->updated_serial = true;
							generate_new_serial();
						}

						ImGui::SameLine();

						if (ImGui::Button("Delete")) {
							remove_serial_from_list(serial);
							break;
						}

						ImGui::PopID();
					}

					if (ImGui::Button("Delete All Serials")) {
						std::ofstream file("serials.ini", std::ios::trunc);
						if (file.is_open()) file.close();
					}
				} gui.end_group_box();

				break;
			}

			EndChild();
			PopStyleVar(2);

		} ImGui::End();

		PopStyleVar();
		ImGui::GetIO().MouseDrawCursor = var->gui.is_open;
	}

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

void c_menu::shutdown(bool before)
{
	if (before)
	{
		ImGui_ImplDX9_InvalidateDeviceObjects();
		return;
	}

	ImGui_ImplDX9_CreateDeviceObjects();
}

bool c_menu::release()
{
	var->winapi.mh_status = MH_CreateHook(&::SetCursorPos, &h_cursor, reinterpret_cast<LPVOID*>(&o_cursor));
	if (var->winapi.mh_status != MH_OK)
	{
		return false;
	}

	var->winapi.mh_status = MH_EnableHook(&::SetCursorPos);
	if (var->winapi.mh_status != MH_OK)
	{

		return false;
	}

	/* ============ */

	void* dw_present = reinterpret_cast<void*>(utilities::c_device::get_address(17));

	var->winapi.mh_status = MH_CreateHook(dw_present, &h_present, reinterpret_cast<LPVOID*>(&o_present));
	if (var->winapi.mh_status != MH_OK)
	{

		return false;
	}

	var->winapi.mh_status = MH_EnableHook(dw_present);
	if (var->winapi.mh_status != MH_OK)
	{

		return false;
	}

	void* dw_reset = reinterpret_cast<void*>(utilities::c_device::get_address(16));

	var->winapi.mh_status = MH_CreateHook(dw_reset, &h_reset, reinterpret_cast<LPVOID*>(&o_reset));
	if (var->winapi.mh_status != MH_OK)
	{
		return false;
	}

	var->winapi.mh_status = MH_EnableHook(dw_reset);
	if (var->winapi.mh_status != MH_OK)
	{

		return false;
	}

	return true;
}
