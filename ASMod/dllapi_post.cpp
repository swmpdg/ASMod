// vi: set ts=4 sw=4 :
// vim: set tw=75 :

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

#include <extdll.h>

#include <dllapi.h>
#include <meta_api.h>

#include "CASMod.h"

static void GameDLLInit_Post()
{
	//Sven Co-op initializes Angelscript in GameDLLInit, so we have to wait.
	//It's good practice to do this here anyway because mods don't do init stuff before this function is called.

	g_ASMod.Initialize();

	RETURN_META( MRES_IGNORED );
}

static int Spawn_Post( edict_t* pEntity )
{
	//Sven Co-op sets up its custom entity stuff when worldspawn spawns, so let it do its thing first and then let our plugins do their thing.
	if( strcmp( STRING( pEntity->v.classname ), "worldspawn" ) == 0 )
		g_ASMod.WorldCreated();

	RETURN_META_VALUE( MRES_IGNORED, 0 );
}

static DLL_FUNCTIONS gFunctionTable_Post =
{
	&GameDLLInit_Post,		// pfnGameInit
	&Spawn_Post,			// pfnSpawn
	NULL,					// pfnThink
	NULL,					// pfnUse
	NULL,					// pfnTouch
	NULL,					// pfnBlocked
	NULL,					// pfnKeyValue
	NULL,					// pfnSave
	NULL,					// pfnRestore
	NULL,					// pfnSetAbsBox

	NULL,					// pfnSaveWriteFields
	NULL,					// pfnSaveReadFields

	NULL,					// pfnSaveGlobalState
	NULL,					// pfnRestoreGlobalState
	NULL,					// pfnResetGlobalState

	NULL,					// pfnClientConnect
	NULL,					// pfnClientDisconnect
	NULL,					// pfnClientKill
	NULL,					// pfnClientPutInServer
	NULL,					// pfnClientCommand
	NULL,					// pfnClientUserInfoChanged
	NULL,					// pfnServerActivate
	NULL,					// pfnServerDeactivate

	NULL,					// pfnPlayerPreThink
	NULL,					// pfnPlayerPostThink

	NULL,					// pfnStartFrame
	NULL,					// pfnParmsNewLevel
	NULL,					// pfnParmsChangeLevel

	NULL,					// pfnGetGameDescription
	NULL,					// pfnPlayerCustomization

	NULL,					// pfnSpectatorConnect
	NULL,					// pfnSpectatorDisconnect
	NULL,					// pfnSpectatorThink

	NULL,					// pfnSys_Error

	NULL,					// pfnPM_Move
	NULL,					// pfnPM_Init
	NULL,					// pfnPM_FindTextureType

	NULL,					// pfnSetupVisibility
	NULL,					// pfnUpdateClientData
	NULL,					// pfnAddToFullPack
	NULL,					// pfnCreateBaseline
	NULL,					// pfnRegisterEncoders
	NULL,					// pfnGetWeaponData
	NULL,					// pfnCmdStart
	NULL,					// pfnCmdEnd
	NULL,					// pfnConnectionlessPacket
	NULL,					// pfnGetHullBounds
	NULL,					// pfnCreateInstancedBaselines
	NULL,					// pfnInconsistentFile
	NULL,					// pfnAllowLagCompensation
};

C_DLLEXPORT int GetEntityAPI2_Post( DLL_FUNCTIONS *pFunctionTable,
							   int *interfaceVersion )
{
	if( !pFunctionTable ) {
		UTIL_LogPrintf( "GetEntityAPI2 called with null pFunctionTable" );
		return( FALSE );
	}
	else if( *interfaceVersion != INTERFACE_VERSION ) {
		UTIL_LogPrintf( "GetEntityAPI2_post version mismatch; requested=%d ours=%d", *interfaceVersion, INTERFACE_VERSION );
		//! Tell metamod what version we had, so it can figure out who is out of date.
		*interfaceVersion = INTERFACE_VERSION;
		return( FALSE );
	}
	memcpy( pFunctionTable, &gFunctionTable_Post, sizeof( DLL_FUNCTIONS ) );
	return( TRUE );
}