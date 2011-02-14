// Copyright 2008-2011 Zhang Yun Gui, rhcad@hotmail.com
// http://sourceforge.net/projects/x3c/

#include "stdafx.h"
#include "Cx_FileUtility.h"
#include <io.h>
#include <shellapi.h>
#include <SysErrStr.h>
#include <RelToAbs.h>

#pragma comment(lib,"shell32.lib")

static long s_nFileOpRet = 0;   // SHFileOperationW ����ֵ

static inline bool IsPathSlash(wchar_t c)
{
    return '\\' == c || '/' == c;
}

static inline bool IsNotNull(const wchar_t* pszText)
{
    return pszText != NULL && *pszText != 0;
}

static void ReplaceSlash(wchar_t* path)
{
    for (; *path; path++)
    {
        if ('/' == *path)
        {
            *path = '\\';
        }
    }
}

bool Cx_FileUtility::IsPathFileExists(const wchar_t* pszFileName, bool bWrite)
{
    return IsNotNull(pszFileName)
        && _waccess(pszFileName, bWrite ? 6 : 0) == 0;
}

bool Cx_FileUtility::IsPath(const wchar_t* pszFileName, bool bCheckExists)
{
    if (IsNotNull(pszFileName))
    {
        if (bCheckExists)
        {
            DWORD dwAttr = ::GetFileAttributesW(pszFileName);
            if (dwAttr != (DWORD)-1)
            {
                return !!(dwAttr & FILE_ATTRIBUTE_DIRECTORY);
            }
        }

        return IsPathSlash(pszFileName[lstrlenW(pszFileName) - 1]);
    }

    return false;
}

Cx_FileUtility::Cx_FileUtility()
    : m_hMsgBoxOwnerWnd(NULL)
{
}

bool Cx_FileUtility::CreateDirectory(const wchar_t* pszFileName, bool bIsPath)
{
    wchar_t szPath[MAX_PATH];
    int i, nLen;
    wchar_t cSaveChar;

    if (!IsNotNull(pszFileName))
        return false;

    lstrcpynW(szPath, pszFileName, MAX_PATH);
    ReplaceSlash(szPath);
    if (bIsPath)
        PathRemoveBackslashW(szPath);
    else
        PathRemoveFileSpecW(szPath);
    PathAddBackslashW(szPath);
    nLen = lstrlenW(szPath);

    if (_waccess(szPath, 0) == 0)
        return true;

    for (i = 2; i < nLen; i++)
    {
        if (IsPathSlash(szPath[i]) && szPath[i-1] != ':')
        {
            cSaveChar = szPath[i];
            szPath[i] = 0;
            ::CreateDirectoryW(szPath, NULL);
            ::SetFileAttributesW(szPath, FILE_ATTRIBUTE_NORMAL);
            szPath[i] = cSaveChar;
        }
    }

    DWORD dwError = GetLastError();
    if (_waccess(szPath, 0) != 0)
    {
        std::wostringstream buf;
        if (dwError != 0)
            buf << GetSystemErrorString(dwError) << L", ";
        buf << szPath;
        LOG_ERROR2(LOGHEAD L"IDS_CREATEDIR_FAIL", buf.str());
        return false;
    }
    
    return true;
}

bool Cx_FileUtility::VerifyFileCanWrite(const wchar_t* pszFileName)
{
    if (!IsNotNull(pszFileName))
        return false;

    if (!CreateDirectory(pszFileName, false))
        return false;

    if (IsPathFileExists(pszFileName)
        && !SetFileAttributesW(pszFileName, FILE_ATTRIBUTE_NORMAL))
    {
        LOG_ERROR2(LOGHEAD L"IDS_FILE_CANNOT_WRITE", pszFileName);
        return false;
    }

    return true;
}

