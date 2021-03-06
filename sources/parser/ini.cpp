///////////////////////////////////////////////////////////////////////////////
// FileName:    ini.cpp
// Created:     2009/04/11
// Author:      titilima
// CopyRight:   Titi Studio (?) 2001-2009
//-----------------------------------------------------------------------------
// Information: ini parser
///////////////////////////////////////////////////////////////////////////////

#include "..\..\include\pdl_parser.h"
#include "..\..\include\pdl_file.h"
#include "..\..\include\pdl_module.h"
#ifdef _WIN32_WCE
#include "..\adaptor\wince_adaptor.h"
#endif // _WIN32_WCE

LIniParser::LIniParser(__in ILock* lock) : LStrList(lock)
{
    m_bDirty = FALSE;
    m_secList.Create(sizeof(LIterator));
}

LIniParser::~LIniParser(void)
{
    Close();
}

void LIniParser::Close(void)
{
    LStrList::Clear();
    m_secList.Clear();
}

// LIniParser::FindKey
// 返回指定 Section、指定 Key 的行首位置。
// [in] lpszSection: 要查找的 Section 名称。
// [in] lpszKey: 要查找的 Key 名称。
// [ret] 如果找到，则返回该行的起始位置，否则返回 -1。

LIterator LIniParser::FindKey(__in PCSTR lpszSection, __in PCSTR lpszKey)
{
    LIterator itSec = FindSection(lpszSection);
    if (NULL == itSec)
        return NULL;

    LIterator itNext = FindNextSection(itSec);
    while (itSec != itNext)
    {
        LStringA str;
        LStrList::GetAt(itSec, &str);
        int nEqual = str.Find('=');
        if (-1 != nEqual)
        {
            LStringA strKey = str.Left(nEqual);
            strKey.Trim(" \t");
            if (0 == lstrcmpiA(lpszKey, strKey))
                return itSec;
        }

        itSec = m_secList.GetNextIterator(itSec);
    }
    return NULL;
}

LIterator LIniParser::FindNextSection(__in LIterator it)
{
    LIterator it2 = m_secList.GetHeadIterator();
    LIterator itSec = NULL;
    while (NULL != it2)
    {
        m_secList.GetAt(it2, &itSec);
        it2 = m_secList.GetNextIterator(it2);
        if (itSec == it && NULL != it2)
        {
            m_secList.GetAt(it2, &itSec);
            return itSec;
        }
    }
    return NULL;
}

// LIniParser::FindSection
// 查找指定的 Section，返回行首位置。
// [in] lpszSection: 要查找的 Section 名称。
// [ret] 如果成功则返回指定的 Section 行首位置，否则返回 -1。

LIterator LIniParser::FindSection(__in PCSTR lpszSection)
{
    LIterator it = m_secList.GetHeadIterator();
    LIterator itSec = NULL;
    while (NULL != it)
    {
        m_secList.GetAt(it, &itSec);

        LStringA str = LStrList::GetAt(itSec);
        str.Trim(" \t");
        str.Trim("[]");
        if (0 == lstrcmpiA(lpszSection, str))
            return itSec;

        it = m_secList.GetNextIterator(it);
    }
    return NULL;
}

int LIniParser::GetInt(
    __in PCSTR lpszSection,
    __in PCSTR lpszKey,
    __in int nDefault)
{
    PSTR pszRet = GetStringA(lpszSection, lpszKey);
    int nRet = nDefault;

    if (NULL != pszRet)
    {
        if (isdigit(pszRet[0]))
            nRet = atoi(pszRet);
        else
            nRet = nDefault;
        delete [] pszRet;
    }
    else
    {
        nRet = nDefault;
    }
    return nRet;
}

DWORD LIniParser::GetString(
    __in PCSTR lpszSection,
    __in PCSTR lpszKey,
    __in PCSTR lpszDefault,
    __out LStringA* strResult)
{
    PSTR pszRet = GetStringA(lpszSection, lpszKey);
    if (NULL != pszRet)
        strResult->Attach(pszRet);
    else
        *strResult = lpszDefault;
    return strResult->GetLength();
}

