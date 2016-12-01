#include <cstdarg>
#include <memory>
#include <string>

#include <angelscript.h>

#include <Angelscript/util/ASLogging.h>

#include <Angelscript/ScriptAPI/ASCDateTime.h>
#include <Angelscript/ScriptAPI/ASCTime.h>

#include <Angelscript/ScriptAPI/SQL/CASSQLThreadPool.h>

#include <Angelscript/ScriptAPI/SQL/ASSQL.h>

#include <Angelscript/ScriptAPI/SQL/SQLite/ASSQLite.h>
#include <Angelscript/ScriptAPI/SQL/SQLite/CASSQLiteConnection.h>

#include <Angelscript/ScriptAPI/SQL/MySQL/ASMySQL.h>
#include <Angelscript/ScriptAPI/SQL/MySQL/CASMySQLConnection.h>

#include <extdll.h>
#include <meta_api.h>

#include "Module.h"

#include "keyvalues/Keyvalues.h"

#include "StringUtils.h"

#include "ASHLSQL.h"

#define SQLITE_BASE_DIR "scripts/databases"
#define SQLITE_EXT ".sqlite"

#define MYSQL_DEFAULT_CONN_BLOCK "default_mysql_connection"

static void SQLLogFunc( const char* const pszFormat, ... )
{
	va_list list;

	va_start( list, pszFormat );
	as::VCritical( pszFormat, list );
	va_end( list );
}

static size_t CalculateThreadCount()
{
	/*
	const unsigned int uiThreads = std::thread::hardware_concurrency();

	if( uiThreads >= 1 )
		return 1;
		*/

	//Only use 1 thread for now. - Solokiller
	return 1;
}

CASSQLThreadPool* g_pSQLThreadPool = nullptr;

static CASSQLiteConnection* HLCreateSQLiteConnection( const std::string& szDatabase )
{
	if( szDatabase.empty() )
	{
		as::Critical( "SQL::CreateSQLiteConnection: Empty database name!\n" );
		return nullptr;
	}

	//TODO
	//g_pFileSystem->CreateDirHierarchy( SQLITE_BASE_DIR, nullptr );

	char szFilename[ MAX_PATH ];

	const char* pszGameDir = gpMetaUtilFuncs->pfnGetGameInfo( PLID, GINFO_GAMEDIR );

	if( !pszGameDir )
	{
		as::Critical( "SQL::CreateSQLiteConnection: Failed to get game directory!\n" );
		return nullptr;
	}

	const int iResult = snprintf( szFilename, sizeof( szFilename ), "%s/%s/%s%s", pszGameDir, SQLITE_BASE_DIR, szDatabase.c_str(), SQLITE_EXT );
	
	if( iResult < 0 || static_cast<size_t>( iResult ) >= sizeof( szFilename ) )
	{
		as::Critical( "SQL::CreateSQLiteConnection: Failed to format database \"%s\" filename!\n", szDatabase.c_str() );
		return nullptr;
	}

	//TODO: filter access to databases here so only authorized access works - Solokiller

	return new CASSQLiteConnection( *g_pSQLThreadPool, szFilename );
}

static unsigned int ParseMySQLPort( std::string& szHostName )
{
	//Based on AMX's SQLX interface; allow scripts to specify a port using host:port format. - Solokiller
	size_t uiPortSep = szHostName.find( ':' );

	unsigned int uiPort = 3306;

	if( uiPortSep != std::string::npos )
	{
		uiPort = strtoul( &szHostName[ uiPortSep + 1 ], nullptr, 10 );

		//Trim the port part from the string.
		szHostName.resize( uiPortSep );
	}

	return uiPort;
}

static CASMySQLConnection* HLCreateMySQLConnection( const std::string& szHost, const std::string& szUser, const std::string& szPassword, const std::string& szDatabase = "" )
{
	std::string szHostName = szHost;

	const unsigned int uiPort = ParseMySQLPort( szHostName );

	return new CASMySQLConnection( *g_pSQLThreadPool, szHostName.c_str(), szUser.c_str(), szPassword.c_str(), szDatabase.c_str(), uiPort, "", 0 );
}

