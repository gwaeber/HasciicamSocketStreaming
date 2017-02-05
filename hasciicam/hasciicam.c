/*  HasciiCam 1.3
 *
 *  (c) 2000-2014 Denis Roio <jaromil@dyne.org>
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Public License as published
 * by the Free Software Foundation; either version 3 of the License,
 * or (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * Please refer to the GNU Public License for more details.
 *
 * You should have received a copy of the GNU Public License along with
 * this source code; if not, write to:
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Code snippets included by:
 * Josto Chinelli
 * Alessandro Preite Martinez
 * Diego Torres aka Rapid
 * Matteo Scassa aka Blended
 * Hellekin O. Wolf
 * Dan Stowell
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <signal.h>

#include <linux/types.h>
#include <linux/videodev2.h>

#include <aalib.h>


// *****************************************************************************
//   HEIA-FR ,  Embedded Systems 3 ,  TP04 - Hasciicam ,  Vallelian & Waeber
// *****************************************************************************
#define FIFO_PATH "/tmp/hasciicamFifo"
#define FIFO_BUF_SIZE 3204               // video 352x288  / ASCII 88x36

// *****************************************************************************



/* hasciicam modes */
#define LIVE 0
#define HTML 1
#define TEXT 2

/* commandline stuff */

char *version =
    "\n%s %s - (h)ascii 4 the masses! - http://ascii.dyne.org\n"
    "(c)2000-2014 by Jaromil @ RASTASOFT\n\n";

char *help =
/* "\x1B" "c" <--- SCREEN CLEANING ESCAPE CODE
   why here? just a reminder for a shamanic secret told by bernie@codewiz.org */
"Usage: hasciicam [options] [rendering options] [aalib options]\n"
"options:\n"
" -h --help         this help\n"
" -H --aahelp       aalib complete help\n"
" -v --version      version information\n"
" -q --quiet        be quiet\n"
" -m --mode         mode: live|html|text      - default live\n"
" -d --device       video grabbing device     - default /dev/video\n"
" -i --input        input channel number      - default 1\n"
" -s --size         ascii image size WxH      - webcam's smallest default\n"
" -o --aafile       dumped file               - default hasciicam.[txt|html]\n"
" -D --daemon       run in background         - default foregrond\n"
" -U --uid          setuid (int)              - default current\n"
" -G --gid          setgid (int)              - default current\n"
"rendering options:\n"
" -S --font-size    html font size (1-4)      - default 1\n"
" -a --font-face    html font to use          - default courier\n"
" -r --refresh      refresh delay             - default 2\n"
" -b --aabright     ascii brightness          - default 60\n"
" -c --aacontrast   ascii contrast            - default 4\n"
" -g --aagamma      ascii gamma               - default 3\n"
" -I --invert       invert colors             - default off\n"
" -B --background   background color (hex)    - default 000000\n"
" -F --foreground   foreground color (hex)    - default 00FF00\n";

const struct option long_options[] = {
  {"help", no_argument, NULL, 'h'},
  {"aahelp", no_argument, NULL, 'H'},
  {"version", no_argument, NULL, 'v'},
  {"quiet", no_argument, NULL, 'q'},
  {"mode", required_argument, NULL, 'm'},
  {"device", required_argument, NULL, 'd'},
  {"input", required_argument, NULL, 'i'},
  {"size", required_argument, NULL, 's'},
  {"aafile", required_argument, NULL, 'o'},
  {"daemon", no_argument, NULL, 'D'},
  {"font-size", required_argument, NULL, 'S'},
  {"font-face", required_argument, NULL, 'a'},
  {"refresh", required_argument, NULL, 'r'},
  {"aabright", required_argument, NULL, 'b'},
  {"aacontrast", required_argument, NULL, 'c'},
  {"aagamma", required_argument, NULL, 'g'},
  {"invert", no_argument, NULL, 'I'},
  {"background", required_argument, NULL, 'B'},
  {"foreground", required_argument, NULL, 'F'},
  {"uid", required_argument, NULL, 'U'},
  {"gid", required_argument, NULL, 'G'},
  {0, 0, 0, 0}
};