DWORD LIniParser::GetString(
    __in PCSTR lpszSection,
    __in PCSTR lpszKey,
    __in PCWSTR lpszDefault,
    __out LStringW* strResult)
{
    PSTR pszRet = GetStringA(lpszSection, lpszKey);
    if (NULL != pszRet)
    {
        strResult->Copy(pszRet);
        delete [] pszRet;
    }
    else
    {
        *strResult = lpszDefault;
    }
    return strResult->GetLength();
}

DWORD LIniParser::GetString(
    __in PCSTR lpszSection,
    __in PCSTR lpszKey,
    __in PCSTR lpszDefault,
    __out PSTR lpszBuffer,
    __in DWORD nSize)
{
    PSTR pszRet = GetStringA(lpszSection, lpszKey);
    if (NULL != pszRet)
    {
        lstrcpynA(lpszBuffer, pszRet, nSize);
        delete [] pszRet;
    }
    else
    {
        lstrcpynA(lpszBuffer, lpszDefault, nSize);
    }
    return lstrlenA(lpszBuffer);
}

DWORD LIniParser::GetString(
    __in PCSTR lpszSection,
    __in PCSTR lpszKey,
    __in PCWSTR lpszDefault,
    __out PWSTR lpszBuffer,
    __in DWORD nSize)
{
    PSTR pszRet = GetStringA(lpszSection, lpszKey);
    if (NULL != pszRet)
    {
        MultiByteToWideChar(CP_ACP, 0, pszRet, -1, lpszBuffer, nSize);
        delete [] pszRet;
    }
    else
    {
        lstrcpynW(lpszBuffer, lpszDefault, nSize);
    }
    return lstrlenW(lpszBuffer);
}

DWORD LIniParser::GetSectionCount(void)
{
    return m_secList.GetCount();
}

// LIniParser::GetStringA
// 获取指定 Section、指定 Key 处的原始字符串。
// [in] lpszSection: 要查找的 Section 名称。
// [in] lpszKey: 要查找的 Key 名称。
// [ret] 如果获取成功则返回原始字符串，这个字符串需要调用 delete [] 来销毁。
//       如果获取失败，则返回 NULL。

PSTR LIniParser::GetStringA(__in PCSTR lpszSection, __in PCSTR lpszKey)
{
    LIterator it = FindKey(lpszSection, lpszKey);
    if (NULL == it)
        return NULL;

    PCSTR lpLine = LStrList::GetAt(it);
    LStringA str = strchr(lpLine, '=') + 1;
    str.Trim(" \t");
    return str.Detach();
}

BOOL LIniParser::Open(__in PCSTR lpszFileName)
{
    LStrList::Clear();
    m_secList.Clear();
    if (NULL == lpszFileName)
        return FALSE;

    LStringA path;
    if (LFile::IsFullPathName(lpszFileName))
    {
        path = lpszFileName;
    }
    else
    {
        LAppModule::GetModulePath(NULL, &path);
        path += lpszFileName;
    }
    if (!LFile::Exists(path))
        return FALSE;

    LTxtFile file;
    if (!file.Open(path, LTxtFile::modeReadWrite))
        return FALSE;

    Open(&file);
    return TRUE;
}

BOOL LIniParser::Open(__in PCWSTR lpszFileName)
{
    LStrList::Clear();
    m_secList.Clear();
    if (NULL == lpszFileName)
        return FALSE;

    LStringW path;
    if (LFile::IsFullPathName(lpszFileName))
    {
        path = lpszFileName;
    }
    else
    {
        LAppModule::GetModulePath(NULL, &path);
        path += lpszFileName;
    }
    if (!LFile::Exists(path))
        return FALSE;

    LTxtFile file;
    if (!file.Open(path, LTxtFile::modeReadWrite))
        return FALSE;

    Open(&file);
    return TRUE;
}

