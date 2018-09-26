#ifndef _STRLIB_H_
#define _STRLIB_H_


 int CheckIPString(char strIPAddr[]);
 double AtoF(const char strBuf[]);
 int AtoI(const char strBuf[]);
 int TrimRightString(char strBuf[]);
 int TrimLeftString(char strBuf[]);
 int TrimString(char strBuf[]);
 int BreakLine(char strLine[100], char strTokens[4][100]);

#endif /** _STRLIB_H_ */
