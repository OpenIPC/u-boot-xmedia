/*
 * 
 */

#ifndef SECUREC_FOR_WCHAR
#define SECUREC_FOR_WCHAR
#endif

#include "secureprintoutput.h"

/*
 * <FUNCTION DESCRIPTION>
 *    The  vswprintf_s  function  is  the  wide-character  equivalent  of the vsprintf_s function
 *
 * <INPUT PARAMETERS>
 *    strDest                  Storage location for the output.
 *    destMax                  Maximum number of characters to store
 *    format                   Format specification.
 *    argList                  pointer to list of arguments
 *
 * <OUTPUT PARAMETERS>
 *    strDest                 is updated
 *
 * <RETURN VALUE>
 *    return  the number of wide characters stored in strDest, not  counting the terminating null wide character.
 *    return -1  if an error occurred.
 *
 * If there is a runtime-constraint violation, strDest[0] will be set to the '\0' when strDest and destMax valid
 */
int vswprintf_s(wchar_t *strDest, size_t destMax, const wchar_t *format, va_list argList)
{
    int retVal;               /* If initialization causes  e838 */
    if (SECUREC_VSPRINTF_PARAM_ERROR(format, strDest, destMax, SECUREC_WCHAR_STRING_MAX_LEN)) {
        SECUREC_VSPRINTF_CLEAR_DEST(strDest, destMax, SECUREC_WCHAR_STRING_MAX_LEN);
        SECUREC_ERROR_INVALID_PARAMTER("vswprintf_s");
        return -1;
    }

    retVal = SecVswprintfImpl(strDest, destMax, format, argList);
    if (retVal < 0) {
        strDest[0] = L'\0';
        if (retVal == SECUREC_PRINTF_TRUNCATE) {
            /* Buffer too small */
            SECUREC_ERROR_INVALID_RANGE("vswprintf_s");
        }
        SECUREC_ERROR_INVALID_PARAMTER("vswprintf_s");
        return -1;
    }

    return retVal;
}

