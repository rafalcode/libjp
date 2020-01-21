/* edited jpegtran.c */
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<ctype.h>

#define JPEG_INTERNALS
#define EXIT_WARNING  2
#define READ_BINARY	"r"
#define WRITE_BINARY "w"
#define SIZEOF(object) ((size_t)sizeof(object))
#define CHKNARGS(x, a); \
    if((x-1) != (a)) { \
        printf("Usage error: program requires %i arguments as follows:\n", (a)); \
        printf("1: i/p jpeg fn, 2,3: crop-width,-height 4,5: x,y-offset 6: outputfilename.\n"); \
        exit(EXIT_FAILURE); \
    }

#include "jpeglib.h"
#include "jpegcomp.h"
#include "transupp.h"		/* Support routines for jpegtran */
#include "jversion.h"		/* for version message */
#include "config.h"

#define dstinfo_min_DCT_h_scaled_size DCTSIZE
#define dstinfo_min_DCT_v_scaled_size DCTSIZE

static const char *progname;	/* program name for error messages */
static JCOPY_OPTION copyoption;	/* -copy switch */
static jpeg_transform_info transformoption; /* image transformation options */

GLOBAL(jvirt_barray_ptr *) jtransform_adjust_parameters (j_decompress_ptr srcinfo, j_compress_ptr dstinfo, jvirt_barray_ptr *src_coef_arrays, jpeg_transform_info *info)
{
  if (info->num_components == 1) {
    /* For a single-component source, we force the destination sampling factors
     * to 1x1, with or without force_grayscale.  This is useful because some
     * decoders choke on grayscale images with other sampling factors.
     */
    dstinfo->comp_info[0].h_samp_factor = 1;
    dstinfo->comp_info[0].v_samp_factor = 1;
  }

  /* Correct the destination's image dimensions as necessary
   * for rotate/flip, resize, and crop operations.
   */
  dstinfo->image_width = info->output_width;
  dstinfo->image_height = info->output_height;

  /* Adjust Exif properties */
  if (srcinfo->marker_list != NULL &&
          srcinfo->marker_list->marker == JPEG_APP0+1 &&
          srcinfo->marker_list->data_length >= 6 &&
          GETJOCTET(srcinfo->marker_list->data[0]) == 0x45 &&
          GETJOCTET(srcinfo->marker_list->data[1]) == 0x78 &&
          GETJOCTET(srcinfo->marker_list->data[2]) == 0x69 &&
          GETJOCTET(srcinfo->marker_list->data[3]) == 0x66 &&
          GETJOCTET(srcinfo->marker_list->data[4]) == 0 &&
          GETJOCTET(srcinfo->marker_list->data[5]) == 0) {
      /* Suppress output of JFIF marker */
      dstinfo->write_JFIF_header = FALSE;
  }

  /* Return the appropriate output data set */
  if (info->workspace_coef_arrays != NULL)
      return info->workspace_coef_arrays;

  return src_coef_arrays;
}

LOCAL(void) do_crop(j_decompress_ptr srcinfo, j_compress_ptr dstinfo, JDIMENSION x_crop_offset, JDIMENSION y_crop_offset, jvirt_barray_ptr *src_coef_arrays, jvirt_barray_ptr *dst_coef_arrays)
{
    JDIMENSION dst_blk_y, x_crop_blocks, y_crop_blocks;
    int ci, offset_y;
    JBLOCKARRAY src_buffer, dst_buffer;
    jpeg_component_info *compptr;

    /* We simply have to copy the right amount of data (the destination's
     * image size) starting at the given X and Y offsets in the source.
     */
    for (ci = 0; ci < dstinfo->num_components; ci++) {
        compptr = dstinfo->comp_info + ci;
        x_crop_blocks = x_crop_offset * compptr->h_samp_factor;
        y_crop_blocks = y_crop_offset * compptr->v_samp_factor;

        for (dst_blk_y = 0; dst_blk_y < compptr->height_in_blocks; dst_blk_y += compptr->v_samp_factor) {

            dst_buffer = (*srcinfo->mem->access_virt_barray)
                ((j_common_ptr) srcinfo, dst_coef_arrays[ci], dst_blk_y, (JDIMENSION) compptr->v_samp_factor, TRUE);

            src_buffer = (*srcinfo->mem->access_virt_barray)
                ((j_common_ptr) srcinfo, src_coef_arrays[ci], dst_blk_y + y_crop_blocks, (JDIMENSION) compptr->v_samp_factor, FALSE);

            for (offset_y = 0; offset_y < compptr->v_samp_factor; offset_y++) {
                jcopy_block_row(src_buffer[offset_y] + x_crop_blocks,
                        dst_buffer[offset_y],
                        compptr->width_in_blocks);
            }
        }
    }
}

