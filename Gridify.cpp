#include "Gridify.h"
#include "opencv2/opencv.hpp"
#include "opencv2/nonfree/features2d.hpp"

#pragma comment(lib, "opencv_core2412.lib")
#pragma comment(lib, "opencv_features2d2412.lib")
#pragma comment(lib, "opencv_imgproc2412.lib")
#pragma comment(lib, "opencv_nonfree2412.lib")

using namespace cv;

static PF_Err ParamsSetup (PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output)
{
	PF_Err		err		= PF_Err_NONE;
	PF_ParamDef	def;	

	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Threshold",
		0, 		// MIN
		90000,	// MAX
		0,		// MIN
		90000, 	// MAX
		500,	// Default 500 - 4000
		Threshold_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Octaves",
		1, 		// MIN
		16,		// MAX
		1,		// MIN
		16, 	// MAX
		2,		// Default
		Octaves_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Layers",
		1, 		// MIN
		16,		// MAX
		1,		// MIN
		16, 	// MAX
		4,		// Default
		Layers_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX("Size multiply",
		0, 		// MIN
		64,		// MAX
		0,		// MIN
		64, 	// MAX
		1,		// Default
		PF_Precision_HUNDREDTHS,
		0,
		0,
		Sizemul_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Grid Size X",
		1, 		// MIN
		64,		// MAX
		1,		// MIN
		64, 	// MAX
		12,		// Default
		GridX_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Grid Size Y",
		1, 		// MIN
		64,		// MAX
		1,		// MIN
		64, 	// MAX
		12,		// Default
		GridY_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Margin",
		0, 		// MIN
		64,		// MAX
		0,		// MIN
		64, 	// MAX
		2,		// Default
		Margin_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Thickness",
		1, 		// MIN
		32,		// MAX
		1,		// MIN
		32, 	// MAX
		1,		// Default
		Thickness_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_CHECKBOX("Fill",
		"",			// Checkbox
		TRUE,		// Default
		0,
		Fill_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_CHECKBOX("Gridify",
		"",			// Checkbox
		TRUE,		// Default
		0,
		Gridify_DISK_ID);

	out_data->num_params = GRIDIFY_NUM_PARAMS;
	return err;
}

static PF_Err Render( PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output)
{
	PF_Err				err		= PF_Err_NONE;
	AEGP_SuiteHandler	suites(in_data->pica_basicP);
	PF_EffectWorld *input = &params[0]->u.ld;
	PF_COPY(input, output, NULL, NULL);

	A_long width = output->width;
	A_long height = output->height;
	A_long outWidth = output->rowbytes / sizeof(PF_Pixel);
	PF_Pixel *AEFrameData = (PF_Pixel *)output->data;


	/* PF_Pixel > CvMat */
	Mat src = Mat(Size(outWidth, height), CV_8UC4);		// 8bit BGRA
	for (int y = 0; y < src.rows; ++y) {
		for (int x = 0; x < src.cols; ++x) {
			src.data[y * src.step + x * src.elemSize() + 0] = AEFrameData[x + y * outWidth].blue;
			src.data[y * src.step + x * src.elemSize() + 1] = AEFrameData[x + y * outWidth].green;
			src.data[y * src.step + x * src.elemSize() + 2] = AEFrameData[x + y * outWidth].red;
			src.data[y * src.step + x * src.elemSize() + 3] = AEFrameData[x + y * outWidth].alpha;
		}
	}
	
	/* OpenCV SURF */
	Mat gray_img;
	cvtColor(src, gray_img, CV_BGR2GRAY);
	normalize(gray_img, gray_img, 0, 255, cv::NORM_MINMAX);

	vector<KeyPoint> keypoints;
	vector<KeyPoint>::iterator it;

	SurfFeatureDetector surf(params[Param_Threshold]->u.sd.value, params[Param_Octaves]->u.sd.value, params[Param_Layers]->u.sd.value);
	surf.detect(gray_img, keypoints);
	Vec4b srcColor = Vec4d(0, 0, 0, 0);

	int cvwidth = src.size().width;
	int cvheight = src.size().height;
	int grid_x = params[Param_GridX]->u.sd.value;
	int grid_y = params[Param_GridY]->u.sd.value;
	int step_x = (int)(cvwidth / grid_x);
	int step_y = (int)(cvheight / grid_y); 
	int margin = params[Param_Margin]->u.sd.value + 1;
	int thickness = params[Param_Thickness]->u.sd.value;
	bool fill = (PF_Boolean)params[Param_Fill]->u.sd.value;
	bool gridify = (PF_Boolean)params[Param_Gridify]->u.sd.value;

	/* gridify */
	Mat canvas = cv::Mat(src.size(), CV_8UC4);
	canvas = cv::Scalar(0, 0, 0, 0);

	for (it = keypoints.begin(); it != keypoints.end(); ++it) {
		Point2f point = it->pt;
		int size = (int)(it->size / 1.2 * params[Param_Sizemul]->u.fs_d.value);

		srcColor = src.at<Vec4b>(point);
		Scalar color = Scalar(srcColor[0], srcColor[1], srcColor[2], 255);

		if(size > 0){
			// Centering
			point.x -= size / 2;
			point.y -= size / 2;
			
			if(gridify){
				// Gridify Enabled
				Point gridfy_pt;
				Point gridfy_pt2;
				for (int step = 0; step < step_x; ++step) {
					if(point.x >= grid_x * step && point.x <= grid_x * (step+1)){
						gridfy_pt.x = grid_x * step;
						gridfy_pt2.x = grid_x * step + ((size / grid_x) * grid_x) - margin;
						break;
					}
				}
				for (int step = 0; step < step_y; ++step) {
					if(point.y >= grid_y * step && point.y <= grid_y * (step+1)){
						gridfy_pt.y = grid_y * step;
						gridfy_pt2.y = grid_y * step + ((size / grid_y) * grid_y) - margin;
						break;
					}
				}

				if(gridfy_pt.x < gridfy_pt2.x && gridfy_pt2.x < cvwidth && gridfy_pt2.y < cvheight){
					if(canvas.at<Vec4b>(gridfy_pt)[3] == 0 && canvas.at<Vec4b>(gridfy_pt2)[3] == 0 && canvas.at<Vec4b>(Point(gridfy_pt2.x, gridfy_pt.y))[3] == 0 && canvas.at<Vec4b>(Point(gridfy_pt.x, gridfy_pt2.y))[3] == 0){
						if(fill)
							rectangle(canvas, gridfy_pt, gridfy_pt2, color, CV_FILLED);
						else
							rectangle(canvas, gridfy_pt, gridfy_pt2, color, thickness);
					}
				}
			}else{
				// Gridify Disabled
				Point point2 = point;

				point2.x += size;
				point2.y += size;

				if(point.x < point2.x && point2.x < cvwidth && point2.y < cvheight){
						if(fill)
							rectangle(canvas, point, point2, color, CV_FILLED);
						else
							rectangle(canvas, point, point2, color, thickness);
				}
			}
		}
	}


	/* CvMat > PF_Pixel */
	for (int y = 0; y < canvas.rows; ++y) {
		for (int x = 0; x < canvas.cols; ++x) {
			AEFrameData[x + y * outWidth].blue	=	canvas.data[y * canvas.step + x * canvas.elemSize() + 0];
			AEFrameData[x + y * outWidth].green	=	canvas.data[y * canvas.step + x * canvas.elemSize() + 1];
			AEFrameData[x + y * outWidth].red	=	canvas.data[y * canvas.step + x * canvas.elemSize() + 2];
			AEFrameData[x + y * outWidth].alpha	=	canvas.data[y * canvas.step + x * canvas.elemSize() + 3];
		}
	}

	return err;
}

static PF_Err About(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output)
{
	AEGP_SuiteHandler suites(in_data->pica_basicP);

	suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg,
		"%s v%d.%d\r%s",
		STR(StrID_Name),
		MAJOR_VERSION,
		MINOR_VERSION,
		STR(StrID_Description));
	return PF_Err_NONE;
}

static PF_Err GlobalSetup(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output)
{
	out_data->my_version = PF_VERSION(MAJOR_VERSION,
		MINOR_VERSION,
		BUG_VERSION,
		STAGE_VERSION,
		BUILD_VERSION);

	//out_data->out_flags = PF_OutFlag_DEEP_COLOR_AWARE;

	return PF_Err_NONE;
}

DllExport PF_Err EntryPointFunc(PF_Cmd cmd, PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output, void *extra)
{
	PF_Err err = PF_Err_NONE;
	
	try {
		switch (cmd) {
			case PF_Cmd_ABOUT:

				err = About(in_data,
							out_data,
							params,
							output);
				break;
				
			case PF_Cmd_GLOBAL_SETUP:

				err = GlobalSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_PARAMS_SETUP:

				err = ParamsSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_RENDER:

				err = Render(	in_data,
								out_data,
								params,
								output);
				break;
		}
	}
	catch(PF_Err &thrown_err){
		err = thrown_err;
	}
	return err;
}