char *short_options = "hHvqm:d:i:s:f:DS:a:r:o:b:c:g:IB:F:O:Q:U:G:";

/* default configuration */
int quiet = 0;
int mode = 0;
int inputch = 0;
int daemon_mode = 0;
int invert = 0;

struct geometry {
  int w, h, size;
  int bright, contrast, gamma; };
struct geometry aa_geo;
struct geometry vid_geo;
/* if width&height have been manually changed */
int whchanged = 0;

char device[256];
int have_tuner = 0;

int refresh = 2;
int fontsize = 1;
int linespace = 5;
char background[64];
char foreground[64];
char fontface[256];

int user_w = 0;
int user_h = 0;

int uid = -1;
int gid = -1;

/* buffers */
unsigned char *image = NULL; /* mmapped */
char aafile[256];
char aatmpfile[256];


/* declare the sighandler */
void quitproc (int Sig);
volatile sig_atomic_t userbreak;

/* ascii context & html formatting stuff*/
aa_context *ascii_context;
struct aa_renderparams *ascii_rndparms;
struct aa_hardware_params ascii_hwparms;
struct aa_savedata ascii_save;

char hascii_header[1024];

char *html_escapes[] =
  { "<", "&lt;", ">", "&gt;", "&", "&amp;", NULL };

struct aa_format hascii_format = {
  79, 25,
  0, 0,
  0,
  AA_NORMAL_MASK | AA_BOLD_MASK | AA_BOLDFONT_MASK,
  NULL,
  "Pure html",
  ".html",
  hascii_header,
  "</PRE>\n</FONT>\n</BODY>\n</HTML>\n",
  "\n",
  /*The order is:normal, dim, bold, boldfont, reverse, special */
  {"%s", "%s", "%s", "%s", "%s"},
  {"", "", "<B>", "", "<B>"},
  {"", "", "</B>", "", "</B>"},
  html_escapes
};

/* v4l2 (thanks Dan)*/
int buftype = V4L2_BUF_TYPE_VIDEO_CAPTURE;
struct v4l2_capability capability;
struct v4l2_input input;
struct v4l2_standard standard;
struct v4l2_format format;
struct v4l2_requestbuffers reqbuf;
struct v4l2_buffer buffer;
struct {
    void *start;
    size_t length;
} *buffers;


int fd = -1;
/* greyscale image is sampled from Y luminance component */
unsigned char *grey;
int YtoRGB[256];
int xstep=2, ystep=4;
int xbytestep;
int ybytestep;
int renderhop=2, framenum=0; // renderhop is how many frames to guzzle before rendering
int gw, gh; // number of cols/rows in grey intermediate representation
int vw, vh; // video w and h
int aw, ah; // ascii w and h
unsigned int greysize;
int vbytesperline;














// *****************************************************************************
//   HEIA-FR ,  Embedded Systems 3 ,  TP04 - Hasciicam ,  Vallelian & Waeber
// *****************************************************************************
int   fifo_fd;               // FIFO file descriptor
char *fifo_buf;              // Read text file buffer
FILE *aafile_fd;             // Hasciicam text file
int   nbBytes;               // Number of bytes read/write from/to file/FIFO



void YUV422_to_grey(unsigned char *src, unsigned char *dst, int w, int h) {
    unsigned char *writehead, *readhead;
    int x,y;
    writehead = dst;
    readhead  = src;
    for(y=0; y<gh; ++y){
        for(x=0; x<gw; ++x){
            *(writehead++) = *readhead;
            readhead += xbytestep;
        }
        readhead += ybytestep;
    }
}

