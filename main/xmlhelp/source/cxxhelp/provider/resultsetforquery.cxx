/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_xmlhelp.hxx"
#include <com/sun/star/ucb/Command.hpp>
#include <com/sun/star/ucb/XCommandEnvironment.hpp>
#include <com/sun/star/i18n/XExtendedTransliteration.hpp>
#include <com/sun/star/ucb/XCommandProcessor.hpp>
#include <com/sun/star/lang/Locale.hpp>
#include <com/sun/star/script/XInvocation.hpp>

#ifndef INCLUDED_STL_ALGORITHM
#include <algorithm>
#define INCLUDED_STL_ALGORITHM
#endif
#ifndef INCLUDED_STL_SET
#include <set>
#define INCLUDED_STL_SET
#endif

#include <qe/Query.hxx>
#include <qe/DocGenerator.hxx>
#include "resultsetforquery.hxx"
#include "databases.hxx"

// For testing
// #define LOGGING

using namespace std;
using namespace chelp;
using namespace xmlsearch::excep;
using namespace xmlsearch::qe;
using namespace com::sun::star;
using namespace com::sun::star::ucb;
using namespace com::sun::star::i18n;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;

struct HitItem
{
	rtl::OUString	m_aURL;
	float			m_fScore;

	HitItem( void )	{}
	HitItem( const rtl::OUString& aURL, float fScore )
		: m_aURL( aURL )
		, m_fScore( fScore )
	{}
	bool operator < ( const HitItem& rHitItem ) const
	{
		return rHitItem.m_fScore < m_fScore;
	}
};