/* Parse an unsigned integer: subroutine for jtransform_parse_crop_spec.
 * Returns TRUE if valid integer found, FALSE if not.
 * *strptr is advanced over the digit string, and *result is set to its value.  */
LOCAL(boolean) jt_read_integer (const char ** strptr, JDIMENSION * result)
{
    const char * ptr = *strptr;
    JDIMENSION val = 0;

    for (; isdigit(*ptr); ptr++) {
        val = val * 10 + (JDIMENSION) (*ptr - '0');
    }
    *result = val;
    if (ptr == *strptr)
        return FALSE;
    *strptr = ptr;
    return TRUE;
}

/* Parse a crop specification (written in X11 geometry style).
 * The routine returns TRUE if the spec string is valid, FALSE if not.
 *
 * The crop spec string should have the format
 *	<width>x<height>{+-}<xoffset>{+-}<yoffset>
 * where width, height, xoffset, and yoffset are unsigned integers.
 * Each of the elements can be omitted to indicate a default value.
 * (A weakness of this style is that it is not possible to omit xoffset
 * while specifying yoffset, since they look alike.)
 *
 * This code is loosely based on XParseGeometry from the X11 distribution.
 */
GLOBAL(boolean) jtransform_parse_crop_spec (jpeg_transform_info *info, const char *spec)
{
    info->crop = FALSE;
    info->crop_width_set = JCROP_UNSET;
    info->crop_height_set = JCROP_UNSET;
    info->crop_xoffset_set = JCROP_UNSET;
    info->crop_yoffset_set = JCROP_UNSET;

    if (isdigit(*spec)) {
        /* fetch width */
        if (! jt_read_integer(&spec, &info->crop_width))
            return FALSE;
        info->crop_width_set = JCROP_POS;
    }
    if (*spec == 'x' || *spec == 'X') {	
        /* fetch height */
        spec++;
        if (! jt_read_integer(&spec, &info->crop_height))
            return FALSE;
        info->crop_height_set = JCROP_POS;
    }
    if (*spec == '+' || *spec == '-') {
        /* fetch xoffset */
        info->crop_xoffset_set = (*spec == '-') ? JCROP_NEG : JCROP_POS;
        spec++;
        if (! jt_read_integer(&spec, &info->crop_xoffset))
            return FALSE;
    }
    if (*spec == '+' || *spec == '-') {
        /* fetch yoffset */
        info->crop_yoffset_set = (*spec == '-') ? JCROP_NEG : JCROP_POS;
        spec++;
        if (! jt_read_integer(&spec, &info->crop_yoffset))
            return FALSE;
    }
    /* We had better have gotten to the end of the string. */
    if (*spec != '\0')
        return FALSE;
    info->crop = TRUE;
    return TRUE;
}

