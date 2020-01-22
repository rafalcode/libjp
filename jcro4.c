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
        printf("Usage error: program requires %i arguments.\n", (a)); \
        exit(EXIT_FAILURE); \
    }

#include "jpeglib.h"
#include "jpegcomp.h"

static const char *progname;	/* program name for error messages */

typedef enum /* JCROP_CODE */
{
	JCROP_UNSET,
	JCROP_POS,
	JCROP_NEG
} JCROP_CODE;

typedef enum /* JXFORM_CODE */
{
	JXFORM_NONE,		/* no transformation */
	JXFORM_FLIP_H,		/* horizontal flip */
	JXFORM_FLIP_V,		/* vertical flip */
	JXFORM_TRANSPOSE,	/* transpose across UL-to-LR axis */
	JXFORM_TRANSVERSE,	/* transpose across UR-to-LL axis */
	JXFORM_ROT_90,		/* 90-degree clockwise rotation */
	JXFORM_ROT_180,		/* 180-degree rotation */
	JXFORM_ROT_270		/* 270-degree clockwise (or 90 ccw) */
} JXFORM_CODE;

typedef struct /* jpeg_transform_info */
{
  JXFORM_CODE transform;	/* image transform operator */
  boolean perfect;		/* if TRUE, fail if partial MCUs are requested */
  boolean trim;			/* if TRUE, trim partial MCUs as needed */
  boolean force_grayscale;	/* if TRUE, convert color image to grayscale */
  boolean crop;			/* if TRUE, crop source image */
  boolean slow_hflip;  /* For best performance, the JXFORM_FLIP_H transform
                          normally modifies the source coefficients in place.
                          Setting this to TRUE will instead use a slower,
                          double-buffered algorithm, which leaves the source
                          coefficients in tact (necessary if other transformed
                          images must be generated from the same set of
                          coefficients. */

  /* Crop parameters: application need not set these unless crop is TRUE.
   * These can be filled in by jtransform_parse_crop_spec().
   */
  JDIMENSION crop_width;	/* Width of selected region */
  JCROP_CODE crop_width_set;
  JDIMENSION crop_height;	/* Height of selected region */
  JCROP_CODE crop_height_set;
  JDIMENSION crop_xoffset;	/* X offset of selected region */
  JCROP_CODE crop_xoffset_set;	/* (negative measures from right edge) */
  JDIMENSION crop_yoffset;	/* Y offset of selected region */
  JCROP_CODE crop_yoffset_set;	/* (negative measures from bottom edge) */

  /* Internal workspace: caller should not touch these */
  int num_components;		/* # of components in workspace */
  jvirt_barray_ptr *workspace_coef_arrays; /* workspace for transformations */
  JDIMENSION output_width;	/* cropped destination dimensions */
  JDIMENSION output_height;
  JDIMENSION x_crop_offset;	/* destination crop offsets measured in iMCUs */
  JDIMENSION y_crop_offset;
  int iMCU_sample_width;	/* destination iMCU size */
  int iMCU_sample_height;
} jpeg_transform_info;

