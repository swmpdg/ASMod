#ifndef KEYVALUES_KEYVALUES_H
#define KEYVALUES_KEYVALUES_H

#include <cassert>

#include "KVForward.h"

#include "KeyvaluesConstants.h"

#include "CKeyvalueNode.h"
#include "CKeyvalue.h"
#include "CKeyvalueBlock.h"

#include "CKeyvaluesLexer.h"
#include "CKeyvaluesParser.h"
#include "CKeyvaluesWriter.h"

namespace keyvalues
{
/**
*	Prints a keyvalue tree.
*/
struct CTreePrinter
{
public:
	/**
	*	@param logFn Function that will be called to output the tree.
	*/
	CTreePrinter( CLogger&& logger )
		: m_Logger( logger )
	{
		assert( m_Logger );
	}

	/**
	*	Print the tree.
	*/
	void operator()( const CKeyvalueNode& node )
	{
		if( !m_Logger )
			return;

		RecursivePrint( node );
	}

private:
	void RecursivePrint( const CKeyvalueNode& node )
	{
		PrintTabs();

		PrintToken( node.GetKey() );

		switch( node.GetType() )
		{
		case NodeType::KEYVALUE:
			{
				m_Logger( " " );
				PrintToken( static_cast<const CKeyvalue&>( node ).GetValue() );
				m_Logger( "\n" );
				break;
			}

		case NodeType::BLOCK:
			{
				const auto& block = static_cast<const CKeyvalueBlock&>( node );

				m_Logger( "\n" );
				PrintTabs();
				m_Logger( "{\n" );

				m_szTabs += "\t";
				
				for( auto pChild : block.GetChildren() )
				{
					RecursivePrint( *pChild );
				}

				m_szTabs.resize( m_szTabs.size() - 1 );

				PrintTabs();
				m_Logger( "}\n" );

				break;
			}

			//No default statement to catch warnings if new types are ever added.
		}
	}

	void PrintTabs()
	{
		m_Logger( "%s", m_szTabs.c_str() );
	}

	void PrintToken( const std::string& szString )
	{
		m_Logger( "\"%s\"", szString.c_str() );
	}

private:
	CLogger m_Logger;
	//To keep dynamic allocation down, store the tab string.
	std::string m_szTabs;
};
}

#endif //KEYVALUES_KEYVALUES_H
