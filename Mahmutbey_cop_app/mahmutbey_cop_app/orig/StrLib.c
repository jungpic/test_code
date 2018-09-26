#include <string.h>

#include "StrLib.h"

static int SeparateString(char strBuf[], const char * strSepChar);
static int IsSpace( char strChar );
static int IsNumbericChar( char strChar );

int CheckIPString(char strIPAddr[])
{
    int iLen = 0;
    int iNumCount = 0;
    int iDotCount = 0;
    int iLoop = 0;
    int iRet = 0;

    if(strcmp(strIPAddr, " ") == 0)
    {
        iRet = -1;
    }
    else
    {
        iLen = strlen(strIPAddr);

        if( iLen > 15 || iLen < 7 )
        {
            iRet = -1;
        }
        else
        {
            for( iLoop = 0; iLoop < iLen; iLoop++)
            {
                if(strIPAddr[iLoop] < '0' || strIPAddr[iLoop] > '9')
                {
                    if(strIPAddr[iLoop] == '.')
                    {
                        ++iDotCount;
                        iNumCount = 0;
                    }
                    else
                    {
                        iRet = -1;
                    }
                }
                else
                {
                    if(++iNumCount > 3)
                    {
                        iRet = -1;
                    }
                }
                if(iRet == -1)
                {
                    break;
                }
            }
        }
    }

    if(iDotCount != 3)
    {
        iRet = -1;
    }

    return iRet;
}

static int IsSpace( char strChar )
{
    int iRet = 0;

    if( (strChar == ' ') || (strChar == '\f') )
    {
        iRet = 1;
    }
    else if( (strChar == (char)'\n') || (strChar == (char)'\r') )
    {
        iRet = 1;
    }
    else if( (strChar == '\t') || (strChar == '\v') )
    {
        iRet = 1;
    }
    else
    {
        iRet = 0;
    }

    return iRet;
}

static int IsNumbericChar( char strChar )
{
    int iRet = 0;

    if( (strChar >= '0') && (strChar <= '9') )
    {
        iRet = 1;
    }
    else
    {
        iRet = 0;
    }

    return iRet;
}

double AtoF(const char strBuf[])
{
    double dRet = 0.0;
    double dRetD = 0.0;
    double dRetE = 0.0;
    int iIndex = 0;
    int iRetSpace = 0;
    int iRetNumber = 0;
    int iSign = 1;
    int iEoS = 1;
    double dFloatingPoint = 0.0;
    int ucFloatingValue = 0;

    while( strBuf[iIndex] != '\0')
    {
        iRetSpace = IsSpace(strBuf[iIndex]);

        if(iRetSpace == 0)
        {
            iEoS = 0;
            break;
        }
        else
        {
            iIndex++;
        }
    }

    if(iEoS == 1)
    {
        dRet = 0.0;
    }
    else
    {
        if(strBuf[iIndex] == '-')
        {
            iSign = -1;
            iIndex++;
        }
        while( strBuf[iIndex] != '\0')
        {
            iRetSpace = IsSpace(strBuf[iIndex]);
            iRetNumber = IsNumbericChar(strBuf[iIndex]);
            if( (iRetSpace != 0) || (iRetNumber != 1) )
            {
                if( (strBuf[iIndex] == '.') && (ucFloatingValue == 0) )
                {
                    ucFloatingValue = 1;
                    dFloatingPoint = 0.1;
                    iIndex++;
                }
                else
                {
                    iEoS = 0;
                    break;
                }
            }
            else if(dFloatingPoint > 0.0)
            {
                /* dRetE = dRetE + ((double)((int)strBuf[iIndex] - (int)'0') * dFloatingPoint); */
                dRetE = dRetE + (((double)strBuf[iIndex] - (double)'0') * dFloatingPoint);
                dFloatingPoint = dFloatingPoint * 0.1;
                iIndex++;
            }
            else
            {
                /* dRetD = (dRetD * 10.0) + (double)((int)strBuf[iIndex] - (int)'0'); */
                dRetD = (dRetD * 10.0) + ((double)strBuf[iIndex] - (double)'0');
                iIndex++;
            }
        }
        dRet = (double)iSign * (dRetD + dRetE);
    }
    return dRet;
}

