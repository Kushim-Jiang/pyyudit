/** 
 *  Yudit Unicode Editor Source File
 *
 *  GNU Copyright (C) 1997-2023  Gaspar Sinai <gaspar@yudit.org>  
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License, version 2,
 *  dated June 1991. See file COPYYING for details.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include "swindow/SFontTTF.h"
#include "swindow/STTables.h"

/*
 * ntohl 
 */
#ifndef USE_WINAPI
#include <netinet/in.h>
#else
#include <winsock.h>
#endif

#include <stdio.h>
#include <ctype.h>
#include <math.h>

#define DEBUG 0
#define HAS_CFF2 0

static const SString SS_TB_CFF("CFF ");
static const SString SS_TB_CFF2("CFF2");

#define SD_DICT_OP_Version 0x00
#define SD_DICT_OP_Notice 0x01
#define SD_DICT_OP_FullName 0x02
#define SD_DICT_OP_FamilyName 0x03
#define SD_DICT_OP_Weight 0x04
#define SD_DICT_OP_FontBox 0x05
#define SD_DICT_OP_BlueValues 0x06
#define SD_DICT_OP_OtherBlues 0x07
#define SD_DICT_OP_FamilyBlues 0x08
#define SD_DICT_OP_FamilyOtherBlues 0x09
#define SD_DICT_OP_StdHW 0x0a
#define SD_DICT_OP_StdVW 0x0b
// purosefully shifted << 8
#define SD_DICT_OP_escape 0x0c00
#define SD_DICT_OP_UniqeuID 0x0d
#define SD_DICT_OP_XUID 0x0e
#define SD_DICT_OP_CharSet 0x0f
#define SD_DICT_OP_Encoding 0x10
#define SD_DICT_OP_CharStrings 0x11
#define SD_DICT_OP_Private 0x12
#define SD_DICT_OP_Subrs 0x13
#define SD_DICT_OP_DefaultWidthX 0x14
#define SD_DICT_OP_NominalWidthX 0x15
// Reserved
// 16 + 7 = 23
#define SD_DICT_OP_blend 0x17
// Reserved
#define SD_DICT_OP_shortint 0x1c
#define SD_DICT_OP_longint 0x1d
#define SD_DICT_OP_BCD 0x1e

// 2 Byte DICT Operators
#define SD_DICT_OP_Copyriight (0x00 | SD_DICT_OP_escape)
#define SD_DICT_OP_isFixedPitch (0x01 | SD_DICT_OP_escape)
#define SD_DICT_OP_isItalicAngle (0x02 | SD_DICT_OP_escape)
#define SD_DICT_OP_UnderlinePosition (0x03 | SD_DICT_OP_escape)
#define SD_DICT_OP_UnderlineThickness (0x04 | SD_DICT_OP_escape)
#define SD_DICT_OP_PaintType (0x05 | SD_DICT_OP_escape)
#define SD_DICT_OP_CharStringType (0x06 | SD_DICT_OP_escape)
#define SD_DICT_OP_FontMatrix (0x07 | SD_DICT_OP_escape)
#define SD_DICT_OP_StrokeWidth (0x08 | SD_DICT_OP_escape)
#define SD_DICT_OP_BlueScale (0x09 | SD_DICT_OP_escape)
#define SD_DICT_OP_BlueShift (0x0a | SD_DICT_OP_escape)
#define SD_DICT_OP_BlueFuzx (0x0b | SD_DICT_OP_escape)
#define SD_DICT_OP_SpemSnapH (0x0c | SD_DICT_OP_escape)
#define SD_DICT_OP_SpemSnapV (0x0d | SD_DICT_OP_escape)
#define SD_DICT_OP_ForceBold (0x0e | SD_DICT_OP_escape)
//reserved
#define SD_DICT_OP_LanguageGroup (0x11 | SD_DICT_OP_escape)
#define SD_DICT_OP_ExpansionFactor (0x12 | SD_DICT_OP_escape)
#define SD_DICT_OP_InitialRandomSpeed (0x13 | SD_DICT_OP_escape)
#define SD_DICT_OP_SynteticBase (0x14 | SD_DICT_OP_escape)
#define SD_DICT_OP_Posscript (0x15 | SD_DICT_OP_escape)
#define SD_DICT_OP_BaseFontName (0x16 | SD_DICT_OP_escape)
#define SD_DICT_OP_BaseFontBlend (0x17 | SD_DICT_OP_escape)
// reserved
#define SD_DICT_OP_ROS (0x1e | SD_DICT_OP_escape)
#define SD_DICT_OP_CIDFontVersion (0x1f | SD_DICT_OP_escape)
#define SD_DICT_OP_CIDFontRevision (0x20 | SD_DICT_OP_escape)
#define SD_DICT_OP_CIDFontType (0x21 | SD_DICT_OP_escape)
#define SD_DICT_OP_CIDCount (0x22 | SD_DICT_OP_escape)
#define SD_DICT_OP_UIDBase (0x23 | SD_DICT_OP_escape)
#define SD_DICT_OP_FDArray (0x24 | SD_DICT_OP_escape)
#define SD_DICT_OP_FDSelect (0x25 | SD_DICT_OP_escape)
#define SD_DICT_OP_FontName (0x26 | SD_DICT_OP_escape)


static SD_USHORT ntohs_p (SD_BYTE* byte) {
    SD_USHORT b0 = byte[0];
    SD_USHORT b1 = byte[1];
    // Big endian high byte low offset
    return (b0<<8) + b1;
}

static SD_USHORT ntohl_p (SD_BYTE* byte) {
    SD_ULONG b0 = byte[0];
    SD_ULONG b1 = byte[1];
    SD_ULONG b2 = byte[2];
    SD_ULONG b3 = byte[3];
    // Big endian high byte low offset
    return (b0<<24) | (b1<<16) | (b2 << 8) | b3;
}