GLOBAL(jvirt_barray_ptr *) jtransform_adjust_parameters(j_decompress_ptr srcinfo, j_compress_ptr dstinfo, jvirt_barray_ptr *src_coef_arrays, jpeg_transform_info *info)
{
    if (info->num_components == 1) {
        /* For a single-component source, we force the destination sampling factors
         * to 1x1, with or without force_grayscale.  This is useful because some
         * decoders choke on grayscale images with other sampling factors. */
        dstinfo->comp_info[0].h_samp_factor = 1;
        dstinfo->comp_info[0].v_samp_factor = 1;
    }

    /* Correct the destination's image dimensions as necessary
     * for rotate/flip, resize, and crop operations. */
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
GLOBAL(boolean) jtransform_request_workspace (j_decompress_ptr srcinfo, jpeg_transform_info *info)
{
    jvirt_barray_ptr *coef_arrays;
    boolean need_workspace, transpose_it;
    jpeg_component_info *compptr;
    JDIMENSION xoffset, yoffset;
    JDIMENSION width_in_iMCUs, height_in_iMCUs;
    JDIMENSION width_in_blocks, height_in_blocks;
    int ci, h_samp_factor, v_samp_factor;

    info->num_components = srcinfo->num_components;

    srcinfo->output_width = srcinfo->image_width;
    srcinfo->output_height = srcinfo->image_height;

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
    if(info->transform == JXFORM_NONE) {
        if (info->x_crop_offset != 0 || info->y_crop_offset != 0)
            need_workspace = TRUE;
    } else {
        printf("Non-crop transforms not allowed. Crop only\n"); 
        exit(EXIT_FAILURE);
    }

    /* Allocate workspace if needed.
     * Note that we allocate arrays padded out to the next iMCU boundary,
     * so that transform routines need not worry about missing edge blocks.
     */
    if (need_workspace) {
        coef_arrays = (jvirt_barray_ptr *)
            (*srcinfo->mem->alloc_small) ((j_common_ptr) srcinfo, JPOOL_IMAGE, SIZEOF(jvirt_barray_ptr) * info->num_components);

        width_in_iMCUs = (JDIMENSION)
            jdiv_round_up((long) info->output_width, (long) info->iMCU_sample_width);

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

LOCAL(void) init_switches (j_compress_ptr cinfo, jpeg_transform_info *transformoption, int cw, int ch, int parrh, int parrv)
{
    transformoption->transform = JXFORM_NONE;
    transformoption->perfect = FALSE;
    transformoption->trim = FALSE;
    transformoption->force_grayscale = FALSE;
    transformoption->slow_hflip = FALSE;
    cinfo->err->trace_level = 0;

    transformoption->crop_width = cw;
    transformoption->crop_width_set = JCROP_POS;
    transformoption->crop_height = ch;
    transformoption->crop_height_set = JCROP_POS;
    transformoption->crop_xoffset = parrh;
    transformoption->crop_xoffset_set = JCROP_POS;
    transformoption->crop_yoffset = parrv;
    transformoption->crop_yoffset_set = JCROP_POS;
    /* and so we can write */
    transformoption->crop = TRUE;

    jpeg_simple_progression(cinfo);
}

int main (int argc, char **argv)
{
    /* declarations of info and error structs */
    struct jpeg_decompress_struct srcinfo;
    struct jpeg_compress_struct dstinfo;
    struct jpeg_error_mgr jsrcerr, jdsterr;

    /* many of your problems are oging to come from these: */
    jvirt_barray_ptr *src_coef_arrays;
    jvirt_barray_ptr *dst_coef_arrays;
    jvirt_barray_ptr *dst_coef_arrays2; /* used for the actuall transform at hand */
    /* this guy is fairly critical as well: */
    jpeg_transform_info transformoption;

    FILE *fp;
    int k;
    char *outfilename=calloc(128, sizeof(char));

    progname = argv[0];

    /* still no input file .. but preparing the info & error structs */
    /* Initialize the JPEG compression object with default error handling. */
    srcinfo.err = jpeg_std_error(&jsrcerr);
    jpeg_create_decompress(&srcinfo);

    /* Open the input file. */
    CHKNARGS(argc, 1);
    if ((fp = fopen(argv[1], READ_BINARY)) == NULL) {
        fprintf(stderr, "%s: can't open %s for reading\n", progname, argv[1]);
        exit(EXIT_FAILURE);
    }

    /* OK, fileptr open, we can continue witht he src file */
    jpeg_stdio_src(&srcinfo, fp);
    (void) jpeg_read_header(&srcinfo, TRUE);
    printessens(srcinfo); /* have a look */
    src_coef_arrays = jpeg_read_coefficients(&srcinfo);
    fclose(fp); /* I risk an early close of the src fileptr */

    int vs[2]={170, 860};
    int hs[2]={230, 640};

    for(k=0;k<2;++k) {

        /* now we're within the loop, we can set up our dst image. Note it has already been declared. */
        dstinfo.err = jpeg_std_error(&jdsterr);
        jpeg_create_compress(&dstinfo);
        init_switches(&dstinfo, &transformoption, 320, 240, vs[k], hs[k]); // givem a crop a position 0,0 or size 320x240
        jsrcerr.trace_level = jdsterr.trace_level;
        srcinfo.mem->max_memory_to_use = dstinfo.mem->max_memory_to_use; // didn't have much of an effect this one.

        /* Any space needed by a transform option must be requested before
         * jpeg_read_coefficients so that memory allocation will be done right.
         the reference to perfect is some sort of switch that jpegtran had. */
        if (!jtransform_request_workspace(&srcinfo, &transformoption)) { /* failing occurs here */
            fprintf(stderr, "%s: transformation is not perfect\n", progname);
            exit(EXIT_FAILURE);
        }

        jpeg_copy_critical_parameters(&srcinfo, &dstinfo);
        dst_coef_arrays = jtransform_adjust_parameters(&srcinfo, &dstinfo, src_coef_arrays, &transformoption);

        /* Open the output file. */
        sprintf(outfilename,"%i_%s", k, argv[1]);
        if (outfilename != NULL)
            if ((fp = fopen(outfilename, WRITE_BINARY)) == NULL) {
                fprintf(stderr, "%s: can't open %s for writing\n", progname, outfilename);
                exit(EXIT_FAILURE);
            }

        jpeg_stdio_dest(&dstinfo, fp);
        jpeg_write_coefficients(&dstinfo, dst_coef_arrays);
        dst_coef_arrays2 = transformoption.workspace_coef_arrays;
        if (transformoption.x_crop_offset != 0 || transformoption.y_crop_offset != 0)
            do_crop(&srcinfo, &dstinfo, transformoption.x_crop_offset, transformoption.y_crop_offset, src_coef_arrays, dst_coef_arrays2);

        jpeg_finish_compress(&dstinfo);
        fclose(fp);
        jpeg_destroy_compress(&dstinfo);
    }

    (void) jpeg_finish_decompress(&srcinfo);
    jpeg_destroy_decompress(&srcinfo);
    free(outfilename);

    exit(jsrcerr.num_warnings + jdsterr.num_warnings ? EXIT_WARNING: EXIT_SUCCESS);
    return 0;
}
