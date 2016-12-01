// vi: set ts=4 sw=4 :
// vim: set tw=75 :

// meta_api.cpp - minimal implementation of metamod's plugin interface

// This is intended to illustrate the (more or less) bare minimum code
// required for a valid metamod plugin, and is targeted at those who want
// to port existing HL/SDK DLL code to run as a metamod plugin.

/*
 * Copyright (c) 2001-2006 Will Day <willday@hpgx.net>
 *
 *    This file is part of Metamod.
 *
 *    Metamod is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    Metamod is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Metamod; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */

#include <extdll.h>			// always

#include <meta_api.h>		// of course

#include "sdk_util.h"		// UTIL_LogPrintf, etc
#include "plugin/SteamworksAPI.h"
#include "CMetaSteamworksListener.h"

#include "info_name.h"

#include "CASMod.h"

// Must provide at least one of these..
static META_FUNCTIONS gMetaFunctionTable = {
	NULL,					// pfnGetEntityAPI				HL SDK; called before game DLL
	NULL,					// pfnGetEntityAPI_Post			META; called after game DLL
	GetEntityAPI2,			// pfnGetEntityAPI2				HL SDK2; called before game DLL
	GetEntityAPI2_Post,		// pfnGetEntityAPI2_Post		META; called after game DLL
	NULL,					// pfnGetNewDLLFunctions		HL SDK2; called before game DLL
	GetNewDLLFunctions_Post,// pfnGetNewDLLFunctions_Post	META; called after game DLL
	GetEngineFunctions,		// pfnGetEngineFunctions		META; called before HL engine
	NULL,					// pfnGetEngineFunctions_Post	META; called after HL engine
};

// Description of plugin
plugin_info_t Plugin_info = {
	META_INTERFACE_VERSION,	// ifvers
	VNAME,			// name
	"1.0 Alpha",	// version
	VDATE,			// date
	VAUTHOR,		// author
	VURL,			// url
	VLOGTAG,		// logtag
	PT_STARTUP,	// (when) loadable
	PT_STARTUP,	// (when) unloadable
};

// Global vars from metamod:
meta_globals_t *gpMetaGlobals;		// metamod globals
gamedll_funcs_t *gpGamedllFuncs;	// gameDLL function tables
mutil_funcs_t *gpMetaUtilFuncs;		// metamod utility functions

/**
*	Factory functions.
*/
MetaFactories_t* g_pFactories = NULL;

/*
*	Replace this with your own listener class or modify it to include your required features.
*/
static CMetaSteamworksListener g_MetaSteamworksListener;

IMetaSteamworksListener* g_pMetaSteamworksListener = &g_MetaSteamworksListener;

// Metamod requesting info about this plugin:
//  ifvers			(given) interface_version metamod is using
//  pPlugInfo		(requested) struct with info about plugin
//  pMetaUtilFuncs	(given) table of utility functions provided by metamod
C_DLLEXPORT int Meta_Query(const char * /*ifvers */, plugin_info_t **pPlugInfo,
		mutil_funcs_t *pMetaUtilFuncs) 
{
	// Give metamod our plugin_info struct
	*pPlugInfo=&Plugin_info;
	// Get metamod utility function table.
	gpMetaUtilFuncs=pMetaUtilFuncs;
	return(TRUE);
}

// Metamod attaching plugin to the server.
//  now				(given) current phase, ie during map, during changelevel, or at startup
//  pFunctionTable	(requested) table of function tables this plugin catches
//  pMGlobals		(given) global vars from metamod
//  pGamedllFuncs	(given) copy of function tables from game dll
C_DLLEXPORT int Meta_Attach(PLUG_LOADTIME /* now */, 
		META_FUNCTIONS *pFunctionTable, meta_globals_t *pMGlobals, 
		gamedll_funcs_t *pGamedllFuncs) 
{
	if(!pMGlobals) {
		LOG_ERROR(PLID, "Meta_Attach called with null pMGlobals");
		return(FALSE);
	}
	gpMetaGlobals=pMGlobals;
	if(!pFunctionTable) {
		LOG_ERROR(PLID, "Meta_Attach called with null pFunctionTable");
		return(FALSE);
	}
	memcpy(pFunctionTable, &gMetaFunctionTable, sizeof(META_FUNCTIONS));
	gpGamedllFuncs=pGamedllFuncs;

	//return g_ASMod.Initialize();
	return true;
}

// Metamod detaching plugin from the server.
// now		(given) current phase, ie during map, etc
// reason	(given) why detaching (refresh, console unload, forced unload, etc)
C_DLLEXPORT int Meta_Detach(PLUG_LOADTIME /* now */, 
		PL_UNLOAD_REASON /* reason */) 
{
	g_ASMod.Shutdown();
	Steamworks_ShutdownLibrary();

	return(TRUE);
}

C_DLLEXPORT int Meta_Factories( MetaFactories_t* pFactories )
{
	g_pFactories = pFactories;

	if( !pFactories )
	{
		LOG_ERROR( PLID, "Meta_Factories called with null pFactories" );
		return( FALSE );
	}

	if( !Steamworks_InitLibrary() )
	{
		LOG_ERROR( PLID, "Failed to initialize Steamworks interface" );
		return FALSE;
	}
	else
	{
		LOG_MESSAGE( PLID, "Steamworks interface initialized" );
		g_pMetaSteamworks->AddListener( PLID, g_pMetaSteamworksListener );
	}

	return TRUE;
}