static bool dictionaryFind (
    SBinVector<double>* operands,
    SD_BYTE* topDICT, SD_ULONG len, int op) {

    SD_BYTE*  dp = topDICT;
    
    SD_ULONG i = 0;
    SS_SINT_32 intValue = 0;
    double doubleValue = 0.0;
    operands->clear();
    // CFF
    //SD_ULONG maxOperands = 48;
    // CFF2
    SD_ULONG maxOperands = 513;
    while (i<len) {
// MAKKA
/*
         fprintf (stderr, "Next DICT operand/operator %u at %u ops:%u\n", 
                (unsigned int)*dp, i, operands->size()); 
*/

        // size 1 integer
        if (*dp >= 32 && *dp <= 246) {
            intValue = (SS_SINT_32)dp[0] - 139;
            dp++; i++;
            doubleValue = (double) intValue;
            operands->append (doubleValue);
        // size 2 integer
        } else  if (*dp >= 247 && *dp <= 250) {
            SS_SINT_32 b0 = *dp;
            dp++; i++; 
            SS_SINT_32 b1 = *dp;
            dp++; i++; 
            intValue = (b0 - 247) * 256  + b1 + 108;
            doubleValue = (double) intValue;
            operands->append (doubleValue);
        // size 2 integer
        } else if (*dp >= 251 && *dp <= 254) {
            SS_SINT_32 b0 = *dp;
            dp++; i++; 
            SS_SINT_32 b1 = *dp;
            dp++; i++; 
            intValue = -(b0 - 251) * 256  - b1 - 108;
            doubleValue = (double) intValue;
            operands->append (doubleValue);
        // size 3 integer
        } else if (*dp == 28) {
            dp++; i++;
            SS_SINT_32 b1 = *dp;
            dp++; i++;
            SS_SINT_32 b2 = *dp;
            dp++; i++;
            SS_SINT_16 v = (b1<<8)|b2;  
            intValue = v; 
            doubleValue = (double) intValue;
            operands->append (doubleValue);
        // size 5 integer 
        } else if (*dp == 29) {
            dp++; i++;
            SS_SINT_32 b1 = *dp;
            dp++; i++;
            SS_SINT_32 b2 = *dp;
            dp++; i++;
            SS_SINT_32 b3 = *dp;
            dp++; i++;
            SS_SINT_32 b4 = *dp;
            dp++; i++;
            intValue = (b1<<24)|(b2<<16)|(b3<<8)|b4;
            doubleValue = (double) intValue;
            operands->append (doubleValue);
        // variable size float
        } else if (*dp == 30) {
            dp++; i++;
            SString s;
            while (i<len) {

                SS_SINT_32 ba[2] = {
                    ((*dp) >> 4) & 0x0f, 
                    (*dp) & 0x0f
                };

                dp++; i++;
                bool finished = false;
                for (unsigned int j=0; j<2; j++) {
                    SS_SINT_32 b = ba[j];
                    if (b == 0xf) {
                        finished = true;
                        break;
                    }
                    if (b <= 9) {
                        s.append ((char) ('0' + (char)b));
                    } else if (b == 0xa) {
                        s.append ('.');
                    } else if (b == 0xb) {
                        s.append ('E');
                    } else if (b == 0xc) {
                        s.append ("E-");
                    // skip d
                    } else if (b == 0xe) {
                        s.append ('-');
                    }
                }
                if (finished) break;
            }
            s.append ((char)0);
            if (sscanf (s.array(), "%lf", &doubleValue) != 1) {
#if DEBUG
                fprintf (stderr, "Bad Float %s\n", s.array());
#endif
                operands->clear();
                return false;
            }
            operands->append (doubleValue);
        // Operator
        //} else if ((*dp >= 0 && *dp <= 21) || *dp == 24
        } else if (*dp >= 0 && *dp <= 24) {
            intValue = *dp;
            dp++; i++;
            // 2 byte operator
            if (intValue == 0x0c ) {
                intValue = (*dp) | SD_DICT_OP_escape;
                dp++; i++;
            }        
// MAKKA
#if 0
            fprintf (stderr, "OperandSize %d operator 0x%04x\n",
                    operands->size(), intValue);
#endif
//fprintf (stderr, "int=%d\n", intValue);
            if (intValue == op) {
                return true;
            }
            operands->clear();
                
        // FIXME 255 is reserved.
        } else {
#if 0
            fprintf (stderr, "Bad DICT operand/operator 0x%02x\n", 
                (unsigned int)*dp); 
#endif
            operands->clear();
            return false;
        }
        
        if (operands->size() > maxOperands) {
#if 0
            fprintf (stderr, "Too many DICT operands.");
#endif
            operands->clear();
            return false;
        }
    }
    operands->clear();
    return false;
}

/**
 * Get an element in the index table.
 */
static SD_BYTE* indexFind (bool cff2, int at, SD_BYTE* indexTable, SD_ULONG* length) {
   SD_ULONG count;
   unsigned int countSize;
   if (cff2) {
        count  = ntohl_p(indexTable);  
        countSize = 4;
   } else {
        count  = ntohs_p(indexTable);  
        countSize = 2;
   }
   *length = 0;
   if (at < 0) {
      fprintf (stderr, "Negative index at %d\n", at);
      return 0;
   }
   if (count == 0) {
       fprintf (stderr, "count error\n");
       return 0;
   }
   if (at >= (int) count) {
      fprintf (stderr, "bounds Error\n");
      return 0;
   }
   SD_BYTE osize = indexTable[countSize]; 
   if (osize > 4 || osize == 0) {
      fprintf (stderr, "offsetSize Error\n");
      return 0;
   }
   SD_BYTE* ip = indexTable + countSize +1 + at * osize;
   SD_ULONG startOffset = 0;
   int j;
   for (j=0; j<osize; j++) {
      startOffset = (startOffset << 8) + (SD_ULONG) (*ip++);
   }
   
   ip = indexTable + countSize +1 + (at+1) * osize;
   SD_ULONG endOffset = 0;
   for (j=0; j<osize; j++) {
      endOffset = (endOffset << 8) + (SD_ULONG) (*ip++);
   }
   if (startOffset > endOffset) {
      fprintf (stderr, "Start-end offset Error at %d\n", at);
      return 0;
   }
   ip = indexTable + countSize +1 + (count) * osize;
   SD_ULONG lastOffset = 0;
   for (j=0; j<osize; j++) {
      lastOffset = (lastOffset << 8) + (SD_ULONG) (*ip++);
   }
   *length = endOffset - startOffset;
   return ip + startOffset -1;
}

/**
 * Get one of the 4 index tables following the header.
 */
static SD_BYTE* getIndexTable (bool cff2, int at, SD_BYTE *header) {
/*
   fprintf (stderr, "hsize = %u\n", 
        (unsigned int) ((SS_CFF_Header*)header)->hsize);
*/
   unsigned int countSize = cff2 ? 4: 2;
   SD_BYTE* now = header + ((SS_CFF_Header*)header)->hsize;
   for (int i=0; i<at; i++) {
      SD_ULONG count = cff2 ? ntohl_p(now): ntohs_p(now);  
      if (count == 0) {
        now = now + countSize;
        continue;
      }
      SD_BYTE osize = now[countSize];
      if (osize > 4 || osize == 0) {
        //  Error
        return 0;
      }
      SD_BYTE* ind = now + countSize+1 + (count) * osize;

      SD_ULONG lastOffset = 0;
      for (int j=0; j<osize; j++) {
         lastOffset = (lastOffset << 8) + (SD_ULONG) (*ind++);
      }
      // The first element is offset 1.
      now = ind + lastOffset-1;
   } 
   return now;
} 

SFontCFF::SFontCFF (void) {
    cff = 0;
    cff2 = 0;

    firstDict = 0;
    firstDictLength = 0;

    firstCharStringsIndex = 0;
    firstCharStringsLength = 0;

    globalSubrIndex = 0;
    globalSubrLength = 0;
    
    localSubrIndex = 0;
    localSubrLength = 0;

    fontDictSelect = 0;
    
}


SFontCFF::~SFontCFF () {
}