/**
*	Creates a MySQL connection using default connection settings defined in the server's MySQL config.
*/
static CASMySQLConnection* HLCreateMySQLConnectionWithDefaults( const std::string& szDatabase = "" )
{
	const char* const pszConfigFile = as_mysql_config->string;

	if( !( *pszConfigFile ) )
	{
		as::Critical( "SQL::CreateMySQLConnectionWithDefaults: No config file specified; cannot create connection\n" );
		return nullptr;
	}

	//Parse the config file.
	//Only load from the mod directory to prevent malicious servers from downloading files and overriding them.
	char szPath[ PATH_MAX ];

	const auto result = snprintf( szPath, sizeof( szPath ), "%s/%s", gpMetaUtilFuncs->pfnGetGameInfo( PLID, GINFO_GAMEDIR ), pszConfigFile );

	if( !PrintfSuccess( result, sizeof( szPath ) ) )
	{
		as::Critical( "SQL::CreateMySQLConnectionWithDefaults: Error while formatting config filename\n" );
		return nullptr;
	}

	kv::Parser parser( szPath );

	if( !parser.HasInputData() )
	{
		as::Critical( "SQL::CreateMySQLConnectionWithDefaults: Couldn't open config file \"%s\"!\n", pszConfigFile );
		return nullptr;
	}

	parser.SetEscapeSeqConversion( GetEscapeSeqConversion() );

	const auto parseResult = parser.Parse();

	if( parseResult != kv::Parser::ParseResult::SUCCESS )
	{
		as::Critical( "SQL::CreateMySQLConnectionWithDefaults: Error while parsing config file \"%s\": %s\n", kv::Parser::ParseResultToString( parseResult ) );
		return nullptr;
	}

	auto pConnection = parser.GetKeyvalues();

	if( !pConnection )
	{
		as::Critical( "SQL::CreateMySQLConnectionWithDefaults: No connection data found\n" );
		return nullptr;
	}

	auto pHost = pConnection->FindFirstChild<kv::KV>( "host" );
	auto pUser = pConnection->FindFirstChild<kv::KV>( "user" );
	auto pPass = pConnection->FindFirstChild<kv::KV>( "pass" );

	bool bHasAllValues = true;

	if( !pHost )
	{
		as::Critical( "SQL::CreateMySQLConnectionWithDefaults: Missing value for key \"host\"\n" );
		bHasAllValues = false;
	}

	if( !pUser )
	{
		as::Critical( "SQL::CreateMySQLConnectionWithDefaults: Missing value for key \"user\"\n" );
		bHasAllValues = false;
	}

	if( !pPass )
	{
		as::Critical( "SQL::CreateMySQLConnectionWithDefaults: Missing value for key \"pass\"\n" );
		bHasAllValues = false;
	}

	if( !bHasAllValues )
		return nullptr;

	std::string szHostName = pHost->GetValue();

	const unsigned int uiPort = ParseMySQLPort( szHostName );

	return new CASMySQLConnection( *g_pSQLThreadPool, szHostName.c_str(), pUser->GetValue().c_str(), pPass->GetValue().c_str(), szDatabase.c_str(), uiPort, "", 0 );
}

void RegisterScriptHLSQL( asIScriptEngine& engine )
{
	g_pSQLThreadPool = new CASSQLThreadPool( CalculateThreadCount(), &::SQLLogFunc );

	//Call an SQLite function to load the library. - Solokiller
	as::Msg( "SQLite library version: %s\n", sqlite3_libversion() );

	//Call a MySQL function to load the library. - Solokiller
	as::Msg( "MariaDB library version: %s\n", mysql_get_client_info() );

	RegisterScriptCTime( engine );
	RegisterScriptCDateTime( engine );

	RegisterScriptSQLCommon( engine );
	RegisterScriptSQLite( engine );
	RegisterScriptMySQL( engine );

	const std::string szOldNS = engine.GetDefaultNamespace();

	engine.SetDefaultNamespace( "SQL" );

	engine.RegisterGlobalFunction(
		"SQLiteConnection@ CreateSQLiteConnection(const " AS_STRING_OBJNAME "& in szDatabase)",
		asFUNCTION( HLCreateSQLiteConnection ), asCALL_CDECL );

	engine.RegisterGlobalFunction(
		"MySQLConnection@ CreateMySQLConnection("
		"const " AS_STRING_OBJNAME "& in szHost, const " AS_STRING_OBJNAME "& in szUser,"
		"const " AS_STRING_OBJNAME "& in szPassword, const " AS_STRING_OBJNAME "& in szDatabase = \"\")",
		asFUNCTION( HLCreateMySQLConnection ), asCALL_CDECL );

	engine.RegisterGlobalFunction(
		"MySQLConnection@ CreateMySQLConnectionWithDefaults(const " AS_STRING_OBJNAME "& in szDatabase = \"\")",
		asFUNCTION( HLCreateMySQLConnectionWithDefaults ), asCALL_CDECL );

	engine.SetDefaultNamespace( szOldNS.c_str() );
}