GLOBAL(boolean) jtransform_request_workspace (j_decompress_ptr srcinfo, jpeg_transform_info *info)
{
    jvirt_barray_ptr *coef_arrays;
    boolean need_workspace, transpose_it;
    jpeg_component_info *compptr;
    JDIMENSION xoffset, yoffset;
    JDIMENSION width_in_iMCUs, height_in_iMCUs;
    JDIMENSION width_in_blocks, height_in_blocks;
    int ci, h_samp_factor, v_samp_factor;

    /* Determine number of components in output image */
    if (info->force_grayscale &&
            srcinfo->jpeg_color_space == JCS_YCbCr &&
            srcinfo->num_components == 3)
        /* We'll only process the first component */
        info->num_components = 1;
    else
        /* Process all the components */
        info->num_components = srcinfo->num_components;

    /* Compute output image dimensions and related values. */
#if JPEG_LIB_VERSION >= 80
    jpeg_core_output_dimensions(srcinfo);
#else
    srcinfo->output_width = srcinfo->image_width;
    srcinfo->output_height = srcinfo->image_height;
#endif

    /* Return right away if -perfect is given and transformation is not perfect.  */
    if (info->perfect) {
        if (info->num_components == 1) {
            if (!jtransform_perfect_transform(srcinfo->output_width,
                        srcinfo->output_height,
                        srcinfo->_min_DCT_h_scaled_size,
                        srcinfo->_min_DCT_v_scaled_size,
                        info->transform))
                return FALSE;
        } else {
            if (!jtransform_perfect_transform(srcinfo->output_width,
                        srcinfo->output_height,
                        srcinfo->max_h_samp_factor * srcinfo->_min_DCT_h_scaled_size,
                        srcinfo->max_v_samp_factor * srcinfo->_min_DCT_v_scaled_size,
                        info->transform))
                return FALSE;
        }
    }

    /* If there is only one output component, force the iMCU size to be 1;
     * else use the source iMCU size.  (This allows us to do the right thing
     * when reducing color to grayscale, and also provides a handy way of
     * cleaning up "funny" grayscale images whose sampling factors are not 1x1.)
     */
    info->output_width = srcinfo->output_width;
    info->output_height = srcinfo->output_height;
    if (info->num_components == 1) {
        info->iMCU_sample_width = srcinfo->_min_DCT_h_scaled_size;
        info->iMCU_sample_height = srcinfo->_min_DCT_v_scaled_size;
    } else {
        info->iMCU_sample_width =
            srcinfo->max_h_samp_factor * srcinfo->_min_DCT_h_scaled_size;
        info->iMCU_sample_height =
            srcinfo->max_v_samp_factor * srcinfo->_min_DCT_v_scaled_size;
    }

    /* If cropping has been requested, compute the crop area's position and
     * dimensions, ensuring that its upper left corner falls at an iMCU boundary.  */
    if (info->crop) {
        /* Insert default values for unset crop parameters */
        if (info->crop_xoffset_set == JCROP_UNSET)
            info->crop_xoffset = 0;	/* default to +0 */
        if (info->crop_yoffset_set == JCROP_UNSET)
            info->crop_yoffset = 0;	/* default to +0 */
        if (info->crop_xoffset >= info->output_width ||
                info->crop_yoffset >= info->output_height)
            ERREXIT(srcinfo, JERR_BAD_CROP_SPEC);
        if (info->crop_width_set == JCROP_UNSET)
            info->crop_width = info->output_width - info->crop_xoffset;
        if (info->crop_height_set == JCROP_UNSET)
            info->crop_height = info->output_height - info->crop_yoffset;
        /* Ensure parameters are valid */
        if (info->crop_width <= 0 || info->crop_width > info->output_width ||
                info->crop_height <= 0 || info->crop_height > info->output_height ||
                info->crop_xoffset > info->output_width - info->crop_width ||
                info->crop_yoffset > info->output_height - info->crop_height)
            ERREXIT(srcinfo, JERR_BAD_CROP_SPEC);
        /* Convert negative crop offsets into regular offsets */
        if (info->crop_xoffset_set == JCROP_NEG)
            xoffset = info->output_width - info->crop_width - info->crop_xoffset;
        else
            xoffset = info->crop_xoffset;
        if (info->crop_yoffset_set == JCROP_NEG)
            yoffset = info->output_height - info->crop_height - info->crop_yoffset;
        else
            yoffset = info->crop_yoffset;
        /* Now adjust so that upper left corner falls at an iMCU boundary */
        info->output_width =
            info->crop_width + (xoffset % info->iMCU_sample_width);
        info->output_height =
            info->crop_height + (yoffset % info->iMCU_sample_height);
        /* Save x/y offsets measured in iMCUs */
        info->x_crop_offset = xoffset / info->iMCU_sample_width;
        info->y_crop_offset = yoffset / info->iMCU_sample_height;
    } else {
        info->x_crop_offset = 0;
        info->y_crop_offset = 0;
    }

    /* Figure out whether we need workspace arrays,
     * and if so whether they are transposed relative to the source.
     */
    need_workspace = FALSE;
    transpose_it = FALSE;
    switch (info->transform) {
        case JXFORM_NONE:
            if (info->x_crop_offset != 0 || info->y_crop_offset != 0)
                need_workspace = TRUE;
            /* No workspace needed if neither cropping nor transforming */
            break;
        default:
            printf("These transforms not allowed. Crop only\n"); 
            exit(EXIT_FAILURE);
            break;
    }

    /* Allocate workspace if needed.
     * Note that we allocate arrays padded out to the next iMCU boundary,
     * so that transform routines need not worry about missing edge blocks.
     */
    if (need_workspace) {
        coef_arrays = (jvirt_barray_ptr *)
            (*srcinfo->mem->alloc_small) ((j_common_ptr) srcinfo, JPOOL_IMAGE,
                    SIZEOF(jvirt_barray_ptr) * info->num_components);
        width_in_iMCUs = (JDIMENSION)
            jdiv_round_up((long) info->output_width,
                    (long) info->iMCU_sample_width);
        height_in_iMCUs = (JDIMENSION)
            jdiv_round_up((long) info->output_height,
                    (long) info->iMCU_sample_height);
        for (ci = 0; ci < info->num_components; ci++) {
            compptr = srcinfo->comp_info + ci;
            if (info->num_components == 1) {
                /* we're going to force samp factors to 1x1 in this case */
                h_samp_factor = v_samp_factor = 1;
            } else if (transpose_it) {
                h_samp_factor = compptr->v_samp_factor;
                v_samp_factor = compptr->h_samp_factor;
            } else {
                h_samp_factor = compptr->h_samp_factor;
                v_samp_factor = compptr->v_samp_factor;
            }
            width_in_blocks = width_in_iMCUs * h_samp_factor;
            height_in_blocks = height_in_iMCUs * v_samp_factor;
            coef_arrays[ci] = (*srcinfo->mem->request_virt_barray)
                ((j_common_ptr) srcinfo, JPOOL_IMAGE, FALSE,
                 width_in_blocks, height_in_blocks, (JDIMENSION) v_samp_factor);
        }
        info->workspace_coef_arrays = coef_arrays;
    } else
        info->workspace_coef_arrays = NULL;

    return TRUE;
}


