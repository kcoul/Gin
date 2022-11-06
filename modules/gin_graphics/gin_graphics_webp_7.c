/*==============================================================================

 Copyright 2018 by Roland Rabien
 For more information visit www.rabiensoftware.c"om

 ==============================================================================*/

#include "webp_warnings.h"

#include "3rdparty/webp/dsp/upsampling_msa.c"
#include "3rdparty/webp/utils/random_utils.c"
#include "3rdparty/webp/dsp/alpha_processing_sse41.c"
#include "3rdparty/webp/dsp/upsampling_sse41.c"
#include "3rdparty/webp/enc/tree_enc.c"
#include "3rdparty/webp/dsp/cost_neon.c"
#include "3rdparty/webp/sharpyuv/sharpyuv_gamma.c"
