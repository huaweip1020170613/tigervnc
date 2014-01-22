/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
 * Copyright 2009-2014 Pierre Ossman for Cendio AB
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <rdr/InStream.h>
#include <rdr/OutStream.h>
#include <rfb/Exception.h>
#include <rfb/PixelFormat.h>
#include <rfb/util.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

using namespace rfb;

PixelFormat::PixelFormat(int b, int d, bool e, bool t,
                         int rm, int gm, int bm, int rs, int gs, int bs)
  : bpp(b), depth(d), trueColour(t), bigEndian(e),
    redMax(rm), greenMax(gm), blueMax(bm),
    redShift(rs), greenShift(gs), blueShift(bs)
{
  assert(isSane());

  updateState();
}

PixelFormat::PixelFormat()
  : bpp(8), depth(8), trueColour(true), bigEndian(false),
    redMax(7), greenMax(7), blueMax(3),
    redShift(0), greenShift(3), blueShift(6)
{
  updateState();
}

bool PixelFormat::equal(const PixelFormat& other) const
{
  return (bpp == other.bpp &&
          depth == other.depth &&
          (bigEndian == other.bigEndian || bpp == 8) &&
          trueColour == other.trueColour &&
          (!trueColour || (redMax == other.redMax &&
                           greenMax == other.greenMax &&
                           blueMax == other.blueMax &&
                           redShift == other.redShift &&
                           greenShift == other.greenShift &&
                           blueShift == other.blueShift)));
}

void PixelFormat::read(rdr::InStream* is)
{
  bpp = is->readU8();
  depth = is->readU8();
  bigEndian = is->readU8();
  trueColour = is->readU8();
  redMax = is->readU16();
  greenMax = is->readU16();
  blueMax = is->readU16();
  redShift = is->readU8();
  greenShift = is->readU8();
  blueShift = is->readU8();
  is->skip(3);

  if (!isSane())
    throw Exception("invalid pixel format");

  updateState();
}

void PixelFormat::write(rdr::OutStream* os) const
{
  os->writeU8(bpp);
  os->writeU8(depth);
  os->writeU8(bigEndian);
  os->writeU8(trueColour);
  os->writeU16(redMax);
  os->writeU16(greenMax);
  os->writeU16(blueMax);
  os->writeU8(redShift);
  os->writeU8(greenShift);
  os->writeU8(blueShift);
  os->pad(3);
}


bool PixelFormat::is888(void) const
{
  if (!trueColour)
    return false;
  if (bpp != 32)
    return false;
  if (depth != 24)
    return false;
  if (redMax != 255)
    return false;
  if (greenMax != 255)
    return false;
  if (blueMax != 255)
    return false;

  return true;
}


bool PixelFormat::isBigEndian(void) const
{
  return bigEndian;
}


bool PixelFormat::isLittleEndian(void) const
{
  return ! bigEndian;
}


void PixelFormat::bufferFromRGB(rdr::U8 *dst, const rdr::U8* src,
                                int pixels, ColourMap* cm) const
{
  if (is888()) {
    // Optimised common case
    rdr::U8 *r, *g, *b, *x;

    if (bigEndian) {
      r = dst + (24 - redShift)/8;
      g = dst + (24 - greenShift)/8;
      b = dst + (24 - blueShift)/8;
      x = dst + (24 - (48 - redShift - greenShift - blueShift))/8;
    } else {
      r = dst + redShift/8;
      g = dst + greenShift/8;
      b = dst + blueShift/8;
      x = dst + (48 - redShift - greenShift - blueShift)/8;
    }

    while (pixels--) {
      *r = *(src++);
      *g = *(src++);
      *b = *(src++);
      *x = 0;
      r += 4;
      g += 4;
      b += 4;
      x += 4;
    }
  } else {
    // Generic code
    Pixel p;
    rdr::U8 r, g, b;

    while (pixels--) {
      r = *(src++);
      g = *(src++);
      b = *(src++);

      p = pixelFromRGB(r, g, b, cm);

      bufferFromPixel(dst, p);
      dst += bpp/8;
    }
  }
}

void PixelFormat::bufferFromRGB(rdr::U8 *dst, const rdr::U8* src,
                                int w, int stride, int h, ColourMap* cm) const
{
  if (is888()) {
    // Optimised common case
    rdr::U8 *r, *g, *b, *x;

    if (bigEndian) {
      r = dst + (24 - redShift)/8;
      g = dst + (24 - greenShift)/8;
      b = dst + (24 - blueShift)/8;
      x = dst + (24 - (48 - redShift - greenShift - blueShift))/8;
    } else {
      r = dst + redShift/8;
      g = dst + greenShift/8;
      b = dst + blueShift/8;
      x = dst + (48 - redShift - greenShift - blueShift)/8;
    }

    int dstPad = (stride - w) * 4;
    while (h--) {
      int w_ = w;
      while (w_--) {
        *r = *(src++);
        *g = *(src++);
        *b = *(src++);
        *x = 0;
        r += 4;
        g += 4;
        b += 4;
        x += 4;
      }
      r += dstPad;
      g += dstPad;
      b += dstPad;
      x += dstPad;
    }
  } else {
    // Generic code
    int dstPad = (stride - w) * 4;
    while (h--) {
      int w_ = w;
      while (w_--) {
        Pixel p;
        rdr::U8 r, g, b;

        r = *(src++);
        g = *(src++);
        b = *(src++);

        p = pixelFromRGB(r, g, b, cm);

        bufferFromPixel(dst, p);
        dst += bpp/8;
      }
      dst += dstPad;
    }
  }
}