bool
SFontCFF::initWithCFF (SD_BYTE* aCff) {
    cff = aCff;
   
    /* firstName */
    SD_BYTE* nameIndex = getIndexTable(false, 0, cff);  
    if (nameIndex == 0) {
        fprintf (stderr, "Can not find font name\n");
        return false;
    }
    SD_ULONG len;    
    SD_BYTE* ptr = indexFind (false, 0, nameIndex, &len);
    firstName = SString ((char*) ptr, (unsigned int)len);

    /* firstDict */
    SD_BYTE* topDictIndex = getIndexTable(false, 1, cff);  
    if (topDictIndex == 0) {
        return false;
    }
    if (ntohs_p(topDictIndex) != 1) {
        fprintf (stderr, "topDictionary has %d names in %*.*s\n",
            (int) ntohs_p(topDictIndex), SSARGS(firstName));
        return false;
    }
    firstDict = indexFind (false, 0, topDictIndex, &firstDictLength);

    /* firstCharStringsIndex */
    SBinVector<double> operands;
    bool found = dictionaryFind (
        &operands, firstDict, firstDictLength, 
            SD_DICT_OP_CharStrings);
    if (!found || operands.size() ==0 || operands[0] <= 0.0) {
#if 0
        fprintf (stderr, "firstDictionary has no CharStrings in %*.*s\n",
            SSARGS(firstName));
#endif
        return false;
    }
    firstCharStringsIndex= cff + (int)operands[0];
    firstCharStringsLength = ntohs_p(firstCharStringsIndex);

    /* globalSubrIndex */
    globalSubrIndex = getIndexTable(false, 3, cff);  
    globalSubrLength = ntohs_p(globalSubrIndex);

    /* localSubrIndex */
    operands.clear();
    found= dictionaryFind (&operands, firstDict, firstDictLength, 
            SD_DICT_OP_Private);

    localSubrIndex = 0;
    localSubrLength = 0;

    if (found && operands.size() == 2) {
        SD_BYTE* privateDict = 0;
        privateDict = cff + (int) operands[1];
        SD_ULONG pdictLen = (SD_ULONG) operands[0];
        operands.clear();
        found= dictionaryFind (
                &operands, privateDict, pdictLen, 
                SD_DICT_OP_Subrs);
        if (found && operands.size() == 1) {
            // relative to privateDict
            localSubrIndex = privateDict + (int) operands[0];
            localSubrLength = ntohs_p(localSubrIndex);
        }
        // dont check for CharStrings 
    }
#if 0
    fprintf (stderr, "SFontType [%*.*s] initialized\n",
        SSARGS(firstName)); 
    fprintf (stderr, 
       "firstCharStrings:%lx,%u globalSubr:%lx,%u localSubr:%lx,%u\n",
        (unsigned long)firstCharStringsIndex, firstCharStringsLength, 
        (unsigned long) globalSubrIndex, globalSubrLength, 
        (unsigned long) localSubrIndex, localSubrLength);
#endif
        
    return true;
}

// Not supported yet
bool
SFontCFF::initWithCFF2 (SD_BYTE* aCff2) {
#if HAS_CFF2 
    cff2 = aCff2;
    firstDict =  cff2 + (SD_ULONG) cff2[2];
    // 16 bit
    firstDictLength = ntohs_p(cff2+3);
#if DEBUG
    fprintf (stderr, "headerSize=%u, firstDictLength=%u\n", 
            (SD_ULONG) cff2[2], firstDictLength);
#endif
    
    firstName = SString ("CFF2");

    /* firstCharStringsIndex */
    SBinVector<double> operands;
    bool found = dictionaryFind (
        &operands, firstDict, firstDictLength, 
            SD_DICT_OP_CharStrings);
    if (!found || operands.size() ==0 || operands[0] <= 0.0) {
        fprintf (stderr, "firstDictionary has no CharStrings in %*.*s\n",
            SSARGS(firstName));
        return false;
    }
    firstCharStringsIndex= cff2 + (int)operands[0];
    firstCharStringsLength = ntohl_p(firstCharStringsIndex);

    /* globalSubrIndex */
    globalSubrIndex = firstDict + firstDictLength;  
    globalSubrLength = ntohl_p(globalSubrIndex);

    /* localSubrIndex */
    operands.clear();
    // Finding FDSelect table.
    
    found= dictionaryFind (&operands, firstDict, firstDictLength, 
            SD_DICT_OP_FDArray);

    localSubrIndex = 0;
    localSubrLength = 0;
    fontDictIndex = 0;
    fontDictLength = 0;
    fontDictSelect = 0;

    if (found && operands.size() == 1) {
        if (operands.size () != 1) return -1;
        fontDictIndex = cff2 + (int) operands[0];
        fontDictLength = ntohl_p (fontDictIndex);
        if (fontDictLength > 1) {
            found= dictionaryFind (&operands, firstDict, firstDictLength, 
                SD_DICT_OP_FDSelect);
            if (operands.size () != 1) {
                fprintf(stderr, "Can not find FDSelect dictionary.\n");
                return -1;
            }
            fontDictSelect = cff2 + (int) operands[0];
        }
        // FIXME
        if (fontDictLength != 1) {
            fprintf (stderr, "Only 1 fontDict is supported, got:%u\n", 
                fontDictLength);
            return false;
        }
    }
#if DEBUG
    fprintf (stderr, "SFontType [%*.*s] initialized\n",
        SSARGS(firstName)); 
    fprintf (stderr, 
       "firstCharStrings:%lx,%u globalSubr:%lx,%u fontDict:%lx,%u fontDictSelect=%lx\n",
        (unsigned long)firstCharStringsIndex, firstCharStringsLength, 
        (unsigned long) globalSubrIndex, globalSubrLength, 
        (unsigned long) fontDictIndex, fontDictLength,
        (unsigned long) fontDictSelect);
#endif
        
    return true;
#else /* has CFF2 */
    fprintf (stderr, "CFF2 format is not supported.\n");
    return false;
#endif
}

// hasCFFData
// TTF_GLYF*
SD_BYTE*
SFontCFF::getType2Glyph (SS_GlyphIndex glyphno, SD_ULONG* len) const {
  *len = 0;
  // TODO
  if (cff != 0) {
    SD_ULONG length;
    SD_BYTE* retP = indexFind (false, (int)glyphno, firstCharStringsIndex, &length);
    //SD_BYTE* retP = indexFind (false, (int)4, firstCharStringsIndex, &length);
#if 0
    fprintf( stderr, "Byte %0x %0x\n", 
        (unsigned int) retP[0], (unsigned int) retP[1]);
#endif
    *len = length;
    return retP;
  }
  if (cff2 != 0) {
    //fprintf (stderr, "TODO: get CFF2 Glyph %x\n", glyphno);
    SD_ULONG length;
    SD_BYTE* retP = indexFind (true, (int)glyphno, firstCharStringsIndex, &length);
    *len = length;
    return retP;
  }
  return 0;
}