int AtoI(const char strBuf[])
{
    int iIndex = 0;
    int iRetSpace = 0;
    int iRetNumber = 0;
    int iSign = 1;
    int iRet = 0;
    int iEoS = 1;

    while( strBuf[iIndex] != '\0')
    {
        iRetSpace = IsSpace(strBuf[iIndex]);

        if(iRetSpace == 0)
        {
            iEoS = 0;
            break;
        }
        else
        {
            iIndex++;
        }
    }

    if(iEoS == 1)
    {
        iRet = 0;
    }
    else
    {
        if(strBuf[iIndex] == '-')
        {
            iSign = -1;
            iIndex++;
        }
        while( strBuf[iIndex] != '\0')
        {
            iRetSpace = IsSpace(strBuf[iIndex]);
            iRetNumber = IsNumbericChar(strBuf[iIndex]);
            if( (iRetSpace != 0) || (iRetNumber != 1) )
            {
                iEoS = 0;
                break;
            }
            else
            {
                iRet = (iRet * 10) + ((int)strBuf[iIndex] - (int)'0');
                iIndex++;
            }
        }
        iRet = iSign * iRet;
    }
    return iRet;
}


 int TrimRightString( char strBuf[] )
{

    int iRet = 0;
    int iLen = 0;
    int iIndex = 0;
    int iRetSpace = 0;

    if (strBuf == NULL)
    {
        iRet = -1;
    }
    else
    {
        iLen = (int)strlen(strBuf);
        iIndex = iLen - 1;
        while(iIndex >= 0)
        {
            iRetSpace = IsSpace(strBuf[iIndex]);
            if(iRetSpace == 0)
            {
                break;
            }
            else
            {
                strBuf[iIndex] = '\0';
                iIndex--;
            }
        }
    }

    return iRet;    
}

 int TrimLeftString( char strBuf[] )
{
    int iRet = 0;
    int iLen = 0;
    int iIndex = 0;
    int iRetSpace = 0;

    if (strBuf == NULL)
    {
        iRet = -1;
    }
    else
    {
        iLen = (int)strlen(strBuf);
        iIndex = 0;
        while(iIndex < iLen)
        {
            iRetSpace = IsSpace(strBuf[iIndex]);
            if(iRetSpace == 0)
            {
                break;
            }
            else
            {
                iIndex++;
            }
        }

        if( strcpy(strBuf, &strBuf[iIndex]) == NULL )
        {
            iRet = -1;
        }
    }

    return iRet;
}

 int TrimString(char strBuf[])
{
    int iRet = 0;
    
    if (strBuf == NULL)
    {
        iRet = -1;
    }
    else
    {
        iRet = TrimLeftString(strBuf);
        if(iRet != -1)
        {
            iRet = TrimRightString(strBuf);
        }
    }

    return iRet;
}

static int SeparateString(char strBuf[], const char * strSepChar)
{
    int iIndex = 0;
    int iIndex1 = 0;

    while(strBuf[iIndex] != '\0')
    {
        if(strchr(strSepChar, (int)strBuf[iIndex]) != NULL)
        {
            iIndex1 = iIndex + 1;
            if(strBuf[iIndex1] != ' ')
            {
                iIndex = iIndex1;
                break;
            }
        }
        iIndex++;
    }

    return iIndex;
}

 int BreakLine(char strLine[100], char strTokens[4][100])
{
    int iIndex = 0;
    int iRet = 0;
    int iNTok = 0;
    int iBreak = 0;
    const char *strSepChar = " \t\n";

    iRet = TrimString(strLine);

    if(iRet != -1)
    {
        for(iNTok = 0; iNTok < 4; iNTok++)
        {
            if(memset(&strTokens[iNTok][0], 0x00, 120U) == NULL)
            {
                iBreak = 1;
            }
            iRet = SeparateString(&strLine[iIndex], strSepChar);
            if(strncpy(&strTokens[iNTok][0], &strLine[iIndex], (unsigned int)iRet) == NULL)
            {
                iBreak = 1;
            }

            iIndex = iIndex+iRet;

            if(iBreak == 1)
            {
                break;
            }
        }
    }

    return iRet;
}