/* Execute the actual transformation, if any.
 *
 * This must be called *after* jpeg_write_coefficients, because it depends
 * on jpeg_write_coefficients to have computed subsidiary values such as
 * the per-component width and height fields in the destination object.
 *
 * Note that some transformations will modify the source data arrays!
 */

GLOBAL(void) jtransform_execute_transform (j_decompress_ptr srcinfo, j_compress_ptr dstinfo, jvirt_barray_ptr *src_coef_arrays, jpeg_transform_info *info)
{
    jvirt_barray_ptr *dst_coef_arrays = info->workspace_coef_arrays;

    /* Note: conditions tested here should match those in switch statement
     * in jtransform_request_workspace() */
    switch (info->transform) {
        case JXFORM_NONE:
            if (info->x_crop_offset != 0 || info->y_crop_offset != 0)
                do_crop(srcinfo, dstinfo, info->x_crop_offset, info->y_crop_offset,
                        src_coef_arrays, dst_coef_arrays);
            break;
        default:
            printf("Usage error: only crop supported.\n"); 
    }
}

/* jtransform_perfect_transform
 *
 * Determine whether lossless transformation is perfectly
 * possible for a specified image and transformation.
 *
 * Inputs:
 *   image_width, image_height: source image dimensions.
 *   MCU_width, MCU_height: pixel dimensions of MCU.
 *   transform: transformation identifier.
 * Parameter sources from initialized jpeg_struct
 * (after reading source header):
 *   image_width = cinfo.image_width
 *   image_height = cinfo.image_height
 *   MCU_width = cinfo.max_h_samp_factor * cinfo.block_size
 *   MCU_height = cinfo.max_v_samp_factor * cinfo.block_size
 * Result:
 *   TRUE = perfect transformation possible
 *   FALSE = perfect transformation not possible
 *           (may use custom action then)
 */

