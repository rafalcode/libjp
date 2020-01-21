/* This header file is for some holding frequently used aspetc ratios and colours. These are constants which
 * are useful when using cairo. The idea is to pull them out using their names:
 * aspff is our aspectratio array
 * colsf is our colour array */

/* For accounting purposes we need to say how big each array is */
/*  Aspect ratio array size */
#define FFASP 29 /* ffmpeg's aspects ratios */
#define MYASP 0
#define SZASP (FFASP+MYASP)

/*  here is the aspect ratio datastruct, "aspstruct" */
typedef struct
{
    char nasp[7];
    int awd, aht;
} aspstruct;

/*  and here is the array itself */
const aspstruct aspff[SZASP]= { \
{"sqcif", 128, 96}, \
{"qcif", 176, 144}, \
{"cif", 352, 288}, \
{"4cif", 704, 576}, \
{"16cif", 1408, 1152}, \
{"qqvga", 160, 120}, \
{"qvga", 320, 240}, \
{"vga", 640, 480}, \
{"svga", 800, 600}, \
{"xga", 1024, 768}, \
{"uxga", 1600, 1200}, \
{"qxga", 2048, 1536}, \
{"sxga", 1280, 1024}, \
{"qsxga", 2560, 2048}, \
{"hsxga", 5120, 4096}, \
{"wvga", 852, 480}, \
{"wxga", 1366, 768}, \
{"wsxga", 1600, 1024}, \
{"wuxga", 1920, 1200}, \
{"woxga", 2560, 1600}, \
{"wqsxga", 3200, 2048}, \
{"wquxga", 3840, 2400}, \
{"whsxga", 6400, 4096}, \
{"whuxga", 7680, 4800}, \
{"cga", 320,200}, \
{"ega", 640, 350}, \
{"hd480", 852, 480}, \
{"hd720", 1280, 720}, \
{"hd1080", 1920, 1080} };
// taken from ffmpeg manual
//       -aspect aspect
//	   Set aspect ratio (4:3, 16:9 or 1.3333, 1.7777).
//

/*  colour array size */
#define SZCOL 21

typedef struct /*  struct for holding our colours, we'll be having an array of these. */
{
    char nam[32];
    float rgb[3];
} colfloat_t;

const colfloat_t colsf[21] = { {"foreground", {0.866667, 0.866667, 0.866667} }, {"background", {0.066667, 0.066667, 0.066667} }, {"color0", {0.098039, 0.098039, 0.098039} }, {"color8", {0.627451, 0.321569, 0.176471} }, {"color1", {0.501961, 0.196078, 0.196078} }, {"color9", {0.596078, 0.168627, 0.168627} }, {"color2", {0.356863, 0.462745, 0.184314} }, {"color10", {0.537255, 0.721569, 0.247059} }, {"color3", {0.666667, 0.600000, 0.262745} }, {"color11", {0.937255, 0.937255, 0.376471} }, {"color4", {0.196078, 0.298039, 0.501961} }, {"color12", {0.168627, 0.309804, 0.596078} }, {"color5", {0.439216, 0.423529, 0.603922} }, {"color13", {0.509804, 0.415686, 0.694118} }, {"color6", {0.572549, 0.694118, 0.619608} }, {"color14", {0.631373, 0.803922, 0.803922} }, {"color7", {1.000000, 1.000000, 1.000000} }, {"color15", {0.866667, 0.866667, 0.866667} }, {"indyell", {0.945098, 0.596078, 0.000000} }, {"italgreen", {0.000000, 0.337255, 0.301961} }, {"strongred", {0.776471, 0.000000, 0.137255} } };
