/**************************************************************************

xinfo

Copyright 2014-2018 Samsung Electronics co., Ltd. All Rights Reserved.

Contact: SooChan Lim <sc1.lim@samsung.com>
Contact: Gwan-gyeong Mun <kk.moon@samsung.com>

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

#ifndef _XINFO_
#define _XINFO_ 1

#define TRUE 1
#define FALSE 0

enum XINFO_VAL
{
	XINFO_TOPWINS,
	XINFO_TOPVWINS,
	XINFO_PING,
	XINFO_XWD_TOPVWINS,
	XINFO_XWD_WIN,
	XINFO_TOPVWINS_PROPS
};

typedef struct  _Xinfo {
	int xinfo_val;
	FILE* output_fd;
	char* xinfovalname;
	char* filename;
	char* pathname;
	unsigned int  win; /* window id from command line */
} Xinfo, *XinfoPtr;

#endif



