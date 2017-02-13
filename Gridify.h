#pragma once

#ifndef GRIDIFY_H
#define GRIDIFY_H

typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned short		u_int16;
typedef unsigned long		u_long;
typedef short int			int16;
#define PF_TABLE_BITS	12
#define PF_TABLE_SZ_16	4096

#define PF_DEEP_COLOR_AWARE 1	// make sure we get 16bpc pixels; 
								// AE_Effect.h checks for this.

#include "AEConfig.h"

#ifdef AE_OS_WIN
	typedef unsigned short PixelType;
	#include <Windows.h>
#endif

#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"
#include "AEFX_ChannelDepthTpl.h"
#include "AEGP_SuiteHandler.h"

#include "Gridify_Strings.h"

/* Versioning information */

#define	MAJOR_VERSION	1
#define	MINOR_VERSION	1
#define	BUG_VERSION		0
#define	STAGE_VERSION	PF_Stage_RELEASE
#define	BUILD_VERSION	1


/* Parameter defaults */

enum {
	GRIDIFY_INPUT = 0,
	Param_Threshold,
	Param_Octaves,
	Param_Layers,
	Param_Sizemul,
	Param_GridX,
	Param_GridY,
	Param_Margin,
	Param_Thickness,
	Param_Fill,
	Param_Gridify,
	GRIDIFY_NUM_PARAMS
};

enum {
	Threshold_DISK_ID = 1,
	Octaves_DISK_ID,
	Layers_DISK_ID,
	Sizemul_DISK_ID,
	GridX_DISK_ID,
	GridY_DISK_ID,
	Margin_DISK_ID,
	Thickness_DISK_ID,
	Fill_DISK_ID,
	Gridify_DISK_ID,
};


#ifdef __cplusplus
	extern "C" {
#endif
	
DllExport	PF_Err 
EntryPointFunc(	
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra) ;

#ifdef __cplusplus
}
#endif

#endif // GRIDIFY_H