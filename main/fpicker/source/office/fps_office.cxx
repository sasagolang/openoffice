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
#include "precompiled_fpicker.hxx"
#include "sal/types.h"

#ifndef _CPPUHELPER_IMPLEMENTATIONENTRY_HXX_
#include "cppuhelper/implementationentry.hxx"
#endif

#include "OfficeFilePicker.hxx"
#include "OfficeFolderPicker.hxx"

static cppu::ImplementationEntry g_entries[] =
{
	{
		SvtFilePicker::impl_createInstance,
		SvtFilePicker::impl_getStaticImplementationName,
		SvtFilePicker::impl_getStaticSupportedServiceNames,
		cppu::createSingleComponentFactory, 0, 0
	},
	{
		SvtFolderPicker::impl_createInstance,
		SvtFolderPicker::impl_getStaticImplementationName,
		SvtFolderPicker::impl_getStaticSupportedServiceNames,
		cppu::createSingleComponentFactory, 0, 0
	},
	{ 0, 0, 0, 0, 0, 0 }
};

extern "C"
{
SAL_DLLPUBLIC_EXPORT void SAL_CALL
component_getImplementationEnvironment(
	const sal_Char ** ppEnvTypeName, uno_Environment ** /* ppEnv */)
{
	*ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}

SAL_DLLPUBLIC_EXPORT void * SAL_CALL component_getFactory(
	const sal_Char * pImplementationName, void * pServiceManager, void * pRegistryKey)
{
	return cppu::component_getFactoryHelper (
		pImplementationName, pServiceManager, pRegistryKey, g_entries);
}

} // extern "C"