void PixelFormat::rgbFromPixel(Pixel p, ColourMap* cm, Colour* rgb) const
{
  rdr::U16 r, g, b;

  rgbFromPixel(p, cm, &r, &g, &b);

  rgb->r = r;
  rgb->g = g;
  rgb->b = b;
}


void PixelFormat::rgbFromBuffer(rdr::U8* dst, const rdr::U8* src, int pixels, ColourMap* cm) const
{
  if (is888()) {
    // Optimised common case
    const rdr::U8 *r, *g, *b;

    if (bigEndian) {
      r = src + (24 - redShift)/8;
      g = src + (24 - greenShift)/8;
      b = src + (24 - blueShift)/8;
    } else {
      r = src + redShift/8;
      g = src + greenShift/8;
      b = src + blueShift/8;
    }

    while (pixels--) {
      *(dst++) = *r;
      *(dst++) = *g;
      *(dst++) = *b;
      r += 4;
      g += 4;
      b += 4;
    }
  } else {
    // Generic code
    Pixel p;
    rdr::U8 r, g, b;

    while (pixels--) {
      p = pixelFromBuffer(src);
      src += bpp/8;

      rgbFromPixel(p, cm, &r, &g, &b);
      *(dst++) = r;
      *(dst++) = g;
      *(dst++) = b;
    }
  }
}


void PixelFormat::rgbFromBuffer(rdr::U8* dst, const rdr::U8* src,
                                int w, int stride, int h, ColourMap* cm) const
{
  if (is888()) {
    // Optimised common case
    const rdr::U8 *r, *g, *b;

    if (bigEndian) {
      r = src + (24 - redShift)/8;
      g = src + (24 - greenShift)/8;
      b = src + (24 - blueShift)/8;
    } else {
      r = src + redShift/8;
      g = src + greenShift/8;
      b = src + blueShift/8;
    }

    int srcPad = (stride - w) * 4;
    while (h--) {
      int w_ = w;
      while (w_--) {
        *(dst++) = *r;
        *(dst++) = *g;
        *(dst++) = *b;
        r += 4;
        g += 4;
        b += 4;
      }
      r += srcPad;
      g += srcPad;
      b += srcPad;
    }
  } else {
    // Generic code
    int srcPad = (stride - w) * bpp/8;
    while (h--) {
      int w_ = w;
      while (w_--) {
        Pixel p;
        rdr::U8 r, g, b;

        p = pixelFromBuffer(src);

        rgbFromPixel(p, cm, &r, &g, &b);

        *(dst++) = r;
        *(dst++) = g;
        *(dst++) = b;
        src += bpp/8;
      }
      src += srcPad;
    }
  }
}


void PixelFormat::print(char* str, int len) const
{
  // Unfortunately snprintf is not widely available so we build the string up
  // using strncat - not pretty, but should be safe against buffer overruns.

  char num[20];
  if (len < 1) return;
  str[0] = 0;
  strncat(str, "depth ", len-1-strlen(str));
  sprintf(num,"%d",depth);
  strncat(str, num, len-1-strlen(str));
  strncat(str, " (", len-1-strlen(str));
  sprintf(num,"%d",bpp);
  strncat(str, num, len-1-strlen(str));
  strncat(str, "bpp)", len-1-strlen(str));
  if (bpp != 8) {
    if (bigEndian)
      strncat(str, " big-endian", len-1-strlen(str));
    else
      strncat(str, " little-endian", len-1-strlen(str));
  }

  if (!trueColour) {
    strncat(str, " color-map", len-1-strlen(str));
    return;
  }

  if (blueShift == 0 && greenShift > blueShift && redShift > greenShift &&
      blueMax  == (1 << greenShift) - 1 &&
      greenMax == (1 << (redShift-greenShift)) - 1 &&
      redMax   == (1 << (depth-redShift)) - 1)
  {
    strncat(str, " rgb", len-1-strlen(str));
    sprintf(num,"%d",depth-redShift);
    strncat(str, num, len-1-strlen(str));
    sprintf(num,"%d",redShift-greenShift);
    strncat(str, num, len-1-strlen(str));
    sprintf(num,"%d",greenShift);
    strncat(str, num, len-1-strlen(str));
    return;
  }

  if (redShift == 0 && greenShift > redShift && blueShift > greenShift &&
      redMax   == (1 << greenShift) - 1 &&
      greenMax == (1 << (blueShift-greenShift)) - 1 &&
      blueMax  == (1 << (depth-blueShift)) - 1)
  {
    strncat(str, " bgr", len-1-strlen(str));
    sprintf(num,"%d",depth-blueShift);
    strncat(str, num, len-1-strlen(str));
    sprintf(num,"%d",blueShift-greenShift);
    strncat(str, num, len-1-strlen(str));
    sprintf(num,"%d",greenShift);
    strncat(str, num, len-1-strlen(str));
    return;
  }

  strncat(str, " rgb max ", len-1-strlen(str));
  sprintf(num,"%d,",redMax);
  strncat(str, num, len-1-strlen(str));
  sprintf(num,"%d,",greenMax);
  strncat(str, num, len-1-strlen(str));
  sprintf(num,"%d",blueMax);
  strncat(str, num, len-1-strlen(str));
  strncat(str, " shift ", len-1-strlen(str));
  sprintf(num,"%d,",redShift);
  strncat(str, num, len-1-strlen(str));
  sprintf(num,"%d,",greenShift);
  strncat(str, num, len-1-strlen(str));
  sprintf(num,"%d",blueShift);
  strncat(str, num, len-1-strlen(str));
}