bool
SFontCFF::updateLocalSubr (SS_GlyphIndex glyphno) {
    if (cff) return true;
    if (fontDictLength != 1) return false;
    SD_ULONG length;
    int index = 0;
    // FIXME Add FDSelect for multiple dicts.
    SD_BYTE* dict = indexFind (true, index, fontDictIndex, &length);
    if (dict == 0 || length == 0) return false;
    SBinVector<double> operands;
   // fprintf (stderr, "updateLocal...\n");
    bool found= dictionaryFind (&operands, dict, length, 
                SD_DICT_OP_Private);
    localSubrIndex = 0;
    localSubrLength = 0;
    if (found && operands.size() == 2) {
        SD_BYTE* privateDict = cff2 + (int) operands[1];
        SD_ULONG privateLength = (SD_ULONG) operands[0];
#if 0
    fprintf (stderr, "XXXXXX   updateLocal %u at %g\n",
         privateLength, operands[1]);
#endif

        operands.clear();
// MAKKA
        found= dictionaryFind (
                &operands, privateDict, privateLength, 
                SD_DICT_OP_Subrs);
        if (found && operands.size() == 1) {
            // relative to privateDict
            //localSubrIndex = privateDict + (int) operands[0];
            localSubrIndex = privateDict + (int) operands[0];
            localSubrLength = ntohl_p(localSubrIndex);
            if (localSubrIndex[4] > 4 || localSubrIndex[4] == 0) return false;
        }
        // dont check for CharStrings 
#if 0
        found= dictionaryFind (
                &operands, privateDict, privateLength, 
                SD_DICT_OP_blend);
        if (!found) {
            fprintf (stderr, "Number of blends not found.\n");
            return false;
        }
        numberOfBlends = operands;
        fprintf (stderr, "Number of blends: %u %g,%g,%g,%g\n", 
            numberOfBlends.size(), 
            numberOfBlends[0], 
            numberOfBlends[1], 
            numberOfBlends[2],
            numberOfBlends[3]
            );
        return false; 
#endif
    }
#if 0
    fprintf (stderr, "localSubrIndex=%lx,%u\n", 
        (unsigned long) localSubrIndex, localSubrLength); 
#endif
    return true;
}

/*
 * In OpenType CFF fonts, except that glyph data is accessed 
 * through the CharStrings INDEX of the 'CFF ' table.
*/
void
SFontCFF::drawGlyphCFF (SCanvas* aCanvas, const SS_Matrix2D& aMatrix,
      SS_GlyphIndex glyphno) {
    SD_ULONG len = 0;
    SD_BYTE* glyph = (SD_BYTE*) getType2Glyph (glyphno, &len);
    if (glyph == 0 || len == 0) return;
    if (!updateLocalSubr(glyphno)) return;
    stack.clear();
    canvas = aCanvas;
    matrix = aMatrix;
    subrCount = 0;
    subrLimit = (cff==0) ? 10 : 10;
    stackLimit = (cff==0) ? 513 : 48;
    lastX = 0.0;
    lastY = 0.0;
    charStringsBegin = true;
    commandCounter = 0;
    stemCount = 0;
    hintStart = false;
    hasGlyphWidth = false;
    glyphWidth = 0.0;
    //fprintf(stderr, "draw start\n");
    parseCharStrings (glyph, len);
#if 0
    fprintf(stderr, "draw end hasGlyphWidth: %d glyphWidth: %g\n",
        hasGlyphWidth, glyphWidth);
#endif
    canvas->closepath();
}

bool
SFontCFF::getBBOXCFF (SS_GlyphIndex glyphno,
      int* xMin, int* yMin, int* xMax, int* yMax) const
{
    SD_ULONG len = 0;
    SD_BYTE* glyph = (SD_BYTE*) getType2Glyph (glyphno, &len);
    if (glyph ==0 || len == 0) return false;
    return false;
}

static int getBias (bool isCFF2, SD_BYTE* subrIndex) {
    SD_ULONG nSubrs = isCFF2 ? ntohl_p (subrIndex) : ntohs_p (subrIndex);
    if (nSubrs < 1240) return 107;
    if (nSubrs < 33900) return 1131;
    return 32768;
}

/*
 * The starting width is not taken care of
 */