bool Cx_FileUtility::DeletePathFile(const wchar_t* pszFileName, bool bRecycle)
{
    if (!IsNotNull(pszFileName))
        return true;

    ASSERT(!PathIsRelativeW(pszFileName));

    if (!IsPathFileExists(pszFileName))
    {
        return true;
    }

    wchar_t szFile[MAX_PATH];
    ZeroMemory(szFile, sizeof(szFile));
    lstrcpynW(szFile, pszFileName, MAX_PATH);
    ReplaceSlash(szFile);
    PathRemoveBackslashW(szFile);

    SHFILEOPSTRUCTW op;
    ZeroMemory(&op, sizeof(op));
    op.hwnd = m_hMsgBoxOwnerWnd;
    op.wFunc = FO_DELETE;
    op.pFrom = szFile;
    op.pTo = NULL;
    op.fFlags = FOF_NOCONFIRMATION;
    if (bRecycle)
        op.fFlags |= FOF_ALLOWUNDO;
    if (NULL == m_hMsgBoxOwnerWnd)
        op.fFlags |= FOF_NOERRORUI;

    InterlockedExchange(&s_nFileOpRet, SHFileOperationW(&op));
    if (s_nFileOpRet != 0)
    {
        std::wostringstream buf;
        buf << pszFileName << L", " << GetSystemErrorString(s_nFileOpRet);

        if (IsPath(pszFileName, true))
            LOG_WARNING2(LOGHEAD L"IDS_DELFOLDER_FAIL", buf.str());
        else
            LOG_ERROR2(LOGHEAD L"IDS_DELFILE_FAIL", buf.str());
        return false;
    }

    return true;
}

bool Cx_FileUtility::TwoFileOperation(const wchar_t* pszOldFile, 
                                      const wchar_t* pszNewFile, UINT wFunc)
{
    if (!IsNotNull(pszOldFile))
        return true;

    ASSERT(!PathIsRelativeW(pszOldFile));
    bool bRet = true;
    
    if (!IsPathFileExists(pszOldFile))
    {
        LOG_INFO2(LOGHEAD L"IDS_FILE_NOTEXIST", pszOldFile);
        InterlockedExchange(&s_nFileOpRet, 0);
        return false;
    }
    else
    {
        wchar_t szOld[MAX_PATH], szNew[MAX_PATH];
        ZeroMemory(szOld, sizeof(szOld));
        ZeroMemory(szNew, sizeof(szNew));   // pTo����������\0����
        lstrcpynW(szOld, pszOldFile, MAX_PATH);
        ReplaceSlash(szOld);
        PathRemoveBackslashW(szOld);
        lstrcpynW(szNew, pszNewFile, MAX_PATH);
        ReplaceSlash(szNew);
        PathRemoveBackslashW(szNew);

        SHFILEOPSTRUCTW op;
        ZeroMemory(&op, sizeof(op));
        op.hwnd = m_hMsgBoxOwnerWnd;
        op.wFunc = wFunc;
        op.pFrom = szOld;
        op.pTo = szNew;
        op.fFlags = FOF_NOCONFIRMATION;
        if (NULL == m_hMsgBoxOwnerWnd)
            op.fFlags |= FOF_NOERRORUI;

        InterlockedExchange(&s_nFileOpRet, SHFileOperationW(&op));
        bRet = (0 == s_nFileOpRet);
    }
    
    return bRet;
}

bool Cx_FileUtility::MovePathFile(const wchar_t* pszOldFile, const wchar_t* pszNewFile)
{
    if (IsPathFileExists(pszOldFile)
        && !CreateDirectory(pszNewFile, false))
    {
        return false;
    }
    if (!TwoFileOperation(pszOldFile, pszNewFile, FO_MOVE))
    {
        if (s_nFileOpRet != 0)
        {
            std::wostringstream buf;
            buf << pszOldFile << L"->" << pszNewFile << L", ";
            buf << GetSystemErrorString(s_nFileOpRet);
            LOG_ERROR2(LOGHEAD L"IDS_MOVEFILE_FAIL", buf.str());
        }
        return false;
    }
    return true;
}

bool Cx_FileUtility::RenamePathFile(const wchar_t* pszOldFile, const wchar_t* pszNewFile)
{
    if (!TwoFileOperation(pszOldFile, pszNewFile, FO_RENAME))
    {
        if (s_nFileOpRet != 0)
        {
            std::wostringstream buf;
            buf << pszOldFile << L"->" << pszNewFile << L", ";
            buf << GetSystemErrorString(s_nFileOpRet);
            LOG_ERROR2(LOGHEAD L"IDS_RENFILE_FAIL", buf.str());
        }
        return false;
    }
    return true;
}

bool Cx_FileUtility::CopyPathFile(const wchar_t* pszOldFile, const wchar_t* pszNewFile)
{
    if (IsPathFileExists(pszOldFile)
        && !CreateDirectory(pszNewFile, false))
    {
        return false;
    }
    if (!TwoFileOperation(pszOldFile, pszNewFile, FO_COPY))
    {
        if (s_nFileOpRet != 0)
        {
            std::wostringstream buf;
            buf << pszOldFile << L"->" << pszNewFile << L", ";
            buf << GetSystemErrorString(s_nFileOpRet);
            LOG_ERROR2(LOGHEAD L"IDS_COPYFILE_FAIL", buf.str());
        }
        return false;
    }
    return true;
}