ResultSetForQuery::ResultSetForQuery( const uno::Reference< lang::XMultiServiceFactory >&  xMSF,
									  const uno::Reference< XContentProvider >&  xProvider,
									  sal_Int32 nOpenMode,
									  const uno::Sequence< beans::Property >& seq,
									  const uno::Sequence< NumberedSortingInfo >& seqSort,
									  URLParameter& aURLParameter,
									  Databases* pDatabases )
	: ResultSetBase( xMSF,xProvider,nOpenMode,seq,seqSort ),
	  m_pDatabases( pDatabases ),
      m_aURLParameter( aURLParameter )
{
    Reference< XTransliteration > xTrans( 
        xMSF->createInstance( rtl::OUString::createFromAscii( "com.sun.star.i18n.Transliteration" ) ),
        UNO_QUERY );
    Locale aLocale( aURLParameter.get_language(),
                    rtl::OUString(),
                    rtl::OUString() );
    if(xTrans.is())
        xTrans->loadModule(TransliterationModules_UPPERCASE_LOWERCASE,
                           aLocale );

	// Access Lucene via XInvocation
	Reference< script::XInvocation > xInvocation( 
        xMSF->createInstance( rtl::OUString::createFromAscii( "com.sun.star.help.HelpSearch" ) ),
        UNO_QUERY );
    
	vector< vector< rtl::OUString > > queryList;
	{
		sal_Int32 idx;
		rtl::OUString query = m_aURLParameter.get_query();
		while( query.getLength() )
		{
			idx = query.indexOf( sal_Unicode( ' ' ) );
			if( idx == -1 )
				idx = query.getLength();
			
			vector< rtl::OUString > currentQuery;
            rtl::OUString tmp(query.copy( 0,idx ));
            rtl:: OUString toliterate = tmp;            
            if(xTrans.is()) {
                Sequence<sal_Int32> aSeq;
                toliterate = xTrans->transliterate(
                    tmp,0,tmp.getLength(),aSeq);
            }

			currentQuery.push_back( toliterate );
			queryList.push_back( currentQuery );

			int nCpy = 1 + idx;
			if( nCpy >= query.getLength() )
				query = rtl::OUString();
			else
				query = query.copy( 1 + idx );
		}
	}

	vector< rtl::OUString > aCompleteResultVector;
	if( xInvocation.is() )
	{
		rtl::OUString scope = m_aURLParameter.get_scope();
		bool bCaptionsOnly = ( scope.compareToAscii( "Heading" ) == 0 );
		sal_Int32 hitCount = m_aURLParameter.get_hitCount();

#ifdef LOGGING
		FILE* pFile = fopen( "d:\\resultset_out.txt", "w" );
#endif

		IndexFolderIterator aIndexFolderIt( *pDatabases, m_aURLParameter.get_module(), m_aURLParameter.get_language() );
		rtl::OUString idxDir;
		bool bExtension = false;
		int iDir = 0;
		vector< vector<HitItem>* > aIndexFolderResultVectorVector;

		bool bTemporary;
		while( (idxDir = aIndexFolderIt.nextIndexFolder( bExtension, bTemporary )).getLength() > 0 )
		{	
			vector<HitItem> aIndexFolderResultVector;

			try
			{
				vector< vector<HitItem>* > aQueryListResultVectorVector;
				set< rtl::OUString > aSet,aCurrent,aResultSet;

				int nQueryListSize = queryList.size();
				if( nQueryListSize > 1 )
					hitCount = 2000;

				for( int i = 0; i < nQueryListSize; ++i )
				{
					vector<HitItem>* pQueryResultVector;
					if( nQueryListSize > 1 )
					{
						pQueryResultVector = new vector<HitItem>();
						aQueryListResultVectorVector.push_back( pQueryResultVector );
					}
					else
					{
						pQueryResultVector = &aIndexFolderResultVector;
					}
					pQueryResultVector->reserve( hitCount );

					int nParamCount = bCaptionsOnly ? 7 : 6;
					Sequence<uno::Any> aParamsSeq( nParamCount );

					aParamsSeq[0] = uno::makeAny( rtl::OUString::createFromAscii( "-lang" ) );
					aParamsSeq[1] = uno::makeAny( m_aURLParameter.get_language() );
                    
					aParamsSeq[2] = uno::makeAny( rtl::OUString::createFromAscii( "-index" ) );
					rtl::OUString aSystemPath;
					osl::FileBase::getSystemPathFromFileURL( idxDir, aSystemPath );
					aParamsSeq[3] = uno::makeAny( aSystemPath );

					aParamsSeq[4] = uno::makeAny( rtl::OUString::createFromAscii( "-query" ) );

					const std::vector< rtl::OUString >& aListItem = queryList[i];
					::rtl::OUString aNewQueryStr = aListItem[0];
					aParamsSeq[5] = uno::makeAny( aNewQueryStr );

					if( bCaptionsOnly )
						aParamsSeq[6] = uno::makeAny( rtl::OUString::createFromAscii( "-caption" ) );

					Sequence< sal_Int16 > aOutParamIndex;
					Sequence< uno::Any > aOutParam;

					uno::Any aRet = xInvocation->invoke( rtl::OUString::createFromAscii( "search" ),
						aParamsSeq, aOutParamIndex, aOutParam );

					Sequence< float > aScoreSeq;
					int nScoreCount = 0;
					int nOutParamCount = aOutParam.getLength();
					if( nOutParamCount == 1 )
					{
						const uno::Any* pScoreAnySeq = aOutParam.getConstArray();
						if( pScoreAnySeq[0] >>= aScoreSeq )
							nScoreCount = aScoreSeq.getLength();
					}

					Sequence<rtl::OUString> aRetSeq;
					if( aRet >>= aRetSeq )
					{
						if( nQueryListSize > 1 )
							aSet.clear();

						const rtl::OUString* pRetSeq = aRetSeq.getConstArray();
						int nCount = aRetSeq.getLength();
						if( nCount > hitCount )
							nCount = hitCount;
						for( int j = 0 ; j < nCount ; ++j )
						{
							float fScore = 0.0;
							if( j < nScoreCount )
								fScore = aScoreSeq[j];

							rtl::OUString aURL = pRetSeq[j];
							pQueryResultVector->push_back( HitItem( aURL, fScore ) );
							if( nQueryListSize > 1 )
								aSet.insert( aURL );

#ifdef LOGGING
							if( pFile )
							{
								rtl::OString tmp(rtl::OUStringToOString( aURL, RTL_TEXTENCODING_UTF8));
								fprintf( pFile, "Dir %d, Query %d, Item: score=%f, URL=%s\n", iDir, i, fScore, tmp.getStr() );
							}
#endif
						}
					}

					// intersect
					if( nQueryListSize > 1 )
					{
						if( i == 0 )
						{
							aResultSet = aSet;
						}
						else
						{
							aCurrent = aResultSet;
							aResultSet.clear();
							set_intersection( aSet.begin(),aSet.end(),
											  aCurrent.begin(),aCurrent.end(),
											  inserter(aResultSet,aResultSet.begin()));
						}
					}
				}

				// Combine results in aIndexFolderResultVector
				if( nQueryListSize > 1 )
				{
					for( int n = 0 ; n < nQueryListSize ; ++n )
					{
						vector<HitItem>* pQueryResultVector = aQueryListResultVectorVector[n];
						vector<HitItem>& rQueryResultVector = *pQueryResultVector;

						int nItemCount = rQueryResultVector.size();
						for( int i = 0 ; i < nItemCount ; ++i )
						{
							const HitItem& rItem = rQueryResultVector[ i ];
							set< rtl::OUString >::iterator it;
							if( (it = aResultSet.find( rItem.m_aURL )) != aResultSet.end() )
							{
								HitItem aItemCopy( rItem );
								aItemCopy.m_fScore /= nQueryListSize;	// To get average score
								if( n == 0 )
								{
									// Use first pass to create entry
									aIndexFolderResultVector.push_back( aItemCopy );

#ifdef LOGGING
									if( pFile )
									{
										rtl::OString tmp(rtl::OUStringToOString( aItemCopy.m_aURL, RTL_TEXTENCODING_UTF8));
										fprintf( pFile, "Combine: Query %d (first pass), Item %d: score=%f (%f), URL=%s\n", n, i, aItemCopy.m_fScore, rItem.m_fScore, tmp.getStr() );
									}
#endif
								}
								else
								{
									// Find entry in vector
									int nCount = aIndexFolderResultVector.size();
									for( int j = 0 ; j < nCount ; ++j )
									{
										HitItem& rFindItem = aIndexFolderResultVector[ j ];
										if( rFindItem.m_aURL.equals( aItemCopy.m_aURL ) )
										{
#ifdef LOGGING
											if( pFile )
											{
												rtl::OString tmp(rtl::OUStringToOString( aItemCopy.m_aURL, RTL_TEXTENCODING_UTF8));
												fprintf( pFile, "Combine: Query %d, Item %d: score=%f + %f = %f, URL=%s\n", n, i, 
													rFindItem.m_fScore, aItemCopy.m_fScore, rFindItem.m_fScore + aItemCopy.m_fScore, tmp.getStr() );
											}
#endif

											rFindItem.m_fScore += aItemCopy.m_fScore;
											break;
										}
									}
								}
							}
						}

						delete pQueryResultVector;
					}

					sort( aIndexFolderResultVector.begin(), aIndexFolderResultVector.end() );
				}

				vector<HitItem>* pIndexFolderHitItemVector = new vector<HitItem>( aIndexFolderResultVector );
				aIndexFolderResultVectorVector.push_back( pIndexFolderHitItemVector );
				aIndexFolderResultVector.clear();
			}
			catch( const Exception& )
			{
			}

			++iDir;

			if( bTemporary )
				aIndexFolderIt.deleteTempIndexFolder( idxDir );

		}	// Iterator


		int nVectorCount = aIndexFolderResultVectorVector.size();
		vector<HitItem>::size_type* pCurrentVectorIndex = new vector<HitItem>::size_type[nVectorCount];
		for( int j = 0 ; j < nVectorCount ; ++j )
			pCurrentVectorIndex[j] = 0;

#ifdef LOGGING
		if( pFile )
		{
			for( int k = 0 ; k < nVectorCount ; ++k )
			{
				vector<HitItem>& rIndexFolderVector = *aIndexFolderResultVectorVector[k];
				int nItemCount = rIndexFolderVector.size();

				fprintf( pFile, "Vector %d, %d elements\n", k, nItemCount );

				for( int i = 0 ; i < nItemCount ; ++i )
				{
					const HitItem& rItem = rIndexFolderVector[ i ];
					rtl::OString tmp(rtl::OUStringToOString(rItem.m_aURL, RTL_TEXTENCODING_UTF8));
					fprintf( pFile, "    Item_vector%d, %d/%d: score=%f, URL=%s\n", k, i, nItemCount, rItem.m_fScore, tmp.getStr() );
				}
			}
		}
#endif

		sal_Int32 nTotalHitCount = m_aURLParameter.get_hitCount();
		sal_Int32 nHitCount = 0;
		while( nHitCount < nTotalHitCount )
		{
			int iVectorWithBestScore = -1;
			float fBestScore = 0.0;
			for( int k = 0 ; k < nVectorCount ; ++k )
			{
				vector<HitItem>& rIndexFolderVector = *aIndexFolderResultVectorVector[k];
				if( pCurrentVectorIndex[k] < rIndexFolderVector.size() )
				{
					const HitItem& rItem = rIndexFolderVector[ pCurrentVectorIndex[k] ];

					if( fBestScore < rItem.m_fScore )
					{
						fBestScore = rItem.m_fScore;
						iVectorWithBestScore = k;
					}
				}
			}

			if( iVectorWithBestScore == -1 )	// No item left at all
				break;

			vector<HitItem>& rIndexFolderVector = *aIndexFolderResultVectorVector[iVectorWithBestScore];
			const HitItem& rItem = rIndexFolderVector[ pCurrentVectorIndex[iVectorWithBestScore] ];

			pCurrentVectorIndex[iVectorWithBestScore]++;

			aCompleteResultVector.push_back( rItem.m_aURL );
			++nHitCount;
		}

		delete[] pCurrentVectorIndex;
		for( int n = 0 ; n < nVectorCount ; ++n )
		{
			vector<HitItem>* pIndexFolderVector = aIndexFolderResultVectorVector[n];
			delete pIndexFolderVector;
		}

#ifdef LOGGING
		fclose( pFile );
#endif
	}

	sal_Int32 replIdx = rtl::OUString::createFromAscii( "#HLP#" ).getLength();
	rtl::OUString replWith = rtl::OUString::createFromAscii( "vnd.sun.star.help://" );
	
	int nResultCount = aCompleteResultVector.size();
	for( int r = 0 ; r < nResultCount ; ++r )
	{
		rtl::OUString aURL = aCompleteResultVector[r];
		rtl::OUString aResultStr = replWith + aURL.copy(replIdx);
  		m_aPath.push_back( aResultStr );
	}

	m_aItems.resize( m_aPath.size() );
	m_aIdents.resize( m_aPath.size() );
	
	Command aCommand;
	aCommand.Name = rtl::OUString::createFromAscii( "getPropertyValues" );
	aCommand.Argument <<= m_sProperty;
	
	for( m_nRow = 0; sal::static_int_cast<sal_uInt32>( m_nRow ) < m_aPath.size(); ++m_nRow )
	{
		m_aPath[m_nRow] = 
			m_aPath[m_nRow]                                          +
			rtl::OUString::createFromAscii( "?Language=" )           +
			m_aURLParameter.get_language()                           +
			rtl::OUString::createFromAscii( "&System=" )             +
			m_aURLParameter.get_system();

		uno::Reference< XContent > content = queryContent();
		if( content.is() )
		{
			uno::Reference< XCommandProcessor > cmd( content,uno::UNO_QUERY );
			cmd->execute( aCommand,0,uno::Reference< XCommandEnvironment >( 0 ) ) >>= m_aItems[m_nRow]; //TODO: check return value of operator >>=
		}
	}
	m_nRow = 0xffffffff;
}