// Open
// 打开一个 ini 文件。
// [in] pFile: ini 文件的对象指针。

void LIniParser::Open(__in LTxtFile* pFile)
{
    LStringA str;
    while (!pFile->Eof())
    {
        pFile->ReadLn(&str);
        if ('\0' != str[0])
            LStrList::AddTail(str);

        int len = str.Trim(" \t");
        if (len > 0 && '[' == str[0] && ']' == str[len - 1])
            m_secList.AddTail(&m_itTail);
    }
    m_bDirty = FALSE;
}

void LIniParser::RemoveSection(__in PCSTR lpszSection)
{
    LIterator itSec = FindSection(lpszSection);
    LIterator itNext = FindNextSection(itSec);
    while (itNext != itSec)
    {
        LIterator itDel = itSec;
        itSec = GetNextIterator(itSec);
        Remove(itDel);
    }
}

BOOL LIniParser::Save(__in_opt PCSTR lpszFileName)
{
    if (NULL == lpszFileName || !m_bDirty)
        return FALSE;

    LStringA path;
    if (LFile::IsFullPathName(lpszFileName))
    {
        path = lpszFileName;
    }
    else
    {
        LAppModule::GetModulePath(NULL, &path);
        path += lpszFileName;
    }
    return LStrList::SaveToFile(path, SLFILE_CLEAR | SLFILE_INCLUDENULL);
}

BOOL LIniParser::Save(__in_opt PCWSTR lpszFileName)
{
    if (NULL == lpszFileName || !m_bDirty)
        return FALSE;

    LStringW path;
    if (LFile::IsFullPathName(lpszFileName))
    {
        path = lpszFileName;
    }
    else
    {
        LAppModule::GetModulePath(NULL, &path);
        path += lpszFileName;
    }
    return LStrList::SaveToFile(path, SLFILE_CLEAR | SLFILE_INCLUDENULL);
}

BOOL LIniParser::WriteInt(
    __in PCSTR lpszSection,
    __in PCSTR lpszKey,
    __in int nValue)
{
    CHAR str[20];
    wsprintfA(str, "%d", nValue);
    return WriteStringA(lpszSection, lpszKey, str);
}

BOOL LIniParser::WriteString(
    __in PCSTR lpszSection,
    __in PCSTR lpszKey,
    __in PCSTR lpszValue)
{
    return WriteStringA(lpszSection, lpszKey, lpszValue);
}

BOOL LIniParser::WriteString(
    __in PCSTR lpszSection,
    __in PCSTR lpszKey,
    __in PCWSTR lpszValue)
{
    LStringA str = lpszValue;
    return WriteStringA(lpszSection, lpszKey, str);
}

// LIniParser::WriteStringA
// 将指定的 Section 和 Key 处设置为指定的字符串。
// [in] lpszSection: 要写入的 Section 名称。
// [in] lpszKey: 要写入的 Key 名称。
// [in] lpszValue: 要写入的字符串。
// [ret] 如果成功则返回 TRUE，否则返回 FALSE。

BOOL LIniParser::WriteStringA(PCSTR lpszSection, PCSTR lpszKey, PCSTR lpszValue)
{
    m_bDirty = TRUE;

    LStringA str;
    LIterator itSec = FindSection(lpszSection);
    if (NULL == itSec)
    {
        // 不存在指定的 section
        str.Format("[%s]", lpszSection);
        LStrList::AddTail(str);
        m_secList.AddTail(&m_itTail);
        itSec = m_itTail;
    }

    str.Format("%s=%s", lpszKey, lpszValue);
    LIterator itKey = FindKey(lpszSection, lpszKey);
    if (NULL != itKey)
    {
        LStrList::SetAt(itKey, str);
        return TRUE;
    }

    // 尾部插入新的值
    LIterator it = FindNextSection(itSec);
    if (NULL != it)
        LStrList::InsertBefore(it, str);
    else
        LStrList::AddTail(str);
    return TRUE;
}
