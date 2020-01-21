/*
 * transupp.h
 *
 * Copyright (C) 1997-2009, Thomas G. Lane, Guido Vollbeding.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains declarations for image transformation routines and
 * other utility code used by the jpegtran sample application.  These are
 * NOT part of the core JPEG library.  But we keep these routines separate
 * from jpegtran.c to ease the task of maintaining jpegtran-like programs
 * that have other user interfaces.
 *
 * NOTE: all the routines declared here have very specific requirements
 * about when they are to be executed during the reading and writing of the
 * source and destination files.  See the comments in transupp.c, or see
 * jpegtran.c for an example of correct usage.
 */

/* If you happen not to want the image transform support, disable it here */
#ifndef TRANSFORMS_SUPPORTED
#define TRANSFORMS_SUPPORTED 1		/* 0 disables transform code */
#endif

/*
 * Although rotating and flipping data expressed as DCT coefficients is not
 * hard, there is an asymmetry in the JPEG format specification for images
 * whose dimensions aren't multiples of the iMCU size.  The right and bottom
 * image edges are padded out to the next iMCU boundary with junk data; but
 * no padding is possible at the top and left edges.  If we were to flip
 * the whole image including the pad data, then pad garbage would become
 * visible at the top and/or left, and real pixels would disappear into the
 * pad margins --- perhaps permanently, since encoders & decoders may not
 * bother to preserve DCT blocks that appear to be completely outside the
 * nominal image area.  So, we have to exclude any partial iMCUs from the
 * basic transformation.
 *
 * Transpose is the only transformation that can handle partial iMCUs at the
 * right and bottom edges completely cleanly.  flip_h can flip partial iMCUs
 * at the bottom, but leaves any partial iMCUs at the right edge untouched.
 * Similarly flip_v leaves any partial iMCUs at the bottom edge untouched.
 * The other transforms are defined as combinations of these basic transforms
 * and process edge blocks in a way that preserves the equivalence.
 *
 * The "trim" option causes untransformable partial iMCUs to be dropped;
 * this is not strictly lossless, but it usually gives the best-looking
 * result for odd-size images.  Note that when this option is active,
 * the expected mathematical equivalences between the transforms may not hold.
 * (For example, -rot 270 -trim trims only the bottom edge, but -rot 90 -trim
 * followed by -rot 180 -trim trims both edges.)
 *
 * We also offer a lossless-crop option, which discards data outside a given
 * image region but losslessly preserves what is inside.  Like the rotate and
 * flip transforms, lossless crop is restricted by the JPEG format: the upper
 * left corner of the selected region must fall on an iMCU boundary.  If this
 * does not hold for the given crop parameters, we silently move the upper left
 * corner up and/or left to make it so, simultaneously increasing the region
 * dimensions to keep the lower right crop corner unchanged.  (Thus, the
 * output image covers at least the requested region, but may cover more.)
 *
 * We also provide a lossless-resize option, which is kind of a lossless-crop
 * operation in the DCT coefficient block domain - it discards higher-order
 * coefficients and losslessly preserves lower-order coefficients of a
 * sub-block.
 *
 * Rotate/flip transform, resize, and crop can be requested together in a
 * single invocation.  The crop is applied last --- that is, the crop region
 * is specified in terms of the destination image after transform/resize.
 *
 * We also offer a "force to grayscale" option, which simply discards the
 * chrominance channels of a YCbCr image.  This is lossless in the sense that
 * the luminance channel is preserved exactly.  It's not the same kind of
 * thing as the rotate/flip transformations, but it's convenient to handle it
 * as part of this package, mainly because the transformation routines have to
 * be aware of the option to know how many components to work on.
 */


/* Short forms of external names for systems with brain-damaged linkers. */

#ifdef NEED_SHORT_EXTERNAL_NAMES
#define jtransform_parse_crop_spec	jTrParCrop
#define jtransform_request_workspace	jTrRequest
#define jtransform_adjust_parameters	jTrAdjust
#define jtransform_execute_transform	jTrExec
#define jtransform_perfect_transform	jTrPerfect
#define jcopy_markers_setup		jCMrkSetup
#define jcopy_markers_execute		jCMrkExec
#endif /* NEED_SHORT_EXTERNAL_NAMES */
#if TRANSFORMS_SUPPORTED

/* Parse a crop specification (written in X11 geometry style) */
EXTERN(boolean) jtransform_parse_crop_spec JPP((jpeg_transform_info *info, const char *spec));
/* Request any required workspace */
EXTERN(boolean) jtransform_request_workspace
	JPP((j_decompress_ptr srcinfo, jpeg_transform_info *info));
/* Adjust output image parameters */
EXTERN(jvirt_barray_ptr *) jtransform_adjust_parameters
	JPP((j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
	     jvirt_barray_ptr *src_coef_arrays,
	     jpeg_transform_info *info));
/* Execute the actual transformation, if any */
EXTERN(void) jtransform_execute_transform
	JPP((j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
	     jvirt_barray_ptr *src_coef_arrays,
	     jpeg_transform_info *info));
/* Determine whether lossless transformation is perfectly
 * possible for a specified image and transformation.
 */
EXTERN(boolean) jtransform_perfect_transform
	JPP((JDIMENSION image_width, JDIMENSION image_height,
	     int MCU_width, int MCU_height,
	     JXFORM_CODE transform));

/* jtransform_execute_transform used to be called
 * jtransform_execute_transformation, but some compilers complain about
 * routine names that long.  This macro is here to avoid breaking any
 * old source code that uses the original name...
 */
#define jtransform_execute_transformation	jtransform_execute_transform

#endif /* TRANSFORMS_SUPPORTED */