int
SFontCFF::execute (SS_SINT_32 command) {
    SD_BYTE* ptr = 0;
    SD_ULONG length = 0;
    commandCounter++;
//SGCXX
    //if (commandCounter > 50) return -1;
#if DEBUG
    if (true || (command != SS_CFF_COMMAND_callgsubr &&
        command != SS_CFF_COMMAND_callsubr && 
        command != SS_CFF_COMMAND_return) ) 
    {
    fprintf (stderr, "Command[%d] %02x args %u hintStart=%d stemCount=%d\n", 
        commandCounter, command, stack.size(), (int)hintStart, stemCount);
    }
#endif
    double vle = 0.0;
    switch (command) {
    case SS_CFF_COMMAND_CFF2_blend: // blend
        {
            if (cff) return 0;
            if (stack.size() == 0) return -1;
            double n = stack[stack.size()-1];
            stack.truncate (stack.size()-1);
            if (stack.size() < n || n < 0 || n > stack.size()) return -1;
            stack.truncate ((unsigned int) n);
        }
        return 2;
    case SS_CFF_COMMAND_CFF2_vsindex: // vsindex
        if (cff) return 0;
 //       fprintf (stderr, "SS_CFF_COMMAND_CFF2_vsindex\n");
        // we dont support it.
        return -1;
    case SS_CFF_COMMAND_rlineto: // rlineto
        {
            if (stack.size() < 2) return -1;
            for (unsigned int i=0; i+1<stack.size(); i=i+2) {
                lastX = lastX + stack[i]; 
                lastY = lastY + stack[i+1];
                canvas->lineto (matrix.toX(lastX,lastY), matrix.toY(lastX,lastY)); 
            }
        }
       return 0;
    case SS_CFF_COMMAND_hlineto: // hlineto
       {
            for (unsigned int i=0; i<stack.size(); i++) {
                if (i%2 ==0) {
                    lastX = lastX + stack[i]; 
                } else {
                    lastY = lastY + stack[i]; 
                }
                canvas->lineto (matrix.toX(lastX,lastY), 
                    matrix.toY(lastX,lastY)); 
            }
            
        }
        return 0;
    case SS_CFF_COMMAND_vlineto: // vlineto
       {
            for (unsigned int i=0; i<stack.size(); i++) {
                if (i%2 ==1) {
                    lastX = lastX + stack[i]; 
                } else {
                    lastY = lastY + stack[i]; 
                }
                canvas->lineto (matrix.toX(lastX,lastY), 
                    matrix.toY(lastX,lastY)); 
            }
            
        }
        return 0;
    case SS_CFF_COMMAND_rrcurveto: // rrcurveto
        {
            unsigned int i=0;
            for (i=0; i+5<stack.size(); i=i+6) {
                double xa = lastX + stack[i];
                double ya = lastY + stack[i+1];

                double xb = xa + stack[i+2];
                double yb = ya + stack[i+3];

                double xc = xb + stack[i+4];
                double yc = yb + stack[i+5];
                canvas->curveto (
                    matrix.toX (xa, ya), matrix.toY (xa, ya),
                    matrix.toX (xb, yb), matrix.toY (xb, yb),
                    matrix.toX (xc, yc), matrix.toY (xc, yc));
                lastX = xc;
                lastY=  yc;
            }
        }
       return 0;
    case SS_CFF_COMMAND_callsubr: // callsubr
        {
            if (stack.size() == 0) return -1;
            if (localSubrIndex == 0) return -1;
            int bias =  getBias(cff==0, localSubrIndex);
            int index = bias+(int)stack[stack.size()-1];
            ptr = indexFind (cff==0, index, localSubrIndex, &length);
#if DEBUG
            fprintf (stderr, "callsubr[%d]\n", index);
#endif
            stack.truncate (stack.size()-1);
            if (ptr == 0) {
                return -1;
            }
            if (!parseCharStrings (ptr, length)) {
                return -1;
            }
        }
        return 2;
    case SS_CFF_COMMAND_return: // return
        return 1;
    case SS_CFF_COMMAND_endchar: //  endchar
        return 0;
    case SS_CFF_COMMAND_rmoveto: // rmoveto
        {
            // FIXME cff2 does not have hasGlyphWidth
            if (charStringsBegin && stack.size() == 3) {
                hasGlyphWidth = 1;
                glyphWidth = stack[0];
                stack.remove (0);
            }
            if (stack.size() != 2) return -1;
            if (!charStringsBegin) {
                canvas->closepath();
            }
            charStringsBegin = false;
            lastX = lastX + stack[0]; 
            lastY = lastY + stack[1];
            canvas->moveto (matrix.toX(lastX, lastY), 
                matrix.toY(lastX, lastY));
        }
       return 0;
    case SS_CFF_COMMAND_hmoveto: // hmoveto
       {
            // FIXME cff2 does not have hasGlyphWidth
            if (charStringsBegin && stack.size() == 2) {
                hasGlyphWidth = 1;
                glyphWidth = stack[0];
                stack.remove (0);
            }
            if (stack.size() != 1) return -1;
            if (!charStringsBegin) {
                canvas->closepath();
            }
            charStringsBegin = false;
            lastX = lastX + stack[0]; 
            stack.truncate (stack.size()-1);
            canvas->moveto (matrix.toX(lastX, lastY), matrix.toY(lastX, lastY));
       }
       return 0;
    case SS_CFF_COMMAND_vmoveto: // vmoveto
       {
            // FIXME cff2 does not have hasGlyphWidth
            if (charStringsBegin && stack.size() == 2) {
                hasGlyphWidth = 1;
                glyphWidth = stack[0];
                stack.remove (0);
            }
            if (stack.size() != 1) return -1;
            if (!charStringsBegin) {
                canvas->closepath();
            }
            charStringsBegin = false;
            lastY = lastY + stack[0]; 
            canvas->moveto (matrix.toX(lastX, lastY), matrix.toY(lastX, lastY));
       }
       return 0;
    case SS_CFF_COMMAND_rcurveline: // rcurveline
       {
            unsigned int i=0;
            for (i=0; i+5<stack.size(); i=i+6) {
                double xa = lastX + stack[i];
                double ya = lastY + stack[i+1];
                double xb = xa + stack[i+2];
                double yb = ya + stack[i+3];
                double xc = xb + stack[i+4];
                double yc = yb + stack[i+5];

                canvas->curveto (
                    matrix.toX (xa, ya), matrix.toY (xa, ya),
                    matrix.toX (xb, yb), matrix.toY (xb, yb),
                    matrix.toX (xc, yc), matrix.toY (xc, yc));

                lastX = xc;
                lastY = yc;
            }
            if (i+1<stack.size()) {
                lastX = lastX + stack[i];
                lastY = lastY + stack[i+1];
                canvas->lineto (
                    matrix.toX(lastX, lastY), matrix.toY (lastX, lastY));
            }
       }
       return 0;
    case SS_CFF_COMMAND_rlinecurve: // rlinecurve
       {
            unsigned int i=0;
            while (i+1<stack.size() && stack.size()-i >= 8) {
                lastX = lastX + stack[i];
                lastY = lastY + stack[i+1];
                canvas->lineto (
                    matrix.toX(lastX, lastY), matrix.toY (lastX, lastY));
                i=i+2;
            }
            while (i+5<stack.size()) {
                double xa = lastX + stack[i];
                double ya = lastY + stack[i+1];
                double xb = xa + stack[i+2];
                double yb = ya + stack[i+3];
                double xc = xb + stack[i+4];
                double yc = yb + stack[i+5];

                canvas->curveto (
                    matrix.toX (xa, ya), matrix.toY (xa, ya),
                    matrix.toX (xb, yb), matrix.toY (xb, yb),
                    matrix.toX (xc, yc), matrix.toY (xc, yc));

                lastX = xc;
                lastY = yc;
                i=i+6;
            }
       }
       return 0;
    case SS_CFF_COMMAND_vvcurveto: // vvcurveto
        {
            unsigned int i=0;
            double dx = 0.0;
            if (stack.size()%4 == 1) {
                dx = stack[i++];
            }
            while (i+3<stack.size()) {
                double xa = lastX + dx;
                double ya = lastY + stack[i];
                double xb = xa + stack[i+1];
                double yb = ya + stack[i+2];
                double xc = xb + 0.0;
                double yc = yb + stack[i+3];
                canvas->curveto (
                    matrix.toX (xa, ya), matrix.toY (xa, ya),
                    matrix.toX (xb, yb), matrix.toY (xb, yb),
                    matrix.toX (xc, yc), matrix.toY (xc, yc));
                lastX = xc;
                lastY = yc;
/*
fprintf (stderr, "vvcurveto %g,%g %g,%g %g,%g\n",
     matrix.toX (xa, ya), matrix.toY (xa, ya),
     matrix.toX (xb, yb), matrix.toY (xb, yb),
     matrix.toX (xc, yc), matrix.toY (xc, yc));
*/
                dx = 0.0;
                i = i+4;
            }
        }
       return 0;
    case SS_CFF_COMMAND_hhcurveto: // hhcurveto
        {
            unsigned int i=0;
            double dy = 0.0;
            if (stack.size()%4 == 1) {
                dy = stack[i++];
            }
            while (i+3<stack.size()) {
                double xa = lastX + stack[i];
                double ya = lastY + dy;
                double xb = xa + stack[i+1];
                double yb = ya + stack[i+2];
                double xc = xb + stack[i+3];
                double yc = yb + 0.0;
                canvas->curveto (
                    matrix.toX (xa, ya), matrix.toY (xa, ya),
                    matrix.toX (xb, yb), matrix.toY (xb, yb),
                    matrix.toX (xc, yc), matrix.toY (xc, yc));

                lastX = xc;
                lastY = yc;
                dy = 0;
                i=i+4;
            }
        }
       return 0;
    case SS_CFF_COMMAND_callgsubr: // callgsubr
        {
            if (stack.size() == 0) return -1;
            if (globalSubrIndex == 0) return -1;
            int bias =  getBias(cff==0, globalSubrIndex);
            int index = bias+(int)stack[stack.size()-1];
            ptr = indexFind (cff==0, index, globalSubrIndex, &length);
#if DEBUG
            fprintf (stderr, "callsubr[%d]\n", index);
#endif
            stack.truncate (stack.size()-1);
            if (ptr == 0) {
                return -1;
            }
            if (!parseCharStrings (ptr, length)) {
                return -1;
            }
        }
        return 2;
    case SS_CFF_COMMAND_vhcurveto: // vhcurveto
        {
            unsigned int i=0;
            while (i+3<stack.size()) {
                if ((i/4) % 2 == 0) {
                    double xa = lastX + 0.0;
                    double ya = lastY + stack[i];
                    double xb = xa + stack[i+1];
                    double yb = ya + stack[i+2];
                    double dy = 0.0;
                    if (i+5 == stack.size()) {
                         dy = stack[i+4];
                    }
                    double xc = xb + stack[i+3];
                    double yc = yb + dy; 
                    canvas->curveto (
                        matrix.toX (xa, ya), matrix.toY (xa, ya),
                        matrix.toX (xb, yb), matrix.toY (xb, yb),
                        matrix.toX (xc, yc), matrix.toY (xc, yc));

                    lastX = xc;
                    lastY = yc;
                } else {
                    double xa = lastX + stack[i];
                    double ya = lastY + 0.0;
                    double xb = xa + stack[i+1];
                    double yb = ya + stack[i+2];
                    double dx = 0;
                    if (i+5 == stack.size()) {
                        dx = stack[i+4];
                    }
                    double xc = xb + dx; 
                    double yc = yb + stack[i+3];
                    canvas->curveto (
                        matrix.toX (xa, ya), matrix.toY (xa, ya),
                        matrix.toX (xb, yb), matrix.toY (xb, yb),
                        matrix.toX (xc, yc), matrix.toY (xc, yc));
                    lastX = xc;
                    lastY = yc;
                }
                i=i+4;
            }
        }
       return 0;
    case SS_CFF_COMMAND_hvcurveto: // hvcurveto
        {
            unsigned int i=0;
            while (i+3<stack.size()) {
                if ((i/4) % 2 == 1) {
                    double xa = lastX + 0.0;
                    double ya = lastY + stack[i];
                    double xb = xa + stack[i+1];
                    double yb = ya + stack[i+2];
                    double dy = 0.0;
                    if (i+5 == stack.size()) {
                        dy = stack[i+4];
                    }
                    double xc = xb + stack[i+3];
                    double yc = yb + dy;
                    canvas->curveto (
                        matrix.toX (xa, ya), matrix.toY (xa, ya),
                        matrix.toX (xb, yb), matrix.toY (xb, yb),
                        matrix.toX (xc, yc), matrix.toY (xc, yc));
                    lastX = xc;
                    lastY = yc;
                } else {
                    double xa = lastX + stack[i];
                    double ya = lastY + 0.0;
                    double xb = xa + stack[i+1];
                    double yb = ya + stack[i+2];
                    double dx = 0;
                    if (i+5 == stack.size()) {
                        dx = stack[i+4];
                    }
                    double xc = xb + dx;
                    double yc = yb + stack[i+3];
                    canvas->curveto (
                        matrix.toX (xa, ya), matrix.toY (xa, ya),
                        matrix.toX (xb, yb), matrix.toY (xb, yb),
                        matrix.toX (xc, yc), matrix.toY (xc, yc));
                    lastX = xc;
                    lastY = yc;
                }
                i=i+4;
            }
        }
       return 0;
    case SS_CFF_COMMAND_flex: // flex as rrcurveto
        {
            unsigned int i=0;
            for (i=0; i+5<stack.size(); i=i+6) {
                double xa = lastX + stack[i];
                double ya = lastY + stack[i+1];

                double xb = xa + stack[i+2];
                double yb = ya + stack[i+3];

                double xc = xb + stack[i+4];
                double yc = yb + stack[i+5];
                canvas->curveto (
                    matrix.toX (xa, ya), matrix.toY (xa, ya),
                    matrix.toX (xb, yb), matrix.toY (xb, yb),
                    matrix.toX (xc, yc), matrix.toY (xc, yc));
                lastX = xc;
                lastY=  yc;
            }
        }
       return 0;
    case SS_CFF_COMMAND_hflex: // hflex as rrcurveto
        {
            if (stack.size() < 7) return -1;
            double xa = lastX + stack[0];
            double ya = lastY;

            double xb = xa + stack[1];
            double yb = ya + stack[2];

            double xc = xb + stack[3];
            double yc = yb;
            canvas->curveto (
                    matrix.toX (xa, ya), matrix.toY (xa, ya),
                    matrix.toX (xb, yb), matrix.toY (xb, yb),
                    matrix.toX (xc, yc), matrix.toY (xc, yc));
            lastX = xc;
            lastY=  yc;

            xa = lastX + stack[4];
            ya = lastY;

            xb = xa + stack[5];
            yb = ya;

            xc = xb + stack[6];
            yc = yb;
            canvas->curveto (
                    matrix.toX (xa, ya), matrix.toY (xa, ya),
                    matrix.toX (xb, yb), matrix.toY (xb, yb),
                    matrix.toX (xc, yc), matrix.toY (xc, yc));
            lastX = xc;
            lastY=  yc;
        }
       return 0;
    case SS_CFF_COMMAND_hflex1: // hflex as rrcurveto
        {
            if (stack.size() < 9) return -1;
            double xa = lastX + stack[0];
            double ya = lastY + stack[1];

            double xb = xa + stack[2];
            double yb = ya + stack[3];

            double xc = xb + stack[4];
            double yc = yb;
            canvas->curveto (
                    matrix.toX (xa, ya), matrix.toY (xa, ya),
                    matrix.toX (xb, yb), matrix.toY (xb, yb),
                    matrix.toX (xc, yc), matrix.toY (xc, yc));
            lastX = xc;
            lastY=  yc;

            xa = lastX + stack[5];
            ya = lastY;

            xb = xa + stack[6];
            yb = ya + stack[7];

            xc = xb + stack[8];
            yc = yb;
            canvas->curveto (
                    matrix.toX (xa, ya), matrix.toY (xa, ya),
                    matrix.toX (xb, yb), matrix.toY (xb, yb),
                    matrix.toX (xc, yc), matrix.toY (xc, yc));
            lastX = xc;
            lastY=  yc;
        }
       return 0;
    case SS_CFF_COMMAND_flex1: // flex1 as rrcurveto
        {
            if (stack.size() < 11) return -1;
            double xa = lastX + stack[0];
            double ya = lastY + stack[1];
            double xs = stack[0];
            double ys = stack[1];

            double xb = xa + stack[2];
            xs += stack[2];
            double yb = ya + stack[3];
            ys += stack[3];

            double xc = xb + stack[4];
            xs += stack[4];
            double yc = yb + stack[5];
            ys += stack[5];
            canvas->curveto (
                matrix.toX (xa, ya), matrix.toY (xa, ya),
                matrix.toX (xb, yb), matrix.toY (xb, yb),
                matrix.toX (xc, yc), matrix.toY (xc, yc));
            lastX = xc;
            lastY=  yc;

            xa = lastX + stack[6];
            xs += stack[6];
            ya = lastY + stack[7];
            ys += stack[7];

            xb = xa + stack[8];
            xs += stack[8];
            yb = ya + stack[9];
            ys += stack[9];

            if (xs < 0) xs = - xs;
            if (ys < 0) ys = - ys;
            if (xs > ys) {
                xc = xb + stack[10];
                yc = yb;
            } else {
                xc = xb;
                yc = yb + stack[10];
            }
            canvas->curveto (
                matrix.toX (xa, ya), matrix.toY (xa, ya),
                matrix.toX (xb, yb), matrix.toY (xb, yb),
                matrix.toX (xc, yc), matrix.toY (xc, yc));
            lastX = xc;
            lastY=  yc;
        }
        return 0;
    case SS_CFF_COMMAND_hstem:
        // width does not affect us
        if (stack.size() % 2 == 1) {
            hasGlyphWidth = 1;
            glyphWidth = stack[0];
            stack.remove (0);
        }
        stemCount += stack.size()/2;
        return 0;
    case SS_CFF_COMMAND_vstem:
        if (stack.size() % 2 == 1) {
            hasGlyphWidth = 1;
            glyphWidth = stack[0];
            stack.remove (0);
        }
        // width does not affect us
        stemCount += stack.size()/2;
        return 0;
    case SS_CFF_COMMAND_hstemhm:
        if (stack.size() % 2 == 1) {
            hasGlyphWidth = 1;
            glyphWidth = stack[0];
            stack.remove (0);
        }
        // width does not affect us
        stemCount += stack.size()/2;
        return 0;
    case SS_CFF_COMMAND_vstemhm:
        if (stack.size() % 2 == 1) {
            hasGlyphWidth = 1;
            glyphWidth = stack[0];
            stack.remove (0);
        }
        // width does not affect us
        stemCount += stack.size()/2;
        return 0;
    case SS_CFF_COMMAND_hintmask:
        if (stack.size() % 2 == 1) {
            hasGlyphWidth = 1;
            glyphWidth = stack[0];
            stack.remove (0);
        }
        //fprintf (stderr, "hintmask adding %d\n", stack.size());
        // width does not affect us
        stemCount += stack.size()/2;
        hintStart = true;
        return 0;
        //stack.clear();
        //stack.append (0); 
        return 0;
    case SS_CFF_COMMAND_cntrmask:
        if (stack.size() % 2 == 1) {
            hasGlyphWidth = 1;
            glyphWidth = stack[0];
            stack.remove (0);
        }
        // width does not affect us
        stemCount += stack.size()/2;
        hintStart = true;
        return 0;
        //stack.clear();
        //stack.append (0); 
        return 0;
    //case SS_CFF_COMMAND_shortint:
    case SS_CFF_COMMAND_and: 
        if (stack.size() < 2) return -1;
        vle = (stack[stack.size()-2] != 0 && stack[stack.size()-1] != 0)
            ? 1 : 0;
        stack.replace (stack.size()-2, vle);
        stack.truncate (stack.size()-1);
        return 2;
    case SS_CFF_COMMAND_or: 
        if (stack.size() < 2) return -1;
        vle = (stack[stack.size()-2] != 0 || stack[stack.size()-1] != 0)
            ? 1 : 0;
        stack.replace (stack.size()-2, vle);
        stack.truncate (stack.size()-1);
        return 2;
    case SS_CFF_COMMAND_not: 
        if (stack.size() < 1) return -1;
        vle = (stack[stack.size()-1] == 0) ? 1 : 0;
        stack.replace (stack.size()-1, vle);
        return 2;
    case SS_CFF_COMMAND_abs: 
        if (stack.size() < 1) return -1;
        vle = (stack[stack.size()-1] < 0) 
            ? -stack[stack.size()-1] : stack[stack.size()-1];
        stack.replace (stack.size()-1, vle);
        return 2;
    case SS_CFF_COMMAND_add: 
        if (stack.size() < 2) return -1;
        vle = stack[stack.size()-2] + stack[stack.size()-1];
        stack.replace (stack.size()-2, vle);
        stack.truncate (stack.size()-1);
        return 2;
    case SS_CFF_COMMAND_sub: 
        if (stack.size() < 2) return -1;
        vle = stack[stack.size()-2] - stack[stack.size()-1];
        stack.replace (stack.size()-2, vle);
        stack.truncate (stack.size()-1);
        return 2;
    case SS_CFF_COMMAND_div: 
        if (stack.size() < 2) return -1;
        vle = stack[stack.size()-2] / stack[stack.size()-1];
        stack.replace (stack.size()-2, vle);
        stack.truncate (stack.size()-1);
        return 2;
    case SS_CFF_COMMAND_neg: 
        if (stack.size() < 1) return -1;
        vle = - stack[stack.size()-1];
        stack.replace (stack.size()-1, vle);
        return 2;
    case SS_CFF_COMMAND_eq: 
        if (stack.size() < 2) return -1;
        vle = (stack[stack.size()-2]==stack[stack.size()-1]) 
            ? 1 : 0;
        stack.replace (stack.size()-2, vle);
        stack.truncate (stack.size()-1);
        return 2;
    case SS_CFF_COMMAND_drop: 
        if (stack.size() < 1) return -1;
        stack.truncate (stack.size()-1);
        return 2;
    case SS_CFF_COMMAND_put: 
        if (stack.size() < 2) return -1;
        if (stack[stack.size()-1] < 0 || stack[stack.size()-1] >= 32) {
            return -1;
        }
        transientArray[(int)stack[stack.size()-1]] = stack[stack.size()-2];
        stack.truncate (stack.size()-2);
        return 2;
    case SS_CFF_COMMAND_get: 
        if (stack.size() < 1) return -1;
        if (stack[stack.size()-1] < 0 || stack[stack.size()-1] >= 32) {
            return -1;
        }
        stack.truncate (stack.size()-1);
        stack.append (transientArray[(int)stack[stack.size()-1]]);
        return 2;
    case SS_CFF_COMMAND_ifelse: 
        if (stack.size() < 4) return -1;
        if (stack[stack.size()-2]<=stack[stack.size()-1]) {
            vle = stack[stack.size()-3];
            stack.replace (stack.size()-4, vle);
        }
        stack.truncate (stack.size()-3);
        return 2;
    case SS_CFF_COMMAND_random: 
        // FIMXE
        stack.append (0.5);
        return 2;
    case SS_CFF_COMMAND_mul: 
        if (stack.size() < 2) return -1;
        vle = stack[stack.size()-2] * stack[stack.size()-1];
        stack.replace (stack.size()-2, vle);
        stack.truncate (stack.size()-1);
        return 2;
    case SS_CFF_COMMAND_sqrt: 
        if (stack.size() < 1) return -1;
        vle = sqrt(stack[stack.size()-1]);
        stack.replace (stack.size()-1, vle);
        return 2;
    case SS_CFF_COMMAND_dup: 
        if (stack.size() < 1) return -1;
        stack.append (stack[stack.size()-1]);
        return 2;
    case SS_CFF_COMMAND_exch: 
    {
        if (stack.size() < 2) return -1;
        double tmp = stack[stack.size()-1];
        stack.replace (stack.size()-1, stack[stack.size()-2]);
        stack.replace (stack.size()-2, tmp);
            
    }
        return 2;
    case SS_CFF_COMMAND_index: 
    {
        if (stack.size() < 2) return -1;
        double i = stack[stack.size()-1];
        if (i < 0) {
            stack.replace (stack.size()-1, stack[stack.size()-2]);
        } else {
            stack.replace (stack.size()-1, stack[stack.size()-(int)i-2]);
        }
    }
        return 2;
    case SS_CFF_COMMAND_roll: 
    {
        if (stack.size() < 1) return -1;
        double count = stack[stack.size()-1];
        if (count < 0) {
            // downward roll
            if (count < 64) return -1;
            for (int i=0; i<(int)count; i++) {
                double tmp = stack[0];
                for (unsigned int j=0; j<stack.size()-2; j++) {
                    vle = stack[j+1];
                    stack.replace (j, vle);
                }
                stack.replace (stack.size()-2, tmp);
            }
        } else {
            // upward roll
            if (count > 64) return -1;
            for (int i=0; i<(int)count; i++) {
                double tmp = stack[stack.size()-2];
                for (unsigned int j=0; j<stack.size()-2; j++) {
                    vle = stack[stack.size()-3-j];
                    stack.replace (stack.size()-2-j, vle);
                }
                stack.replace (0, tmp);
            }
        }
        stack.truncate (stack.size()-1);
    }
        return 2;
    default:
#if DEBUG
       fprintf (stderr, "execute %04x [%u] failed\n", (int)command, stack.size());
#endif
        break;
    }
    //fprintf (stderr, "return error\n");
    return -1;
}