GLOBAL(boolean) jtransform_perfect_transform(JDIMENSION image_width, JDIMENSION image_height, int MCU_width, int MCU_height, JXFORM_CODE transform)
{
    boolean result = TRUE; /* initialize TRUE */

    switch (transform) {
        case JXFORM_FLIP_H:
        case JXFORM_ROT_270:
            if (image_width % (JDIMENSION) MCU_width)
                result = FALSE;
            break;
        case JXFORM_FLIP_V:
        case JXFORM_ROT_90:
            if (image_height % (JDIMENSION) MCU_height)
                result = FALSE;
            break;
        case JXFORM_TRANSVERSE:
        case JXFORM_ROT_180:
            if (image_width % (JDIMENSION) MCU_width)
                result = FALSE;
            if (image_height % (JDIMENSION) MCU_height)
                result = FALSE;
            break;
        default:
            break;
    }

    return result;
}

/* Setup decompression object to save desired markers in memory.
 * This must be called before jpeg_read_header() to have the desired effect.
 */

    GLOBAL(void)
jcopy_markers_setup (j_decompress_ptr srcinfo, JCOPY_OPTION option)
{
#ifdef SAVE_MARKERS_SUPPORTED
    int m;

    /* Save comments except under NONE option */
    if (option != JCOPYOPT_NONE) {
        jpeg_save_markers(srcinfo, JPEG_COM, 0xFFFF);
    }
    /* Save all types of APPn markers iff ALL option */
    if (option == JCOPYOPT_ALL) {
        for (m = 0; m < 16; m++)
            jpeg_save_markers(srcinfo, JPEG_APP0 + m, 0xFFFF);
    }
#endif /* SAVE_MARKERS_SUPPORTED */
}

/* Copy markers saved in the given source object to the destination object.
 * This should be called just after jpeg_start_compress() or
 * jpeg_write_coefficients().
 * Note that those routines will have written the SOI, and also the
 * JFIF APP0 or Adobe APP14 markers if selected.
 */

    GLOBAL(void)
jcopy_markers_execute (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
        JCOPY_OPTION option)
{
    jpeg_saved_marker_ptr marker;

    /* In the current implementation, we don't actually need to examine the
     * option flag here; we just copy everything that got saved.
     * But to avoid confusion, we do not output JFIF and Adobe APP14 markers
     * if the encoder library already wrote one.
     */
    for (marker = srcinfo->marker_list; marker != NULL; marker = marker->next) {
        if (dstinfo->write_JFIF_header &&
                marker->marker == JPEG_APP0 &&
                marker->data_length >= 5 &&
                GETJOCTET(marker->data[0]) == 0x4A &&
                GETJOCTET(marker->data[1]) == 0x46 &&
                GETJOCTET(marker->data[2]) == 0x49 &&
                GETJOCTET(marker->data[3]) == 0x46 &&
                GETJOCTET(marker->data[4]) == 0)
            continue;			/* reject duplicate JFIF */
        if (dstinfo->write_Adobe_marker &&
                marker->marker == JPEG_APP0+14 &&
                marker->data_length >= 5 &&
                GETJOCTET(marker->data[0]) == 0x41 &&
                GETJOCTET(marker->data[1]) == 0x64 &&
                GETJOCTET(marker->data[2]) == 0x6F &&
                GETJOCTET(marker->data[3]) == 0x62 &&
                GETJOCTET(marker->data[4]) == 0x65)
            continue;			/* reject duplicate Adobe */
#ifdef NEED_FAR_POINTERS
        /* We could use jpeg_write_marker if the data weren't FAR... */
        {
            unsigned int i;
            jpeg_write_m_header(dstinfo, marker->marker, marker->data_length);
            for (i = 0; i < marker->data_length; i++)
                jpeg_write_m_byte(dstinfo, marker->data[i]);
        }
#else
        jpeg_write_marker(dstinfo, marker->marker,
                marker->data, marker->data_length);
#endif
    }
}