void Cx_FileUtility::SetMsgBoxOwnerWnd(HWND hWnd)
{
    m_hMsgBoxOwnerWnd = hWnd;
}

HWND Cx_FileUtility::GetMsgBoxOwnerWnd()
{
    return m_hMsgBoxOwnerWnd;
}

std::wstring Cx_FileUtility::RelToAbs(const wchar_t* pszRel, bool bRelIsFile, 
                                      const wchar_t* pszBase, bool bBaseIsFile)
{
    wchar_t szPath[MAX_PATH * 2] = { 0 };
    
    if (pszRel != NULL)
    {
        if (IsPathSlash(pszRel[0]) && !IsPathSlash(pszRel[1])   // "\xxx"
            && (!IsNotNull(pszBase) || StrChrW(pszBase, ':') != NULL))
        {
            if (IsNotNull(pszBase))
            {
                lstrcpynW(szPath, pszBase, MAX_PATH);
            }
            else
            {
                GetModuleFileNameW(GetMainModuleHandle(), szPath, MAX_PATH);
            }

            lstrcpynW(StrChrW(szPath, ':') + 1, pszRel, MAX_PATH);
        }
        else if (PathIsRelativeW(pszRel))
        {
            if (IsNotNull(pszBase))
            {
                lstrcpynW(szPath, pszBase, MAX_PATH);
                if (bBaseIsFile)
                {
                    ReplaceSlash(szPath);
                    PathRemoveFileSpecW(szPath);
                }
            }
            else
            {
                GetModuleFileNameW(GetMainModuleHandle(), szPath, MAX_PATH);
                PathRemoveFileSpecW(szPath);
            }
            PathAppendW(szPath, pszRel);
        }
        else
        {
            lstrcpynW(szPath, pszRel, MAX_PATH);
        }
        
        PathRemoveBackslashW(szPath);
        if (!bRelIsFile)
            PathAddBackslashW(szPath);
    }

    return szPath;
}

std::wstring Cx_FileUtility::AbsToRel(const wchar_t* pszAbs, bool bAbsIsFile, 
                                      const wchar_t* pszBase, bool bBaseIsFile)
{
    wchar_t szPath[MAX_PATH] = { 0 };
    wchar_t szBasePath[MAX_PATH] = { 0 };
    
    if (pszAbs != NULL)
    {
        if (!IsNotNull(pszBase))
        {
            GetModuleFileNameW(GetMainModuleHandle(), szBasePath, MAX_PATH);
            pszBase = szBasePath;
            bBaseIsFile = true;
        }
        if (IsNotNull(pszBase) && PathRelativePathToW(szPath, pszBase, 
            bBaseIsFile ? FILE_ATTRIBUTE_NORMAL : FILE_ATTRIBUTE_DIRECTORY, 
            pszAbs, 
            bAbsIsFile ? FILE_ATTRIBUTE_NORMAL : FILE_ATTRIBUTE_DIRECTORY))
        {
            if (IsPathSlash(szPath[0]))     //  \xxx  ->  .\xxx
            {
                StrCpyW(szBasePath, szPath);
                szPath[0] = '.';
                StrCpyW(szPath + 1, szBasePath);
            }
        }
        else
        {
            lstrcpynW(szPath, pszAbs, MAX_PATH);
        }
    }

    return szPath;
}

std::wstring Cx_FileUtility::ChangeFileNameSuffix(const wchar_t* pszFileName, 
                                                  const wchar_t* pszSuffix)
{
    wchar_t szNewFile[MAX_PATH] = { 0 };

    if (pszFileName != NULL && pszSuffix != NULL)
    {
        ASSERT(wcschr(pszSuffix, '.') != NULL);
        lstrcpynW(szNewFile, pszFileName, MAX_PATH);
        PathRenameExtensionW(szNewFile, pszSuffix);
    }

    return szNewFile;
}

std::wstring Cx_FileUtility::GetFileTitle(const wchar_t* pszFileName)
{
    wchar_t szNewFile[MAX_PATH] = { 0 };

    if (pszFileName != NULL)
    {
        wchar_t* pszName = PathFindFileNameW(pszFileName);
        if (pszName != NULL)
        {
            lstrcpynW(szNewFile, pszName, MAX_PATH);
            ReplaceSlash(szNewFile);
            PathRemoveExtensionW(szNewFile);
        }
    }

    return szNewFile;
}

