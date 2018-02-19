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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <xinfo.h>
#include <time.h>
#include <string.h>

void display_topwins(XinfoPtr pXinfo);

static long parse_long (char *s)
{
    const char *fmt = "%lu";
    long retval = 0L;
    int thesign = 1;

    if (s && s[0]) {
	if (s[0] == '-') s++, thesign = -1;
	if (s[0] == '0') s++, fmt = "%lo";
	if (s[0] == 'x' || s[0] == 'X') s++, fmt = "%lx";
	(void) sscanf (s, fmt, &retval);
    }
    return (thesign * retval);
}

static void free_xinfo(XinfoPtr pXinfo)
{
	FILE *fd;
	if(pXinfo)
	{
		fd = pXinfo->output_fd;
		if(fd != stderr)
			fclose(fd);
		free(pXinfo);
	}
}

static void
make_directory(XinfoPtr pXinfo, char *args)
{
	char tmp1[255], tmp2[255], tmp3[255];
	time_t timer;
	struct tm *t, *buf;
	pid_t pid;
	char *argument[4];

	timer = time(NULL);
	if (!timer)
	{
		fprintf(stderr,"fail to get time\n");
		exit(1);
	}

	buf = calloc (1, sizeof (struct tm));
	t = localtime_r(&timer, buf);
	if (!t)
	{
		fprintf(stderr,"fail to get local time\n");
		exit(1);
	}

	pXinfo->pathname = (char*) calloc(1, 255*sizeof(char));
	if (!pXinfo->pathname)
	{
	    free(buf);
	    fprintf(stderr, "fail to alloc pathname memory\n");
	    exit(1);
	}

	if(pXinfo->xinfo_val == XINFO_XWD_TOPVWINS)
	{
		if(args)
			snprintf(tmp1, sizeof(tmp1), "%s", args);
		else
			snprintf(tmp1, sizeof(tmp1), "./");
	}
	else if(pXinfo->xinfo_val == XINFO_XWD_WIN)
	{
		if(args)
			snprintf(tmp1, sizeof(tmp1), "./");
	}

	/* make the folder for the result of xwd files */
	snprintf(tmp2, 255, "%s_%02d%02d-%02d%02d%02d", pXinfo->xinfovalname, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
	snprintf(pXinfo->pathname, 255, "%s/%s",tmp1, tmp2);
	snprintf(tmp3, 255, "%s", pXinfo->pathname);
	argument[0] = strdup ("/bin/mkdir");
	argument[1] = strdup ("-p");
	argument[2] = tmp3;
	argument[3] = NULL;
	switch(pid = fork())
	{
		case 0:
			if (execv("/bin/mkdir", argument) == -1)
			{
				free (argument [0]);
				free (argument [1]);
				free (buf);
				fprintf(stderr, "error execv: mkdir\n");
				exit(1);
			}
			break;
		default:
			break;
	}
	free (argument [0]);
	free (argument [1]);
	free (buf);
}

static void
init_xinfo(XinfoPtr pXinfo, int xinfo_val, char* args)
{
	char full_pathfile[255];
	time_t timer;
	struct tm *t, *buf;
	int is_xwd = FALSE;
	int is_xwd_win = FALSE;

	timer = time(NULL);
	if (!timer)
	{
		fprintf(stderr,"fail to get time\n");
		exit(1);
	}

	buf = calloc (1, sizeof(struct tm));
	t = localtime_r(&timer, buf);
	if (!t)
	{
		fprintf(stderr,"fail to get local time\n");
		free(buf);
		exit(1);
	}

	pXinfo->xinfo_val = xinfo_val;

	pXinfo->xinfovalname = (char*) calloc(1, 255*sizeof(char));
	if (!pXinfo->xinfovalname)
	{
		fprintf(stderr,"fail to alloc pXinfo->xinfovalname\n");
		free(buf);
		exit(1);
	}
	switch(pXinfo->xinfo_val)
	{
		case XINFO_TOPWINS:
				snprintf(pXinfo->xinfovalname, 255, "%s", "topwins");
				break;
		case XINFO_TOPVWINS:
				snprintf(pXinfo->xinfovalname, 255, "%s", "topvwins");
				break;
		case XINFO_PING:
				snprintf(pXinfo->xinfovalname, 255, "%s", "ping");
				break;
		case XINFO_XWD_TOPVWINS:
				snprintf(pXinfo->xinfovalname, 255, "%s", "xwd_topvwins");
				is_xwd = TRUE;
				break;
		case XINFO_XWD_WIN:
				snprintf(pXinfo->xinfovalname, 255, "%s", "xwd_win");
				is_xwd = TRUE;
				is_xwd_win = TRUE;
				break;
		case XINFO_TOPVWINS_PROPS:
				snprintf(pXinfo->xinfovalname, 255, "%s", "topvwins_props");
				break;
		default:
				break;
	}

	if(is_xwd)
	{
		make_directory(pXinfo, args);
		pXinfo->output_fd = stderr;
		pXinfo->filename = NULL;
		if(is_xwd_win)
			pXinfo->win = parse_long(args);
		goto out;
	}

	/* set the path and output_fd */
	if(!args)
	{
		pXinfo->output_fd = stderr;
		pXinfo->filename = NULL;
		pXinfo->pathname = NULL;
		goto out;
	}
	else
	{
		pXinfo->pathname = (char*) calloc(1, 255*sizeof(char));
		pXinfo->filename = (char*) calloc(1, 255*sizeof(char));

		if ( (!pXinfo->pathname) || (!pXinfo->filename) )
		{
			fprintf(stderr, "fail to alloc memory to pathname or filename\n");
			free_xinfo(pXinfo);
			free(buf);
			exit(1);
		}

		snprintf(pXinfo->pathname, 255, "%s", args);
		snprintf(pXinfo->filename, 255, "%s_%02d%02d-%02d%02d%02d.log", pXinfo->xinfovalname, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
		snprintf(full_pathfile, 255, "%s/%s", pXinfo->pathname, pXinfo->filename);
		pXinfo->output_fd = fopen(full_pathfile, "a+");
		if(pXinfo->output_fd == NULL)
		{
			fprintf(stderr, "Error - can open file \n");
			free_xinfo(pXinfo);
			free(buf);
			exit(1);
		}
	}
out:
	free(buf);
	return;
}



static void
usage()
{
	fprintf(stderr,"usage : xinfo [-options] [output_path] \n\n");
	fprintf(stderr,"where options include: \n");
	fprintf(stderr,"    -topwins [output_path]      : print all top level windows. (default output_path : stdout) \n");
	fprintf(stderr,"    -topvwins [output_path]     : print all top level visible windows (default output_path : stdout) \n");
	fprintf(stderr,"    -ping or -p [output_path]   : ping test for all top level windows (default output_path : stdout) \n");
	fprintf(stderr,"    -xwd_topvwins [output_path] : dump topvwins (default output_path : current working directory) \n");
	fprintf(stderr,"    -xwd_win [window id] : dump window id (a xwd file stores in current working directory) \n");
	fprintf(stderr,"    -topvwins_props : print all top level visible windows's property \n");
	fprintf(stderr,"\n\n");
	exit(1);
}

int main(int argc, char **argv)
{
	XinfoPtr pXinfo;

	int xinfo_value = -1;
	if(argv[1])
	{
		if( !strcmp(argv[1],"-h") || !strcmp(argv[1],"-help"))
				usage();
		else if(!strcmp(argv[1], "-topwins"))
		{
			xinfo_value = XINFO_TOPWINS;
		}
		else if(!strcmp(argv[1], "-topvwins"))
		{
			xinfo_value = XINFO_TOPVWINS;
		}
		else if(!strcmp(argv[1], "-ping") || !strcmp(argv[1], "-p") )
		{
			xinfo_value = XINFO_PING;
		}
		else if(!strcmp(argv[1], "-xwd_topvwins"))
		{
			xinfo_value = XINFO_XWD_TOPVWINS;
		}
		else if(!strcmp(argv[1], "-xwd_win"))
		{
			xinfo_value = XINFO_XWD_WIN;
			if(!argv[2])
			{
				fprintf(stderr, "Error : no window id \n");
				usage();
				exit(1);
			}
		}
		else if(!strcmp(argv[1], "-topvwins_props"))
		{
			xinfo_value = XINFO_TOPVWINS_PROPS;
		}

		else
			usage();

	  	pXinfo = (XinfoPtr) malloc(sizeof(Xinfo));
		if(!pXinfo)
		{
			fprintf(stderr, " alloc error \n");
			exit(1);
		}

		if(argv[2])
			init_xinfo(pXinfo, xinfo_value, argv[2]);
		else
			init_xinfo(pXinfo, xinfo_value, NULL);

		if(xinfo_value == XINFO_TOPWINS || xinfo_value == XINFO_TOPVWINS || xinfo_value == XINFO_PING ||
			xinfo_value == XINFO_XWD_TOPVWINS || xinfo_value == XINFO_XWD_WIN || xinfo_value == XINFO_TOPVWINS_PROPS)
		{
			display_topwins(pXinfo);
		}
		else
		{
			fprintf(stderr, "error unsupported xinfo vaule\n");
			goto error;
		}


	}
	else
		usage();

error:
	free_xinfo(pXinfo);

	 return 0;
}