GLOBAL(FILE *) read_stdin(void)
{
    FILE * input_file = stdin;

#ifdef USE_SETMODE		/* need to hack file mode? */
    setmode(fileno(stdin), O_BINARY);
#endif
#ifdef USE_FDOPEN		/* need to re-open in binary mode? */
    if ((input_file = fdopen(fileno(stdin), READ_BINARY)) == NULL) {
        fprintf(stderr, "Cannot reopen stdin\n");
        exit(EXIT_FAILURE);
    }
#endif
    return input_file;
}

GLOBAL(FILE *) write_stdout (void)
{
    FILE * output_file = stdout;

#ifdef USE_SETMODE		/* need to hack file mode? */
    setmode(fileno(stdout), O_BINARY);
#endif
#ifdef USE_FDOPEN		/* need to re-open in binary mode? */
    if ((output_file = fdopen(fileno(stdout), WRITE_BINARY)) == NULL) {
        fprintf(stderr, "Cannot reopen stdout\n");
        exit(EXIT_FAILURE);
    }
#endif
    return output_file;
}

void printessens(struct jpeg_decompress_struct dcinfo)
{
    fprintf(stderr, "width: %i, height: %i inpcomps: %i inpcspace: %i\n", dcinfo.image_width, dcinfo.image_height, dcinfo.num_components, dcinfo.jpeg_color_space);
}

LOCAL(void) init_switches (j_compress_ptr cinfo, int cw, int ch, int parrh, int parrv)
{

    copyoption = JCOPYOPT_DEFAULT;
    transformoption.transform = JXFORM_NONE;
    transformoption.perfect = FALSE;
    transformoption.trim = FALSE;
    transformoption.force_grayscale = FALSE;
    transformoption.slow_hflip = FALSE;
    cinfo->err->trace_level = 0;

    transformoption.crop_width = cw;
    transformoption.crop_width_set = JCROP_POS;
    transformoption.crop_height = ch;
    transformoption.crop_height_set = JCROP_POS;
    transformoption.crop_xoffset = parrh;
    transformoption.crop_xoffset_set = JCROP_POS;
    transformoption.crop_yoffset = parrv;
    transformoption.crop_yoffset_set = JCROP_POS;
    /* and so we can write */
    transformoption.crop = TRUE;

    jpeg_simple_progression(cinfo);
}