std::wstring Cx_FileUtility::GetFileName(const wchar_t* pszFileName)
{
    if (NULL == pszFileName || 0 == pszFileName[0])
        return L"";

    wchar_t szTemp[MAX_PATH];
    lstrcpynW(szTemp, PathFindFileNameW(pszFileName), MAX_PATH);
    PathRemoveBackslashW(szTemp);

    return szTemp;
}

std::wstring Cx_FileUtility::GetExtension(const wchar_t* pszFileName)
{
    return pszFileName ? PathFindExtensionW(pszFileName) : L"";
}

std::wstring Cx_FileUtility::GetPathOfFile(const wchar_t* pszFileName)
{
    wchar_t szPath[MAX_PATH] = { 0 };
    
    if (pszFileName != NULL)
    {
        lstrcpynW(szPath, pszFileName, MAX_PATH);
        ReplaceSlash(szPath);
        PathRemoveBackslashW(szPath);
        PathRemoveFileSpecW(szPath);
        if (szPath[0] != 0)
            PathAddBackslashW(szPath);
    }
    
    return szPath;
}

std::wstring Cx_FileUtility::MakeFileName(const std::wstring& wstrPath, 
                                          const std::wstring& wstrFileTitle, 
                                          const std::wstring& wstrExtName)
{
    wchar_t szFileName[MAX_PATH * 2] = { 0 };

    if (wstrPath.empty() || wstrFileTitle.empty())
        return szFileName;

    lstrcpynW(szFileName, wstrPath.c_str(), MAX_PATH);

    if (wstrExtName.empty())
    {
        PathAppendW(szFileName, GetFileName(wstrFileTitle.c_str()).c_str());
    }
    else if (StrChrW(wstrExtName.c_str(), '.') == NULL)
    {
        PathAppendW(szFileName, GetFileTitle(wstrFileTitle.c_str()).c_str());
        lstrcatW(szFileName, L".");
        lstrcatW(szFileName, wstrExtName.c_str());
    }
    else
    {
        PathAppendW(szFileName, GetFileTitle(wstrFileTitle.c_str()).c_str());
        lstrcatW(szFileName, PathFindExtensionW(wstrExtName.c_str()));
    }

    szFileName[MAX_PATH - 1] = 0;

    return szFileName;
}

std::wstring Cx_FileUtility::CreateFileName(const std::wstring& wstrPath, 
                                            const std::wstring& wstrPrefix, 
                                            const std::wstring& wstrExtName, 
                                            bool bReturnRel)
{
    wchar_t szFileName[MAX_PATH] = { 0 };

    for (int i = 0; i < 9999; i++)
    {
        if (0 == i)
        {
            wcscpy_s(szFileName, _countof(szFileName), wstrPrefix.c_str());
        }
        else
        {
            swprintf_s(szFileName, _countof(szFileName), L"%s%03d", wstrPrefix.c_str(), i);
        }
        if (!wstrExtName.empty())
        {
            if (wcschr(wstrExtName.c_str(), '.') == NULL)
                wcscat_s(szFileName, _countof(szFileName), L".");
            wcscat_s(szFileName, _countof(szFileName), wstrExtName.c_str());
        }

        std::wstring wstrFile (RelToAbs(szFileName, true, wstrPath.c_str(), false));

        if (!FileUtility()->IsPathFileExists(wstrFile.c_str()))
        {
            if (!bReturnRel)
                return wstrFile;
            break;
        }
    }

    return szFileName;
}

