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


#ifndef INCLUDED_unotools_CMDOPTIONS_HXX
#define INCLUDED_unotools_CMDOPTIONS_HXX

//_________________________________________________________________________________________________________________
//	includes
//_________________________________________________________________________________________________________________

#include "unotools/unotoolsdllapi.h"
#include <sal/types.h>
#include <osl/mutex.hxx>
#include <com/sun/star/uno/Sequence.h>
#include <com/sun/star/frame/XFrame.hpp>
#include <rtl/ustring.hxx>
#include <unotools/options.hxx>

//_________________________________________________________________________________________________________________
//	types, enums, ...
//_________________________________________________________________________________________________________________

/*-************************************************************************************************************//**
	@descr			The method GetList() returns a list of property values.
					Use follow defines to separate values by names.
*//*-*************************************************************************************************************/
#define CMDOPTIONS_PROPERTYNAME_URL                    ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM( "CommandURL" ))

//_________________________________________________________________________________________________________________
//	forward declarations
//_________________________________________________________________________________________________________________

/*-************************************************************************************************************//**
	@short			forward declaration to our private date container implementation
	@descr			We use these class as internal member to support small memory requirements.
					You can create the container if it is necessary. The class which use these mechanism
					is faster and smaller then a complete implementation!
*//*-*************************************************************************************************************/

class SvtCommandOptions_Impl;

//_________________________________________________________________________________________________________________
//	declarations
//_________________________________________________________________________________________________________________

/*-************************************************************************************************************//**
    @short          collect informations about dynamic menus
    @descr          Make it possible to configure dynamic menu structures of menus like "new" or "wizard".

	@implements		-
	@base			-

	@devstatus		ready to use
*//*-*************************************************************************************************************/

class UNOTOOLS_DLLPUBLIC SvtCommandOptions: public utl::detail::Options
{
	friend class SvtCommandOptions_Impl;

	//-------------------------------------------------------------------------------------------------------------
	//	public methods
	//-------------------------------------------------------------------------------------------------------------

	public:

		enum CmdOption
		{
			CMDOPTION_DISABLED,
			CMDOPTION_NONE
		};

		//---------------------------------------------------------------------------------------------------------
		//	constructor / destructor
		//---------------------------------------------------------------------------------------------------------

		/*-****************************************************************************************************//**
			@short		standard constructor and destructor
			@descr		This will initialize an instance with default values.
						We implement these class with a refcount mechanism! Every instance of this class increase it
						at create and decrease it at delete time - but all instances use the same data container!
						He is implemented as a static member ...

			@seealso	member m_nRefCount
			@seealso	member m_pDataContainer

			@param		-
			@return		-

			@onerror	-
		*//*-*****************************************************************************************************/

         SvtCommandOptions();
        virtual ~SvtCommandOptions();

		//---------------------------------------------------------------------------------------------------------
		//	interface
		//---------------------------------------------------------------------------------------------------------

        /*-****************************************************************************************************//**
			@short		clear complete sepcified list
            @descr      Call this methods to clear the whole list.
                        To fill it again use AppendItem().

			@seealso	-

            @param      "eMenu" select right menu to clear.
			@return		-

			@onerror	-
		*//*-*****************************************************************************************************/

        void Clear( CmdOption eOption );

		/*-****************************************************************************************************//**
            @short      return complete specified list
            @descr      Call it to get all entries of an dynamic menu.
                        We return a list of all nodes with his names and properties.

			@seealso	-

            @param      "eOption" select the list to retrieve.
            @return     A list of command strings is returned.

            @onerror    We return an empty list.
		*//*-*****************************************************************************************************/

        sal_Bool HasEntries( CmdOption eOption ) const;

        /*-****************************************************************************************************//**
			@short		Lookup if a command URL is inside a given list
            @descr      Lookup if a command URL is inside a given lst

			@seealso	-

            @param      "eOption" select right command list
			@param		"aCommandURL" a command URL that is used for the look up
			@return		"sal_True" if the command is inside the list otherwise "sal_False"

			@onerror	-
		*//*-*****************************************************************************************************/

        sal_Bool Lookup( CmdOption eOption, const ::rtl::OUString& aCommandURL ) const;

		/*-****************************************************************************************************//**
            @short      return complete specified list
            @descr      Call it to get all entries of an dynamic menu.
                        We return a list of all nodes with his names and properties.

			@seealso	-

            @param      "eOption" select the list to retrieve.
            @return     A list of command strings is returned.

            @onerror    We return an empty list.
		*//*-*****************************************************************************************************/

        ::com::sun::star::uno::Sequence< ::rtl::OUString > GetList( CmdOption eOption ) const;

		/*-****************************************************************************************************//**
            @short      adds a new command to specified options list
            @descr      You can add a command to specified options list!

			@seealso	method Clear()

			@param		"eOption"			specifies the command list
			@param      "sURL"              URL for dispatch
			@return		-

			@onerror	-
		*//*-*****************************************************************************************************/

        void AddCommand( CmdOption eOption, const ::rtl::OUString& sURL );

        /*-****************************************************************************************************//**
            @short      register an office frame, which must update its dispatches if
                        the underlying configuration was changed.

            @descr      To avoid using of "dead" frame objects or implementing
                        deregistration mechanism too, we use weak references to
                        the given frames.

            @param      "xFrame"            points to the frame, which wish to be
                                            notified, if configuration was changed.
            @return     -

            @onerror    -
        *//*-*****************************************************************************************************/

        void EstablisFrameCallback(const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& xFrame);

	//-------------------------------------------------------------------------------------------------------------
	//	private methods
	//-------------------------------------------------------------------------------------------------------------

	private:

		/*-****************************************************************************************************//**
			@short		return a reference to a static mutex
			@descr		These class is partially threadsafe (for de-/initialization only).
						All access methods are'nt safe!
						We create a static mutex only for one ime and use at different times.

			@seealso	-

			@param		-
			@return		A reference to a static mutex member.

			@onerror	-
		*//*-*****************************************************************************************************/

        UNOTOOLS_DLLPRIVATE static ::osl::Mutex& GetOwnStaticMutex();

	//-------------------------------------------------------------------------------------------------------------
	//	private member
	//-------------------------------------------------------------------------------------------------------------

	private:

		/*Attention

			Don't initialize these static member in these header!
			a) Double dfined symbols will be detected ...
			b) and unresolved externals exist at linking time.
			Do it in your source only.
		 */

        static SvtCommandOptions_Impl*	m_pDataContainer    ;   /// impl. data container as dynamic pointer for smaller memory requirements!
        static sal_Int32				m_nRefCount         ;   /// internal ref count mechanism

};      // class SvtCmdOptions

#endif  // #ifndef INCLUDED_unotools_CMDOPTIONS_HXX
