#include "jinclude.h"
#include "jpeglib.h"
#include "transupp.h"		/* My own external interface */
#include "jpegcomp.h"
#include <ctype.h>		/* to declare isdigit() */


#if JPEG_LIB_VERSION >= 70
#define dstinfo_min_DCT_h_scaled_size dstinfo->min_DCT_h_scaled_size
#define dstinfo_min_DCT_v_scaled_size dstinfo->min_DCT_v_scaled_size
#else
#define dstinfo_min_DCT_h_scaled_size DCTSIZE
#define dstinfo_min_DCT_v_scaled_size DCTSIZE
#endif


/*
 * Lossless image transformation routines.  These routines work on DCT
 * coefficient arrays and thus do not require any lossy decompression
 * or recompression of the image.
 * Thanks to Guido Vollbeding for the initial design and code of this feature,
 * and to Ben Jackson for introducing the cropping feature.
 *
 * Horizontal flipping is done in-place, using a single top-to-bottom
 * pass through the virtual source array.  It will thus be much the
 * fastest option for images larger than main memory.
 *
 * The other routines require a set of destination virtual arrays, so they
 * need twice as much memory as jpegtran normally does.  The destination
 * arrays are always written in normal scan order (top to bottom) because
 * the virtual array manager expects this.  The source arrays will be scanned
 * in the corresponding order, which means multiple passes through the source
 * arrays for most of the transforms.  That could result in much thrashing
 * if the image is larger than main memory.
 *
 * If cropping or trimming is involved, the destination arrays may be smaller
 * than the source arrays.  Note it is not possible to do horizontal flip
 * in-place when a nonzero Y crop offset is specified, since we'd have to move
 * data from one block row to another but the virtual array manager doesn't
 * guarantee we can touch more than one row at a time.  So in that case,
 * we have to use a separate destination array.
 *
 * Some notes about the operating environment of the individual transform
 * routines:
 * 1. Both the source and destination virtual arrays are allocated from the
 *    source JPEG object, and therefore should be manipulated by calling the
 *    source's memory manager.
 * 2. The destination's component count should be used.  It may be smaller
 *    than the source's when forcing to grayscale.
 * 3. Likewise the destination's sampling factors should be used.  When
 *    forcing to grayscale the destination's sampling factors will be all 1,
 *    and we may as well take that as the effective iMCU size.
 * 4. When "trim" is in effect, the destination's dimensions will be the
 *    trimmed values but the source's will be untrimmed.
 * 5. When "crop" is in effect, the destination's dimensions will be the
 *    cropped values but the source's will be uncropped.  Each transform
 *    routine is responsible for picking up source data starting at the
 *    correct X and Y offset for the crop region.  (The X and Y offsets
 *    passed to the transform routines are measured in iMCU blocks of the
 *    destination.)
 * 6. All the routines assume that the source and destination buffers are
 *    padded out to a full iMCU boundary.  This is true, although for the
 *    source buffer it is an undocumented property of jdcoefct.c.
 */

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

GLOBAL(boolean)
jtransform_request_workspace (j_decompress_ptr srcinfo,
			      jpeg_transform_info *info)
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
LOCAL(boolean)
jt_read_integer (const char ** strptr, JDIMENSION * result)
{
  const char * ptr = *strptr;
  JDIMENSION val = 0;

  for (; isdigit(*ptr); ptr++) {
    val = val * 10 + (JDIMENSION) (*ptr - '0');
  }
  *result = val;
  if (ptr == *strptr)
    return FALSE;		/* oops, no digits */
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

GLOBAL(boolean)
jtransform_parse_crop_spec (jpeg_transform_info *info, const char *spec)
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