int vid_detect(char *devfile) {
    int errno;
    unsigned int i;

    if (-1 == (fd = open(devfile,O_RDWR|O_NONBLOCK))) {
        perror("!! error in opening video capture device: ");
        return -1;
    } else {
        close(fd);
        fd = open(devfile,O_RDWR);
    }

// Check that streaming is supported
    memset(&capability, 0, sizeof(capability));
    if(-1 == ioctl(fd, VIDIOC_QUERYCAP, &capability)) {
	perror("VIDIOC_QUERYCAP");
	exit(EXIT_FAILURE);
    }
    if((capability.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0){
	printf("Fatal: Device %s does not support video capture\n", capability.card);
	exit(EXIT_FAILURE);
    }
    if((capability.capabilities & V4L2_CAP_STREAMING) == 0){
	printf("Fatal: Device %s does not support streaming data capture\n", capability.card);
	exit(EXIT_FAILURE);
    }

    fprintf(stderr,"Device detected is %s\n",devfile);
    fprintf(stderr,"Card name: %s\n",capability.card);

// Switch to the selected video input
    if(-1 == ioctl(fd, VIDIOC_S_INPUT, &inputch)) {
	perror("VIDIOC_S_INPUT");
	exit(EXIT_FAILURE);
    }

// Get info about current video input
    memset(&input, 0, sizeof(input));
    input.index = inputch;
    if(-1 == ioctl(fd, VIDIOC_ENUMINPUT, &input)) {
	perror("VIDIOC_ENUMINPUT");
	exit(EXIT_FAILURE);
    }
    printf("Current input is %s\n", input.name);
// example 1-6
    memset(&standard, 0, sizeof(standard));
    standard.index = 0;
    while(0 == ioctl (fd, VIDIOC_ENUMSTD, &standard)){
	if(standard.id & input.std)
            printf("   - %s\n", standard.name);
	standard.index++;
    }
// Need to find out (request?) specific data format (sec 1.10.1)
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    /* the format.fmt.pix member, a v4l2_pix_format, is now filled in
       with default falues */
    if(-1 == ioctl(fd, VIDIOC_G_FMT, &format)) {
      perror("VIDIOC_G_FMT");
      exit(EXIT_FAILURE);
    }

    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;

    if(whchanged==1) {
    // fill in user defined values
     fprintf(stderr,"user defined size: %u x %u\n", user_w, user_h);
     format.fmt.pix.width  = user_w;
     format.fmt.pix.height = user_h;
    }

    if(-1 == ioctl(fd, VIDIOC_S_FMT, &format)) {
      perror("VIDIOC_S_FMT");
      exit(EXIT_FAILURE);
    }

    printf("Current capture is %u x %u\n",
           format.fmt.pix.width, format.fmt.pix.height);
    printf("format %4.4s, %u bytes-per-line\n",
           (char*)&format.fmt.pix.pixelformat,
           format.fmt.pix.bytesperline);

    return 1;
}


int vid_init() {
    int i, j;

    // TODO QUAA
    vw = format.fmt.pix.width;
    vh = format.fmt.pix.height;
    vbytesperline = format.fmt.pix.bytesperline;
    xbytestep = xstep + xstep; // for YUV422. for other formats may differ
    ybytestep = vbytesperline * (ystep-1);
    // we shrink our pixels crudely, by hopping over them:
    gw = vw / xstep;
    gh = vh / ystep;
    // aalib converts every block of 4 pixels to one character, so our sizes shrink by 2:
    aw = gw / 2;
    ah = gh / 2;

    greysize = gw * gh;
    grey = malloc(greysize); // To get grey from YUYV we simply ignore the U and V bytes
    printf("Grey buffer is %u bytes\n", greysize);
    for (j=0; j< 256; ++j)
        YtoRGB[j] = 1.164*(j-256);

    memset (&reqbuf, 0, sizeof (reqbuf));
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    reqbuf.count = 32;

    if (-1 == ioctl (fd, VIDIOC_REQBUFS, &reqbuf)) {
        if (errno == EINVAL)
            printf ("Fatal: Video capturing by mmap-streaming is not supported\n");
        else
            perror ("VIDIOC_REQBUFS");

        exit (EXIT_FAILURE);
    }
    buffers = calloc (reqbuf.count, sizeof (*buffers));
    if (buffers == NULL){
        printf("calloc failure!");
        exit (EXIT_FAILURE);
    }

    for (i = 0; i < reqbuf.count; i++) {

        memset (&buffer, 0, sizeof (buffer));
        buffer.type = reqbuf.type;
	buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = i;

        if (-1 == ioctl (fd, VIDIOC_QUERYBUF, &buffer)) {
            perror ("VIDIOC_QUERYBUF");
            exit (EXIT_FAILURE);
        }

        buffers[i].length = buffer.length; /* remember for munmap() */

        buffers[i].start = mmap (NULL, buffer.length,
                                 PROT_READ | PROT_WRITE, /* recommended */
                                 MAP_SHARED,             /* recommended */
                                 fd, buffer.m.offset);

        if (MAP_FAILED == buffers[i].start) {
            /* If you do not exit here you should unmap() and free()
             *                    the buffers mapped so far. */
            perror ("mmap");
            exit (EXIT_FAILURE);
        }
    }
/* OK, the memory is mapped, so let's queue up the buffers,
   next is: turn on streaming, and do the business. */

    for (i = 0; i < reqbuf.count; i++) {
        // queue up all the buffers for the first time
        memset (&buffer, 0, sizeof (buffer));
        buffer.type = reqbuf.type;
	buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = i;

        if (-1 == ioctl (fd, VIDIOC_QBUF, &buffer)) {
            perror ("VIDIOC_QBUF");
            exit (EXIT_FAILURE);
        }
    }

    // turn on streaming
    if(-1 == ioctl(fd, VIDIOC_STREAMON, &buftype)) {
	perror("VIDIOC_STREAMON");
	exit(EXIT_FAILURE);
    }


    for (i = 0; i < greysize; i++) {
	grey[i] = i % 160; //256;
    }
}


void grab_one () {

    // Can we have a buffer please?
    if (-1 == ioctl (fd, VIDIOC_DQBUF, &buffer)) {
        perror ("VIDIOC_DQBUF");
        exit (EXIT_FAILURE);
    }

    if((++framenum) == renderhop){
        framenum=0;
        YUV422_to_grey(buffers[buffer.index].start, grey, vw, vh);

        memcpy( aa_image(ascii_context), grey, greysize);
        aa_fastrender(ascii_context, 0, 0, vw/(xstep*2), vh/(ystep*2)); //TODO are the w&h args correct?
//		aa_render(ascii_context, ascii_rndparms, 0, 0, vw/(xstep*2), vh/(ystep*2)); //TODO are the w&h args correct?
        aa_flush(ascii_context);
    }


    // Thanks for lending us your buffer, you may have it back again:
    if (-1 == ioctl (fd, VIDIOC_QBUF, &buffer)) {
        perror ("VIDIOC_QBUF");
        exit (EXIT_FAILURE);
    }

}



void
config_init (int argc, char *argv[]) {
  int res;

  /* setup defaults */

  { /* device filename */
    struct stat st;
    if( stat("/dev/video",&st) <0)
      strcpy(device,"/dev/video0");
    else
      strcpy(device,"/dev/video");
  }
  strcpy(background,"000000");
  strcpy(foreground,"00FF00");
  strcpy(fontface,"courier"); /* you'd better choose monospace fonts */

  aa_geo.w = 80; // 96;
  aa_geo.h = 40; // 72;
  aa_geo.bright =  60;
  aa_geo.contrast = 4;
  aa_geo.gamma = 3;

  do {
    res = getopt_long (argc, argv, short_options, long_options, NULL);

    switch (res) {
    case 'h':
      fprintf (stderr, "%s", help);
      exit (0);
      break;
    case 'H':
      fprintf (stderr, "%s", help);
      fprintf (stderr, "\naalib options:\n%s",aa_help);
      exit(0);
    case 'v':
      exit (0);
      break;
    case 'q':
      quiet = 1;
      break;
    case 'm':
      if (strcasecmp (optarg, "live") == 0) {
        mode = LIVE;
      } else if (strcasecmp (optarg, "html") == 0) {
        mode = HTML;
        strcpy(aafile,"hasciicam.html");
      } else if (strcasecmp (optarg, "text") == 0) {
        mode = TEXT;
        strcpy(aafile,"hasciicam.asc");
      } else {
        fprintf (stderr, "!! invalid mode selected, using live\n");
        mode = LIVE;
      }
      break;
    case 'd':
      strncpy(device,optarg,256);
      break;
    case 'i':
      inputch = atoi (optarg);
      /*
	 here we assume that capture cards have maximum 3 channels
	 (usually the 4th, when present, is the radio tuner)
      */
      if (inputch > 3) {
	fprintf (stderr, "invalid input selected\n");
	exit (1);
      }
      break;

    case 's':
      {
	char *t;
	char *tt;
	t = optarg;
	while (isdigit (*t))
	  t++;
	*t = 0;
	user_w = atoi (optarg);
	tt = ++t;
	while (isdigit (*tt))
	  tt++;
	*tt = 0;
	user_h = atoi (t);
	whchanged = 1;
      }
      break;
    case 'S':
      fontsize = atoi (optarg);
      switch (fontsize) {
      case 1: linespace = 5; break;
      case 2: linespace = 10; break;
      case 3: linespace = 11; break;
      case 4: linespace = 13; break;
      default: linespace = 15; break;
      }
      break;
    case 'a':
      strncpy(fontface,optarg,256);
      break;
    case 'r':
      refresh = atoi (optarg);
      break;
    case 'o':
      if(mode>0)
	strncpy(aafile,optarg,256);
      break;
    case 'D':
      daemon_mode = 1;
      break;
    case 'b':
      aa_geo.bright = atoi (optarg);
      break;
    case 'c':
      aa_geo.contrast = atoi (optarg);
      break;
    case 'g':
      aa_geo.gamma = atoi (optarg);
      break;
    case 'I':
      invert = 1;
      break;
    case 'B':
      strncpy(background,optarg,64);
      break;
    case 'F':
      strncpy(foreground,optarg,64);
      break;
    case 'U':
      uid = atoi (optarg);
      break;
    case 'G':
      gid = atoi (optarg);
      break;
    }
  } while (res > 0);

}

/* here we go (chmicl broz rlz! :)*/

int
main (int argc, char **argv) {
    int i;
    char temp[160];

    /* reminder:
       !!! grabbing height & width should be double
       the ascii context width and height !!! */

  /* register signal traps */
  if (signal (SIGINT, quitproc) == SIG_ERR) {
      perror ("Couldn't install SIGINT handler"); exit (1); }
  fprintf (stderr, version, PACKAGE, VERSION);

  /* default values */
  uid = getuid ();
  gid = getgid ();


  /* initialize aalib default params */
  memcpy (&ascii_hwparms, &aa_defparams, sizeof (struct aa_hardware_params));
  ascii_rndparms = aa_getrenderparams();

  /* gathering aalib commandline options */
  aa_parseoptions (&ascii_hwparms, ascii_rndparms, &argc, argv);

  /* set hasciicam options */
  config_init (argc, argv);
  /* detect and init video device */
  if( vid_detect(device) > 0 ) {
    vid_init();
  } else
    exit(-1);

  /* width/height image setup */
  ascii_hwparms.font = NULL; // default font, thanks
  ascii_hwparms.width = aw;
  ascii_hwparms.height = ah;


  /* init the html header */
  snprintf (&hascii_header[0], 1024,
	    "<HTML>\n <HEAD> <TITLE>wow! (h)ascii 4 the masses!</TITLE>\n"
	    "<META HTTP-EQUIV=\"refresh\" CONTENT=\"%u\"; url=\"%s\">\n"
	    "<META HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\">\n"
	    "<STYLE TYPE=\"text/css\">\n"
	    "<!--\npre {\nletter-spacing: 1px;\n"
	    "layer-background-color: Black;\n"
	    "left : auto;\nline-height : %upx;\n}\n-->\n"
	    "</STYLE>\n</HEAD>\n<BODY bgcolor=\"#%s\" text=\"#%s\">\n"
	    "<FONT SIZE=%u face=\"%s\">\n<PRE>\n",
	    refresh, aafile, linespace, background, foreground, fontsize,
	    fontface);


  if( setuid (uid) != 0)
    fprintf(stderr, "Error: setuid to %u failed: %s\n",uid, strerror(errno));
  if( setgid (gid) != 0)
    fprintf(stderr, "Error: setgid to %u failed: %s\n",uid, strerror(errno));


  fprintf (stderr, "Ascii size is %dx%d\n", aw, ah);

  switch (mode)
    {
    case LIVE:
      fprintf (stderr, "using LIVE mode\n");
      break;

    case HTML:
      snprintf(aatmpfile,255,"%s.tmp",aafile);
      ascii_save.name = aatmpfile;
      ascii_save.format = &hascii_format;
      ascii_save.file = NULL;

      fprintf (stderr, "using HTML mode dumping to file %s\n", aafile);
      break;

    case TEXT:
      ascii_save.name = aafile;
      ascii_save.format = &aa_text_format;
      ascii_save.file = NULL;

      fprintf (stderr, "using TEXT mode dumping to file %s\n", aafile);

      break;

    default:
      break;
    }

  fprintf(stderr,"\n");

  /* aalib init */
  if (mode > 0)
    ascii_context = aa_init (&save_d, &ascii_hwparms, &ascii_save);
  else
    ascii_context = aa_autoinit (&ascii_hwparms);

  if(!ascii_context) {
    fprintf(stderr,"!! cannot initialize aalib\n");
    exit(-1);
  }


  ascii_rndparms->bright = aa_geo.bright;
  ascii_rndparms->contrast = aa_geo.contrast;
  ascii_rndparms->gamma = aa_geo.gamma;
  // those are left to be setted by aalib options
  //  ascii_rndparms->dither = AA_FLOYD_S;
  //  ascii_rndparms->inversion = invert;
  //  ascii_rndparms->randomval = 0;




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


  if (daemon_mode)
    if( daemon (0, 1) != 0)
      fprintf(stderr, "Error: cannot daemonize to background: %s\n", strerror(errno));




// *****************************************************************************
//   HEIA-FR ,  Embedded Systems 3 ,  TP04 - Hasciicam ,  Vallelian & Waeber
// *****************************************************************************

// Open the FIFO file to communicate with thread server_thr_send
fifo_fd = open(FIFO_PATH, O_WRONLY);
if(fifo_fd == -1) printf("Unable to open FIFO for writing !\n");

fifo_buf = malloc(FIFO_BUF_SIZE);

// *****************************************************************************



  while (userbreak <1) {
    grab_one ();
	/*aa_setpalette (gamma di colori, indice, colore rosso, verde, blu)*/

	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    memcpy (aa_image (ascii_context), grey, vid_geo.size);
    aa_render (ascii_context, ascii_rndparms, 0, 0,
	       vid_geo.w,vid_geo.h);

    aa_flush (ascii_context);
    //  unlink(aafile);
    rename(aatmpfile,aafile);



   // *****************************************************************************
   //   HEIA-FR ,  Embedded Systems 3 ,  TP04 - Hasciicam ,  Vallelian & Waeber
   // *****************************************************************************

   // Open the aafile
   aafile_fd = fopen(aafile, "rb");
   if(aafile_fd == -1) printf("Unable to open the aafile (%s) \n", aafile);

   // aafile start
   fseek(aafile_fd, 0L, SEEK_SET);
   while(!feof(aafile_fd)){

      nbBytes = fread(fifo_buf, sizeof(char), FIFO_BUF_SIZE, aafile_fd);
      //printf("%d bytes read from file\n", nbBytes);

      nbBytes = write(fifo_fd, fifo_buf, nbBytes);
      //printf("%d bytes wrote in FIFO\n", nbBytes);

   }

   fclose(aafile_fd);


   // *****************************************************************************



  }


  /* CLEAN EXIT */

  // turn off streaming
  if(-1 == ioctl(fd, VIDIOC_STREAMOFF, &buftype)) {
      perror("VIDIOC_STREAMOFF");
  }

/* Cleanup. */

  for (i = 0; i < reqbuf.count; i++)
      munmap (buffers[i].start, buffers[i].length);

  aa_close(ascii_context);
  free(grey);
  if(fd>0) close(fd);
  fprintf (stderr, "cya!\n");
  exit (0);
/*++userbreak;*/
}

/* signal handling */
void
quitproc (int Sig)
{

  fprintf (stderr, "interrupt caught, exiting.\n");
  userbreak = 1;
}
