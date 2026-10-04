#include "extdll.h"
#include "util.h"

#include "cbase.h"

/* LINUX COMPILE */
#ifdef _WIN32
// Required DLL entry point
BOOL WINAPI DllMain(
   HINSTANCE hinstDLL,
   DWORD fdwReason,
   LPVOID lpvReserved)
{
        if      (fdwReason == DLL_PROCESS_ATTACH)
    {
    }
        else if (fdwReason == DLL_PROCESS_DETACH)
    {
    }
        return TRUE;
}
#endif
/* END LINUX COMPILE */

enginefuncs_t g_engfuncs;
globalvars_t* gpGlobals;

/* LINUX COMPILE */
#ifdef _WIN32
void DLLEXPORT GiveFnptrsToDll( enginefuncs_t* pengfuncsFromEngine, globalvars_t *pGlobals )
#else
extern "C" void DLLEXPORT GiveFnptrsToDll( enginefuncs_t* pengfuncsFromEngine, globalvars_t *pGlobals )
#endif
{
    memcpy(&g_engfuncs, pengfuncsFromEngine, sizeof(enginefuncs_t));
    gpGlobals = pGlobals;
}
/* END LINUX COMPILE */