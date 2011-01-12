// Copyright 2008-2011 Zhang Yun Gui, rhcad@hotmail.com
// http://sourceforge.net/projects/x3c/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*! \file Ix_GuidGenerator.h
 *  \brief ����GUID�������Ľӿ� Ix_GuidGenerator
 *  \author Zhang Yun Gui, C++ Plugin Framework
 *  \date   2010.10.22
 */
#ifndef X3_UTIL_IGUIDGENERATOR_H_
#define X3_UTIL_IGUIDGENERATOR_H_

#include <XComPtr.h>

const XCLSID CLSID_GuidGenerator("8c14f0c5-7795-4ec3-b13a-982f653a700a");

//! ȫ��Ψһ����������Ľӿ�
/*!
    \interface Ix_GuidGenerator
    \ingroup _GROUP_UTILITY_
    \see CLSID_GuidGenerator
*/
interface Ix_GuidGenerator
{
    //! �����µ�GUID��
    virtual std::wstring CreateGuid(bool withBrackets = false) = 0;

    //! �õ�û�����˴����ŵ�GUID��
    virtual std::wstring RemoveGuidBrackets(const std::wstring& uid) = 0;

    //! �õ���ǰ����ʱ����ַ�������ʽΪ��YYYY-MM-DD HH:MM:SS��
    virtual std::wstring GetCurrentTimeString(bool hasYear = true) = 0;

    //! ����ΨһID����ǰ��������ʱ��Ч
    virtual ULONG CreateID(long type = 0) = 0;
};

//! �����µ�GUID��
/*!
    \ingroup _GROUP_UTILITY_
    \see RemoveGuidBrackets, Ix_GuidGenerator
*/
inline std::wstring CreateGuid(bool withBrackets = false)
{
    Cx_Interface<Ix_GuidGenerator> pIFGenerator(CLSID_GuidGenerator);
    ASSERT(pIFGenerator.IsNotNull());
    return pIFGenerator->CreateGuid(withBrackets);
}

//! �õ�û�����˴����ŵ�GUID��
/*!
    \ingroup _GROUP_UTILITY_
    \see CreateGuid, Ix_GuidGenerator
*/
inline std::wstring RemoveGuidBrackets(const std::wstring& uid)
{
    Cx_Interface<Ix_GuidGenerator> pIFGenerator(CLSID_GuidGenerator);
    ASSERT(pIFGenerator.IsNotNull());
    return pIFGenerator->RemoveGuidBrackets(uid);
}

//! ����ΨһID����ǰ��������ʱ��Ч
/*!
    \ingroup _GROUP_UTILITY_
    \see Ix_GuidGenerator
*/
inline ULONG GuidCreateID(long type = 0)
{
    Cx_Interface<Ix_GuidGenerator> pIFGenerator(CLSID_GuidGenerator);
    ASSERT(pIFGenerator.IsNotNull());
    return pIFGenerator->CreateID(type);
}

//! �õ���ǰ����ʱ����ַ�������ʽΪ��YYYY-MM-DD HH:MM:SS��
/*!
    \ingroup _GROUP_UTILITY_
*/
inline std::wstring GetCurrentTimeString(bool hasYear = true)
{
    Cx_Interface<Ix_GuidGenerator> pIFGenerator(CLSID_GuidGenerator);
    ASSERT(pIFGenerator.IsNotNull());
    return pIFGenerator->GetCurrentTimeString(hasYear);
}

#endif // X3_UTIL_IGUIDGENERATOR_H_