std::wstring Cx_FileUtility::GetModifyTime(const std::wstring& wstrFileName)
{
    wchar_t szTime[20] = { 0 };

    HANDLE hFile = ::CreateFileW(wstrFileName.c_str(), 
        GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        FILETIME ftCreate, ftAccess, ftWrite;
        SYSTEMTIME stUTC, stLocal;

        if (::GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
        {
            FileTimeToSystemTime(&ftWrite, &stUTC);
            SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
            
            swprintf_s(szTime, _countof(szTime), L"%04d-%02d-%02d %02d:%02d:%02d",
                stLocal.wYear, stLocal.wMonth, stLocal.wDay, 
                stLocal.wHour, stLocal.wMinute, stLocal.wSecond);
        }

        ::CloseHandle(hFile);
    }

    return szTime;
}

ULONG Cx_FileUtility::GetFileSize(const std::wstring& wstrFileName)
{
    ULONG nFileSize = 0;
    HANDLE hFile = ::CreateFileW(wstrFileName.c_str(), 
        GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwFileSizeHigh = 0;
        nFileSize = ::GetFileSize(hFile, &dwFileSizeHigh);
        if (dwFileSizeHigh > 0)
            nFileSize = (ULONG)-1;

        ::CloseHandle(hFile);
    }

    return nFileSize;
}

int Cx_FileUtility::CompareFileName(const wchar_t* pszFileName1, const wchar_t* pszFileName2, 
                                    long* pSamePartCount)
{
    int nRet = 0;

    if (!IsNotNull(pszFileName1) || !IsNotNull(pszFileName2))
    {
        if (IsNotNull(pszFileName1) != IsNotNull(pszFileName2))
        {
            nRet = (IsNotNull(pszFileName1) > IsNotNull(pszFileName2)) ? 1 : -1;
        }
        if (pSamePartCount)
            *pSamePartCount = 0;
        return nRet;
    }

    long nSamePartCount = 0;
    const wchar_t* pszFile1 = pszFileName1;
    const wchar_t* pszFile2 = pszFileName2;

    if ('.' == pszFile1[0] && IsPathSlash(pszFile1[1]))
        pszFile1 += 2;
    if ('.' == pszFile2[0] && IsPathSlash(pszFile2[1]))
        pszFile2 += 2;

    while (0 == nRet && (*pszFile1 != 0 || *pszFile2 != 0))
    {
        int nPos1 = StrCSpnW(pszFile1, L"\\/");
        int nPos2 = StrCSpnW(pszFile2, L"\\/");

        nRet = StrCmpNIW(pszFile1, pszFile2, max(nPos1, nPos2));
        if (0 == nRet && nPos1 > 0)
        {
            nSamePartCount++;
        }

        if (0 == pszFile1[nPos1])
            pszFile1 += nPos1;
        else
            pszFile1 += nPos1 + 1;

        if (0 == pszFile2[nPos2])
            pszFile2 += nPos2;
        else
            pszFile2 += nPos2 + 1;
    }

    if (pSamePartCount)
        *pSamePartCount = nSamePartCount;

    return nRet;
}

#pragma comment(lib, "version.lib")

bool Cx_FileUtility::GetFileVersion(
        std::wstring& version, const std::wstring& filename)
{
    WORD ver1, ver2, ver3, ver4;

    version.resize(0);
    if (GetFileVersion(ver1, ver2, ver3, ver4, filename))
    {
    }

    return !version.empty();
}

bool Cx_FileUtility::GetFileVersion(
        WORD& ver1, WORD& ver2, WORD& ver3, WORD& ver4, 
        const std::wstring& filename)
{
    DWORD handle = 0;
    wchar_t *block = NULL;
    bool ret = false;

    ver1 = 0;
    ver2 = 0;
    ver3 = 0;
    ver4 = 0;

    UINT size = ::GetFileVersionInfoSizeW((wchar_t*)filename.c_str(), &handle);
    if (size > 0 && NULL != (block = new wchar_t[size]))
    {
        ::GetFileVersionInfoW((wchar_t*)filename.c_str(), handle, size, block);

        VS_FIXEDFILEINFO* pFixedInfo = NULL;
        if (::VerQueryValueW(block, L"\\", (LPVOID*)&pFixedInfo, &size))
        {
            ret = true;
            ver1 = HIWORD(pFixedInfo->dwFileVersionMS);
            ver2 = LOWORD(pFixedInfo->dwFileVersionMS);
            ver3 = HIWORD(pFixedInfo->dwFileVersionLS);
            ver4 = LOWORD(pFixedInfo->dwFileVersionLS);
        }

        delete[] block;
    }

    return ret;
}

bool Cx_FileUtility::GetFileDescription(
        std::wstring& description, const std::wstring& filename)
{
    DWORD handle = 0;
    wchar_t *block = NULL;
    bool ret = false;

    description.resize(0);

    UINT size = ::GetFileVersionInfoSizeW((wchar_t*)filename.c_str(), &handle);
    if (size > 0 && NULL != (block = new wchar_t[size]))
    {
        ::GetFileVersionInfoW((wchar_t*)filename.c_str(), handle, size, block);

        LPWORD lpTranslate = NULL;
        wchar_t szSubBlock[41] = { 0 };
        wchar_t* szStr;

        if (::VerQueryValueW(block, L"\\VarFileInfo\\Translation",
            (LPVOID*)&lpTranslate, &size))
        {
            ret = true;

            swprintf_s(szSubBlock, _countof(szSubBlock), 
                L"\\StringFileInfo\\%04x%04x\\FileDescription",
                lpTranslate[0], lpTranslate[1]);
            if (::VerQueryValueW(block, szSubBlock, (LPVOID*)&szStr, &size))
            {
                description = szStr;
            }
        }

        delete[] block;
    }

    return ret;
}