int main (int argc, char **argv)
{
    CHKNARGS(argc, 6);
    struct jpeg_decompress_struct srcinfo;
    struct jpeg_compress_struct dstinfo;
    struct jpeg_error_mgr jsrcerr, jdsterr; /* two error structs, one for source "js*" and one for dest "jd*" */
    jvirt_barray_ptr *src_coef_arrays;
    jvirt_barray_ptr *dst_coef_arrays;
    /* We assume all-in-memory processing and can therefore use only a
     * single file pointer for sequential input and output operation.  */
    FILE *fp;

    progname = argv[0]; /* hm ... no strcpy? */

    /* Error handling important in libjpeg (probably good practice too).i
     * Initialize the JPEG decompression object with default error handling. */
    srcinfo.err = jpeg_std_error(&jsrcerr);
    jpeg_create_decompress(&srcinfo);
    /* Initialize the JPEG compression object with default error handling. */
    dstinfo.err = jpeg_std_error(&jdsterr);
    jpeg_create_compress(&dstinfo);

    /* Scan command line to find file names.
     * It is convenient to use just one switch-parsing routine, but the switch
     * values read here are mostly ignored; we will rescan the switches after
     * opening the input file.  Also note that most of the switches affect the
     * destination JPEG object, so we parse into that and then copy over what
     * needs to affects the source too.  */
    // file_index = parse_switches(&dstinfo, argc, argv, 0, FALSE); //nope instead now
    // init_switches(&dstinfo, 320, 240, 80, 230); // givem a crop a position 0,0 or size 320x240
    int wid = atoi(argv[2]);
    int hei = atoi(argv[3]);
    int xpo = atoi(argv[4]);
    int ypo = atoi(argv[5]);
    init_switches(&dstinfo, wid, hei, xpo, ypo); // givem a crop a position 0,0 or size 320x240

    jsrcerr.trace_level = jdsterr.trace_level;
    srcinfo.mem->max_memory_to_use = dstinfo.mem->max_memory_to_use; // didn't have much of an effect this one.

    /* Open the input file. */
    if ((fp = fopen(argv[1], READ_BINARY)) == NULL) {
        fprintf(stderr, "%s: can't open %s for reading\n", progname, argv[1]);
        exit(EXIT_FAILURE);
    }

    /* Specify data source for decompression */
    jpeg_stdio_src(&srcinfo, fp);

    /* Enable saving of extra markers that we want to copy */
    jcopy_markers_setup(&srcinfo, copyoption);

    /* Read file header */
    (void) jpeg_read_header(&srcinfo, TRUE);
    printessens(srcinfo); /* have a look */

    /* Any space needed by a transform option must be requested before
     * jpeg_read_coefficients so that memory allocation will be done right.
    the reference to perfect is some sort of switch that jpegtran had. */
    if (!jtransform_request_workspace(&srcinfo, &transformoption)) { /* failing occurs here */
        fprintf(stderr, "%s: transformation is not perfect\n", progname);
        exit(EXIT_FAILURE);
    }

    /* Read source file as DCT coefficients */
    src_coef_arrays = jpeg_read_coefficients(&srcinfo);

    /* Initialize destination compression parameters from source values */
    jpeg_copy_critical_parameters(&srcinfo, &dstinfo);

    /* Adjust destination parameters if required by transform options;
     * also find out which set of coefficient arrays will hold the output.  */
    dst_coef_arrays = jtransform_adjust_parameters(&srcinfo, &dstinfo, src_coef_arrays, &transformoption);

    /* Close input file, if we opened it.
     * Note: we assume that jpeg_read_coefficients consumed all input
     * until JPEG_REACHED_EOI, and that jpeg_finish_decompress will
     * only consume more while (! cinfo->inputctl->eoi_reached).
     * We cannot call jpeg_finish_decompress here since we still need the
     * virtual arrays allocated from the source object for processing.
     */
    if (fp != stdin)
        fclose(fp);

    /* Open the output file. */
    if ((fp = fopen(argv[6], WRITE_BINARY)) == NULL) {
        fprintf(stderr, "%s: can't open %s for writing\n", progname, argv[6]);
        exit(EXIT_FAILURE);
    }

    /* Adjust default compression parameters by re-parsing the optionsi: not sure what this gets us */
    // Nope ! dispense
    // file_index = parse_switches(&dstinfo, argc, argv, 0, TRUE);

    /* Specify data destination for compression */
    jpeg_stdio_dest(&dstinfo, fp);

    /* Start compressor (note no image data is actually written here) */
    jpeg_write_coefficients(&dstinfo, dst_coef_arrays);

    /* Copy to the output file any extra markers that we want to preserve */
    jcopy_markers_execute(&srcinfo, &dstinfo, copyoption);

    // jtransform_execute_transformation(&srcinfo, &dstinfo, src_coef_arrays, &transformoption);
    jvirt_barray_ptr *dst_coef_arrays2 = transformoption.workspace_coef_arrays;
    if (transformoption.x_crop_offset != 0 || transformoption.y_crop_offset != 0)
        do_crop(&srcinfo, &dstinfo, transformoption.x_crop_offset, transformoption.y_crop_offset, src_coef_arrays, dst_coef_arrays2);

    /* Finish compression and release memory */
    jpeg_finish_compress(&dstinfo);
    jpeg_destroy_compress(&dstinfo);
    (void) jpeg_finish_decompress(&srcinfo);
    jpeg_destroy_decompress(&srcinfo);

    /* Close output file, if we opened it */
    if (fp != stdout)
        fclose(fp);

    /* All done. */
    exit(jsrcerr.num_warnings + jdsterr.num_warnings ? EXIT_WARNING: EXIT_SUCCESS);
    return 0;			/* suppress no-return-value warnings */
}