// initialize stakc before calling
bool
SFontCFF::parseCharStrings(SD_BYTE* strings, SD_ULONG len) {

    SD_BYTE*  dp = strings;
    SD_ULONG i = 0;
    SS_SINT_32 intValue = 0;
    double doubleValue = 0.0;
    if (++subrCount > subrLimit+1) {
#if DEBUG
        fprintf (stderr, "Too many subroutines : %u > %u\n", 
            subrCount, subrLimit+1);
#endif
        return false;
    }
#if DEBUG
fprintf (stderr, "parseCharStrings %lx args %u len:%u\n", 
        (unsigned long) strings, stack.size(), len);
#endif

    // remove hint arguments that comes after the command.
    while (i<len) {
        if (hintStart) {
            int bytes = (stemCount + 7) / 8;
            if (bytes == 0) bytes = 1;
            while (bytes > 0 && i<len) {
                dp++; i++; 
                bytes--;
            }
            hintStart = false;
        }
#if DEBUG
fprintf (stderr, "dp[%u] = %u  %lx\n", i, *dp, (unsigned long) dp);
#endif
         // size 1 integer
        if (*dp >= 32 && *dp <= 246) {
            intValue = ((SS_SINT_32)dp[0]) - 139;
            dp++; i++;
            doubleValue = (double) intValue;
            stack.append (doubleValue);
//fprintf (stderr, "1. value: %g\n", doubleValue);
        // size 2 integer
        } else  if (*dp >= 247 && *dp <= 250) {
            SS_SINT_32 b0 = *dp;
            dp++; i++; 
            SS_SINT_32 b1 = *dp;
            dp++; i++; 
            intValue = (b0 - 247) * 256  + b1 + 108;
            doubleValue = (double) intValue;
//fprintf (stderr, "2. value: %g\n", doubleValue);
            stack.append (doubleValue);
        // size 2 integer
        } else if (*dp >= 251 && *dp <= 254) {
            SS_SINT_32 b0 = *dp;
            dp++; i++; 
            SS_SINT_32 b1 = *dp;
            dp++; i++; 
            intValue = -(b0 - 251) * 256  - b1 - 108;
            doubleValue = (double) intValue;
//fprintf (stderr, "3. value: %g\n", doubleValue);
            stack.append (doubleValue);
        // size 3 integer
        } else if (*dp == 28) {
            dp++; i++;
            SS_SINT_32 b1 = *dp;
            dp++; i++;
            SS_SINT_32 b2 = *dp;
            dp++; i++;
            SS_SINT_16 v = (b1<<8)|b2;  
            intValue = v; 
            doubleValue = (double) intValue;
//fprintf (stderr, "4. value: %g\n", doubleValue);
            stack.append (doubleValue);
        // size 5 integer 
        } else if (*dp == 255) {
            dp++; i++;
            SS_SINT_32 b1 = *dp;
            dp++; i++;
            SS_SINT_32 b2 = *dp;
            dp++; i++;
            SS_SINT_32 b3 = *dp;
            dp++; i++;
            SS_SINT_32 b4 = *dp;
            dp++; i++;
            intValue = (b1<<24)|(b2<<16)|(b3<<8)|b4;
            doubleValue = (double) intValue / (double) (1 << 16);
//fprintf (stderr, "5. value: %g\n", doubleValue);
            // fixed
            stack.append (doubleValue);
        // Operator
        } else if (*dp == SS_CFF_COMMAND_shortint) {
            dp++; i++;
            SS_SINT_16 b1 = *dp;
            b1 = b1 << 8;
            dp++; i++;
            b1 = b1 | *dp;
            dp++; i++;
            intValue = b1; 
            doubleValue = (double) intValue;
            stack.append (doubleValue);
//fprintf (stderr, "6. value: %g\n", doubleValue);
        } else if ((*dp >= 0 && *dp <= 27) 
            || *dp == 29 || *dp == 30 || *dp == 31) {

            intValue = *dp;
            dp++; i++;
            // 2 byte operator
            if (intValue == 0x0c ) {
                intValue = (*dp) | SD_DICT_OP_escape;
                dp++; i++;
            }       
            int res = execute (intValue);
            if (res == 1) {
                break; // return
                //return true;
            }
            // callsubr callgbsubr does not clear stack
            if (res != 2) { 
                stack.clear();
            }
            if (res == -1) {
                return false;
            }
        } else {
#if DEBUG
            fprintf(stderr, "parseCharStrings error %u\n", (unsigned int) *dp);
#endif
            dp++; i++;
        }
        if (stack.size() > stackLimit) {
#if DEBUG
            fprintf (stderr, "Too many DICT operands.");
#endif
            return false;
        }
        //fprintf (stderr, "i=%u\n", i);
    }
    if (subrCount > 0) subrCount--;
#if DEBUG
    fprintf (stderr, "end args at %i of %u, stack=%u\n", 
        i, len, stack.size());
#endif
    return true;
}