bool PixelFormat::parse(const char* str)
{
  char rgbbgr[4];
  int bits1, bits2, bits3;
  if (sscanf(str, "%3s%1d%1d%1d", rgbbgr, &bits1, &bits2, &bits3) < 4)
    return false;
  
  depth = bits1 + bits2 + bits3;
  bpp = depth <= 8 ? 8 : ((depth <= 16) ? 16 : 32);
  trueColour = true;
  rdr::U32 endianTest = 1;
  bigEndian = (*(rdr::U8*)&endianTest == 0);

  greenShift = bits3;
  greenMax = (1 << bits2) - 1;

  if (strcasecmp(rgbbgr, "bgr") == 0) {
    redShift = 0;
    redMax = (1 << bits3) - 1;
    blueShift = bits3 + bits2;
    blueMax = (1 << bits1) - 1;
  } else if (strcasecmp(rgbbgr, "rgb") == 0) {
    blueShift = 0;
    blueMax = (1 << bits3) - 1;
    redShift = bits3 + bits2;
    redMax = (1 << bits1) - 1;
  } else {
    return false;
  }

  assert(isSane());

  updateState();

  return true;
}


static int bits(rdr::U16 value)
{
  int bits;

  bits = 16;

  if (!(value & 0xff00)) {
    bits -= 8;
    value <<= 8;
  }
  if (!(value & 0xf000)) {
    bits -= 4;
    value <<= 4;
  }
  if (!(value & 0xc000)) {
    bits -= 2;
    value <<= 2;
  }
  if (!(value & 0x8000)) {
    bits -= 1;
    value <<= 1;
  }

  return bits;
}

void PixelFormat::updateState(void)
{
  int endianTest = 1;

  redBits = bits(redMax);
  greenBits = bits(greenMax);
  blueBits = bits(blueMax);

  maxBits = redBits;
  if (greenBits > maxBits)
    maxBits = greenBits;
  if (blueBits > maxBits)
    maxBits = blueBits;

  minBits = redBits;
  if (greenBits < minBits)
    minBits = greenBits;
  if (blueBits < minBits)
    minBits = blueBits;

  if (((*(char*)&endianTest) == 0) != bigEndian)
    endianMismatch = true;
  else
    endianMismatch = false;
}

bool PixelFormat::isSane(void)
{
  int totalBits;

  if ((bpp != 8) && (bpp != 16) && (bpp != 32))
    return false;
  if (depth > bpp)
    return false;

  if (!trueColour && (depth != 8))
    return false;

  if (trueColour) {
    if ((redMax & (redMax + 1)) != 0)
      return false;
    if ((greenMax & (greenMax + 1)) != 0)
      return false;
    if ((blueMax & (blueMax + 1)) != 0)
      return false;

    /*
     * We don't allow individual channels > 8 bits in order to keep our
     * conversions simple.
     */
    if (redMax >= (1 << 8))
      return false;
    if (greenMax >= (1 << 8))
      return false;
    if (blueMax >= (1 << 8))
      return false;

    totalBits = bits(redMax) + bits(greenMax) + bits(blueMax);
    if (totalBits > bpp)
      return false;

    if (((redMax << redShift) & (greenMax << greenShift)) != 0)
      return false;
    if (((redMax << redShift) & (blueMax << blueShift)) != 0)
      return false;
    if (((greenMax << greenShift) & (blueMax << blueShift)) != 0)
      return false;
  }

  return true;
}
