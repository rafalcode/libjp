/* edited jpegtran.c */
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<ctype.h>

#define JPEG_INTERNALS

#include "jinclude.h"
#include "jpeglib.h"
#include "jpegcomp.h"
#include "cdjpeg.h"
#include "transupp.h"		/* Support routines for jpegtran */
#include "jversion.h"		/* for version message */
#include "config.h"



#if JPEG_LIB_VERSION >= 70
#define dstinfo_min_DCT_h_scaled_size dstinfo->min_DCT_h_scaled_size
#define dstinfo_min_DCT_v_scaled_size dstinfo->min_DCT_v_scaled_size
#else
#define dstinfo_min_DCT_h_scaled_size DCTSIZE
#define dstinfo_min_DCT_v_scaled_size DCTSIZE
#endif

static const char *progname;	/* program name for error messages */
static char * outfilename;	/* for -outfile switch */
static JCOPY_OPTION copyoption;	/* -copy switch */
static jpeg_transform_info transformoption; /* image transformation options */

LOCAL(void) do_crop (j_decompress_ptr srcinfo, j_compress_ptr dstinfo, JDIMENSION x_crop_offset, JDIMENSION y_crop_offset, jvirt_barray_ptr *src_coef_arrays, jvirt_barray_ptr *dst_coef_arrays)
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
        for (dst_blk_y = 0; dst_blk_y < compptr->height_in_blocks;
                dst_blk_y += compptr->v_samp_factor) {
            dst_buffer = (*srcinfo->mem->access_virt_barray)
                ((j_common_ptr) srcinfo, dst_coef_arrays[ci], dst_blk_y,
                 (JDIMENSION) compptr->v_samp_factor, TRUE);
            src_buffer = (*srcinfo->mem->access_virt_barray)
                ((j_common_ptr) srcinfo, src_coef_arrays[ci],
                 dst_blk_y + y_crop_blocks,
                 (JDIMENSION) compptr->v_samp_factor, FALSE);
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
 * *strptr is advanced over the digit string, and *result is set to its value.
 */
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


/* Trim off any partial iMCUs on the indicated destination edge */

    LOCAL(void)
trim_right_edge (jpeg_transform_info *info, JDIMENSION full_width)
{
    JDIMENSION MCU_cols;

    MCU_cols = info->output_width / info->iMCU_sample_width;
    if (MCU_cols > 0 && info->x_crop_offset + MCU_cols ==
            full_width / info->iMCU_sample_width)
        info->output_width = MCU_cols * info->iMCU_sample_width;
}

    LOCAL(void)
trim_bottom_edge (jpeg_transform_info *info, JDIMENSION full_height)
{
    JDIMENSION MCU_rows;

    MCU_rows = info->output_height / info->iMCU_sample_height;
    if (MCU_rows > 0 && info->y_crop_offset + MCU_rows ==
            full_height / info->iMCU_sample_height)
        info->output_height = MCU_rows * info->iMCU_sample_height;
}


/* Request any required workspace.
 *
 * This routine figures out the size that the output image will be
 * (which implies that all the transform parameters must be set before
 * it is called).
 *
 * We allocate the workspace virtual arrays from the source decompression
 * object, so that all the arrays (both the original data and the workspace)
 * will be taken into account while making memory management decisions.
 * Hence, this routine must be called after jpeg_read_header (which reads
 * the image dimensions) and before jpeg_read_coefficients (which realizes
 * the source's virtual arrays).
 *
 * This function returns FALSE right away if -perfect is given
 * and transformation is not perfect.  Otherwise returns TRUE.
 */

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

    /* Return right away if -perfect is given and transformation is not perfect.
    */
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
    switch (info->transform) {
        case JXFORM_TRANSPOSE:
        case JXFORM_TRANSVERSE:
        case JXFORM_ROT_90:
        case JXFORM_ROT_270:
            info->output_width = srcinfo->output_height;
            info->output_height = srcinfo->output_width;
            if (info->num_components == 1) {
                info->iMCU_sample_width = srcinfo->_min_DCT_v_scaled_size;
                info->iMCU_sample_height = srcinfo->_min_DCT_h_scaled_size;
            } else {
                info->iMCU_sample_width =
                    srcinfo->max_v_samp_factor * srcinfo->_min_DCT_v_scaled_size;
                info->iMCU_sample_height =
                    srcinfo->max_h_samp_factor * srcinfo->_min_DCT_h_scaled_size;
            }
            break;
        default:
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
            break;
    }

    /* If cropping has been requested, compute the crop area's position and
     * dimensions, ensuring that its upper left corner falls at an iMCU boundary.
     */
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
        case JXFORM_FLIP_H:
            if (info->trim)
                trim_right_edge(info, srcinfo->output_width);
            if (info->y_crop_offset != 0 || info->slow_hflip)
                need_workspace = TRUE;
            /* do_flip_h_no_crop doesn't need a workspace array */
            break;
        case JXFORM_FLIP_V:
            if (info->trim)
                trim_bottom_edge(info, srcinfo->output_height);
            /* Need workspace arrays having same dimensions as source image. */
            need_workspace = TRUE;
            break;
        case JXFORM_TRANSPOSE:
            /* transpose does NOT have to trim anything */
            /* Need workspace arrays having transposed dimensions. */
            need_workspace = TRUE;
            transpose_it = TRUE;
            break;
        case JXFORM_TRANSVERSE:
            if (info->trim) {
                trim_right_edge(info, srcinfo->output_height);
                trim_bottom_edge(info, srcinfo->output_width);
            }
            /* Need workspace arrays having transposed dimensions. */
            need_workspace = TRUE;
            transpose_it = TRUE;
            break;
        case JXFORM_ROT_90:
            if (info->trim)
                trim_right_edge(info, srcinfo->output_height);
            /* Need workspace arrays having transposed dimensions. */
            need_workspace = TRUE;
            transpose_it = TRUE;
            break;
        case JXFORM_ROT_180:
            if (info->trim) {
                trim_right_edge(info, srcinfo->output_width);
                trim_bottom_edge(info, srcinfo->output_height);
            }
            /* Need workspace arrays having same dimensions as source image. */
            need_workspace = TRUE;
            break;
        case JXFORM_ROT_270:
            if (info->trim)
                trim_bottom_edge(info, srcinfo->output_width);
            /* Need workspace arrays having transposed dimensions. */
            need_workspace = TRUE;
            transpose_it = TRUE;
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


/* Transpose destination image parameters */

    LOCAL(void)
transpose_critical_parameters (j_compress_ptr dstinfo)
{
    int tblno, i, j, ci, itemp;
    jpeg_component_info *compptr;
    JQUANT_TBL *qtblptr;
    JDIMENSION jtemp;
    UINT16 qtemp;

    /* Transpose image dimensions */
    jtemp = dstinfo->image_width;
    dstinfo->image_width = dstinfo->image_height;
    dstinfo->image_height = jtemp;
#if JPEG_LIB_VERSION >= 70
    itemp = dstinfo->min_DCT_h_scaled_size;
    dstinfo->min_DCT_h_scaled_size = dstinfo->min_DCT_v_scaled_size;
    dstinfo->min_DCT_v_scaled_size = itemp;
#endif

    /* Transpose sampling factors */
    for (ci = 0; ci < dstinfo->num_components; ci++) {
        compptr = dstinfo->comp_info + ci;
        itemp = compptr->h_samp_factor;
        compptr->h_samp_factor = compptr->v_samp_factor;
        compptr->v_samp_factor = itemp;
    }

    /* Transpose quantization tables */
    for (tblno = 0; tblno < NUM_QUANT_TBLS; tblno++) {
        qtblptr = dstinfo->quant_tbl_ptrs[tblno];
        if (qtblptr != NULL) {
            for (i = 0; i < DCTSIZE; i++) {
                for (j = 0; j < i; j++) {
                    qtemp = qtblptr->quantval[i*DCTSIZE+j];
                    qtblptr->quantval[i*DCTSIZE+j] = qtblptr->quantval[j*DCTSIZE+i];
                    qtblptr->quantval[j*DCTSIZE+i] = qtemp;
                }
            }
        }
    }
}


/* Adjust Exif image parameters.
 *
 * We try to adjust the Tags ExifImageWidth and ExifImageHeight if possible.
 */

    LOCAL(void)
adjust_exif_parameters (JOCTET FAR * data, unsigned int length,
        JDIMENSION new_width, JDIMENSION new_height)
{
    boolean is_motorola; /* Flag for byte order */
    unsigned int number_of_tags, tagnum;
    unsigned int firstoffset, offset;
    JDIMENSION new_value;

    if (length < 12) return; /* Length of an IFD entry */

    /* Discover byte order */
    if (GETJOCTET(data[0]) == 0x49 && GETJOCTET(data[1]) == 0x49)
        is_motorola = FALSE;
    else if (GETJOCTET(data[0]) == 0x4D && GETJOCTET(data[1]) == 0x4D)
        is_motorola = TRUE;
    else
        return;

    /* Check Tag Mark */
    if (is_motorola) {
        if (GETJOCTET(data[2]) != 0) return;
        if (GETJOCTET(data[3]) != 0x2A) return;
    } else {
        if (GETJOCTET(data[3]) != 0) return;
        if (GETJOCTET(data[2]) != 0x2A) return;
    }

    /* Get first IFD offset (offset to IFD0) */
    if (is_motorola) {
        if (GETJOCTET(data[4]) != 0) return;
        if (GETJOCTET(data[5]) != 0) return;
        firstoffset = GETJOCTET(data[6]);
        firstoffset <<= 8;
        firstoffset += GETJOCTET(data[7]);
    } else {
        if (GETJOCTET(data[7]) != 0) return;
        if (GETJOCTET(data[6]) != 0) return;
        firstoffset = GETJOCTET(data[5]);
        firstoffset <<= 8;
        firstoffset += GETJOCTET(data[4]);
    }
    if (firstoffset > length - 2) return; /* check end of data segment */

    /* Get the number of directory entries contained in this IFD */
    if (is_motorola) {
        number_of_tags = GETJOCTET(data[firstoffset]);
        number_of_tags <<= 8;
        number_of_tags += GETJOCTET(data[firstoffset+1]);
    } else {
        number_of_tags = GETJOCTET(data[firstoffset+1]);
        number_of_tags <<= 8;
        number_of_tags += GETJOCTET(data[firstoffset]);
    }
    if (number_of_tags == 0) return;
    firstoffset += 2;

    /* Search for ExifSubIFD offset Tag in IFD0 */
    for (;;) {
        if (firstoffset > length - 12) return; /* check end of data segment */
        /* Get Tag number */
        if (is_motorola) {
            tagnum = GETJOCTET(data[firstoffset]);
            tagnum <<= 8;
            tagnum += GETJOCTET(data[firstoffset+1]);
        } else {
            tagnum = GETJOCTET(data[firstoffset+1]);
            tagnum <<= 8;
            tagnum += GETJOCTET(data[firstoffset]);
        }
        if (tagnum == 0x8769) break; /* found ExifSubIFD offset Tag */
        if (--number_of_tags == 0) return;
        firstoffset += 12;
    }

    /* Get the ExifSubIFD offset */
    if (is_motorola) {
        if (GETJOCTET(data[firstoffset+8]) != 0) return;
        if (GETJOCTET(data[firstoffset+9]) != 0) return;
        offset = GETJOCTET(data[firstoffset+10]);
        offset <<= 8;
        offset += GETJOCTET(data[firstoffset+11]);
    } else {
        if (GETJOCTET(data[firstoffset+11]) != 0) return;
        if (GETJOCTET(data[firstoffset+10]) != 0) return;
        offset = GETJOCTET(data[firstoffset+9]);
        offset <<= 8;
        offset += GETJOCTET(data[firstoffset+8]);
    }
    if (offset > length - 2) return; /* check end of data segment */

    /* Get the number of directory entries contained in this SubIFD */
    if (is_motorola) {
        number_of_tags = GETJOCTET(data[offset]);
        number_of_tags <<= 8;
        number_of_tags += GETJOCTET(data[offset+1]);
    } else {
        number_of_tags = GETJOCTET(data[offset+1]);
        number_of_tags <<= 8;
        number_of_tags += GETJOCTET(data[offset]);
    }
    if (number_of_tags < 2) return;
    offset += 2;

    /* Search for ExifImageWidth and ExifImageHeight Tags in this SubIFD */
    do {
        if (offset > length - 12) return; /* check end of data segment */
        /* Get Tag number */
        if (is_motorola) {
            tagnum = GETJOCTET(data[offset]);
            tagnum <<= 8;
            tagnum += GETJOCTET(data[offset+1]);
        } else {
            tagnum = GETJOCTET(data[offset+1]);
            tagnum <<= 8;
            tagnum += GETJOCTET(data[offset]);
        }
        if (tagnum == 0xA002 || tagnum == 0xA003) {
            if (tagnum == 0xA002)
                new_value = new_width; /* ExifImageWidth Tag */
            else
                new_value = new_height; /* ExifImageHeight Tag */
            if (is_motorola) {
                data[offset+2] = 0; /* Format = unsigned long (4 octets) */
                data[offset+3] = 4;
                data[offset+4] = 0; /* Number Of Components = 1 */
                data[offset+5] = 0;
                data[offset+6] = 0;
                data[offset+7] = 1;
                data[offset+8] = 0;
                data[offset+9] = 0;
                data[offset+10] = (JOCTET)((new_value >> 8) & 0xFF);
                data[offset+11] = (JOCTET)(new_value & 0xFF);
            } else {
                data[offset+2] = 4; /* Format = unsigned long (4 octets) */
                data[offset+3] = 0;
                data[offset+4] = 1; /* Number Of Components = 1 */
                data[offset+5] = 0;
                data[offset+6] = 0;
                data[offset+7] = 0;
                data[offset+8] = (JOCTET)(new_value & 0xFF);
                data[offset+9] = (JOCTET)((new_value >> 8) & 0xFF);
                data[offset+10] = 0;
                data[offset+11] = 0;
            }
        }
        offset += 12;
    } while (--number_of_tags);
}


/* Adjust output image parameters as needed.
 *
 * This must be called after jpeg_copy_critical_parameters()
 * and before jpeg_write_coefficients().
 *
 * The return value is the set of virtual coefficient arrays to be written
 * (either the ones allocated by jtransform_request_workspace, or the
 * original source data arrays).  The caller will need to pass this value
 * to jpeg_write_coefficients().
 */

    GLOBAL(jvirt_barray_ptr *)
jtransform_adjust_parameters (j_decompress_ptr srcinfo,
        j_compress_ptr dstinfo,
        jvirt_barray_ptr *src_coef_arrays,
        jpeg_transform_info *info)
{
    /* If force-to-grayscale is requested, adjust destination parameters */
    if (info->force_grayscale) {
        /* First, ensure we have YCbCr or grayscale data, and that the source's
         * Y channel is full resolution.  (No reasonable person would make Y
         * be less than full resolution, so actually coping with that case
         * isn't worth extra code space.  But we check it to avoid crashing.)
         */
        if (((dstinfo->jpeg_color_space == JCS_YCbCr &&
                        dstinfo->num_components == 3) ||
                    (dstinfo->jpeg_color_space == JCS_GRAYSCALE &&
                     dstinfo->num_components == 1)) &&
                srcinfo->comp_info[0].h_samp_factor == srcinfo->max_h_samp_factor &&
                srcinfo->comp_info[0].v_samp_factor == srcinfo->max_v_samp_factor) {
            /* We use jpeg_set_colorspace to make sure subsidiary settings get fixed
             * properly.  Among other things, it sets the target h_samp_factor &
             * v_samp_factor to 1, which typically won't match the source.
             * We have to preserve the source's quantization table number, however.
             */
            int sv_quant_tbl_no = dstinfo->comp_info[0].quant_tbl_no;
            jpeg_set_colorspace(dstinfo, JCS_GRAYSCALE);
            dstinfo->comp_info[0].quant_tbl_no = sv_quant_tbl_no;
        } else {
            /* Sorry, can't do it */
            ERREXIT(dstinfo, JERR_CONVERSION_NOTIMPL);
        }
    } else if (info->num_components == 1) {
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
#if JPEG_LIB_VERSION >= 70
    dstinfo->jpeg_width = info->output_width;
    dstinfo->jpeg_height = info->output_height;
#endif

    /* Transpose destination image parameters */
    switch (info->transform) {
        case JXFORM_TRANSPOSE:
        case JXFORM_TRANSVERSE:
        case JXFORM_ROT_90:
        case JXFORM_ROT_270:
#if JPEG_LIB_VERSION < 70
            dstinfo->image_width = info->output_height;
            dstinfo->image_height = info->output_width;
#endif
            transpose_critical_parameters(dstinfo);
            break;
        default:
#if JPEG_LIB_VERSION < 70
            dstinfo->image_width = info->output_width;
            dstinfo->image_height = info->output_height;
#endif
            break;
    }

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
#if JPEG_LIB_VERSION >= 70
        /* Adjust Exif image parameters */
        if (dstinfo->jpeg_width != srcinfo->image_width ||
                dstinfo->jpeg_height != srcinfo->image_height)
            /* Align data segment to start of TIFF structure for parsing */
            adjust_exif_parameters(srcinfo->marker_list->data + 6, srcinfo->marker_list->data_length - 6, dstinfo->jpeg_width, dstinfo->jpeg_height);
#endif
    }

    /* Return the appropriate output data set */
    if (info->workspace_coef_arrays != NULL)
        return info->workspace_coef_arrays;
    return src_coef_arrays;
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

    GLOBAL(boolean)
jtransform_perfect_transform(JDIMENSION image_width, JDIMENSION image_height,
        int MCU_width, int MCU_height,
        JXFORM_CODE transform)
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

GLOBAL(boolean) keymatch (char *arg, const char *keyword, int minchars)
{
    register int ca, ck;
    register int nmatched = 0;

    while ((ca = *arg++) != '\0') {
        if ((ck = *keyword++) == '\0')
            return FALSE;		/* arg longer than keyword, no good */
        if (isupper(ca))		/* force arg to lcase (assume ck is already) */
            ca = tolower(ca);
        if (ca != ck)
            return FALSE;		/* no good */
        nmatched++;			/* count matched characters */
    }
    /* reached end of argument; fail if it's too short for unique abbrev */
    if (nmatched < minchars)
        return FALSE;
    return TRUE;			/* A-OK */
}

/* Routines to establish binary I/O mode for stdin and stdout.
 * Non-Unix systems often require some hacking to get out of text mode.  */

void printessens(struct jpeg_decompress_struct dcinfo)
{
    fprintf(stderr, "width: %i, height: %i inpcomps: %i inpcspace: %i\n", dcinfo.image_width, dcinfo.image_height, dcinfo.num_components, dcinfo.jpeg_color_space);
}

LOCAL(void) usage (void) /* complain about bad command line */
{
    fprintf(stderr, "usage: %s [switches] ", progname);
#ifdef TWO_FILE_COMMANDLINE
    fprintf(stderr, "inputfile outputfile\n");
#else
    fprintf(stderr, "[inputfile]\n");
#endif

    fprintf(stderr, "Switches (names may be abbreviated):\n");
    fprintf(stderr, "  -copy none     Copy no extra markers from source file\n");
    fprintf(stderr, "  -copy comments Copy only comment markers (default)\n");
    fprintf(stderr, "  -copy all      Copy all extra markers\n");
#ifdef ENTROPY_OPT_SUPPORTED
    fprintf(stderr, "  -optimize      Optimize Huffman table (smaller file, but slow compression)\n");
#endif
#ifdef C_PROGRESSIVE_SUPPORTED
    fprintf(stderr, "  -progressive   Create progressive JPEG file\n");
#endif
    fprintf(stderr, "Switches for modifying the image:\n");
#if TRANSFORMS_SUPPORTED
    fprintf(stderr, "  -crop WxH+X+Y  Crop to a rectangular subarea\n");
    fprintf(stderr, "  -grayscale     Reduce to grayscale (omit color data)\n");
    fprintf(stderr, "  -flip [horizontal|vertical]  Mirror image (left-right or top-bottom)\n");
    fprintf(stderr, "  -perfect       Fail if there is non-transformable edge blocks\n");
    fprintf(stderr, "  -rotate [90|180|270]         Rotate image (degrees clockwise)\n");
#endif
#if TRANSFORMS_SUPPORTED
    fprintf(stderr, "  -transpose     Transpose image\n");
    fprintf(stderr, "  -transverse    Transverse transpose image\n");
    fprintf(stderr, "  -trim          Drop non-transformable edge blocks\n");
#endif
    fprintf(stderr, "Switches for advanced users:\n");
    fprintf(stderr, "  -restart N     Set restart interval in rows, or in blocks with B\n");
    fprintf(stderr, "  -maxmemory N   Maximum memory to use (in kbytes)\n");
    fprintf(stderr, "  -outfile name  Specify name for output file\n");
    fprintf(stderr, "  -verbose  or  -debug   Emit debug output\n");
    fprintf(stderr, "Switches for wizards:\n");
#ifdef C_ARITH_CODING_SUPPORTED
    fprintf(stderr, "  -arithmetic    Use arithmetic coding\n");
#endif
    exit(EXIT_FAILURE);
}

LOCAL(void) select_transform (JXFORM_CODE transform) /* Silly little routine to detect multiple transform options, which we can't handle. */
{
#if TRANSFORMS_SUPPORTED
    if (transformoption.transform == JXFORM_NONE ||
            transformoption.transform == transform) {
        transformoption.transform = transform;
    } else {
        fprintf(stderr, "%s: can only do one image transformation at a time\n", progname);
        usage();
    }
#else
    fprintf(stderr, "%s: sorry, image transformation was not compiled\n",
            progname);
    exit(EXIT_FAILURE);
#endif
}

LOCAL(int) parse_switches (j_compress_ptr cinfo, int argc, char **argv, int last_file_arg_seen, boolean for_real)
{
    /* Parse optional switches.
     * Returns argv[] index of first file-name argument (== argc if none).
     * Any file names with indexes <= last_file_arg_seen are ignored;
     * they have presumably been processed in a previous iteration.
     * (Pass 0 for last_file_arg_seen on the first or only iteration.)
     * for_real is FALSE on the first (dummy) pass; we may skip any expensive
     * processing.  */
    int argn;
    char *arg;
    boolean simple_progressive;

    /* Set up default JPEG parameters. */
    simple_progressive = FALSE;
    outfilename = NULL;
    copyoption = JCOPYOPT_DEFAULT;
    transformoption.transform = JXFORM_NONE;
    transformoption.perfect = FALSE;
    transformoption.trim = FALSE;
    transformoption.force_grayscale = FALSE;
    transformoption.crop = FALSE;
    transformoption.slow_hflip = FALSE;
    cinfo->err->trace_level = 0;

    /* Scan command line options, adjust parameters */
    for (argn = 1; argn < argc; argn++) {
        arg = argv[argn];
        if (*arg != '-') {
            /* Not a switch, must be a file name argument */
            if (argn <= last_file_arg_seen) {
                outfilename = NULL;	/* -outfile applies to just one input file */
                continue;		/* ignore this name if previously processed */
            }
            break;			/* else done parsing switches */
        }
        arg++;			/* advance past switch marker character */

        if (keymatch(arg, "arithmetic", 1)) {
            /* Use arithmetic coding. */
#ifdef C_ARITH_CODING_SUPPORTED
            cinfo->arith_code = TRUE;
            fprintf(stderr, "cinfo->arith_code:%i\n", cinfo->arith_code);
#else
            fprintf(stderr, "%s: sorry, arithmetic coding not supported\n",
                    progname);
            exit(EXIT_FAILURE);
#endif

        } else if (keymatch(arg, "copy", 2)) {
            /* Select which extra markers to copy. */
            if (++argn >= argc)	/* advance to next argument */
                usage();
            if (keymatch(argv[argn], "none", 1)) {
                copyoption = JCOPYOPT_NONE;
            } else if (keymatch(argv[argn], "comments", 1)) {
                copyoption = JCOPYOPT_COMMENTS;
            } else if (keymatch(argv[argn], "all", 1)) {
                copyoption = JCOPYOPT_ALL;
            } else
                usage();

        } else if (keymatch(arg, "crop", 2)) {
            /* Perform lossless cropping. */
#if TRANSFORMS_SUPPORTED
            if (++argn >= argc)	/* advance to next argument */
                usage();
            if (! jtransform_parse_crop_spec(&transformoption, argv[argn])) {
                fprintf(stderr, "%s: bogus -crop argument '%s'\n",
                        progname, argv[argn]);
                exit(EXIT_FAILURE);
            }
#else
            select_transform(JXFORM_NONE);	/* force an error */
#endif

        } else if (keymatch(arg, "debug", 1) || keymatch(arg, "verbose", 1)) {
            /* Enable debug printouts. */
            /* On first -d, print version identification */
            static boolean printed_version = FALSE;

            if (! printed_version) {
                fprintf(stderr, "%s version %s (build %s)\n",
                        PACKAGE_NAME, VERSION, BUILD);
                fprintf(stderr, "%s\n\n", LJTCOPYRIGHT);
                fprintf(stderr, "Based on Independent JPEG Group's libjpeg, version %s\n%s\n\n",
                        JVERSION, JCOPYRIGHT);
                printed_version = TRUE;
            }
            cinfo->err->trace_level++;

        } else if (keymatch(arg, "flip", 1)) {
            /* Mirror left-right or top-bottom. */
            if (++argn >= argc)	/* advance to next argument */
                usage();
            if (keymatch(argv[argn], "horizontal", 1))
                select_transform(JXFORM_FLIP_H);
            else if (keymatch(argv[argn], "vertical", 1))
                select_transform(JXFORM_FLIP_V);
            else
                usage();

        } else if (keymatch(arg, "grayscale", 1) || keymatch(arg, "greyscale",1)) {
            /* Force to grayscale. */
#if TRANSFORMS_SUPPORTED
            transformoption.force_grayscale = TRUE;
#else
            select_transform(JXFORM_NONE);	/* force an error */
#endif

        } else if (keymatch(arg, "maxmemory", 3)) {
            /* Maximum memory in Kb (or Mb with 'm'). */
            long lval;
            char ch = 'x';

            if (++argn >= argc)	/* advance to next argument */
                usage();
            if (sscanf(argv[argn], "%ld%c", &lval, &ch) < 1)
                usage();
            if (ch == 'm' || ch == 'M')
                lval *= 1000L;
            cinfo->mem->max_memory_to_use = lval * 1000L;

        } else if (keymatch(arg, "optimize", 1) || keymatch(arg, "optimise", 1)) {
            /* Enable entropy parm optimization. */
#ifdef ENTROPY_OPT_SUPPORTED
            cinfo->optimize_coding = TRUE;
#else
            fprintf(stderr, "%s: sorry, entropy optimization was not compiled\n",
                    progname);
            exit(EXIT_FAILURE);
#endif

        } else if (keymatch(arg, "outfile", 4)) {
            /* Set output file name. */
            if (++argn >= argc)	/* advance to next argument */
                usage();
            outfilename = argv[argn];	/* save it away for later use */

        } else if (keymatch(arg, "perfect", 2)) {
            /* Fail if there is any partial edge MCUs that the transform can't
             * handle. */
            transformoption.perfect = TRUE;

        } else if (keymatch(arg, "progressive", 2)) {
            /* Select simple progressive mode. */
#ifdef C_PROGRESSIVE_SUPPORTED
            simple_progressive = TRUE;
            /* We must postpone execution until num_components is known. */
#else
            fprintf(stderr, "%s: sorry, progressive output was not compiled\n",
                    progname);
            exit(EXIT_FAILURE);
#endif

        } else if (keymatch(arg, "restart", 1)) {
            /* Restart interval in MCU rows (or in MCUs with 'b'). */
            long lval;
            char ch = 'x';

            if (++argn >= argc)	/* advance to next argument */
                usage();
            if (sscanf(argv[argn], "%ld%c", &lval, &ch) < 1)
                usage();
            if (lval < 0 || lval > 65535L)
                usage();
            if (ch == 'b' || ch == 'B') {
                cinfo->restart_interval = (unsigned int) lval;
                cinfo->restart_in_rows = 0; /* else prior '-restart n' overrides me */
            } else {
                cinfo->restart_in_rows = (int) lval;
                /* restart_interval will be computed during startup */
            }

        } else if (keymatch(arg, "rotate", 2)) {
            /* Rotate 90, 180, or 270 degrees (measured clockwise). */
            if (++argn >= argc)	/* advance to next argument */
                usage();
            if (keymatch(argv[argn], "90", 2))
                select_transform(JXFORM_ROT_90);
            else if (keymatch(argv[argn], "180", 3))
                select_transform(JXFORM_ROT_180);
            else if (keymatch(argv[argn], "270", 3))
                select_transform(JXFORM_ROT_270);
            else
                usage();

        } else if (keymatch(arg, "scans", 1)) {
            fprintf(stderr, "%s: sorry, multi-scan output was not compiled\n", progname);
            exit(EXIT_FAILURE);

        } else if (keymatch(arg, "transpose", 1)) {
            /* Transpose (across UL-to-LR axis). */
            select_transform(JXFORM_TRANSPOSE);

        } else if (keymatch(arg, "transverse", 6)) {
            /* Transverse transpose (across UR-to-LL axis). */
            select_transform(JXFORM_TRANSVERSE);

        } else if (keymatch(arg, "trim", 3)) {
            /* Trim off any partial edge MCUs that the transform can't handle. */
            transformoption.trim = TRUE;

        } else {
            usage();			/* bogus switch */
        }
    }

    /* Post-switch-scanning cleanup */

    if (for_real) {

#ifdef C_PROGRESSIVE_SUPPORTED
        if (simple_progressive)	/* process -progressive; -scans can override */
            jpeg_simple_progression(cinfo);
#endif

    }

    return argn;			/* return index of next arg (file name) */
}

int main (int argc, char **argv)
{
    struct jpeg_decompress_struct srcinfo;
    struct jpeg_compress_struct dstinfo;
    struct jpeg_error_mgr jsrcerr, jdsterr; /* two error structs, one for source "js*" and one for dest "jd*" */
    jvirt_barray_ptr *src_coef_arrays;
    jvirt_barray_ptr *dst_coef_arrays;
    int file_index;
    /* We assume all-in-memory processing and can therefore use only a
     * single file pointer for sequential input and output operation.  */
    FILE * fp;

    progname = argv[0];
    if (progname == NULL || progname[0] == 0)
        progname = "jpegtran";	/* in case C library doesn't provide it */

    /* Error handling important in libjpeg (probably good practice too).i
     * Initialize the JPEG decompression object with default error handling. */
    srcinfo.err = jpeg_std_error(&jsrcerr);
    jpeg_create_decompress(&srcinfo);
    /* Initialize the JPEG compression object with default error handling. */
    dstinfo.err = jpeg_std_error(&jdsterr);
    jpeg_create_compress(&dstinfo);

    /* Now safe to enable signal catcher.
     * Note: we assume only the decompression object will have virtual arrays. */
#ifdef NEED_SIGNAL_CATCHER
    enable_signal_catcher((j_common_ptr) &srcinfo);
#endif

    /* Scan command line to find file names.
     * It is convenient to use just one switch-parsing routine, but the switch
     * values read here are mostly ignored; we will rescan the switches after
     * opening the input file.  Also note that most of the switches affect the
     * destination JPEG object, so we parse into that and then copy over what
     * needs to affects the source too.  */
    file_index = parse_switches(&dstinfo, argc, argv, 0, FALSE);
    jsrcerr.trace_level = jdsterr.trace_level;
    srcinfo.mem->max_memory_to_use = dstinfo.mem->max_memory_to_use;

#ifdef TWO_FILE_COMMANDLINE
    /* Must have either -outfile switch or explicit output file name */
    if (outfilename == NULL) {
        if (file_index != argc-2) {
            fprintf(stderr, "%s: must name one input and one output file\n",
                    progname);
            usage();
        }
        outfilename = argv[file_index+1];
    } else {
        if (file_index != argc-1) {
            fprintf(stderr, "%s: must name one input and one output file\n",
                    progname);
            usage();
        }
    }
#else
    /* Unix style: expect zero or one file name */
    if (file_index < argc-1) {
        fprintf(stderr, "%s: only one input file\n", progname);
        usage();
    }
#endif /* TWO_FILE_COMMANDLINE */

    /* Open the input file. */
    if (file_index < argc) {
        if ((fp = fopen(argv[file_index], READ_BINARY)) == NULL) {
            fprintf(stderr, "%s: can't open %s for reading\n", progname, argv[file_index]);
            exit(EXIT_FAILURE);
        }
    } else {
        /* default input file is stdin */
        fp = read_stdin();
    }

    /* Specify data source for decompression */
    jpeg_stdio_src(&srcinfo, fp);

    /* Enable saving of extra markers that we want to copy */
    jcopy_markers_setup(&srcinfo, copyoption);

    /* Read file header */
    (void) jpeg_read_header(&srcinfo, TRUE);
    printessens(srcinfo); /* have a look */

    /* Any space needed by a transform option must be requested before
     * jpeg_read_coefficients so that memory allocation will be done right.  */
#if TRANSFORMS_SUPPORTED
    /* Fail right away if -perfect is given and transformation is not perfect.  */
    if (!jtransform_request_workspace(&srcinfo, &transformoption)) {
        fprintf(stderr, "%s: transformation is not perfect\n", progname);
        exit(EXIT_FAILURE);
    }
#endif

    /* Read source file as DCT coefficients */
    src_coef_arrays = jpeg_read_coefficients(&srcinfo);

    /* Initialize destination compression parameters from source values */
    jpeg_copy_critical_parameters(&srcinfo, &dstinfo);

    /* Adjust destination parameters if required by transform options;
     * also find out which set of coefficient arrays will hold the output.  */
#if TRANSFORMS_SUPPORTED
    dst_coef_arrays = jtransform_adjust_parameters(&srcinfo, &dstinfo, src_coef_arrays, &transformoption);
#else
    dst_coef_arrays = src_coef_arrays;
#endif

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
    if (outfilename != NULL) {
        if ((fp = fopen(outfilename, WRITE_BINARY)) == NULL) {
            fprintf(stderr, "%s: can't open %s for writing\n", progname, outfilename);
            exit(EXIT_FAILURE);
        }
    } else {
        /* default output file is stdout */
        fp = write_stdout();
    }

    /* Adjust default compression parameters by re-parsing the options */
    file_index = parse_switches(&dstinfo, argc, argv, 0, TRUE);

    /* Specify data destination for compression */
    jpeg_stdio_dest(&dstinfo, fp);

    /* Start compressor (note no image data is actually written here) */
    jpeg_write_coefficients(&dstinfo, dst_coef_arrays);

    /* Copy to the output file any extra markers that we want to preserve */
    jcopy_markers_execute(&srcinfo, &dstinfo, copyoption);

    /* Execute image transformation, if any */
#if TRANSFORMS_SUPPORTED
    // jtransform_execute_transformation(&srcinfo, &dstinfo, src_coef_arrays, &transformoption);
    jvirt_barray_ptr *dst_coef_arrays2 = transformoption.workspace_coef_arrays;
    do_crop(&srcinfo, &dstinfo, transformoption.x_crop_offset, transformoption.y_crop_offset, src_coef_arrays, dst_coef_arrays2);
#endif

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
